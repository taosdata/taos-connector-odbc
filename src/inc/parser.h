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

#ifndef _parser_h_
#define _parser_h_

// https://docs.microsoft.com/en-us/sql/odbc/reference/syntax/sqldriverconnect-function?view=sql-server-ver16

#include "macros.h"
#include "typedefs.h"

#include <stddef.h>

EXTERN_C_BEGIN

void parser_param_release(parser_param_t *param) FA_HIDDEN;

int parser_parse(const char *input, size_t len,
    parser_param_t *param) FA_HIDDEN;

EXTERN_C_END

#endif // _parser_h_

