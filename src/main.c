#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <windows.h>
#include <process.h>
#include <winsock2.h>
#include "../lib/application.h"
#include "../lib/bpt.h"
#include "../lib/persister.h"
#include "../lib/dfh.h"
#include "../lib/service.h"
#include <cJSON.h>

#pragma comment(lib, "ws2_32.lib")

#define MAX_DATASET_NUMBER 100
#define DATASETS_FILE "datasets.txt"
#define INACTIVE_TIMEOUT 1800
#define CHECK_INTERVAL 360
#define MAX_REQUEST_SIZE 61440  // 60KB in bytes
#define DEFAULT_PORT 6667
#define MAX_PENDING_CONNECTIONS 10

// Function declarations
void update_access_time(int index);
DWORD WINAPI cleanup_inactive_datasets(LPVOID arg);
void load_datasets(void);
BPT* find_BPT_by_name(const char* name);
DWORD WINAPI handle_client(LPVOID client_socket);
DWORD WINAPI run_server(LPVOID arg);
void add_dataset_to_file(const char* name);
void remove_dataset_from_file(const char* name);

typedef struct Dataset {
    char name[MAX_PATH_LENGTH];
    BPT* tree;
    time_t last_accessed;
} Dataset;

Dataset datasets[MAX_DATASET_NUMBER];
int dataset_count = 0;
CRITICAL_SECTION datasets_mutex;

void update_access_time(int index) {
    EnterCriticalSection(&datasets_mutex);
    datasets[index].last_accessed = time(NULL);
    LeaveCriticalSection(&datasets_mutex);
}

DWORD WINAPI cleanup_inactive_datasets(LPVOID arg) {
    while (1) {
        time_t current_time = time(NULL);
        EnterCriticalSection(&datasets_mutex);
        
        for (int i = 0; i < dataset_count; i++) {
            if (datasets[i].tree != NULL) {
                double diff = difftime(current_time, datasets[i].last_accessed);
                if (diff > INACTIVE_TIMEOUT) {
                    printf("Freeing inactive dataset: %s (inactive for %.1f minutes)\n", 
                           datasets[i].name, diff/60);
                    free_tree(datasets[i].tree);
                    datasets[i].tree = NULL;
                }
            }
        }
        
        LeaveCriticalSection(&datasets_mutex);
        Sleep(CHECK_INTERVAL * 1000);
    }
    return 0;
}

void load_datasets() {
    FILE* file = fopen(DATASETS_FILE, "r");
    
    if (!file) {
        // File doesn't exist, create it
        file = fopen(DATASETS_FILE, "w");
        if (!file) {
            printf("Error: Could not create datasets file\n");
            return;
        }
        fclose(file);
        return;
    }

    // Read existing datasets
    char line[MAX_PATH_LENGTH];
    while (fgets(line, sizeof(line), file)) {
        // Remove newline if present
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') {
            line[len-1] = '\0';
        }

        if (dataset_count >= MAX_DATASET_NUMBER) {
            printf("Warning: Maximum number of datasets reached\n");
            break;
        }

        // Create Dataset entry
        strncpy(datasets[dataset_count].name, line, MAX_PATH_LENGTH - 1);
        datasets[dataset_count].tree = NULL;
        datasets[dataset_count].last_accessed = time(NULL);
        dataset_count++;
        
        printf("Found dataset: %s\n", line);
    }

    fclose(file);
}

BPT* find_BPT_by_name(const char* name) {
    EnterCriticalSection(&datasets_mutex);
    BPT* tree = NULL;
    int found_index = -1;
    
    for (int i = 0; i < dataset_count; i++) {
        if (strcmp(datasets[i].name, name) == 0) {
            found_index = i;
            if (datasets[i].tree) {
                tree = datasets[i].tree;
            } else {
                tree = load_tree_from_json(datasets[i].name);
                if (tree) {
                    datasets[i].tree = tree;
                }
            }
            break;
        }
    }
    
    if (found_index >= 0) {
        datasets[found_index].last_accessed = time(NULL);
    }
    
    LeaveCriticalSection(&datasets_mutex);
    return tree;
}

