/*
 * MIT License
 *
 * Copyright (c) 2022 freemine <freemine@yeah.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "internal.h"

#include "typesinfo.h"

#include "errs.h"
#include "log.h"
#include "taos_helpers.h"
#include "tsdb.h"

#include <errno.h>

static const column_meta_t _typesinfo_meta[] = {
  {
    /* 1 */
    /* name                            */ "TYPE_NAME",
    /* column_type_name                */ "VARCHAR",
    /* SQL_DESC_CONCISE_TYPE           */ SQL_VARCHAR,
    /* SQL_DESC_LENGTH                 */ 1024,               /* hard-coded, big enough */
    /* SQL_DESC_OCTET_LENGTH           */ 1024,               /* hard-coded, big enough */
    /* SQL_DESC_PRECISION              */ 1024,
    /* SQL_DESC_SCALE                  */ 0,
    /* SQL_DESC_AUTO_UNIQUE_VALUE      */ SQL_FALSE,
    /* SQL_DESC_UPDATABLE              */ SQL_ATTR_READONLY,
    /* SQL_DESC_NULLABLE               */ SQL_NO_NULLS,
    /* SQL_DESC_UNSIGNED               */ SQL_TRUE,
    /* SQL_DESC_NUM_PREC_RADIX         */ 0,
  },{
    /* 2 */
    /* name                            */ "DATA_TYPE",
    /* column_type_name                */ "SMALLINT",         /* FIXME: what to fill? */
    /* SQL_DESC_CONCISE_TYPE           */ SQL_SMALLINT,
    /* SQL_DESC_LENGTH                 */ 5,                  /* FIXME: what to fill? */
    /* SQL_DESC_OCTET_LENGTH           */ 2,                  /* hard-coded, big enough */
    /* SQL_DESC_PRECISION              */ 5,
    /* SQL_DESC_SCALE                  */ 0,
    /* SQL_DESC_AUTO_UNIQUE_VALUE      */ SQL_FALSE,
    /* SQL_DESC_UPDATABLE              */ SQL_ATTR_READONLY,
    /* SQL_DESC_NULLABLE               */ SQL_NO_NULLS,
    /* SQL_DESC_UNSIGNED               */ SQL_FALSE,          /* NOTE: check it later */
    /* SQL_DESC_NUM_PREC_RADIX         */ 10,                 /* NOTE: check it later */
  },{
    /* 3 */
    /* name                            */ "COLUMN_SIZE",
    /* column_type_name                */ "INT",              /* FIXME: what to fill? */
    /* SQL_DESC_CONCISE_TYPE           */ SQL_INTEGER,
    /* SQL_DESC_LENGTH                 */ 10,                 /* FIXME: what to fill? */
    /* SQL_DESC_OCTET_LENGTH           */ 4,                  /* hard-coded, big enough */
    /* SQL_DESC_PRECISION              */ 10,
    /* SQL_DESC_SCALE                  */ 0,
    /* SQL_DESC_AUTO_UNIQUE_VALUE      */ SQL_FALSE,
    /* SQL_DESC_UPDATABLE              */ SQL_ATTR_READONLY,
    /* SQL_DESC_NULLABLE               */ SQL_NULLABLE,
    /* SQL_DESC_UNSIGNED               */ SQL_FALSE,          /* NOTE: check it later */
    /* SQL_DESC_NUM_PREC_RADIX         */ 10,                 /* NOTE: check it later */
  },{
    /* 4 */
    /* name                            */ "LITERAL_PREFIX",
    /* column_type_name                */ "VARCHAR",
    /* SQL_DESC_CONCISE_TYPE           */ SQL_VARCHAR,
    /* SQL_DESC_LENGTH                 */ 1024,               /* hard-coded, big enough */
    /* SQL_DESC_OCTET_LENGTH           */ 1024,               /* hard-coded, big enough */
    /* SQL_DESC_PRECISION              */ 1024,
    /* SQL_DESC_SCALE                  */ 0,
    /* SQL_DESC_AUTO_UNIQUE_VALUE      */ SQL_FALSE,
    /* SQL_DESC_UPDATABLE              */ SQL_ATTR_READONLY,
    /* SQL_DESC_NULLABLE               */ SQL_NULLABLE,
    /* SQL_DESC_UNSIGNED               */ SQL_TRUE,
    /* SQL_DESC_NUM_PREC_RADIX         */ 0,
  },{
    /* 5 */
    /* name                            */ "LITERAL_SUFFIX",
    /* column_type_name                */ "VARCHAR",
    /* SQL_DESC_CONCISE_TYPE           */ SQL_VARCHAR,
    /* SQL_DESC_LENGTH                 */ 1024,               /* hard-coded, big enough */
    /* SQL_DESC_OCTET_LENGTH           */ 1024,               /* hard-coded, big enough */
    /* SQL_DESC_PRECISION              */ 1024,
    /* SQL_DESC_SCALE                  */ 0,
    /* SQL_DESC_AUTO_UNIQUE_VALUE      */ SQL_FALSE,
    /* SQL_DESC_UPDATABLE              */ SQL_ATTR_READONLY,
    /* SQL_DESC_NULLABLE               */ SQL_NULLABLE,
    /* SQL_DESC_UNSIGNED               */ SQL_TRUE,
    /* SQL_DESC_NUM_PREC_RADIX         */ 0,
  },{
    /* 6 */
    /* name                            */ "CREATE_PARAMS",
    /* column_type_name                */ "VARCHAR",
    /* SQL_DESC_CONCISE_TYPE           */ SQL_VARCHAR,
    /* SQL_DESC_LENGTH                 */ 1024,               /* hard-coded, big enough */
    /* SQL_DESC_OCTET_LENGTH           */ 1024,               /* hard-coded, big enough */
    /* SQL_DESC_PRECISION              */ 1024,
    /* SQL_DESC_SCALE                  */ 0,
    /* SQL_DESC_AUTO_UNIQUE_VALUE      */ SQL_FALSE,
    /* SQL_DESC_UPDATABLE              */ SQL_ATTR_READONLY,
    /* SQL_DESC_NULLABLE               */ SQL_NULLABLE,
    /* SQL_DESC_UNSIGNED               */ SQL_TRUE,           /* NOTE: check it later */
    /* SQL_DESC_NUM_PREC_RADIX         */ 0,
  },{
    /* 7 */
    /* name                            */ "NULLABLE",
    /* column_type_name                */ "SMALLINT",         /* FIXME: what to fill? */
    /* SQL_DESC_CONCISE_TYPE           */ SQL_SMALLINT,
    /* SQL_DESC_LENGTH                 */ 5,                  /* FIXME: what to fill? */
    /* SQL_DESC_OCTET_LENGTH           */ 2,                  /* hard-coded, big enough */
    /* SQL_DESC_PRECISION              */ 5,
    /* SQL_DESC_SCALE                  */ 0,
    /* SQL_DESC_AUTO_UNIQUE_VALUE      */ SQL_FALSE,
    /* SQL_DESC_UPDATABLE              */ SQL_ATTR_READONLY,
    /* SQL_DESC_NULLABLE               */ SQL_NO_NULLS,
    /* SQL_DESC_UNSIGNED               */ SQL_FALSE,          /* NOTE: check it later */
    /* SQL_DESC_NUM_PREC_RADIX         */ 10,                 /* NOTE: check it later */
  },{
    /* 8 */
    /* name                            */ "CASE_SENSITIVE",
    /* column_type_name                */ "SMALLINT",         /* FIXME: what to fill? */
    /* SQL_DESC_CONCISE_TYPE           */ SQL_SMALLINT,
    /* SQL_DESC_LENGTH                 */ 5,                  /* FIXME: what to fill? */
    /* SQL_DESC_OCTET_LENGTH           */ 2,                  /* hard-coded, big enough */
    /* SQL_DESC_PRECISION              */ 5,
    /* SQL_DESC_SCALE                  */ 0,
    /* SQL_DESC_AUTO_UNIQUE_VALUE      */ SQL_FALSE,
    /* SQL_DESC_UPDATABLE              */ SQL_ATTR_READONLY,
    /* SQL_DESC_NULLABLE               */ SQL_NO_NULLS,
    /* SQL_DESC_UNSIGNED               */ SQL_FALSE,          /* NOTE: check it later */
    /* SQL_DESC_NUM_PREC_RADIX         */ 10,                 /* NOTE: check it later */
  },{
    /* 9 */
    /* name                            */ "SEARCHABLE",
    /* column_type_name                */ "SMALLINT",         /* FIXME: what to fill? */
    /* SQL_DESC_CONCISE_TYPE           */ SQL_SMALLINT,
    /* SQL_DESC_LENGTH                 */ 5,                  /* FIXME: what to fill? */
    /* SQL_DESC_OCTET_LENGTH           */ 2,                  /* hard-coded, big enough */
    /* SQL_DESC_PRECISION              */ 5,
    /* SQL_DESC_SCALE                  */ 0,
    /* SQL_DESC_AUTO_UNIQUE_VALUE      */ SQL_FALSE,
    /* SQL_DESC_UPDATABLE              */ SQL_ATTR_READONLY,
    /* SQL_DESC_NULLABLE               */ SQL_NO_NULLS,
    /* SQL_DESC_UNSIGNED               */ SQL_FALSE,          /* NOTE: check it later */
    /* SQL_DESC_NUM_PREC_RADIX         */ 10,                 /* NOTE: check it later */
  },{
    /* 10 */
    /* name                            */ "UNSIGNED_ATTRIBUTE",
    /* column_type_name                */ "SMALLINT",         /* FIXME: what to fill? */
    /* SQL_DESC_CONCISE_TYPE           */ SQL_SMALLINT,
    /* SQL_DESC_LENGTH                 */ 5,                  /* FIXME: what to fill? */
    /* SQL_DESC_OCTET_LENGTH           */ 2,                  /* hard-coded, big enough */
    /* SQL_DESC_PRECISION              */ 5,
    /* SQL_DESC_SCALE                  */ 0,
    /* SQL_DESC_AUTO_UNIQUE_VALUE      */ SQL_FALSE,
    /* SQL_DESC_UPDATABLE              */ SQL_ATTR_READONLY,  /* FIXME: SQL_ATTR_READWRITE_UNKNOWN */
    /* SQL_DESC_NULLABLE               */ SQL_NULLABLE,
    /* SQL_DESC_UNSIGNED               */ SQL_FALSE,          /* NOTE: check it later */
    /* SQL_DESC_NUM_PREC_RADIX         */ 10,                 /* NOTE: check it later */
  },{
    /* 11 */
    /* name                            */ "FIXED_PREC_SCALE",
    /* column_type_name                */ "SMALLINT",         /* FIXME: what to fill? */
    /* SQL_DESC_CONCISE_TYPE           */ SQL_SMALLINT,
    /* SQL_DESC_LENGTH                 */ 5,                  /* FIXME: what to fill? */
    /* SQL_DESC_OCTET_LENGTH           */ 2,                  /* hard-coded, big enough */
    /* SQL_DESC_PRECISION              */ 5,
    /* SQL_DESC_SCALE                  */ 0,
    /* SQL_DESC_AUTO_UNIQUE_VALUE      */ SQL_FALSE,
    /* SQL_DESC_UPDATABLE              */ SQL_ATTR_READONLY,  /* FIXME: SQL_ATTR_READWRITE_UNKNOWN */
    /* SQL_DESC_NULLABLE               */ SQL_NO_NULLS,
    /* SQL_DESC_UNSIGNED               */ SQL_FALSE,          /* NOTE: check it later */
    /* SQL_DESC_NUM_PREC_RADIX         */ 10,                 /* NOTE: check it later */
  },{
    /* 12 */
    /* name                            */ "AUTO_UNIQUE_VALUE",
    /* column_type_name                */ "SMALLINT",         /* FIXME: what to fill? */
    /* SQL_DESC_CONCISE_TYPE           */ SQL_SMALLINT,
    /* SQL_DESC_LENGTH                 */ 5,                  /* FIXME: what to fill? */
    /* SQL_DESC_OCTET_LENGTH           */ 2,                  /* hard-coded, big enough */
    /* SQL_DESC_PRECISION              */ 5,
    /* SQL_DESC_SCALE                  */ 0,
    /* SQL_DESC_AUTO_UNIQUE_VALUE      */ SQL_FALSE,
    /* SQL_DESC_UPDATABLE              */ SQL_ATTR_READONLY,  /* FIXME: SQL_ATTR_READWRITE_UNKNOWN */
    /* SQL_DESC_NULLABLE               */ SQL_NULLABLE,
    /* SQL_DESC_UNSIGNED               */ SQL_FALSE,          /* NOTE: check it later */
    /* SQL_DESC_NUM_PREC_RADIX         */ 10,                 /* NOTE: check it later */
  },{
    /* 13 */
    /* name                            */ "LOCAL_TYPE_NAME",
    /* column_type_name                */ "VARCHAR",
    /* SQL_DESC_CONCISE_TYPE           */ SQL_VARCHAR,
    /* SQL_DESC_LENGTH                 */ 1024,               /* hard-coded, big enough */
    /* SQL_DESC_OCTET_LENGTH           */ 1024,               /* hard-coded, big enough */
    /* SQL_DESC_PRECISION              */ 1024,
    /* SQL_DESC_SCALE                  */ 0,
    /* SQL_DESC_AUTO_UNIQUE_VALUE      */ SQL_FALSE,
    /* SQL_DESC_UPDATABLE              */ SQL_ATTR_READONLY,
    /* SQL_DESC_NULLABLE               */ SQL_NULLABLE,
    /* SQL_DESC_UNSIGNED               */ SQL_TRUE,           /* NOTE: check it later */
    /* SQL_DESC_NUM_PREC_RADIX         */ 0,
  },{
    /* 14 */
    /* name                            */ "MINIMUM_SCALE",
    /* column_type_name                */ "SMALLINT",         /* FIXME: what to fill? */
    /* SQL_DESC_CONCISE_TYPE           */ SQL_SMALLINT,
    /* SQL_DESC_LENGTH                 */ 5,                  /* FIXME: what to fill? */
    /* SQL_DESC_OCTET_LENGTH           */ 2,                  /* hard-coded, big enough */
    /* SQL_DESC_PRECISION              */ 5,
    /* SQL_DESC_SCALE                  */ 0,
    /* SQL_DESC_AUTO_UNIQUE_VALUE      */ SQL_FALSE,
    /* SQL_DESC_UPDATABLE              */ SQL_ATTR_READONLY,
    /* SQL_DESC_NULLABLE               */ SQL_NULLABLE,
    /* SQL_DESC_UNSIGNED               */ SQL_FALSE,          /* NOTE: check it later */
    /* SQL_DESC_NUM_PREC_RADIX         */ 10,                 /* NOTE: check it later */
  },{
    /* 15 */
    /* name                            */ "MAXIMUM_SCALE",
    /* column_type_name                */ "SMALLINT",         /* FIXME: what to fill? */
    /* SQL_DESC_CONCISE_TYPE           */ SQL_SMALLINT,
    /* SQL_DESC_LENGTH                 */ 5,                  /* FIXME: what to fill? */
    /* SQL_DESC_OCTET_LENGTH           */ 2,                  /* hard-coded, big enough */
    /* SQL_DESC_PRECISION              */ 5,
    /* SQL_DESC_SCALE                  */ 0,
    /* SQL_DESC_AUTO_UNIQUE_VALUE      */ SQL_FALSE,
    /* SQL_DESC_UPDATABLE              */ SQL_ATTR_READONLY,
    /* SQL_DESC_NULLABLE               */ SQL_NULLABLE,
    /* SQL_DESC_UNSIGNED               */ SQL_FALSE,          /* NOTE: check it later */
    /* SQL_DESC_NUM_PREC_RADIX         */ 10,                 /* NOTE: check it later */
  },{
    /* 16 */
    /* name                            */ "SQL_DATA_TYPE",
    /* column_type_name                */ "SMALLINT",         /* FIXME: what to fill? */
    /* SQL_DESC_CONCISE_TYPE           */ SQL_SMALLINT,
    /* SQL_DESC_LENGTH                 */ 5,                  /* FIXME: what to fill? */
    /* SQL_DESC_OCTET_LENGTH           */ 2,                  /* hard-coded, big enough */
    /* SQL_DESC_PRECISION              */ 5,
    /* SQL_DESC_SCALE                  */ 0,
    /* SQL_DESC_AUTO_UNIQUE_VALUE      */ SQL_FALSE,
    /* SQL_DESC_UPDATABLE              */ SQL_ATTR_READONLY,
    /* SQL_DESC_NULLABLE               */ SQL_NO_NULLS,
    /* SQL_DESC_UNSIGNED               */ SQL_FALSE,          /* NOTE: check it later */
    /* SQL_DESC_NUM_PREC_RADIX         */ 10,                 /* NOTE: check it later */
  },{
    /* 17 */
    /* name                            */ "SQL_DATETIME_SUB",
    /* column_type_name                */ "SMALLINT",         /* FIXME: what to fill? */
    /* SQL_DESC_CONCISE_TYPE           */ SQL_SMALLINT,
    /* SQL_DESC_LENGTH                 */ 5,                  /* FIXME: what to fill? */
    /* SQL_DESC_OCTET_LENGTH           */ 2,                  /* hard-coded, big enough */
    /* SQL_DESC_PRECISION              */ 5,
    /* SQL_DESC_SCALE                  */ 0,
    /* SQL_DESC_AUTO_UNIQUE_VALUE      */ SQL_FALSE,
    /* SQL_DESC_UPDATABLE              */ SQL_ATTR_READONLY,
    /* SQL_DESC_NULLABLE               */ SQL_NULLABLE,
    /* SQL_DESC_UNSIGNED               */ SQL_FALSE,          /* NOTE: check it later */
    /* SQL_DESC_NUM_PREC_RADIX         */ 10,                 /* NOTE: check it later */
  },{
    /* 18 */
    /* name                            */ "NUM_PREC_RADIX",
    /* column_type_name                */ "INT",              /* FIXME: what to fill? */
    /* SQL_DESC_CONCISE_TYPE           */ SQL_INTEGER,
    /* SQL_DESC_LENGTH                 */ 10,                 /* FIXME: what to fill? */
    /* SQL_DESC_OCTET_LENGTH           */ 4,                  /* hard-coded, big enough */
    /* SQL_DESC_PRECISION              */ 10,
    /* SQL_DESC_SCALE                  */ 0,
    /* SQL_DESC_AUTO_UNIQUE_VALUE      */ SQL_FALSE,
    /* SQL_DESC_UPDATABLE              */ SQL_ATTR_READONLY,
    /* SQL_DESC_NULLABLE               */ SQL_NULLABLE,
    /* SQL_DESC_UNSIGNED               */ SQL_FALSE,          /* NOTE: check it later */
    /* SQL_DESC_NUM_PREC_RADIX         */ 10,                 /* NOTE: check it later */
  },{
    /* 19 */
    /* name                            */ "INTERVAL_PRECISION",
    /* column_type_name                */ "SMALLINT",         /* FIXME: what to fill? */
    /* SQL_DESC_CONCISE_TYPE           */ SQL_SMALLINT,
    /* SQL_DESC_LENGTH                 */ 5,                  /* FIXME: what to fill? */
    /* SQL_DESC_OCTET_LENGTH           */ 2,                  /* hard-coded, big enough */
    /* SQL_DESC_PRECISION              */ 5,
    /* SQL_DESC_SCALE                  */ 0,
    /* SQL_DESC_AUTO_UNIQUE_VALUE      */ SQL_FALSE,
    /* SQL_DESC_UPDATABLE              */ SQL_ATTR_READONLY,
    /* SQL_DESC_NULLABLE               */ SQL_NULLABLE,
    /* SQL_DESC_UNSIGNED               */ SQL_FALSE,          /* NOTE: check it later */
    /* SQL_DESC_NUM_PREC_RADIX         */ 10,                 /* NOTE: check it later */
  },
};

