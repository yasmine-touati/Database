#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"
#include "bpt.h"
#include "node.h"

// Recursive function to create nodes from JSON
Node* json_to_node(cJSON *node_json, int T) {
    Node *node = create_node(cJSON_GetObjectItem(node_json, "is_leaf")->valueint, T);
    cJSON *keys = cJSON_GetObjectItem(node_json, "keys");
    cJSON *children = cJSON_GetObjectItem(node_json, "children");

    // Set keys
    int size = cJSON_GetArraySize(keys);
    for (int i = 0; i < size; i++) {
        node->keys[i] = cJSON_GetArrayItem(keys, i)->valueint;
    }
    node->n = size;

    // Set file pointer if leaf
    if (node->is_leaf) {
        cJSON *fp = cJSON_GetObjectItem(node_json, "file_pointer");
        if (fp) {
            node->file_pointer = strdup(fp->valuestring);
        }
    } else {
        // Recurse to children
        int child_count = cJSON_GetArraySize(children);
        for (int i = 0; i < child_count; i++) {
            node->children[i] = json_to_node(cJSON_GetArrayItem(children, i), T);
            node->children[i]->parent = node;
        }
    }

    return node;
}

// Main function to reconstruct the B+ tree from JSON
BPT* reconstruct_from_json(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("File open failed");
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    long length = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char *data = (char *)malloc(length + 1);
    fread(data, 1, length, fp);
    fclose(fp);

    cJSON *json = cJSON_Parse(data);
    free(data);

    if (!json) {
        printf("Error parsing JSON\n");
        return NULL;
    }

    int T = cJSON_GetObjectItem(json, "T")->valueint;
    BPT *tree = create_BPT(T);
    tree->root = json_to_node(cJSON_GetObjectItem(json, "root"), T);

    cJSON_Delete(json);
    return tree;
}
