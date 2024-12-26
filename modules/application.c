#include "../lib/application.h"
#include "../lib/service.h"
#include "../lib/persister.h"
#include "../lib/dfh.h"
#include "../lib/utils.h"
#include <dirent.h>
#include <errno.h>
#include <time.h>

BPT* create_dataset(const char* name, int T) {
    if (MKDIR(name) != 0) {
        printf("Error: Could not create directory %s\n", name);
        return NULL;
    }

    char data_path[MAX_PATH_LENGTH];
    snprintf(data_path, sizeof(data_path), "%s/data", name);
    if (MKDIR(data_path) != 0) {
        printf("Error: Could not create data directory %s\n", data_path);
        return NULL;
    }

    // Create logs file
    char log_path[MAX_PATH_LENGTH];
    snprintf(log_path, sizeof(log_path), "%s/logs.txt", name);
    FILE* log_file = fopen(log_path, "w");
    if (log_file) {
        time_t now;
        struct tm* timeinfo;
        char timestamp[26];
        
        time(&now);
        timeinfo = localtime(&now);
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);
        
        fprintf(log_file, "=== Dataset Created: %s ===\n", timestamp);
        fprintf(log_file, "Order (T): %d\n", T);
        fprintf(log_file, "=====================================\n");
        fclose(log_file);
    }

    BPT* tree = create_BPT(name, T);
    if (!tree) {
        printf("Error: Could not create B+ tree\n");
        return NULL;
    }

    save_tree_to_json(tree);
    return tree;
}

void delete_dataset(const char* name) {
    char data_path[MAX_PATH_LENGTH];
    snprintf(data_path, sizeof(data_path), "%s/data", name);
    
    DIR* dir = opendir(data_path);
    if (dir) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }
            
            char file_path[MAX_PATH_LENGTH];
            snprintf(file_path, MAX_PATH_LENGTH, "%s/%s", data_path, entry->d_name);
            
            if (remove(file_path) != 0) {
                printf("Warning: Failed to delete file %s: %s\n", 
                       file_path, strerror(errno));
            }
        }
        closedir(dir);
    } else {
        printf("Warning: Could not open data directory %s: %s\n", 
               data_path, strerror(errno));
    }


    char index_path[MAX_PATH_LENGTH];
    snprintf(index_path, MAX_PATH_LENGTH, "%s/index.json", name);
    if (remove(index_path) != 0 && errno != ENOENT) {
        printf("Warning: Failed to delete index file %s: %s\n", 
               index_path, strerror(errno));
    }


    if (RMDIR(data_path) != 0 && errno != ENOENT) {
        printf("Warning: Failed to delete data directory %s: %s\n", 
               data_path, strerror(errno));
    }

    if (RMDIR(name) != 0 && errno != ENOENT) {
        printf("Warning: Failed to delete dataset directory %s: %s\n", 
               name, strerror(errno));
    }
}

int bulk_insert(BPT* tree, const cJSON* entries) {
    if (!tree || !entries || !cJSON_IsArray(entries)) {
        printf("Error: Invalid parameters for bulk insert\n");
        return -1;
    }

    int count = 0;
    cJSON* entry = NULL;
    cJSON_ArrayForEach(entry, entries) {
        cJSON* key_obj = cJSON_GetObjectItem(entry, "key");
        cJSON* line_obj = cJSON_GetObjectItem(entry, "line");

        if (!cJSON_IsNumber(key_obj) || !cJSON_IsString(line_obj)) {
            printf("Warning: Skipping invalid entry in bulk insert\n");
            continue;
        }

        int key = key_obj->valueint;
        const char* line = line_obj->valuestring;

        insert(tree, key, line);
        count++;
    }
    return count;
}

cJSON* search_key(BPT* tree, int key) {
    if (!tree) return NULL;

    Node* leaf = search(tree, key);
    if (!leaf) {
        printf("Key %d not found\n", key);
        return NULL;
    }

    int pos = binary_search(leaf->keys, leaf->n, key);
    if (pos == -1) {
        printf("Key %d not found\n", key);
        return NULL;
    }

    char buffer[1024];
    int result = dfh_read_line(tree->dataset_name, leaf->file_pointer, key, buffer, sizeof(buffer));
    if (result != DFH_SUCCESS) {
        printf("Failed to read data for key %d\n", key);
        return NULL;
    }

    cJSON* response = cJSON_CreateObject();
    if (!response) return NULL;

    cJSON_AddNumberToObject(response, "key", key);
    cJSON_AddStringToObject(response, "line", buffer);

    return response;
}

cJSON* range_query_dataset(BPT* tree, int start_key, int end_key) {
    if (!tree || start_key > end_key) {
        printf("Invalid range query parameters\n");
        return NULL;
    }

    cJSON* results = cJSON_CreateArray();
    if (!results) return NULL;

    Node* cursor = tree->root;
    while (!cursor->is_leaf) {
        cursor = cursor->children[0];
    }

    while (cursor && cursor->keys[0] <= end_key) {
        char buffer[1024];
        
        for (int i = 0; i < cursor->n; i++) {
            int current_key = cursor->keys[i];
            
            if (current_key >= start_key && current_key <= end_key) {
                if (dfh_read_line(tree->dataset_name, cursor->file_pointer, 
                                current_key, buffer, sizeof(buffer)) == DFH_SUCCESS) {
                    cJSON* entry = cJSON_CreateObject();
                    if (entry) {
                        cJSON_AddNumberToObject(entry, "key", current_key);
                        cJSON_AddStringToObject(entry, "line", buffer);
                        cJSON_AddItemToArray(results, entry);
                    }
                }
            }
            
            if (current_key > end_key) {
                return results;
            }
        }
        
        cursor = cursor->next;
    }

    return results;
}

int delete_from_dataset(BPT* tree, int key) {
    if (!tree) {
        printf("Error: Invalid tree\n");
        return -1;
    }

    int result = delete(tree, key);
    if (result == -1) {
        printf("Error: Failed to delete key %d from dataset\n", key);
        return -1;
    }
    return 0;
}

void log_request(const char* dataset_name, const char* raw_request, const char* client_ip, int client_port) {
    char log_path[MAX_PATH_LENGTH];
    snprintf(log_path, sizeof(log_path), "%s/logs.txt", dataset_name);
    
    FILE* log_file = fopen(log_path, "a");
    if (!log_file) return;

    time_t now;
    struct tm* timeinfo;
    char timestamp[26];
    
    time(&now);
    timeinfo = localtime(&now);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);

    fprintf(log_file, "\n=== %s ===\n", timestamp);
    fprintf(log_file, "Client: %s:%d\n", client_ip, client_port);
    fprintf(log_file, "Request:\n%s\n", raw_request);
    fprintf(log_file, "=====================================\n");

    fclose(log_file);
}






