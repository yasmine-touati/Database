#include <stdio.h>
#include <stdlib.h>
#include "../bpt/bpt.h"

int main() {
    // Create a B+ tree with order 4 (T=4)
    BPT* tree = create_BPT(4);

    // Insert some test values
    int test_values[] = {5, 8, 1, 3, 9, 6, 4, 2, 7};
    int num_values = sizeof(test_values) / sizeof(test_values[0]);

    printf("Inserting values: ");
    for (int i = 0; i < num_values; i++) {
        printf("%d ", test_values[i]);
        insert(tree, test_values[i]);
    }
    printf("\n\n");

    // Print the tree structure
    printf("B+ Tree structure:\n");
    print_tree(tree->root, 0);
    printf("\n");

    // Test search
    int search_key = 6;
    Node* result = search(tree, search_key);
    if (result) {
        printf("Found key %d in tree\n", search_key);
    } else {
        printf("Key %d not found in tree\n", search_key);
    }

    // Test deletion
    int delete_key = 5;
    printf("\nDeleting key %d\n", delete_key);
    delete(tree, delete_key);
    printf("\nTree after deletion:\n");
    print_tree(tree->root, 0);

    // Clean up (you should add proper cleanup functions)
    free(tree);

    return 0;
} 