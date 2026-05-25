#include "os_port.h"

#include <taos.h>

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#define tzset _tzset
#endif

#define DUMP(fmt, ...) fprintf(stderr, fmt "\n", ##__VA_ARGS__)

typedef struct args_s {
  const char *host;
  uint16_t    port;
  const char *user;
  const char *pass;
  const char *driver;
  int         pre_tzset;
  int         post_tzset;
} args_t;

static const char *get_tz_env(void)
{
#ifdef _WIN32
  static char buf[256] = {0};
  char *v = NULL;
  size_t n = 0;
  errno_t rc = _dupenv_s(&v, &n, "TZ");
  if (rc || !v || !*v) {
    if (v) free(v);
    snprintf(buf, sizeof(buf), "(null)");
    return buf;
  }
  snprintf(buf, sizeof(buf), "%s", v);
  free(v);
  return buf;
#else
  const char *tz = getenv("TZ");
  return tz ? tz : "(null)";
#endif
}

#ifdef _WIN32
static const char *get_tz_env_winapi(void)
{
  static char buf[256] = {0};
  DWORD n = GetEnvironmentVariableA("TZ", buf, (DWORD)sizeof(buf));
  if (n == 0) {
    snprintf(buf, sizeof(buf), "(null)");
    return buf;
  }
  if (n >= sizeof(buf)) {
    snprintf(buf, sizeof(buf), "(too-long)");
    return buf;
  }
  return buf;
}
#endif

static int parse_u16(const char *s, uint16_t *v)
{
  if (!s || !v) return -1;
  char *end = NULL;
  unsigned long x = strtoul(s, &end, 10);
  if (!end || *end || x > 65535ul) return -1;
  *v = (uint16_t)x;
  return 0;
}

static int parse_driver(const char *s, const char **driver)
{
  if (strcmp(s, "native") == 0 || strcmp(s, "websocket") == 0) {
    *driver = s;
    return 0;
  }
  return -1;
}

static void dump_crt_tz_state(void)
{
#ifdef _WIN32
  long sec_west = 0;
  int daylight = 0;
  char std_name[64] = {0};
  char dst_name[64] = {0};
  size_t std_len = 0;
  size_t dst_len = 0;
  errno_t e1 = _get_timezone(&sec_west);
  errno_t e2 = _get_daylight(&daylight);
  errno_t e3 = _get_tzname(&std_len, std_name, sizeof(std_name), 0);
  errno_t e4 = _get_tzname(&dst_len, dst_name, sizeof(dst_name), 1);
  DUMP("CRT: _get_timezone=%d(%ld sec west), _get_daylight=%d(%d), _get_tzname(std)=%d(%s), _get_tzname(dst)=%d(%s)",
      (int)e1, sec_west, (int)e2, daylight, (int)e3, std_name, (int)e4, dst_name);
#endif
}

static void dump_modules(void)
{
#ifdef _WIN32
  const char *mods[] = {"taos.dll", "taosnative.dll", "taosws.dll"};
  for (size_t i = 0; i < sizeof(mods) / sizeof(mods[0]); ++i) {
    HMODULE h = GetModuleHandleA(mods[i]);
    if (!h) {
      DUMP("%s: (not loaded)", mods[i]);
      continue;
    }
    char path[MAX_PATH] = {0};
    DWORD n = GetModuleFileNameA(h, path, (DWORD)sizeof(path));
    if (n == 0 || n >= sizeof(path)) {
      DUMP("%s: loaded, path unavailable", mods[i]);
      continue;
    }
    DUMP("%s: %s", mods[i], path);
  }
#endif
}

static void snapshot(const char *tag)
{
  DUMP("---- %s ----", tag);
  DUMP("env.TZ=%s", get_tz_env());
#ifdef _WIN32
  DUMP("winapi.TZ=%s", get_tz_env_winapi());
#endif
  dump_crt_tz_state();
  dump_modules();
}

static int exec_sql(TAOS *taos, const char *sql)
{
  TAOS_RES *res = taos_query(taos, sql);
  if (!res) {
    DUMP("taos_query returned NULL: %s", sql);
    return -1;
  }
  int eno = taos_errno(res);
  if (eno) {
    DUMP("taos_query failed[%d]: %s; sql: %s", eno, taos_errstr(res), sql);
    taos_free_result(res);
    return -1;
  }
  taos_free_result(res);
  return 0;
}

static int fetch_one_ts_raw(TAOS *taos, const char *sql, int64_t *raw, int *precision)
{
  TAOS_RES *res = taos_query(taos, sql);
  if (!res) {
    DUMP("taos_query returned NULL: %s", sql);
    return -1;
  }
  int eno = taos_errno(res);
  if (eno) {
    DUMP("taos_query failed[%d]: %s; sql: %s", eno, taos_errstr(res), sql);
    taos_free_result(res);
    return -1;
  }

  if (taos_field_count(res) != 1) {
    DUMP("unexpected field count");
    taos_free_result(res);
    return -1;
  }

  TAOS_ROW row = taos_fetch_row(res);
  if (!row || !row[0]) {
    DUMP("taos_fetch_row got empty row");
    taos_free_result(res);
    return -1;
  }

  *raw = *(const int64_t*)row[0];
  *precision = taos_result_precision(res);
  taos_free_result(res);
  return 0;
}

