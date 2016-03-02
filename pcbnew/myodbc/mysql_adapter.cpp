/**
 * mysql_adapter.cpp
 * 
 * implementation for MySQL ODBC wrapper class
 * 
 * @author cheetah
 */

#include "mysql_adapter.h"
#include "mysql_results.h"
#include <string>


bool MYSQL_ADAPTER::Connect(const connection_parameters & parameters) {
    if(!Init())
        return false;
    std::string connection_string;
    connection_string += "DRIVER={MySQL ODBC 5.3 Driver};SERVER={";
    connection_string += parameters.host;
    if(parameters.port.length() > 0) {
        connection_string += ":";
        connection_string += parameters.port;
    }
    connection_string += "};USER={";
    connection_string += parameters.username;
    connection_string += "};PASSWORD={";
    connection_string += parameters.password;
    connection_string += "};DATABASE={";
    connection_string += parameters.db;
    connection_string += "}";
    return DriverConnect(connection_string);
}

bool MYSQL_ADAPTER::Query(const std::string & query, MYSQL_RESULTS ** results_handle) {
    if(results_handle != 0) {
        if(AllocHandle(SQL_HANDLE_STMT, m_hdbc, &m_hstmt)) {
            *results_handle = new MYSQL_RESULTS_BUILDER(m_hstmt);
            if(ExecDirect(m_hstmt, (SQLCHAR *)query.c_str(), query.length())) {
                return ((MYSQL_RESULTS_BUILDER *)*results_handle)->Prepare(this);
            } else {
                delete *results_handle;
                *results_handle = 0;
                return false;
            }
        } else
            return false;
    } else {
        m_last_errstr = "Null results handle";
        return false;
    }
}

bool MYSQL_ADAPTER::Prepare(const std::string & statement) {
    if(AllocHandle(SQL_HANDLE_STMT, m_hdbc, &m_hstmt)) {
        return ODBC_ADAPTER::Prepare(m_hstmt, (SQLCHAR *)statement.c_str(), statement.length());
    } else
        return false;    
}

bool MYSQL_ADAPTER::Bind(SQLUSMALLINT position, SQLSMALLINT direction, SQLSMALLINT c_type, SQLSMALLINT sql_type, SQLULEN column_size, SQLSMALLINT digits, SQLPOINTER buffer, SQLLEN buffer_length, SQLLEN * indptr)
{
    return SqlOK(SQLBindParameter(m_hstmt, position, direction, c_type, sql_type, column_size, digits, buffer, buffer_length, indptr), SQL_HANDLE_STMT, m_hstmt);
}

bool MYSQL_ADAPTER::Execute() {
    return SqlOK(SQLExecute(m_hstmt), SQL_HANDLE_STMT, m_hstmt);
}