typedef struct type_info_rec_s                  type_info_rec_t;
struct type_info_rec_s {
  /*  1 */ const char                *TYPE_NAME;
  /*  2 */ int64_t                    DATA_TYPE;
  /*  3 */ int64_t                    COLUMN_SIZE;
  /*  4 */ const char                *LITERAL_PREFIX;
  /*  5 */ const char                *LITERAL_SUFFIX;
  /*  6 */ const char                *CREATE_PARAMS;
  /*  7 */ int64_t                    NULLABLE;
  /*  8 */ int64_t                    CASE_SENSITIVE;
  /*  9 */ int64_t                    SEARCHABLE;
  /* 10 */ int64_t                    UNSIGNED_ATTRIBUTE;
  /* 11 */ int64_t                    FIXED_PREC_SCALE;
  /* 12 */ int64_t                    AUTO_UNIQUE_VALUE;
  /* 13 */ const char                *LOCAL_TYPE_NAME;
  /* 14 */ int64_t                    MINIMUM_SCALE;
  /* 15 */ int64_t                    MAXIMUM_SCALE;
  /* 16 */ int64_t                    SQL_DATA_TYPE;
  /* 17 */ int64_t                    SQL_DATETIME_SUB;
  /* 18 */ int64_t                    NUM_PREC_RADIX;
  /* 19 */ int64_t                    INTERVAL_PRECISION;
};

