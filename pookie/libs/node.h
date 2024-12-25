#ifndef NODE_H
#define NODE_H

#include <stdbool.h>
typedef struct Node {
    int *keys;
    struct Node **children;
    struct Node *parent;
    struct Node *next;
    int n;
    bool is_leaf;
    char *file_pointer;  
} Node;

char* generate_file_pointer();
Node* create_node(bool is_leaf, int T);

#endif