/**
 * odbc_adapter.cpp
 * 
 * implementation for ODBC wrapper class
 * 
 * @author cheetah
 */

#include "odbc_adapter.h"
#include <sqlext.h>
#include <memory>
#include <sstream>

#define SQL_OK(x) ((x) == SQL_SUCCESS || (x) == SQL_SUCCESS_WITH_INFO || (x) == SQL_NO_DATA_FOUND)

ODBC_ADAPTER::ODBC_ADAPTER() :
    m_henv(SQL_NULL_HANDLE),
    m_hdbc(SQL_NULL_HDBC),
    m_hstmt(SQL_NULL_HSTMT)
{
}

ODBC_ADAPTER::~ODBC_ADAPTER() {
    //Disconnect();
    SQLFreeHandle(SQL_HANDLE_ENV, m_henv);
}


bool ODBC_ADAPTER::Init() {
    clear_errors();
    SQLRETURN r = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &m_henv);
    if(!SQL_OK(r)) {
        m_last_errstr = "Could not allocate environment handle.";
        return false;
    }
    r = SQLSetEnvAttr(m_henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER) SQL_OV_ODBC3, 0);
    if(!SQL_OK(r)) {
        m_last_errstr = "Could not select ODBC Version 3.";
        return false;
    }
    return true;
}

bool ODBC_ADAPTER::AllocHandle(SQLSMALLINT htype, SQLHANDLE in_handle, SQLHANDLE * out_handle) {
    return SqlOK(SQLAllocHandle(htype, in_handle, out_handle), htype, in_handle);
}

bool ODBC_ADAPTER::BindCol(SQLHANDLE hstmt, SQLUSMALLINT col, SQLSMALLINT type, SQLPOINTER buffer, SQLINTEGER blen, SQLLEN * strlen) 
{
    return SqlOK(SQLBindCol(hstmt, col, type, buffer, blen, strlen), SQL_HANDLE_STMT, hstmt);
}


bool ODBC_ADAPTER::Connect(std::string host, std::string user, std::string password) {
    clear_errors();
    if(m_hdbc != SQL_NULL_HDBC) {
        m_last_errstr = "Already connected.";
        return false;
    }
    m_last_errstr = "";
    SQLRETURN r = SQLAllocHandle(SQL_HANDLE_DBC, m_henv, &m_hdbc);
    if(!SQL_OK(r)) {
        m_last_errstr = "Could not allocate database handle.";
        return false;
    }
    std::string connection_string;
//    connection_string
    return SQL_ERROR;
    
}

bool ODBC_ADAPTER::DriverConnect(std::string connection_string) {
    clear_errors();
    if(m_hdbc != SQL_NULL_HDBC) {
        m_last_errstr = "Already connected.";
        return false;
    }
    m_last_errstr = "";
    SQLRETURN r = SQLAllocHandle(SQL_HANDLE_DBC, m_henv, &m_hdbc);
    if(!SqlOK(r, SQL_HANDLE_ENV, m_henv)) {
        return false;
    }

    r= SQLDriverConnect(
        m_hdbc,
        NULL,
        (SQLCHAR *)connection_string.c_str(),
        connection_string.length(),
        NULL,
        0,
        NULL,
        SQL_DRIVER_NOPROMPT);
    return SqlOK(r, SQL_HANDLE_DBC, m_hdbc);
}

bool ODBC_ADAPTER::Disconnect() {
    clear_errors();
    if(m_hdbc != SQL_NULL_HDBC) {
        if(!SqlOK(SQLDisconnect(m_hdbc), SQL_HANDLE_DBC, m_hdbc)) {
            return false;
        }
        SQLFreeHandle(SQL_HANDLE_DBC, m_hdbc);
        m_hdbc = SQL_NULL_HANDLE;
    }
    return true;
}

bool ODBC_ADAPTER::DescribeCol(SQLHANDLE hstmt, SQLSMALLINT colnum, SQLCHAR * namebuf, SQLSMALLINT bufLen, 
                               SQLSMALLINT * nameLength, SQLSMALLINT * dataType, SQLULEN * columnSize, 
                               SQLSMALLINT * decDigits, SQLSMALLINT * nullable) 
{
    
    return SqlOK(SQLDescribeCol(hstmt, colnum, namebuf, bufLen, nameLength, dataType, columnSize, decDigits, nullable), SQL_HANDLE_STMT, hstmt);
}

bool ODBC_ADAPTER::ExecDirect(SQLHANDLE hstmt, SQLCHAR * query, SQLSMALLINT strlen) {
    return SqlOK(SQLExecDirect(hstmt, query, strlen), SQL_HANDLE_STMT, hstmt);
}

bool ODBC_ADAPTER::GetData(SQLHANDLE hstmt, SQLUSMALLINT cn, SQLSMALLINT type, SQLPOINTER targ, SQLLEN len, SQLLEN * indptr) {
    clear_errors();
    return SqlOK(SQLGetData(hstmt, cn, type, targ, len, indptr), SQL_HANDLE_STMT, hstmt);
}

