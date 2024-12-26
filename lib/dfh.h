#ifndef DFH_H
#define DFH_H

#include <stdio.h>
#include "node.h"
#include <stdbool.h>

#define DFH_SUCCESS 0
#define DFH_ERROR_OPEN -1
#define DFH_ERROR_READ -2
#define DFH_ERROR_WRITE -3
#define DFH_ERROR_SEEK -4

#define MAX_LINE_SIZE 1024
#define MAX_PATH_LENGTH 256

char* get_full_path(const char* dataset_name, const char* file_pointer);
int dfh_create_datafile(const char* dataset_name, const char* file_pointer);
int dfh_write_line(const char* dataset_name, const char* file_pointer, int key, const char* line);
int dfh_read_line(const char* dataset_name, const char* file_pointer, int key, char* buffer, size_t buffer_size);
int dfh_delete_lines(const char* dataset_name, const char* file_pointer, int* keys, int num_keys);
int dfh_move_lines(const char* dataset_name, const char* source_fp, const char* dest_fp, int* keys, int num_keys);
int dfh_merge_files(const char* dataset_name, const char* taker_fp, const char* giver_fp);
int dfh_verify_file(const char* dataset_name, const char* file_pointer, int* keys, int num_keys);
bool is_file_empty(const char* dataset_name, const char* file_pointer);

#endif