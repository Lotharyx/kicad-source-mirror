/**
 * \file ODBC library class implementation
 *
 * @author lotharyx
 */


#include "class_odbc_library.h"

#include <sqlext.h>
#include <richio.h>
#include <sstream>
#include <macros.h>

#define duplicate_name_msg  \
    _(  "Library '%s' has duplicate entry name '%s'.\n" \
            "This may cause some unexpected behavior when loading components into a schematic." )
            

ODBC_LIB::ODBC_LIB( int aType, const wxString& aConnectionFileName ) : PART_LIB(aType, aConnectionFileName),
m_henv(0),
m_hdbc(0),
m_hstmt(0),
m_connection_string("")
{
    // The PART_LIB constructor does everything we need it to do for the time being, since we don't 
    // have a robust mechanism for configuring a new library within KiCAD.  A user must create the
    // descriptor file in advance and "add" the library rather than "create" it.
}

ODBC_LIB::~ODBC_LIB() {
    // Just make sure everything's cleaned up.
    DisconnectAndCleanUp();
}

bool ODBC_LIB::Save( OUTPUTFORMATTER& aFormatter ) { 
    if( isModified )
    {
        timeStamp = GetNewTimeStamp();
        isModified = false;
    }
    
        bool had_errors = false;
    wxString error_message;
    
    // When eeschema symbol editor calls Save, it has already opened and 
    // truncated our file.  In addition to the RDBMS work we must do, we
    // must also rewrite the config file.  Do that first.
    aFormatter.Print( 0, "%s\n", "ODBC eeschema library descriptor" );
    aFormatter.Print( 0, "%s\n", (const char *)m_connection_string.c_str() );

    // Now do the RDBMS work.
    // The DB has a stored procedure which will only create a new revision of the symbol if the data has changed.
    // What we do is loop through ALL our symbols, serialize them, and hand them over to the stored procedure one
    // by one.
    ConnectAndGetStatementHandle();
    std::string query = "CALL update_symbol(?, ?)";
    SQLRETURN rc = SQLPrepareA(m_hstmt, (SQLCHAR *)query.c_str(), query.length());
    if(rc == SQL_SUCCESS) {
        SQLLEN name_indptr;
        rc = SQLBindParameter(m_hstmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 255, 0, (SQLPOINTER)98, 0, &name_indptr); // The 98 is an application-defined value which will be returned by SQLParamData
        if(rc == SQL_SUCCESS) {
            SQLLEN data_indptr;
            rc = SQLBindParameter(m_hstmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_LONGVARCHAR, 0, 0, (SQLPOINTER)99, 0, &data_indptr); // The 99 is an application-defined value which will be returned by SQLParamData
            if(rc == SQL_SUCCESS) {
                for( LIB_ALIAS_MAP::iterator it=m_amap.begin();  it!=m_amap.end();  it++ )
                {
                    if( !it->second->IsRoot() )
                        continue;
                    
                    STRING_FORMATTER formatter;
                    it->second->GetPart()->Save( formatter );
                    std::string part_name(it->second->GetPart()->GetName());
                    std::string db_name = part_name;
                    if(m_symbol_db_names.find(part_name) == m_symbol_db_names.end()) {
                        m_symbol_db_names[part_name] = db_name;
                    } else {
                        db_name = m_symbol_db_names[part_name];
                    }
                    name_indptr = SQL_DATA_AT_EXEC;
                    data_indptr = SQL_DATA_AT_EXEC;
                    rc = SQLExecute(m_hstmt);
                    if(rc == SQL_NEED_DATA) {
                        SQLPOINTER param_id;
                        while((rc = SQLParamData(m_hstmt, &param_id)) == SQL_NEED_DATA) {
                            if(param_id == (SQLPOINTER)98) {
                                rc = SQLPutData(m_hstmt, (SQLPOINTER)db_name.c_str(), SQL_NTS);
                            }
                            if(param_id == (SQLPOINTER)99) {
                                rc = SQLPutData(m_hstmt, (SQLPOINTER)formatter.GetString().c_str(), SQL_NTS);
                            }
                            if(rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
                                error_message = GetAllODBCErrors();
                                error_message.Prepend( "ODBC Library: Library save failed (put data).  Details:\n" );
                                had_errors = true;
                            }
                        }
                        if(rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
                            error_message = GetAllODBCErrors();
                            error_message.Prepend( "ODBC Library: Library save failed (ParamData).  Details:\n" );
                            had_errors = true;
                        }
                    } else {
                        error_message = GetAllODBCErrors();
                        error_message.Prepend( "ODBC Library: Library save failed (execute).  Details:\n" );
                        had_errors = true;
                    }
                }
            } else {
                error_message = GetAllODBCErrors();
                error_message.Prepend( "ODBC Library: Library save failed (bind data).  Details:\n" );
                had_errors = true;
            }
        } else {
            error_message = GetAllODBCErrors();
            error_message.Prepend( "ODBC Library: Library save failed (bind ID).  Details:\n" );
            had_errors = true;
        }
    } else {
        error_message = GetAllODBCErrors();
        error_message.Prepend( "ODBC Library: Library save failed (prepare).  Details:\n" );
        had_errors = true;
    }
    DisconnectAndCleanUp();
    if(had_errors)
        THROW_IO_ERROR(error_message);
    
    return true; 
    
}

