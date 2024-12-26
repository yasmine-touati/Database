#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include "../lib/node.h"
#include "../lib/utils.h"
#include "../lib/dfh.h"

static int seed_initialized = 0;

char* generate_file_pointer() {
    if (!seed_initialized) {
        srand((unsigned int)time(NULL));
        seed_initialized = 1;
    }
    
    char *file_pointer = (char *)malloc(33 * sizeof(char)); 
    if (file_pointer == NULL) {
        memory_allocation_failed();
    }

    for (int i = 0; i < 32; i++) {
        file_pointer[i] = '0' + (rand() % 10); 
    }
    file_pointer[32] = '\0'; 

    return file_pointer;
}

Node* create_node(const char* dataset_name, bool is_leaf, int T) {
    Node *node = (Node *)malloc(sizeof(Node));
    if (node == NULL) {
       memory_allocation_failed();
    }

    node->keys = (int *)malloc((T - 1) * sizeof(int));
    if (node->keys == NULL) {
        memory_allocation_failed();
    }

    node->children = (Node **)malloc(T * sizeof(Node *));
    if (node->children == NULL) {
        memory_allocation_failed();
    }

    node->is_leaf = is_leaf;
    node->n = 0;
    node->parent = NULL;
    node->next = NULL;
    if (is_leaf) {
        node->file_pointer = generate_file_pointer();
        dfh_create_datafile(dataset_name, node->file_pointer);
    } else {
        node->file_pointer = NULL;
    }

    return node;
}

void insert_into_node(Node *node, int key) {
    int i = node->n - 1;
    while (i >= 0 && node->keys[i] > key) {
        node->keys[i + 1] = node->keys[i];
        i--;
    }
    node->keys[i + 1] = key;
    node->n++;
}

void insert_into_leaf(const char* dataset_name, Node *node, int key, const char* line) {
    // First write the data to ensure it's stored
    if (dfh_write_line(dataset_name, node->file_pointer, key, line) != DFH_SUCCESS) {
        printf("Failed to write data for key %d\n", key);
        return;
    }

    // Then insert the key into the node
    int i = node->n - 1;
    while (i >= 0 && node->keys[i] > key) {
        node->keys[i + 1] = node->keys[i];
        i--;
    }
    node->keys[i + 1] = key;
    node->n++;
}
