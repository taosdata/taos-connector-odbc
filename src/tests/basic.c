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

#include "../core/internal.h" // FIXME:

#include "conn.h"
#include "env.h"
#include "errs.h"
#include "helpers.h"
#include "insert_eval.h"
#include "logger.h"
#include "conn_parser.h"
#include "ext_parser.h"
#include "sqls_parser.h"
#include "ejson_parser.h"
#include "url_parser.h"
#include "tls.h"
#include "utils.h"

#include <errno.h>
#include <string.h>

#define DUMP(fmt, ...)          printf(fmt "\n", ##__VA_ARGS__)

static int cmp_strs(const char *s1, const char *s2)
{
  int r = 0;
  if ((!!s1) ^ (!!s2)) {
    return s1 ? 1 : -1;
  }
  if (s1 && (r=strcmp(s1, s2))) {
    return r;
  }

  return 0;
}

static int test_conn_parser(void)
{
  conn_parser_param_t param = {0};
  // param.ctx.debug_flex = 1;
  // param.ctx.debug_bison = 1;

  const struct {
    int                        line;
    const char                *conn_str;
    conn_cfg_t                 expected;
  } _cases[] = {
    {
      __LINE__,
      " driver = {    ax bcd ; ;   }; uid=xxx; pwd=yyy",
      {
        .driver                 = "ax bcd ; ;",
        .uid                    = "xxx",
        .pwd                    = "yyy",
      },
    },{
      // https://www.connectionstrings.com/dsn/
      __LINE__,
      "DSN=myDsn;Uid=myUsername;",
      {
        .dsn                    = "myDsn",
        .uid                    = "myUsername",
      },
    },{
      __LINE__,
      "Driver={any odbc driver's name};OdbcKey1=someValue;OdbcKey2=someValue;",
      {
        .driver                 = "any odbc driver's name",
      },
    },{
      __LINE__,
      // http://lunar.lyris.com/help/Content/sample_odbc_connection_str.html
      "Driver={SQL Server};Server=lmtest;Database=lmdb;Uid=sa;Pwd=pass",
      {
        .driver                 = "SQL Server",
        .uid                    = "sa",
        .pwd                    = "pass",
        .ip                     = "lmtest",
        .db                     = "lmdb",
      },
    },{
      __LINE__,
      "DSN=DSN_Name;Server=lmtest;Uid=lmuser;Pwd=pass",
      {
        .dsn                    = "DSN_Name",
        .uid                    = "lmuser",
        .pwd                    = "pass",
        .ip                     = "lmtest",
      },
    },{
      __LINE__,
      "Driver={MySQL ODBC 3.51 driver};server=lmtest;database=lmdb;uid=mysqluser;pwd=pass;",
      {
        .driver                 = "MySQL ODBC 3.51 driver",
        .uid                    = "mysqluser",
        .pwd                    = "pass",
        .ip                     = "lmtest",
        .db                     = "lmdb",
      },
    },{
      __LINE__,
      "Driver={MySQL ODBC 3.51 driver};server=lm.test;database=lmdb;uid=mysqluser;pwd=pass;",
      {
        .driver                 = "MySQL ODBC 3.51 driver",
        .uid                    = "mysqluser",
        .pwd                    = "pass",
        .ip                     = "lm.test",
        .db                     = "lmdb",
      },
    },{
      __LINE__,
      "Driver={MySQL ODBC 3.51 driver};server=lmtest:378;database=lmdb;uid=mysqluser;pwd=pass;",
      {
        .driver                 = "MySQL ODBC 3.51 driver",
        .uid                    = "mysqluser",
        .pwd                    = "pass",
        .ip                     = "lmtest",
        .db                     = "lmdb",
        .port                   = 378,
      },
    },{
      __LINE__,
      "Driver={MySQL ODBC 3.51 driver};server=lmtest:378378;database=lmdb;uid=mysqluser;pwd=pass;",
      {
        .driver                 = "MySQL ODBC 3.51 driver",
        .uid                    = "mysqluser",
        .pwd                    = "pass",
        .ip                     = "lmtest",
        .db                     = "lmdb",
        .port                   = 378378,
      },
    },{
      __LINE__,
      "Driver={MySQL ODBC 3.51 中文 driver};server=lmtest:378378378;database=lmdb;uid=mysqluser;pwd=pass;性别=你猜",
      {
        .driver                 = "MySQL ODBC 3.51 中文 driver",
        .uid                    = "mysqluser",
        .pwd                    = "pass",
        .ip                     = "lmtest",
        .db                     = "lmdb",
        .port                   = 378378378,
      },
    },{
      __LINE__,
      "Driver={MySQL ODBC 3.51 中文 driver};URL={taos://localhost:6041}",
      {
        .driver                 = "MySQL ODBC 3.51 中文 driver",
        .url                    = "taos://localhost:6041",
      },
    },{
      __LINE__,
      "DSN=TAOS_ODBC_DSN;UNSIGNED_PROMOTION=1;CHARSET_ENCODER_FOR_PARAM_BIND=UTF-8",
      {
        .dsn                    = "TAOS_ODBC_DSN",
        .unsigned_promotion     = 1,
        .charset_for_col_bind   = "UTF-8",
      },
    },
  };

  const size_t _cases_nr = sizeof(_cases)/sizeof(_cases[0]);
  for (size_t i=0; i<_cases_nr; ++i) {
    const conn_cfg_t *expected = &_cases[i].expected;
    conn_cfg_t parsed = {0};
    const char *s = _cases[i].conn_str;
    const int line = _cases[i].line;
    param.conn_cfg = &parsed;
    param.ctx.debug_flex = 1;
    // param.ctx.debug_bison = 1;
    int r = conn_parser_parse(s, strlen(s), &param);
    do {
      if (r) {
        parser_loc_t *loc = &param.ctx.bad_token;
        E("parsing[@line:%d]:%s", line, s);
        E("location:(%d,%d)->(%d,%d)",
            loc->first_line, loc->first_column, loc->last_line, loc->last_column);
        E("failed:%s", param.ctx.err_msg);
        break;
      }
      if (cmp_strs(expected->driver, param.conn_cfg->driver)) {
        E("parsing[@line:%d]:%s", line, s);
        E("driver expected to be `%s`, but got ==%s==", expected->driver, param.conn_cfg->driver);
        r = -1;
        break;
      }
      if (cmp_strs(expected->url, param.conn_cfg->url)) {
        E("parsing[@line:%d]:%s", line, s);
        E("url expected to be `%s`, but got ==%s==", expected->url, param.conn_cfg->url);
        r = -1;
        break;
      }
      if (cmp_strs(expected->dsn, param.conn_cfg->dsn)) {
        E("parsing[@line:%d]:%s", line, s);
        E("dsn expected to be `%s`, but got ==%s==", expected->dsn, param.conn_cfg->dsn);
        r = -1;
        break;
      }
      if (cmp_strs(expected->uid, param.conn_cfg->uid)) {
        E("parsing[@line:%d]:%s", line, s);
        E("uid expected to be `%s`, but got ==%s==", expected->uid, param.conn_cfg->uid);
        r = -1;
        break;
      }
      if (cmp_strs(expected->pwd, param.conn_cfg->pwd)) {
        E("parsing[@line:%d]:%s", line, s);
        E("pwd expected to be `%s`, but got ==%s==", expected->pwd, param.conn_cfg->pwd);
        r = -1;
        break;
      }
      if (cmp_strs(expected->ip, param.conn_cfg->ip)) {
        E("parsing[@line:%d]:%s", line, s);
        E("ip expected to be `%s`, but got ==%s==", expected->ip, param.conn_cfg->ip);
        r = -1;
        break;
      }
      if (cmp_strs(expected->db, param.conn_cfg->db)) {
        E("parsing[@line:%d]:%s", line, s);
        E("db expected to be `%s`, but got ==%s==", expected->db, param.conn_cfg->db);
        r = -1;
        break;
      }
      if (expected->port != param.conn_cfg->port) {
        E("parsing[@line:%d]:%s", line, s);
        E("port expected to be `%d`, but got ==%d==", expected->port, param.conn_cfg->port);
        r = -1;
        break;
      }
    } while (0);
    conn_parser_param_release(&param);
    conn_cfg_release(&parsed);
    if (r) return -1;
  }

  return 0;
}

