#include <stdio.h>
#include "bpt.h"
#include "json_bpt.h"
#include <stdbool.h>

int main() {
    printf("Starting program...\n");
    
    // Try to open and read the JSON file
    printf("Attempting to read tree.json...\n");
    BPT *tree = reconstruct_from_json("tree.json");
    
    if (tree) {
        printf("\nB+ Tree successfully reconstructed from JSON.\n");
        printf("Tree properties: T = %d\n", tree->T);
        printf("\nTree structure:\n");
        printf("----------------\n");
        print_tree(tree->root, 0);  // Visualize the reconstructed tree
    } else {
        printf("Failed to reconstruct B+ Tree. Check if tree.json exists and is readable.\n");
    }
    
    printf("\nProgram finished.\n");
    return 0;
}