// NOTE: this is tricky approach
#define NULL_FIELD       ((int64_t)0x8000000000000000L)
static const type_info_rec_t _records[] = {
  {
    /* TYPE_NAME             */      "VARCHAR",
    /* DATA_TYPE             */      SQL_VARCHAR,
    /* COLUMN_SIZE           */      16384,            // NOTE: hard-coded
    /* LITERAL_PREFIX        */      "'",
    /* LITERAL_SUFFIX        */      "'",
    /* CREATE_PARAMS         */      "max length",     // NOTE: what does `max` means?
    /* NULLABLE              */      SQL_NULLABLE,
    /* CASE_SENSITIVE        */      SQL_FALSE,
    /* SEARCHABLE            */      SQL_SEARCHABLE,
    /* UNSIGNED_ATTRIBUTE    */      NULL_FIELD,
    /* FIXED_PREC_SCALE      */      SQL_FALSE,
    /* AUTO_UNIQUE_VALUE     */      NULL_FIELD,
    /* LOCAL_TYPE_NAME       */      "VARCHAR",
    /* MINIMUM_SCALE         */      NULL_FIELD,
    /* MAXIMUM_SCALE         */      NULL_FIELD,
    /* SQL_DATA_TYPE         */      SQL_VARCHAR,
    /* SQL_DATETIME_SUB      */      NULL_FIELD,
    /* NUM_PREC_RADIX        */      NULL_FIELD,
    /* INTERVAL_PRECISION    */      NULL_FIELD,
  },{
    /* TYPE_NAME             */      "INT",
    /* DATA_TYPE             */      SQL_INTEGER,
    /* COLUMN_SIZE           */      10,
    /* LITERAL_PREFIX        */      NULL,
    /* LITERAL_SUFFIX        */      NULL,
    /* CREATE_PARAMS         */      NULL,
    /* NULLABLE              */      SQL_NULLABLE,
    /* CASE_SENSITIVE        */      SQL_FALSE,
    /* SEARCHABLE            */      SQL_PRED_BASIC,
    /* UNSIGNED_ATTRIBUTE    */      SQL_FALSE,
    /* FIXED_PREC_SCALE      */      SQL_FALSE,
    /* AUTO_UNIQUE_VALUE     */      SQL_FALSE,
    /* LOCAL_TYPE_NAME       */      "INT",
    /* MINIMUM_SCALE         */      SQL_FALSE,
    /* MAXIMUM_SCALE         */      SQL_FALSE,
    /* SQL_DATA_TYPE         */      SQL_INTEGER,
    /* SQL_DATETIME_SUB      */      NULL_FIELD,
    /* NUM_PREC_RADIX        */      10,
    /* INTERVAL_PRECISION    */      NULL_FIELD,
  },{
    /* TYPE_NAME             */      "TIMESTAMP",
    /* DATA_TYPE             */      SQL_BINARY,
    /* COLUMN_SIZE           */      8,
    /* LITERAL_PREFIX        */      "0x",
    /* LITERAL_SUFFIX        */      NULL,
    /* CREATE_PARAMS         */      NULL,
    /* NULLABLE              */      SQL_NO_NULLS,
    /* CASE_SENSITIVE        */      SQL_FALSE,
    /* SEARCHABLE            */      SQL_PRED_BASIC,
    /* UNSIGNED_ATTRIBUTE    */      NULL_FIELD,
    /* FIXED_PREC_SCALE      */      SQL_FALSE,
    /* AUTO_UNIQUE_VALUE     */      NULL_FIELD,
    /* LOCAL_TYPE_NAME       */      "TIMESTAMP",
    /* MINIMUM_SCALE         */      NULL_FIELD,
    /* MAXIMUM_SCALE         */      NULL_FIELD,
    /* SQL_DATA_TYPE         */      SQL_BINARY,
    /* SQL_DATETIME_SUB      */      NULL_FIELD,
    /* NUM_PREC_RADIX        */      NULL_FIELD,
    /* INTERVAL_PRECISION    */      NULL_FIELD,
  }
};