static int test_ext_parser(void)
{
#define OK_TOPIC(x)     {__LINE__, x, 0, 0, 0, 0, 1, 1 }
#define BAD_TOPIC(x)    {__LINE__, x, 0, 0, 0, 0, 0, 1 }
#define OK_INSERT(...)  {__LINE__, ##__VA_ARGS__, 1, 0 }
#define BAD_INSERT(...) {__LINE__, ##__VA_ARGS__, 0, 0 }
  static const struct {
    int                     line;
    const char             *sql;
    uint8_t                 tbl_param;
    int8_t                  nr_tags;
    int8_t                  nr_cols;
    int8_t                  nr_params;
    uint8_t                 ok:1;
    uint8_t                 is_topic:1;
  } _cases [] = {
    // !topic
    OK_TOPIC("!topic demo"),
    OK_TOPIC("!topic demo {}"),
    OK_TOPIC("!topic demo {helloworld}"),
    OK_TOPIC("!topic demo {helloworld=good}"),
    OK_TOPIC("!topic demo {helloworld=good;helloworld=good}"),
    OK_TOPIC("!topic demo foo {helloworld=good; helloworld=good}"),
    OK_TOPIC(
      "!topic demo {"
      " enable.auto.commit=true;"
      " auto.commit.interval.ms=100;"
      " group.id=cgrpName;"
      " client.id=user_defined_name;"
      " td.connect.user=root;"
      " td.connect.pass=taosdata;"
      " auto.offset.reset=earliest;"
      " experimental.snapshot.enable=false"
      "}"),
    OK_INSERT("!insert into t (ts, v) values (1234,5)", 0, 0, 2, 0),
    OK_INSERT("!insert into t (ts, v) values (?, ?)", 0, 0, 2, 2),
    OK_INSERT("!insert into ? (ts, v) values (1234,5)", 1, 0, 2, 0),
    OK_INSERT("!insert into t.x (ts, v) values (1234,5)", 0, 0, 2, 0),
    OK_INSERT("!insert into ? using st (ts, v) values (123,4)", 1, 0, 2, 0),
    OK_INSERT("!insert into ? using st with (x,y) tags (4,5) (ts, v) values (123,4)", 1, 2, 2, 0),
    OK_INSERT("!insert into ? using st with (x,y) tags (4,5) (ts, v) values (123, add(4,5))", 1, 2, 2, 0),
    BAD_INSERT("!insert into \"t\" (ts, v) values (1234,5)", 0, 0, 2, 0),
    OK_INSERT("!insert into `t` (ts, v) values (1234,5)", 0, 0, 2, 0),
    BAD_INSERT("!insert into 't' (ts, v) values (1234,5)", 0, 0, 2, 0),
    BAD_INSERT("!insert into \"t\"\"t\" (ts, v) values (1234,5)", 0, 0, 2, 0),
    BAD_INSERT("!insert into 't''t' (ts, v) values (1234,5)", 0, 0, 2, 0),
    OK_INSERT("!insert into `t``t` (ts, v) values (1234,5)", 0, 0, 2, 0),
    OK_INSERT("!insert into t (s, v) values (-1234,5)", 0, 0, 2, 0),
    OK_INSERT("!insert into t (s, v) values ('h''w', 4)", 0, 0, 2, 0),
    OK_INSERT("!insert into t (s, v) values ('h''w', \"a\"\"b\")", 0, 0, 2, 0),
    OK_INSERT("!insert into t (s, v) values ('', \"a\"\"b\")", 0, 0, 2, 0),
    OK_INSERT("!insert into t (s, v) values (\"\", \"a\"\"b\")", 0, 0, 2, 0),
  };
  const size_t _nr_cases = sizeof(_cases) / sizeof(_cases[0]);
#undef BAD_INSERT
#undef OK_INSERT
#undef BAD_TOPIC
#undef OK_TOPIC

  for (size_t i=0; i<_nr_cases; ++i) {
    int         line            = _cases[i].line;
    const char *sql             = _cases[i].sql;
    uint8_t     ok              = _cases[i].ok;
    uint8_t     is_topic        = _cases[i].is_topic;
    ext_parser_param_t param = {0};
    param.ctx.debug_flex = 1;
    // param.ctx.debug_bison = 1;
    int r = ext_parser_parse(sql, strlen(sql), &param);
    if (!r != !!ok) {
      E("parsing:@%dL:%s", line, sql);
      if (r) {
        parser_loc_t *loc = &param.ctx.bad_token;
        E("location:(%d,%d)->(%d,%d)",
            loc->first_line, loc->first_column, loc->last_line, loc->last_column);
        E("failed:%s", param.ctx.err_msg);
      } else {
        E("expecting failure, but got ==success==");
      }
    } else if (r == 0) {
      A(is_topic == param.is_topic, "internal logic error");
      if (is_topic == 0) {
        insert_eval_t *insert_eval = &param.insert_eval;
        uint8_t     exp_tbl_param       = !!_cases[i].tbl_param;
        int8_t      exp_nr_tags         = _cases[i].nr_tags;
        int8_t      exp_nr_cols         = _cases[i].nr_cols;
        int8_t      exp_nr_params       = _cases[i].nr_params;
        uint8_t     tbl_param       = !!insert_eval->tbl_param;
        int8_t      nr_tags         = insert_eval_nr_tags(insert_eval);
        int8_t      nr_cols         = insert_eval_nr_cols(insert_eval);
        int8_t      nr_params       = insert_eval->nr_params;
        if (r == 0 && !!exp_tbl_param != !!tbl_param) {
          E("parsing:@%dL:%s", line, sql);
          E("expecting tbl '?', but got ==NONE==");
          r = -1;
        }
        if (r == 0 && exp_nr_tags != nr_tags) {
          E("parsing:@%dL:%s", line, sql);
          E("expecting %d tags, but got ==%d==", exp_nr_tags, nr_tags);
          r = -1;
        }
        if (r == 0 && exp_nr_cols != nr_cols) {
          E("parsing:@%dL:%s", line, sql);
          E("expecting %d cols, but got ==%d==", exp_nr_cols, nr_cols);
          r = -1;
        }
        if (r == 0 && exp_nr_params != nr_params) {
          E("parsing:@%dL:%s", line, sql);
          E("expecting %d params, but got ==%d==", exp_nr_params, nr_params);
          r = -1;
        }
      }
    }

    ext_parser_param_release(&param);
    if (!r != !!ok) return -1;
  }

  return 0;
}

