#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include "node.h"
#include "utils.h"




char* generate_file_pointer() {
    char *file_pointer = (char *)malloc(33 * sizeof(char)); 
    if (file_pointer == NULL) {
        
    }

    srand((unsigned int)time(NULL));

    for (int i = 0; i < 32; i++) {
        file_pointer[i] = '0' + (rand() % 10); 
    }
    file_pointer[32] = '\0'; 

    return file_pointer;
}


Node* create_node(bool is_leaf, int T) {
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
        node->file_pointer = NULL;
    } else {
        node->file_pointer = generate_file_pointer();
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