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
#include "errs.h"

void errs_init(errs_t *errs)
{
  INIT_TOD_LIST_HEAD(&errs->errs);
  INIT_TOD_LIST_HEAD(&errs->frees);
}

static void err_set_x(err_t *err, const char *file, int line, const char *func, const char *data_source, const char *sql_state, int e, const char *estr)
{
  const char *vendor = "freemine@yeah.net";
  const char *odbc_component = "TAOS ODBC Driver";
  err->err = e;
  char buf[PATH_MAX + 1];
  char *p = tod_basename(file, buf, sizeof(buf));
  snprintf(err->buf, sizeof(err->buf),
      "[%s][%s][%s]%s[%d]:%s(): %s",
      vendor, odbc_component, data_source,
      p, line, func,
      estr);
  err->estr = err->buf;
  strncpy((char*)err->sql_state, sql_state, sizeof(err->sql_state));
}

void errs_append_x(errs_t *errs, const char *file, int line, const char *func, const char *data_source, const char *sql_state, int e, const char *estr)
{
  if (tod_list_empty(&errs->frees)) {
    err_t *err = (err_t*)calloc(1, sizeof(*err));
    if (!err) {
      OD("out of memory");
      return;
    }

    err->err           = 0;
    err->estr          = "";
    err->sql_state[0]  = '\0';
    err->buf[0]        = '\0';

    tod_list_add_tail(&err->node, &errs->frees);
  }

  err_t *err = NULL;
  tod_list_first_entry_or_null(err, &errs->frees, err_t, node);
  tod_list_del(&err->node);
  err_set_x(err, file, line, func, data_source, sql_state, e, estr);
  tod_list_add_tail(&err->node, &errs->errs);
}

void errs_clr_x(errs_t *errs)
{
  if (tod_list_empty(&errs->errs)) return;

  err_t *p, *n;
  tod_list_for_each_entry_safe(p, n, &errs->errs, err_t, node) {
    tod_list_del(&p->node);
    tod_list_add_tail(&p->node, &errs->frees);
  }
}

void errs_release_x(errs_t *errs)
{
  err_t *p, *n;
  tod_list_for_each_entry_safe(p, n, &errs->errs, err_t, node) {
    tod_list_del(&p->node);
    free(p);
  }

  tod_list_for_each_entry_safe(p, n, &errs->frees, err_t, node) {
    tod_list_del(&p->node);
    free(p);
  }
}

SQLRETURN errs_get_diag_rec_x(
    errs_t         *errs,
    SQLSMALLINT     RecNumber,
    SQLCHAR        *SQLState,
    SQLINTEGER     *NativeErrorPtr,
    SQLCHAR        *MessageText,
    SQLSMALLINT     BufferLength,
    SQLSMALLINT    *TextLengthPtr)
{
  if (RecNumber == 0) return SQL_NO_DATA;
  if (tod_list_empty(&errs->errs)) return SQL_NO_DATA;

  int i = 1;

  int found = 0;
  err_t *p = NULL;
  tod_list_for_each_entry(p, &errs->errs, err_t, node) {
    if (i == RecNumber) {
      found = 1;
      break;
    }
    ++i;
  }

  if (!found) return SQL_NO_DATA;

  if (NativeErrorPtr) *NativeErrorPtr = p->err;
  if (SQLState) strncpy((char*)SQLState, (const char*)p->sql_state, 6);
  int n = snprintf((char*)MessageText, BufferLength, "%s", p->estr);
  if (TextLengthPtr) *TextLengthPtr = n;

  return SQL_SUCCESS;
}

