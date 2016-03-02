/**
 * odbc_adapter.h
 * 
 * declaration for ODBC wrapper class
 * 
 * @author cheetah
 */

#include <sql.h>
#include <string>
#include <ostream>

#ifndef ODBC_ADAPTER_H_
#define ODBC_ADAPTER_H_

class ODBC_ADAPTER {
public:
    ODBC_ADAPTER();
    virtual ~ODBC_ADAPTER();
    
    /// \brief Sets up the ODBC environment
    /// \returns false if an error occurred
    bool Init();
    
    bool AllocHandle(SQLSMALLINT, SQLHANDLE, SQLHANDLE *);
    bool BindCol(SQLHANDLE, SQLUSMALLINT, SQLSMALLINT, SQLPOINTER, SQLINTEGER, SQLLEN *);
    
    /// \brief Connects using a DSN
    bool Connect(std::string dsn, std::string user, std::string password);
    /// \brief Makes a dsn-less connection
    bool DriverConnect(std::string connection_string);
    /// \brief Disconnects from the database
    bool Disconnect();
    
    bool DescribeCol(SQLHANDLE, SQLSMALLINT, SQLCHAR *, SQLSMALLINT, SQLSMALLINT *, SQLSMALLINT *, SQLULEN *, SQLSMALLINT *, SQLSMALLINT *);
    bool ExecDirect(SQLHANDLE, SQLCHAR *, SQLSMALLINT);
    bool GetData(SQLHANDLE hstmt, SQLUSMALLINT cn, SQLSMALLINT type, SQLPOINTER targ, SQLLEN len, SQLLEN * indptr);
    bool Prepare(SQLHANDLE, SQLCHAR*, SQLSMALLINT);
    bool NumResultCols(SQLHANDLE handle, SQLSMALLINT * num_cols);
    
    /// \brief Returns a string describing the last error that occurred.
    /// \details This string may come from the DM, DB, or this class.
    std::string LastErrorString();
    
    /// \brief Writes all errors to the given output stream
    void DumpAllErrors(std::ostream & target);
    
    /// \brief Lists available drivers to the given output stream
    void ListDrivers(std::ostream & target);
    
protected:
    bool SqlOK(SQLRETURN r, SQLSMALLINT handle_type, SQLHANDLE handle);
    void AppendErrorMessages(SQLSMALLINT handle_type, SQLHANDLE handle);
    
    inline void clear_errors() { m_last_errstr = ""; }
    
    SQLHANDLE   m_henv;
    SQLHANDLE   m_hdbc;
    SQLHANDLE   m_hstmt;
    
    std::string m_last_errstr;
    
    static void DumpErrors(SQLSMALLINT handle_type, SQLHANDLE handle, std::ostream & target);
};


#endif