static int test_ejson_parser(void)
{
#define RECORD(x, y) {x, y, __LINE__}
  const struct {
    const char            *text;
    const char            *match;
    int                    __line__;
  } _cases[] = {
    RECORD("\"abc\\tdef\"", "\"abc\\tdef\""),
    RECORD("{d}]", NULL),
    RECORD("'\\\\'", "\"\\\\\""),
    RECORD("'\\b'", "\"\\b\""),
    RECORD("'\\f'", "\"\\f\""),
    RECORD("'\\n'", "\"\\n\""),
    RECORD("'\\r'", "\"\\r\""),
    RECORD("'\\t'", "\"\\t\""),
    // // RECORD("'\\u12af'"),
    // // RECORD("'\\u12BC'"),
    // // RECORD("'\\ua'"),
    RECORD("true", "true"),
    RECORD("false", "false"),
    RECORD("null", "null"),
    RECORD("\"", NULL),
    RECORD("'", NULL),
    RECORD("`", NULL),
    RECORD("345", "345"),
    RECORD("-345.456e+123", "-3.45456e+125"),
    RECORD("+345.456E-123", "3.45456e-121"),
    RECORD("hello", "\"hello\""),
    RECORD("'hello'", "\"hello\""),
    RECORD("\"hello\"", "\"hello\""),
    RECORD("`hello`", "\"hello\""),
    RECORD("'345'", "\"345\""),
    RECORD("{}", "{}"),
    RECORD("[]", "[]"),
    RECORD("a", "\"a\""),
    RECORD("人", "\"人\""),
    RECORD("_人", "\"_人\""),
    RECORD("-人", NULL),
    RECORD("a-人", NULL),
    RECORD("[a]", "[\"a\"]"),
    RECORD("[人]", "[\"人\"]"),
    RECORD("[a,人]", "[\"a\",\"人\"]"),
    RECORD("[a,人,true,false,null]", "[\"a\",\"人\",true,false,null]"),
    RECORD("{a:b}", "{\"a\":\"b\"}"),
    RECORD("{a:b,测:试}", "{\"a\":\"b\",\"测\":\"试\"}"),
    RECORD("{'a':'b','测':'试'}", "{\"a\":\"b\",\"测\":\"试\"}"),
    RECORD("{'a':'b','测':'试','我们':[]}", "{\"a\":\"b\",\"测\":\"试\",\"我们\":[]}"),
    RECORD("{'a':'b','测':'试','我们':{}}", "{\"a\":\"b\",\"测\":\"试\",\"我们\":{}}"),
    RECORD("[a,人,[]]", "[\"a\",\"人\",[]]"),
    RECORD("[a,人,{}]", "[\"a\",\"人\",{}]"),
    RECORD("[true,false,null]", "[true,false,null]"),
    RECORD("{a:true,b:false,c:null}", "{\"a\":true,\"b\":false,\"c\":null}"),
    RECORD("{a:true,b:false,x,c:null}", "{\"a\":true,\"b\":false,\"x\":null,\"c\":null}"),
    RECORD("{a:true,b:false,x,c:null}]", NULL),
    RECORD("{a:true,b:false,x,c:}", "{\"a\":true,\"b\":false,\"x\":null,\"c\":null}"),
    RECORD("{,,a:true,,b:false,x,,}", "{\"a\":true,\"b\":false,\"x\":null}"),
    RECORD("[{a:true,b:false,x,c:null,d:{m,n},e:[]},[x,y,z]]", "[{\"a\":true,\"b\":false,\"x\":null,\"c\":null,\"d\":{\"m\":null,\"n\":null},\"e\":[]},[\"x\",\"y\",\"z\"]]"),
    RECORD("[{a:true,b:false,x,c:null,d:{m,n},e:[]},[x,y,z]", NULL),
    RECORD("[{a:true,b:false,x,c:null,d:{m,n},e:[]},[x,y,z", NULL),
    RECORD("[{a:true,b:false,x,c:null,d:{m,n},e:[]},[x,y,", NULL),
    RECORD("[{a:true,b:false,x,c:null,d:{m,n},e:[]},[x,y", NULL),
    RECORD("[{a:true,b:false,x,c:null,d:{m,n},e:[]},[x,", NULL),
    RECORD("[{a:true,b:false,x,c:null,d:{m,n},e:[]},[x", NULL),
    RECORD("[{a:true,b:false,x,c:null,d:{m,n},e:[]},[", NULL),
    RECORD("[{a:true,b:false,x,c:null,d:{m,n},e:[]},", NULL),
    RECORD("[{a:true,b:false,x,c:null,d:{m,n},e:[]}", NULL),
    RECORD("[{a:true,b:false,x,c:null,d:{m,n},e:[]", NULL),
    RECORD("[{a:true,b:false,x,c:null,d:{m,n},e:[", NULL),
    RECORD("[{a:true,b:false,x,c:null,d:{m,n},e:", NULL),
    RECORD("[{a:true,b:false,x,c:null,d:{m,n},e", NULL),
    RECORD("[{a:true,b:false,x,c:null,d:{m,n},", NULL),
    RECORD("[{a:true,b:false,x,c:null,d:{m,n}", NULL),
    RECORD("[{a:true,b:false,x,c:null,d:{m,n", NULL),
    RECORD("[{a:true,b:false,x,c:null,d:{m,", NULL),
    RECORD("[{a:true,b:false,x,c:null,d:{m", NULL),
    RECORD("[{a:true,b:false,x,c:null,d:{", NULL),
    RECORD("[{a:true,b:false,x,c:null,d:", NULL),
    RECORD("[{a:true,b:false,x,c:null,d", NULL),
    RECORD("[{a:true,b:false,x,c:null,", NULL),
    RECORD("[{a:true,b:false,x,c:null", NULL),
    RECORD("[{a:true,b:false,x,c:nul", NULL),
    RECORD("[{a:true,b:false,x,c:", NULL),
    RECORD("[{a:true,b:false,x,c", NULL),
    RECORD("[{a:true,b:false,x,", NULL),
    RECORD("[{a:true,b:false,x", NULL),
    RECORD("[{a:true,b:false,", NULL),
    RECORD("[{a:true,b:false", NULL),
    RECORD("[{a:true,b:fals", NULL),
    RECORD("[{a:true,b:", NULL),
    RECORD("[{a:true,b", NULL),
    RECORD("[{a:true,", NULL),
    RECORD("[{a:true", NULL),
    RECORD("[{a:tru", NULL),
    RECORD("[{a:", NULL),
    RECORD("[{a", NULL),
    RECORD("[{", NULL),
    RECORD("[", NULL),
    RECORD("[,,,,a,,,b,,,,,,]", "[\"a\",\"b\"]"),
    RECORD("[,,,,,,,,,,,,,]", "[]"),
    RECORD("{,,,,,,,,,,,,,}", "{}"),
    RECORD("{,,,,x,,,y,,,,,,}", "{\"x\":null,\"y\":null}"),
    RECORD("{,,,,x,,,,,,,,,,}", "{\"x\":null}"),
  };
  const size_t nr = sizeof(_cases)/sizeof(_cases[0]);
#undef RECORD

  ejson_parser_param_t param = {0};
  param.ctx.debug_flex = 1;
  // param.ctx.debug_bison = 1;

  char buf[4096]; buf[0] = '\0';
  for (size_t i=0; i<nr; ++i) {
    const char *s         = _cases[i].text;
    const char *match     = _cases[i].match;
    int         __line__  = _cases[i].__line__;
    int r = ejson_parser_parse(s, strlen(s), &param);
    buf[0] = '\0';
    do {
      if (r == 0) {
        int n = ejson_serialize(param.ejson, buf, sizeof(buf));
        E("parsing @[%dL]:%s", __line__, s);
        E("dumping [n:%d]", n);
        E("%s", buf);
      }
      if ((!!r) ^ (!match)) {
        E("parsing @[%dL]:%s", __line__, s);
        if (r) {
          parser_loc_t *loc = &param.ctx.bad_token;
          E("location:(%d,%d)->(%d,%d)",
              loc->first_line, loc->first_column, loc->last_line, loc->last_column);
          E("failed:%s", param.ctx.err_msg);
          E("expecting:%s", match);
          E("but got:%s", buf);
        } else {
          E("expected to fail, but succeed");
        }
        r = -1;
        break;
      }
      if (r == 0) {
        if (strcmp(buf, match)) {
          E("parsing @[%dL]:%s", __line__, s);
          E("expecting:[%zd]%s", strlen(match), match);
          E("but got:[%zd]%s", strlen(buf), buf);
          r = -1;
          break;
        }
      }
      r = 0;
    } while (0);

    ejson_parser_param_release(&param);
    if (r) return -1;
  }

  return 0;
}

