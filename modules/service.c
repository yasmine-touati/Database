#include "../lib/service.h"
#include <stdlib.h>
#include <string.h>

void parse_request(const char* raw_request, Request* req) {
    if (!raw_request || !req) return;
    
    printf("\n=== Debug: Request Parsing ===\n");
    printf("Raw request length: %zu\n", strlen(raw_request));
    
    // Initialize request
    memset(req, 0, sizeof(Request));
    
    // Parse first line for method and path
    char* request_line = strdup(raw_request);
    char* first_newline = strchr(request_line, '\n');
    if (first_newline) *first_newline = '\0';
    
    char* method = strtok(request_line, " ");
    char* path = strtok(NULL, " ");
    
    if (method && path) {
        strncpy(req->method, method, sizeof(req->method) - 1);
        strncpy(req->path, path, sizeof(req->path) - 1);
        printf("Parsed method: %s\n", req->method);
        printf("Parsed path: %s\n", req->path);
    }
    
    free(request_line);
    
    // Find and parse body
    const char* body_start = strstr(raw_request, "\r\n\r\n");
    if (body_start) {
        body_start += 4;
        size_t body_length = strlen(body_start);
        printf("Body length before copy: %zu\n", body_length);
        if (body_length >= sizeof(req->body)) {
            printf("Warning: Body truncated from %zu to %zu bytes\n", 
                   body_length, sizeof(req->body) - 1);
        }
        strncpy(req->body, body_start, sizeof(req->body) - 1);
        printf("Body length after copy: %zu\n", strlen(req->body));
    }
}

void parse_path_params(Request* req) {
    if (!req || !req->path[0]) return;
    
    char* path = strdup(req->path);
    char* token = strtok(path, "/");
    req->path_param_count = 0;
    
    // Skip initial empty token if path starts with '/'
    if (token && strcmp(token, "") == 0) {
        token = strtok(NULL, "/");
    }
    
    while (token && req->path_param_count < MAX_PATH_PARAMS - 1) {
        // For dataset parameter
        if (strcmp(token, "dataset") == 0) {
            char* value = strtok(NULL, "/");
            if (value) {
                strncpy(req->path_params[req->path_param_count].key, "dataset", MAX_PARAM_LENGTH - 1);
                strncpy(req->path_params[req->path_param_count].value, value, MAX_PARAM_LENGTH - 1);
                req->path_param_count++;
            }
        }
        // For order parameter
        else if (strcmp(token, "order") == 0) {
            char* value = strtok(NULL, "/");
            if (value) {
                strncpy(req->path_params[req->path_param_count].key, "order", MAX_PARAM_LENGTH - 1);
                strncpy(req->path_params[req->path_param_count].value, value, MAX_PARAM_LENGTH - 1);
                req->path_param_count++;
            }
        }
        // For key parameter
        else if (strcmp(token, "key") == 0) {
            char* value = strtok(NULL, "/");
            if (value) {
                strncpy(req->path_params[req->path_param_count].key, "key", MAX_PARAM_LENGTH - 1);
                strncpy(req->path_params[req->path_param_count].value, value, MAX_PARAM_LENGTH - 1);
                req->path_param_count++;
            }
        }
        // For start parameter
        else if (strcmp(token, "start") == 0) {
            char* value = strtok(NULL, "/");
            if (value) {
                strncpy(req->path_params[req->path_param_count].key, "start", MAX_PARAM_LENGTH - 1);
                strncpy(req->path_params[req->path_param_count].value, value, MAX_PARAM_LENGTH - 1);
                req->path_param_count++;
            }
        }
        // For end parameter
        else if (strcmp(token, "end") == 0) {
            char* value = strtok(NULL, "/");
            if (value) {
                strncpy(req->path_params[req->path_param_count].key, "end", MAX_PARAM_LENGTH - 1);
                strncpy(req->path_params[req->path_param_count].value, value, MAX_PARAM_LENGTH - 1);
                req->path_param_count++;
            }
        }
        
        token = strtok(NULL, "/");
    }
    
    free(path);
}

void parse_query_params(Request* req) {
    if (!req || !req->path[0]) return;
    
    char* query = strchr(req->path, '?');
    if (!query) return;
    
    *query++ = '\0';  // Split path and query
    req->query_param_count = 0;
    
    char* pair = strtok(query, "&");
    while (pair && req->query_param_count < MAX_QUERY_PARAMS) {
        char* value = strchr(pair, '=');
        if (value) {
            *value++ = '\0';
            strncpy(req->query_params[req->query_param_count].key, pair, MAX_PARAM_LENGTH - 1);
            strncpy(req->query_params[req->query_param_count].value, value, MAX_PARAM_LENGTH - 1);
            req->query_param_count++;
        }
        pair = strtok(NULL, "&");
    }
}

Param* get_path_param(Request* req, const char* key) {
    if (!req || !key) return NULL;
    
    for (int i = 0; i < req->path_param_count; i++) {
        if (strcmp(req->path_params[i].key, key) == 0) {
            return &req->path_params[i];
        }
    }
    return NULL;
}

Param* get_query_param(Request* req, const char* key) {
    if (!req || !key) return NULL;
    
    for (int i = 0; i < req->query_param_count; i++) {
        if (strcmp(req->query_params[i].key, key) == 0) {
            return &req->query_params[i];
        }
    }
    return NULL;
}
