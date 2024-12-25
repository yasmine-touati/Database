#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../lib/dfh.h"
#include "../lib/utils.h"

#define MAX_LINE_SIZE 1024

char* get_full_path(const char* file_pointer) {
    const char* DATA_DIR = "data/";
    size_t path_len = strlen(DATA_DIR) + strlen(file_pointer) + 5; // +5 for ".dat\0"
    char* full_path = malloc(path_len);
    if (!full_path) return NULL;
    
    snprintf(full_path, path_len, "%s%s.dat", DATA_DIR, file_pointer);
    return full_path;
}

int dfh_create_datafile(const char* file_pointer) {
    char* full_path = get_full_path(file_pointer);
    if (!full_path) return DFH_ERROR_OPEN;
    
    FILE* file = fopen(full_path, "w");  // Changed to text mode
    free(full_path);
    
    if (!file) return DFH_ERROR_OPEN;
    fclose(file);
    return DFH_SUCCESS;
}

int dfh_write_line(const char* file_pointer, int key, const char* line) {
    char* full_path = get_full_path(file_pointer);
    if (!full_path) return DFH_ERROR_OPEN;
    
    // First, check if key already exists and read current content
    FILE* read_file = fopen(full_path, "r");
    char temp_path[1024];
    snprintf(temp_path, sizeof(temp_path), "%s.tmp", full_path);
    FILE* temp_file = fopen(temp_path, "w");
    
    if (!temp_file) {
        if (read_file) fclose(read_file);
        free(full_path);
        return DFH_ERROR_OPEN;
    }

    bool key_written = false;
    if (read_file) {
        char buffer[MAX_LINE_SIZE];
        int current_key;
        
        // Copy existing content and insert new line in order
        while (fgets(buffer, sizeof(buffer), read_file)) {
            sscanf(buffer, "%d", &current_key);
            
            // If we find where our new key should go
            if (!key_written && key < current_key) {
                if (line[strlen(line)-1] == '\n') {
                    fprintf(temp_file, "%d\t%s", key, line);
                } else {
                    fprintf(temp_file, "%d\t%s\n", key, line);
                }
                key_written = true;
            }
            
            // Skip if it's a duplicate key
            if (current_key != key) {
                fputs(buffer, temp_file);
            }
        }
        fclose(read_file);
    }
    
    // If key wasn't written yet (largest key or empty file)
    if (!key_written) {
        if (line[strlen(line)-1] == '\n') {
            fprintf(temp_file, "%d\t%s", key, line);
        } else {
            fprintf(temp_file, "%d\t%s\n", key, line);
        }
    }
    
    fclose(temp_file);
    
    // Replace original file with temp file
    remove(full_path);
    rename(temp_path, full_path);
    
    free(full_path);
    return DFH_SUCCESS;
}

int dfh_read_line(const char* file_pointer, int key, char* buffer, size_t buffer_size) {
    char* full_path = get_full_path(file_pointer);
    if (!full_path) return DFH_ERROR_OPEN;
    
    FILE* file = fopen(full_path, "r");
    free(full_path);
    
    if (!file) return DFH_ERROR_OPEN;

    char line[MAX_LINE_SIZE];
    int found_key;
    while (fgets(line, sizeof(line), file)) {
        if (sscanf(line, "%d", &found_key) == 1 && found_key == key) {
            char* tab_pos = strchr(line, '\t');
            if (tab_pos) {
                tab_pos++; // Skip the tab
                strncpy(buffer, tab_pos, buffer_size - 1);
                buffer[buffer_size - 1] = '\0';
                fclose(file);
                return DFH_SUCCESS;
            }
        }
    }

    fclose(file);
    return DFH_ERROR_READ;
}