bool ODBC_LIB::SaveDocs( OUTPUTFORMATTER& aFormatter ) { 
    /// \todo Extend database schema to include dcm info
    return true; 
}


bool ODBC_LIB::Load( wxString& aErrorMsg ) { 
    bool had_errors = false;
    ReadInit();
    ConnectAndGetStatementHandle();
    
    wxString query = 
        "SELECT `name`, `data` FROM  `symbols` s1 "
        "WHERE `revision_stamp` = ("
            "SELECT MAX(`revision_stamp`) "
            "from `symbols` s2 where `s1`.`name` = `s2`.`name`) "
        "ORDER BY `name` ";
    SQLRETURN rc = SQLExecDirectA(m_hstmt, (SQLCHAR *)(char *)query.char_str(), query.length());
    if(rc == SQL_SUCCESS) {
        // We will bind a column for the ID field (a bigint), but will fetch the data field in chunks.
        SQLCHAR symbol_name[256]; // SQL_C_CHAR
        SQLCHAR symbol_data_buffer[255]; // SQL_C_CHAR
        rc = SQLBindCol(m_hstmt, 1, SQL_C_CHAR, &symbol_name, 256, 0);
        if(rc == SQL_SUCCESS) {
            while((rc = SQLFetch(m_hstmt)) == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO) {
                std::string data_string;
                while((rc = SQLGetData(m_hstmt, 2, SQL_C_CHAR, symbol_data_buffer, 255, 0)) == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO) {
                    data_string += (const char *)symbol_data_buffer;
                }
                if(rc != SQL_NO_DATA) {
                    aErrorMsg = GetAllODBCErrors();
                    aErrorMsg.Prepend( "ODBC Library: Errors while fetching data.  Details:\n" );
                    had_errors = true;
                } else {
                    // Got to this point, we've got a complete id and data package.  Make a part!
                    LIB_PART* part = new LIB_PART( wxEmptyString, this );
                    STRING_LINE_READER reader(data_string, wxString("symbol data"));
                    // part->Load expects the reader to already be "primed" and doesn't understand
                    // the very same comments that the formatter writes out, so we have to skip them ourselves.
                    while(*reader.ReadLine() == '#');
                    if( part->Load( reader, aErrorMsg ) )
                    {
                        // Check for duplicate entry names and warn the user about
                        // the potential conflict.
                        if( FindEntry( part->GetName() ) != NULL )
                        {
                            wxString msg = duplicate_name_msg;
                            
                            wxLogWarning( msg,
                                          GetChars( fileName.GetName() ),
                                          GetChars( part->GetName() ) );
                        }
                        
                        LoadAliases( part );
                        std::string part_name(part->GetName());
                        m_symbol_db_names[part_name] = std::string((char *)symbol_name);
                    } else {
                        had_errors = true;
                    }
                }
            }
            if(rc != SQL_NO_DATA) {
                aErrorMsg = GetAllODBCErrors();
                aErrorMsg.Prepend( "ODBC Library: Errors while fetching data.  Details:\n" );
                had_errors = true;
            }
        } else {
            aErrorMsg = GetAllODBCErrors();
            aErrorMsg.Prepend( "ODBC Library: Column bind failed.  Details:\n" );
            had_errors = true;
        }
    } else {
        aErrorMsg = GetAllODBCErrors();
        aErrorMsg.Prepend( "ODBC Library: Symbol load failed.  Details:\n" );
        had_errors = true;
    }
    DisconnectAndCleanUp();
    return !had_errors; 
}

bool ODBC_LIB::LoadDocs( wxString& aErrorMsg ) { return false; }
    

