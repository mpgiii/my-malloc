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
    header.addr = addr;
    header.size = size;
    header.free = free;
    header.next = NULL;
    if (prev) {
        prev->next = &header;
    }
    return header;
}


void* calloc(size_t nmemb, size_t size) {
    return NULL;
}

void* malloc(size_t size){
    Node* head_ptr;

    /* make sure the amount allocated is actually divisible by 16. 
     * If not, round up */
    size = size + (size % 16);

    if (getenv("DEBUG_MALLOC")) {
        char debug[MAX_DEBUG_LEN];
        snprintf(debug, MAX_DEBUG_LEN, "MALLOC: malloc(%d)      =>  "
                 "(ptr=%p, size=%d)\n", size, head_ptr->addr, head_ptr->size);
    }
    return NULL;
}

void free(void* ptr) {
    return;
}

void* realloc(void* ptr, size_t size) {
    return NULL;
}

int main(int argc, char* argv[]) {
    
}
