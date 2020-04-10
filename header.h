#ifndef HEADER_H
#define HEADER_H

#include <stddef.h>
#include <stdint.h>
#include <unistd.h>

#define CHUNK_SIZE 64000
#define NODE_SIZE sizeof(struct Node)

struct Node {
    uintptr_t addr;
    size_t size;
    int free;
    struct Node* next;
};

extern void* global_end;

struct Node makeHeader(size_t size, struct Node* prev, uintptr_t addr, 
                       int free);

struct Node* findNextFree(size_t size);

struct Node* getMoreSpace(struct Node* end, size_t size);

#endif