DWORD WINAPI handle_client(LPVOID client_socket) {
    SOCKET sock = (SOCKET)client_socket;
    char* buffer = malloc(MAX_REQUEST_SIZE);
    if (!buffer) {
        const char* error = "{\"error\": \"Server memory allocation failed\", \"code\": 500}";
        send(sock, error, strlen(error), 0);
        closesocket(sock);
        return 1;
    }

    // Initialize buffer
    memset(buffer, 0, MAX_REQUEST_SIZE);
    int total_bytes = 0;
    int bytes_received;

    // Read in a loop until we get the complete request
    while ((bytes_received = recv(sock, buffer + total_bytes, MAX_REQUEST_SIZE - total_bytes - 1, 0)) > 0) {
        total_bytes += bytes_received;
        buffer[total_bytes] = '\0';  // Ensure null termination
        
        printf("Received %d bytes, total: %d bytes\n", bytes_received, total_bytes);

        // Check if we've received the complete request
        if (strstr(buffer, "\r\n\r\n")) {
            // If this is a POST request, check Content-Length
            if (strstr(buffer, "POST") == buffer) {
                char* content_length_str = strstr(buffer, "Content-Length: ");
                if (content_length_str) {
                    int content_length = atoi(content_length_str + 16);
                    char* body = strstr(buffer, "\r\n\r\n");
                    if (body) {
                        body += 4;
                        printf("Body length: %zu, Expected length: %d\n", strlen(body), content_length);
                        if ((int)strlen(body) >= content_length) {
                            printf("Complete request received!\n");
                            break;  // We have the complete request
                        } else {
                            printf("Still need %d more bytes\n", content_length - (int)strlen(body));
                        }
                    }
                } else {
                    // No Content-Length header for POST request
                    printf("POST request without Content-Length, stopping\n");
                    break;
                }
            } else {
                // Not a POST request, we can stop after headers
                printf("Not a POST request, stopping after headers\n");
                break;
            }
        }
        
        // Check if buffer is full
        if (total_bytes >= MAX_REQUEST_SIZE - 1) {
            printf("Request too large (max: %d)\n", MAX_REQUEST_SIZE);
            const char* error = "{\"error\": \"Request too large\", \"code\": 413}";
            send(sock, error, strlen(error), 0);
            free(buffer);
            closesocket(sock);
            return 1;
        }

        printf("Waiting for more data...\n");
        Sleep(1);  // Small delay to prevent tight loop
    }

    // Check for connection errors
    if (bytes_received == SOCKET_ERROR) {
        printf("Socket error: %d\n", WSAGetLastError());
        const char* error = "{\"error\": \"Connection error\", \"code\": 500}";
        send(sock, error, strlen(error), 0);
        free(buffer);
        closesocket(sock);
        return 1;
    }

    if (total_bytes <= 0) {
        const char* error = "{\"error\": \"Empty request received\", \"code\": 400}";
        send(sock, error, strlen(error), 0);
        free(buffer);
        closesocket(sock);
        return 1;
    }

    // Parse request
    Request req = {0};
    Response res = {0};
    parse_request(buffer, &req);
    parse_path_params(&req);

    // Get dataset name
    Param* dataset_param = get_path_param(&req, "dataset");
    if (!dataset_param) {
        const char* error = "{\"error\": \"Missing dataset parameter\", \"code\": 400}";
        send(sock, error, strlen(error), 0);
        free(buffer);
        closesocket(sock);
        return 1;
    }

    // Find dataset
    BPT* tree = NULL;
    if (strcmp(req.method, "POST") != 0 || !strstr(req.path, "/create")) {
        tree = find_BPT_by_name(dataset_param->value);
        if (!tree) {
            char error[256];
            snprintf(error, sizeof(error), 
                "{\"error\": \"Dataset '%s' not found\", \"code\": 404}", 
                dataset_param->value);
            send(sock, error, strlen(error), 0);
            free(buffer);
            closesocket(sock);
            return 1;
        }
    }

    // Handle operations
    if (strcmp(req.method, "POST") == 0) {
        if (strstr(req.path, "/bulk")) {
            printf("\n=== Debug: Bulk Insert Request ===\n");
            printf("Request body: %s\n", req.body);
            
            if (!req.body[0]) {
                const char* error = "{\"error\": \"Empty request body\", \"code\": 400}";
                send(sock, error, strlen(error), 0);
            } else {
                cJSON* root = cJSON_Parse(req.body);
                if (!root) {
                    printf("JSON Parse Error: %s\n", cJSON_GetErrorPtr());
                    const char* error = "{\"error\": \"Invalid JSON in request body\", \"code\": 400}";
                    send(sock, error, strlen(error), 0);
                } else {
                    printf("JSON parsed successfully. Root type: %d\n", root->type);
                    cJSON* entries = cJSON_GetObjectItem(root, "entries");
                    if (!entries || !cJSON_IsArray(entries)) {
                        printf("Entries not found or not an array\n");
                        const char* error = "{\"error\": \"Missing or invalid 'entries' array\", \"code\": 400}";
                        send(sock, error, strlen(error), 0);
                        cJSON_Delete(root);
                    } else {
                        printf("Found entries array with %d items\n", cJSON_GetArraySize(entries));
                        int count = bulk_insert(tree, entries);
                        cJSON_Delete(root);

                        cJSON* response = cJSON_CreateObject();
                        if (count >= 0) {
                            cJSON_AddBoolToObject(response, "success", true);
                            cJSON_AddNumberToObject(response, "inserted", count);
                        } else {
                            cJSON_AddBoolToObject(response, "success", false);
                            cJSON_AddStringToObject(response, "error", "Bulk insert failed");
                        }
                        char* json_str = cJSON_Print(response);
                        send(sock, json_str, strlen(json_str), 0);
                        free(json_str);
                        cJSON_Delete(response);
                    }
                }
            }
        } 
        else if (strstr(req.path, "/create")) {
            Param* order_param = get_path_param(&req, "order");
            if (!order_param) {
                const char* error = "{\"error\": \"Missing order parameter\", \"code\": 400}";
                send(sock, error, strlen(error), 0);
            } else {
                int T = atoi(order_param->value);
                if (T < 3) {
                    const char* error = "{\"error\": \"Order must be at least 3\", \"code\": 400}";
                    send(sock, error, strlen(error), 0);
                } else {
                    EnterCriticalSection(&datasets_mutex);
                    
                    // Check if dataset already exists
                    for (int i = 0; i < dataset_count; i++) {
                        if (strcmp(datasets[i].name, dataset_param->value) == 0) {
                            LeaveCriticalSection(&datasets_mutex);
                            const char* error = "{\"error\": \"Dataset already exists\", \"code\": 400}";
                            send(sock, error, strlen(error), 0);
                            return 0;
                        }
                    }
                    
                    // Create new dataset
                    BPT* new_tree = create_dataset(dataset_param->value, T);
                    if (new_tree) {
                        // Add to datasets array
                        if (dataset_count < MAX_DATASET_NUMBER) {
                            strncpy(datasets[dataset_count].name, dataset_param->value, MAX_PATH_LENGTH - 1);
                            datasets[dataset_count].tree = new_tree;
                            datasets[dataset_count].last_accessed = time(NULL);
                            dataset_count++;
                            
                            // Add to file
                            add_dataset_to_file(dataset_param->value);
                            
                            cJSON* response = cJSON_CreateObject();
                            cJSON_AddBoolToObject(response, "success", true);
                            cJSON_AddStringToObject(response, "message", "Dataset created successfully");
                            cJSON_AddNumberToObject(response, "order", T);
                            char* json_str = cJSON_Print(response);
                            send(sock, json_str, strlen(json_str), 0);
                            free(json_str);
                            cJSON_Delete(response);
                        } else {
                            free_tree(new_tree);
                            const char* error = "{\"error\": \"Maximum number of datasets reached\", \"code\": 500}";
                            send(sock, error, strlen(error), 0);
                        }
                    } else {
                        const char* error = "{\"error\": \"Failed to create dataset\", \"code\": 500}";
                        send(sock, error, strlen(error), 0);
                    }
                    
                    LeaveCriticalSection(&datasets_mutex);
                }
            }
        }
    } 
    else if (strcmp(req.method, "GET") == 0) {
        if (strstr(req.path, "/search")) {
            Param* key_param = get_path_param(&req, "key");
            if (!key_param) {
                const char* error = "{\"error\": \"Missing key parameter\", \"code\": 400}";
                send(sock, error, strlen(error), 0);
            } else {
                int key = atoi(key_param->value);
                cJSON* result = search_key(tree, key);
                if (result) {
                    char* json_str = cJSON_Print(result);
                    send(sock, json_str, strlen(json_str), 0);
                    free(json_str);
                    cJSON_Delete(result);
                } else {
                    char error[256];
                    snprintf(error, sizeof(error), 
                        "{\"error\": \"Key %d not found\", \"code\": 404}", key);
                    send(sock, error, strlen(error), 0);
                }
            }
        } 
        else if (strstr(req.path, "/range")) {
            Param* start_param = get_path_param(&req, "start");
            Param* end_param = get_path_param(&req, "end");
            if (!start_param || !end_param) {
                const char* error = "{\"error\": \"Missing range parameters\", \"code\": 400}";
                send(sock, error, strlen(error), 0);
            } else {
                int start = atoi(start_param->value);
                int end = atoi(end_param->value);
                if (start > end) {
                    const char* error = "{\"error\": \"Invalid range: start > end\", \"code\": 400}";
                    send(sock, error, strlen(error), 0);
                } else {
                    cJSON* result = range_query_dataset(tree, start, end);
                    if (result) {
                        char* json_str = cJSON_Print(result);
                        send(sock, json_str, strlen(json_str), 0);
                        free(json_str);
                        cJSON_Delete(result);
                    } else {
                        const char* error = "{\"error\": \"Range query failed\", \"code\": 500}";
                        send(sock, error, strlen(error), 0);
                    }
                }
            }
        }
    }
    else if (strcmp(req.method, "DELETE") == 0) {
        if (strstr(req.path, "/key")) {
            Param* key_param = get_path_param(&req, "key");
            if (!key_param) {
                const char* error = "{\"error\": \"Missing key parameter\", \"code\": 400}";
                send(sock, error, strlen(error), 0);
            } else {
                int key = atoi(key_param->value);
                int result = delete_from_dataset(tree, key);
                cJSON* response = cJSON_CreateObject();
                if (result == 0) {
                    cJSON_AddBoolToObject(response, "success", true);
                    cJSON_AddStringToObject(response, "message", "Key deleted successfully");
                } else {
                    cJSON_AddBoolToObject(response, "success", false);
                    cJSON_AddStringToObject(response, "error", "Key not found or deletion failed");
                }
                char* json_str = cJSON_Print(response);
                send(sock, json_str, strlen(json_str), 0);
                free(json_str);
                cJSON_Delete(response);
            }
        }
        else if (strstr(req.path, "/dataset")) {
            EnterCriticalSection(&datasets_mutex);
            
            // Find and remove from array
            int found_index = -1;
            for (int i = 0; i < dataset_count; i++) {
                if (strcmp(datasets[i].name, dataset_param->value) == 0) {
                    found_index = i;
                    if (datasets[i].tree) {
                        free_tree(datasets[i].tree);
                    }
                    break;
                }
            }
            
            if (found_index >= 0) {
                // Remove from file
                remove_dataset_from_file(dataset_param->value);
                
                // Remove from array by shifting remaining elements
                for (int i = found_index; i < dataset_count - 1; i++) {
                    datasets[i] = datasets[i + 1];
                }
                dataset_count--;
                
                // Delete actual dataset
                delete_dataset(dataset_param->value);
                
                cJSON* response = cJSON_CreateObject();
                cJSON_AddBoolToObject(response, "success", true);
                cJSON_AddStringToObject(response, "message", "Dataset deleted successfully");
                char* json_str = cJSON_Print(response);
                send(sock, json_str, strlen(json_str), 0);
                free(json_str);
                cJSON_Delete(response);
            } else {
                const char* error = "{\"error\": \"Dataset not found\", \"code\": 404}";
                send(sock, error, strlen(error), 0);
            }
            
            LeaveCriticalSection(&datasets_mutex);
        }
    }

    free(buffer);
    closesocket(sock);
    return 0;
}

