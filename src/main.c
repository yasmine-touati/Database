#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef _WIN32
    #include <windows.h>
#else
    #include <dirent.h>
    #include <sys/types.h>
#endif
#include "../lib/bpt.h"
#include "../lib/dfh.h"
#include "../lib/persister.h"



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

    printf("\n=== Testing B+ Tree with DFH Integration (T=30) ===\n");

    

    // Generate 600 test entries

    const int NUM_ENTRIES = 600;

    char** csv_lines = malloc(NUM_ENTRIES * sizeof(char*));

    for (int i = 0; i < NUM_ENTRIES; i++) {

        csv_lines[i] = malloc(MAX_LINE_SIZE);

        snprintf(csv_lines[i], MAX_LINE_SIZE, 

                "%d,Person_%d,%d,City_%d\n", 

                i + 1,    // key

                i + 1,    // name

                20 + (i % 50), // age

                i + 1     // city

        );

    }



    // Create B+ tree with order 30

    BPT* tree = create_BPT(30);

    

    // Test 1: Sequential Insertion

    printf("\nTest 1: Sequential Insertion of 600 entries\n");

    for (int i = 0; i < NUM_ENTRIES; i++) {

        insert(tree, i + 1, csv_lines[i]);

        if ((i + 1) % 50 == 0) {

            printf("Inserted %d entries\n", i + 1);

        }

    }

    

    // Print tree structure summary

    printf("\nTree structure summary:\n");

    print_tree(tree->root, 0);



    // Test 2: Data File Distribution

    printf("\nTest 2: Verifying Data File Distribution\n");

    Node* cursor = get_first_leaf_node(tree);

    int leaf_count = 0;

    int total_keys = 0;

    while (cursor) {

        printf("Leaf node %d (file: %s) contains %d keys\n", 

               leaf_count++, cursor->file_pointer, cursor->n);

        total_keys += cursor->n;

        cursor = cursor->next;

    }

    printf("Total keys across all leaves: %d\n", total_keys);



    // Test 3: Multiple Range Queries

    printf("\nTest 3: Testing Multiple Range Queries\n");

    struct {

        int low;

        int high;

        const char* description;

    } test_ranges[] = {

        {50, 150, "Range 50-150 (early range)"},

        {300, 400, "Range 300-400 (middle range)"},

        {500, 600, "Range 500-600 (end range)"},

        {1, 600, "Range 1-600 (full range)"},

        {275, 325, "Range 275-325 (mid-point range)"}

    };



    for (int test = 0; test < 5; test++) {

        int low = test_ranges[test].low;

        int high = test_ranges[test].high;

        printf("\n=== Testing %s ===\n", test_ranges[test].description);

        

        int low_offset, up_offset;

        char** files = ranged_query(tree, low, high, &low_offset, &up_offset);

        

        if (files) {

            int file_count = 0;

            while (files[file_count] != NULL) file_count++;

            printf("Found %d files containing keys %d-%d\n", file_count, low, high);

            

            // Verify a sample of keys from the range

            int sample_size = 5;

            int step = (high - low + 1) / sample_size;

            printf("\nVerifying sample keys:\n");

            for (int i = 0; i < file_count; i++) {

                for (int key = low + (step * i); key <= high && key <= low + (step * (i + 1)); key += step) {

                    char buffer[MAX_LINE_SIZE];

                    if (dfh_read_line(files[i], key, buffer, MAX_LINE_SIZE) == DFH_SUCCESS) {

                        printf("Key %d: %s", key, buffer);

                    }

                }

            }

            

            // Free the result

            for (int i = 0; files[i] != NULL; i++) {

                free(files[i]);

            }

            free(files);

        }



      



    }



    // Clean up

    for (int i = 0; i < NUM_ENTRIES; i++) {

        free(csv_lines[i]);

    }



    // add deletion test for random deletion

    printf("\n=== Testing Deletion of 50 entries ===\n");

    int  j = 0;

    while (j < 50) {

        int random_key = rand() % NUM_ENTRIES + 1;

        printf("Deleting key %d\n", random_key);

        delete(tree, random_key);

        j++;

    }



    // print tree structure summary

    printf("\nTree structure summary after deletion:\n");

    print_tree(tree->root, 0);





   



    free(csv_lines);



    printf("\nAll integration tests completed.\n");

}



