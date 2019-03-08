/**
 * \file ODBC library class implementation
 *
 * @author lotharyx
 */


#include "class_odbc_library.h"
#include <class_library.h>
#include <class_libentry.h>
#include <legacy_part_serializer.h>

#include <sqlext.h>
#include <richio.h>
#include <sstream>
#include <macros.h>
#include <time.h>
#include <sys/time.h>

#ifdef NDEBUG
#ifndef WXDEBUG
#error Please enable WXDEBUG
#endif
#endif

// also defined in legacy_part_serializer.cpp
static bool strCompare( const char* aString, const char* aLine, const char** aOutput = NULL )
{
    size_t len = strlen( aString );
    bool retv = ( strncasecmp( aLine, aString, len ) == 0 ) &&
                ( isspace( aLine[ len ] ) || aLine[ len ] == 0 );

    if( retv && aOutput )
    {
        const char* tmp = aLine;

        // Move past the end of the token.
        tmp += len;

        // Move to the beginning of the next token.
        while( *tmp && isspace( *tmp ) )
            tmp++;

        *aOutput = tmp;
    }

    return retv;
}


ENABLE_ODBC_TRACE static_tracer;
#define traceMask "MyODBC_trace"

#define duplicate_name_msg  \
    _(  "Library '%s' has duplicate entry name '%s'.\n" \
        "This may cause some unexpected behavior when loading components into a schematic." )
    
ENABLE_ODBC_TRACE::ENABLE_ODBC_TRACE() {
    wxLogDebug("Adding ODBC trace mask");
    wxLog::AddTraceMask(wxT(traceMask));
    wxLogTrace(traceMask, "ODBC trace mask added.");
}

std::map<wxString, ODBC_SCH_PLUGIN::READER_PTR> ODBC_SCH_PLUGIN::m_libs_by_path;
std::map<wxString, ODBC_SCH_PLUGIN::READER_PTR> ODBC_SCH_PLUGIN::m_libs_by_name;


/******************************************************************
 * ODBC_SCH_READER class, internal to this plugin 
 * ***************************************************************/

class ODBC_SCH_READER {
private:
    HENV        m_henv;
    HDBC        m_hdbc;
    HSTMT       m_hstmt;
    wxString    m_connection_string;
    wxString    m_file_path;
    mutable wxString    m_last_error_string;
    LIB_ALIAS_MAP       m_aliases;
    LIB_ALIAS_MAP       m_db_names;                 ///< For handling DB primary keys that contain illegal name characters
    std::map<wxString, wxString> m_name_map;        ///< For mapping sanitized names back to their unsanitary DB names
    SQL_TIMESTAMP_STRUCT   m_last_refresh_stamp;    ///< The freshest actual source timestamp
    timeval     m_last_refresh_check;               ///< The moment we last checked if our data is fresh
    time_t      m_source_check_interval;            ///< How often we're allowed to query the source for fresh data
    bool        m_loaded;
    
    bool hasXRef( const wxString & aSanitized ) 
    {
        return m_name_map.find( aSanitized ) != m_name_map.end();
    }
    
    /**
     * Gets the unsanitary DB key for a given sanitized alias name
     */
    const wxString & getXRef( const wxString & aSanitizedName ) 
    {
        try 
        {
            return m_name_map.at( aSanitizedName );
        } 
        catch ( std::out_of_range & ) 
        {
            return aSanitizedName;
        }
    }
    
    /**
     * Gets the sanitized alias name for a given unsanitary DB key
     */
    const wxString & getDeXRef( const wxString & aDBkey )
    {
        if(m_db_names.find( aDBkey ) == m_db_names.end() )
            return aDBkey;
        return m_db_names.at( aDBkey )->GetName();
    }
    
    void addXRef( const wxString & aSanitized, const wxString & aDBkey ) 
    {
        if( m_aliases.find( aSanitized ) == m_aliases.end() ) 
        {
            wxLogTrace( traceMask, "No alias found for sanitized name %s", aSanitized );
        } 
        else 
        {
            LIB_ALIAS * alias = m_aliases.at( aSanitized );
            if( m_name_map.find( aSanitized ) == m_name_map.end() ) {
                m_db_names[aDBkey] = alias;
                m_name_map[aSanitized] = aDBkey;
            } else {
                wxLogTrace( traceMask, "addXRef: Duplicate sanitized name: %s", aSanitized );
            }
        }
    }
    
    void delXRef( const wxString & aSanitized ) 
    {
        if( m_name_map.find( aSanitized ) != m_name_map.end() ) 
        {
            wxString dbkey = m_name_map.at( aSanitized );
            m_db_names.erase( dbkey );
            m_name_map.erase( aSanitized );
        }
        else
        {
            wxLogTrace( traceMask, "delXRef: No entry for sanitized name %s", aSanitized );
        }
    }
    /**
     * Section: ODBC wrapper functions
     * These methods handle error reporting by querying ODBC for all errors and concatenating
     * any errors into a string passed to THROW_IO_ERROR
     */
    /// \brief Reads the config file and populates member variables
    void readInit() 
    {
        FILE* file;

        file = wxFopen( m_file_path, wxT( "rt" ) );

        if( file != 0 )
        {
            FILE_LINE_READER reader( file, m_file_path );

            if( !reader.ReadLine() )
            {
                THROW_IO_ERROR( std::string( "ODBC Library: Config file is empty!" ) );
            }

            // First line can be ignored; it's our header.
            // The next line is the connection string.
            m_connection_string = reader.ReadLine();
            m_connection_string.Trim();
            // and that's it!
        }
        else
        {
            THROW_IO_ERROR( std::string( "ODBC Library: Config file not found." ) );
        }
    }
    /// \brief Allocates ODBC resources and establishes a connection to an RDBMS
    void connectAndGetStatementHandle() 
    {
        SQLRETURN rc = SQLAllocHandle( SQL_HANDLE_ENV, SQL_NULL_HANDLE, &m_henv );

        if( rc != SQL_SUCCESS )
            THROW_IO_ERROR( wxString( "ODBC Library: Could not intialize ODBC system." ) );

        rc = SQLSetEnvAttr( m_henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER) SQL_OV_ODBC3, 0 );

