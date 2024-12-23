#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "bpt.h"
#include "node.h"
#include "utils.h"




// BPT CREATION 

BPT* create_BPT(int T) {
    BPT *bpt = (BPT *)malloc(sizeof(BPT));
    if (bpt == NULL) {
        memory_allocation_failed();
    }

    bpt->root = create_node(true, T);
    bpt->T = T;

    return bpt;
}

// ---------------------------------------------------------

//BPT INSERTION 

// BPT INSERTION HELPER FUNCTIONS 



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

    insert_into_node(cursor, key);

    if (cursor->n == tree->T) {
        int promote_key;
        Node *new_leaf = split_leaf_node(cursor, tree->T, &promote_key);
        propagate_up(tree, cursor, new_leaf, promote_key);
    }
}

// ---------------------------------------------------------

// BPT SEARCHING 

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

// ---------------------------------------------------------

// BPT GETTING LEAF NODES 

Node* get_last_leaf_node(BPT *tree) {
    int key = tree->root->keys[tree->root->n - 1];
    Node *cursor = search(tree, key);
    while (cursor->next) {
        cursor = cursor->next;
    }
    return cursor;
}

Node* get_first_leaf_node(BPT *tree) {
    Node *cursor = tree->root;
    while (!cursor->is_leaf) {
        cursor = cursor->children[0];
    }
    return cursor;
}

// ---------------------------------------------------------

// BPT PRINTING TREE 


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

// ---------------------------------------------------------

// BPT DELETION 

int index_in_parent(Node *node) {
    Node *parent = node->parent;
    for (int i = 0; i <= parent->n; i++) {
        if (parent->children[i] == node) {
            return i;
        }
    }
    return -1;
}

void delete_key(Node *node, int key) {
    if (node->n == 1 && node->keys[0] == key) {
        node->n = 0;
        return;
    }
    int pos = binary_search(node->keys, node->n, key);
    if (pos == -1) {
        return;
    }
    for (int i = pos; i < node->n - 1; i++) {
        node->keys[i] = node->keys[i + 1];
    }
    node->n--;
}

void delete_child(Node *node, int child_index) {
    for (int i = child_index; i <= node->n - 1; i++) {
        node->children[i] = node->children[i + 1];
    }
    node->children[node->n] = NULL;
}

void merge(Node *taker, Node *giver, Node *parent) {
    for (int i = 0; i < giver->n; i++) {
        insert_into_node(taker, giver->keys[i]);
    }
    if (!taker->is_leaf) {
        for (int i = 0; i <= giver->n; i++) {
            taker->children[taker->n + i] = giver->children[i];
            if (giver->children[i]) {
                giver->children[i]->parent = taker;
            }
        }
    }
    if (taker->is_leaf) {
        taker->next = giver->next;
    }
    int giver_index = index_in_parent(giver);
    delete_child(parent, giver_index);
    if (giver_index > 0) {
        delete_key(parent, parent->keys[giver_index - 1]);
    } else {
        delete_key(parent, parent->keys[0]);
    }
    free(giver->keys);
    free(giver->children);
    free(giver);
}

void borrow_keys(Node *lender, Node *borrower, Node *parent,bool borrow_from_right) {
    if (borrow_from_right) {
        int key = lender->keys[0];
        insert_into_node(borrower, key);
        delete_key(lender, key);
        int borrower_index = index_in_parent(borrower);
        parent->keys[borrower_index] = lender->keys[0];
    } else {
        int key = lender->keys[lender->n - 1];
        insert_into_node(borrower, key);
        delete_key(lender, key);
        int lender_index = index_in_parent(lender);
        parent->keys[lender_index] = borrower->keys[0];
    }
}

void propagate_up_deletion(Node *node, int deleted_key, int replace_key) {
    if (node == NULL) return;
    int pos = binary_search(node->keys, node->n, deleted_key);
    if (pos != -1) {
        node->keys[pos] = replace_key;
        propagate_up_deletion(node->parent, deleted_key, replace_key);
    } else {
        if (node->parent) {
            propagate_up_deletion(node->parent, deleted_key, replace_key);
        }
    }
}

bool can_lend(Node *node, int T) {
    if (node->n > (T + 1) / 2) {
        return true;
    }
    return false;
}

void delete(BPT *tree, int key) {
    Node *cursor = tree->root;
    while (!cursor->is_leaf) {
        int i = 0;
        while (i < cursor->n && key >= cursor->keys[i]) i++;
        cursor = cursor->children[i];
    }

    int pos = binary_search(cursor->keys, cursor->n, key);
    if (pos == -1) return;

    if (cursor == tree->root) {
        delete_key(cursor, key);
        return;
    }

    delete_key(cursor, key);

    int min_keys = (tree->T - 1) / 2;
    if (cursor->n >= min_keys) {
        if (pos == 0) {
            propagate_up_deletion(cursor->parent, key, cursor->keys[0]);
        }
        return;
    }

    Node *parent = cursor->parent;
    int cursor_index = index_in_parent(cursor);
    Node *left_sibling = cursor_index > 0 ? parent->children[cursor_index - 1] : NULL;
    Node *right_sibling = cursor_index < parent->n ? parent->children[cursor_index + 1] : NULL;

    if (left_sibling && left_sibling->n > min_keys) {
        borrow_keys(left_sibling, cursor, parent,false);
    } else if (right_sibling && right_sibling->n > min_keys) {
        borrow_keys(right_sibling, cursor, parent,true);
    } else if (left_sibling) {
        merge(left_sibling, cursor, parent);
        cursor = left_sibling;
    } else if (right_sibling) {
        merge(cursor, right_sibling, parent);
    }
    if (parent->n == 0 && parent == tree->root) {
        tree->root = parent->children[0];
        tree->root->parent = NULL;
        free(parent->keys);
        free(parent->children);
        free(parent);
    }
}




