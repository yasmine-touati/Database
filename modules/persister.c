#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../lib/persister.h"
#include "../lib/bpt.h"
#include "../lib/node.h"
#include "../lib/dfh.h"

cJSON* node_to_json(Node* node) {
    if (!node) return NULL;
    
    cJSON* json_node = cJSON_CreateObject();
    if (!json_node) return NULL;
    
    // Add node properties
    if (!cJSON_AddBoolToObject(json_node, "is_leaf", node->is_leaf) ||
        !cJSON_AddNumberToObject(json_node, "n", node->n)) {
        cJSON_Delete(json_node);
        return NULL;
    }
    
    // Add keys array
    cJSON* keys_array = cJSON_CreateArray();
    if (!keys_array) {
        cJSON_Delete(json_node);
        return NULL;
    }
    
    for (int i = 0; i < node->n; i++) {
        cJSON* key = cJSON_CreateNumber(node->keys[i]);
        if (!key) {
            cJSON_Delete(json_node);
            return NULL;
        }
        cJSON_AddItemToArray(keys_array, key);
    }
    cJSON_AddItemToObject(json_node, "keys", keys_array);
    
    // Add file_pointer for leaf nodes
    if (node->is_leaf) {
        if (!cJSON_AddStringToObject(json_node, "file_pointer", 
            node->file_pointer ? node->file_pointer : "")) {
            cJSON_Delete(json_node);
            return NULL;
        }
    }
    
    // Add children recursively
    if (!node->is_leaf) {
        cJSON* children_array = cJSON_CreateArray();
        if (!children_array) {
            cJSON_Delete(json_node);
            return NULL;
        }
        
        for (int i = 0; i <= node->n; i++) {
            cJSON* child_json = node_to_json(node->children[i]);
            if (!child_json) {
                cJSON_Delete(json_node);
                return NULL;
            }
            cJSON_AddItemToArray(children_array, child_json);
        }
        cJSON_AddItemToObject(json_node, "children", children_array);
    }
    
    return json_node;
}

Node* json_to_node(cJSON* json, Node* parent) {
    if (!json) return NULL;
    
    cJSON* is_leaf_item = cJSON_GetObjectItem(json, "is_leaf");
    cJSON* n_item = cJSON_GetObjectItem(json, "n");
    if (!is_leaf_item || !n_item) return NULL;
    
    bool is_leaf = is_leaf_item->valueint;
    int n = n_item->valueint;
    
    // Create node
    Node* node = create_node(is_leaf, n + 1);
    if (!node) return NULL;
    
    node->n = n;
    node->parent = parent;
    
    // Get keys
    cJSON* keys = cJSON_GetObjectItem(json, "keys");
    if (!keys) {
        free_node(node);
        return NULL;
    }
    
    for (int i = 0; i < n; i++) {
        cJSON* key_item = cJSON_GetArrayItem(keys, i);
        if (!key_item) {
            free_node(node);
            return NULL;
        }
        node->keys[i] = key_item->valueint;
    }
    
    // Get file_pointer for leaf nodes
    if (is_leaf) {
        cJSON* file_pointer = cJSON_GetObjectItem(json, "file_pointer");
        if (file_pointer && file_pointer->valuestring) {
            node->file_pointer = strdup(file_pointer->valuestring);
            if (!node->file_pointer) {
                free_node(node);
                return NULL;
            }
        }
    }
    
    // Get children recursively
    if (!is_leaf) {
        cJSON* children = cJSON_GetObjectItem(json, "children");
        if (!children) {
            free_node(node);
            return NULL;
        }
        
        for (int i = 0; i <= n; i++) {
            node->children[i] = json_to_node(cJSON_GetArrayItem(children, i), node);
            if (!node->children[i]) {
                free_node(node);
                return NULL;
            }
        }
    }
    
    return node;
}

int save_tree_to_json(BPT* tree, const char* filename) {
    if (!tree || !tree->root || !filename) {
        return -1;
    }
    
    cJSON* json_tree = cJSON_CreateObject();
    if (!json_tree) {
        return -1;
    }
    
    if (!cJSON_AddNumberToObject(json_tree, "T", tree->T)) {
        cJSON_Delete(json_tree);
        return -1;
    }
    
    cJSON* root_json = node_to_json(tree->root);
    if (!root_json) {
        cJSON_Delete(json_tree);
        return -1;
    }
    cJSON_AddItemToObject(json_tree, "root", root_json);
    
    char* json_str = cJSON_Print(json_tree);
    if (!json_str) {
        cJSON_Delete(json_tree);
        return -1;
    }
    
    FILE* file = fopen(filename, "wb");
    if (!file) {
        free(json_str);
        cJSON_Delete(json_tree);
        return -1;
    }
    
    size_t len = strlen(json_str);
    size_t written = fwrite(json_str, 1, len, file);
    fclose(file);
    
    free(json_str);
    cJSON_Delete(json_tree);
    
    return (written == len) ? 0 : -1;
}

BPT* load_tree_from_json(const char* filename) {
    if (!filename) return NULL;
    
    FILE* file = fopen(filename, "rb");
    if (!file) return NULL;
    
    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return NULL;
    }
    
    long size = ftell(file);
    if (size == -1) {
        fclose(file);
        return NULL;
    }
    
    if (fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return NULL;
    }
    
    char* json_str = malloc(size + 1);
    if (!json_str) {
        fclose(file);
        return NULL;
    }
    
    size_t read = fread(json_str, 1, size, file);
    fclose(file);
    
    if (read != (size_t)size) {
        free(json_str);
        return NULL;
    }
    
    json_str[size] = '\0';
    
    cJSON* json_tree = cJSON_Parse(json_str);
    free(json_str);
    
    if (!json_tree) return NULL;
    
    cJSON* T_item = cJSON_GetObjectItem(json_tree, "T");
    if (!T_item) {
        cJSON_Delete(json_tree);
        return NULL;
    }
    
    BPT* tree = create_BPT(T_item->valueint);
    if (!tree) {
        cJSON_Delete(json_tree);
        return NULL;
    }
    
    cJSON* json_root = cJSON_GetObjectItem(json_tree, "root");
    if (!json_root) {
        cJSON_Delete(json_tree);
        free(tree);
        return NULL;
    }
    
    tree->root = json_to_node(json_root, NULL);
    if (!tree->root) {
        cJSON_Delete(json_tree);
        free(tree);
        return NULL;
    }
    
    // Reconstruct leaf node links
    Node* cursor = tree->root;
    Node* prev = NULL;
    
    while (!cursor->is_leaf) {
        cursor = cursor->children[0];
    }
    
    while (cursor) {
        if (prev) prev->next = cursor;
        prev = cursor;
        cursor = cursor->next;
    }
    
    cJSON_Delete(json_tree);
    return tree;
}