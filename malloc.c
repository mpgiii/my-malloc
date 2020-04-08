#define CHUNK_SIZE 64000
#define MAX_DEBUG_LEN 64
#define NODE_SIZE sizeof(struct Node)

struct Node {
    uintptr_t addr;
    size_t size;
    int free;
    struct Node* next;
}

/* creates a node. If prev = NULL, this is the first node */
struct Node makeHeader(size_t size, struct Node* prev, uintpt_t addr, int free) {
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

void* global_header = NULL;

struct Node* findNextFree(size_t size) {
    struct Node* current = global_header;

    /* iterate through the linked list until the following conditions met:
     * - we have not hit the end of the list
     * - the current node is free and has a big enough enough */
    while (current && !((current->free) && (current->size >= size))) {
        current = current->next;
    }
    return current;
}

struct Node* getMoreSpace(struct Node* end, size_t size) {
    /* put our new node right where the memory ends */
    struct Node* new_node = sbrk(0);

    /* request more space for this new block
     * TODO: call sbrk with CHUNK_SIZE instead of size + NODE_SIZE
     * to avoi calling it every time we call malloc */
    if (sbrk(size + NODE_SIZE) == (void*) -1) {
        return NULL;
    }

    new_node->addr = new_node + NODE_SIZE;
    new_node->size = size;
    new_node->free = 0;
    new_node->next = NULL;
    if (end) {
        end->next = new_node;
    }
}

void* calloc(size_t nmemb, size_t size) {
    return NULL;
}

void* malloc(size_t size){
    struct Node* head_ptr;
    struct Node* end;

    /* make sure the amount allocated is actually divisible by 16. 
     * If not, round up to align size. */
    if (size <= 0) {
        return NULL;
    }
    size = size + (size % 16);

    /* if the global header is not yet defined, this is the first call to 
     * malloc. set up appropriately */
    if (global_header == NULL) {
        /* sets up head pointer. if this fails, return NULL */
        if (NULL == (head_ptr = getMoreSpace(NULL, size))) {
            return NULL;
        }
    }

    else {
        /* set the head pointer to the next free node */
        head_ptr = findNextFree(size);
        /* if findNextFree returns NULL, we know we need more space */
        if (head_ptr == NULL) {
            head_ptr = getMoreSpace(
/* TODO TODO TODO: I think getMoreSpace is broken. Look into that. */
        }
        /* to get to this else statement, head_ptr represents free block */
        else {
            head_ptr->free = 0;
        }   
    }

    if (getenv("DEBUG_MALLOC")) {
        char debug[MAX_DEBUG_LEN];
        snprintf(debug, MAX_DEBUG_LEN, "MALLOC: malloc(%d)      =>  "
                 "(ptr=%p, size=%d)\n", size, head_ptr->addr, head_ptr->size);
    }

    /* return the newly allocated memory.
     * NOTE: the +1 means adding the size of a Node struct, thus giving
     * the actual address a user can use. Thanks for reminding me of
     * this, Professor Nico! :) */
    return block+1;
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
    node_ptr->free = 1;
}

void* realloc(void* ptr, size_t size) {
    return NULL;
}

int main(int argc, char* argv[]) {
    
}
