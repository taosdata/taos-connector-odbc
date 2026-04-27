/*
 * MIT License
 *
 * Copyright (c) 2022-2023 freemine <freemine@yeah.net>
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

#define _GNU_SOURCE

#include "os_port.h"
#include "taos_helpers.h"

#define _HELPER_RAW_BLOCK_NBIT                (3u)
#define _helper_raw_bit_pos(_n)               ((_n) & ((1 << _HELPER_RAW_BLOCK_NBIT) - 1))
#define _helper_raw_bitmap_char_pos(bm_, r_)  ((bm_)[(r_) >> _HELPER_RAW_BLOCK_NBIT])
#define _helper_raw_col_data_is_null(bm_, r_) ((_helper_raw_bitmap_char_pos(bm_, r_) & (1u << (7u - _helper_raw_bit_pos(r_)))) == (1u << (7u - _helper_raw_bit_pos(r_))))
#define _helper_raw_bitmap_len(_n)            (((_n) + ((1 << _HELPER_RAW_BLOCK_NBIT) - 1)) >> _HELPER_RAW_BLOCK_NBIT)

typedef struct _helper_raw_block_layout_s {
  int32_t magic;
  int32_t blockSize;
  int32_t rows;
  int32_t cols;
  int32_t unknowns[3];
} _helper_raw_block_layout_t;

#pragma pack(push, 1)
typedef struct _helper_raw_field_layout_s {
  int8_t  type;
  int32_t bytes;
} _helper_raw_field_layout_t;

typedef struct _helper_raw_col_bytes_s {
  int32_t bytes;
} _helper_raw_col_bytes_t;
#pragma pack(pop)

typedef struct _helper_raw_block_s {
  const char                     *base;
  int                             nr_rows;
  const _helper_raw_block_layout_t *block;
  const _helper_raw_field_layout_t *fields;
  const _helper_raw_col_bytes_t    *col_bytes;
  const char                     *data;
} _helper_raw_block_t;

static int _helper_is_valid_utf32le_text(const char *s, int len)
{
  if (!s || len <= 0) return 0;
  if ((len % 4) != 0) return 0;

  for (int i = 0; i < len; i += 4) {
    const unsigned char *p = (const unsigned char *)s + i;
    uint32_t cp = ((uint32_t)p[0]) |
                  ((uint32_t)p[1] << 8) |
                  ((uint32_t)p[2] << 16) |
                  ((uint32_t)p[3] << 24);
    if (cp == 0) return 0;
    if (cp > 0x10FFFF) return 0;
    if (cp >= 0xD800 && cp <= 0xDFFF) return 0;
  }

  return 1;
}

static const char *_helper_detect_nchar_encoder(const char *s, int len)
{
  return _helper_is_valid_utf32le_text(s, len) ? "UTF-32LE" : "UTF-8";
}

static int _helper_raw_field_is_fixed(int8_t field_type, size_t *nr_fix)
{
  switch (field_type) {
    case TSDB_DATA_TYPE_BOOL:
    case TSDB_DATA_TYPE_TINYINT:
    case TSDB_DATA_TYPE_UTINYINT:
      *nr_fix = sizeof(int8_t);
      return 1;
    case TSDB_DATA_TYPE_SMALLINT:
    case TSDB_DATA_TYPE_USMALLINT:
      *nr_fix = sizeof(int16_t);
      return 1;
    case TSDB_DATA_TYPE_INT:
    case TSDB_DATA_TYPE_UINT:
      *nr_fix = sizeof(int32_t);
      return 1;
    case TSDB_DATA_TYPE_BIGINT:
    case TSDB_DATA_TYPE_UBIGINT:
    case TSDB_DATA_TYPE_TIMESTAMP:
      *nr_fix = sizeof(int64_t);
      return 1;
    case TSDB_DATA_TYPE_FLOAT:
      *nr_fix = sizeof(float);
      return 1;
    case TSDB_DATA_TYPE_DOUBLE:
      *nr_fix = sizeof(double);
      return 1;
    case TSDB_DATA_TYPE_VARCHAR:
    case TSDB_DATA_TYPE_NCHAR:
    case TSDB_DATA_TYPE_JSON:
    case TSDB_DATA_TYPE_VARBINARY:
    case TSDB_DATA_TYPE_GEOMETRY:
      *nr_fix = 0;
      return 0;
    default:
      *nr_fix = 0;
      return -1;
  }
}

static int _helper_raw_block_init(_helper_raw_block_t *raw_block, int nr_rows, const void *block, char *buf, size_t len)
{
  if (!raw_block || !block) {
    snprintf(buf, len, "invalid raw block");
    return -1;
  }

  const _helper_raw_block_layout_t *header = (const _helper_raw_block_layout_t*)block;

  // Validate block version (magic field)
  if (header->magic != 1 && header->magic != 2) {
    snprintf(buf, len, "raw block unsupported version: %d", header->magic);
    return -1;
  }

  if (header->cols <= 0) {
    snprintf(buf, len, "raw block invalid cols: %d", header->cols);
    return -1;
  }

  if (header->rows < 0) {
    snprintf(buf, len, "raw block invalid rows: %d", header->rows);
    return -1;
  }

  raw_block->base      = (const char*)block;
  raw_block->nr_rows   = nr_rows;
  raw_block->block     = header;
  raw_block->fields    = (const _helper_raw_field_layout_t*)&raw_block->block[1];
  raw_block->col_bytes = (const _helper_raw_col_bytes_t*)&raw_block->fields[raw_block->block->cols];
  raw_block->data      = (const char*)&raw_block->col_bytes[raw_block->block->cols];

  if (raw_block->block->rows != raw_block->nr_rows) {
    snprintf(buf, len, "raw block rows mismatch: %d <> %d", raw_block->block->rows, raw_block->nr_rows);
    return -1;
  }

  return 0;
}

static int _helper_raw_block_get_col_ptr(const _helper_raw_block_t *raw_block, int i_col, const char **col_ptr, size_t *nr_head, size_t *nr_fix, int *is_fix, char *buf, size_t len)
{
  if (!raw_block || !col_ptr || !nr_head || !nr_fix || !is_fix) {
    snprintf(buf, len, "invalid raw block access");
    return -1;
  }

  if (i_col < 0 || i_col >= raw_block->block->cols) {
    snprintf(buf, len, "raw block column[%d] out of range", i_col + 1);
    return -1;
  }

  const char *p = raw_block->data;
  for (int i = 0; i < i_col; ++i) {
    size_t width = 0;
    int fixed = _helper_raw_field_is_fixed(raw_block->fields[i].type, &width);
    if (fixed < 0) {
      snprintf(buf, len, "raw block column[%d] type `%s[0x%x/%d]` not implemented yet",
          i + 1,
          taos_data_type(raw_block->fields[i].type),
          raw_block->fields[i].type,
          raw_block->fields[i].type);
      return -1;
    }

    size_t head = fixed ? _helper_raw_bitmap_len(raw_block->nr_rows) : sizeof(uint32_t) * (size_t)raw_block->nr_rows;
    p += head;
    p += raw_block->col_bytes[i].bytes;
  }

  *is_fix = _helper_raw_field_is_fixed(raw_block->fields[i_col].type, nr_fix);
  if (*is_fix < 0) {
    snprintf(buf, len, "raw block column[%d] type `%s[0x%x/%d]` not implemented yet",
        i_col + 1,
        taos_data_type(raw_block->fields[i_col].type),
        raw_block->fields[i_col].type,
        raw_block->fields[i_col].type);
    return -1;
  }

  *nr_head = *is_fix ? _helper_raw_bitmap_len(raw_block->nr_rows) : sizeof(uint32_t) * (size_t)raw_block->nr_rows;
  *col_ptr = p;

  return 0;
}

// decode a single column value from pre-extracted raw block column data
static int _helper_get_tsdb_col(int time_precision, const char *name, uint8_t col_type, const void *col_data, uint32_t col_len,
    int i_row, int i_col, tsdb_data_t *tsdb, char *buf, size_t len)
{
  // FIXME: what to tell the difference between is_null(res[i_row,i_col])? and i_row/i_col out of bound?
  if (!col_data) {
    tsdb->type       = col_type;
    tsdb->is_null    = 1;
    return 0;
  }

  tsdb->is_null = 0;

  switch (col_type) {
    case TSDB_DATA_TYPE_BOOL:
      {
        uint8_t *col = (uint8_t*)col_data;
        tsdb->b = !!*col;
      } break;
    case TSDB_DATA_TYPE_TINYINT:
      {
        int8_t *col = (int8_t*)col_data;
        tsdb->i8 = *col;
      } break;
    case TSDB_DATA_TYPE_UTINYINT:
      {
        uint8_t *col= (uint8_t*)col_data;
        tsdb->u8 = *col;
      } break;
    case TSDB_DATA_TYPE_SMALLINT:
      {
        int16_t *col = (int16_t*)col_data;
        tsdb->i16 = *col;
      } break;
    case TSDB_DATA_TYPE_USMALLINT:
      {
        uint16_t *col = (uint16_t*)col_data;
        tsdb->u16 = *col;
      } break;
    case TSDB_DATA_TYPE_INT:
      {
        int32_t *col = (int32_t*)col_data;
        tsdb->i32 = *col;
      } break;
    case TSDB_DATA_TYPE_UINT:
      {
        uint32_t *col = (uint32_t*)col_data;
        tsdb->u32 = *col;
      } break;
    case TSDB_DATA_TYPE_BIGINT:
      {
        int64_t *col = (int64_t*)col_data;
        tsdb->i64 = *col;
      } break;
    case TSDB_DATA_TYPE_UBIGINT:
      {
        uint64_t *col = (uint64_t*)col_data;
        tsdb->u64 = *col;
      } break;
    case TSDB_DATA_TYPE_FLOAT:
      {
        float *col = (float*)col_data;
        tsdb->flt = *col;
      } break;
    case TSDB_DATA_TYPE_DOUBLE:
      {
        double *col = (double*)col_data;
        tsdb->dbl = *col;
      } break;
    case TSDB_DATA_TYPE_NCHAR:
      {
        char *col = (char*)col_data;
        tsdb->str.str = col;
        tsdb->str.len = col_len;
        tsdb->str.encoder = "UTF-32LE"; // raw block NCHAR is always UTF-32LE
      } break;
    case TSDB_DATA_TYPE_JSON:
      {
        char *col = (char*)col_data;
        tsdb->str.str = col;
        tsdb->str.len = col_len;
        tsdb->str.encoder = NULL;
      } break;
    case TSDB_DATA_TYPE_VARCHAR:
      {
        char *col = (char*)col_data;
        // // FIXME:
        // int16_t length = *(int16_t*)col;
        // col += sizeof(int16_t);
        tsdb->str.str = col;
        tsdb->str.len = strnlen(col, col_len); // FIXME:
        tsdb->str.encoder = NULL;
      } break;
    case TSDB_DATA_TYPE_VARBINARY:
      {
        tsdb->bin.bin = (const unsigned char*)col_data;
        tsdb->bin.len = col_len;
      } break;
    case TSDB_DATA_TYPE_GEOMETRY:
      {
        tsdb->geo.geo = (const unsigned char*)col_data;
        tsdb->geo.len = col_len;
      } break;
    case TSDB_DATA_TYPE_TIMESTAMP:
      {
        int64_t *col = (int64_t*)col_data;
        tsdb->ts.ts = *col;
        tsdb->ts.precision = time_precision;
      } break;
    default:
      snprintf(buf, len, "Column[(%d,%d)/%s] conversion from `%s[0x%x/%d]` not implemented yet",
          i_row + 1, i_col + 1, name, taos_data_type(col_type), col_type, col_type);
      return -1;
  }

  tsdb->type       = col_type;

  return 0;
}

int helper_get_tsdb(TAOS_RES *res, int block, TAOS_FIELD *fields, int time_precision, TAOS_ROW rows, int i_row, int i_col, tsdb_data_t *tsdb, char *buf, size_t len)
{
  TAOS_FIELD *field = fields + i_col;

  if (CALL_taos_is_null(res, i_row, i_col)) {
    tsdb->is_null = 1;
    return 0;
  }

  tsdb->is_null = 0;

  switch(field->type) {
    case TSDB_DATA_TYPE_BOOL:
      {
        uint8_t *col = (uint8_t*)rows[i_col];
        col += i_row;
        tsdb->b = !!*col;
      } break;
    case TSDB_DATA_TYPE_TINYINT:
      {
        int8_t *col = (int8_t*)rows[i_col];
        col += i_row;
        tsdb->i8 = *col;
      } break;
    case TSDB_DATA_TYPE_UTINYINT:
      {
        uint8_t *col= (uint8_t*)rows[i_col];
        col += i_row;
        tsdb->u8 = *col;
      } break;
    case TSDB_DATA_TYPE_SMALLINT:
      {
        int16_t *col = (int16_t*)rows[i_col];
        col += i_row;
        tsdb->i16 = *col;
      } break;
    case TSDB_DATA_TYPE_USMALLINT:
      {
        uint16_t *col = (uint16_t*)rows[i_col];
        col += i_row;
        tsdb->u16 = *col;
      } break;
    case TSDB_DATA_TYPE_INT:
      {
        int32_t *col = (int32_t*)rows[i_col];
        col += i_row;
        tsdb->i32 = *col;
      } break;
    case TSDB_DATA_TYPE_UINT:
      {
        uint32_t *col = (uint32_t*)rows[i_col];
        col += i_row;
        tsdb->u32 = *col;
      } break;
    case TSDB_DATA_TYPE_BIGINT:
      {
        int64_t *col = (int64_t*)rows[i_col];
        col += i_row;
        tsdb->i64 = *col;
      } break;
    case TSDB_DATA_TYPE_UBIGINT:
      {
        uint64_t *col = (uint64_t*)rows[i_col];
        col += i_row;
        tsdb->u64 = *col;
      } break;
    case TSDB_DATA_TYPE_FLOAT:
      {
        float *col = (float*)rows[i_col];
        col += i_row;
        tsdb->flt = *col;
      } break;
    case TSDB_DATA_TYPE_DOUBLE:
      {
        double *col = (double*)rows[i_col];
        col += i_row;
        tsdb->dbl = *col;
      } break;
    case TSDB_DATA_TYPE_VARCHAR:
    case TSDB_DATA_TYPE_NCHAR:
    case TSDB_DATA_TYPE_JSON:
    case TSDB_DATA_TYPE_VARBINARY:
    case TSDB_DATA_TYPE_GEOMETRY:
      if (block) {
        int *offsets = CALL_taos_get_column_data_offset(res, i_col);
        char *col = (char*)(rows[i_col]);
        col += offsets[i_row];
        int16_t length = *(int16_t*)col;
        col += sizeof(int16_t);

        if (field->type == TSDB_DATA_TYPE_VARBINARY) {
          tsdb->bin.bin = (const unsigned char*)col;
          tsdb->bin.len = length;
        } else if (field->type == TSDB_DATA_TYPE_GEOMETRY) {
          tsdb->geo.geo = (const unsigned char*)col;
          tsdb->geo.len = length;
        } else {
          tsdb->str.str = col;
          tsdb->str.len = length;
          tsdb->str.encoder = (field->type == TSDB_DATA_TYPE_NCHAR)
              ? _helper_detect_nchar_encoder(col, length)
              : NULL;
        }
      } else {
        char *col = (char*)(rows[i_col]);
        int16_t length = ((int16_t*)col)[-1];
        if (field->type != TSDB_DATA_TYPE_VARBINARY) {
          tsdb->str.str = col;
          tsdb->str.len = length;
          tsdb->str.encoder = (field->type == TSDB_DATA_TYPE_NCHAR)
              ? _helper_detect_nchar_encoder(col, length)
              : NULL;
        } else {
          snprintf(buf, len, "Column[(%d,%d)/%s] conversion from `%s[0x%x/%d]` not implemented yet for non-block-fetching mode",
              i_row + 1, i_col + 1, field->name, taos_data_type(field->type), field->type, field->type);
          return -1;
        }
      } break;
    case TSDB_DATA_TYPE_TIMESTAMP:
      {
        int64_t *col = (int64_t*)rows[i_col];
        col += i_row;
        tsdb->ts.ts = *col;
        tsdb->ts.precision = time_precision;
      } break;
    default:
      snprintf(buf, len, "Column[(%d,%d)/%s] conversion from `%s[0x%x/%d]` not implemented yet",
          i_row + 1, i_col + 1, field->name, taos_data_type(field->type), field->type, field->type);
      return -1;
  }

  tsdb->type   = field->type;

  return 0;
}

int helper_get_tsdb_from_raw_block(const void *block, int nr_rows, TAOS_FIELD *fields, int time_precision, int i_row, int i_col, tsdb_data_t *tsdb, char *buf, size_t len)
{
  _helper_raw_block_t raw_block = {0};
  const char *col_ptr = NULL;
  size_t nr_head = 0;
  size_t nr_fix = 0;
  int is_fix = 0;

  if (!fields) {
    snprintf(buf, len, "invalid fields");
    return -1;
  }

  if (i_row < 0 || i_row >= nr_rows) {
    snprintf(buf, len, "raw block row[%d] out of range", i_row + 1);
    return -1;
  }

  if (_helper_raw_block_init(&raw_block, nr_rows, block, buf, len)) {
    return -1;
  }

  if (_helper_raw_block_get_col_ptr(&raw_block, i_col, &col_ptr, &nr_head, &nr_fix, &is_fix, buf, len)) {
    return -1;
  }

  const char *name = fields[i_col].name;
  uint8_t field_type = raw_block.fields[i_col].type;

  if (is_fix) {
    const unsigned char *bitmap = (const unsigned char*)col_ptr;
    if (_helper_raw_col_data_is_null(bitmap, i_row)) {
      return _helper_get_tsdb_col(time_precision, name, field_type, NULL, 0, i_row, i_col, tsdb, buf, len);
    }

    const char *col_data = col_ptr + nr_head + nr_fix * (size_t)i_row;
    return _helper_get_tsdb_col(time_precision, name, field_type, col_data, (uint32_t)nr_fix, i_row, i_col, tsdb, buf, len);
  }

  const uint32_t *offsets = (const uint32_t*)col_ptr;
  if (offsets[i_row] == UINT32_MAX) {
    return _helper_get_tsdb_col(time_precision, name, field_type, NULL, 0, i_row, i_col, tsdb, buf, len);
  }

  const char *var_data = col_ptr + nr_head + offsets[i_row];
  uint16_t col_len = *(const uint16_t*)var_data;
  const char *col_data = var_data + sizeof(uint16_t);
  int r = _helper_get_tsdb_col(time_precision, name, field_type, col_data, col_len, i_row, i_col, tsdb, buf, len);
  return r;
}


#ifdef FAKE_TAOS
void taos_cleanup(void)
{
}

setConfRet taos_set_config(const char *config)
{
  (void)config;
  setConfRet r = {0, "unknown setConfRet"};
  return r;
}

int taos_init(void)
{
  return 0;
}

TAOS* taos_connect(const char *ip, const char *user, const char *pass, const char *db, uint16_t port)
{
  (void)ip;
  (void)user;
  (void)pass;
  (void)db;
  (void)port;
  return (TAOS*)1;
}

TAOS* taos_connect_auth(const char *ip, const char *user, const char *auth, const char *db, uint16_t port)
{
  (void)ip;
  (void)user;
  (void)auth;
  (void)db;
  (void)port;
  return (TAOS*)1;
}

void taos_close(TAOS *taos)
{
  (void)taos;
}

const char* taos_data_type(int type)
{
  (void)type;
  return "unknown taos_data_type";
}

TAOS_STMT* taos_stmt_init(TAOS *taos)
{
  (void)taos;
  return (TAOS_STMT*)1;
}

int taos_stmt_prepare(TAOS_STMT *stmt, const char *sql, unsigned long length)
{
  (void)stmt;
  (void)sql;
  (void)length;
  return 0;
}

int taos_stmt_set_tbname_tags(TAOS_STMT *stmt, const char *name, TAOS_MULTI_BIND *tags)
{
  (void)stmt;
  (void)name;
  (void)tags;
  return 0;
}

int taos_stmt_set_tbname(TAOS_STMT *stmt, const char *name)
{
  (void)stmt;
  (void)name;
  return 0;
}

int taos_stmt_set_tags(TAOS_STMT *stmt, TAOS_MULTI_BIND *tags)
{
  (void)stmt;
  (void)tags;
  return 0;
}

int taos_stmt_set_sub_tbname(TAOS_STMT *stmt, const char *name)
{
  (void)stmt;
  (void)name;
  return 0;
}

int taos_stmt_get_tag_fields(TAOS_STMT *stmt, int *fieldNum, TAOS_FIELD_E **fields)
{
  (void)stmt;
  (void)fieldNum;
  (void)fields;
  return 0;
}

int taos_stmt_get_col_fields(TAOS_STMT *stmt, int *fieldNum, TAOS_FIELD_E **fields)
{
  (void)stmt;
  if (fieldNum) *fieldNum = 0;
  if (fields) *fields = NULL;
  return 0;
}

void taos_stmt_reclaim_fields(TAOS_STMT *stmt, TAOS_FIELD_E *fields)
{
  (void)stmt;
  (void)fields;
}

int taos_stmt_is_insert(TAOS_STMT *stmt, int *insert)
{
  (void)stmt;
  if (insert) *insert = 0;
  return 0;
}

int taos_stmt_num_params(TAOS_STMT *stmt, int *nums)
{
  (void)stmt;
  (void)nums;
  return 0;
}

int taos_stmt_get_param(TAOS_STMT *stmt, int idx, int *type, int *bytes)
{
  (void)stmt;
  (void)idx;
  if (type) *type = 0;
  if (bytes) *bytes = 0;
  return 0;
}

int taos_stmt_bind_param(TAOS_STMT *stmt, TAOS_MULTI_BIND *bind)
{
  (void)stmt;
  (void)bind;
  return 0;
}

int taos_stmt_bind_param_batch(TAOS_STMT *stmt, TAOS_MULTI_BIND *bind)
{
  (void)stmt;
  (void)bind;
  return 0;
}

int taos_stmt_bind_single_param_batch(TAOS_STMT *stmt, TAOS_MULTI_BIND *bind, int colIdx)
{
  (void)stmt;
  (void)bind;
  (void)colIdx;
  return 0;
}

int taos_stmt_add_batch(TAOS_STMT *stmt)
{
  (void)stmt;
  return 0;
}

int taos_stmt_execute(TAOS_STMT *stmt)
{
  (void)stmt;
  return 0;
}

TAOS_RES* taos_stmt_use_result(TAOS_STMT *stmt)
{
  (void)stmt;
  return NULL;
}

int taos_stmt_close(TAOS_STMT *stmt)
{
  (void)stmt;
  return 0;
}

char* taos_stmt_errstr(TAOS_STMT *stmt)
{
  (void)stmt;
  return "unknown taos_stmt_errstr";
}

int taos_stmt_affected_rows(TAOS_STMT *stmt)
{
  (void)stmt;
  return 0;
}

int taos_stmt_affected_rows_once(TAOS_STMT *stmt)
{
  (void)stmt;
  return 0;
}

TAOS_RES* taos_query(TAOS *taos, const char *sql)
{
  (void)taos;
  (void)sql;
  return (TAOS_RES*)1;
}

TAOS_ROW taos_fetch_row(TAOS_RES *res)
{
  (void)res;
  return (TAOS_ROW)1;
}

int taos_result_precision(TAOS_RES *res)  // get the time precision of result
{
  (void)res;
  return 0;
}

void taos_free_result(TAOS_RES *res)
{
  (void)res;
}

void taos_kill_query(TAOS *taos)
{
  (void)taos;
}

int taos_field_count(TAOS_RES *res)
{
  (void)res;
  return 0;
}

int taos_affected_rows(TAOS_RES *res)
{
  (void)res;
  return 0;
}

TAOS_FIELD* taos_fetch_fields(TAOS_RES *res)
{
  (void)res;
  return NULL;
}

int taos_select_db(TAOS *taos, const char *db)
{
  (void)taos;
  (void)db;
  return 0;
}

int taos_print_row(char *str, TAOS_ROW row, TAOS_FIELD *fields, int num_fields)
{
  (void)row;
  (void)fields;
  (void)num_fields;
  if (str) *str = '\0';
  return 0;
}

void taos_stop_query(TAOS_RES *res)
{
  (void)res;
}

bool taos_is_null(TAOS_RES *res, int32_t row, int32_t col)
{
  (void)res;
  (void)row;
  (void)col;
  return 0;
}

bool taos_is_update_query(TAOS_RES *res)
{
  (void)res;
  return 1;
}

int taos_fetch_block(TAOS_RES *res, TAOS_ROW *rows)
{
  (void)res;
  if (rows) *rows = NULL;
  return 0;
}

int taos_fetch_block_s(TAOS_RES *res, int *numOfRows, TAOS_ROW *rows)
{
  (void)res;
  if (numOfRows) *numOfRows = 0;
  if (rows) *rows = NULL;
  return 0;
}

int taos_fetch_raw_block(TAOS_RES *res, int *numOfRows, void **pData)
{
  (void)res;
  if (numOfRows) *numOfRows = 0;
  if (pData) *pData = NULL;
  return -1;
}

int* taos_get_column_data_offset(TAOS_RES *res, int columnIndex)
{
  (void)res;
  (void)columnIndex;
  return 0;
}

int taos_validate_sql(TAOS *taos, const char *sql)
{
  (void)taos;
  (void)sql;
  return 0;
}

void taos_reset_current_db(TAOS *taos)
{
  (void)taos;
}

int* taos_fetch_lengths(TAOS_RES *res)
{
  (void)res;
  return 0;
}

TAOS_ROW* taos_result_block(TAOS_RES *res)
{
  (void)res;
  return NULL;
}

const char* taos_get_server_info(TAOS *taos)
{
  (void)taos;
  return "unknown taos_get_server_info";
}

const char* taos_get_client_info(void)
{
  return "unknown taos_get_client_info";
}

const char* taos_errstr(TAOS_RES *res)
{
  (void)res;
  return "unknown taos_errstr";
}

int taos_errno(TAOS_RES *res)
{
  (void)res;
  return 0;
}

void taos_query_a(TAOS *taos, const char *sql, __taos_async_fn_t fp, void *param)
{
  (void)taos;
  (void)sql;
  (void)fp;
  (void)param;
}

void taos_fetch_rows_a(TAOS_RES *res, __taos_async_fn_t fp, void *param)
{
  (void)res;
  (void)fp;
  (void)param;
}

void taos_fetch_raw_block_a(TAOS_RES *res, __taos_async_fn_t fp, void *param)
{
  (void)res;
  (void)fp;
  (void)param;
}

const void* taos_get_raw_block(TAOS_RES *res)
{
  (void)res;
  return NULL;
}

int taos_load_table_info(TAOS *taos, const char *tableNameList)
{
  (void)taos;
  (void)tableNameList;
  return -1;
}

TAOS_RES* taos_schemaless_insert(TAOS *taos, char *lines[], int numLines, int protocol, int precision)
{
  (void)taos;
  (void)lines;
  (void)numLines;
  (void)protocol;
  (void)precision;
  return NULL;
}

tmq_list_t* tmq_list_new(void)
{
  return NULL;
}

int32_t     tmq_list_append(tmq_list_t *topic_list, const char *name)
{
  (void)topic_list;
  (void)name;
  return -1;
}

void        tmq_list_destroy(tmq_list_t *topic_list)
{
  (void)topic_list;
}

int32_t     tmq_list_get_size(const tmq_list_t *topic_list)
{
  (void)topic_list;
  return -1;
}

char**      tmq_list_to_c_array(const tmq_list_t *topic_list)
{
  (void)topic_list;
  return NULL;
}

tmq_t*      tmq_consumer_new(tmq_conf_t *conf, char *errstr, int32_t errstrLen)
{
  (void)conf;
  (void)errstr;
  (void)errstrLen;
  return NULL;
}

const char* tmq_err2str(int32_t code)
{
  (void)code;
  return "unknown tmq_err2str";
}

int32_t   tmq_subscribe(tmq_t *tmq, const tmq_list_t *topic_list)
{
  (void)tmq;
  (void)topic_list;
  return -1;
}

int32_t   tmq_unsubscribe(tmq_t *tmq)
{
  (void)tmq;
  return -1;
}

int32_t   tmq_subscription(tmq_t *tmq, tmq_list_t **topics)
{
  (void)tmq;
  (void)topics;
  return -1;
}

TAOS_RES* tmq_consumer_poll(tmq_t *tmq, int64_t timeout)
{
  (void)tmq;
  (void)timeout;
  return NULL;
}

int32_t   tmq_consumer_close(tmq_t *tmq)
{
  (void)tmq;
  return 0;
}

int32_t   tmq_commit_sync(tmq_t *tmq, const TAOS_RES *msg)
{
  (void)tmq;
  (void)msg;
  return -1;
}

void      tmq_commit_async(tmq_t *tmq, const TAOS_RES *msg, tmq_commit_cb *cb, void *param)
{
  (void)tmq;
  (void)msg;
  (void)cb;
  (void)param;
}

tmq_conf_t*    tmq_conf_new(void)
{
  return NULL;
}

tmq_conf_res_t tmq_conf_set(tmq_conf_t *conf, const char *key, const char *value)
{
  (void)conf;
  (void)key;
  (void)value;
  return 0;
}

void           tmq_conf_destroy(tmq_conf_t *conf)
{
  (void)conf;
}

void           tmq_conf_set_auto_commit_cb(tmq_conf_t *conf, tmq_commit_cb *cb, void *param)
{
  (void)conf;
  (void)cb;
  (void)param;
}

const char* tmq_get_topic_name(TAOS_RES *res)
{
  (void)res;
  return "unknown tmq_get_topic_name";
}

const char* tmq_get_db_name(TAOS_RES *res)
{
  (void)res;
  return "unknown tmq_get_db_name";
}

int32_t     tmq_get_vgroup_id(TAOS_RES *res)
{
  (void)res;
  return 0;
}

const char* tmq_get_table_name(TAOS_RES *res)
{
  (void)res;
  return "unknown tmq_get_table_name";
}

tmq_res_t   tmq_get_res_type(TAOS_RES *res)
{
  (void)res;
  return 0;
}

TSDB_SERVER_STATUS taos_check_server_status(const char *fqdn, int port, char *details, int maxlen)
{
  (void)fqdn;
  (void)port;
  (void)details;
  (void)maxlen;
  return 0;
}

int taos_get_current_db(TAOS *taos, char *database, int len, int *required)
{
  (void)taos;
  (void)database;
  (void)len;
  (void)required;
  return 0;
}
#endif