static int format_local_timestamp(int64_t val, int precision, char *buf, size_t len)
{
  if (!buf || len == 0) return -1;

  time_t tt = 0;
  int32_t fraction = 0;
  int width = 0;

  switch (precision) {
    case 2:
      tt = (time_t)(val / 1000000000);
      fraction = (int32_t)(val % 1000000000);
      width = 9;
      break;
    case 1:
      tt = (time_t)(val / 1000000);
      fraction = (int32_t)(val % 1000000);
      width = 6;
      break;
    case 0:
      tt = (time_t)(val / 1000);
      fraction = (int32_t)(val % 1000);
      width = 3;
      break;
    default:
      return -1;
  }

  struct tm tmv = {0};
#ifdef _WIN32
  errno_t rc = localtime_s(&tmv, &tt);
  if (rc) return -1;
#else
  if (!localtime_r(&tt, &tmv)) return -1;
#endif

  int n = snprintf(buf, len,
      "%04d-%02d-%02d %02d:%02d:%02d.%0*d",
      tmv.tm_year + 1900, tmv.tm_mon + 1, tmv.tm_mday,
      tmv.tm_hour, tmv.tm_min, tmv.tm_sec,
      width, fraction);
  if (n <= 0 || (size_t)n >= len) return -1;
  return 0;
}

static void usage(const char *app)
{
  DUMP("usage:");
  DUMP("  %s [--driver native|websocket] [--host <host>] [--port <port>] [--user <user>] [--pass <pass>] [--pre-tzset] [--post-tzset]", app);
  DUMP("  default: --driver native --port 6030");
}

static int parse_args(int argc, char *argv[], args_t *args)
{
  memset(args, 0, sizeof(*args));
  args->host = "127.0.0.1";
  args->port = 6030;
  args->driver = "native";

  for (int i = 1; i < argc; ++i) {
    if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
      usage(argv[0]);
      return 1;
    }
    if (strcmp(argv[i], "--driver") == 0) {
      if (++i >= argc || parse_driver(argv[i], &args->driver)) return -1;
      continue;
    }
    if (strcmp(argv[i], "--host") == 0) {
      if (++i >= argc) return -1;
      args->host = argv[i];
      continue;
    }
    if (strcmp(argv[i], "--port") == 0) {
      if (++i >= argc || parse_u16(argv[i], &args->port)) return -1;
      continue;
    }
    if (strcmp(argv[i], "--user") == 0) {
      if (++i >= argc) return -1;
      args->user = argv[i];
      continue;
    }
    if (strcmp(argv[i], "--pass") == 0) {
      if (++i >= argc) return -1;
      args->pass = argv[i];
      continue;
    }
    if (strcmp(argv[i], "--pre-tzset") == 0) {
      args->pre_tzset = 1;
      continue;
    }
    if (strcmp(argv[i], "--post-tzset") == 0) {
      args->post_tzset = 1;
      continue;
    }
    DUMP("unknown argument: %s", argv[i]);
    return -1;
  }

  return 0;
}

int main(int argc, char *argv[])
{
  args_t args = {0};
  int ar = parse_args(argc, argv, &args);
  if (ar > 0) return 0;
  if (ar < 0) {
    usage(argv[0]);
    return 1;
  }

  DUMP("driver=%s", args.driver);
  snapshot("startup");

  if (args.pre_tzset) {
    tzset();
    snapshot("after pre _tzset()");
  }

  int rc = taos_options(TSDB_OPTION_DRIVER, args.driver);
  DUMP("taos_options(TSDB_OPTION_DRIVER, %s) => %d", args.driver, rc);
  if (rc) {
    snapshot("after taos_options failure");
    return 2;
  }
  snapshot("after taos_options");

  TAOS *taos = taos_connect(args.host, args.user, args.pass, NULL, args.port);
  DUMP("taos_connect(host:%s,port:%u,user:%s) => %p", args.host, (unsigned)args.port, args.user ? args.user : "(null)", taos);
  if (!taos) {
    DUMP("taos_connect failed: errno=%d, errstr=%s", taos_errno(NULL), taos_errstr(NULL));
    snapshot("after taos connect failure");
    return 2;
  }

  snapshot("after taos connect");

  if (args.post_tzset) {
    tzset();
    snapshot("after post _tzset()");
  }

  int r = 0;
  int64_t raw = 0;
  int precision = 0;
  char local[128] = {0};
  if (exec_sql(taos, "drop database if exists tzset_repro")) r = -1;
  if (!r && exec_sql(taos, "create database tzset_repro")) r = -1;
  if (!r && exec_sql(taos, "create table tzset_repro.t0 (ts timestamp, v int)")) r = -1;
  if (!r && exec_sql(taos, "insert into tzset_repro.t0 values ('2024-08-25 10:20:45.678', 1)")) r = -1;
  if (!r && fetch_one_ts_raw(taos, "select ts from tzset_repro.t0", &raw, &precision)) r = -1;

  if (!r) {
    if (format_local_timestamp(raw, precision, local, sizeof(local))) {
      DUMP("format_local_timestamp failed: raw=%" PRId64 ", precision=%d", raw, precision);
      r = -1;
    } else {
      DUMP("query timestamp raw=%" PRId64 ", precision=%d, localtime=%s", raw, precision, local);
      DUMP("expected localtime: 2024-08-25 10:20:45.678");
    }
  }

  taos_close(taos);
  taos_cleanup();
  return r ? 3 : 0;
}
