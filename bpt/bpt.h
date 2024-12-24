#ifndef BPT_H
#define BPT_H

#include "node.h"

typedef struct {
    Node *root;
    int T;
} BPT;

void insert(BPT *tree, int key, const char* line);
void delete(BPT *tree, int key);
Node* search(BPT *tree, int key);
void print_tree(Node *node, int level);
BPT* create_BPT(int T);
Node* get_first_leaf_node(BPT *tree);
Node* get_last_leaf_node(BPT *tree);
char** ranged_query(BPT *tree, int low_limit, int up_limit, int *low_offset, int *up_offset);

#endif

