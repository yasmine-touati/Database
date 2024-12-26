#ifndef SERVICE_H
#define SERVICE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <unistd.h>
#endif

#include "bpt.h"
#include <cJSON.h>

// Server configuration
#define PORT 6667
#define BACKLOG 10
#define BUFFER_SIZE 65536  // 64KB for request buffer

// Data limits
#define MAX_ENTRIES_PER_REQUEST 50
#define MAX_ENTRY_SIZE 1024
#define MAX_BODY_SIZE (MAX_ENTRIES_PER_REQUEST * MAX_ENTRY_SIZE)

// HTTP Status codes
#define HTTP_OK 200
#define HTTP_CREATED 201
#define HTTP_BAD_REQUEST 400
#define HTTP_NOT_FOUND 404
#define HTTP_PAYLOAD_TOO_LARGE 413
#define HTTP_INTERNAL_SERVER_ERROR 500

// HTTP Methods
typedef enum {
    HTTP_METHOD_UNKNOWN,
    HTTP_METHOD_GET,
    HTTP_METHOD_POST,
    HTTP_METHOD_DELETE,
    HTTP_METHOD_PUT
} http_method_t;

// Request structure
typedef struct {
    http_method_t method;
    char path[256];
    char* body;
    size_t content_length;
    char content_type[64];
} http_request_t;

// Response structure
typedef struct {
    int status_code;
    char* body;
    size_t content_length;
    char content_type[64];
} http_response_t;

// Error response structure
typedef struct {
    int code;
    char message[256];
    char details[1024];
} error_response_t;

// Function declarations
http_request_t* parse_request(const char* raw_request);
void free_request(http_request_t* request);

http_response_t* create_response(int status_code, const char* body, const char* content_type);
void free_response(http_response_t* response);

// Response creation helpers
http_response_t* create_error_response(int status_code, const char* message, const char* details);
http_response_t* create_json_response(int status_code, cJSON* json_body);
http_response_t* create_success_response(int status_code, const char* message);

// Request validation
typedef enum {
    VALIDATION_OK,
    VALIDATION_INVALID_JSON,
    VALIDATION_TOO_MANY_ENTRIES,
    VALIDATION_ENTRY_TOO_LARGE,
    VALIDATION_MISSING_REQUIRED_FIELDS,
    VALIDATION_INVALID_DATA_TYPE
} validation_result_t;

validation_result_t validate_insert_request(const http_request_t* request, char* error_msg);
validation_result_t validate_delete_request(const http_request_t* request, char* error_msg);
validation_result_t validate_search_request(const http_request_t* request, char* error_msg);

// Response formatting
char* format_response_headers(const http_response_t* response);
char* format_full_response(const http_response_t* response);

// Content type constants
#define CONTENT_TYPE_JSON "application/json"
#define CONTENT_TYPE_TEXT "text/plain"

// Header formatting
#define HTTP_HEADER_FORMAT "HTTP/1.1 %d %s\r\n" \
                         "Content-Type: %s\r\n" \
                         "Content-Length: %zu\r\n" \
                         "Connection: close\r\n" \
                         "\r\n"

// Error messages
#define ERROR_INVALID_JSON "Invalid JSON format"
#define ERROR_TOO_MANY_ENTRIES "Too many entries in request (maximum 50)"
#define ERROR_ENTRY_TOO_LARGE "Entry size exceeds maximum allowed (1024 bytes)"
#define ERROR_MISSING_FIELDS "Missing required fields"
#define ERROR_INVALID_TYPE "Invalid data type for field"
#define ERROR_INTERNAL "Internal server error"

#endif // SERVICE_H