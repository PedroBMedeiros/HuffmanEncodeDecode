#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include "./huffman.h"
#include "./functions.h"


/* implements huffman node precedence rule */
bool AprecedesB(HuffmanNode* a, HuffmanNode* b) {
    if (a->frequency == b->frequency) { /* tiebreak case */
        return (a->asciiValue < b->asciiValue); /* lower character wins */
                    /* when a and b identical supernodes, new node goes left */
    } else {
        return (a->frequency < b->frequency);
    }
}

/* check if node at the end of tree */
bool node_is_leaf(HuffmanNode* node) {
    if ((node->left == NULL) && (node->right == NULL)) {
        return true;
    }
    return false;
}

/* build histogram for char frequency from file */
int *countOccurrences(int file, int size) {
    int bytesRead, i;
    unsigned char buffer[BUFFER_SIZE];
    int *array = malloc(sizeof(int) * size);
    for (i = 0; i < size; i++) { /* zero out histogram */
        array[i] = 0;
    }
    while (( bytesRead = read(file, &buffer, BUFFER_SIZE) ) > 0) { 
        for (i =0; i < bytesRead; i++) {
            array[buffer[i]]++;
        }
        
    }
    return array;
}

/* initilization for list of nodes */
HuffmanNode* newList() {
    HuffmanNode* head = NULL;
    return head;
}

/* insert new node in tree with auto sort according to huffman standards */
int list_insert(HuffmanNode** head, int ascii, int freq,
                 HuffmanNode* left, HuffmanNode* right) {
    HuffmanNode* newNode = (HuffmanNode*)malloc(sizeof(HuffmanNode));
    HuffmanNode* current; /* current is used for sorting swaps*/
    newNode->asciiValue = ascii;
    newNode->frequency = freq;
    newNode->left = left;
    newNode->right = right;
    newNode->prev = NULL;
    newNode->next = NULL;
    
    /* case of empty list*/
    if (*head == NULL) {
        *head = newNode;
        return 0;
    }

    current = *head;
    /* positioning new node in list using doubly linked list logic */
    /*incrementing and scanning for precedence*/
    while (current->next != NULL && AprecedesB(current, newNode)) {
        current = current->next;
    }
    /* once precedence rule breaks, insert node in new location */
    if (!AprecedesB(current, newNode)) { 
        /* insert new node in the middle */
        newNode->prev = current->prev;
        newNode->next = current;
        
        if (current->prev != NULL) { /* if not head */
            current->prev->next = newNode;
        } else { /* if head */
            (*head) = newNode;
        }
        current->prev = newNode; /* inserting to the left of reference */
    } else { /* reached end of the list, new node to the end */
        newNode->prev = current;
        newNode->next = NULL;
        current->next = newNode;
    }
    return 0;
}

/* remove node from top of list and returns its pointer */
HuffmanNode* list_pop(HuffmanNode** head) {
    HuffmanNode* top;
    HuffmanNode* next;
    if (*head == NULL) {
        return NULL;
    }
    top = *head;
    next = (*head)->next;
    if (next != NULL) {
        next->prev = NULL;
    }
    *head = next;
    return top;
}

/* utility function to check size of list of nodes */
int list_size(HuffmanNode* head) {
    int count = 0;
    HuffmanNode* current = head;
    while (current != NULL) {
        count++;
        current = current->next;
    }
    return count;
}

/* utility function to echo contents of node in list */
void list_print(HuffmanNode* head) {
  HuffmanNode* current = head;
  while (current != NULL) {
    printf("%d - %#04x('%c') %d\n", current->asciiValue, current->asciiValue,
                                     current->asciiValue, current->frequency);
    current = current->next;
  }
  printf("\n");
}

/* tree creation logic, returns root of tree */
HuffmanNode* create_hufftree(HuffmanNode** head) {
    int ascii;
    int freq;
    HuffmanNode *left, *right;
    if (*head == NULL) {
        return NULL;
    }
    while (list_size(*head) > 1) {
        left = list_pop(head); 
        right = list_pop(head);
        ascii = -1; /* supernode assign value lower than any real 
                        character for tiebreak procedure */
        freq = left->frequency + right->frequency; /* sum of frequencies*/
        list_insert(head, ascii, freq, left, right); /* add node to list */
        /* list_print(*head);*/
    }
    /* last node left in the list is the root of tree */
    return list_pop(head);
}

/* utility function to write temp code string into codeTable */
int record_code(int ascii, char** codetable, char* auxString, int depth) {
    int i;
    char code[256];
    for (i = 0; i<depth; i++) { 
        code[i] = auxString[i]; 
    }
    code[depth] = '\0';
    /* printf("0x%02x: %s\n", ascii, code);*/
    codetable[ascii] = realloc(codetable[ascii], sizeof(char)*(strlen(code)+1));
    if (codetable[ascii] == NULL) {
        perror("realloc");
        return -1;
    }
    strcpy(codetable[ascii], code);
    return 0;
}

/* tree traversal to create huffman code for each character node */
int traverse_for_codes(HuffmanNode* root, char* codetable[], 
                        char* auxString, int depth) {
    /* assign 0 to left and enter recursion */
    if (root->left) {
        auxString[depth] = '0';
        traverse_for_codes(root->left, codetable, auxString, depth+1);
    }
    /* assign 1 to right and enter recursion */
    if (root->right) {
        auxString[depth] = '1';
        traverse_for_codes(root->right, codetable, auxString, depth+1);
    }
    /* if node is leaf record code string in codeTable*/
    if (node_is_leaf(root)) {
        if (record_code(root->asciiValue, codetable, auxString, depth) != 0) {
            perror("record code");
            exit(1);
        }
        free(root); /* huffman node is no longer needed */
    }
    return 0;
}

int traverse_free_memory(HuffmanNode* root, int depth) {
    /* go left and enter recursion */
    if (root->left) {
        traverse_free_memory(root->left, depth+1);
    }
    /* go right and enter recursion */
    if (root->right) {
        traverse_free_memory(root->right, depth+1);
    }
    /* if node is leaf, free memory */
    if (node_is_leaf(root)) {
        free(root); /* huffman node is no longer needed */
    }
    return 0;
}

int traverse_for_characters(HuffmanNode* root, uint32_t charCountEncoded, 
                            int filein, int fileout) {
    uint32_t charCountDecoded = 0;
    int bytesRead, i;
    unsigned char buffer;
    char eightBitString[9];
    HuffmanNode *current;
    current = root;
    
    while (charCountDecoded < charCountEncoded) {
        bytesRead = read(filein, &buffer, 1);
        
        if (bytesRead < 0) {
            perror("problem reading encoded file into buffer");
            return -1;
        }
        if (bytesRead > 0) {
            char_to_8_bit_string(buffer, eightBitString);

            for (i = 0; i < 8; i++) {
                if (eightBitString[i] == '0') {
                    current = current->left;
                } else {
                    current = current->right;
                }
                if (node_is_leaf(current)) {
                    /*printf("%c", current->asciiValue);*/
                    buffer = current->asciiValue;
                    write(fileout, &buffer, sizeof(buffer));
                    charCountDecoded++;
                    if (charCountDecoded >= charCountEncoded) {
                        return 0;
                    }
                    current = root;
                }
            }
        } else { /* corner case for single character file (bytesRead = 0)*/
            buffer = current->asciiValue;
            write(fileout, &buffer, sizeof(buffer));
            charCountDecoded++;
            if (charCountDecoded >= charCountEncoded) {
                return 0;
            }   
        }
        
    }    
    return 0;
}