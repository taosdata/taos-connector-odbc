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

#define _XOPEN_SOURCE

#include "helpers.h"

#include <time.h>

#ifdef _WIN32
// NOTE: not exported in taos.dll
const char *taos_data_type(int type)
{
  (void)type;
  return "UNKNOWN";
}
#endif
#ifdef _WIN32
char* tod_strptime(const char *s, const char *format, struct tm *tm)
{
  (void)s;
  (void)format;
  (void)tm;
  return NULL;
}
#else
char* tod_strptime(const char *s, const char *format, struct tm *tm)
{
  return strptime(s, format, tm);
}
#endif

#ifdef _WIN32
char* tod_basename(const char *path, char *buf, size_t sz)
{
  char *file = NULL;
  DWORD dw = GetFullPathName(path, (DWORD)sz, buf, &file);
  if (dw == 0) {
    errno = GetLastError();
    return NULL;
  }
  if (dw >= sz) {
    errno = E2BIG;
    return NULL;
  }

  return file;
}
#else
char* tod_basename(const char *path, char *buf, size_t sz)
{
  (void)buf;
  (void)sz;
  return basename((char*)path);
}
#endif

#ifdef _WIN32
char* tod_dirname(const char *path, char *buf, size_t sz)
{
  (void)path;
  return NULL;
}
#elif defined(__APPLIE__)
char* tod_dirname(const char *path, char *buf, size_t sz)
{
  if (strlen(path) >= sz) {
    errno = E2BIG;
    return NULL;
  }

  return dirname_r(path, buf)
}
#else
char* tod_dirname(const char *path, char *buf, size_t sz)
{
  int n = snprintf(buf, sz, "%s", path);
  if (n >= sz) {
    errno = E2BIG;
    return NULL;
  }

  return dirname(buf);
}
#endif