DWORD WINAPI run_server(LPVOID arg) {
    WSADATA wsa;
    SOCKET server_socket, client_socket;
    struct sockaddr_in server, client;
    int c;

    printf("Initializing Winsock...\n");
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        printf("Failed. Error Code: %d\n", WSAGetLastError());
        return 1;
    }

    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        printf("Could not create socket: %d\n", WSAGetLastError());
        return 1;
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(DEFAULT_PORT);

    if (bind(server_socket, (struct sockaddr *)&server, sizeof(server)) == SOCKET_ERROR) {
        printf("Bind failed: %d\n", WSAGetLastError());
        closesocket(server_socket);
        return 1;
    }

    listen(server_socket, MAX_PENDING_CONNECTIONS);
    printf("Server listening on port %d...\n", DEFAULT_PORT);

    c = sizeof(struct sockaddr_in);
    while (1) {
        client_socket = accept(server_socket, (struct sockaddr *)&client, &c);
        if (client_socket == INVALID_SOCKET) {
            printf("Accept failed: %d\n", WSAGetLastError());
            continue;
        }

        printf("Connection accepted\n");
        HANDLE client_thread = CreateThread(NULL, 0, handle_client, 
                                         (LPVOID)client_socket, 0, NULL);
        if (client_thread == NULL) {
            printf("Could not create client thread\n");
            closesocket(client_socket);
        } else {
            CloseHandle(client_thread);
        }
    }

    closesocket(server_socket);
    WSACleanup();
    return 0;
}