SQLSMALLINT typesinfo_get_count_of_col_meta(void)
{
  SQLSMALLINT nr = (SQLSMALLINT)(sizeof(_typesinfo_meta) / sizeof(_typesinfo_meta[0]));
  return nr;
}

const column_meta_t* typesinfo_get_col_meta(int i_col)
{
  int nr = (int)(sizeof(_typesinfo_meta) / sizeof(_typesinfo_meta[0]));
  if (i_col < 0) return NULL;
  if (i_col >= nr) return NULL;
  return _typesinfo_meta + i_col;
}

void typesinfo_reset(typesinfo_t *typesinfo)
{
  if (!typesinfo) return;
}

void typesinfo_release(typesinfo_t *typesinfo)
{
  if (!typesinfo) return;
  typesinfo_reset(typesinfo);

  typesinfo->owner = NULL;
}

static SQLRETURN _query(stmt_base_t *base, const char *sql)
{
  (void)sql;

  typesinfo_t *typesinfo = (typesinfo_t*)base;
  (void)typesinfo;
  stmt_append_err(typesinfo->owner, "HY000", 0, "General error:internal logic error");
  return SQL_ERROR;
}

static SQLRETURN _execute(stmt_base_t *base)
{
  typesinfo_t *typesinfo = (typesinfo_t*)base;
  (void)typesinfo;
  stmt_append_err(typesinfo->owner, "HY000", 0, "General error:internal logic error");
  return SQL_ERROR;
}

