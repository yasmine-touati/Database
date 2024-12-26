#include "../lib/application.h"
#include "../lib/service.h"
#include "../lib/persister.h"
#include "../lib/dfh.h"
#include "../lib/utils.h"
#include <dirent.h>
#include <errno.h>

BPT* create_dataset(const char* name, int T) {
    // Create main directory
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
    
    // First, delete all files in the data directory
    DIR* dir = opendir(data_path);
    if (dir) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL) {
            // Skip . and .. directories
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }
            
            char file_path[MAX_PATH_LENGTH];
            snprintf(file_path, MAX_PATH_LENGTH, "%s/%s", data_path, entry->d_name);
            
            printf("Deleting file: %s\n", file_path);
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

    // Delete index.json from main directory
    char index_path[MAX_PATH_LENGTH];
    snprintf(index_path, MAX_PATH_LENGTH, "%s/index.json", name);
    printf("Deleting file: %s\n", index_path);
    if (remove(index_path) != 0 && errno != ENOENT) {
        printf("Warning: Failed to delete index file %s: %s\n", 
               index_path, strerror(errno));
    }

    // Delete the data subdirectory
    printf("Deleting directory: %s\n", data_path);
    if (RMDIR(data_path) != 0 && errno != ENOENT) {
        printf("Warning: Failed to delete data directory %s: %s\n", 
               data_path, strerror(errno));
    }

    // Finally delete the main dataset directory
    printf("Deleting directory: %s\n", name);
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

        printf("Bulk inserting: Key=%d, Value=%s\n", key, line);
        insert(tree, key, line);
        count++;
    }

    printf("Bulk insert completed: %d entries inserted\n", count);
    return count;
}

cJSON* search_key(BPT* tree, int key) {
    if (!tree) return NULL;

    Node* leaf = search(tree, key);
    if (!leaf) {
        printf("Key %d not found\n", key);
        return NULL;
    }

    // Find the key's position in the leaf node
    int pos = binary_search(leaf->keys, leaf->n, key);
    if (pos == -1) {
        printf("Key %d not found\n", key);
        return NULL;
    }

    // Read the line from the data file
    char buffer[1024];
    int result = dfh_read_line(tree->dataset_name, leaf->file_pointer, key, buffer, sizeof(buffer));
    if (result != DFH_SUCCESS) {
        printf("Failed to read data for key %d\n", key);
        return NULL;
    }

    // Create JSON response
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

    // Create JSON array for results
    cJSON* results = cJSON_CreateArray();
    if (!results) return NULL;

    // Get first leaf node
    Node* cursor = tree->root;
    while (!cursor->is_leaf) {
        cursor = cursor->children[0];
    }

    // Find the first node containing keys in our range
    while (cursor && cursor->keys[0] <= end_key) {
        char buffer[1024];
        
        // Check each key in this node
        for (int i = 0; i < cursor->n; i++) {
            int current_key = cursor->keys[i];
            
            // If key is in range
            if (current_key >= start_key && current_key <= end_key) {
                // Read directly from file
                if (dfh_read_line(tree->dataset_name, cursor->file_pointer, 
                                current_key, buffer, sizeof(buffer)) == DFH_SUCCESS) {
                    // Add to results
                    cJSON* entry = cJSON_CreateObject();
                    if (entry) {
                        cJSON_AddNumberToObject(entry, "key", current_key);
                        cJSON_AddStringToObject(entry, "line", buffer);
                        cJSON_AddItemToArray(results, entry);
                    }
                }
            }
            
            // If we've passed the end key, we're done
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
    printf("Key %d deleted from dataset\n", key);
    return 0;
}






