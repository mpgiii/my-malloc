#ifndef HEADER_H
#define HEADER_H

#include <stddef.h>
#include <stdint.h>
#include <unistd.h>

#define CHUNK_SIZE 64000
#define NODE_SIZE sizeof(struct Node) + (sizeof(struct Node) % 16)

struct Node {
    uintptr_t addr;
    size_t size;
    int free;
    struct Node* next;
    struct Node* prev;
};

extern void* global_start;

struct Node makeHeader(size_t size, struct Node* prev, uintptr_t addr, 
                       int free);

struct Node* findNextFree();

struct Node* getMoreSpace(struct Node* end, size_t size);

#endif