bool ODBC_ADAPTER::NumResultCols(SQLHANDLE handle, SQLSMALLINT * num_cols) {
    clear_errors();
    return SqlOK(SQLNumResultCols(handle, num_cols), SQL_HANDLE_STMT, handle);
}

bool ODBC_ADAPTER::Prepare(SQLHANDLE hstmt, SQLCHAR* query, SQLSMALLINT len) {
    clear_errors();
    return SqlOK(SQLPrepare(hstmt, query, len), SQL_HANDLE_STMT, hstmt);
}

std::string ODBC_ADAPTER::LastErrorString() {
    return m_last_errstr;
}


void ODBC_ADAPTER::DumpAllErrors(std::ostream & target) {
    if(m_hstmt != SQL_NULL_HSTMT) {
        DumpErrors(SQL_HANDLE_STMT, m_hstmt, target);
    }
    if(m_hdbc != SQL_NULL_HDBC) {
        DumpErrors(SQL_HANDLE_DBC, m_hdbc, target);
    }
    if(m_henv != SQL_NULL_HANDLE) {
        DumpErrors(SQL_HANDLE_ENV, m_henv, target);
    }
    target << "Internal error: " << m_last_errstr << std::endl;
}

void ODBC_ADAPTER::ListDrivers(std::ostream & target) {
    SQLCHAR driver[128];
    SQLCHAR attrs[256];
    SQLRETURN r = SQLDrivers(
        m_henv,
        SQL_FETCH_FIRST,
        driver,
        128,
        0,
        attrs,
        256,
        0);
    if(SQL_OK(r)) {
        target << "Driver: " << (char *)driver << std::endl;
        target << "  Attributes: " << (char *)attrs << std::endl;
        while(SQL_NO_DATA !=
            SQLDrivers(
                m_henv,
                SQL_FETCH_NEXT,
                driver,
                128,
                0,
                attrs,
                256,
                0)) 
        {
            target << "Driver: " << (char *)driver << std::endl;
            target << "  Attributes: " << (char *)attrs << std::endl;
        }
    }
}

bool ODBC_ADAPTER::SqlOK(SQLRETURN r, SQLSMALLINT handle_type, SQLHANDLE handle) {
    if(r != SQL_SUCCESS && r != SQL_NO_DATA_FOUND)
        AppendErrorMessages(handle_type, handle);
    return SQL_OK(r);
}

void ODBC_ADAPTER::AppendErrorMessages(SQLSMALLINT handle_type, SQLHANDLE handle) {
    std::stringstream out; 
    DumpAllErrors(out);
    m_last_errstr += out.str();
    //     SQLCHAR sqlstate[6] = {0,0,0,0,0,0};
    //     SQLINTEGER native_error = 0;
    //     std::auto_ptr<SQLCHAR> message;
    //     SQLSMALLINT message_length = 0;
    //     SQLRETURN r = SQL_SUCCESS;
    //     for(int rec = 1; r == SQL_SUCCESS; ++rec) {
    //         r = SQLGetDiagRec(
    //             handle_type,
    //             handle,
    //             rec,
    //             sqlstate,
    //             &native_error,
    //             NULL,
    //             0, 
    //             &message_length);
    //         if(r == SQL_SUCCESS || r == SQL_SUCCESS_WITH_INFO) {
    //             message.reset(new SQLCHAR[message_length + 1]);
    //             r = SQLGetDiagRec(
    //                 handle_type,
    //                 handle,
    //                 rec,
    //                 sqlstate,
    //                 &native_error,
    //                 message.get(),
    //                 message_length + 1, 
    //                 &message_length);
    //             if(SQL_OK(r)) {
    //                 m_last_errstr += (const char *)message.get();
    //                 m_last_errstr += "\n";
    //             }
    //         }
    //             
    //     }
}

void ODBC_ADAPTER::DumpErrors(SQLSMALLINT handle_type, SQLHANDLE handle, std::ostream & target) {
    SQLCHAR sqlstate[6] = {0,0,0,0,0,0};
    SQLINTEGER native_error = 0;
    std::auto_ptr<SQLCHAR> message;
    SQLSMALLINT message_length = 0;
    SQLRETURN r = 0;
    for(int rec = 1; r == SQL_SUCCESS; ++rec) {
        r = SQLGetDiagRec(
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
            r = SQLGetDiagRec(
                handle_type,
                handle,
                rec,
                sqlstate,
                &native_error,
                message.get(),
                message_length + 1, 
                &message_length);
            if(SQL_OK(r)) {
                target << "SQLSTATE: " << sqlstate << std::endl;
                target << "Native error: " << native_error << std::endl;
                target << "Message: " << (char *)message.get() << std::endl;
            }
        }
            
    }
}