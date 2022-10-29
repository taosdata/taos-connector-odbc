###############################################################################
# MIT License
#
# Copyright (c) 2022 freemine <freemine@yeah.net>
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
###############################################################################

macro(check_odbc_requirement)
  find_package(FLEX)
  if(NOT FLEX_FOUND)
    message(FATAL_ERROR "you need to install `flex` first")
  endif()
  if(CMAKE_C_COMPILER_ID STREQUAL "GNU" AND CMAKE_C_COMPILER_VERSION VERSION_LESS 5.0.0)
    message(FATAL_ERROR "gcc 4.8.0 will complain too much about flex-generated code, we just bypass building ODBC driver in such case")
  endif()

  find_package(BISON)
  if(NOT BISON_FOUND)
    message(FATAL_ERROR "you need to install `bison` first")
  endif()
  if(CMAKE_C_COMPILER_ID STREQUAL "GNU" AND CMAKE_C_COMPILER_VERSION VERSION_LESS 5.0.0)
    message(FATAL_ERROR "gcc 4.8.0 will complain too much about bison-generated code, we just bypass building ODBC driver in such case")
  endif()

  find_program(TAOS_ODBC_ODBCINST_INSTALLED NAMES odbcinst)
  if(NOT TAOS_ODBC_ODBCINST_INSTALLED)
    if(TAOS_ODBC_DARWIN)
      message(FATAL_ERROR "unixodbc is not installed yet, you may install it under macOS by typing: brew install unixodbc")
    else()
      message(FATAL_ERROR "odbcinst is not installed yet, you may install it under Ubuntu by typing: sudo apt install odbcinst")
    endif()
  endif()

  find_program(TAOS_ODBC_ISQL_INSTALLED NAMES isql)
  if(NOT TAOS_ODBC_ISQL_INSTALLED)
    if(TAOS_ODBC_DARWIN)
      message(FATAL_ERROR "unixodbc is not installed yet, you may install it under macOS by typing: brew install unixodbc")
    else()
      message(FATAL_ERROR "unixodbc is not installed yet, you may install it under Ubuntu by typing: sudo apt install unixodbc")
    endif()
  endif()

  find_program(TAOS_ODBC_PKG_CONFIG_INSTALLED NAMES pkg-config)
  if(NOT TAOS_ODBC_PKG_CONFIG_INSTALLED)
    if(TAOS_ODBC_DARWIN)
      message(FATAL_ERROR "pkg-config is not installed yet, you may install it under macOS by typing: brew install pkg-config")
    else()
      message(FATAL_ERROR "pkg-config is not installed yet, you may install it under Ubuntu by typing: sudo apt install pkg-config")
    endif()
  endif()

  execute_process(COMMAND pkg-config --variable=includedir odbc ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE ODBC_INCLUDE_DIRECTORY)
  execute_process(COMMAND pkg-config --variable=libdir odbc ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE ODBC_LIBRARY_DIRECTORY)
  execute_process(COMMAND pkg-config --libs-only-L odbc ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE ODBC_LINK_OPTIONS)

  execute_process(COMMAND pkg-config --variable=includedir odbcinst ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE ODBCINST_INCLUDE_DIRECTORY)
  execute_process(COMMAND pkg-config --variable=libdir odbcinst ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE ODBCINST_LIBRARY_DIRECTORY)
  execute_process(COMMAND pkg-config --libs-only-L odbcinst ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE ODBCINST_LINK_OPTIONS)

  include(CheckSymbolExists)

  set(CMAKE_REQUIRED_LIBRARIES odbc odbcinst)
  set(CMAKE_REQUIRED_INCLUDES ${ODBC_INCLUDE_DIRECTORY} ${ODBCINST_INCLUDE_DIRECTORY})
  set(CMAKE_REQUIRED_LINK_OPTIONS ${ODBC_LINK_OPTIONS} ${ODBCINST_LINK_OPTIONS})

  check_symbol_exists(SQLExecute "sql.h" HAVE_ODBC_DEV)
  if(NOT HAVE_ODBC_DEV)
    message(FATAL_ERROR "odbc requirement not satisfied, please install unixodbc-dev. Check detail in ${CMAKE_BINARY_DIR}/CMakeFiles/CMakeError.log")
  endif()

  check_symbol_exists(SQLInstallODBC "odbcinst.h" HAVE_ODBCINST_DEV)
  if(NOT HAVE_ODBCINST_DEV)
    message(FATAL_ERROR "odbc requirement not satisfied, check detail in ${CMAKE_BINARY_DIR}/CMakeFiles/CMakeError.log")
  endif()

endmacro()

macro(parser_gen _name)
    set(_parser          ${_name}_parser)
    set(_scanner         ${_name}_scanner)
    set(_src_path        ${CMAKE_CURRENT_SOURCE_DIR})
    set(_dst_path        ${CMAKE_CURRENT_BINARY_DIR})
    set(_src_name        ${_src_path}/${_name})
    set(_dst_name        ${_dst_path}/${_name})
    set(_y               ${_src_name}.y)
    set(_l               ${_src_name}.l)
    set(_tab_c           ${_src_name}_tab.c)
    set(_dst_tab_c       ${_dst_name}.tab.c)
    set(_dst_lex_c       ${_dst_name}.lex.c)
    set(_dst_lex_h       ${_dst_name}.lex.h)

    BISON_TARGET(${_parser} ${_y} ${_dst_tab_c}
        COMPILE_FLAGS "--warnings=error -Dapi.prefix={${_name}_yy}")
    FLEX_TARGET(${_scanner} ${_l} ${_dst_lex_c}
        COMPILE_FLAGS "--header-file=${_dst_lex_h} --prefix=${_name}_yy")
    ADD_FLEX_BISON_DEPENDENCY(${_scanner} ${_parser})

    set(_bison_outputs ${BISON_${_parser}_OUTPUTS})
    set(_flex_outputs ${FLEX_${_scanner}_OUTPUTS})
    set(_outputs ${_bison_outputs} ${_flex_outputs})

    set_source_files_properties(${_tab_c}
            PROPERTY COMPILE_FLAGS "-I${_dst_path}")
    set_source_files_properties(${_tab_c}
            PROPERTY OBJECT_DEPENDS "${_outputs}")

    unset(_bison_outputs)
    unset(_flex_outputs)
    unset(_outputs)

    unset(_parser)
    unset(_scanner)
    unset(_src_path)
    unset(_dst_path)
    unset(_src_name)
    unset(_dst_name)
    unset(_y)
    unset(_l)
    unset(_tab_c)
    unset(_dst_tab_c)
    unset(_dst_lex_c)
    unset(_dst_lex_h)
endmacro()

