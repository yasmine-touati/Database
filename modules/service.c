#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../lib/service.h"
#include "../lib/bpt.h"
#include "../lib/dfh.h"

// Helper function to get HTTP status message
static const char* get_status_message(int status_code) {
    switch (status_code) {
        case HTTP_OK: return "OK";
        case HTTP_CREATED: return "Created";
        case HTTP_BAD_REQUEST: return "Bad Request";
        case HTTP_NOT_FOUND: return "Not Found";
        case HTTP_PAYLOAD_TOO_LARGE: return "Payload Too Large";
        case HTTP_INTERNAL_SERVER_ERROR: return "Internal Server Error";
        default: return "Unknown";
    }
}

// Parse HTTP request
http_request_t* parse_request(const char* raw_request) {
    if (!raw_request) return NULL;
    
    http_request_t* request = malloc(sizeof(http_request_t));
    if (!request) return NULL;
    
    memset(request, 0, sizeof(http_request_t));
    
    // Parse method
    if (strncmp(raw_request, "GET", 3) == 0) {
        request->method = HTTP_METHOD_GET;
    } else if (strncmp(raw_request, "POST", 4) == 0) {
        request->method = HTTP_METHOD_POST;
    } else if (strncmp(raw_request, "DELETE", 6) == 0) {
        request->method = HTTP_METHOD_DELETE;
    } else if (strncmp(raw_request, "PUT", 3) == 0) {
        request->method = HTTP_METHOD_PUT;
    } else {
        request->method = HTTP_METHOD_UNKNOWN;
    }
    
    // Parse path
    const char* path_start = strchr(raw_request, ' ') + 1;
    const char* path_end = strchr(path_start, ' ');
    size_t path_len = path_end - path_start;
    if (path_len >= sizeof(request->path)) {
        free(request);
        return NULL;
    }
    strncpy(request->path, path_start, path_len);
    request->path[path_len] = '\0';
    
    // Find content type
    const char* content_type = strstr(raw_request, "Content-Type: ");
    if (content_type) {
        content_type += 14;
        const char* content_type_end = strchr(content_type, '\r');
        if (content_type_end) {
            size_t len = content_type_end - content_type;
            if (len < sizeof(request->content_type)) {
                strncpy(request->content_type, content_type, len);
                request->content_type[len] = '\0';
            }
        }
    }
    
    // Find content length and body
    const char* content_length = strstr(raw_request, "Content-Length: ");
    if (content_length) {
        request->content_length = atoi(content_length + 16);
        const char* body = strstr(raw_request, "\r\n\r\n");
        if (body && request->content_length > 0) {
            body += 4;
            request->body = malloc(request->content_length + 1);
            if (request->body) {
                memcpy(request->body, body, request->content_length);
                request->body[request->content_length] = '\0';
            }
        }
    }
    
    return request;
}

void free_request(http_request_t* request) {
    if (request) {
        free(request->body);
        free(request);
    }
}

// Create HTTP response
http_response_t* create_response(int status_code, const char* body, const char* content_type) {
    http_response_t* response = malloc(sizeof(http_response_t));
    if (!response) return NULL;
    
    response->status_code = status_code;
    response->content_length = body ? strlen(body) : 0;
    
    if (body) {
        response->body = strdup(body);
        if (!response->body) {
            free(response);
            return NULL;
        }
    } else {
        response->body = NULL;
    }
    
    strncpy(response->content_type, content_type, sizeof(response->content_type) - 1);
    response->content_type[sizeof(response->content_type) - 1] = '\0';
    
    return response;
}

void free_response(http_response_t* response) {
    if (response) {
        free(response->body);
        free(response);
    }
}

// Create error response
http_response_t* create_error_response(int status_code, const char* message, const char* details) {
    cJSON* error = cJSON_CreateObject();
    if (!error) return NULL;
    
    cJSON_AddNumberToObject(error, "code", status_code);
    cJSON_AddStringToObject(error, "message", message);
    if (details) {
        cJSON_AddStringToObject(error, "details", details);
    }
    
    char* error_str = cJSON_Print(error);
    cJSON_Delete(error);
    
    if (!error_str) return NULL;
    
    http_response_t* response = create_response(status_code, error_str, CONTENT_TYPE_JSON);
    free(error_str);
    
    return response;
}

// Create JSON response
http_response_t* create_json_response(int status_code, cJSON* json_body) {
    if (!json_body) return NULL;
    
    char* json_str = cJSON_Print(json_body);
    if (!json_str) return NULL;
    
    http_response_t* response = create_response(status_code, json_str, CONTENT_TYPE_JSON);
    free(json_str);
    
    return response;
}

// Create success response
http_response_t* create_success_response(int status_code, const char* message) {
    cJSON* success = cJSON_CreateObject();
    if (!success) return NULL;
    
    cJSON_AddBoolToObject(success, "success", true);
    if (message) {
        cJSON_AddStringToObject(success, "message", message);
    }
    
    char* success_str = cJSON_Print(success);
    cJSON_Delete(success);
    
    if (!success_str) return NULL;
    
    http_response_t* response = create_response(status_code, success_str, CONTENT_TYPE_JSON);
    free(success_str);
    
    return response;
}

