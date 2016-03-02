/**
 * mysql_adapter.h
 * 
 * implementation for MySQL ODBC wrapper class
 * 
 * @author cheetah
 */

#include <mysql_results.h>
#include <mysql_adapter.h>


MYSQL_RESULTS::MYSQL_RESULTS() {
    m_hstmt = 0;
}

MYSQL_RESULTS::~MYSQL_RESULTS() {
    SQLFreeHandle(SQL_HANDLE_STMT, m_hstmt);
    FreeBuffers();
}

bool MYSQL_RESULTS::Fetch() {
    return SQLFetch(m_hstmt) != SQL_NO_DATA;
}

bool MYSQL_RESULTS::GetData(SQLUSMALLINT column, std::string & value) {
    value = m_buffers[column - 1];
    return true;
}

int MYSQL_RESULTS::Columns() {
    return m_columns;
}

void MYSQL_RESULTS::FreeBuffers() {
    std::vector<char *>::iterator it = m_buffers.begin();
    for(; it != m_buffers.end(); ++it) {
        delete *it;
    }
    m_buffers.clear();
    m_types.clear();
}

MYSQL_RESULTS_BUILDER::MYSQL_RESULTS_BUILDER(HSTMT stmt) {
    m_hstmt = stmt;
}

bool MYSQL_RESULTS_BUILDER::Prepare(MYSQL_ADAPTER * adapter) {
    SQLSMALLINT column_count;
    if(adapter->NumResultCols(m_hstmt, &column_count)) {
        m_columns = column_count;
        m_buffers.resize(m_columns);
        m_types.resize(m_columns);
        for(int col = 1; col <= m_columns; ++col) {
            SQLSMALLINT sql_type;
            SQLULEN col_size = 0;
            if(adapter->DescribeCol(m_hstmt, col, 0, 0, 0, &sql_type, &col_size, 0, 0)) {
                m_types[col - 1] = sql_type;
                m_buffers[col - 1] = new char[col_size + 1];
                if(!adapter->BindCol(m_hstmt, col, SQL_C_CHAR, (SQLCHAR *)m_buffers[col - 1], col_size, 0)) {
                    FreeBuffers();
                    return false;
                }
            } else {
                FreeBuffers();
                return false;
            }
        }
        return true;
    }
    return false;
}

