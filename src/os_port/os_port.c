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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <limits.h>
#include <libgen.h>
#endif

#include "os_port.h"


void tod_get_exe_dir(char *buffer, size_t buffer_size) {
#ifdef _WIN32
  DWORD len = GetModuleFileNameA(NULL, buffer, (DWORD)buffer_size);
  if (len == 0 || len >= buffer_size) {
    snprintf(buffer, buffer_size, ".\\");
    return;
  }

  char drive[_MAX_DRIVE];
  char dir[_MAX_DIR];
  char fname[_MAX_FNAME];
  char ext[_MAX_EXT];

  _splitpath_s(buffer, drive, _MAX_DRIVE, dir, _MAX_DIR, fname, _MAX_FNAME, ext, _MAX_EXT);
  _makepath_s(buffer, buffer_size, drive, dir, "", "");

#else
  ssize_t count = readlink("/proc/self/exe", buffer, buffer_size - 1);
  if (count != -1) {
    buffer[count] = '\0';
    char *dir = dirname(buffer);
    snprintf(buffer, buffer_size, "%s/", dir);
  } else {
    snprintf(buffer, buffer_size, "./");
  }
#endif
}

void tod_get_file_dir(const char *file_path, char *buffer, size_t buffer_size) {
#ifdef _WIN32
  char drive[_MAX_DRIVE];
  char dir[_MAX_DIR];
  char fname[_MAX_FNAME];
  char ext[_MAX_EXT];

  _splitpath_s(file_path, drive, _MAX_DRIVE, dir, _MAX_DIR, fname, _MAX_FNAME, ext, _MAX_EXT);
  _makepath_s(buffer, buffer_size, drive, dir, "", "");
#else
  char path[PATH_MAX];
  strncpy(path, file_path, sizeof(path) - 1);
  path[sizeof(path) - 1] = '\0';

  char *dir = dirname(path);
  snprintf(buffer, buffer_size, "%s/", dir);
#endif
}