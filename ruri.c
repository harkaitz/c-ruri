#include "ruri.h"
#include <hiredis/hiredis.h>
#include <libgen.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <syslog.h>

#define COPYRIGHT_LINE \
    "Bug reports, feature requests to gemini|https://harkadev.com/oss" "\n" \
    "Copyright (c) 2022 Harkaitz Agirre, harkaitz.aguirre@gmail.com" "\n" \
    ""

#define RURI_URL_ENVVAR  "RURI_URL"
#define RURI_URL_EXAMPLE "https://your-domain.com/ruri.cgi"
#define REDIS_HOST       "127.0.0.1"
#define REDIS_PORT       6379

static __attribute__((unused)) /* Returns [-1/errno] on output error. */
int urlencode_f (FILE *_fp, const char _s[]) {
    const char *hex = "0123456789abcdef";
    size_t      sz  = strlen(_s);
    int         res = 0;
    for (size_t i=0; i < sz; i++) {
        if (('a' <= _s[i] && _s[i] <= 'z') ||
            ('A' <= _s[i] && _s[i] <= 'Z') ||
            ('0' <= _s[i] && _s[i] <= '9')) {
            res = fputc(_s[i], _fp);
            if (res==EOF/*err*/) return -1;
        } else {
            res = fputc('%', _fp);
            if (res==EOF/*err*/) return -1;
            res = fputc(hex[_s[i] >> 4], _fp);
            if (res==EOF/*err*/) return -1;
            res = fputc(hex[_s[i] & 15], _fp);
            if (res==EOF/*err*/) return -1;
        }
    }
    return 0;
}


int main (int _argc, char *_argv[]) {

    int            retval          = 1;
    redisContext  *redis           = NULL;
    redisReply    *reply1          = NULL;
    char const    *program_name    = basename(_argv[0]);
    char const    *ruri_cgi_url    = getenv(RURI_URL_ENVVAR);
    char const    *opt_key         = NULL;
    char const    *opt_next_url    = NULL;
    bool           opt_create      = false;
    long           opt_wait        = 0;
    char const    *opt_vars[]      = {0};
    size_t         opt_varsz       = 0;
    char          *redirect_uri_m  = NULL;
    int            opt,e;
    
    /* Print help. */
    if (_argc == 1 || !strcmp(_argv[1], "--help") || !strcmp(_argv[1], "-h")) {
        printf("Usage: %s -k KEY OPERATIONS..."                      "\n"
               ""                                                    "\n"
               "Create a unique \"redirect URI\" that points to"     "\n"
               "ruri.cgi(1) and wait/query results."                 "\n"
               ""                                                    "\n"
               "Environment variables:"                              "\n"
               ""                                                    "\n"
               "    %s=\"%s\""                                       "\n"
               ""                                                    "\n"
               "Operations:"                                         "\n"
               ""                                                    "\n"
               "    -c [-u URL] : Print a redirect URI."             "\n"
               "    -w          : Wait the redirect URI be reached." "\n"
               "    -g VAR      : Get variable received in query."   "\n"
               ""                                                    "\n"
               COPYRIGHT_LINE,
               program_name,
               RURI_URL_ENVVAR,
               (ruri_cgi_url)?ruri_cgi_url:RURI_URL_EXAMPLE);
        return 0;
    }

    /* Parse command line arguments. */
    while((opt = getopt (_argc, _argv, "k:cu:wg:")) != -1) {
        switch (opt) {
        case 'k': opt_key = optarg;      break;
        case 'c': opt_create = true;     break;
        case 'u': opt_next_url = optarg; break;
        case 'w': opt_wait = -1;         break;
        case 'g': if (opt_varsz<20) { opt_vars[opt_varsz++] = optarg; } break;
        case '?':
        default:
            return 1;
        }
    }

    /* Initialize logging. */
    openlog(program_name, LOG_PERROR, LOG_USER);
    
    /* Check arguments. */
    e = true; {
        if (!ruri_cgi_url && opt_create) {
            syslog(LOG_ERR, "%s: Environment variable not set.", RURI_URL_ENVVAR);
            e = false;
        }
        if ((!opt_create)&&(!opt_wait)&&(!opt_varsz)) {
            syslog(LOG_ERR, "Invalid arguments: At least specify -c/-w/-g");
            e = false;
        }
        if (!opt_key) {
            syslog(LOG_ERR, "Invalid arguments: Missing key, specify it with -k.");
            e = false;
        }
    }
    if (!e/*err*/) goto cleanup;
    
    /* Connect to redis.*/
    redis = redisConnect(REDIS_HOST, REDIS_PORT);
    if (!redis/*err*/) goto error_malloc;
    e = redis->errstr && redis->err;
    if (e/*err*/) goto error_redis;
    
    /* Create. */
    if (opt_create) {
        e = ruri_create(redis, &redirect_uri_m, ruri_cgi_url, opt_key, opt_next_url);
        if (!e/*err*/) goto cleanup;
        urlencode_f(stdout, redirect_uri_m);
        fputc('\n', stdout);
    }

    /* Wait. */
    if (opt_wait || opt_varsz) {
        e = ruri_query(redis, opt_key, opt_wait, &reply1);
        if (!e/*err*/) goto cleanup;
    }

    /* Print variables. */
    if (reply1) {
        for (size_t i=0; i < opt_varsz; i++) {
            for (size_t j=0; j < reply1->elements; j+=2) {
                if (!strcmp(opt_vars[i], reply1->element[j]->str)) {
                    fputs(reply1->element[j+1]->str, stdout);
                    fputc('\n', stdout);
                }
            }
        }
    }
    
    /* Success return. */
    retval = 0;
 cleanup:
    if (redis) redisFree(redis);
    if (reply1) freeReplyObject(reply1);
    if (redirect_uri_m) free(redirect_uri_m);
    return retval;
 error_malloc:
    syslog(LOG_ERR, "Can't allocate memory.");
    goto cleanup;
 error_redis:
    syslog(LOG_ERR, "redis: %s", redis->errstr);
    goto cleanup;
}
