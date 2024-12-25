#ifndef PERSISTER_H
#define PERSISTER_H

#include "bpt.h"
#include <cJSON.h>

// Save B+ tree to JSON file
int save_tree_to_json(BPT* tree, const char* filename);

// Load B+ tree from JSON file
BPT* load_tree_from_json(const char* filename);

// Helper functions
cJSON* node_to_json(Node* node);
Node* json_to_node(cJSON* json, Node* parent);

#endif // PERSISTER_H 