#ifndef DFH_H
#define DFH_H

#include <stdio.h>
#include "node.h"

// File operations status codes
#define DFH_SUCCESS 0
#define DFH_ERROR_OPEN -1
#define DFH_ERROR_READ -2
#define DFH_ERROR_WRITE -3
#define DFH_ERROR_SEEK -4

#define MAX_LINE_SIZE 1024

// CSV line operations
int dfh_write_line(const char* file_pointer, int key, const char* line);
int dfh_read_line(const char* file_pointer, int key, char* buffer, size_t buffer_size);
int dfh_read_lines(const char* file_pointer, int start_offset, int end_offset, char** lines, int* num_lines);
int dfh_delete_lines(const char* file_pointer, int* keys, int num_keys);

// Node operations
int dfh_move_lines(const char* source_fp, const char* dest_fp, int* keys, int num_keys);
int dfh_merge_files(const char* taker_fp, const char* giver_fp);
int dfh_create_datafile(const char* file_pointer);
int dfh_verify_file(const char* file_pointer, int* keys, int num_keys);
int dfh_remove_line(const char* file_pointer, int key);


#endif // DFH_H