#ifndef BPT_H
#define BPT_H

#include "node.h"

typedef struct BPT{
    Node *root;
    int T;
    char* dataset_name;
} BPT;

BPT* create_BPT( const char *dataset_name, int T);
void insert(BPT *tree, int key, const char *line);
int delete(BPT *tree, int key);
Node* search(BPT *tree, int key);
void print_tree(Node *node, int level);
Node* get_first_leaf_node(BPT *tree);
Node* get_last_leaf_node(BPT *tree);
char** ranged_query(BPT *tree, int low_limit, int up_limit, int *low_offset, int *up_offset);
void free_tree(BPT* tree);
void free_node(Node *node, const char* dataset_name);

#endif

