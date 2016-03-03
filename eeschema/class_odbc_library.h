/**
 * \file ODBC symbol library loader
 */

#ifndef ODBC_SYMBOL_LIBRARY_H_
#define ODBC_SYMBOL_LIBRARY_H_

#include <class_library.h>
#include <sqlext.h>
#include <map>

class ODBC_LIB : public PART_LIB {
public:
    ODBC_LIB( int aType, const wxString& aConnectionFileName );
    virtual ~ODBC_LIB();
    
    /**
     * Function Save
     * writes library to \a aFormatter.
     *
     * @param aFormatter An #OUTPUTFORMATTER object to write the library to.
     * @return True if success writing to \a aFormatter.
     */
    virtual bool Save( OUTPUTFORMATTER& aFormatter ) /*override*/;
    
    /**
     * Function SaveDocs
     * write the library document information to \a aFormatter.
     *
     * @param aFormatter An #OUTPUTFORMATTER object to write the library documentation to.
     * @return True if success writing to \a aFormatter.
     */
    virtual bool SaveDocs( OUTPUTFORMATTER& aFormatter ) /*override*/;
    
    /**
     * Load library from file.
     *
     * @param aErrorMsg - Error message if load fails.
     * @return True if load was successful otherwise false.
     */
    virtual bool Load( wxString& aErrorMsg ) /*override*/;
    
    virtual bool LoadDocs( wxString& aErrorMsg ) /*override*/;
    
private:
    /**
     * Section: ODBC wrapper functions
     * These methods handle error reporting by querying ODBC for all errors and concatenating
     * any errors into a string passed to THROW_IO_ERROR
     */
    /// \brief Reads the config file and populates member variables
    void ReadInit();
    /// \brief Allocates ODBC resources and establishes a connection to an RDBMS
    void ConnectAndGetStatementHandle();
    /// \brief Disconnects from an RDBMS and frees all ODBC resources
    /// \details Called by destructor; do not make virtual.
    void DisconnectAndCleanUp();
    
    /// \brief Concatenates any and all active ODBC error messages to a string result
    wxString GetAllODBCErrors();
    
    /// \brief Concatenates all active ODBC error messages for a specific handle
    wxString GetODBCErrors(SQLSMALLINT handle_type, SQLHANDLE handle);
    
    HENV        m_henv;
    HDBC        m_hdbc;
    HSTMT       m_hstmt;
    wxString    m_connection_string;
    std::map<std::string, std::string>   m_symbol_db_names;
};

#endif
