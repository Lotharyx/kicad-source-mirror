#ifdef WINDOWS
#include "stdafx.h"
#endif

#include "ODBCWrapper.h"



/***********************************************
* Constructors / Destructors                   *
***********************************************/

ODBCWrapper::
ODBCWrapper() {
	_currHenv = 0;
	_currHdbc = 0;
	_currHstmt = 0;
	_suppressWarnings = FALSE;
    _argEnd = _argHead = 0;
}


ODBCWrapper::
~ODBCWrapper() {
    // chase down and delete all arg holders.
    LLARGNODE * an = _argHead;
    while(an != 0) {
        free(an->arg);
        LLARGNODE * n = an->next;
        free(an);
        an = n;
    }
}



/***********************************************
* ODBC Wrapper methods                         *
***********************************************/

SQLRETURN ODBCWrapper::
AllocHandle(SQLSMALLINT htype, SQLHANDLE inHandle, SQLHANDLE * outHandle) {
	SQLRETURN ret = SQLAllocHandle(htype, inHandle, outHandle);
	process_error(ret, htype, inHandle);

    // Store the newly-allocated handle as the "Current" handle of that type
	switch(htype) {
		case SQL_HANDLE_ENV:
			_currHenv = *outHandle;
			break;

		case SQL_HANDLE_DBC:
			_currHdbc = *outHandle;
			break;
		
		case SQL_HANDLE_DESC:
			_currHdesc = *outHandle;
			break;

		case SQL_HANDLE_STMT:
			_currHstmt = *outHandle;
			break;
	}
	return ret;
}


SQLRETURN ODBCWrapper::
BindCol(SQLHANDLE hstmt, SQLUSMALLINT col, SQLSMALLINT type, SQLPOINTER buffer, SQLINTEGER blen, SQLLEN * strlen) {
	SQLRETURN ret = SQLBindCol(hstmt, col, type, buffer, blen, strlen);
	process_error(ret, SQL_HANDLE_STMT, hstmt);
	return ret;
}



SQLRETURN ODBCWrapper:: 
BindParameter(SQLHANDLE hstmt, SQLUSMALLINT pnum, SQLSMALLINT iotype, SQLSMALLINT valtype, SQLSMALLINT parmtype, 
			  SQLUINTEGER colsize, SQLSMALLINT digits, SQLPOINTER buffer, SQLINTEGER blen, SQLINTEGER * strlen) {
	SQLRETURN ret = SQLBindParameter(hstmt, pnum, iotype, valtype, parmtype, colsize, digits, buffer, blen, strlen);
	process_error(ret, SQL_HANDLE_STMT, hstmt);
	return ret;
}


SQLRETURN ODBCWrapper::
Connect(SQLHANDLE hdbc, SQLCHAR * dsn, SQLSMALLINT dsnlen, SQLCHAR * user, SQLSMALLINT userlen, SQLCHAR * auth, SQLSMALLINT authlen) {
	SQLRETURN ret = SQLConnect(hdbc, dsn, dsnlen, user, userlen, auth, authlen);
	process_error(ret, SQL_HANDLE_DBC, hdbc);
    process_error(ret, SQL_HANDLE_ENV, _currHenv);
	return ret;
}



SQLRETURN ODBCWrapper::
DescribeCol(SQLHANDLE hstmt, SQLSMALLINT colnum, SQLCHAR * namebuf, SQLSMALLINT bufLen, 
            SQLSMALLINT * nameLength, SQLSMALLINT * dataType, SQLUINTEGER * columnSize, 
			SQLSMALLINT * decDigits, SQLSMALLINT * nullable) {
	
	SQLRETURN ret = SQLDescribeCol(hstmt, colnum, namebuf, bufLen, nameLength, dataType, columnSize, decDigits, nullable);
	process_error(ret, SQL_HANDLE_STMT, hstmt);
	return ret;
}


SQLRETURN ODBCWrapper::
DescribeParam(SQLHANDLE hstmt, SQLUSMALLINT pnum, SQLSMALLINT * type, SQLUINTEGER * size, SQLSMALLINT * digits, SQLSMALLINT * nullable) {
	SQLRETURN ret = SQLDescribeParam(hstmt, pnum, type, size, digits, nullable);
	process_error(ret, SQL_HANDLE_STMT, hstmt);
	return ret;
}


SQLRETURN ODBCWrapper::
Disconnect(SQLHANDLE hdbc) {
	if(hdbc != 0) {  // Only act if handle is valid
		SQLRETURN ret = SQLDisconnect(hdbc);
		process_error(ret, SQL_HANDLE_DBC, hdbc);
		return ret;
	} else
		return SQL_SUCCESS;
}


