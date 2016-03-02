/**
 * mysql_adapter.h
 * 
 * declaration for MySQL ODBC wrapper class
 * 
 * @author cheetah
 */

#include "odbc_adapter.h"

#ifndef MYSQL_ADAPTER_H_
#define MYSQL_ADAPTER_H_

class MYSQL_RESULTS;

class MYSQL_ADAPTER : public ODBC_ADAPTER {
public:
    struct connection_parameters {
        std::string host;
        std::string port;
        std::string username;
        std::string password;
        std::string db;
    };
    
    MYSQL_ADAPTER() { }
    virtual ~MYSQL_ADAPTER() { }
    
    bool Connect(const connection_parameters & parameters);
    
    bool Query(const std::string & query, MYSQL_RESULTS ** results_handle);
    bool Prepare(const std::string & query);
    bool Bind(SQLUSMALLINT position, SQLSMALLINT direction, SQLSMALLINT c_type, SQLSMALLINT sql_type, SQLULEN column_size, SQLSMALLINT digits, SQLPOINTER buffer, SQLLEN buffer_length, SQLLEN * indptr);
    bool Execute();
    bool Fetch();
    void FreeStmt() { SQLFreeHandle(SQL_HANDLE_STMT, m_hstmt); }
    
    std::string LastErrorString() { return ODBC_ADAPTER::LastErrorString(); }
    
private:
};

#endif
