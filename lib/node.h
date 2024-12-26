#ifndef NODE_H
#define NODE_H

#include <stdbool.h>

typedef struct Node {
    int *keys;
    struct Node **children;
    struct Node *parent;
    struct Node *next;
    char *file_pointer;
    int n;
    bool is_leaf;
} Node;

Node* create_node(const char* dataset_name, bool is_leaf, int T);
void insert_into_node(Node *node, int key);
void insert_into_leaf(const char* dataset_name, Node *node, int key, const char* line);
char* generate_file_pointer(void);

#endif