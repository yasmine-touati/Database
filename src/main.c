#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../bpt/bpt.h"
#include "../bpt/dfh.h"

void print_file_content(const char* filename) {
    printf("\n=== Content of %s ===\n", filename);
    char buffer[MAX_LINE_SIZE];
    
    // Try to read all possible keys (1-10 in our case)
    for (int i = 1; i <= 10; i++) {
        if (dfh_read_line(filename, i, buffer, sizeof(buffer)) == DFH_SUCCESS) {
            printf("%s", buffer);
        }
    }
    printf("\n");
}

void test_dfh_operations() {
    printf("\n=== Testing DFH Operations ===\n");
    
    // Test data - 8 entries
    const char* csv_lines[] = {
        "1,John Doe,25,New York\n",
        "2,Jane Smith,30,Los Angeles\n",
        "3,Bob Johnson,45,Chicago\n",
        "4,Alice Brown,28,Seattle\n",
        "5,Charlie Davis,35,Boston\n",
        "6,David Wilson,42,Miami\n",
        "7,Eve Martin,33,Denver\n",
        "8,Frank White,39,Phoenix\n",
        "9,Grace Lee,29,Portland\n",
        "10,Henry Ford,36,Detroit\n"
    };
    
    // 1. Create and populate dat1 with 8 entries
    printf("\nCreating dat1 with 8 entries...\n");
    dfh_create_datafile("dat1");
    for (int i = 0; i < 8; i++) {
        if (dfh_write_line("dat1", i + 1, csv_lines[i]) == DFH_SUCCESS) {
            printf("Written key %d to dat1\n", i + 1);
        }
    }
    
    // Read dat1 content
    printf("\nReading dat1 content:\n");
    char buffer[MAX_LINE_SIZE];
    for (int i = 1; i <= 8; i++) {
        if (dfh_read_line("dat1", i, buffer, sizeof(buffer)) == DFH_SUCCESS) {
            printf("%s", buffer);
        }
    }
    
    // 2. Split dat1 - move last 4 entries to dat2
    printf("\nSplitting dat1 - moving keys 5-8 to dat2...\n");
    int keys_to_move[] = {5, 6, 7, 8};
    dfh_create_datafile("dat2");
    if (dfh_move_lines("dat1", "dat2", keys_to_move, 4) == DFH_SUCCESS) {
        printf("Successfully moved entries to dat2\n");
    }
    
    // 3. Create dat3 with entries 9-10
    printf("\nCreating dat3 with entries 9-10...\n");
    dfh_create_datafile("dat3");
    for (int i = 8; i < 10; i++) {
        if (dfh_write_line("dat3", i + 1, csv_lines[i]) == DFH_SUCCESS) {
            printf("Written key %d to dat3\n", i + 1);
        }
    }
    
    // 4. Merge dat1 with dat3
    printf("\nMerging dat1 with dat3...\n");
    if (dfh_merge_files("dat1", "dat3") == DFH_SUCCESS) {
        printf("Successfully merged dat3 into dat1\n");
    }
    
    // Read final content of dat1
    printf("\nReading final content of dat1:\n");
    for (int i = 1; i <= 4; i++) {
        if (dfh_read_line("dat1", i, buffer, sizeof(buffer)) == DFH_SUCCESS) {
            printf("%s", buffer);
        }
    }
    for (int i = 9; i <= 10; i++) {
        if (dfh_read_line("dat1", i, buffer, sizeof(buffer)) == DFH_SUCCESS) {
            printf("%s", buffer);
        }
    }

    printf("\n=== Final State of All Files ===\n");
    print_file_content("dat1");
    print_file_content("dat2");
    print_file_content("dat3");
}

