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
/* NODE_SIZE rounds to the nearest 16. That's what that math is */
#define NODE_SIZE (((sizeof(struct Node) - 1) | 15) + 1)
#define MAX_DEBUG_LEN 128

#define TRUE 1
#define FALSE 0

/* flag to know when to print debug info */
int in_malloc = TRUE;

struct Node {
    uintptr_t addr;
    size_t size;
    int free;
    struct Node* next;
    struct Node* prev;
};

/* creates a node. If prev = NULL, this is the first node */
struct Node makeHeader(size_t size, struct Node* prev, struct Node* next, 
                       uintptr_t node_addr, int free) {
    struct Node header;
    header.addr = node_addr + NODE_SIZE;
    header.size = size;
    header.free = free;
    header.next = next;
    header.prev = prev;
    return header;
}

/* global start and end point to make sure each call to malloc keeps track
 * of where our list is */
struct Node* global_end = NULL;
struct Node* list_head = NULL;

/* increases the size of global_end's free datai by CHUNK_SIZE */
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
    struct Node* new = (struct Node*)((uintptr_t)freeNode + NODE_SIZE + size);
    *new = makeHeader(freeNode->size - size - NODE_SIZE, freeNode,
                      freeNode->next, freeNode->addr + size, TRUE);
    freeNode->next = new;
    freeNode->size = size;
    freeNode->free = FALSE;
    return new;
}

/* goes through the list and finds the next free node that has a size
 * of at least the passed in size. 
 * only returns NULL if getMoreSpace() failed */
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

void debugMalloc(size_t in_size, void* out_ptr, size_t out_size) {
    char debug[MAX_DEBUG_LEN];

    if (in_malloc && getenv("DEBUG_MALLOC")) {
        snprintf(debug, MAX_DEBUG_LEN, "MALLOC: cmalloc(%d)       =>  "
                 "(ptr=%p, size=%d)\n", (int)in_size, out_ptr, (int)out_size); 
        puts(debug);
    }
}


void* malloc(size_t size){
    struct Node* head_ptr;

    /* make sure the amount allocated is actually divisible by 16. 
     * If not, round up to align size. */
    if (size == 0) {
        debugMalloc(size, NULL, size);
        return NULL;
    }
    size = ((size - 1) | 15) + 1;

    /* if global_end is NULL, we need to initialize our list */
    if (!global_end) {
        if (!(global_end = getMoreSpace())){
            debugMalloc(size, NULL, 0);
            return NULL;
        }
        list_head = global_end;
    }

    /* set the head pointer to the next free node.
     * if findNextFree returns NULL, we know getMoreSpace failed. */
    if (!(head_ptr = findNextFree(size))) {
        debugMalloc(size, NULL, 0);
        return NULL;
    }

    debugMalloc(size, (void*)head_ptr->addr, head_ptr->size);
    /* return the newly allocated memory's address. */
    return (void*)head_ptr->addr;
}

void* calloc(size_t nmemb, size_t size) {
    void* result;
    char debug[MAX_DEBUG_LEN];

    /* make sure not to print debug stuff we don't want! */
    in_malloc = FALSE;

    /* first, run malloc to allocate nmemb number of items at size size each */
    result = malloc(nmemb * size);

    /* now, set all memory in that allocated slot to be zero. */
    memset(result, 0, nmemb * size);

    in_malloc = TRUE;

    if (getenv("DEBUG_MALLOC")) {
        snprintf(debug, MAX_DEBUG_LEN, "MALLOC: calloc(%d, %d)      =>  "
                 "(ptr=%p, size=%d)\n", (int)nmemb, (int)size, result, 
                 (int)size);
        puts(debug);
    }


    /* that's all! pretty easy I think */
    return result;
}

/* pass in a newly freed node -- if the node on either side of it is
 * also free, turn it into one big happy node. */