typedef struct sqls_check_s         sqls_check_t;
typedef struct expected_sql_s       expected_sql_t;
struct expected_sql_s {
  const char                *sql;
  int32_t                    qms;
};

struct sqls_check_s {
  const char                *sqls;
  const expected_sql_t      *expects;
  size_t                     expects_cap;
  size_t                     idx;

  uint8_t                    failed:1;
};

static int _sql_found(sqls_parser_param_t *param, size_t _start, size_t _end, int32_t qms, void *arg)
{
  (void)param;

  sqls_check_t *check = (sqls_check_t*)arg;
  const char *sqls = check->sqls;
  --_end;

  if (check->idx >= check->expects_cap) {
    E("expected:<empty>");
    E("but  got:[%.*s]", (int)(_end - _start), sqls + _start);
    check->failed = 1;
    return -1;
  }

  const expected_sql_t *expected = check->expects + check->idx;
  if (strlen(expected->sql) != (_end - _start)) {
    E("expected:[%s]", expected->sql);
    E("but  got:[%.*s]", (int)(_end - _start), sqls + _start);
    check->failed = 1;
    return -1;
  }
  if (strncmp(expected->sql, sqls + _start, _end - _start)) {
    E("expected:[%s]", expected->sql);
    E("but  got:[%.*s]", (int)(_end - _start), sqls + _start);
    check->failed = 1;
    return -1;
  }

  if (expected->qms != qms) {
    E("sql:%s", expected->sql);
    E("parameter-place-marker expected:[%d]", expected->qms);
    E("but  got:[%d]", qms);
    check->failed = 1;
    return -1;
  }

  ++check->idx;
  return 0;
}