        if( rc != SQL_SUCCESS )
        {
            disconnectAndCleanUp();
            THROW_IO_ERROR( wxString( "ODBC Library: Required ODBCv3 not available." ) );
        }

        rc = SQLAllocHandle( SQL_HANDLE_DBC, m_henv, &m_hdbc );

        if( rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO )
        {
            disconnectAndCleanUp();
            THROW_IO_ERROR( wxString( "ODBC Library: No database connection handle available." ) );
        }

        rc = SQLDriverConnectA(
                m_hdbc,
                NULL,
                (SQLCHAR*) (char*) m_connection_string.char_str(),
                m_connection_string.length(),
                NULL,
                0,
                NULL,
                SQL_DRIVER_NOPROMPT );

        if( rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO )
        {
            wxString errors = getAllODBCErrors();
            disconnectAndCleanUp();
            errors.Prepend( "ODBC Library: Connection failed.  Details:\n" );
            THROW_IO_ERROR( errors );
        }

        rc = SQLAllocHandle( SQL_HANDLE_STMT, m_hdbc, &m_hstmt );

        if( rc != SQL_SUCCESS )
        {
            wxString errors = getAllODBCErrors();
            disconnectAndCleanUp();
            errors.Prepend( "ODBC Library: No statement handles available.  Details:\n" );
            THROW_IO_ERROR( errors );
        }
    }
    /// \brief Disconnects from an RDBMS and frees all ODBC resources
    /// \details Called by destructor; do not make virtual.
    void disconnectAndCleanUp() 
    {
        SQLFreeHandle( SQL_HANDLE_STMT, m_hstmt );
        m_hstmt = 0;
        SQLDisconnect( m_hdbc );
        SQLFreeHandle( SQL_HANDLE_DBC, m_hdbc );
        m_hdbc = 0;
        SQLFreeHandle( SQL_HANDLE_ENV, m_henv );
        m_henv = 0;
    }
    
    /// \brief Concatenates any and all active ODBC error messages to a string result
    wxString getAllODBCErrors() const 
    {
        wxString errors;

        if( m_hstmt != 0 )
            errors += getODBCErrors( SQL_HANDLE_STMT, m_hstmt );

        if( m_hdbc != 0 )
            errors += getODBCErrors( SQL_HANDLE_DBC, m_hdbc );

        if( m_henv != 0 )
            errors += getODBCErrors( SQL_HANDLE_ENV, m_henv );

        return errors;

    }
    
    /// \brief Concatenates all active ODBC error messages for a specific handle
    wxString getODBCErrors(SQLSMALLINT handle_type, SQLHANDLE handle) const 
    {
        std::stringstream errors;
        SQLCHAR sqlstate[6] = { 0, 0, 0, 0, 0, 0 };
        SQLINTEGER native_error = 0;
        std::auto_ptr<SQLCHAR> message;
        SQLSMALLINT message_length = 0;
        SQLRETURN   r = 0;

        for( int rec = 1; r == SQL_SUCCESS; ++rec )
        {
            r = SQLGetDiagRecA(
                    handle_type,
                    handle,
                    rec,
                    sqlstate,
                    &native_error,
                    NULL,
                    0,
                    &message_length );

            if( r == SQL_SUCCESS || r == SQL_SUCCESS_WITH_INFO )
            {
                message.reset( new SQLCHAR[message_length + 1] );
                r = SQLGetDiagRecA(
                        handle_type,
                        handle,
                        rec,
                        sqlstate,
                        &native_error,
                        message.get(),
                        message_length + 1,
                        &message_length );

                if( r == SQL_SUCCESS )
                {
                    errors << "SQLSTATE: " << (char*) sqlstate << std::endl;
                    errors << "Native error: " << native_error << std::endl;
                    errors << "Message: " << (char*) message.get() << std::endl;
                }
            }
        }

        return errors.str();

    }
    
    void load() 
    {
        if( !m_loaded || refreshNeeded() )
        {
            discardCache();
            bool had_errors = false;
            wxLogTrace(traceMask, "ODBC_SCH_READER(%s)::load", m_file_path);
            connectAndGetStatementHandle();
            
            // Note: For now (very early development following EESCHEMA's recent sweeping library changes),
            // we will just load names and not actual symbols, just to see what EESCHEMA's behavior versus
            // the SCH_PLUGIN api is.
            
            wxString query =
                "SELECT `name`, `data` FROM  `symbols` s1 "
                "WHERE `revision_stamp` = ("
                "SELECT MAX(`revision_stamp`) "
                "from `symbols` s2 where `s1`.`name` = `s2`.`name`) "
                "ORDER BY `name` ";
            SQLRETURN rc = SQLExecDirectA( m_hstmt, (SQLCHAR*) (char*) query.char_str(), query.length() );

            if( rc == SQL_SUCCESS )
            {
                SQLCHAR symbol_name[256];           // SQL_C_CHAR
                SQLCHAR symbol_data_buffer[255];
                rc = SQLBindCol( m_hstmt, 1, SQL_C_CHAR, &symbol_name, 256, 0 );
                
                if( rc == SQL_SUCCESS )
                {
                    while( (rc = SQLFetch( m_hstmt ) ) == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO )
                    {
                        std::string data_string;

                        while( (rc =
                                    SQLGetData( m_hstmt, 2, SQL_C_CHAR, symbol_data_buffer, 255,
                                            0 ) ) == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO )
                        {
                            data_string += (const char*) symbol_data_buffer;
                        }

                        if( rc != SQL_NO_DATA )
                        {
                            m_last_error_string = getAllODBCErrors();
                            m_last_error_string.Prepend( "ODBC Library: Errors while fetching data.  Details:\n" );
                            had_errors = true;
                        }
                        else
                        {
                            // Got to this point, we've got a complete id and data package.  Make a part!
                            /// \todo This may duplicate parts that should just be aliased.
                            LIB_PART* part = new LIB_PART( wxEmptyString );
                            parsePart( part, data_string );
                            wxLogTrace( traceMask, "Loaded symbol %s (%s)", (const char *)symbol_name, part->GetName() );
                            wxString db_name = (const char *)symbol_name;
                            cachePart( part );
                            // Sometimes the db's unique key contains illegal characters which get mangled,
                            // or otherwise does not match the part's name once it's loaded, which can
                            // cause the doc loader to not be able to find the corresponding alias. 
                            // Detect if the part's name isn't identical to the name from the DB, and add
                            // an entry in a separate map to the part's root alias.
                            if( !db_name.IsSameAs( part->GetName() ) )
                                addXRef( part->GetName(), db_name );
                        }
                    }
                }
                else
                {
                    m_last_error_string = getAllODBCErrors();
                    m_last_error_string.Prepend( "ODBC Library: Column bind failed.  Details:\n" );
                    had_errors = true;
                }
            }
            else
            {
                m_last_error_string = getAllODBCErrors();
                m_last_error_string.Prepend( "ODBC Library: Symbol load failed.  Details:\n" );
                had_errors = true;
            }
            disconnectAndCleanUp();
            if(had_errors)
                THROW_IO_ERROR(m_last_error_string);
            
            loadDocs();
            
            // Because SCH_LEGACY_PLUGIN did this, we are going to do it too.
            PART_LIBS::s_modify_generation++;

            m_loaded = true;
            update_last_refresh();
        } else {
            wxLogTrace( traceMask, "ODBC_SCH_READER: already loaded." );
        }
    }
    
    void saveDocs() 
    {
        bool had_errors = false;
        
        // The DB has a stored procedure which will only create a new revision of the symbol if the data has changed.
        // What we do is loop through ALL our symbols, serialize them, and hand them over to the stored procedure one
        // by one.
        connectAndGetStatementHandle();
        wxString query = "CALL update_symbol_docs(?, ?)";
        SQLRETURN   rc = SQLPrepareA( m_hstmt, (SQLCHAR*) (char *) query.char_str(), query.length() );

        if( rc == SQL_SUCCESS )
        {
            SQLLEN name_indptr;
            rc =
                SQLBindParameter( m_hstmt,
                        1,
                        SQL_PARAM_INPUT,
                        SQL_C_CHAR,
                        SQL_VARCHAR,
                        255,
                        0,
                        (SQLPOINTER) 98, // The 98 is an application-defined value which will be returned by SQLParamData
                        0,
                        &name_indptr ); 

            if( rc == SQL_SUCCESS )
            {
                SQLLEN data_indptr;
                rc =
                    SQLBindParameter( m_hstmt,
                            2,
                            SQL_PARAM_INPUT,
                            SQL_C_CHAR,
                            SQL_LONGVARCHAR,
                            0,
                            0,
                            (SQLPOINTER) 99, // The 99 is an application-defined value which will be returned by SQLParamData
                            0,
                            &data_indptr );

                if( rc == SQL_SUCCESS )
                {
                    for( LIB_ALIAS_MAP::iterator it = m_aliases.begin(); it!=m_aliases.end(); it++ )
                    {
                        if( !it->second->IsRoot() ) // ODBC schematic library doesn't yet support per-alias docs
                            continue;

                        STRING_FORMATTER formatter;
                        {
                            wxString description =  it->second->GetDescription();
                            wxString keyWords = it->second->GetKeyWords();
                            wxString docFileName = it->second->GetDocFileName();

                            if( description.IsEmpty() && keyWords.IsEmpty() && docFileName.IsEmpty() )
                                continue;

                            formatter.Print( 0, "#\n$CMP %s\n", TO_UTF8( it->second->GetName() ) );

                            if( !description.IsEmpty() )
                                formatter.Print( 0, "D %s\n", TO_UTF8( description ) );

                            if( !keyWords.IsEmpty() )
                                formatter.Print( 0, "K %s\n", TO_UTF8( keyWords ) );

                            if( !docFileName.IsEmpty() )
                                formatter.Print( 0, "F %s\n", TO_UTF8( docFileName ) );

                            formatter.Print( 0, "$ENDCMP\n" );
                        }
                        const wxString & db_name = getXRef( it->second->GetPart()->GetName() );
                        name_indptr = SQL_DATA_AT_EXEC;
                        data_indptr = SQL_DATA_AT_EXEC;
                        rc = SQLExecute( m_hstmt );

                        if( rc == SQL_NEED_DATA )
                        {
                            SQLPOINTER param_id;

                            while( (rc = SQLParamData( m_hstmt, &param_id ) ) == SQL_NEED_DATA )
                            {
                                if( param_id == (SQLPOINTER) 98 )
                                {
                                    rc = SQLPutData( m_hstmt, (SQLPOINTER) (char *)db_name.char_str(), SQL_NTS );
                                }

                                if( param_id == (SQLPOINTER) 99 )
                                {
                                    rc = SQLPutData( m_hstmt,
                                            (SQLPOINTER) formatter.GetString().c_str(),
                                            SQL_NTS );
                                }

                                if( rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO )
                                {
                                    m_last_error_string = getAllODBCErrors();
                                    m_last_error_string.Prepend(
                                            "ODBC Library: Library save docs failed (put data).  Details:\n" );
                                    had_errors = true;
                                }
                            }

                            if( rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO )
                            {
                                m_last_error_string = getAllODBCErrors();
                                m_last_error_string.Prepend(
                                        "ODBC Library: Library save docs failed (ParamData).  Details:\n" );
                                had_errors = true;
                            }
                        }
                        else
                        {
                            m_last_error_string = getAllODBCErrors();
                            m_last_error_string.Prepend(
                                    "ODBC Library: Library save docs failed (execute).  Details:\n" );
                            had_errors = true;
                        }
                    }
                }
                else
                {
                    m_last_error_string = getAllODBCErrors();
                    m_last_error_string.Prepend(
                            "ODBC Library: Library save docs failed (bind data).  Details:\n" );
                    had_errors = true;
                }
            }
            else
            {
                m_last_error_string = getAllODBCErrors();
                m_last_error_string.Prepend( "ODBC Library: Library save docs failed (bind ID).  Details:\n" );
                had_errors = true;
            }
        }
        else
        {
            m_last_error_string = getAllODBCErrors();
            m_last_error_string.Prepend( "ODBC Library: Library save docs failed (prepare).  Details:\n" );
            had_errors = true;
        }

        disconnectAndCleanUp();

        if( had_errors )
            THROW_IO_ERROR( m_last_error_string );
    }
    
    bool refreshNeeded() 
    {
        {
            timeval rightnow;
            gettimeofday( &rightnow, 0 );
            if( rightnow.tv_sec - m_last_refresh_check.tv_sec < m_source_check_interval )
                return false;
        }
        bool needed = false;
        connectAndGetStatementHandle();
        wxString query = "SELECT max(`revision_stamp`) > ? FROM `symbols`";
        SQLRETURN rc = SQLPrepareA( m_hstmt, (SQLCHAR*) (char*) query.char_str(), query.length() );
        if( rc == SQL_SUCCESS )
        {
            rc = SQLBindParameter( m_hstmt, 1, SQL_PARAM_INPUT, SQL_C_TYPE_TIMESTAMP, SQL_TYPE_TIMESTAMP,
                                    0, 0, (SQLPOINTER) &m_last_refresh_stamp, 0, 0 );  
            if( rc == SQL_SUCCESS ) 
            {
                rc = SQLExecute( m_hstmt );
                if( rc == SQL_SUCCESS ) 
                {
                    int truth = 0;
                    SQLLEN indptr = 0;
                    rc = SQLBindCol( m_hstmt, 1, SQL_C_LONG, (SQLPOINTER) &truth, 0, &indptr );
                    if( rc == SQL_SUCCESS ) {
                        rc = SQLFetch( m_hstmt );
                        if( rc == SQL_SUCCESS ) 
                        {
                            needed = (truth == 1);
                        }
                    }
                }
            }
        }
        if(!needed) // symbols haven't changed, but maybe docs have?
        {
            SQLFreeStmt( m_hstmt, SQL_CLOSE );
            query = "SELECT max(`revision_stamp`) > ? FROM `symbol_docs`";
            rc = SQLPrepareA( m_hstmt, (SQLCHAR*) (char*) query.char_str(), query.length() );
            if( rc == SQL_SUCCESS )
            {
                rc = SQLBindParameter( m_hstmt, 1, SQL_PARAM_INPUT, SQL_C_TYPE_TIMESTAMP, SQL_TYPE_TIMESTAMP,
                                        0, 0, (SQLPOINTER) &m_last_refresh_stamp, 0, 0 );  
                if( rc == SQL_SUCCESS ) 
                {
                    rc = SQLExecute( m_hstmt );
                    if( rc == SQL_SUCCESS ) 
                    {
                        int truth = 0;
                        SQLLEN indptr = 0;
                        rc = SQLBindCol( m_hstmt, 1, SQL_C_LONG, (SQLPOINTER) &truth, 0, &indptr );
                        if( rc == SQL_SUCCESS ) {
                            rc = SQLFetch( m_hstmt );
                            if( rc == SQL_SUCCESS ) 
                            {
                                needed = (truth == 1);
                            }
                        }
                    }
                }
            }
        }
        disconnectAndCleanUp();
        return needed;
    }

    void parsePart(LIB_PART * aPart, const wxString & partData) 
    {
        if(aPart != nullptr) {
            STRING_LINE_READER reader( partData.ToStdString(), wxT( "ODBC part data" ) );
            // There may be cruft at the start; advance to the line containing "DEF"
            while( !strCompare("DEF", reader.Line() ) )
                reader.ReadLine();
            LEGACY_PART_SERIALIZER::ReadPart( aPart, reader );
        }
    }
    
    void loadDocs() 
    {
        bool had_errors = false;
        wxString aLine;
        LIB_ALIAS* entry;
        
        connectAndGetStatementHandle();

        wxString query =
            "SELECT `name`, `docs` FROM  `symbol_docs` s1 "
            "WHERE `revision_stamp` = ("
            "SELECT MAX(`revision_stamp`) "
            "from `symbol_docs` s2 where `s1`.`name` = `s2`.`name`) "
            "ORDER BY `name` ";
        SQLRETURN rc = SQLExecDirectA( m_hstmt, (SQLCHAR*) (char*) query.char_str(), query.length() );

        if( rc == SQL_SUCCESS )
        {
            // We will bind a column for the ID field (a bigint), but will fetch the data field in chunks.
            SQLCHAR symbol_name[256];           // SQL_C_CHAR
            SQLCHAR symbol_data_buffer[255];    // SQL_C_CHAR
            rc = SQLBindCol( m_hstmt, 1, SQL_C_CHAR, &symbol_name, 256, 0 );

            if( rc == SQL_SUCCESS )
            {
                while( (rc = SQLFetch( m_hstmt ) ) == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO )
                {
                    std::string data_string;

                    while( (rc =
                                SQLGetData( m_hstmt, 2, SQL_C_CHAR, symbol_data_buffer, 255,
                                        0 ) ) == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO )
                    {
                        data_string += (const char*) symbol_data_buffer;
                    }

                    if( rc != SQL_NO_DATA )
                    {
                        m_last_error_string = getAllODBCErrors();
                        m_last_error_string.Prepend( "ODBC Library: Errors while fetching data.  Details:\n" );
                        had_errors = true;
                    }
                    else
                    {
                        wxString dbkey = FROM_UTF8( (char *)symbol_name );
                        auto foundling = m_aliases.find( getDeXRef( dbkey ) );
                        if( foundling != m_aliases.end() ) {
                            entry = foundling->second;
                            if( entry != nullptr ) {
                                // Got to this point, we've got the documents for the part.  
                                // Figure out how to actually use them...
                                STRING_LINE_READER reader( data_string, wxString( "symbol docs" ) );
                                const char * reader_line = 0;
                                // Expectation is that we are handling exactly one set of part documents,
                                // so the entire string should end with one and only one $ENDCMP
                                while( ( reader_line = reader.ReadLine() ) != 0 ) 
                                {
                                    aLine = reader_line;
                                    if( aLine.IsSameAs( "$ENDCMP" ) )
                                        break;
                                    wxString aDoc = aLine.SubString( 2, aLine.Len() - 1 );
                                    aDoc.Trim();
                                    switch( (wchar_t) aLine[0] )
                                    {
                                        case 'D':
                                            entry->SetDescription( aDoc );
                                            break;
                                            
                                        case 'K':
                                            entry->SetKeyWords( aDoc );
                                            break;
                                            
                                        case 'F':
                                            entry->SetDocFileName( aDoc  );
                                            break;
                                    }
                                    
                                }
                            }
                            else
                            {
                                wxLogError( "loadDocs: Alias %s is null!", dbkey );
                            }
                        }
                        else 
                        {
                            wxLogWarning( "loadDocs: Alias %s not found in library", dbkey );
                        }
                    }
                }

                if( rc != SQL_NO_DATA )
                {
                    m_last_error_string = getAllODBCErrors();
                    m_last_error_string.Prepend( "ODBC Library: Errors while fetching data.  Details:\n" );
                    had_errors = true;
                }
            }
            else
            {
                m_last_error_string = getAllODBCErrors();
                m_last_error_string.Prepend( "ODBC Library: Column bind failed.  Details:\n" );
                had_errors = true;
            }
        }
        else
        {
            m_last_error_string = getAllODBCErrors();
            m_last_error_string.Prepend( "ODBC Library: Symbol load failed.  Details:\n" );
            had_errors = true;
        }

        disconnectAndCleanUp();
        if( had_errors )
        {
            THROW_IO_ERROR( m_last_error_string );
        }
    }
    
    void update_last_refresh() 
    {
        tm                   broken_down_time;

        gettimeofday(&m_last_refresh_check, 0);
        gmtime_r (&(m_last_refresh_check.tv_sec), &broken_down_time);

        m_last_refresh_stamp.year     = broken_down_time.tm_year + 1900;
        m_last_refresh_stamp.month    = broken_down_time.tm_mon;
        m_last_refresh_stamp.day      = broken_down_time.tm_mday;
        m_last_refresh_stamp.hour     = broken_down_time.tm_hour;
        m_last_refresh_stamp.minute   = broken_down_time.tm_min;
        m_last_refresh_stamp.second   = broken_down_time.tm_sec;
        m_last_refresh_stamp.fraction = m_last_refresh_check.tv_usec * 1000; // micros to nanos
    }
    
    void discardCache() 
    {
        // When the cache is destroyed, all of the alias objects on the heap should be deleted.
        for( const auto & entry : m_aliases )
        {
            LIB_PART* part = entry.second->GetPart();
            LIB_ALIAS* alias = entry.second;
            wxString partName = part->GetName();
            if( alias->IsRoot() )
                wxLogTrace( traceMask, "Deleting root alias for %s", partName );
            else
                wxLogTrace( traceMask, "Deleting alias %s", alias->GetName() );
            if( hasXRef( alias->GetName() ) )
                delXRef( alias->GetName() );
            delete alias;

            // When the last alias of a part is destroyed, the part is no longer required and it
            // too is destroyed.
            size_t remaining_aliases = part->GetAliasCount();
            if( remaining_aliases == 0 ) {
                wxLogTrace(traceMask, "Deleting part %s", partName );
                delete part;
            } else if(part->GetAliasCount() > 0) {
                wxLogTrace(traceMask, "Part %s has %d dangling alias%s.", partName, ( int )remaining_aliases, ( ( remaining_aliases > 1 ) ? "es" : "" ) );
            }
        }
        m_aliases.clear();
    }
    
    void recachePart( const LIB_PART * aPart ) 
    {
        wxString root_name = aPart->GetName();
        LIB_ALIASES aliases = aPart->GetAliases();
        for( LIB_ALIAS * anAlias : aliases ) {
            wxString alias_name = anAlias->GetName();
            try {
                if( m_aliases.at( alias_name ) != anAlias ) {
                    delete m_aliases.at( alias_name );
                    wxLogTrace( traceMask, "Deleted stale alias %s", alias_name );
                }
                else
                {
                    wxLogTrace( traceMask, "Alias already known: %s", alias_name );
                }
            } 
            catch( std::out_of_range & ) 
            { 
                /* nothing to do; maybe it's a new alias */ 
                wxLogTrace( traceMask, "New alias: %s (part %s)", alias_name, root_name );
            }
            m_aliases[alias_name] = anAlias;
        }
    }
        
    
    void cachePart( const LIB_PART * aPart ) 
    {
        if( aPart->GetAliasCount() > 0 ) {
            LIB_ALIASES aliases = aPart->GetAliases();
            for( LIB_ALIAS * anAlias : aliases ) {
                if( anAlias != nullptr ) {
                    const wxString & aliasName = anAlias->GetName();
                    if( m_aliases.find( aliasName ) != m_aliases.end() ) {
                        wxLogTrace(traceMask, "ODBC_SCH_READER: WARNING: duplicate alias found: %s", aliasName );
                    }
                    else
                    {
                        m_aliases[aliasName] = anAlias;
                    }
                } else {
                    wxLogTrace(traceMask, "ODBC_SCH_READER: ERROR: part returned null alias");
                }
            }  
        } 
        else
        {
            wxLogTrace(traceMask, "ODBC_SCH_READER::cachePart: ERROR: Part has no aliases?");
        }
    }