void ODBC_LIB::ReadInit() {
    FILE*          file;
    file = wxFopen( fileName.GetFullPath(), wxT( "rt" ) );
    if(file != 0) {
        FILE_LINE_READER reader( file, fileName.GetFullPath() );
        if( !reader.ReadLine() )
        {
            THROW_IO_ERROR( std::string("ODBC Library: Config file is empty!") );
        }
        // First line can be ignored; it's our header.
        // The next line is the connection string.
        m_connection_string = reader.ReadLine();
        m_connection_string.Trim();
        // and that's it!
    } else {
        THROW_IO_ERROR( std::string("ODBC Library: Config file not found.") );
    }
    
}


void ODBC_LIB::ConnectAndGetStatementHandle() {
    SQLRETURN rc = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &m_henv);
    if(rc != SQL_SUCCESS)
        THROW_IO_ERROR( wxString( "ODBC Library: Could not intialize ODBC system." ) );
    rc = SQLSetEnvAttr(m_henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER) SQL_OV_ODBC3, 0);
    if(rc != SQL_SUCCESS) {
        DisconnectAndCleanUp();
        THROW_IO_ERROR( wxString( "ODBC Library: Required ODBCv3 not available." ) );
    }
    rc = SQLAllocHandle(SQL_HANDLE_DBC, m_henv, &m_hdbc);
    if(rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
        DisconnectAndCleanUp();
        THROW_IO_ERROR( wxString("ODBC Library: No database connection handle available.") );
    }
    rc = SQLDriverConnectA(
            m_hdbc,
            NULL,
            (SQLCHAR *)(char *)m_connection_string.char_str(),
            m_connection_string.length(),
            NULL,
            0,
            NULL,
            SQL_DRIVER_NOPROMPT);
    if(rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
        wxString errors = GetAllODBCErrors();
        DisconnectAndCleanUp();
        errors.Prepend( "ODBC Library: Connection failed.  Details:\n" );
        THROW_IO_ERROR( errors );
    }
    rc = SQLAllocHandle(SQL_HANDLE_STMT, m_hdbc, &m_hstmt);
    if(rc != SQL_SUCCESS) {
        wxString errors = GetAllODBCErrors();
        DisconnectAndCleanUp();
        errors.Prepend( "ODBC Library: No statement handles available.  Details:\n" );
        THROW_IO_ERROR( errors );
    }
}

void ODBC_LIB::DisconnectAndCleanUp() {
    SQLFreeHandle(SQL_HANDLE_STMT, m_hstmt);
    m_hstmt = 0;
    SQLDisconnect(m_hdbc);
    SQLFreeHandle(SQL_HANDLE_DBC, m_hdbc);
    m_hdbc = 0;
    SQLFreeHandle(SQL_HANDLE_ENV, m_henv);
    m_henv = 0;
}
    
    
wxString ODBC_LIB::GetAllODBCErrors() {
    wxString errors;
    if(m_hstmt != 0)
        errors += GetODBCErrors(SQL_HANDLE_STMT, m_hstmt);
    if(m_hdbc != 0)
        errors += GetODBCErrors(SQL_HANDLE_DBC, m_hdbc);
    if(m_henv != 0)
        errors += GetODBCErrors(SQL_HANDLE_ENV, m_henv);
    return errors;
}

wxString ODBC_LIB::GetODBCErrors(SQLSMALLINT handle_type, SQLHANDLE handle) {
    std::stringstream errors;
    SQLCHAR sqlstate[6] = {0,0,0,0,0,0};
    SQLINTEGER native_error = 0;
    std::auto_ptr<SQLCHAR> message;
    SQLSMALLINT message_length = 0;
    SQLRETURN r = 0;
    for(int rec = 1; r == SQL_SUCCESS; ++rec) {
        r = SQLGetDiagRecA(
            handle_type,
            handle,
            rec,
            sqlstate,
            &native_error,
            NULL,
            0, 
            &message_length);
        if(r == SQL_SUCCESS || r == SQL_SUCCESS_WITH_INFO) {
            message.reset(new SQLCHAR[message_length + 1]);
            r = SQLGetDiagRecA(
                handle_type,
                handle,
                rec,
                sqlstate,
                &native_error,
                message.get(),
                              message_length + 1, 
                              &message_length);
            if(r == SQL_SUCCESS) {
                errors << "SQLSTATE: " << (char *)sqlstate << std::endl;
                errors << "Native error: " << native_error << std::endl;
                errors << "Message: " << (char *)message.get() << std::endl;
            }
        }
                    
    }
    return errors.str();
}