void test_bpt_deletion() {

    printf("\n=== Testing B+ Tree Deletion Operations (T=30) ===\n");

    

    // Create tree with 500 entries

    const int NUM_ENTRIES = 500;

    BPT* tree = create_BPT(30);

    

    // Insert entries sequentially

    printf("\nInserting %d entries...\n", NUM_ENTRIES);

    for (int i = 1; i <= NUM_ENTRIES; i++) {

        char line[MAX_LINE_SIZE];

        snprintf(line, MAX_LINE_SIZE, "%d,Person_%d,%d,City_%d\n", 

                i, i, 20 + (i % 50), i);

        insert(tree, i, line);

        if (i % 50 == 0) {

            printf("Inserted %d entries\n", i);

        }

    }

    

    printf("\nInitial tree structure:\n");

    print_tree(tree->root, 0);

    

    // Print initial data file distribution

    printf("\nInitial data file distribution:\n");

    Node* cursor = get_first_leaf_node(tree);

    while (cursor) {

        printf("\nLeaf node file: %s contains keys: ", cursor->file_pointer);

        for (int i = 0; i < cursor->n; i++) {

            printf("%d ", cursor->keys[i]);

        }

        printf("\n");

        cursor = cursor->next;

    }



    // Test case 1: Sequential deletion from middle

    printf("\nTest 1: Sequential deletion from middle (keys 240-260)\n");

    for (int i = 240; i <= 260; i++) {

        printf("\nDeleting key %d\n", i);

        delete(tree, i);

    }

    

    printf("\nTree structure after middle deletion:\n");

    print_tree(tree->root, 0);

    

    // Test case 2: Delete sequence causing multiple borrows

    printf("\nTest 2: Delete sequence causing multiple borrows (keys 100-120)\n");

    for (int i = 100; i <= 120; i++) {

        printf("\nDeleting key %d\n", i);

        delete(tree, i);

    }

    

    printf("\nTree structure after borrow operations:\n");

    print_tree(tree->root, 0);

    

    // Test case 3: Delete sequence causing multiple merges

    printf("\nTest 3: Delete sequence causing multiple merges (keys 300-320)\n");

    for (int i = 300; i <= 320; i++) {

        printf("\nDeleting key %d\n", i);

        delete(tree, i);

    }

    

    printf("\nTree structure after merge operations:\n");

    print_tree(tree->root, 0);

    

    // Test case 4: Random deletions

    printf("\nTest 4: Random deletions (50 keys)\n");

    srand(time(NULL));

    for (int i = 0; i < 50; i++) {

        int key = rand() % NUM_ENTRIES + 1;

        printf("\nDeleting key %d\n", key);

        delete(tree, key);

    }

    

    // Final verification

    printf("\nFinal tree structure:\n");

    print_tree(tree->root, 0);

    

    printf("\nFinal data file verification:\n");

    cursor = get_first_leaf_node(tree);

    while (cursor) {

        printf("\nLeaf node file: %s\n", cursor->file_pointer);

        printf("Keys in node: ");

        for (int i = 0; i < cursor->n; i++) {

            printf("%d ", cursor->keys[i]);

        }

        printf("\nVerifying data file contents:\n");

        for (int i = 0; i < cursor->n; i++) {

            char buffer[MAX_LINE_SIZE];

            if (dfh_read_line(cursor->file_pointer, cursor->keys[i], buffer, MAX_LINE_SIZE) == DFH_SUCCESS) {

                printf("Key %d: %s", cursor->keys[i], buffer);

            } else {

                printf("ERROR: Key %d not found in data file!\n", cursor->keys[i]);

            }

        }

        cursor = cursor->next;

    }

    

    printf("\nDeletion tests completed.\n");

}



void test_persistence() {

    printf("\nTesting B+ Tree Persistence\n");

    

    // Create and populate a tree

    BPT* tree = create_BPT(4);

    for (int i = 1; i <= 10; i++) {

        char line[MAX_LINE_SIZE];

        snprintf(line, MAX_LINE_SIZE, "%d,Person_%d,%d,City_%d\n", i, i, 20 + i, i);

        insert(tree, i, line);

    }

    

    // Save tree to JSON

    printf("Saving tree to index.json...\n");

    if (save_tree_to_json(tree, "index.json") == 0) {

        printf("Tree saved successfully\n");

    }

    

    // Load tree from JSON

    printf("Loading tree from index.json...\n");

    BPT* loaded_tree = load_tree_from_json("index.json");

    if (loaded_tree) {

        printf("Tree loaded successfully\n");

        print_tree(loaded_tree->root, 0);

    }

}