public:
    ODBC_SCH_READER(const wxString & path) :
        m_henv(0),
        m_hdbc(0),
        m_hstmt(0),
        m_connection_string(wxT("")),
        m_file_path(path),
        m_source_check_interval(60),
        m_loaded(false)
    {
        wxLogTrace(traceMask, "ODBC_SCH_READER instantiated for library path %s", path);
        update_last_refresh();
        readInit();
    }
    
    ~ODBC_SCH_READER() 
    {
        wxLogTrace(traceMask, "ODBC_SCH_READER destroyed for library path %s", m_file_path);
        disconnectAndCleanUp();
        discardCache();
    }
    
    void EnumerateSymbolLib( wxArrayString & anArrayString, const PROPERTIES * aProperties ) 
    {
        load();
        for( const auto & entry : m_aliases ) {
            anArrayString.Add(entry.first);
        }
    }
    
    void EnumerateSymbolLib( std::vector<LIB_ALIAS*>& aAliasList, const PROPERTIES* aProperties )
    {
        load();
        for( const auto & entry : m_aliases ) {
            aAliasList.push_back(entry.second);
        }
    }
    
    LIB_ALIAS* LoadSymbol( const wxString& aAliasName, const PROPERTIES* aProperties ) 
    {
        load();
        wxLogTrace( traceMask, "LoadSymbol: %s", aAliasName );
        auto foundling = m_aliases.find( aAliasName );
        if(foundling != m_aliases.end())
            return foundling->second;
        return NULL;  // Theoretically we shouldn't be asked to load an alias that wasn't enumerated
    } 

    
    void SaveSymbol( const LIB_PART* aPart, const PROPERTIES* aProperties ) 
    {
        // Oh FFS, we can't depend on the part's IS_CHANGED flag OR its timestamp.
        // Back to the old approach of letting a stored procedure on the DB determine 
        // if it really needs to be saved.  Eventually maybe I'll bring the effort 
        // back to this class and do something with checksums.  Eh.
        wxLogTrace( traceMask, "SaveSymbol %s", aPart->GetName());
        recachePart( aPart );
    }
    
    void SaveLibrary( const PROPERTIES* aProperties )
    {
        bool had_errors = false;
        
        // The DB has a stored procedure which will only create a new revision of the symbol if the data has changed.
        // What we do is loop through ALL our symbols, serialize them, and hand them over to the stored procedure one
        // by one.
        connectAndGetStatementHandle();
        const wxString query = "CALL update_symbol(?, ?)";
        SQLRETURN   rc = SQLPrepareA( m_hstmt, (SQLCHAR*) (char *) query.char_str(), query.length() );

        if( rc == SQL_SUCCESS )
        {
            SQLLEN name_indptr;
            rc =
                SQLBindParameter( m_hstmt,
                        1,
                        SQL_PARAM_INPUT,
                        SQL_C_CHAR,
                        SQL_VARCHAR,
                        255,
                        0,
                        (SQLPOINTER) 98,
                        0,
                        &name_indptr );                                                                                           // The 98 is an application-defined value which will be returned by SQLParamData

            if( rc == SQL_SUCCESS )
            {
                SQLLEN data_indptr;
                rc =
                    SQLBindParameter( m_hstmt,
                            2,
                            SQL_PARAM_INPUT,
                            SQL_C_CHAR,
                            SQL_LONGVARCHAR,
                            0,
                            0,
                            (SQLPOINTER) 99,
                            0,
                            &data_indptr );                                                                                             // The 99 is an application-defined value which will be returned by SQLParamData

                if( rc == SQL_SUCCESS )
                {
                    for( LIB_ALIAS_MAP::iterator it = m_aliases.begin(); it!=m_aliases.end(); it++ )
                    {
                        if( !it->second->IsRoot() )
                            continue;

                        STRING_FORMATTER formatter;
                        LEGACY_PART_SERIALIZER::WritePart( it->second->GetPart(), formatter );
                        const wxString & dbkey = getXRef( it->second->GetPart()->GetName() );
                        
                        name_indptr = SQL_DATA_AT_EXEC;
                        data_indptr = SQL_DATA_AT_EXEC;
                        rc = SQLExecute( m_hstmt );

                        if( rc == SQL_NEED_DATA )
                        {
                            SQLPOINTER param_id;

                            while( (rc = SQLParamData( m_hstmt, &param_id ) ) == SQL_NEED_DATA )
                            {
                                if( param_id == (SQLPOINTER) 98 )
                                {
                                    rc = SQLPutData( m_hstmt, (SQLPOINTER) (char *) dbkey.char_str(), SQL_NTS );
                                }

                                if( param_id == (SQLPOINTER) 99 )
                                {
                                    rc = SQLPutData( m_hstmt,
                                            (SQLPOINTER) formatter.GetString().c_str(),
                                            SQL_NTS );
                                }

                                if( rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO )
                                {
                                    m_last_error_string = getAllODBCErrors();
                                    m_last_error_string.Prepend(
                                            "ODBC Library: Library save failed (put data).  Details:\n" );
                                    had_errors = true;
                                }
                            }

                            if( rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO )
                            {
                                m_last_error_string = getAllODBCErrors();
                                m_last_error_string.Prepend(
                                        "ODBC Library: Library save failed (ParamData).  Details:\n" );
                                had_errors = true;
                            }
                        }
                        else
                        {
                            m_last_error_string = getAllODBCErrors();
                            m_last_error_string.Prepend(
                                    "ODBC Library: Library save failed (execute).  Details:\n" );
                            had_errors = true;
                        }
                    }
                }
                else
                {
                    m_last_error_string = getAllODBCErrors();
                    m_last_error_string.Prepend( "ODBC Library: Library save failed (bind data).  Details:\n" );
                    had_errors = true;
                }
            }
            else
            {
                m_last_error_string = getAllODBCErrors();
                m_last_error_string.Prepend( "ODBC Library: Library save failed (bind ID).  Details:\n" );
                had_errors = true;
            }
        }
        else
        {
            m_last_error_string = getAllODBCErrors();
            m_last_error_string.Prepend( "ODBC Library: Library save failed (prepare).  Details:\n" );
            had_errors = true;
        }

        disconnectAndCleanUp();
        if( had_errors )
            THROW_IO_ERROR( m_last_error_string );
        saveDocs();
    }
    
    size_t GetSymbolLibCount( const PROPERTIES* aProperties ) 
    {
        load();
        return m_aliases.size();
    }
};