SQLRETURN ODBCWrapper::
ExecDirect(SQLHANDLE hstmt, SQLCHAR * q, SQLSMALLINT l) {
	printf("ExecDirect \"%s\"\n", (const char *)q);
	SQLRETURN ret = SQLExecDirect(hstmt, q, l);
	process_error(ret, SQL_HANDLE_STMT, hstmt);
	return ret;
}


SQLRETURN ODBCWrapper:: 
Execute(SQLHANDLE hstmt) {
	SQLRETURN ret = SQLExecute(hstmt);
	process_error(ret, SQL_HANDLE_STMT, hstmt);
	return ret;
}



SQLRETURN ODBCWrapper::
ExtendedFetch(SQLHANDLE hstmt, SQLUSMALLINT orient, SQLINTEGER offset, SQLUINTEGER * rowcountptr, SQLUSMALLINT * statarray) {
	SQLRETURN ret = SQLExtendedFetch(hstmt, orient, offset, rowcountptr, statarray);
	process_error(ret, SQL_HANDLE_STMT, hstmt);
	return ret;
}



SQLRETURN ODBCWrapper::
Fetch(SQLHANDLE hstmt) {
	SQLRETURN ret = SQLFetch(hstmt);
	process_error(ret, SQL_HANDLE_STMT, hstmt);
	return ret;
}


SQLRETURN ODBCWrapper::
FreeHandle(SQLSMALLINT hType, SQLHANDLE handle) {
	if(handle != 0) {	// Only act if this is a valid handle.
		SQLRETURN ret = SQLFreeHandle(hType, handle);
		process_error(ret, hType, handle);
		return ret;
	} else 
		return SQL_SUCCESS;
}


SQLRETURN ODBCWrapper::
FreeStmt(SQLHANDLE hstmt, SQLUSMALLINT option) {
	if(hstmt != 0) { // Only act if handle is valid.
		SQLRETURN ret = SQLFreeStmt(hstmt, option);
		process_error(ret, SQL_HANDLE_STMT, hstmt);
		return ret;
	} else
		return SQL_SUCCESS;
}


SQLRETURN ODBCWrapper::
GetConnectAttr(SQLHANDLE hdbc, SQLINTEGER attr, SQLPOINTER out, SQLINTEGER blen, SQLINTEGER * outln) {
	SQLRETURN ret = SQLGetConnectAttr(hdbc, attr, out, blen, outln);
	process_error(ret, SQL_HANDLE_DBC, hdbc);
	return ret;
}


SQLRETURN ODBCWrapper::
GetData(SQLHANDLE hstmt, SQLUSMALLINT cn, SQLSMALLINT type, SQLPOINTER targ, SQLINTEGER len, SQLINTEGER * indptr) {
	SQLRETURN ret = SQLGetData(hstmt, cn, type, targ, len, indptr);
	process_error(ret, SQL_HANDLE_STMT, hstmt);
	return ret;
}


SQLRETURN ODBCWrapper::
GetInfo(SQLHANDLE hdbc, SQLUSMALLINT type, SQLPOINTER out, SQLSMALLINT blen, SQLSMALLINT * outln) {
	SQLRETURN ret = SQLGetInfo(hdbc, type, out, blen, outln);
	process_error(ret, SQL_HANDLE_DBC, hdbc);
	return ret;
}


SQLRETURN ODBCWrapper::
MoreResults(SQLHANDLE hstmt) {
	SQLRETURN ret = SQLMoreResults(hstmt);
	process_error(ret, SQL_HANDLE_STMT, hstmt);
	return ret;
}


SQLRETURN ODBCWrapper::
NumParams(SQLHANDLE hstmt, SQLSMALLINT * parmCount) {
	SQLRETURN ret = SQLNumParams(hstmt, parmCount);
	process_error(ret, SQL_HANDLE_STMT, hstmt);
	return ret;
}


SQLRETURN ODBCWrapper::
NumResultCols(SQLHANDLE hstmt, SQLSMALLINT * targ) {
	SQLRETURN ret = SQLNumResultCols(hstmt, targ);
	process_error(ret, SQL_HANDLE_STMT, hstmt);
	return ret;
}


SQLRETURN ODBCWrapper::
ParamData(SQLHANDLE hstmt, SQLPOINTER * vpp) {
	SQLRETURN ret = SQLParamData(hstmt, vpp);
	process_error(ret, SQL_HANDLE_STMT, hstmt);
	return ret;
}


