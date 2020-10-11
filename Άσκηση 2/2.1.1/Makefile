CC = gcc
CFLAGS = Wall

all: skeletos.o proc-common.o
	$(CC) -$(CFLAGS) skeletos.o proc-common.o -o skeletos

proc-common.o: proc-common.c
	$(CC) -$(CFLAGS) -c proc-common.c

skeletos.o: skeletos.c
	$(CC) -$(CFLAGS) -c skeletos.c

clean:
	rm fork-tree.o proc-common.o fork-tree