ODBC_SCH_PLUGIN::ODBC_SCH_PLUGIN() : 
    m_dirty_count( 0 )
{
    // The PART_LIB constructor does everything we need it to do for the time being, since we don't
    // have a robust mechanism for configuring a new library within KiCAD.  A user must create the
    // descriptor file in advance and "add" the library rather than "create" it.
    wxLogTrace(traceMask, "ODBC_SCH_PLUGIN instantiated!");
}


ODBC_SCH_PLUGIN::~ODBC_SCH_PLUGIN()
{
    wxLogTrace(traceMask, "ODBC_SCH_PLUGIN destroyed!");
    // Just make sure everything's cleaned up.
}


void ODBC_SCH_PLUGIN::SaveLibrary( const wxString& aFileName, const PROPERTIES* aProperties ) 
{
    getReaderByPath( aFileName )->SaveLibrary( aProperties );
}


size_t ODBC_SCH_PLUGIN::GetSymbolLibCount( const wxString&   aLibraryPath,
                                      const PROPERTIES* aProperties )  
{
    return getReaderByPath( aLibraryPath )->GetSymbolLibCount( aProperties );
}

void ODBC_SCH_PLUGIN::EnumerateSymbolLib( wxArrayString&    aAliasNameList,
                                     const wxString&   aLibraryPath,
                                     const PROPERTIES* aProperties ) 
{
    getReaderByPath( aLibraryPath )->EnumerateSymbolLib( aAliasNameList, aProperties );
}