SQLRETURN ODBCWrapper::
Prepare(SQLHANDLE hstmt, SQLCHAR * txt, SQLSMALLINT l) {
	printf("Prepare \"%s\"\n", (const char *)txt);
	SQLRETURN ret = SQLPrepare(hstmt, txt, l);
	process_error(ret, SQL_HANDLE_STMT, hstmt);
	return ret;
}


SQLRETURN ODBCWrapper::
Procedures(SQLHANDLE hstmt, SQLCHAR* cn, SQLSMALLINT cnl, SQLCHAR* sn, SQLSMALLINT snl, SQLCHAR* pn, SQLSMALLINT pnl) {
#ifdef ODBCW_VERBOSE
	printf("Call: SQLProcedures\n");
	printf("HSTMT address:  0x%x\n", (long long) hstmt);
	if(cn)
		printf("Catalog name:   %s [%d]\n", cn, cnl);
	else
		printf("Catalog name:   <NULL>\n");
	if(sn)
		printf("Schema name:    %s [%d]\n", sn, snl);
	else
		printf("Schema name:    <NULL>\n");
	if(pn)
		printf("Procedure name: %s [%d]\n", pn, pnl);
	else
		printf("Procedure name: <NULL>\n");
#endif
	SQLRETURN ret = SQLProcedures(hstmt, cn, cnl, sn, snl, pn, pnl);
	process_error(ret, SQL_HANDLE_STMT, hstmt);
	return ret;
}


SQLRETURN ODBCWrapper::
ProcedureColumns(SQLHANDLE hstmt, SQLCHAR* cn, SQLSMALLINT cnl, SQLCHAR* sn, SQLSMALLINT snl, SQLCHAR* pn, SQLSMALLINT pnl, SQLCHAR* coln, SQLSMALLINT colnl) {
#ifdef ODBCW_VERBOSE
	printf("Call: ProcedureColumns\n");
	printf("HSTMT address: %x\n", hstmt);
	if(cn)
		printf("Catalog name:   %s [%d]\n", cn, cnl);
	else
		printf("Catalog name:   <NULL>\n");
	if(sn)
		printf("Schema name:    %s [%d]\n", sn, snl);
	else
		printf("Schema name:    <NULL>\n");
	if(pn)
		printf("Procedure name: %s [%d]\n", pn, pnl);
	else
		printf("Procedure name: <NULL>\n");
	if(coln)
		printf("Column name:    %s [%d]\n", coln, colnl);
	else
		printf("Column name:    <NULL>\n");
#endif
	SQLRETURN ret = SQLProcedureColumns(hstmt, cn, cnl, sn, snl, pn, pnl, coln, colnl);
	process_error(ret, SQL_HANDLE_STMT, hstmt);
	return ret;
}


SQLRETURN ODBCWrapper::
PutData(SQLHANDLE hstmt, SQLPOINTER ptr, SQLINTEGER soi) {
	SQLRETURN ret = SQLPutData(hstmt, ptr, soi);
	process_error(ret, SQL_HANDLE_STMT, hstmt);
	return ret;
}


SQLRETURN ODBCWrapper::
SetEnvAttr(SQLHANDLE henv, SQLINTEGER attr, SQLPOINTER value, SQLINTEGER len) {
	SQLRETURN ret = SQLSetEnvAttr(henv, attr, value, len);
	process_error(ret, SQL_HANDLE_ENV, henv);
	return ret;
}

SQLRETURN ODBCWrapper::
SetConnectAttr(SQLHANDLE hdbc, SQLINTEGER attr, SQLPOINTER value, SQLINTEGER len) {
	SQLRETURN ret = SQLSetConnectAttr(hdbc, attr, value, len);
	process_error(ret, SQL_HANDLE_DBC, hdbc);
	return ret;
}

SQLRETURN ODBCWrapper::
SetStmtAttr(SQLHANDLE hstmt, SQLINTEGER attr, SQLPOINTER value, SQLINTEGER len) {
	SQLRETURN ret = SQLSetStmtAttr(hstmt, attr, value, len);
	process_error(ret, SQL_HANDLE_STMT, hstmt);
	return ret;
}



/***********************************************
* Error-management methods                     *
***********************************************/

