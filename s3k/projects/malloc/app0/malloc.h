#pragma once
#include "altc/altio.h"
#include "s3k/s3k.h"

extern int __heap_pointer;
extern int __heap_size;

typedef struct HeapObject{
    bool is_used;
    uint64_t start_pos;
    uint64_t end_pos;

} HeapObject;

typedef struct MallocHeap{
    HeapObject objects[10];
} MallocHeap;

void s3k_init_malloc();

void* s3k_simple_malloc(uint64_t size);


void s3k_simple_free(void* ptr);

void test();