static SQLRETURN _fetch_row(stmt_base_t *base)
{
  typesinfo_t *typesinfo = (typesinfo_t*)base;

again:

  if (typesinfo->pos >= sizeof(_records)/sizeof(_records[0])) return SQL_NO_DATA;

  typesinfo->pos += 1;

  if (typesinfo->data_type != SQL_ALL_TYPES) {
    const type_info_rec_t *rec = _records + typesinfo->pos - 1;
    if (rec->DATA_TYPE != typesinfo->data_type) {
      goto again;
    }
  }

  return SQL_SUCCESS;
}

static SQLRETURN _describe_param(stmt_base_t *base,
    SQLUSMALLINT    ParameterNumber,
    SQLSMALLINT    *DataTypePtr,
    SQLULEN        *ParameterSizePtr,
    SQLSMALLINT    *DecimalDigitsPtr,
    SQLSMALLINT    *NullablePtr)
{
  (void)ParameterNumber;
  (void)DataTypePtr;
  (void)ParameterSizePtr;
  (void)DecimalDigitsPtr;
  (void)NullablePtr;

  typesinfo_t *typesinfo = (typesinfo_t*)base;
  (void)typesinfo;
  stmt_append_err(typesinfo->owner, "HY000", 0, "General error:not implemented yet");
  return SQL_ERROR;
}

