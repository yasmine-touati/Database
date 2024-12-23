#ifndef BPT_H
#define BPT_H

#include "node.h"

typedef struct BPT {
    Node *root;
    int T;
} BPT;

void print_tree(Node *node, int level);

BPT* create_BPT(int T);
void insert(BPT *tree, int key);
Node* search(BPT *tree, int key);
char** ranged_query(BPT *tree, int low_limit, int up_limit, int *low_offset, int *up_offset);

#endif

