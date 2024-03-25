IDIR =./include
SRCDIR = ./src
CC=gcc
CFLAGS=-g -Wall -pedantic -std=c89
 
all: hencode hdecode
 
hencode: hencode.o huffman.o functions.o
	${CC} ${CFLAGS} $^ -o $@
 
hdecode: hdecode.o huffman.o functions.o
	${CC} ${CFLAGS} $^ -o $@
 
hencode.o: hencode.c
	${CC} ${CFLAGS} -c $^ -o $@

hdecode.o: hdecode.c
	${CC} ${CFLAGS} -c $^ -o $@

huffman.o: huffman.c
	${CC} ${CFLAGS} -c $^ -o $@

functions.o: functions.c
	${CC} ${CFLAGS} -c $^ -o $@

.PHONY: clean

clean:
	rm -f *.o