void test_bpt_dfh_integration() {
    printf("\n=== Testing B+ Tree with DFH Integration (T=4) ===\n");
    
    // Test data - 22 entries
    const char* csv_lines[] = {
        "1,John Doe,25,New York\n",
        "2,Jane Smith,30,Los Angeles\n",
        "3,Bob Johnson,45,Chicago\n",
        "4,Alice Brown,28,Seattle\n",
        "5,Charlie Davis,35,Boston\n",
        "6,David Wilson,42,Miami\n",
        "7,Eve Martin,33,Denver\n",
        "8,Frank White,39,Phoenix\n",
        "9,Grace Lee,29,Portland\n",
        "10,Henry Ford,36,Detroit\n",
        "11,Ivy Chen,31,San Francisco\n",
        "12,Jack Black,44,Austin\n",
        "13,Kelly Green,27,Dallas\n",
        "14,Liam Brown,38,Houston\n",
        "15,Mary Jones,32,Atlanta\n",
        "16,Nick Miller,40,Orlando\n",
        "17,Olivia Davis,34,Tampa\n",
        "18,Paul Smith,37,Nashville\n",
        "19,Quinn Adams,26,Memphis\n",
        "20,Rachel King,41,Charlotte\n",
        "21,Sam Wilson,43,Raleigh\n",
        "22,Tom Baker,30,Richmond\n"
    };

    // Create B+ tree
    BPT* tree = create_BPT(4);
    
    // Test 1: Sequential Insertion
    printf("\nTest 1: Sequential Insertion\n");
    for (int i = 0; i < 22; i++) {
        insert(tree, i + 1, csv_lines[i]);
        printf("Inserted key %d\n", i + 1);
        
        // Verify data after each insertion
        Node* leaf = search(tree, i + 1);
        if (leaf) {
            char buffer[MAX_LINE_SIZE];
            if (dfh_read_line(leaf->file_pointer, i + 1, buffer, MAX_LINE_SIZE) == DFH_SUCCESS) {
                printf("Verified data: %s", buffer);
            }
        }
    }
    
    // Print tree structure
    printf("\nTree structure after insertions:\n");
    print_tree(tree->root, 0);

    // Test 2: Data File Distribution
    printf("\nTest 2: Verifying Data File Distribution\n");
    Node* cursor = get_first_leaf_node(tree);
    int leaf_count = 0;
    while (cursor) {
        printf("\nLeaf node %d contents (file: %s):\n", leaf_count++, cursor->file_pointer);
        for (int i = 0; i < cursor->n; i++) {
            char buffer[MAX_LINE_SIZE];
            if (dfh_read_line(cursor->file_pointer, cursor->keys[i], buffer, MAX_LINE_SIZE) == DFH_SUCCESS) {
                printf("Key %d: %s", cursor->keys[i], buffer);
            }
        }
        cursor = cursor->next;
    }

    // Test 3: Deletion and Redistribution
    printf("\nTest 3: Testing Deletion and Redistribution\n");
    int delete_keys[] = {5, 10, 15, 20};
    for (int i = 0; i < 4; i++) {
        printf("\nDeleting key %d\n", delete_keys[i]);
        delete(tree, delete_keys[i]);
        
        // Print tree structure after each deletion
        printf("Tree structure after deletion:\n");
        print_tree(tree->root, 0);
        
        // Verify data distribution
        cursor = get_first_leaf_node(tree);
        while (cursor) {
            printf("\nLeaf node contents after deletion (file: %s):\n", cursor->file_pointer);
            for (int j = 0; j < cursor->n; j++) {
                char buffer[MAX_LINE_SIZE];
                if (dfh_read_line(cursor->file_pointer, cursor->keys[j], buffer, MAX_LINE_SIZE) == DFH_SUCCESS) {
                    printf("Key %d: %s", cursor->keys[j], buffer);
                }
            }
            cursor = cursor->next;
        }
    }

    // Test 4: Range Query
    printf("\nTest 4: Testing Range Query\n");

    // Test different ranges
    struct {
        int low;
        int high;
        const char* description;
    } test_ranges[] = {
        {7, 12, "Range 7-12 (across multiple nodes)"},
        {3, 6, "Range 3-6 (within early nodes)"},
        {16, 19, "Range 16-19 (within later nodes)"}
    };

    for (int test = 0; test < 3; test++) {
        int low = test_ranges[test].low;
        int high = test_ranges[test].high;
        printf("\n=== Testing %s ===\n", test_ranges[test].description);
        
        printf("Searching for nodes containing keys %d to %d...\n", low, high);
        
        // Debug: Print all leaf nodes first
        printf("\nCurrent leaf node distribution:\n");
        Node* cursor = get_first_leaf_node(tree);
        while (cursor) {
            printf("Leaf node (file: %s) contains keys: ", cursor->file_pointer);
            for (int i = 0; i < cursor->n; i++) {
                printf("%d ", cursor->keys[i]);
            }
            printf("\n");
            cursor = cursor->next;
        }

        int low_offset, up_offset;
        char** files = ranged_query(tree, low, high, &low_offset, &up_offset);
        
        printf("\nRange query results:\n");
        printf("Low offset: %d, Up offset: %d\n", low_offset, up_offset);
        
        if (files) {
            int file_count = 0;
            while (files[file_count] != NULL) file_count++;
            printf("Found %d files containing relevant keys\n", file_count);
            
            for (int i = 0; files[i] != NULL; i++) {
                printf("\nReading from file: %s\n", files[i]);
                char buffer[MAX_LINE_SIZE];
                
                // Read all entries in this file
                cursor = get_first_leaf_node(tree);
                while (cursor) {
                    if (strcmp(cursor->file_pointer, files[i]) == 0) {
                        // Found the matching leaf node
                        printf("Found matching leaf node with keys: ");
                        for (int j = 0; j < cursor->n; j++) {
                            printf("%d ", cursor->keys[j]);
                        }
                        printf("\n");
                        
                        for (int j = 0; j < cursor->n; j++) {
                            int key = cursor->keys[j];
                            if (key >= low && key <= high) {
                                if (dfh_read_line(files[i], key, buffer, MAX_LINE_SIZE) == DFH_SUCCESS) {
                                    printf("Key %d: %s", key, buffer);
                                } else {
                                    printf("Failed to read key %d\n", key);
                                }
                            }
                        }
                        break;
                    }
                    cursor = cursor->next;
                }
            }
            
            // Free the result
            for (int i = 0; files[i] != NULL; i++) {
                free(files[i]);
            }
            free(files);
        } else {
            printf("No files found containing keys %d-%d\n", low, high);
        }
    }

    // Test 5: Insert after deletion
    printf("\nTest 5: Testing Insert After Deletion\n");
    const char* new_entries[] = {
        "5,New Charlie,36,Boston\n",
        "10,New Henry,37,Detroit\n",
        "15,New Mary,33,Atlanta\n"
    };
    
    for (int i = 0; i < 3; i++) {
        int key = (i + 1) * 5;
        printf("\nInserting key %d\n", key);
        insert(tree, key, new_entries[i]);
        
        // Verify insertion
        Node* leaf = search(tree, key);
        if (leaf) {
            char buffer[MAX_LINE_SIZE];
            if (dfh_read_line(leaf->file_pointer, key, buffer, MAX_LINE_SIZE) == DFH_SUCCESS) {
                printf("Verified new data: %s", buffer);
            }
        }
        
        // Print tree structure
        printf("Tree structure after insertion:\n");
        print_tree(tree->root, 0);
    }

    printf("\nAll integration tests completed.\n");
}

int main() {
    // Create data directory if it doesn't exist
    #ifdef _WIN32
        system("if not exist data mkdir data");
    #else
        system("mkdir -p data");
    #endif

    test_bpt_dfh_integration();
    return 0;
} 