void ODBCWrapper::
process_error(SQLRETURN ret, SQLSMALLINT handletype, SQLHANDLE handle) {
  if(ret == SQL_SUCCESS || ret == SQL_NO_DATA_FOUND)
	  return;
  if(_suppressWarnings && ret == SQL_SUCCESS_WITH_INFO)
	  return;
  SQLCHAR       ebuf[SQL_MAX_MESSAGE_LENGTH];
  SQLINTEGER	nativer = 0;
  SQLSMALLINT	mesgl = 0;
  SQLRETURN		err = SQL_SUCCESS;
  SQLCHAR       sqlstate[24]; 

  memset(ebuf, 0, sizeof(ebuf));
  memset(sqlstate, 0, sizeof(sqlstate));

  SQLSMALLINT recNo = 1;
  while(err == SQL_SUCCESS) {
	  err = SQLGetDiagRec(handletype, handle, recNo++, sqlstate,
						  (SQLINTEGER *) &nativer,
						  (SQLCHAR*) ebuf, SQL_MAX_MESSAGE_LENGTH - 1,
						  &mesgl);
	  if(err == SQL_SUCCESS) {
          printf("SQLState: %s, Native Error: %d\n", sqlstate, nativer);
          printf("Error message: %s\n", ebuf);
      }
  }

  memset(ebuf, 0, sizeof(ebuf));
  err = mesgl = 0;
  nativer = 0;
}



void ODBCWrapper::
PrintDBCInfo() {
	SQLCHAR buffer[255] = "";  SQLSMALLINT retcount = 0;
	GetInfo(_currHdbc, SQL_DATA_SOURCE_NAME, buffer, 255, &retcount);
	printf("DSN:              %s\n", buffer);
	GetInfo(_currHdbc, SQL_DRIVER_NAME, buffer, 255, &retcount);
	printf("Driver:           %s\n", buffer);
	GetInfo(_currHdbc, SQL_DRIVER_VER, buffer, 255, &retcount);
	printf("Driver version:   %s\n", buffer);
	GetInfo(_currHdbc, SQL_DBMS_NAME, buffer, 255, &retcount);
	printf("Database name:    %s\n", buffer);
	GetInfo(_currHdbc, SQL_DBMS_VER, buffer, 255, &retcount);
	printf("Database version: %s\n", buffer);
}



/***********************************************
* Command-line parsing methods                 *
***********************************************/

void ODBCWrapper::
AddArg_s(const char * arg, char * target, int tlen) {
    // Allocate a new argnode.
    LLARGNODE * n = (LLARGNODE *) malloc(sizeof(LLARGNODE));
    memset(n, 0, sizeof(n));

    // Allocate new storage to hold argument string.
    char * a = (char *) malloc(strlen(arg) + 1);  // Don't forget room for the null
    strcpy(a, arg);

    // Populate argument node
    n->arg = a;
    n->target = target;
    n->tlen = tlen;
    n->next = 0;

    // If the head node is null, set it to point to this one.
    if(_argHead == 0)
        _argHead = n;

    // If the tail node is not null, point its next pointer at the new argnode
    if(_argEnd)
        _argEnd->next = n;

    // Point the tail node to this argnode
    _argEnd = n;
}


void ODBCWrapper::
ProcessArgs(int argc, char ** argv) {
    for(int i = 1; i < argc; i++) {
        // For each argument on the command line, chase down the argument list for matches.
        LLARGNODE * n = _argHead;
        while(n) {
            int ld;
            if(0 == substrcmp(n->arg, argv[i], &ld)) {   // If it's a matching argument...
                if(n->arg[strlen(n->arg) - 1] == '=') {      // If the arg ends with an =, it's a k/v so chop off the key
                    substr_s(n->target, n->tlen, argv[i], strlen(n->arg), -1);
                } else {                                 // If the arg doesn't end with "=", it's a flag
                    if(ld == 0)                              // Only copy if it's an exact length match
                        substr_s(n->target, n->tlen, argv[i], 0, -1);
                }
            }
            n = n->next;
        }
    }
}


void ODBCWrapper::
HexPrint(SQLSMALLINT grouping, void * ptr, SQLINTEGER len) {
    unsigned char * in = (unsigned char *) ptr;
    for(int p = 0; p < len; p++) {
        printf("0x");
        for(int q = 0; q < grouping; q++) {
            printf("%02x", *(in + ((p * grouping) + q)));
        }
        if(p % 8 == 0) {
            printf("\n");
        } else {
            printf(" ");
        }
    }
    printf("\n");
}

// substring function
void substr_s(char * target, int tlen, const char * source, int start, int len) {
    int i = 0;
    if(len == -1)
        len = strlen(source);
    for(; (i < tlen) && ((start + i) < len); i++) {
        *(target + i) = *(source + start + i);
    }
    if(i < tlen)
        *(target + i) = '\0';
}


int substrcmp(const char * a, const char * b, int * ldiff = 0) {
    int d = 0;
    int al = strlen(a);
    int bl = strlen(b);
    for(int i = 0; (i < al) && (i < bl); i++)
        d += *(a + i) - *(b + i);
    
    if(ldiff)
        * ldiff = al - bl;

    return d;
}

