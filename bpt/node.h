#ifndef NODE_H
#define NODE_H

#include <stdbool.h>

typedef struct Node {
    int *keys;              // Array of keys
    struct Node **children; // Array of child pointers
    struct Node *parent;    // Pointer to parent node
    struct Node *next;      // Pointer to next leaf node (for leaf nodes only)
    char *file_pointer;     // Pointer to file location (for leaf nodes)
    int n;                  // Current number of keys
    bool is_leaf;          // Whether this node is a leaf
} Node;

// Node creation and manipulation functions
Node* create_node(bool is_leaf, int T);
void insert_into_node(Node *node, int key);
void insert_into_leaf(Node *node, int key, const char* line);
char* generate_file_pointer(void);

#endif // NODE_H