void ODBC_SCH_PLUGIN::EnumerateSymbolLib( std::vector<LIB_ALIAS*>& aAliasList,
                                     const wxString&   aLibraryPath,
                                     const PROPERTIES* aProperties ) 
{
    getReaderByPath(aLibraryPath)->EnumerateSymbolLib(aAliasList, aProperties);
}

LIB_ALIAS* ODBC_SCH_PLUGIN::LoadSymbol( const wxString& aLibraryPath, const wxString& aAliasName,
                                   const PROPERTIES* aProperties )
{
    return getReaderByPath( aLibraryPath )->LoadSymbol( aAliasName, aProperties );
}

void ODBC_SCH_PLUGIN::SaveSymbol( const wxString& aLibraryName, const LIB_PART* aSymbol,
                             const PROPERTIES* aProperties ) 
{
    READER_PTR lib = getReaderByName( aLibraryName );
    if(lib == nullptr) {
        wxLogError("Couldn't find an active library interface named %s", aLibraryName);
    } else {
        lib->SaveSymbol(aSymbol, aProperties);
    }
}

void ODBC_SCH_PLUGIN::DeleteAlias( const wxString& aLibraryPath, const wxString& aAliasName,
                              const PROPERTIES* aProperties ) 
{
    wxLogTrace(traceMask, "ODBC_SCH_PLUGIN::DeleteAlias called with filename %s", aLibraryPath);
}

