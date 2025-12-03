#pragma once
#include "s3k/s3k.h"



extern int __heap_pointer;
extern int __heap_size;
extern int __heap_metadata_pointer;
extern int __heap_metadata_size;

typedef struct HeapObject{
    bool is_used;
    uint64_t start_pos;
    uint64_t end_pos;
    struct HeapObject* prev;
    struct HeapObject* next;
} HeapObject;

typedef struct MallocMatadata{
    uint64_t number_of_objects;
    // Array of objects of length number_of_objects
    HeapObject objects[0]; 
} MallocMatadata;

void print_malloc_debug_info(char* title);

void s3k_init_malloc();

void* s3k_simple_malloc(uint64_t size);

void* s3k_simple_malloc_random(uint64_t size);

HeapObject* s3k_simple_find_empty_slot(HeapObject* next, uint64_t size, bool forward);

HeapObject* s3k_try_combine(HeapObject* start_object, uint64_t target_size);

void s3k_try_trim_extend(HeapObject* object, uint64_t target_size);

void s3k_simple_free(void* ptr);