static SQLRETURN _describe_col(stmt_base_t *base,
    SQLUSMALLINT   ColumnNumber,
    SQLCHAR       *ColumnName,
    SQLSMALLINT    BufferLength,
    SQLSMALLINT   *NameLengthPtr,
    SQLSMALLINT   *DataTypePtr,
    SQLULEN       *typesinfoizePtr,
    SQLSMALLINT   *DecimalDigitsPtr,
    SQLSMALLINT   *NullablePtr)
{
  typesinfo_t *typesinfo = (typesinfo_t*)base;

  const column_meta_t *col_meta = typesinfo_get_col_meta(ColumnNumber - 1);

  *NameLengthPtr    = (SQLSMALLINT)strlen(col_meta->name);
  *DataTypePtr      = (SQLSMALLINT)col_meta->DESC_CONCISE_TYPE;
  *typesinfoizePtr    = col_meta->DESC_OCTET_LENGTH;
  *DecimalDigitsPtr = 0;
  *NullablePtr      = (SQLSMALLINT)col_meta->DESC_NULLABLE;

  int n = snprintf((char*)ColumnName, BufferLength, "%s", col_meta->name);
  if (n < 0 || n >= BufferLength) {
    stmt_append_err(typesinfo->owner, "01004", 0, "String data, right truncated");
    return SQL_SUCCESS_WITH_INFO;
  }

  return SQL_SUCCESS;
}

static SQLRETURN _col_attribute(stmt_base_t *base,
    SQLUSMALLINT    ColumnNumber,
    SQLUSMALLINT    FieldIdentifier,
    SQLPOINTER      CharacterAttributePtr,
    SQLSMALLINT     BufferLength,
    SQLSMALLINT    *StringLengthPtr,
    SQLLEN         *NumericAttributePtr)
{
  (void)ColumnNumber;
  (void)FieldIdentifier;
  (void)CharacterAttributePtr;
  (void)BufferLength;
  (void)StringLengthPtr;
  (void)NumericAttributePtr;
  int n = 0;
  typesinfo_t *typesinfo = (typesinfo_t*)base;
  (void)typesinfo;
  const column_meta_t *col_meta = typesinfo_get_col_meta(ColumnNumber - 1);
  if (!col_meta) {
    stmt_append_err_format(typesinfo->owner, "HY000", 0, "General error:column[%d] out of range", ColumnNumber);
    return SQL_ERROR;
  }

  switch(FieldIdentifier) {
    case SQL_DESC_CONCISE_TYPE:
      *NumericAttributePtr = col_meta->DESC_CONCISE_TYPE;
      OW("Column%d:[SQL_DESC_CONCISE_TYPE]:[%d]%s", ColumnNumber, (int)*NumericAttributePtr, sql_data_type((SQLSMALLINT)*NumericAttributePtr));
      return SQL_SUCCESS;
    case SQL_DESC_OCTET_LENGTH:
      *NumericAttributePtr = col_meta->DESC_OCTET_LENGTH;
      OW("Column%d:[SQL_DESC_OCTET_LENGTH]:%d", ColumnNumber, (int)*NumericAttributePtr);
      return SQL_SUCCESS;
    // https://learn.microsoft.com/en-us/sql/odbc/reference/syntax/sqlcolattribute-function?view=sql-server-ver16#backward-compatibility
    case SQL_DESC_PRECISION:
    case SQL_COLUMN_PRECISION:
      *NumericAttributePtr = col_meta->DESC_PRECISION;
      OW("Column%d:[SQL_DESC_PRECISION]:%d", ColumnNumber, (int)*NumericAttributePtr);
      return SQL_SUCCESS;
    // https://learn.microsoft.com/en-us/sql/odbc/reference/syntax/sqlcolattribute-function?view=sql-server-ver16#backward-compatibility
    case SQL_DESC_SCALE:
    case SQL_COLUMN_SCALE:
      *NumericAttributePtr = col_meta->DESC_SCALE;
      OW("Column%d:[SQL_DESC_SCALE]:%d", ColumnNumber, (int)*NumericAttributePtr);
      return SQL_SUCCESS;
    case SQL_DESC_AUTO_UNIQUE_VALUE:
      *NumericAttributePtr = col_meta->DESC_AUTO_UNIQUE_VALUE;
      OW("Column%d:[SQL_DESC_AUTO_UNIQUE_VALUE]:%d", ColumnNumber, (int)*NumericAttributePtr);
      return SQL_SUCCESS;
    case SQL_DESC_UPDATABLE:
      *NumericAttributePtr = col_meta->DESC_UPDATABLE;
      OW("Column%d:[SQL_DESC_UPDATABLE]:[%d]%s", ColumnNumber, (int)*NumericAttributePtr, sql_updatable(*NumericAttributePtr));
      return SQL_SUCCESS;
    case SQL_DESC_NULLABLE:
      *NumericAttributePtr = col_meta->DESC_NULLABLE;
      OW("Column%d:[SQL_DESC_NULLABLE]:[%d]%s", ColumnNumber, (int)*NumericAttributePtr, sql_nullable((SQLSMALLINT)*NumericAttributePtr));
      return SQL_SUCCESS;
    case SQL_DESC_NAME:
      n = snprintf(CharacterAttributePtr, BufferLength, "%s", col_meta->name);
      if (n < 0) {
        int e = errno;
        stmt_append_err_format(typesinfo->owner, "HY000", 0, "General error:internal logic error:[%d]%s", e, strerror(e));
        return SQL_ERROR;
      }
      if (StringLengthPtr) *StringLengthPtr = n;
      OW("Column%d:[SQL_DESC_NAME]:%.*s", ColumnNumber, n, (const char*)CharacterAttributePtr);
      return SQL_SUCCESS;
    case SQL_COLUMN_TYPE_NAME:
      n = snprintf(CharacterAttributePtr, BufferLength, "%s", col_meta->column_type_name);
      if (n < 0) {
        int e = errno;
        stmt_append_err_format(typesinfo->owner, "HY000", 0, "General error:internal logic error:[%d]%s", e, strerror(e));
        return SQL_ERROR;
      }
      if (StringLengthPtr) *StringLengthPtr = n;
      OW("Column%d:[SQL_COLUMN_TYPE_NAME]:%.*s", ColumnNumber, n, (const char*)CharacterAttributePtr);
      return SQL_SUCCESS;
    case SQL_DESC_LENGTH:
      *NumericAttributePtr = col_meta->DESC_LENGTH;
      OW("Column%d:[SQL_DESC_LENGTH]:%d", ColumnNumber, (int)*NumericAttributePtr);
      return SQL_SUCCESS;
    case SQL_DESC_NUM_PREC_RADIX:
      *NumericAttributePtr = col_meta->DESC_NUM_PREC_RADIX;
      OW("Column%d:[SQL_DESC_NUM_PREC_RADIX]:%d", ColumnNumber, (int)*NumericAttributePtr);
      return SQL_SUCCESS;
    case SQL_DESC_UNSIGNED:
      *NumericAttributePtr = col_meta->DESC_UNSIGNED;
      OW("Column%d:[SQL_DESC_UNSIGNED]:%s", ColumnNumber, ((int)*NumericAttributePtr) ? "SQL_TRUE" : "SQL_FALSE");
      return SQL_SUCCESS;
    default:
      stmt_append_err_format(typesinfo->owner, "HY000", 0, "General error:`%s[%d/0x%x]` not supported yet", sql_col_attribute(FieldIdentifier), FieldIdentifier, FieldIdentifier);
      return SQL_ERROR;
  }

  stmt_append_err(typesinfo->owner, "HY000", 0, "General error:not implemented yet");
  return SQL_ERROR;
}

