/* 
 * header.c
 * A data structure and function to be used by my implementation of malloc
 * Written by Michael Georgariou for CPE 453
 * April 2020 
 */

#define CHUNK_SIZE 64000
#define NODE_SIZE sizeof(struct Node)

struct Node {
    uintptr_t addr;
    size_t size;
    int free;
    struct Node* next;
}

/* creates a node. If prev = NULL, this is the first node */
struct Node makeHeader(size_t size, struct Node* prev, uintptr_t addr, int free) {
    struct Node header;
    header.addr = addr + NODE_SIZE;
    header.size = size;
    header.free = free;
    header.next = NULL;
    if (prev) {
        prev->next = &header;
    }
    return header;
}

struct Node* findNextFree(size_t size) {
    struct Node* temp = global_end;

    /* iterate through the linked list until the following conditions met:
     * - we have not hit the end of the list
     * - the current node is free and has a big enough size */
    while (temp && !((temp->free) && (temp->size >= size))) {
        temp = temp->next;
    }
    return temp;
}

struct Node* getMoreSpace(struct Node* end, size_t size) {
    /* put our new node right where the memory ends */
    struct Node* new_node = sbrk(0);

    /* request more space for this new block
     * TODO: call sbrk with CHUNK_SIZE instead of size + NODE_SIZE
     * to avoid calling it every time we call malloc */
    if (sbrk(size + NODE_SIZE) == (void*) -1) {
        return NULL;
    }

    /* put the data needed into our new header node and return it */
    *new_node = (size, end, new_node, 0);
    return new_node;
}