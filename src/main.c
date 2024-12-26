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
#include <cJSON.h>

#pragma comment(lib, "ws2_32.lib")

#define MAX_DATASET_NUMBER 100
#define DATASETS_FILE "datasets.txt"
#define INACTIVE_TIMEOUT 1800
#define CHECK_INTERVAL 360
#define MAX_REQUEST_SIZE 61440  // 60KB in bytes
#define DEFAULT_PORT 6667
#define MAX_PENDING_CONNECTIONS 10

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
        closesocket(sock);
        return 1;
    }

    int bytes_received = recv(sock, buffer, MAX_REQUEST_SIZE - 1, 0);
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';
        printf("Received request: %s\n", buffer);
        
        // TODO: Process request and send response
        const char* response = "{\"status\": \"ok\"}";
        send(sock, response, strlen(response), 0);
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