static SQLRETURN _get_num_params(stmt_base_t *base, SQLSMALLINT *ParameterCountPtr)
{
  (void)ParameterCountPtr;

  typesinfo_t *typesinfo = (typesinfo_t*)base;
  (void)typesinfo;
  stmt_append_err(typesinfo->owner, "HY000", 0, "General error:not implemented yet");
  return SQL_ERROR;
}

static SQLRETURN _check_params(stmt_base_t *base)
{
  typesinfo_t *typesinfo = (typesinfo_t*)base;
  (void)typesinfo;
  stmt_append_err(typesinfo->owner, "HY000", 0, "General error:not implemented yet");
  return SQL_ERROR;
}

static SQLRETURN _tsdb_field_by_param(stmt_base_t *base, int i_param, TAOS_FIELD_E **field)
{
  (void)i_param;
  (void)field;

  typesinfo_t *typesinfo = (typesinfo_t*)base;
  (void)typesinfo;
  stmt_append_err(typesinfo->owner, "HY000", 0, "General error:not implemented yet");
  return SQL_ERROR;
}

static SQLRETURN _row_count(stmt_base_t *base, SQLLEN *row_count_ptr)
{
  typesinfo_t *typesinfo = (typesinfo_t*)base;
  (void)typesinfo;

  if (row_count_ptr) *row_count_ptr = 0;

  return SQL_SUCCESS;
}

static SQLRETURN _get_num_cols(stmt_base_t *base, SQLSMALLINT *ColumnCountPtr)
{
  (void)base;

  *ColumnCountPtr = typesinfo_get_count_of_col_meta();

  return SQL_SUCCESS;
}