static int test_sqls_parser(void)
{
  const struct {
    const char *sqls;
    const expected_sql_t expects[16];
  } _cases[] = {
    {
      "insert into t (ts, name)",
      {
        {"insert into t (ts, name)", 0},
      },
    },{
      "insert into t (ts, name) values (now(), value)",
      {
        {"insert into t (ts, name) values (now(), value)", 0},
      },
    },{
      "select * from a;xselect * from bb;insert into t (ts, name) values (now(), \"hello\")",
      {
        {"select * from a", 0},
        {"xselect * from bb", 0},
        {"insert into t (ts, name) values (now(), \"hello\")", 0},
      },
    },{
      "select * from a;select * from bb;insert into t (ts, name) values (now(), \"世界\")",
      {
        {"select * from a", 0},
        {"select * from bb", 0},
        {"insert into t (ts, name) values (now(), \"世界\")", 0},
      },
    },{
      "    select * from a   ;   select * from b     ;     insert into t (ts, name) values (now(), \"世界\")      ",
      {
        {"select * from a", 0},
        {"select * from b", 0},
        {"insert into t (ts, name) values (now(), \"世界\")", 0},
      },
    },{
      "select 34 as `abc`",
      {
        {"select 34 as `abc`", 0},
      },
    },{
      "    select * \r\n from a  \r\n ;  select * \nfrom b     ;     insert into \nt (ts, \nname) values (now(), \"世界\")  ;;;;;;    ",
      {
        {"select * \r\n from a", 0},
        {"select * \nfrom b", 0},
        {"insert into \nt (ts, \nname) values (now(), \"世界\")", 0},
      },
    },{
      "    select * from \f a  ;   select * from b     ;  \r   insert \rinto t (ts, name) values (now(), \"x世界\")      ",
      {
        {"select * from \f a", 0},
        {"select * from b", 0},
        {"insert \rinto t (ts, name) values (now(), \"x世界\")", 0},
      },
    },{
      "",
      {
        {NULL, 0},
      },
    },{
      "ab",
      {
        {"ab", 0},
      },
    },{
      ";;;;;ab;;;;;;;",
      {
        {"ab", 0},
      },
    },{
      "!topic demo good {group.id=cgrpName; enable.auto.commit=false; auto.commit.interval.ms=10000}",
      {
        {"!topic demo good {group.id=cgrpName; enable.auto.commit=false; auto.commit.interval.ms=10000}", 0},
      },
    },{
      "select * from t where x = ?",
      {
        {"select * from t where x = ?", 1},
      },
    },{
      "select * from t where x = '?'",
      {
        {"select * from t where x = '?'", 0},
      },
    },{
      "insert into t (ts, name) values (now(), ?)",
      {
        {"insert into t (ts, name) values (now(), ?)", 1},
      },
    },{
      "insert into ? (ts, name) values (now(), 'a''')",
      {
        {"insert into ? (ts, name) values (now(), 'a''')", 1},
      },
    },{
      "insert into ? (ts, name) values (now(), `a```)",
      {
        {"insert into ? (ts, name) values (now(), `a```)", 1},
      },
    },{
      "insert into ? (ts, name) values (now(), \"a\"\"\")",
      {
        {"insert into ? (ts, name) values (now(), \"a\"\"\")", 1},
      },
    },{
      "insert into ? using tags (?, ?) values (?, ?)",
      {
        {"insert into ? using tags (?, ?) values (?, ?)", 5},
      },
    }
  };
  const size_t _cases_nr = sizeof(_cases)/sizeof(_cases[0]);
  for (size_t i=0; i<_cases_nr; ++i) {
    const char  *sqls             = _cases[i].sqls;
    const expected_sql_t *expects = _cases[i].expects;

    sqls_check_t check = {
      .sqls        = sqls,
      .expects     = expects,
      .expects_cap = sizeof(_cases[0].expects)/sizeof(_cases[0].expects[0]),
      .failed      = 0,
    };

    sqls_parser_param_t param = {0};
    // param.ctx.debug_flex = 1;
    // param.ctx.debug_bison = 1;
    param.sql_found = _sql_found;
    param.arg       = &check;

    D("parsing:\n%s ...", sqls);
    int r = sqls_parser_parse(sqls, strlen(sqls), &param);
    if (r) {
      parser_loc_t *loc = &param.ctx.bad_token;
      E("location:(%d,%d)->(%d,%d)",
          loc->first_line, loc->first_column, loc->last_line, loc->last_column);
      E("failed:%s", param.ctx.err_msg);
    } else if (check.failed) {
      r = -1;
    } else if (expects[check.idx].sql) {
      E("expected:[%s]", expects[check.idx].sql);
      E("but  got:<null>");
      r = -1;
    }

    sqls_parser_param_release(&param);
    if (r) return -1;
  }
  return 0;
}

