# ODBC Driver #

- **on-going implementation of ODBC driver for TAOS 3.0**

- **currently exported ODBC functions are**:
```
SQLAllocHandle
SQLBindCol
SQLBindParameter
SQLColAttribute
SQLConnect
SQLDescribeCol
SQLDescribeParam
SQLDisconnect
SQLDriverConnect
SQLEndTran
SQLExecDirect
SQLExecute
SQLFetch
SQLFreeHandle
SQLFreeStmt
SQLGetData
SQLGetDiagField
SQLGetDiagRec
SQLGetInfo
SQLGetPrivateProfileString
SQLNumParams
SQLNumResultCols
SQLPrepare
SQLRowCount
SQLSetConnectAttr
SQLSetEnvAttr
SQLSetStmtAttr
```

- **enable ODBC-aware software to communicate with TAOS, at this very beginning, we support linux only**

- **enable any programming language with ODBC-bindings/ODBC-plugings to communicate with TAOS, potentially**

- **still going on**...

### Supported platform
- Linux

### Requirements
- flex, 2.6.4 or above
- bison, 3.5.1 or above
- odbc driver manager, such as unixodbc(2.3.6 or above) in linux
- iconv, should've been already included in libc
- valgrind, if you wish to debug and profile executables, such as detecting potential memory leakages
- node, if you wish to enable nodejs-test-cases
- rust, if you wish to enable rust-test-cases

### Building under Linux, use Ubuntu as example
```
sudo apt install flex bison unixodbc unixodbc-dev && echo -=Done=-
rm -rf build && cmake -B build -DCMAKE_BUILD_TYPE=Debug && cmake --build build && sudo cmake --install build && echo -=Done=-
```

### Test
```
pushd build >/dev/null && TAOS_TEST_CASES=$(pwd)/../tests/taos/taos_test.cases ODBC_TEST_CASES=$(pwd)/../tests/c/odbc_test.cases ctest --output-on-failure && echo -=Done=-; popd >/dev/null
```

## ODBC references
- https://learn.microsoft.com/en-us/sql/odbc/reference/introduction-to-odbc?view=sql-server-ver16
- https://learn.microsoft.com/en-us/sql/odbc/reference/syntax/odbc-api-reference?view=sql-server-ver16

