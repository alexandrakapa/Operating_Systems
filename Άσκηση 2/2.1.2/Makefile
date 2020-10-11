CC = gcc
CFLAGS = Wall
all: 2.2.o proc-common.o tree.o
	$(CC) -$(CFLAGS) 2.2.o proc-common.o tree.o -o 2.2

tree.o: tree.c
	$(CC) -$(CFLAGS) -c tree.c

proc-common.o: proc-common.c
	$(CC) -$(CFLAGS) -c proc-common.c

2.2.o: 2.2.c
	$(CC) -$(CFLAGS) -c 2.2.c