void test_bpt_persistence() {

    printf("\n=== Testing B+ Tree Persistence (T=50) ===\n");

    

    // Create initial tree

    printf("\n1. Creating and populating initial tree with 500 entries\n");

    BPT* tree = create_BPT(50);

    if (!tree) {

        printf("Failed to create tree\n");

        return;

    }



    // Insert entries

    for (int i = 1; i <= 500; i++) {

        char line[MAX_LINE_SIZE];

        snprintf(line, MAX_LINE_SIZE, "%d,Person_%d,%d,City_%d\n", 

                i, i, 20 + (i % 50), i);

        insert(tree, i, line);

        if (i % 50 == 0) {

            printf("Inserted %d entries\n", i);

        }

    }

    

    printf("\nInitial tree structure:\n");

    print_tree(tree->root, 0);

    

    // Save tree to JSON

    printf("\n2. Saving tree to index.json...\n");

    if (save_tree_to_json(tree, "index.json") == 0) {

        printf("Tree saved successfully\n");

    } else {

        printf("Failed to save tree\n");

        free(tree);

        return;

    }

    

    // Load tree from JSON

    printf("\n3. Loading tree from index.json...\n");

    BPT* loaded_tree = load_tree_from_json("index.json");

    if (!loaded_tree) {

        printf("Failed to load tree\n");

        free(tree);

        return;

    }

    

    printf("Tree loaded successfully\n");

    printf("\nLoaded tree structure:\n");

    print_tree(loaded_tree->root, 0);

    

    // Verify data integrity

    printf("\n4. Verifying data integrity...\n");

    bool data_valid = true;

    for (int i = 1; i <= 500; i++) {

        Node* node = search(loaded_tree, i);

        if (!node) {

            printf("Error: Key %d not found in tree\n", i);

            data_valid = false;

            continue;

        }

        

        char buffer[MAX_LINE_SIZE];

        if (dfh_read_line(node->file_pointer, i, buffer, MAX_LINE_SIZE) != DFH_SUCCESS) {

            printf("Error: Could not read data for key %d\n", i);

            data_valid = false;

            continue;

        }

        

        char expected[MAX_LINE_SIZE];

        snprintf(expected, MAX_LINE_SIZE, "%d,Person_%d,%d,City_%d\n", 

                i, i, 20 + (i % 50), i);

        

        if (strcmp(buffer, expected) != 0) {

            printf("Error: Data mismatch for key %d\n", i);

            printf("Expected: %s", expected);

            printf("Got: %s", buffer);

            data_valid = false;

        }

        

        if (i % 100 == 0) {

            printf("Verified %d entries\n", i);

        }

    }

    

    if (data_valid) {

        printf("\nAll data verified successfully!\n");

    } else {

        printf("\nData verification failed!\n");

    }

    

    // Test some operations on loaded tree

    printf("\n5. Testing operations on loaded tree...\n");

    

    // Delete some entries

    printf("Deleting entries 100, 200, 300...\n");

    delete(loaded_tree, 100);

    delete(loaded_tree, 200);

    delete(loaded_tree, 300);

    

    // Insert some new entries

    printf("Inserting new entries 501, 502, 503...\n");

    for (int i = 501; i <= 503; i++) {

        char line[MAX_LINE_SIZE];

        snprintf(line, MAX_LINE_SIZE, "%d,Person_%d,%d,City_%d\n", 

                i, i, 20 + (i % 50), i);

        insert(loaded_tree, i, line);

    }

    

    printf("\nFinal tree structure:\n");

    print_tree(loaded_tree->root, 0);

    

    // Cleanup

    if (tree) free(tree);

    if (loaded_tree) free(loaded_tree);

    

    printf("\nPersistence tests completed.\n");

}



void cleanup_empty_datafiles() {
    printf("\n=== Cleaning up empty data files ===\n");
    
    #ifdef _WIN32
        WIN32_FIND_DATA findData;
        HANDLE hFind;
        char search_path[256] = "data\\*.dat";
        
        hFind = FindFirstFile(search_path, &findData);
        if (hFind == INVALID_HANDLE_VALUE) {
            printf("No data files found\n");
            return;
        }
        
        do {
            char filepath[256];
            snprintf(filepath, sizeof(filepath), "data\\%s", findData.cFileName);
            
            FILE* file = fopen(filepath, "r");
            if (file) {
                // Check if file is empty
                fseek(file, 0, SEEK_END);
                long size = ftell(file);
                fclose(file);
                
                if (size == 0) {
                    if (remove(filepath) == 0) {
                        printf("Removed empty file: %s\n", findData.cFileName);
                    } else {
                        printf("Failed to remove empty file: %s\n", findData.cFileName);
                    }
                }
            }
        } while (FindNextFile(hFind, &findData));
        
        FindClose(hFind);
    #else
        DIR *dir;
        struct dirent *ent;
        
        dir = opendir("data");
        if (dir == NULL) {
            printf("Could not open data directory\n");
            return;
        }
        
        while ((ent = readdir(dir)) != NULL) {
            if (strstr(ent->d_name, ".dat") != NULL) {
                char filepath[256];
                snprintf(filepath, sizeof(filepath), "data/%s", ent->d_name);
                
                FILE* file = fopen(filepath, "r");
                if (file) {
                    // Check if file is empty
                    fseek(file, 0, SEEK_END);
                    long size = ftell(file);
                    fclose(file);
                    
                    if (size == 0) {
                        if (remove(filepath) == 0) {
                            printf("Removed empty file: %s\n", ent->d_name);
                        } else {
                            printf("Failed to remove empty file: %s\n", ent->d_name);
                        }
                    }
                }
            }
        }
        closedir(dir);
    #endif
}



int main() {

    #ifdef _WIN32

        system("if not exist data mkdir data");

    #else

        system("mkdir -p data");

    #endif



    test_bpt_persistence();
    
    // Add cleanup at the end
    cleanup_empty_datafiles();
    
    return 0;

}
