/**
 * mysql_results.h
 * 
 * declaration for MySQL ODBC result set class class
 * 
 * @author cheetah
 */

#ifndef MYSQL_RESULTS_H_
#define MYSQL_RESULTS_H_

#include <sqlext.h>
#include <vector>
#include <string>

class MYSQL_ADAPTER;

class MYSQL_RESULTS {
public:
    MYSQL_RESULTS();
    virtual ~MYSQL_RESULTS();

    /// \brief Fetches the next row of data from the RDBMS
    bool Fetch();
    /// \brief Returns a particular column's data as a string value
    bool GetData(SQLUSMALLINT column, std::string & value);
    /// \brief Indicates the number of columns in this result set
    int  Columns();
    
protected:
    SQLHANDLE      m_hstmt;
    int            m_columns;
    std::vector<char *>      m_buffers;
    std::vector<SQLSMALLINT> m_types;
    
    void FreeBuffers();
};


class MYSQL_RESULTS_BUILDER : public MYSQL_RESULTS {
public:
    MYSQL_RESULTS_BUILDER(SQLHANDLE statement_handle);
    virtual ~MYSQL_RESULTS_BUILDER() { }
    bool Prepare(MYSQL_ADAPTER * adapter);
};


#endif
