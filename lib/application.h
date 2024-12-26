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

#define MAX_PATH_LENGTH 256
#define MAX_REQUEST_SIZE 65536  // Increase to 64KB


BPT* create_dataset(const char* name, int T);
void delete_dataset(const char* name);


int bulk_insert(BPT* tree, const cJSON* entries);


cJSON* search_key(BPT* tree, int key);  
cJSON* range_query_dataset(BPT* tree, int start_key, int end_key);  
int delete_from_dataset(BPT* tree, int key);  
void log_request(const char* dataset_name, const char* raw_request, const char* client_ip, int client_port);

#endif 
