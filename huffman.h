#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>

#define ASCII_TABLE_LENGTH 256 /* used for size of arrays */
#define MAX_CODE_LENGTH 256 /* theoretical max length of a code */
#define BUFFER_SIZE 1024 /* file read buffer size */


/* huffman node struct type */
typedef struct HuffmanNode {
    int asciiValue; /* 0-255 ascii character */
    int frequency; /* occurrence of characters*/
    struct HuffmanNode *left; /* children of huff nodes*/
    struct HuffmanNode *right; 
    struct HuffmanNode *next; /* pointers for doubly linked lists*/
    struct HuffmanNode *prev;
} HuffmanNode;

bool AprecedesB(HuffmanNode* a, HuffmanNode* b);
bool node_is_leaf(HuffmanNode* node);
int *countOccurrences(int file, int size);
HuffmanNode* newList();
int list_insert(HuffmanNode** head, int ascii, int freq, 
                    HuffmanNode* left, HuffmanNode* right);
HuffmanNode* list_pop(HuffmanNode** head);
int list_size(HuffmanNode* head);
void list_print(HuffmanNode* head);
HuffmanNode* create_hufftree(HuffmanNode** head);
int record_code(int ascii, char** codetable, char* auxString, int n);
int traverse_for_codes(HuffmanNode* root, 
                        char* codetable[], char* auxString, int top);
int traverse_free_memory(HuffmanNode* root, int depth);
int traverse_for_characters(HuffmanNode* root, uint32_t charCountEncoded, 
                            int filein, int fileout);
