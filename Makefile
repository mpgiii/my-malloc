CC = gcc
CFLAGS = -Wall -pedantic -g3 -fpic
MAIN = malloc

all : $(MAIN)

.PHONY: malloc
malloc: intel-all

intel-all: lib/libmalloc.so lib64/libmalloc.so

lib/libmalloc.a: lib malloc32.o
	ar r lib/libmalloc.a malloc32.o

lib64/libmalloc.a: lib64 malloc64.o
	ar r lib64/libmalloc.a malloc64.o

lib/libmalloc.so: lib malloc32.o
	$(CC) $(CFLAGS) -m32 -shared -o $@ malloc32.o

lib64/libmalloc.so: lib64 malloc64.o
	$(CC) $(CFLAGS) -shared -o $@ malloc64.o

lib:
	mkdir lib

lib64:
	mkdir lib64

malloc32.o: malloc.c
	$(CC) $(CFLAGS) -m32 -c -o malloc32.o malloc.c

malloc64.o: malloc.c
	$(CC) $(CFLAGS) -m64 -c -o malloc64.o malloc.c

clean :
	rm -f *.o
