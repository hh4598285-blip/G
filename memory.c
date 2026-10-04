#include <stdint.h>
#include <stddef.h>

#define HEAP_START 0x00400000u
#define HEAP_SIZE  0x00100000u

typedef struct block {
    uint32_t size;
    uint8_t free;
    struct block* next;
} block_t;

static block_t* first = (block_t*)(uintptr_t)HEAP_START;
static int ready;

void memory_init(void){
    first->size=HEAP_SIZE-(uint32_t)sizeof(block_t);
    first->free=1;
    first->next=0;
    ready=1;
}

static void split(block_t*b,uint32_t size){
    if(b->size < size + sizeof(block_t) + 16)return;
    block_t*n=(block_t*)((uint8_t*)b+sizeof(block_t)+size);
    n->size=b->size-size-sizeof(block_t);
    n->free=1;
    n->next=b->next;
    b->size=size;
    b->next=n;
}

void* gos_malloc(size_t size){
    if(!ready||!size)return 0;
    size=(size+7u)&~7u;
    for(block_t*b=first;b;b=b->next){
        if(b->free&&b->size>=size){split(b,(uint32_t)size);b->free=0;return (uint8_t*)b+sizeof(block_t);}
    }
    return 0;
}

static void merge(void){
    for(block_t*b=first;b&&b->next;b=b->next){
        if(b->free&&b->next->free){b->size+=sizeof(block_t)+b->next->size;b->next=b->next->next;}
    }
}

void gos_free(void*p){
    if(!p)return;
    block_t*b=(block_t*)((uint8_t*)p-sizeof(block_t));
    b->free=1;
    merge();
}

uint32_t memory_free_bytes(void){
    uint32_t total=0;
    for(block_t*b=first;b;b=b->next)if(b->free)total+=b->size;
    return total;
}