void consolidateFree(struct Node* node_ptr) {
    if (!node_ptr) {
        return;
    }

    if (node_ptr->next && node_ptr->next->free) {
        node_ptr->size = node_ptr->size + NODE_SIZE + node_ptr->next->size;
        /* if we are consolidating with the global_end, make sure to change
         * the global pointer to reflect that. */
        if (node_ptr->next == global_end) {
            global_end = node_ptr;
        }
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

    /* to find which of the malloc'd nodes we're looking for, we need
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

void freeDebug(void* ptr) {
    char debug[MAX_DEBUG_LEN];
    if (getenv("DEBUG_MALLOC")) {
        snprintf(debug, MAX_DEBUG_LEN, "MALLOC: free(%p)\n", ptr);
        puts(debug);
    }
}

void free(void* ptr) {
    struct Node* node_ptr = list_head;

    intptr_t intptr = (intptr_t) ptr;

    /* according to the man page of free:
     * "if ptr is a null pointer, no action occurs. */
    if (ptr == NULL) {
        /* no debug printout here, because when snprintf is called it
         * will call free(NULL); */
        return;
    }

    /* find the node which contains this ptr */
    node_ptr = getNodePtr(intptr);

    /* if we got all the way to the end of our list, we know something is up.
     * the user must have passed a ptr that it did not malloc. This is an
     * error. */
    if (!node_ptr) {
        freeDebug(ptr);
        perror("bad pointer");
        return;
    }

    /* set the node to be free */
    node_ptr->free = TRUE;

    /* check and see if the data before and after this one
     * is free, to consolidate nodes. */
    consolidateFree(node_ptr);

    /* TODO: not required, but if the node_ptr is the global_end,
     * consider giving that memory back to the OS and move the global_end
     * to be global_end->prev */

    freeDebug(ptr);
}

void reallocDebug(void* in_ptr, size_t in_size, void* out_ptr, 
                  size_t out_size) {
    char debug[MAX_DEBUG_LEN];
    if (getenv("DEBUG_MALLOC")) {
        snprintf(debug, MAX_DEBUG_LEN, "MALLOC: realloc(%p, %d)     =>  "
                 "(ptr=%p, size=%d)\n", in_ptr, (int)in_size, out_ptr,
                 (int)out_size);
        puts(debug);
    }
    in_malloc = TRUE;
}

void* realloc(void* ptr, size_t size) {
    struct Node* node_ptr = list_head;
    intptr_t intptr = (intptr_t) ptr;
    void* result;

    /* don't wanna print debug stuff twice! */
    in_malloc = FALSE;

    /* if realloc is called with NULL, it is just malloc. */
    if (ptr == NULL) {
        result = malloc(size);
        reallocDebug(NULL, size, result, size);
        return result;
    }

    /* if realloc is called with size 0 then we are just calling free. */
    if (size == 0) {
        free(ptr);
        reallocDebug(ptr, 0, ptr, 0);
        return ptr;
    }
    /* that ^ should handle our "special" cases */

    /* align size to be a multiple of 16 */
    size = ((size - 1) | 15) + 1;

    /* get the actual node this ptr is stored in */
    node_ptr = getNodePtr(intptr);

    /* if its null, the user did something stupid! We should probably report
     * this... TODO. */
    if (!node_ptr) {
        reallocDebug(ptr, size, NULL, 0);
        return NULL;
    }

    /* if the node already has enough space... hey, just leave it there! */
    if (node_ptr->size >= size) {
        /* if the spot has enough size to make a new free node above it,
         * do that, so we can utilize this otherwise leaked memory */
        if (node_ptr->size >= size + NODE_SIZE) {
            splitFreeNode(node_ptr, size);
        }

        /* return that node's address, because we didn't have to move it. */
        reallocDebug(ptr, size, (void*)node_ptr->addr, node_ptr->size);
        return (void*)node_ptr->addr;
    }

    /* if the node doesn't have enough space, but the node above it is free,
     * we should try and expand the node it currently is in. */
    if (node_ptr->next && node_ptr->next->free) {
        if (node_ptr->size + NODE_SIZE + node_ptr->next->size >= size) {
            /* combine the node and the node above it */
            node_ptr->size = node_ptr->size + NODE_SIZE + node_ptr->next->size;
            /* if we are consolidating with the global_end, make sure to change
             * the global pointer to reflect that. */
            if (node_ptr->next == global_end) {
                global_end = node_ptr;
            }
            node_ptr->next = node_ptr->next->next;

            /* if this new node is way too big (i.e. can fit another node above
             * it after this) we should make a new free node above it, so we
             * can avoid leaking too much memory */
            if (node_ptr->size >= size + NODE_SIZE) {
                splitFreeNode(node_ptr, size);
            }

            /* return it! we have consolidated data and not leaked memory.
             * mom would be so proud. */
            reallocDebug(ptr, size, (void*)node_ptr->addr, node_ptr->size);
            return (void*)node_ptr->addr;
        }
    }

    /* if it's too big to fit currently, let's just copy everything 
     * over to a newly allocated spot with the size we need, and free the
     * old spot. */

    /* NOTE: This is where the code currently fails under heavy testing..
     * I think? However, I cannot see what's going wrong -- I was fairly
     * confident I was handling this correctly. */
    result = malloc(size);
    memcpy(result, (void*)node_ptr->addr, node_ptr->size);
    free(ptr);

    reallocDebug(ptr, size, result, size);
    return result;
}
