#include<stdio.h>
#include<stdlib.h>
#include "../lib/utils.h"

void memory_allocation_failed(){
    fprintf(stderr, "Memory allocation failed\n");
    exit(1);
}

int binary_search(int *arr, int n, int key) {
    int low = 0, high = n - 1;
    while (low <= high) {
        int mid = low + (high - low) / 2;
        if (arr[mid] == key) {
            return mid;
        } else if (arr[mid] < key) {
            low = mid + 1;
        } else {
            high = mid - 1;
        }
    }
    return -1;
}

char** append(char **arr, int *size, int *capacity, char* new_element) {
    if (*size == *capacity) {
        *capacity += 50;
        arr = (char **)realloc(arr, sizeof(char *) * (*capacity));
        if (arr == NULL) {
            memory_allocation_failed();
        }
    }
    arr[*size] = new_element;
    (*size)++;
    return arr;
}