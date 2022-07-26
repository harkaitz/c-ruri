#include "ruri.h"
#include <libgen.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <syslog.h>
#include <hiredis/hiredis.h>

#define QUERY_MAX            2048
#define REDIS_HOST           "127.0.0.1"
#define REDIS_PORT           6379
#define HMSET_MAX_ARGS       20

int main (int _argc __attribute__((unused)), char *_argv[]) {

    int            retval                = 1;
    redisContext  *redis                 = NULL;
    redisReply    *reply1                = NULL;
    redisReply    *reply2                = NULL;
    char const    *query_string          = getenv("QUERY_STRING");
    size_t         query_string_l        = 0;
    char          *query_string_m        = NULL;
    char          *program_name          = basename(_argv[0]);
    char          *key_m                 = NULL;
    char          *sub_m                 = NULL;
    char const    *hset[HMSET_MAX_ARGS]  = {"HMSET", NULL, REDIS_FIELD_SIGNALLED, "1"};
    size_t         hsetsz                = 4;
    char const    *hget[]                = {"HGET", NULL, REDIS_FIELD_REDIRECT, NULL};
    size_t         hgetsz                = 3;
    char const    *publish[]             = {"PUBLISH", NULL, "1"};
    size_t         publishsz             = 3;
    int            e;
    
    /* Initialize logging. */
    openlog(program_name, (getenv("DEBUG"))?LOG_PERROR:0, LOG_USER);
    
    /* Get CGI variables. */
    if (!query_string) {
        syslog(LOG_ERR, "CGI: QUERY_STRING: Environment variable not set.");
        goto cleanup;
    } else if ((query_string_l = strlen(query_string)) >= QUERY_MAX) {
        syslog(LOG_ERR, "CGI: The query string is too long (QUERY_STRING): %li", query_string_l);
        goto cleanup;
    }

    /* Get service, request and parameters. */
    e = true; {
        query_string_m = malloc(query_string_l+1);
        if (!query_string_m/*err*/) goto error_malloc;
        memcpy(query_string_m, query_string, query_string_l+1);
        for (char *var,*val,*nxt = query_string_m; nxt;) {
            if (*nxt == '?') nxt++;
            var = nxt;
            val = strchr(nxt, '=');
            if (val) {
                *(val++) = '\0';
                nxt = strchr(val, '&');
            } else {
                val = "";
                nxt = strchr(var, '&');
            }
            if (nxt) *(nxt++) = '\0';
            
            if (!strcmp(var, QUERY_VAR_REQUEST_ID)) {
                size_t l = strlen(val);
                key_m = malloc(strlen(RURI_KEY_PREFIX)+1+l);
                sub_m = malloc(strlen(RURI_SUB_PREFIX)+1+l);
                if (!key_m || !sub_m/*err*/) goto error_malloc;
                sprintf(key_m, "%s%s", RURI_KEY_PREFIX, val);
                sprintf(sub_m, "%s%s", RURI_SUB_PREFIX, val);
                hset[1] = key_m;
                hget[1] = key_m;
                publish[1] = sub_m;
            } else if (!strcmp(var, REDIS_FIELD_REDIRECT)) {
                
            } else if (hsetsz < (20-2)) {
                hset[hsetsz++] = var;
                hset[hsetsz++] = val; /* Todo URL decode. */
            }
        }
        if (!hset[1]) {
            syslog(LOG_ERR, "CGI: QUERY_STRING: Key '%s' not found.", QUERY_VAR_REQUEST_ID);
            e = false;
        }
    }
    if (!e/*err*/) goto cleanup;
    
    /* Connect to Redis. */
    redis = redisConnect(REDIS_HOST, REDIS_PORT);
    if (!redis/*err*/) goto error_malloc;
    e = redis->errstr && redis->err;
    if (e/*err*/) goto error_redis;

    /* Get redirection and close CGI. */ {
        reply1 = redisCommandArgv(redis, hgetsz, hget, NULL);
        if (!reply1/*err*/) goto error_redis;
        switch (reply1->type) {
        case REDIS_REPLY_ERROR: goto error_reply1;
        case REDIS_REPLY_NIL:
            fprintf(stdout,
                    "Content-Type: text/html" "\r\n"
                    ""                        "\r\n"
                    "No redirection."         "\r\n");
            fclose(stdout);
            break;
        case REDIS_REPLY_STRING:
            fprintf(stdout,
                    "Location: %s" "\r\n"
                    ""             "\r\n", reply1->str);
            fclose(stdout);
            break;
        }
    }

    /* Publish. */
    freeReplyObject(redisCommandArgv(redis, publishsz, publish, NULL));
    
    /* Set variable. */ if (hsetsz > 2) {
        reply2 = redisCommandArgv(redis, hsetsz, hset, NULL);
        if (!reply2/*err*/) goto error_redis;
        if (reply2->type == REDIS_REPLY_ERROR/*err*/) goto error_reply2;
    }
    
    /* Successful return. */
    retval = 0;
 cleanup:
    if (redis) redisFree(redis);
    if (reply1) freeReplyObject(reply1);
    if (reply2) freeReplyObject(reply2);
    if (query_string_m) free(query_string_m);
    if (key_m) free(key_m);
    if (sub_m) free(sub_m);
    return retval;
 error_malloc:
    syslog(LOG_ERR, "Can't allocate memory.");
    goto cleanup;
 error_redis:
    syslog(LOG_ERR, "redis: %s", redis->errstr);
    goto cleanup;
 error_reply1:
    syslog(LOG_ERR, "redis: hget: %s", reply1->str);
    goto cleanup;
 error_reply2:
    syslog(LOG_ERR, "redis: hset: %s", reply2->str);
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