// Request validation functions
validation_result_t validate_insert_request(const http_request_t* request, char* error_msg) {
    if (!request->body) {
        strcpy(error_msg, "Missing request body");
        return VALIDATION_MISSING_REQUIRED_FIELDS;
    }
    
    cJSON* json = cJSON_Parse(request->body);
    if (!json) {
        strcpy(error_msg, ERROR_INVALID_JSON);
        return VALIDATION_INVALID_JSON;
    }
    
    cJSON* entries = cJSON_GetObjectItem(json, "entries");
    if (!entries || !cJSON_IsArray(entries)) {
        cJSON_Delete(json);
        strcpy(error_msg, "Missing or invalid 'entries' array");
        return VALIDATION_MISSING_REQUIRED_FIELDS;
    }
    
    int entry_count = cJSON_GetArraySize(entries);
    if (entry_count > MAX_ENTRIES_PER_REQUEST) {
        cJSON_Delete(json);
        strcpy(error_msg, ERROR_TOO_MANY_ENTRIES);
        return VALIDATION_TOO_MANY_ENTRIES;
    }
    
    for (int i = 0; i < entry_count; i++) {
        cJSON* entry = cJSON_GetArrayItem(entries, i);
        if (!entry || !cJSON_IsObject(entry)) {
            cJSON_Delete(json);
            sprintf(error_msg, "Invalid entry at index %d", i);
            return VALIDATION_INVALID_DATA_TYPE;
        }
        
        char* entry_str = cJSON_Print(entry);
        if (strlen(entry_str) > MAX_ENTRY_SIZE) {
            free(entry_str);
            cJSON_Delete(json);
            sprintf(error_msg, "Entry at index %d exceeds size limit", i);
            return VALIDATION_ENTRY_TOO_LARGE;
        }
        free(entry_str);
    }
    
    cJSON_Delete(json);
    return VALIDATION_OK;
}

validation_result_t validate_delete_request(const http_request_t* request, char* error_msg) {
    if (!request->body) {
        strcpy(error_msg, "Missing request body");
        return VALIDATION_MISSING_REQUIRED_FIELDS;
    }
    
    cJSON* json = cJSON_Parse(request->body);
    if (!json) {
        strcpy(error_msg, ERROR_INVALID_JSON);
        return VALIDATION_INVALID_JSON;
    }
    
    cJSON* keys = cJSON_GetObjectItem(json, "keys");
    if (!keys || !cJSON_IsArray(keys)) {
        cJSON_Delete(json);
        strcpy(error_msg, "Missing or invalid 'keys' array");
        return VALIDATION_MISSING_REQUIRED_FIELDS;
    }
    
    int key_count = cJSON_GetArraySize(keys);
    if (key_count > MAX_ENTRIES_PER_REQUEST) {
        cJSON_Delete(json);
        strcpy(error_msg, ERROR_TOO_MANY_ENTRIES);
        return VALIDATION_TOO_MANY_ENTRIES;
    }
    
    cJSON_Delete(json);
    return VALIDATION_OK;
}

validation_result_t validate_search_request(const http_request_t* request, char* error_msg) {
    if (!request->body) {
        strcpy(error_msg, "Missing request body");
        return VALIDATION_MISSING_REQUIRED_FIELDS;
    }
    
    cJSON* json = cJSON_Parse(request->body);
    if (!json) {
        strcpy(error_msg, ERROR_INVALID_JSON);
        return VALIDATION_INVALID_JSON;
    }
    
    cJSON* key = cJSON_GetObjectItem(json, "key");
    if (!key || !cJSON_IsNumber(key)) {
        cJSON_Delete(json);
        strcpy(error_msg, "Missing or invalid 'key' field");
        return VALIDATION_MISSING_REQUIRED_FIELDS;
    }
    
    cJSON_Delete(json);
    return VALIDATION_OK;
}

// Response formatting functions
char* format_response_headers(const http_response_t* response) {
    if (!response) return NULL;
    
    size_t max_header_size = 256;
    char* headers = malloc(max_header_size);
    if (!headers) return NULL;
    
    snprintf(headers, max_header_size, HTTP_HEADER_FORMAT,
             response->status_code,
             get_status_message(response->status_code),
             response->content_type,
             response->content_length);
    
    return headers;
}

char* format_full_response(const http_response_t* response) {
    if (!response) return NULL;
    
    char* headers = format_response_headers(response);
    if (!headers) return NULL;
    
    size_t total_size = strlen(headers) + response->content_length + 1;
    char* full_response = malloc(total_size);
    if (!full_response) {
        free(headers);
        return NULL;
    }
    
    strcpy(full_response, headers);
    if (response->body) {
        strcat(full_response, response->body);
    }
    
    free(headers);
    return full_response;
}