void ODBC_SCH_PLUGIN::DeleteSymbol( const wxString& aLibraryPath, const wxString& aAliasName,
                               const PROPERTIES* aProperties ) 
{
    wxLogTrace(traceMask, "ODBC_SCH_PLUGIN::DeleteSymbol called with filename %s", aLibraryPath);
}


bool ODBC_SCH_PLUGIN::IsSymbolLibWritable( const wxString& aLibraryPath ) 
{
    wxLogTrace(traceMask, "ODBC_SCH_PLUGIN::IsSymbolLibWritable called with filename %s", aLibraryPath);
    return true;
}


void ODBC_SCH_PLUGIN::SymbolLibOptions( PROPERTIES* aListToAppendTo ) const 
{
    wxLogTrace( traceMask, "SymbolLibOptions!" );
}

bool ODBC_SCH_PLUGIN::CheckHeader( const wxString & aFileName ) 
{
    wxLogTrace(traceMask, "ODBC_SCH_PLUGIN::CheckHeader called with filename %s", aFileName);
    return ChkHeader(aFileName);
}

bool ODBC_SCH_PLUGIN::ChkHeader( const wxString& aFileName ) 
{
    wxLogTrace(traceMask, "ODBC_SCH_PLUGIN checking %s", aFileName);
    bool goodHeader = false;
    FILE* file;

    file = wxFopen( aFileName, wxT( "rt" ) );

    if( file != 0 )
    {
        {
            FILE_LINE_READER reader( file, aFileName );
            wxString header = reader.ReadLine();
            goodHeader = header.StartsWith(wxT("[MyODBC"));
        }
    }
    else
    {
        THROW_IO_ERROR( std::string( "ODBC Library: Config file not found." ) );
    }
    wxLogTrace(traceMask, "ODBC_SCH_PLUGIN %s", goodHeader ? "identified ODBC library" : "did not identify ODBC library");
    return goodHeader;
}

