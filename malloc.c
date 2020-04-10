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

#define CHUNK_SIZE 64000
#define NODE_SIZE sizeof(struct Node) + (sizeof(struct Node) % 16)
#define MAX_DEBUG_LEN 64

/* creates a node. If prev = NULL, this is the first node */
struct Node makeHeader(size_t size, struct Node* prev, uintptr_t addr, 
                       int free) {
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
 * list is */
void* global_start = sbrk(0);

struct Node* findNextFree() {
    struct Node* temp = global_start;

    /* iterate through the linked list until the following conditions met:
     * - we have not hit the end of the list
     * - the current node is free*/
    while (temp && !i(temp->free)) {
        temp = temp->next;
    }
    return temp;
}

struct Node* getMoreSpace() {
    /* put our new node right where the memory ends */
    struct Node* new_node = sbrk(0);

    /* request more space for this new block
     * TODO: call sbrk with CHUNK_SIZE instead of size + NODE_SIZE
     * to avoid calling it every time we call malloc */
    if (sbrk(CHUNK_SIZE) == (void*) -1) {
        return NULL;
    }

    /* put the data needed into our new header node and return it */
    *new_node = makeHeader(CHUNK_SIZE, global_start, (uintptr_t)new_node, 0);
    return new_node;

}

void* malloc(size_t size){
    struct Node* head_ptr;

    /* make sure the amount allocated is actually divisible by 16. 
     * If not, round up to align size. */
    if (size == 0) {
        return NULL;
    }
    size = size + (size % 16);

    /* set the head pointer to the next free node */
    head_ptr = findNextFree();
    /* if findNextFree returns NULL, we know we need more space */
    if (head_ptr == NULL ) {
        if (!(head_ptr = getMoreSpace())) {
            return NULL;
        }
/* TODO TODO TODO: I think getMoreSpace is broken. Look into that. */
    }
    /* to get to this else statement, head_ptr represents free block */
    else {
        head_ptr->free = 0;
    }   

    /* set the global end to this new spot on the linked list we just made */
    global_end = head_ptr;

    /* debug stuff */
    if (getenv("DEBUG_MALLOC")) {
        char debug[MAX_DEBUG_LEN];
        snprintf(debug, MAX_DEBUG_LEN, "MALLOC: malloc(%d)      =>  "
                 "(ptr=%p, size=%d)\n", size, head_ptr->addr, head_ptr->size);
    }

    /* return the newly allocated memory.
     * NOTE: the +1 means adding the size of a Node struct, thus giving
     * the actual address a user can use. Thanks for reminding me of
     * this, Professor Nico! :) */
    return head_ptr+1;
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

    /* again, using the fact that the minus one really means
     * subtract the size of one Node struct */
    node_ptr = (struct Node*)ptr - 1;

    /* and set the free bit in that node header to be true */
    node_ptr->free = 1;
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
        free(0);
        return ptr;
    }
    /* that ^ should handle our "special" cases */


    /* first off, if we try to realloc and the size of that block is already
     * big enough, we don't need to do anything, for now
     * TODO: make realloc shrink in this case */
    ptr_head = (struct Node*)ptr - 1;
    if (ptr_head->size >= size) {
        return ptr;
    }

    /* */
    result = malloc(size);

    return(result);
}

int main(int argc, char* argv[]) {
    return 2;   
}