static SQLRETURN _get_data(stmt_base_t *base, SQLUSMALLINT Col_or_Param_Num, tsdb_data_t *tsdb)
{
  (void)Col_or_Param_Num;
  (void)tsdb;
  typesinfo_t *typesinfo = (typesinfo_t*)base;
  (void)typesinfo;

  tsdb->is_null = 0;

  const type_info_rec_t *rec = _records + typesinfo->pos - 1;

  switch (Col_or_Param_Num) {
    case 1: // TYPE_NAME:
      tsdb->type = TSDB_DATA_TYPE_VARCHAR;
      tsdb->str.str = rec->TYPE_NAME;
      tsdb->str.len = strlen(rec->TYPE_NAME);
      break;
    case 2: // DATA_TYPE
      tsdb->type = TSDB_DATA_TYPE_BIGINT;
      tsdb->i64 = rec->DATA_TYPE;
      break;
    case 3: // COLUMN_SIZE
      tsdb->type = TSDB_DATA_TYPE_BIGINT;
      tsdb->i64 = rec->COLUMN_SIZE;
      break;
    case 4: // LITERAL_PREFIX
      tsdb->type = TSDB_DATA_TYPE_VARCHAR;
      if (rec->LITERAL_PREFIX == NULL) {
        tsdb->is_null = 1;
      } else {
        tsdb->str.str = rec->LITERAL_PREFIX;
        tsdb->str.len = strlen(rec->LITERAL_PREFIX);
      }
      break;
    case 5: // LITERAL_SUFFIX
      tsdb->type = TSDB_DATA_TYPE_VARCHAR;
      if (rec->LITERAL_SUFFIX == NULL) {
        tsdb->is_null = 1;
      } else {
        tsdb->str.str = rec->LITERAL_SUFFIX;
        tsdb->str.len = strlen(rec->LITERAL_SUFFIX);
      }
      break;
    case 6: // CREATE_PARAMS
      tsdb->type = TSDB_DATA_TYPE_VARCHAR;
      if (rec->CREATE_PARAMS == NULL) {
        tsdb->is_null = 1;
      } else {
        tsdb->str.str = rec->CREATE_PARAMS;
        tsdb->str.len = strlen(rec->CREATE_PARAMS);
      }
      break;
    case 7: // NULLABLE
      tsdb->type = TSDB_DATA_TYPE_BIGINT;
      tsdb->i64 = rec->NULLABLE;
      break;
    case 8: // CASE_SENSITIVE
      tsdb->type = TSDB_DATA_TYPE_BIGINT;
      tsdb->i64 = rec->CASE_SENSITIVE;
      break;
    case 9: // SEARCHABLE
      tsdb->type = TSDB_DATA_TYPE_BIGINT;
      tsdb->i64 = rec->SEARCHABLE;
      break;
    case 10: // UNSIGNED_ATTRIBUTE
      tsdb->type = TSDB_DATA_TYPE_BIGINT;
      if (rec->UNSIGNED_ATTRIBUTE == NULL_FIELD) {
        tsdb->is_null = 1;
      } else {
        tsdb->i64 = rec->UNSIGNED_ATTRIBUTE;
      }
      break;
    case 11: // FIXED_PREC_SCALE
      tsdb->type = TSDB_DATA_TYPE_BIGINT;
      if (rec->FIXED_PREC_SCALE == NULL_FIELD) {
        tsdb->is_null = 1;
      } else {
        tsdb->i64 = rec->FIXED_PREC_SCALE;
      }
      break;
    case 12: // AUTO_UNIQUE_VALUE
      tsdb->type = TSDB_DATA_TYPE_BIGINT;
      tsdb->i64 = rec->AUTO_UNIQUE_VALUE;
      break;
    case 13: // LOCAL_TYPE_NAME
      tsdb->type = TSDB_DATA_TYPE_VARCHAR;
      tsdb->str.str = rec->LOCAL_TYPE_NAME;
      tsdb->str.len = strlen(rec->LOCAL_TYPE_NAME);
      break;
    case 14: // MINIMUM_SCALE
      tsdb->type = TSDB_DATA_TYPE_BIGINT;
      if (rec->MINIMUM_SCALE == NULL_FIELD) {
        tsdb->is_null = 1;
      } else {
        tsdb->i64 = rec->MINIMUM_SCALE;
      }
      break;
    case 15: // MAXIMUM_SCALE
      tsdb->type = TSDB_DATA_TYPE_BIGINT;
      if (rec->MAXIMUM_SCALE == NULL_FIELD) {
        tsdb->is_null = 1;
      } else {
        tsdb->i64 = rec->MAXIMUM_SCALE;
      }
      break;
    case 16: // SQL_DATA_TYPE
      tsdb->type = TSDB_DATA_TYPE_BIGINT;
      tsdb->i64 = rec->SQL_DATA_TYPE;
      break;
    case 17: // SQL_DATETIME_SUB
      tsdb->type = TSDB_DATA_TYPE_BIGINT;
      if (rec->SQL_DATETIME_SUB == NULL_FIELD) {
        tsdb->is_null = 1;
      } else {
        tsdb->i64 = rec->SQL_DATETIME_SUB;
      }
      break;
    case 18: // NUM_PREC_RADIX
      tsdb->type = TSDB_DATA_TYPE_BIGINT;
      if (rec->NUM_PREC_RADIX == NULL_FIELD) {
        tsdb->is_null = 1;
      } else {
        tsdb->i64 = rec->NUM_PREC_RADIX;
      }
      break;
    case 19: // INTERVAL_PRECISION
      tsdb->type = TSDB_DATA_TYPE_BIGINT;
      if (rec->INTERVAL_PRECISION == NULL_FIELD) {
        tsdb->is_null = 1;
      } else {
        tsdb->i64 = rec->INTERVAL_PRECISION;
      }
      break;
    default:
      stmt_append_err_format(typesinfo->owner, "HY000", 0, "General error:Column[%d] not implemented yet", Col_or_Param_Num);
      return SQL_ERROR;
  }

  return SQL_SUCCESS;
}

void typesinfo_init(typesinfo_t *typesinfo, stmt_t *stmt)
{
  typesinfo->owner = stmt;
  typesinfo->base.query                        = _query;
  typesinfo->base.execute                      = _execute;
  typesinfo->base.fetch_row                    = _fetch_row;
  typesinfo->base.describe_param               = _describe_param;
  typesinfo->base.describe_col                 = _describe_col;
  typesinfo->base.col_attribute                = _col_attribute;
  typesinfo->base.get_num_params               = _get_num_params;
  typesinfo->base.check_params                 = _check_params;
  typesinfo->base.tsdb_field_by_param          = _tsdb_field_by_param;
  typesinfo->base.row_count                    = _row_count;
  typesinfo->base.get_num_cols                 = _get_num_cols;
  typesinfo->base.get_data                     = _get_data;
}

SQLRETURN typesinfo_open(
    typesinfo_t     *typesinfo,
    SQLSMALLINT      DataType)
{
  SQLRETURN sr = SQL_SUCCESS;

  typesinfo_reset(typesinfo);

  typesinfo->data_type = DataType;

  return sr;
}