<!-- omit in toc -->
# TDengine ODBC Connector

![GitHub commit activity](https://img.shields.io/github/commit-activity/m/taosdata/taos-connector-odbc)
![GitHub License](https://img.shields.io/github/license/taosdata/taos-connector-odbc)
<br />
[![Twitter Follow](https://img.shields.io/twitter/follow/tdenginedb?label=TDengine&style=social)](https://twitter.com/tdenginedb)
[![YouTube Channel](https://img.shields.io/badge/Subscribe_@tdengine--white?logo=youtube&style=social)](https://www.youtube.com/@tdengine)
[![Discord Community](https://img.shields.io/badge/Join_Discord--white?logo=discord&style=social)](https://discord.com/invite/VZdSuUg4pS)
[![LinkedIn](https://img.shields.io/badge/Follow_LinkedIn--white?logo=linkedin&style=social)](https://www.linkedin.com/company/tdengine)
[![StackOverflow](https://img.shields.io/badge/Ask_StackOverflow--white?logo=stackoverflow&style=social&logoColor=orange)](https://stackoverflow.com/questions/tagged/tdengine)

English | [简体中文](README-CN.md)

## 1. Introduction
`taos-connector-odbc` is the official ODBC connector for TDengine, enabling applications written in various programming languages such as C/C++, C#, Go, Rust, Python, Node.js, and more to interact with the TDengine database. By leveraging the standardized ODBC interface, `taos-connector-odbc` facilitates seamless data writing, querying and parameter binding across different platforms and environments.

The `taos-connector-odbc` supports multiple operating systems, including Windows, Linux, and macOS. 

## 2. Documentation
- To use the ODBC connector, please check [Developer Guide](https://docs.tdengine.com/developer-guide/), which includes how an application can introduce the `taos-connector-odbc`, as well as examples of data writing, querying, and parameter binding across different platforms and environments.
- For other reference information, please check [Reference Manual](https://docs.tdengine.com/tdengine-reference/client-libraries/odbc/), which includes version history, data types, example programs, API descriptions, and FAQs.
- This quick guide is mainly for developers who like to contribute/build/test the ODBC connector by themselves. To learn more about TDengine, you can visit the official documentation.

### 2.1 Features
- **Currently exported ODBC functions are**:

| ODBC/Setup API | linux (ubuntu 22.04) | macosx (ventura 13.2.1) | windows 10/11 | note |
| :----- | :---- | :---- | :---- | :---- |
| ConfigDSN | ❌ | ❌ | ✅ | |
| ConfigDriver | ❌ | ❌ | ✅ | |
| ConfigTranslator | ❌ | ❌ | ✅ | |
| SQLAllocHandle | ✅ | ✅ | ✅ | |
| SQLBindCol  | ✅ | ✅ | ✅ | Column-Wise Binding only |
| SQLBindParameter | ✅ | ✅ | ✅ | Column-Wise Binding only |
| SQLBrowseConnect | ❌ | ❌ | ❌ | |
| SQLBulkOperations | ❌ | ❌ | ❌ | TDengine has no counterpart |
| SQLCloseCursor | ✅ | ✅ | ✅ | |
| SQLColAttribute | ✅ | ✅ | ✅ | |
| SQLColumnPrivileges | ❌ | ❌ | ❌ | TDengine has no strict counterpart |
| SQLColumns | ✅ | ✅ | ✅ | |
| SQLCompleteAsync | ❌ | ❌ | ❌ | |
| SQLConnect | ✅ | ✅ | ✅ | |
| SQLCopyDesc | ❌ | ❌ | ❌ | |
| SQLDescribeCol | ✅ | ✅ | ✅ | |
| SQLDescribeParam | ✅ | ✅ | ✅ | |
| SQLDisconnect | ✅ | ✅ | ✅ | |
| SQLDriverConnect | ✅ | ✅ | ✅ | |
| SQLEndTran | ✅ | ✅ | ✅ | TDengine is non-transactional, thus this is at most simulating |
| SQLExecDirect | ✅ | ✅ | ✅ | |
| SQLExecute | ✅ | ✅ | ✅ | |
| SQLExtendedFetch | ❌ | ❌ | ❌ | |
| SQLFetch | ✅ | ✅ | ✅ | |
| SQLFetchScroll | ✅ | ✅ | ✅ | TDengine has no counterpart, just implement SQL_FETCH_NEXT |
| SQLForeignKeys | ❌ | ❌ | ❌ | TDengine has no counterpart |
| SQLFreeHandle | ✅ | ✅ | ✅ | |
| SQLFreeStmt | ✅ | ✅ | ✅ | |
| SQLGetConnectAttr | ✅ | ✅ | ✅ | |
| SQLGetCursorName | ❌ | ❌ | ❌ | TDengine has no counterpart |
| SQLGetData | ✅ | ✅ | ✅ | |
| SQLGetDescField | ❌ | ❌ | ❌ | |
| SQLGetDescRec | ❌ | ❌ | ❌ | |
| SQLGetDiagField | ✅ | ✅ | ✅ | |
| SQLGetDiagRec | ✅ | ✅ | ✅ | |
| SQLGetEnvAttr | ✅ | ✅ | ✅ | |
| SQLGetInfo | ✅ | ✅ | ✅ | |
| SQLGetStmtAttr | ✅ | ✅ | ✅ | |
| SQLGetTypeInfo | ✅ | ✅ | ✅ | |
| SQLMoreResults | ✅ | ✅ | ✅ | |
| SQLNativeSql | ❌ | ❌ | ❌ | TDengine has no counterpart |
| SQLNumParams | ✅ | ✅ | ✅ | |
| SQLNumResultCols | ✅ | ✅ | ✅ | |
| SQLParamData | ❌ | ❌ | ❌ | TDengine has no counterpart |
| SQLPrepare | ✅ | ✅ | ✅ | |
| SQLPrimaryKeys | ✅ | ✅ | ✅ | |
| SQLProcedureColumns | ❌ | ❌ | ❌ | TDengine has no counterpart |
| SQLProcedures | ❌ | ❌ | ❌ | TDengine has no counterpart |
| SQLPutData | ❌ | ❌ | ❌ | TDengine has no counterpart |
| SQLRowCount | ✅ | ✅ | ✅ | |
| SQLSetConnectAttr | ✅ | ✅ | ✅ | |
| SQLSetCursorName | ❌ | ❌ | ❌ | TDengine has no counterpart |
| SQLSetDescField | ❌ | ❌ | ❌ | |
| SQLSetDescRec | ❌ | ❌ | ❌ | |
| SQLSetEnvAttr | ✅ | ✅ | ✅ | |
| SQLSetPos | ❌ | ❌ | ❌ | TDengine has no counterpart |
| SQLSetStmtAttr | ✅ | ✅ | ✅ | |
| SQLSpecialColumns | ❌ | ❌ | ❌ | TDengine has no counterpart |
| SQLStatistics | ❌ | ❌ | ❌ | TDengine has no counterpart |
| SQLTablePrivileges | ❌ | ❌ | ❌ | TDengine has no strict counterpart |
| SQLTables | ✅ | ✅ | ✅ | |

- **Non-supported-statement-attributes (SQLSetStmtAttr)**

| Attribute | Note |
| :----- | :---- |
| SQL_ATTR_CONCURRENCY | TDengine has no updatable-CURSOR machanism |
| SQL_ATTR_FETCH_BOOKMARK_PTR | TDengine has no BOOKMARK machanism |
| SQL_ATTR_IMP_PARAM_DESC | |
| SQL_ATTR_IMP_ROW_DESC | |
| SQL_ATTR_KEYSET_SIZE | |
| SQL_ATTR_PARAM_BIND_OFFSET_PTR | |
| SQL_ATTR_PARAM_OPERATION_PTR | |
| SQL_ATTR_ROW_NUMBER | Readonly attribute |
| SQL_ATTR_ROW_OPERATION_PTR | |
| SQL_ATTR_SIMULATE_CURSOR | |

- **Non-supported-connection-attributes (SQLSetConnectAttr)**

| Attribute | Note |
| :----- | :---- |
| SQL_ATTR_AUTO_IPD | Readonly attribute |
| SQL_ATTR_CONNECTION_DEAD | Readonly attribute |
| SQL_ATTR_ENLIST_IN_DTC | |
| SQL_ATTR_PACKET_SIZE | |
| SQL_ATTR_TRACE | |
| SQL_ATTR_TRACEFILE | |
| SQL_ATTR_TRANSLATE_LIB | |
| SQL_ATTR_TRANSLATE_OPTION | |

- **Enable any programming language with ODBC-bindings/ODBC-plugings to communicate with TDengine. programming languages listed as follows are demonstrated in test-cases:**

| programming language | ODBC-API or bindings/plugins |
| :----- | :---- |
| C/C++ | ODBC-API |
| CSharp | System.Data.Odbc |
| Erlang | odbc module |
| Go | github.com/alexbrainman/odbc, database/sql |
| Haskell | HDBC, HDBC-odbc |
| Common Lisp | plain-odbc |
| Nodejs | odbc |
| Python3 | pyodbc |
| Rust | odbc |


### 2.2 Layout of source code, directories only
```
<root>
├── benchmark
├── cmake
├── common
├── inc
├── samples
│   └── c
├── sh
├── src
│   ├── core
│   ├── inc
│   ├── os_port
│   ├── parser
│   ├── tests
│   └── utils
├── templates
├── tests
│   ├── c
│   ├── cpp
│   ├── cs
│   ├── erl
│   ├── go
│   ├── hs
│   │   └── app
│   ├── lisp
│   ├── node
│   ├── python
│   ├── R
│   ├── rust
│   │   └── main
│   │       └── src
│   ├── sh
│   ├── taos
│   └── ws
└── valgrind
```

## 3. Prerequisites
First, ensure that TDengine has been deployed locally. For detailed deployment steps, please refer to [Deploy TDengine](https://docs.tdengine.com/get-started/deploy-from-package/). Ensure that both taosd and taosAdapter services are up and running.

Afterwards, before installing and using the `taos-connector-odbc`, ensure that you have met the following prerequisites for your specific platform.
- cmake, 3.16.3 or above, please refer to [cmake](https://cmake.org/).
- flex, 2.6.4 or above. NOTE: win_flex_bison on windows platform to be installed.
- bison, 3.5.1 or above. NOTE: win_flex_bison on windows platform to be installed.
- odbc driver manager, such as unixodbc(2.3.6 or above) in linux. NOTE: odbc driver manager is pre-installed on windows platform.
- iconv, should've been already included in libc. NOTE: win_iconv would be downloaded when building this project.
- valgrind, if you wish to debug and profile executables, such as detecting potential memory leakages.
- node, v12.0 or above if you wish to enable nodejs-test-cases.
  - node odbc, 2.4.4 or above, please refer to [node odbc](https://www.npmjs.com/package/odbc/).
- rust, v1.63 or above if you wish to enable rust-test-cases.
  - odbc, 0.17.0 or above, please refer to [rust odbc](https://docs.rs/odbc/latest/odbc/).
  - env_logger, 0.8.2 or above, please refer to [env_logger](https://docs.rs/env_logger/latest/env_logger/).
  - json, please refer to [json](https://docs.rs/json/latest/json/).
- python3, v3.10 or above if you wish to enable python3-test-cases
  - pyodbc, 4.0.39 or above, please refer to [python odbc](https://pypi.org/project/pyodbc/).
- go, v1.17 or above if you wish to enable go-test-cases
  - odbc, please refer to [go odbc](https://github.com/alexbrainman/odbc).
- erlang, v12.2 or above if you wish to enable erlang-test-cases.
  - odbc, please refer to [erlang odbc](https://www.erlang.org/doc/apps/odbc/getting_started.html).
- haskell, cabal v3.6 or above, ghc v9.2 or above, if you wish to enable haskell-test-cases.
  - hsql-odbc, please refer to [haskell odbc](https://hackage.haskell.org/package/hsql-odbc). 
- common lisp, sbcl v2.1.11 or above if you wish to enable common-lisp-test-cases.
  - plain-odbc, please refer to [common lisp odbc](https://plain-odbc.common-lisp.dev/).
- R, v4.3 or above, if you wish to enable R-test-cases.
  - odbc, please refer to [R odbc](https://cran.r-project.org/web/packages/odbc/index.html).

### 3.1 Windows Platform (Windows 11 Example)
- Install win_flex_bison 2.5.25:
  - Download from: [win_flex_bison-2.5.25.zip](https://github.com/lexxmark/winflexbison/releases/download/v2.5.25/win_flex_bison-2.5.25.zip).
  - Extract the files and add the directory to your system's PATH environment variable.
- Verify Installation:
  ```
  win_flex --version
  ```
- Install ODBC Driver Manager:
  - Ensure that the Microsoft ODBC Driver Manager is installed on your system. It is typically pre-installed on Windows platforms.

### 3.2 Linux Platform (Ubuntu 20.04 Example)
- Install Required Dependencies:
  ```
  sudo apt install flex bison
  ```
- Install ODBC Driver Manager:
  ```
  sudo apt install unixodbc unixodbc-dev
  ```

### 3.3 macOS Platform (macOS Big Sur Example)
- Install Required Dependencies:
  ```
  brew install flex bison
  ```
- Install ODBC Driver Manager:
  ```
  brew install unixodbc
  ```

## 4. Building and Installing
This section provides detailed instructions for building and installing the `taos-connector-odbc` on different platforms.

### 4.1 Windows Platform (Windows 11 Example)
- Open Command Prompt as an Administrator:
  - Follow the instructions provided [here](https://www.makeuseof.com/windows-run-command-prompt-admin/) to open Command Prompt with administrative privileges.
- Change to the root directory of this project:
  ```
  cd path\to\taos-connector-odbc
  ```
- Optionally, setup building environment:
  - If you have installed Visual Studio Community 2022 on a 64-bit Windows platform, run the following command to set up the build environment:
    ```
    "\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
    ```
- Generate make files:
  ```
  cmake --no-warn-unused-cli -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE -B build -G "Visual Studio 17 2022" -A x64
  ```
  - **Troubleshooting**: If compiler errors occur during the following steps, such as <path_to_winbase.h> warning C5105: macro expansion producing 'defined' has undefined behavior, retry with the following command:
    ```
    cmake --no-warn-unused-cli -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE -B build -G "Visual Studio 17 2022" -A x64 -DDISABLE_C5105:BOOL=ON
    ```
- Building the project:
  ```
  cmake --build build --config Debug -j 4
  ```
- Installing connector:
  - This will install taos_odbc.dll into C:\TDengine\taos_odbc\x64\ by default.
    ```
    cmake --install build --config Debug
    cmake --build build --config Debug --target install_templates
    ```
- Verify installation:
  - Check if a new TAOS_ODBC_DSN registry has been set up in the Windows Registry:
    ```
    HKEY_LOCAL_MACHINE\SOFTWARE\ODBC\ODBCINST.INI\TDengine
    HKEY_CURRENT_USER\Software\ODBC\Odbc.ini\TAOS_ODBC_DSN
    ```
### 4.2 Linux Platform (Ubuntu 20.04 Example)
- Change to the root directory of this project:
  ```
  cd path\to\taos-connector-odbc
  ```
- Generate make files:
  ```
  cmake -B debug -DCMAKE_BUILD_TYPE=Debug
  ```
- Build the project:
  ```
  cmake --build debug
  ```
- Install connector:
  ```
  sudo cmake --install debug
  cmake --build debug --target install_templates
  ```
- Confirm installation:
  - Check if the ODBC DSN configuration file (e.g., /etc/odbc.ini or ~/.odbc.ini) contains TAOS_ODBC_DSN entry.

4.3 macOS Platform (Big Sur Example)
- Change to the root directory of this project:
  ```
  cd path\to\taos-connector-odbc
  ```
- Generate make files:
  ```
  cmake -B debug -DCMAKE_BUILD_TYPE=Debug
  ```
- Build the project:
  ```
  cmake --build debug
  ```
- Install connector:
  ```
  sudo cmake --install debug
  cmake --build debug --target install_templates
  ```
- Confirm installation:
  - Check if the ODBC DSN configuration file (e.g., /etc/odbc.ini or ~/.odbc.ini) contains TAOS_ODBC_DSN entry.

## 5. Testing
### 5.1 Test Execution
The ODBC Connector testing framework uses ctest for running test cases. The test cases are located in the tests directory of the project root.

#### 5.1.1 Windows Platform (Windows 11 Example)
- Setup testing environment variables:
  - Set the following environment variables to configure logging levels and destinations:
    ```
    set TAOS_ODBC_LOG_LEVEL=ERROR
    set TAOS_ODBC_LOGGER=stderr
    set TAOS_TEST_CASES=%cd%\tests\taos\taos_test.cases
    set ODBC_TEST_CASES=%cd%\tests\c\odbc_test.cases
    ```
  - Available log levels are: VERBOSE, DEBUG, INFO, WARN, ERROR, FATAL. Lower levels provide more detailed logs.
  - Available loggers are: stderr, temp. stderr logs to standard error, and temp logs to a file in the temporary directory.
- Run the tests:
  ```
  ctest --test-dir build --output-on-failure -C Debug
  ```

#### 5.1.2 Linux Platform (Ubuntu 20.04 Example)
- Setup testing environment variables:
  - Set the following environment variables to configure logging levels and destinations:
    ```
    export TAOS_ODBC_LOG_LEVEL=ERROR
    export TAOS_ODBC_LOGGER=stderr
    export TAOS_TEST_CASES=$(pwd)/tests/taos/taos_test.cases
    export ODBC_TEST_CASES=$(pwd)/tests/c/odbc_test.cases
    ```
  - Available log levels are: VERBOSE, DEBUG, INFO, WARN, ERROR, FATAL. Lower levels provide more detailed logs.
  - Available loggers are: stderr, temp, syslog. stderr logs to standard error, temp logs to a file in the temporary directory, and syslog logs to the system log.
- Run the tests:
  ```
  pushd debug >/dev/null && ctest --output-on-failure && echo -=Done=-; popd >/dev/null
  ```

#### 5.1.3 macOS Platform (Big Sur Example)
- Setup testing environment variables:
  - Set the following environment variables to configure logging levels and destinations:
    ```
    export TAOS_ODBC_LOG_LEVEL=ERROR
    export TAOS_ODBC_LOGGER=stderr
    export TAOS_TEST_CASES=$(pwd)/tests/taos/taos_test.cases
    export ODBC_TEST_CASES=$(pwd)/tests/c/odbc_test.cases
    ```
  - Available log levels are: VERBOSE, DEBUG, INFO, WARN, ERROR, FATAL. Lower levels provide more detailed logs.
  - Available loggers are: stderr, temp. stderr logs to standard error, and temp logs to a file in the temporary directory.
- Run the tests:
  ```
  pushd debug >/dev/null && ctest --output-on-failure && echo -=Done=-; popd >/dev/null
  ```

### 5.2 Test Case Addition
All tests are located in the `./tests/` directory of the project. This directory contains subdirectories for various languages and types of tests. The basic functionality verification test cases are written in C and are located in the `c` and `taos` folders.
You can add new test files or add test cases in existing test files that comply with the testing framework standards.
For C/C++ tests, ensure they follow the CTest framework conventions.
Place new test files in the appropriate directories within the tests directory.

### 5.3 Performance Testing
Performance testing is in progress.

## 6. CI/CD
- [Build Workflow] -TODO
- [Code Coverage] -TODO

## 7. Submitting Issues
We welcome the submission of [GitHub Issue](https://github.com/taosdata/taos-connector-odbc/issues/new?template=Blank+issue). When submitting an issue, please provide the following information to help us diagnose and resolve the problem more efficiently:

### 7.1 Required Information
- Problem Description:
  - Provide a clear and detailed description of the issue you are encountering.
  - Indicate whether the issue occurs consistently or intermittently.
  - If possible, include a detailed call stack or error message that can help in diagnosing the problem.
- ODBC Connector Version or Commit ID:
  - Specify the version or commit id of the ODBC Connector you are using. 
- ODBC Connection Parameters:
  - Provide your ODBC connection string or DSN (Data Source Name) configuration details. Please do not include sensitive information such as usernames and passwords.
  - Example:
    ```
    DSN=TAOS_ODBC_DSN;UID=<your_username>;PWD=<your_password>
    ```
    Replace <your_username> and <your_password> with placeholders or omit them for security reasons.
- TDengine Server Version:
  - Specify the version of the TDengine server you are connecting to. This can be found by running the following command on your TDengine server:
    ```
    taosd -V
    ```
### 7.2 Additional Information (Optional but Helpful)
- Operating System: Specify the operating system and its version (e.g., Windows 11, Ubuntu 20.04, macOS Big Sur).
- Steps to Reproduce: Provide step-by-step instructions on how to reproduce the issue. This helps us replicate and verify the problem.
- Environment Configuration: Include any relevant environment configurations, such as specific settings in odbc.ini, odbcinst.ini, or other configuration files.
- Logs: Attach any relevant logs that might help in diagnosing the issue. Logs can be found in the location specified by your logging configuration (e.g., stderr, temp, or syslog).

## 8. Submitting PRs
We welcome developers to contribute to this project. When submitting Pull Requests (PRs), please follow these steps:

1. Fork the Project:
   - Fork this repository by following the instructions provided [here](https://docs.github.com/en/get-started/quickstart/fork-a-repo).
2. Create a New Branch:
   - Create a new branch from the main branch with a meaningful name that reflects the changes you are making. Do not modify the main branch directly.
      ```
      git checkout -b my_branch
      ```
3. Modify the Code:
   - Make the necessary code modifications. Ensure that all existing unit tests pass and add new unit tests to verify your changes.
4. Push Changes to Remote Branch:
   - Push your changes to the remote branch on your forked repository.
      ```
      git push origin my_branch
      ```
5. Create a Pull Request (PR):
   - Create a Pull Request on GitHub by following the instructions provided [here](https://docs.github.com/en/pull-requests/collaborating-with-pull-requests/proposing-changes-to-your-work-with-pull-requests/creating-a-pull-request).
6. Check CI Status:
   - After submitting the PR, you can find it through the [Pull Requests](https://github.com/taosdata/taos-connector-odbc/pulls) page.
   - Click on the corresponding link to see if the Continuous Integration (CI) checks for your PR have passed. If they have passed, it will display "All checks have passed". Regardless of whether the CI passes or not, you can click "Show all checks" -> "Details" to view the detailed test case logs.
7. Review Test Coverage:
   - After submitting the PR, if the CI checks pass, you can find your PR on the [Codecov](https://app.codecov.io/gh/taosdata/taos-connector-odbc/pulls) page to check the test coverage.
   - Ensure that your changes maintain or improve the overall test coverage.

## 9. References
- [TDengine Official Website](https://www.tdengine.com/) 
- [TDengine GitHub](https://github.com/taosdata/TDengine) 
- [Microsoft Introduction to ODBC](https://learn.microsoft.com/en-us/sql/odbc/reference/introduction-to-odbc)
- [Microsoft ODBC API Reference](https://learn.microsoft.com/en-us/sql/odbc/reference/syntax/odbc-api-reference)

## 10. License
[MIT License](./LICENSE)
- 
## originally initiated by freemine@yeah.net

