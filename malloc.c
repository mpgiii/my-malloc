/* 
 * malloc.c
 * Written by Michael Georgariou for CPE 453
 * April 2020 
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>

#define CHUNK_SIZE 64000
#define NODE_SIZE (((sizeof(struct Node) - 1) | 15) + 1)
#define MAX_DEBUG_LEN 64

struct Node {
    uintptr_t addr;
    size_t size;
    int free;
    struct Node* next;
    struct Node* prev;
};

/* creates a node. If prev = NULL, this is the first node */
struct Node makeHeader(size_t size, struct Node* prev, struct Node* next, 
                       uintptr_t addr, int free) {
    struct Node header;
    header.addr = addr + NODE_SIZE;
    header.size = size;
    header.free = free;
    header.next = next;
    header.prev = prev;
    return header;
}

/* global start point to make sure each call to malloc keeps track of where our
 * list is (should always be free) */
struct Node* global_end = NULL;
struct Node* list_head = NULL;

struct Node* getMoreSpace() {
    void* addr;

    /* request more space for our list */
    if ((addr = sbrk(CHUNK_SIZE)) == (void*) -1) {
        errno = ENOMEM;
        return NULL;
    }

    /* if global_end is NULL, we need to initialize it. */
    if (!global_end) {
        global_end = addr;
        *global_end = makeHeader((size_t)CHUNK_SIZE - NODE_SIZE, NULL, 
                                 NULL, (intptr_t) addr, 1);
    }

    /* if global_end is already initialized, add CHUNK_SIZE to its size to
     * keep track of how much space we have available */
    else {
        global_end->size = global_end->size + CHUNK_SIZE;
    }
    return global_end;
}


/* changes freeNode's size to the given size, and leaves everything
 * above it as a new free node 
 * returns the new free node */
struct Node* splitFreeNode(struct Node* freeNode, size_t size) {
    struct Node* new = (uintptr_t)freeNode + NODE_SIZE + size;
    *new = makeHeader(freeNode->size - size - NODE_SIZE, freeNode,
                      freeNode->next, freeNode->addr + size, 1);
    freeNode->next = new;
    freeNode->size = size;
    freeNode->free = 0;
    return new;
}

struct Node* findNextFree(size_t size) {
    struct Node* temp = list_head;

    /* Look through list to see our size fits anywhere first */
    while (temp && temp != global_end) {
        /* if we find a node that can fit this, use it to avoid leaking
         * memory. */
        if (temp->free && temp->size >= size + NODE_SIZE) {
            splitFreeNode(temp, size);
            return temp;
        }
        temp = temp->next;
    }

    /* if we got here, it means we found NO nodes that can fit this data.
     * to fix this, keep increasing the size of the global end point until
     * it has enough room to store this data */
    while (global_end->size <= size + NODE_SIZE) {
        if (!(global_end = getMoreSpace())) {
            return NULL;
        }
    }

    /* now global_end has enough space. let's chop off the part of it that
     * we want to give to the user and move the global_end to after that now */
    global_end = splitFreeNode(global_end, size);

    return global_end->prev;
}


void* malloc(size_t size){
    struct Node* head_ptr;

    /* make sure the amount allocated is actually divisible by 16. 
     * If not, round up to align size. */
    if (size == 0) {
        return NULL;
    }
    size = ((size - 1) | 15) + 1;

    /* if global_pointer is NULL, we need to initialize our list */
    if (!global_end) {
        if (!(global_end = getMoreSpace())){
            return NULL;
        }
        list_head = global_end;
    }

    /* set the head pointer to the next free node.
     * if findNextFree returns NULL, we know getMoreSpace failed. */
    if (!(head_ptr = findNextFree(size))) {
        return NULL;
    }

    /* debug stuff */
    if (getenv("DEBUG_MALLOC")) {
        char debug[MAX_DEBUG_LEN];
        snprintf(debug, MAX_DEBUG_LEN, "MALLOC: malloc(%d)      =>  "
                 "(ptr=%p, size=%d)\n", size, head_ptr->addr, head_ptr->size);
    }

    /* return the newly allocated memory's address. */
    return head_ptr->addr;
}

