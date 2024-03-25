#include "./huffman.h"
#include "./functions.h"
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/stat.h>
#include <arpa/inet.h>


#define CHAR_LENGTH 8 /* number of bits in a char*/
#define WORKING_STRING_LENGTH 1024 /* size of block to hold 0s and 1s being
                                                         converted*/

/* utility function for writing body by converting strings to unsigned chars */
int write_body(int fout, const char codeBits[]) {
    int i, codeBitsloc, k;
    int bits = strlen(codeBits); /* bit counter */
    int bytes = bits/CHAR_LENGTH; /* byte counter */
    unsigned char buffer = 0;
    codeBitsloc = 0;
    k = 0;
    
    while (k < bytes) {         
        for (i = 0; i < CHAR_LENGTH; i++) { /* jump 8 at a time */
            if (codeBits[codeBitsloc] == '1') {
                buffer = buffer | 1 << (7 - i); 
                /* buffer starts with 00000000 then
                gets 1s in the corresponding position by adding 00001000 (ex.)
                with bit level OR addition*/
            }
            codeBitsloc++;
        }
        if (write(fout, &buffer, sizeof(unsigned char)) == -1){
            perror("body writing");
            return -1;
        }
        buffer = 0;
        k++;
    }    
    return 0; 
}

int main(int argc, char *argv[]) {
    int fin, fout, i, bytesRead, j, padCount, bytesWritten;
    int *histogram; /* pointer array to hold histogram of occurences */
    HuffmanNode* head; /* pointer to head of list of huffman nodes */
    HuffmanNode* root; /* pointer to root of code tree */
    char *codeTable[ASCII_TABLE_LENGTH]; /* array to hold translation table*/

    uint8_t charNum; /* number of unique characters minus 1*/
    uint8_t headerC; /* 1 byte for each unique character in header file */
    uint32_t headerCcount; /* 4 bytes to hold each character's frequency */
    off_t offset; /* offset for lseek */
    unsigned char buffer[BUFFER_SIZE]; /* reading buffer */
    char codeString[WORKING_STRING_LENGTH]; /* working string for conversion*/

    /* working variables to build code table*/
    int depth; 
    char aux_string[MAX_CODE_LENGTH];
    depth = 0;
    aux_string[0] = '\0';

    /* command line parsing */
    switch(argc) {
        case 1: /* no arguments */
            printf("usage hencode infile [ outfile ]\n");
            return -1;
            break;
        case 2: /* in file only */
            fin = open(argv[1], O_RDONLY);
            if (fin == -1) {
                perror(argv[1]);
                return -1;
            }
            fout = STDOUT_FILENO;
            break;
        case 3: /* both in file and out file */
            fin = open(argv[1], O_RDONLY);
            if (fin == -1) {
                perror(argv[1]);
                return -1;
            }
            fout = open(argv[2], 
                        O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
            if (fout == -1) {
                perror(argv[2]);
                return -1;
            }
            break;
        default:
            printf("usage hencode infile [ outfile ]\n");
            return -1;
            break;
    }

    /* testing for empty file */
    if (fin != STDIN_FILENO) {
        if (file_is_empty(fin) == 0) {
            if (fin != -1 && fout != -1) {
                close(fin);
                close(fout);
            }
            return 0;
        }
    }

    /* building histogram of character occurrences */
    histogram = countOccurrences(fin, ASCII_TABLE_LENGTH);

    head = newList(); /* initialize list that holds initial nodes */

    for (i = 0; i < ASCII_TABLE_LENGTH; i++) {
        if (histogram[i] != 0) {
            if (list_insert(&head, i, histogram[i], NULL, NULL) != 0){
                perror("list insert");
                exit(1);
            }
        }
    }

    charNum = list_size(head) - 1; /* to be used for header */

    root = create_hufftree(&head); /* creating code tree */
    if(root == NULL) {
        perror("tree creation");
        exit(1);
    }

    /* initializing code table with pointers to NULL */
    for (i = 0; i < ASCII_TABLE_LENGTH; i++) {
        codeTable[i] = malloc(sizeof(char));
        codeTable[i] = NULL;
    }

    /* traversing tree to write codes in codeTable */
    if (traverse_for_codes(root, codeTable, aux_string, depth) != 0) {
        perror("traversal");
        exit(1);
    }
    
    /* printing final htable to visualize steps */
    /* for (i = 0; i < ASCII_TABLE_LENGTH; i++) {
        if (codeTable[i] != NULL){
            printf("0x%02x '%c': %s\n", i, i, codeTable[i]);
        }
    } */
    
    /* writing header */
    bytesWritten = write(fout, &charNum, sizeof(charNum));
    if (bytesWritten == -1) {
        perror("header write");
        return -1;
    }
    
    for (i = 0; i < ASCII_TABLE_LENGTH; i++) {
        if (histogram[i] != 0){
            headerC = i;
            headerCcount = htonl(histogram[i]);
            bytesWritten = write(fout, &headerC, sizeof(headerC));
            if (bytesWritten == -1) {
                perror("header write");
                return -1;
            }
            bytesWritten = write(fout, &headerCcount, sizeof(headerCcount));
            if (bytesWritten == -1) {
                perror("header write");
                return -1;
            }
        }
    }    

    offset = lseek(fin, 0, SEEK_SET); /* seek beginning of file for re-read */
    if (offset == -1) {
        perror("lseek");
        return -1;    
    }

    /* writing body */
    codeString[0] = '\0';
    padCount = 0;
    while (( bytesRead = read(fin, &buffer, BUFFER_SIZE) ) != 0) {
        if (bytesRead == -1) {
            perror("read buffer");
            return -1;
        }
        for (i=0; i<bytesRead; i++) {
            strcat(codeString, codeTable[buffer[i]]);
            /* case of last byte of final buffer*/
            
            if (strlen(codeString)%CHAR_LENGTH == 0) { /* everytime the string 
                        length is a multiple of 8 it can be converted*/
                if (write_body(fout, codeString) == -1) {
                    perror("write body");
                    return -1;
                }
                codeString[0] = '\0';
            }
        }
    }
    /* case of last byte of last buffer */
    if ((bytesRead == 0) && (strlen(codeString) > 0)) { 
        /* calculate amount of padding */
        padCount = (CHAR_LENGTH - 
                    (strlen(codeString) % CHAR_LENGTH)) % CHAR_LENGTH; 
        for (j = 0; j < padCount; j++) {
            strcat(codeString, "0");
        }
        if(write_body(fout, codeString) == -1){
            perror("write padding");
            return -1;
        }
        codeString[0] = '\0';
    }

    /* close files */
    close(fin);
    close(fout);

    /* frees */
    free(histogram);

    for(i=0; i < ASCII_TABLE_LENGTH; i++) {
        free(codeTable[i]);
    }

    /* huffman nodes freed up inside traversal routine */

    return 0;
}