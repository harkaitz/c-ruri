#ifndef RURI_H
#define RURI_H

#include <stdbool.h>
#include <uuid/uuid.h>

typedef struct redisContext redisContext;
typedef struct redisReply   redisReply;

#define QUERY_VAR_REQUEST_ID  "state"
#define RURI_KEY_PREFIX       "ruri:key:"
#define RURI_SUB_PREFIX       "ruri:sub:"
#define REDIS_FIELD_REDIRECT  "ruri_redirect"
#define REDIS_FIELD_SIGNALLED "ruri_signalled"

bool
ruri_create(redisContext *_redis,
            char        **_out_redirect_uri_m,
            char const    _ruri_cgi_url[],
            char const    _key[],
            char const    _opt_next_url[]);

bool
ruri_query(redisContext *_redis,
           char const    _key[],
           long          _wait_ms /* 0: Do not wait, -1: Forever. */, 
           redisReply  **_reply);

#endif
/**l*
 * 
 * MIT License
 * 
 * Bug reports, feature requests to gemini|https://harkadev.com/oss
 * Copyright (c) 2022 Harkaitz Agirre, harkaitz.aguirre@gmail.com
 * 
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 **l*/