void* calloc(size_t nmemb, size_t size) {
    void* result;

    /* first, run malloc to allocate nmemb number of items at size size each */
    result = malloc(nmemb * size);

    /* now, set all memory in that allocated slot to be zero. */
    memset(result, 0, nmemb * size);

    /* that's all! pretty easy I think */
    return result;
}

void consolidateFree(struct Node* node_ptr) {
    if (!node_ptr) {
        return;
    }

    node_ptr->free = 1;

    if (node_ptr->next && node_ptr->next->free) {
        node_ptr->size = node_ptr->size + NODE_SIZE + node_ptr->next->size;
        node_ptr->next = node_ptr->next->next;
    }

    if (node_ptr->prev && node_ptr->prev->free) {
        node_ptr->prev->size = node_ptr->prev->size + NODE_SIZE + 
                               node_ptr->size;
        node_ptr->prev->next = node_ptr->next;
    }

}

/* get the node pointer of a certain pointer (in case
 * the user gives us something in the middle of two heads..
 * for some reason) */
struct Node* getNodePtr(intptr_t ptr) {
    struct Node* node_ptr = list_head;

    /* to find which of the malloc'd nodes we'd like to free, we need
     * to go through our linked list to find which node contains the
     * pointer the user passed. */
    while (node_ptr) {
        /* if we have hit the address exactly, or found it is between the
         * current node's address and the next, we know to free this node. */
        if (node_ptr->addr == ptr || (node_ptr->addr < ptr &&
                                      node_ptr->next && 
                                      node_ptr->next->addr > ptr)) {
            break;
        }
        node_ptr = node_ptr->next;
    }
    return node_ptr;
}

void free(void* ptr) {
    struct Node* node_ptr = list_head;

    intptr_t intptr = (intptr_t) ptr;

    /* according to the man page of free:
     * "if ptr is a null pointer, no action occurs. */
    if (ptr == NULL) {
        return;
    }

    /* find the node which contains this ptr */
    node_ptr = getNodePtr(intptr);

    /* if we got all the way to the end of our list, we know something is up.
     * the user must have passed a ptr that it did not malloc. This is an
     * error. TODO: actually report this error instead of just doing nothin */
    if (!node_ptr) {
        return;
    }

    /* check and see if the data before and after this one
     * is free, to consolidate nodes.
     * NOTE: also sets the node_ptr's free bit to 1. */
    consolidateFree(node_ptr);

    /* TODO: not required, but if the node_ptr is the global_end,
     * consider giving that memory back to the OS and move the global_end
     * to be global_end->prev */
}

void* realloc(void* ptr, size_t size) {
    struct Node* node_ptr = list_head;
    intptr_t intptr = (intptr_t) ptr;
    void* result;

    /* if realloc is called with NULL, it is just malloc. */
    if (ptr == NULL) {
        return malloc(size);
    }

    /* if realloc is called with size 0 then we are just calling free. */
    if (size == 0) {
        free(ptr);
        return ptr;
    }
    /* that ^ should handle our "special" cases */


    /* first off, if we try to realloc and the size of that block is already
     * big enough, we don't need to do anything, for now
     * TODO: make realloc shrink in this case to avoid memory leaks */

    node_ptr = getNodePtr(intptr);

    /* if its null, the user did something stupid! We should probably report
     * this... TODO. */
    if (!node_ptr) {
        return NULL;
    }

    /* if the node already has enough space... eh, just leave it for now.
     * TODO: resize, and make a new free node above it */
    if (node_ptr->size >= size) {
        return ptr;
    }

    /* for now, let's just copy everything over to a newly allocated spot
     * with the size we need. */
    result = malloc(size);
    memcpy(result, (void*)node_ptr->addr, node_ptr->size);
    free(ptr);

    return(result);
}

int main(int argc, char* argv[]) {
    int* hello;
    int i;
    for (i=0; i<8192; i++) {
        hello = malloc(i);
    }
    return 1;
}
