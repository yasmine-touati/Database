#ifndef SERVICE_H
#define SERVICE_H

#include <stdio.h>
#include <string.h>
#include "bpt.h"

#define MAX_PATH_PARAMS 10
#define MAX_PARAM_LENGTH 32
#define MAX_QUERY_PARAMS 10

typedef struct {
    char key[MAX_PARAM_LENGTH];
    char value[MAX_PARAM_LENGTH];
} Param;

typedef struct {
    char method[10];
    char path[256];
    char body[8192];
    Param path_params[MAX_PATH_PARAMS];
    int path_param_count;
    Param query_params[MAX_QUERY_PARAMS];
    int query_param_count;
} Request;

typedef struct {
    int status_code;
    char content_type[32];
    char body[1024];
} Response;

void parse_request(const char* raw_request, Request* req);
void parse_path_params(Request* req);
void parse_query_params(Request* req);
Param* get_path_param(Request* req, const char* key);
Param* get_query_param(Request* req, const char* key);

#endif