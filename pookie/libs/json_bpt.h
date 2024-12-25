#ifndef JSON_BPT_H
#define JSON_BPT_H

#include "bpt.h"
#include "cJSON.h"

// Function declarations
Node* json_to_node(cJSON *node_json, int T);
BPT* reconstruct_from_json(const char *filename);

#endif 