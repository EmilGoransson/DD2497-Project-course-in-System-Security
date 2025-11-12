#include "malloc.h"

// Placed on stack for now
static MallocHeap s3k_heap;

int get_num_heap_slots(){
    return sizeof(s3k_heap.objects)/sizeof(HeapObject);
}

void s3k_init_malloc(){
    alt_printf("Heap pointer %x\n", &__heap_pointer);
    alt_printf("Heap size %x\n", &__heap_size);
    uint64_t heap_size = (uint64_t)&__heap_size;
    uint64_t heap_start = (uint64_t)&__heap_pointer;
    for(uint64_t i=0; i<get_num_heap_slots(); i++){
        uint64_t object_size = heap_size / get_num_heap_slots();
        s3k_heap.objects[i].start_pos = heap_start + i*object_size;
        s3k_heap.objects[i].end_pos = heap_start + (i+1)*object_size;
        s3k_heap.objects[i].is_used = false;
        alt_printf("Object pos: 0x%x --> 0x%x\nJ", s3k_heap.objects[i].start_pos, s3k_heap.objects[i].end_pos);
    }
}

// 

void* s3k_simple_malloc(uint64_t size){
    if (size > (uint64_t)&__heap_size / get_num_heap_slots()){
        (void*)0;
    }
    for(int i = 0; i < get_num_heap_slots(); i++){
        if (!s3k_heap.objects[i].is_used){
            s3k_heap.objects[i].is_used = true;
            return (void*)s3k_heap.objects[i].start_pos;
        }
    }
    return (void*)0;
}

/* Slow and basic implementation of free */
void s3k_simple_free(void* ptr){
    for(int i=0; i<get_num_heap_slots(); i++){
        uint64_t heap_size = (uint64_t)&__heap_size;
        uint64_t object_size = heap_size / get_num_heap_slots();
        if((void*)s3k_heap.objects[i].start_pos == ptr){
            s3k_heap.objects[i].is_used = false;
            return;
        }
    }        
}

void test(){
    alt_printf("RUNNING TEST FUNCTION\n");
}