static int test_url_parser(void)
{
#define RECORD(x,y) {__LINE__, x, y}
  struct {
    int             line;
    const char     *url;
    const char     *ok_or_failure;
  } _cases[] = {
    RECORD("http://example.com/h/g?fasd?fsd=fsd#fasd", "http://example.com/h/g?fasd?fsd=fsd#fasd"),
    RECORD("foo://example.com:8042/over/there?name=ferret#nose", "foo://example.com:8042/over/there?name=ferret#nose"),
    RECORD("http://192.168.0.1", "http://192.168.0.1"),
    RECORD("http://foo:bar@255.255.255.255/root?name=foo?age=3?path=/fad#first?what", "http://foo:bar@255.255.255.255/root?name=foo?age=3?path=/fad#first?what"),
    RECORD("http://example.com/~/h/g?fda%5e~", "http://example.com/~/h/g?fda%5e~"),
    RECORD("http://example.com/~/%5eh/g%5e?fda%5e~", "http://example.com/~/%5eh/g%5e?fda%5e~"),
    RECORD("http://f%5eoo:b%5eh@192.168.0.1", "http://f%5eoo:b%5eh@192.168.0.1"),
    RECORD("urn:example:animal:ferret:nose", "urn:example:animal:ferret:nose"),
    RECORD("http://www.com/#", "http://www.com/#"),
    RECORD("http://www.com/?", "http://www.com/?"),
    RECORD("http://www.com#", "http://www.com#"),
    RECORD("http://www.com?", "http://www.com?"),
    RECORD("http://example.com/根", "(0,19)->(0,20)"),
    RECORD("file:///fasd", "(0,0)->(0,4)"),
    RECORD("foo:/abc:def", "foo:/abc:def"),
    RECORD("http://hello%20world.com", "http://hello%20world.com"),
  };
#undef RECORD
  const size_t _cases_nr = sizeof(_cases)/sizeof(_cases[0]);
  for (size_t i=0; i<_cases_nr; ++i) {
    int line = _cases[i].line;
    const char *url = _cases[i].url;
    const char *ok_or_failure = _cases[i].ok_or_failure;
    url_parser_param_t param = {0};
    param.ctx.debug_flex = 1;
    // param.ctx.debug_bison = 1;

    int r = url_parser_parse(url, strlen(url), &param);
    if (r) {
      parser_loc_t *loc = &param.ctx.bad_token;
      char buf[4096];
      snprintf(buf, sizeof(buf), "(%d,%d)->(%d,%d)",
          loc->first_line, loc->first_column, loc->last_line, loc->last_column);
      if (strcmp(buf, ok_or_failure) == 0) {
        r = 0;
      } else {
        E("parsing @[%dL]:%s", line, url);
        E("location:(%d,%d)->(%d,%d)",
            loc->first_line, loc->first_column, loc->last_line, loc->last_column);
        E("failed:%s", param.ctx.err_msg);
      }
    } else {
      char *out = NULL;
      url_encode(&param.url, &out);
      if (strcmp(ok_or_failure, out)) {
        E("parsing success @[%dL]:%s", line, url);
        E("expecting:         %s", ok_or_failure);
        E("but does not match:%s", out);
        r = -1;
      }
      TOD_SAFE_FREE(out);
    }

    url_parser_param_release(&param);
    if (r) return -1;
  }
  return 0;
}

static int _wildcard_match(const string_t *ex, const string_t *str, const int match)
{
  int r;
  wildex_t *wild = NULL;

  r = wildcomp(&wild, ex);
  if (r) {
    E("failed to compile wildcard `%.*s`", (int)ex->bytes, ex->str);
    return -1;
  }

  int matched = 0;
  r = wildexec(wild, str, &matched);
  if (r) {
    E("wildcard match failed");
    return -1;
  }
  if ((!matched) != (!match)) {
    if (!matched) E("`%.*s` does not match by `%.*s`", (int)str->bytes, str->str, (int)ex->bytes, ex->str);
    else          E("`%.*s` unexpectedly match by `%.*s`", (int)str->bytes, str->str, (int)ex->bytes, ex->str);
    r = -1;
  } else {
    r = 0;
  }

  wildfree(wild);

  return r ? -1 : 0;
}

static int test_wildmatch(void)
{
  int r = 0;

#ifdef _WIN32            /* { */
  const char charset[] = "GB18030";
#else                    /* }{ */
  const char charset[] = "UTF-8";
#endif                   /* } */

  struct {
    const char *pattern;
    const char *content;
    int         match;
  } cases[] = {
    {"hello",            "hello",              1},
    {"%",                "hello",              1},
    {"%%%%%%%%%%%%%%%%", "hello",              1},
    {"_",                "h",                  1},
    {"_____",            "hello",              1},
    {"_%lo",             "hello",              1},
    {"_%el%o",           "hello",              1},
    {"_%ll%o",           "hello",              1},
    {"_a",               "人a",                1},
    {"a_",               "ah",                 1},
    {"a_",               "a人",                1},
    {"_a_",              "hah",                1},
    {"_%好啊",           "你好啊",             1},
    {"_%%好%%啊",        "你好啊",             1},
    {"_%%x好%%啊",       "你好啊",             0},
    {"你__",             "你好啊",             1},
    {"你%_%_%",          "你好啊",             1},
    {"你%%_%%_%%",       "你好啊",             1},
    {"你___",            "你好啊",             0},
    {"你\\_",            "你h",                0},
    {"你\\_",            "你_",                1},
    {"你\\_",            "你.",                0},
    {"",                 "",                   1},
  };

  for (size_t i=0; i<sizeof(cases)/sizeof(cases[0]); ++i) {
    string_t pattern = {
      .charset              = charset,
      .str                  = cases[i].pattern,
      .bytes                = strlen(cases[i].pattern),
    };
    string_t content = {
      .charset              = charset,
      .str                  = cases[i].content,
      .bytes                = strlen(cases[i].content),
    };
    r = _wildcard_match(&pattern, &content, cases[i].match);
    if (r) return -1;
  }

  return 0;
}

