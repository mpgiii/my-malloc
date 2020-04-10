/* 
 * header.c
 * A data structure and function to be used by my implementation of malloc
 * Written by Michael Georgariou for CPE 453
 * April 2020 
 */

#include "header.h"

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
