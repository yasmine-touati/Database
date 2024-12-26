#ifndef APPLICATION_H
#define APPLICATION_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "bpt.h"
#include <cJSON.h>

#ifdef _WIN32
    #include <direct.h>
    #define MKDIR(dir) _mkdir(dir)
    #define RMDIR(dir) _rmdir(dir)
#else
    #include <sys/stat.h>
    #include <unistd.h>
    #define MKDIR(dir) mkdir(dir, 0777)
    #define RMDIR(dir) rmdir(dir)
#endif

// Add dataset path helpers
#define MAX_PATH_LENGTH 256

// Helper function declarations


// Main operations
BPT* create_dataset(const char* name, int T);
void delete_dataset(const char* name);

// Bulk insert operation
int bulk_insert(BPT* tree, const cJSON* entries);

// Data operations
cJSON* search_key(BPT* tree, int key);  // New search operation
cJSON* range_query_dataset(BPT* tree, int start_key, int end_key);  // New function
int delete_from_dataset(BPT* tree, int key);  // New delete operation

#endif // APPLICATION_H
