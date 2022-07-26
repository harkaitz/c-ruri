#include "ruri.h"
#include <hiredis/hiredis.h>
#include <string.h>
#include <stdlib.h>
#include <syslog.h>


bool
ruri_create(redisContext *_redis,
            char        **_out_redirect_uri_m,
            char const    _ruri_cgi_url[],
            char const    _key[],
            char const    _opt_next_url[]) {

    bool           retval  = false;
    size_t         keysz   = strlen(RURI_KEY_PREFIX)+strlen(_key)+1;
    size_t         urlsz   = strlen(_ruri_cgi_url)+1+strlen(QUERY_VAR_REQUEST_ID)+1+strlen(_key)+1;
    char          *key_m   = malloc(keysz);
    char          *url_m   = malloc(urlsz);
    redisReply    *reply   = NULL;
    char const    *cmd[20] = {0};
    size_t         cmdsz   = 0;
    
    if (!key_m || !url_m/*err*/) goto error_malloc;
    
    sprintf(key_m, "%s%s", RURI_KEY_PREFIX, _key);
    sprintf(url_m, "%s?%s=%s", _ruri_cgi_url, QUERY_VAR_REQUEST_ID, _key);

    cmd[cmdsz++] = "HMSET";
    cmd[cmdsz++] = key_m;
    cmd[cmdsz++] = REDIS_FIELD_SIGNALLED;
    cmd[cmdsz++] = "0";
    if (_opt_next_url) {
        cmd[cmdsz++] = REDIS_FIELD_REDIRECT;
        cmd[cmdsz++] = _opt_next_url;
    }
    
    reply = redisCommandArgv(_redis, cmdsz, cmd, NULL);
    if (!reply/*err*/) goto error_redis;
    if (reply->type == REDIS_REPLY_ERROR/*err*/) goto error_reply;
    
    *_out_redirect_uri_m = url_m;
    url_m = NULL;
    retval = true;
 cleanup:
    if (reply) freeReplyObject(reply);
    if (key_m) free(key_m);
    if (url_m) free(url_m);
    return retval;
 error_malloc:
    syslog(LOG_ERR, "Can't allocate memory.");
    goto cleanup;
 error_redis:
    syslog(LOG_ERR, "redis: %s", _redis->errstr);
    goto cleanup;
 error_reply:
    syslog(LOG_ERR, "redis: hget: %s", reply->str);
    goto cleanup;
}

bool
ruri_query(redisContext *_redis,
           char const    _key[],
           long          _wait_ms /* 0: Do not wait, -1: Forever. */, 
           redisReply  **_reply) {

    bool           retval  = false;
    size_t         keysz   = strlen(RURI_KEY_PREFIX)+strlen(_key)+1;
    size_t         subsz   = strlen(RURI_SUB_PREFIX)+strlen(_key)+1;
    char          *key_m   = malloc(keysz);
    char          *sub_m   = malloc(subsz);
    redisReply    *reply   = NULL;

    if (!key_m/*err*/) goto error_malloc;

    sprintf(key_m, "%s%s", RURI_KEY_PREFIX, _key);
    sprintf(sub_m, "%s%s", RURI_SUB_PREFIX, _key);
    
    reply = redisCommand(_redis, "HGET %s %s", key_m, REDIS_FIELD_SIGNALLED);
    if (!reply/*err*/) goto error_redis;
    if (reply->type == REDIS_REPLY_ERROR/*err*/) goto error_reply;
    if (!strcmp(reply->str, "0") && _wait_ms == -1) {
        freeReplyObject(reply);
        reply = NULL;
        freeReplyObject(redisCommand(_redis, "SUBSCRIBE %s", sub_m));
        redisGetReply(_redis,(void *)&reply);
        freeReplyObject(reply);
        redisCommand(_redis, "UNSUBSCRIBE");
        reply = NULL;
        
    } else {
        freeReplyObject(reply);
        reply = NULL;
    }
    
    reply = redisCommand(_redis, "HGETALL %s", key_m);
    if (!reply/*err*/) goto error_redis;
    if (reply->type == REDIS_REPLY_ERROR/*err*/) goto error_reply;

    *_reply = reply;
    reply = NULL;
    retval = true;
 cleanup:
    if (reply) freeReplyObject(reply);
    if (key_m) free(key_m);
    if (sub_m) free(sub_m);
    return retval;
 error_malloc:
    syslog(LOG_ERR, "Can't allocate memory.");
    goto cleanup;
 error_redis:
    syslog(LOG_ERR, "redis: %s", _redis->errstr);
    goto cleanup;
 error_reply:
    syslog(LOG_ERR, "redis: hget: %s", reply->str);
    goto cleanup;
}
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
