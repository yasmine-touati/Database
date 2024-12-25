#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "bpt.h"
#include "node.h"
#include "utils.h"
#include "cJSON.h"


BPT* create_BPT(int T) {
    BPT *bpt = (BPT *)malloc(sizeof(BPT));
    if (bpt == NULL) {
        memory_allocation_failed();
    }

    bpt->root = create_node(true, T);
    bpt->T = T;

    return bpt;
}

void insert_into_leaf(Node *node, int key) {
    int i = node->n - 1;
    while (i >= 0 && node->keys[i] > key) {
        node->keys[i + 1] = node->keys[i];
        i--;
    }
    node->keys[i + 1] = key;
    node->n++;
}

Node* split_leaf_node(Node *node, int T, int *promote_key) {
    int mid = node->n / 2;
    Node *new_leaf = create_node(true, T);

    for (int i = mid; i < node->n; i++) {
        new_leaf->keys[i - mid] = node->keys[i];
    }
    new_leaf->n = node->n - mid;
    node->n = mid;

    *promote_key = new_leaf->keys[0];
    new_leaf->next = node->next;
    node->next = new_leaf;

    return new_leaf;
}

Node* split_internal_node(Node *node, int T, int *promote_key) {
    int mid = node->n / 2;
    Node *new_node = create_node(false, T);

    for (int i = mid + 1; i < node->n; i++) {
        new_node->keys[i - mid - 1] = node->keys[i];
    }

    for (int i = mid + 1; i <= node->n; i++) {
        new_node->children[i - mid - 1] = node->children[i];
        if (new_node->children[i - mid - 1]) {
            new_node->children[i - mid - 1]->parent = new_node;
        }
        node->children[i] = NULL;
    }

    *promote_key = node->keys[mid];
    new_node->n = node->n - mid - 1;
    node->n = mid;

    return new_node;
}

void propagate_up(BPT *tree, Node *child, Node *sibling, int promote_key) {
    Node *parent = child->parent;

    if (!parent) {
        Node *new_root = create_node(false, tree->T);
        new_root->keys[0] = promote_key;
        new_root->children[0] = child;
        new_root->children[1] = sibling;
        new_root->n = 1;
        child->parent = sibling->parent = new_root;
        tree->root = new_root;
    } else {
        int i = parent->n - 1;
        while (i >= 0 && parent->keys[i] > promote_key) {
            parent->keys[i + 1] = parent->keys[i];
            parent->children[i + 2] = parent->children[i + 1];
            i--;
        }
        parent->keys[i + 1] = promote_key;
        parent->children[i + 2] = sibling;
        sibling->parent = parent;
        parent->n++;

        if (parent->n == tree->T) {
            int new_promote_key;
            Node *new_sibling = split_internal_node(parent, tree->T, &new_promote_key);
            propagate_up(tree, parent, new_sibling, new_promote_key);
        }
    }
}

void insert(BPT *tree, int key) {
    Node *cursor = tree->root;

    while (!cursor->is_leaf) {
        int i = 0;
        while (i < cursor->n && key > cursor->keys[i]) i++;
        cursor = cursor->children[i];
    }

    insert_into_leaf(cursor, key);

    if (cursor->n == tree->T) {
        int promote_key;
        Node *new_leaf = split_leaf_node(cursor, tree->T, &promote_key);
        propagate_up(tree, cursor, new_leaf, promote_key);
    }
}

Node* search(BPT *tree,int key) {
    Node *cursor = tree->root;
    while (!cursor->is_leaf) {
        int i = 0;
        while (i < cursor->n && key > cursor->keys[i]) i++;
        cursor = cursor->children[i];
    }
    int pos = binary_search(cursor->keys, cursor->n, key);
    if (pos == -1) {
        return NULL;
    } else {
        return cursor;
    }
}

char** ranged_query(BPT *tree, int low_limit, int up_limit, int *low_offset, int *up_offset) {
    Node *low_limit_node = search(tree, low_limit);
    if (low_limit_node == NULL) {
        return NULL;
    }
    Node *up_limit_node = search(tree, up_limit);
    if (up_limit_node == NULL) {
        return NULL;
    }
    (*low_offset) = binary_search(low_limit_node->keys, low_limit_node->n, low_limit);
    (*up_offset) = binary_search(up_limit_node->keys, up_limit_node->n, up_limit);
    int size = 0, capacity = 50;
    char **result = (char **)malloc(sizeof(char *) * capacity);
    if (result == NULL) {
        memory_allocation_failed();
    }
    append(result, &size, &capacity, low_limit_node->file_pointer);
    Node *cursor = low_limit_node->next;
    while (cursor != up_limit_node) {
        append(result, &size, &capacity, cursor->file_pointer);
        cursor = cursor->next;
    }
    append(result, &size, &capacity, up_limit_node->file_pointer);
    return result;
}

Node* get_last_leaf_node(BPT *tree) {
    int key = tree->root->keys[tree->root->n - 1];
    Node *cursor = search(tree, key);
    while (cursor->next) {
        cursor = cursor->next;
    }
    return cursor;
}

Node *get_first_leaf_node(BPT *tree) {
    Node *cursor = tree->root;
    while (!cursor->is_leaf) {
        cursor = cursor->children[0];
    }
    return cursor;
}




void print_tree(Node *node, int level) {
    if (node == NULL) return;

    printf("Level %d: ", level);
    for (int i = 0; i < node->n; i++) {
        printf("%d ", node->keys[i]);
    }
    printf("\n");

    if (!node->is_leaf) {
        for (int i = 0; i <= node->n; i++) {
            if (node->children[i] != NULL) {
                print_tree(node->children[i], level + 1);
            }
        }
    }
}

//int main() {
//    printf("B+ Tree Program\n");
//    return 0;
//}

