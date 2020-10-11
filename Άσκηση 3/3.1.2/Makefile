CC = gcc
CFLAGS = -Wall -O2 -pthread

all: mandel

mandel: mandel.o mandel-lib.o
	$(CC) $(CFLAGS) -o mandel mandel.o mandel-lib.o

mandel.o: mandel.c
	$(CC) $(CFLAGS) -c -o mandel.o mandel.c

mandel-lib.o: mandel-lib.c
	$(CC) $(CFLAGS) -c -o mandel-lib.o mandel-lib.c