int main() {
    // Initialize critical section
    InitializeCriticalSection(&datasets_mutex);

    printf("Loading datasets...\n");
    load_datasets();
    printf("Found %d datasets\n", dataset_count);

    // Start cleanup thread
    HANDLE cleanup_thread = CreateThread(NULL, 0, 
        cleanup_inactive_datasets, NULL, 0, NULL);
    if (cleanup_thread == NULL) {
        printf("Error: Failed to create cleanup thread\n");
        DeleteCriticalSection(&datasets_mutex);
        return 1;
    }

    // Start server thread
    HANDLE server_thread = CreateThread(NULL, 0, run_server, NULL, 0, NULL);
    if (server_thread == NULL) {
        printf("Error: Failed to create server thread\n");
        DeleteCriticalSection(&datasets_mutex);
        CloseHandle(cleanup_thread);
        return 1;
    }

    // Wait for server thread (keeps program running)
    WaitForSingleObject(server_thread, INFINITE);

    // Cleanup
    CloseHandle(cleanup_thread);
    CloseHandle(server_thread);
    DeleteCriticalSection(&datasets_mutex);
    return 0;
}

// Add this function to handle dataset file updates
void add_dataset_to_file(const char* name) {
    FILE* file = fopen(DATASETS_FILE, "a");
    if (file) {
        fprintf(file, "%s\n", name);
        fclose(file);
    }
}

void remove_dataset_from_file(const char* name) {
    FILE* file = fopen(DATASETS_FILE, "r");
    FILE* temp = fopen("temp.txt", "w");
    
    if (!file || !temp) {
        if (file) fclose(file);
        if (temp) fclose(temp);
        return;
    }

    char line[MAX_PATH_LENGTH];
    while (fgets(line, sizeof(line), file)) {
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') {
            line[len-1] = '\0';
        }
        
        if (strcmp(line, name) != 0) {
            fprintf(temp, "%s\n", line);
        }
    }

    fclose(file);
    fclose(temp);
    remove(DATASETS_FILE);
    rename("temp.txt", DATASETS_FILE);
}