static int test_basename_dirname(void)
{
  const char *path = "/Users/foo/bar.txt";
  char buf[PATH_MAX + 1];
  char *p;
  p = tod_basename(path, buf, sizeof(buf));
  if (strcmp(p, "bar.txt")) {
    E("`bar.txt` expected, but got ==%s==", p);
    return -1;
  }
  p = tod_dirname(path, buf, sizeof(buf));
#ifdef _WIN32
  errno_t err;
  char drive[16];
  char dir[PATH_MAX+1];
  char fname[PATH_MAX+1];
  char ext[PATH_MAX+1];
  err = _splitpath_s(p, drive, sizeof(drive), dir, sizeof(dir), fname, sizeof(fname), ext, sizeof(ext));
  char fullpath[PATH_MAX+1];
  snprintf(fullpath, sizeof(fullpath), "%s%s%s", dir, fname, ext);
  if (strcmp(fullpath, "\\Users\\foo")) {
    E("`\\Users\\foo` expected, but got ==%s==", fullpath);
    return -1;
  }
#else
  if (strcmp(p, "/Users/foo")) {
    E("`/Users/foo` expected, but got ==%s==", p);
    return -1;
  }
#endif
  return 0;
}

static int test_pthread_once_tick = 0;
static void test_pthread_once_init(void)
{
  ++test_pthread_once_tick;
}

static int test_pthread_once_step(void)
{
  static pthread_once_t once = PTHREAD_ONCE_INIT;
  int r = 0;
  r = pthread_once(&once, test_pthread_once_init);
  if (r) return -1;
  if (test_pthread_once_tick != 1) return -1;

  return 0;
}

static int test_pthread_once(void)
{
  int r = 0;
  r = test_pthread_once_step();
  if (r) return -1;

  r = test_pthread_once_step();
  if (r) return -1;

  return 0;
}

static int test_iconv(void)
{
  int r = 0;

  const char *tocode = "GB18030";
  const char *fromcode = "UTF-8";

  iconv_t cnv = iconv_open(tocode, fromcode);
  if (!cnv) {
    int e = errno;
    E("iconv_open(tocode:%s, fromcode:%s) failed:[%d]%s", tocode, fromcode, e, strerror(e));
    return -1;
  }

  do {
    char buf[1024];
    char utf8[] = "\xe4\xb8\xad\xe6\x96\x87"; //"中文";
    char          *inbuf                = utf8;
    size_t         inbytes              = sizeof(utf8);
    size_t         inbytesleft          = inbytes;
    char          *outbuf               = buf;
    size_t         outbytes;
    size_t         outbytesleft;
    size_t n;

    outbytes = 0;
    outbytesleft = outbytes;
    n = iconv(cnv, &inbuf, &inbytesleft, &outbuf, &outbytesleft);
    A(n == (size_t)-1, "-1 expected, but got ==%zd==", n);
    A(errno == E2BIG, "[%d]%s", errno, strerror(errno));
    A(inbytes - inbytesleft == 0, "0 expected, but got ==%zd==", inbytes - inbytesleft);
    A(inbuf - utf8 == 0, "");
    A(outbytes - outbytesleft == 0, "");
    A(outbuf - buf == 0, "");

    outbytes = 3;
    outbytesleft = outbytes;
    n = iconv(cnv, &inbuf, &inbytesleft, &outbuf, &outbytesleft);
    A(n == (size_t)-1, "-1 expected, but got ==%zd==", n);
    A(errno == E2BIG, "");
    A(inbytes - inbytesleft == 3, "3 expected, but got ==%zd==", inbytes - inbytesleft);
    A(inbuf - utf8 == 3, "");
    A(outbytes - outbytesleft == 2, "");
    A(outbuf - buf == 2, "");
    A(strncmp(buf, "\xd6\xd0", 2)==0, "");

    outbytes = 2;
    outbytesleft = outbytes;
    n = iconv(cnv, &inbuf, &inbytesleft, &outbuf, &outbytesleft);
    A(n == (size_t)-1, "-1 expected, but got ==%zd==", n);
    A(errno == E2BIG, "");
    A(inbytes - inbytesleft == 6, "6 expected, but got ==%zd==", inbytes - inbytesleft);
    A(outbytes - outbytesleft == 2, "");
    A(strncmp(buf, "\xd6\xd0\xce\xc4", 4)==0, "");

    outbytes = 1;
    outbytesleft = outbytes;
    n = iconv(cnv, &inbuf, &inbytesleft, &outbuf, &outbytesleft);
    A(n == 0, "0 expected, but got ==%zd==", n);
    A(inbytes - inbytesleft == 7, "7 expected, but got ==%zd==", inbytes - inbytesleft);
    A(outbytes - outbytesleft == 1, "");
    A(outbuf[-1] == '\0', "");
  } while (0);

  iconv_close(cnv);

  return r ? -1 : 0;
}


static int _test_iconv_perf_gen_iconv(iconv_t *cnv)
{
  const char *tocode = "GB18030";
  const char *fromcode = "UTF-8";

  *cnv = iconv_open(tocode, fromcode);
  if (!*cnv) {
    int e = errno;
    E("iconv_open(tocode:%s, fromcode:%s) failed:[%d]%s", tocode, fromcode, e, strerror(e));
    return -1;
  }

  return 0;
}