int dfh_move_lines(const char* source_fp, const char* dest_fp, int* keys, int num_keys) {
    char buffer[MAX_LINE_SIZE];
    
    // First copy the lines to destination
    for (int i = 0; i < num_keys; i++) {
        if (dfh_read_line(source_fp, keys[i], buffer, sizeof(buffer)) == DFH_SUCCESS) {
            if (dfh_write_line(dest_fp, keys[i], buffer) != DFH_SUCCESS) {
                return DFH_ERROR_WRITE;
            }
        } else {
            return DFH_ERROR_READ;
        }
    }
    
    // Then delete them from source - ensure this happens
    int delete_result = dfh_delete_lines(source_fp, keys, num_keys);
    if (delete_result != DFH_SUCCESS) {
        printf("Failed to delete moved lines from source file\n");
        return delete_result;
    }
    
    return DFH_SUCCESS;
}

int dfh_merge_files(const char* taker_fp, const char* giver_fp) {
    char* taker_path = get_full_path(taker_fp);
    char* giver_path = get_full_path(giver_fp);
    
    if (!taker_path || !giver_path) {
        free(taker_path);
        free(giver_path);
        return DFH_ERROR_OPEN;
    }
    
    FILE* giver = fopen(giver_path, "r");
    if (!giver) {
        free(taker_path);
        free(giver_path);
        return DFH_ERROR_OPEN;
    }

    // Read all lines from giver and write to taker
    char line[MAX_LINE_SIZE];
    int key;
    char content[MAX_LINE_SIZE];
    
    while (fgets(line, sizeof(line), giver)) {
        if (sscanf(line, "%d\t%[^\n]", &key, content) == 2) {
            if (dfh_write_line(taker_fp, key, content) != DFH_SUCCESS) {
                fclose(giver);
                free(taker_path);
                free(giver_path);
                return DFH_ERROR_WRITE;
            }
        }
    }

    fclose(giver);

    // Delete the giver file after successful merge
    if (remove(giver_path) != 0) {
        printf("Warning: Failed to delete giver file %s\n", giver_path);
    }

    free(taker_path);
    free(giver_path);
    return DFH_SUCCESS;
}

int dfh_verify_file(const char* file_pointer, int* keys, int num_keys) {
    char buffer[MAX_LINE_SIZE];
    for (int i = 0; i < num_keys; i++) {
        if (dfh_read_line(file_pointer, keys[i], buffer, MAX_LINE_SIZE) != DFH_SUCCESS) {
            printf("Failed to verify key %d in file %s\n", keys[i], file_pointer);
            return DFH_ERROR_READ;
        }
    }
    return DFH_SUCCESS;
}

bool is_file_empty(const char* file_pointer) {
    char* full_path = get_full_path(file_pointer);
    if (!full_path) return true;
    
    FILE* file = fopen(full_path, "r");
    if (!file) {
        free(full_path);
        return true;
    }
    
    char buffer[MAX_LINE_SIZE];
    bool is_empty = (fgets(buffer, sizeof(buffer), file) == NULL);
    
    fclose(file);
    free(full_path);
    return is_empty;
}

int dfh_delete_lines(const char* file_pointer, int* keys, int num_keys) {
    char* full_path = get_full_path(file_pointer);
    if (!full_path) return DFH_ERROR_OPEN;
    
    FILE* original = fopen(full_path, "r");
    if (!original) {
        free(full_path);
        return DFH_ERROR_OPEN;
    }
    
    char temp_path[1024];
    snprintf(temp_path, sizeof(temp_path), "%s.tmp", full_path);
    FILE* temp = fopen(temp_path, "w");
    
    if (!temp) {
        fclose(original);
        free(full_path);
        return DFH_ERROR_OPEN;
    }

    char line[MAX_LINE_SIZE];
    int current_key;
    bool has_content = false;
    
    while (fgets(line, sizeof(line), original)) {
        bool should_keep = true;
        sscanf(line, "%d", &current_key);
        
        for (int i = 0; i < num_keys; i++) {
            if (current_key == keys[i]) {
                should_keep = false;
                break;
            }
        }
        
        if (should_keep) {
            fputs(line, temp);
            has_content = true;
        }
    }

    fclose(original);
    fclose(temp);
    
    if (!has_content) {
        // If file is empty after deletions, remove both files
        remove(full_path);
        remove(temp_path);
    } else {
        // Replace original with temp file
        remove(full_path);
        rename(temp_path, full_path);
    }
    
    free(full_path);
    return DFH_SUCCESS;
} 