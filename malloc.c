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
#define NODE_SIZE ((sizeof(struct Node) - 1) | 15) + 1
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
    header.next = NULL;
    header.prev = prev;
    if (prev) {
        prev->next = &header;
    }
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
        list_head = NULL;
        *global_end = makeHeader(0, NULL, NULL, (intptr_t) addr, 1);
    }

    /* resize our global end node to account for the new size increase */
    global_end->size = global_end->size + CHUNK_SIZE;
    return global_end;
}

void splitFreeNode(struct Node* freeNode, size_t size) {
    struct Node* new = freeNode + size + NODE_SIZE;
    new->size = freeNode->size - size - NODE_SIZE;
    new->free = 1;
    new->next = freeNode->next;
    freeNode->size = size;
    freeNode->free = 0;
    freeNode->next = new;
}

struct Node* findNextFree(size_t size) {
    struct Node* temp = list_head;
    struct Node* new;

    /* Look through list to see our size fits anywhere first */
    while (temp) {
        if (temp->free && temp->size >= size) {
            splitFreeNode(temp, size);
            return temp;
        }
        temp = temp->next;
    }

    /* if the needed size is greater than what we have available 
     * we need to get more space */ 
    while (global_end->size <= size) {
        if (!(global_end = getMoreSpace())) {
            return NULL;
        }
    }

    new = global_end;

    /* now shift the global end to after the new temp node */
    global_end->prev = new;
    global_end->size = global_end->size + size + NODE_SIZE;

    /* now that there is enough space, create a temp to return, with the
     * size passed in, global_end as its "next", global_end's "prev" as its 
     * "prev", its addr as global_end's old addr, and mark it as not free. */
    *new = makeHeader(size, global_end->prev, global_end, global_end->addr, 0);

    return new;
}


void* malloc(size_t size){
    struct Node* head_ptr;

    /* make sure the amount allocated is actually divisible by 16. 
     * If not, round up to align size. */
    if (size == 0) {
        return NULL;
    }
    size = ((size - 1) | 15) + 1;

    /* if global_pointer is NULL, we need to initialize our memory */
    if (!global_end) {
        if (!(global_end = getMoreSpace())){
            return NULL;
        }
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

void free(void* ptr) {
    struct Node* node_ptr;

    /* according to the man page of free:
     * "if ptr is a null pointer, no action occurs. */
    if (ptr == NULL) {
        return;
    }

    /* using the fact that the minus one really means
     * subtract the size of one Node struct */
    node_ptr = (struct Node*) ptr - 1;

    /* TODO: check and see if the data before and after this one
     * is free, to consolidate data */

    /* and set the free bit in that node header to be true */
    node_ptr->free = 1;


    /* TODO: not required, but if the node_ptr is the global_end,
     * consider giving that memory back to the OS and move the global_end
     * to be global_end->prev */
}

void* realloc(void* ptr, size_t size) {
    struct Node* ptr_head;
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
    ptr_head = (struct Node*)ptr - 1;
    if (ptr_head->size >= size) {
        return ptr;
    }

    /* for now, let's just copy everything over to a newly allocated spot
     * with the size we need. */
    result = malloc(size);
    memcpy(result, ptr, ptr_head->size);
    free(ptr);

    return(result);
}

int main(int argc, char* argv[]) {
    return 2;   
}