const wxString& ODBC_SCH_PLUGIN::GetError() const 
{ 
    return m_last_error_string;
}

ODBC_SCH_PLUGIN::READER_PTR ODBC_SCH_PLUGIN::getReaderByPath( const wxString & libPath ) 
{
    READER_PTR reader;
    auto loaded = m_libs_by_path.find( libPath );
    if( loaded == m_libs_by_path.end() ) 
    {
        reader = std::make_shared<ODBC_SCH_READER>( libPath );
        m_libs_by_path[libPath] = reader;
        wxFileName name( libPath );
        m_libs_by_name[name.GetName()] = reader;
    } 
    else 
    {
        reader = loaded->second;
    }
    return reader;
}

ODBC_SCH_PLUGIN::READER_PTR ODBC_SCH_PLUGIN::getReaderByName( const wxString & name ) const
{
    try 
    {
        return m_libs_by_name.at(name);
    } 
    catch( std::out_of_range& )
    {
        return nullptr;
    }
}


#if 0
bool ODBC_SCH_PLUGIN::Save( OUTPUTFORMATTER& aFormatter )
{
    if( isModified )
    {
        timeStamp   = GetNewTimeStamp();
        isModified  = false;
    }

    bool had_errors = false;
    wxString error_message;

    // When eeschema symbol editor calls Save, it has already opened and
    // truncated our file.  In addition to the RDBMS work we must do, we
    // must also rewrite the config file.  Do that first.
    aFormatter.Print( 0, "%s\n", "ODBC eeschema library descriptor" );
    aFormatter.Print( 0, "%s\n", (const char*) m_connection_string.c_str() );

    // Now do the RDBMS work.
    // The DB has a stored procedure which will only create a new revision of the symbol if the data has changed.
    // What we do is loop through ALL our symbols, serialize them, and hand them over to the stored procedure one
    // by one.
    ConnectAndGetStatementHandle();
    std::string query = "CALL update_symbol(?, ?)";
    SQLRETURN   rc = SQLPrepareA( m_hstmt, (SQLCHAR*) query.c_str(), query.length() );

    if( rc == SQL_SUCCESS )
    {
        SQLLEN name_indptr;
        rc =
            SQLBindParameter( m_hstmt,
                    1,
                    SQL_PARAM_INPUT,
                    SQL_C_CHAR,
                    SQL_VARCHAR,
                    255,
                    0,
                    (SQLPOINTER) 98,
                    0,
                    &name_indptr );                                                                                           // The 98 is an application-defined value which will be returned by SQLParamData

        if( rc == SQL_SUCCESS )
        {
            SQLLEN data_indptr;
            rc =
                SQLBindParameter( m_hstmt,
                        2,
                        SQL_PARAM_INPUT,
                        SQL_C_CHAR,
                        SQL_LONGVARCHAR,
                        0,
                        0,
                        (SQLPOINTER) 99,
                        0,
                        &data_indptr );                                                                                             // The 99 is an application-defined value which will be returned by SQLParamData

            if( rc == SQL_SUCCESS )
            {
                for( LIB_ALIAS_MAP::iterator it = m_amap.begin(); it!=m_amap.end(); it++ )
                {
                    if( !it->second->IsRoot() )
                        continue;

                    STRING_FORMATTER formatter;
                    it->second->GetPart()->Save( formatter );
                    std::string part_name( it->second->GetPart()->GetName() );
                    std::string db_name = part_name;

                    if( m_symbol_db_names.find( part_name ) == m_symbol_db_names.end() )
                    {
                        m_symbol_db_names[part_name] = db_name;
                    }
                    else
                    {
                        db_name = m_symbol_db_names[part_name];
                    }

                    name_indptr = SQL_DATA_AT_EXEC;
                    data_indptr = SQL_DATA_AT_EXEC;
                    rc = SQLExecute( m_hstmt );

                    if( rc == SQL_NEED_DATA )
                    {
                        SQLPOINTER param_id;

                        while( (rc = SQLParamData( m_hstmt, &param_id ) ) == SQL_NEED_DATA )
                        {
                            if( param_id == (SQLPOINTER) 98 )
                            {
                                rc = SQLPutData( m_hstmt, (SQLPOINTER) db_name.c_str(), SQL_NTS );
                            }

                            if( param_id == (SQLPOINTER) 99 )
                            {
                                rc = SQLPutData( m_hstmt,
                                        (SQLPOINTER) formatter.GetString().c_str(),
                                        SQL_NTS );
                            }

                            if( rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO )
                            {
                                error_message = GetAllODBCErrors();
                                error_message.Prepend(
                                        "ODBC Library: Library save failed (put data).  Details:\n" );
                                had_errors = true;
                            }
                        }

                        if( rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO )
                        {
                            error_message = GetAllODBCErrors();
                            error_message.Prepend(
                                    "ODBC Library: Library save failed (ParamData).  Details:\n" );
                            had_errors = true;
                        }
                    }
                    else
                    {
                        error_message = GetAllODBCErrors();
                        error_message.Prepend(
                                "ODBC Library: Library save failed (execute).  Details:\n" );
                        had_errors = true;
                    }
                }
            }
            else
            {
                error_message = GetAllODBCErrors();
                error_message.Prepend( "ODBC Library: Library save failed (bind data).  Details:\n" );
                had_errors = true;
            }
        }
        else
        {
            error_message = GetAllODBCErrors();
            error_message.Prepend( "ODBC Library: Library save failed (bind ID).  Details:\n" );
            had_errors = true;
        }
    }
    else
    {
        error_message = GetAllODBCErrors();
        error_message.Prepend( "ODBC Library: Library save failed (prepare).  Details:\n" );
        had_errors = true;
    }

    DisconnectAndCleanUp();

    if( had_errors )
        THROW_IO_ERROR( error_message );

    return true;
}


#endif