static int _test_iconv_perf(iconv_t cnv)
{
  int r = 0;
  size_t n = 0;

  const char src[] = "hello";
  char buf[4096];

  for (size_t i=0; i<1024*16; ++i) {
    char          *inbuf                = (char*)src;
    size_t         inbytesleft          = sizeof(src);
    char          *outbuf               = buf;
    size_t         outbytesleft         = sizeof(buf);

    if (cnv == NULL) {
      iconv_t cv;
      r = _test_iconv_perf_gen_iconv(&cv);
      if (r) return -1;

      n = iconv(cv, &inbuf, &inbytesleft, &outbuf, &outbytesleft);
      if (n == (size_t)-1) r = -1;

      iconv_close(cv);
    } else {
      n = iconv(cnv, &inbuf, &inbytesleft, &outbuf, &outbytesleft);
      if (n == (size_t)-1) r = -1;
    }

    if (r) return -1;
  }

  return 0;
}

static int test_iconv_perf_reuse(void)
{
  int r = 0;

  iconv_t cnv;
  r = _test_iconv_perf_gen_iconv(&cnv);
  if (r) return -1;

  r = _test_iconv_perf(cnv);

  iconv_close(cnv);

  return r;
}

static int test_iconv_perf_on_the_fly(void)
{
  int r = 0;

  iconv_t cnv = NULL;

  r = _test_iconv_perf(cnv);

  return r;
}
static int get_int(void)
{
  static int tick = 0;
  return tick++;
}

static int test_buffer(void)
{
  buffer_t str = {0};
  buffer_concat_fmt(&str, "%d", get_int());
  if (strcmp("0", str.base)) {
    E("0 expected, but got ==%s==", str.base);
    return -1;
  }
  int r = 0;
  do {
    int tick = get_int();
    if (tick != 1) {
      E("1 expected, but got ==%d==", tick);
      r = -1;
      break;
    }
  } while (0);
  buffer_release(&str);
  return r ? -1 : 0;
}

static int test_trim(void)
{
  struct {
    const char *src;
    const char *chk;
  } _cases[] = {
    {"", ""},
    {" ", ""},
    {"  ", ""},
    {"abc", "abc"},
    {" abc", "abc"},
    {"  abc", "abc"},
    {"abc ", "abc"},
    {"abc  ", "abc"},
    {" abc", "abc"},
    {" abc ", "abc"},
    {" abc  ", "abc"},
    {"  abc", "abc"},
    {"  abc ", "abc"},
    {"  abc  ", "abc"},
    {"a\0b", "a"},
  };

  for (size_t i=0; i<sizeof(_cases)/sizeof(_cases[0]); ++i) {
    const char *start, *end;
    const char *src = _cases[i].src;
    const char *chk = _cases[i].chk;
    trim_string(src, -1, &start, &end);
    size_t nr = end - start;
    if (strncmp(chk, start, nr) == 0 && chk[nr] == '\0') continue;
    DUMP("case[%zd]:expecting `%s`, but got ==%.*s==", i+1, chk, (int)nr, start);
    return -1;
  }

  return 0;
}

static int test_gettimeofday(void)
{
  int r = 0;
  time_t t0 = 0;
  time(&t0);
  struct timeval tv0 = {0};
  r = gettimeofday(&tv0, NULL);
  if (r) {
    DUMP("gettimeofday failed:[%d]%s", errno, strerror(errno));
    return -1;
  }
  DUMP("timeval:%" PRId64 ",%" PRId64 "", (int64_t)tv0.tv_sec, (int64_t)tv0.tv_usec);
  DUMP("t0:%" PRId64 "", (int64_t)t0);
  struct tm tm0 = {0};
  t0 = (time_t)tv0.tv_sec;
  localtime_r(&t0, &tm0);
  DUMP("local:%04d-%02d-%02d %02d:%02d:%02d.%06zd",
    tm0.tm_year + 1900, tm0.tm_mon + 1, tm0.tm_mday,
    tm0.tm_hour, tm0.tm_min, tm0.tm_sec,
    (size_t)tv0.tv_usec);
  return 0;
}

typedef int (*test_case_f)(void);

#define RECORD(x) {x, #x}

static struct {
  test_case_f           func;
  const char           *name;
} _cases[] = {
  RECORD(test_url_parser),
  RECORD(test_conn_parser),
  RECORD(test_ext_parser),
  RECORD(test_sqls_parser),
  RECORD(test_ejson_parser),
  RECORD(test_url_parser),
  RECORD(test_wildmatch),
  RECORD(test_basename_dirname),
  RECORD(test_pthread_once),
  RECORD(test_iconv),
  RECORD(test_iconv_perf_reuse),
  RECORD(test_iconv_perf_on_the_fly),
  RECORD(test_buffer),
  RECORD(test_trim),
  RECORD(test_gettimeofday),
};

static void usage(const char *arg0)
{
  DUMP("usage:");
  DUMP("  %s -h", arg0);
  DUMP("");
  DUMP("supported test cases:");
  for (size_t i=0; i<sizeof(_cases)/sizeof(_cases[0]); ++i) {
    DUMP("  %s", _cases[i].name);
  }
}

static int run(const char *test_case)
{
  int r = 0;

  int tested = 0;

  for (size_t i=0; i<sizeof(_cases)/sizeof(_cases[0]); ++i) {
    const char *name = _cases[i].name;
    if (!test_case || strcmp(name, test_case) == 0) {
      tested = 1;
      r = _cases[i].func();
      if (r) return -1;
    }
  }

  if (!tested) {
    fprintf(stderr, "test case [%s] not exists\n", test_case);
    return -1;
  }

  return 0;
}

int main(int argc, char *argv[])
{
  int r = 0;

  int tested = 0;

  for (int i=1; i<argc; ++i) {
    if (strcmp(argv[i], "-h") == 0) {
      usage(argv[0]);
      return 0;
    }
    tested = 1;
    r = run(argv[i]);
    if (r) return 1;
  }

  if (!tested) r = run(NULL);

  fprintf(stderr,"==%s==\n", r ? "failure" : "success");

  return !!r;
}

