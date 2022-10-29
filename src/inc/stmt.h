#ifndef _stmt_h_
#define _stmt_h_

#include "conn.h"

EXTERN_C_BEGIN

typedef struct stmt_s              stmt_t;

stmt_t* stmt_create(conn_t *conn) FA_HIDDEN;
stmt_t* stmt_ref(stmt_t *stmt) FA_HIDDEN;
stmt_t* stmt_unref(stmt_t *stmt) FA_HIDDEN;
SQLRETURN stmt_free(stmt_t *stmt) FA_HIDDEN;
void stmt_clr_errs(stmt_t *stmt) FA_HIDDEN;

SQLRETURN stmt_exec_direct(stmt_t *stmt, const char *sql, int len) FA_HIDDEN;
SQLRETURN stmt_get_row_count(stmt_t *stmt, SQLLEN *row_count_ptr) FA_HIDDEN;
SQLRETURN stmt_get_col_count(stmt_t *stmt, SQLSMALLINT *col_count_ptr) FA_HIDDEN;

SQLRETURN stmt_describe_col(stmt_t *stmt,
    SQLUSMALLINT   ColumnNumber,
    SQLCHAR       *ColumnName,
    SQLSMALLINT    BufferLength,
    SQLSMALLINT   *NameLengthPtr,
    SQLSMALLINT   *DataTypePtr,
    SQLULEN       *ColumnSizePtr,
    SQLSMALLINT   *DecimalDigitsPtr,
    SQLSMALLINT   *NullablePtr) FA_HIDDEN;
SQLRETURN stmt_bind_col(stmt_t *stmt,
    SQLUSMALLINT   ColumnNumber,
    SQLSMALLINT    TargetType,
    SQLPOINTER     TargetValuePtr,
    SQLLEN         BufferLength,
    SQLLEN        *StrLen_or_IndPtr) FA_HIDDEN;

SQLRETURN stmt_fetch(stmt_t *stmt) FA_HIDDEN;

SQLRETURN stmt_get_diag_rec(
    stmt_t         *stmt,
    SQLSMALLINT     RecNumber,
    SQLCHAR        *SQLState,
    SQLINTEGER     *NativeErrorPtr,
    SQLCHAR        *MessageText,
    SQLSMALLINT     BufferLength,
    SQLSMALLINT    *TextLengthPtr) FA_HIDDEN;

SQLRETURN stmt_get_data(
    stmt_t        *stmt,
    SQLUSMALLINT   Col_or_Param_Num,
    SQLSMALLINT    TargetType,
    SQLPOINTER     TargetValuePtr,
    SQLLEN         BufferLength,
    SQLLEN        *StrLen_or_IndPtr) FA_HIDDEN;

SQLRETURN stmt_prepare(stmt_t *stmt,
    SQLCHAR      *StatementText,
    SQLINTEGER    TextLength) FA_HIDDEN;

SQLRETURN stmt_get_num_params(
    stmt_t         *stmt,
    SQLSMALLINT    *ParameterCountPtr) FA_HIDDEN;

SQLRETURN stmt_describe_param(
    stmt_t         *stmt,
    SQLUSMALLINT    ParameterNumber,
    SQLSMALLINT    *DataTypePtr,
    SQLULEN        *ParameterSizePtr,
    SQLSMALLINT    *DecimalDigitsPtr,
    SQLSMALLINT    *NullablePtr) FA_HIDDEN;

SQLRETURN stmt_bind_param(
    stmt_t         *stmt,
    SQLUSMALLINT    ParameterNumber,
    SQLSMALLINT     InputOutputType,
    SQLSMALLINT     ValueType,
    SQLSMALLINT     ParameterType,
    SQLULEN         ColumnSize,
    SQLSMALLINT     DecimalDigits,
    SQLPOINTER      ParameterValuePtr,
    SQLLEN          BufferLength,
    SQLLEN         *StrLen_or_IndPtr) FA_HIDDEN;

SQLRETURN stmt_execute(
    stmt_t         *stmt) FA_HIDDEN;

void stmt_dissociate_APD(stmt_t *stmt) FA_HIDDEN;
void stmt_dissociate_ARD(stmt_t *stmt) FA_HIDDEN;

SQLRETURN stmt_set_attr(stmt_t *stmt, SQLINTEGER Attribute, SQLPOINTER ValuePtr, SQLINTEGER StringLength) FA_HIDDEN;

SQLRETURN stmt_free_stmt(stmt_t *stmt, SQLUSMALLINT Option) FA_HIDDEN;

EXTERN_C_END

#endif //  _stmt_h_
