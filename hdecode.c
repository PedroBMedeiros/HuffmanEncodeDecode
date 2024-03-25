#include "./huffman.h"
#include "./functions.h"
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/stat.h>
#include <arpa/inet.h>

#define BUFF_HEADER_SIZE 5 /* amount of chars per entry in header */


int main(int argc, char *argv[]) {
    int fin, fout, i;
    uint32_t charCountEncoded;
    int bytesRead;
    unsigned char buffer[BUFFER_SIZE];
    int tableLength; /* number of unique chars */
    uint32_t *histogram; /* pointer to array to hold histogram of occurrences*/
    
    HuffmanNode* head; /* pointer to head of list of huff nodes*/
    HuffmanNode* root; /* pointer to root of code tree */
    
    /* command line parsing */
    switch(argc) {
        case 1: /* no arguments */
            fin = STDIN_FILENO;
            fout = STDOUT_FILENO;
            break;
        case 2: /* one argument for input */
            if (argv[1][0] == '-') { /* using stdin */
                fin = STDIN_FILENO;
            } else {
                fin = open(argv[1], O_RDONLY);
                if (fin == -1) {
                    perror(argv[1]);
                    return -1;
                }     
            }
            fout = STDOUT_FILENO;
            break;
        case 3: /* both in file and out file */
            if (argv[1][0] == '-') { /* using stdin */
                fin = STDIN_FILENO;
            } else {
                fin = open(argv[1], O_RDONLY);
                if (fin == -1) {
                    perror(argv[1]);
                    return -1;
                }     
            }
            fout = open(argv[2], 
                        O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
            if (fout == -1) {
                perror(argv[2]);
                return -1;
            }
            break;
        default:
            printf("usage hdecode [ ( infile | - ) [ outfile ] ]\n");
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

    /* reading header to build frequency table */
    if ((bytesRead = read(fin, &buffer, 1)) > 0) {
        tableLength = (int)buffer[0] + 1; /* add 1 because of num -1 format*/
    }
    
    /*printf("size of table from header: %d\n", tableLength);*/

    histogram = malloc(sizeof(uint32_t) * ASCII_TABLE_LENGTH);
    if (histogram == NULL) {
        perror("malloc");
        exit(1);
    }
    for (i = 0; i < ASCII_TABLE_LENGTH; i++) {
        histogram[i] = 0;
    }

    charCountEncoded = 0;
    for (i = 0; i< tableLength; i++) {
        /* 1 byte for c; 4 bytes for count of c */
        bytesRead = read(fin, &buffer, BUFF_HEADER_SIZE); 
        if (bytesRead != BUFF_HEADER_SIZE) {
            perror("buffer read");
            exit(1);
        }
        /* converting 4 chars to uint32 reverting significance order*/
        /* shift last char byte 3 times to left, 3rd byte two times, and so on*/
        /* add them all up using logical or. Result will be the value to store 
                                            in occurence table*/
        histogram[(int) buffer[0]] = 
        (buffer[1] << 24) | (buffer[2] << 16) | (buffer[3] << 8) | buffer[4];
        /*   AA       BB       CC       DD
        * AA=00000000 00000000 00000000 aaaaaaaa  (buffer[1])
        * BB=00000000 00000000 00000000 bbbbbbbb  (buffer[2])
        * CC=00000000 00000000 00000000 cccccccc  (buffer[3])
        * DD=00000000 00000000 00000000 dddddddd  (buffer[4])
        * (buffer[1] << 24) | (buffer[2] << 16) | (buffer[3] << 8) | buffer[4]

        * aaaaaaaa 00000000 00000000 00000000 <= 3 bytes (24) shift
        * 00000000 bbbbbbbb 00000000 00000000 <= 2 bytes (16) shift
        * 00000000 00000000 cccccccc 00000000 <= 1 byte (8) shift
        * 00000000 00000000 00000000 dddddddd <= no shift
        * ---------- OR sum -----------------
        * aaaaaaaa bbbbbbbb cccccccc dddddddd */

        /* to know how many characters to decode later */
        charCountEncoded += histogram[(int) buffer[0]]; 
    }

    /* build code tree */
    head = newList(); /* initialize list that holds initial nodes */


    for (i = 0; i < ASCII_TABLE_LENGTH; i++) {
        if (histogram[i] != 0) {
            if (list_insert(&head, i, histogram[i], NULL, NULL) != 0) {
                perror("list insert");
                exit(1);
            }
        }
    }

    root = create_hufftree(&head); /* creating code tree */
    if (root == NULL) {
        perror("tree creation");
        exit(1);
    }
    
    if (traverse_for_characters(root, charCountEncoded, fin, fout) == -1) {
        perror("traversing tree for characters");
        exit(1); 
    }


    close(fin);
    close(fout);

    free(histogram);
    if (traverse_free_memory(root, 0) != 0){
        perror("free tree");
        return -1;
    };
    free(head);

    return 0;
}