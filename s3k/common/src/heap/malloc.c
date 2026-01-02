#include "altc/altio.h"
#include "string.h"
#include "heap/malloc.h"
#include "heap/canary.h"
#include "heap/randomize.h"
#include "heap/utils.h"


#define HEAP_OBJECT_MIN_SIZE 16
#define HEAP_OBJECT_MAX_SIZE 512

#define CANARY_SIZE sizeof(((CanaryObject*)0)->canary) 

// Placed on stack for now
static MallocMatadata* s3k_heap;

uint64_t get_num_heap_slots(){
    return s3k_heap->number_of_objects;
}

uint64_t get_heap_object_size(HeapObject obj){
    return (uint64_t)(obj.end_pos - obj.start_pos); 
}

void print_malloc_debug_info_list(char* title){
    alt_printf("%s\n", title);
    HeapObject* curr = &s3k_heap->objects[0];
    while(curr){
        alt_printf("Object pos: 0x%x --> 0x%x, NP: 0x%x\n", curr->start_pos, curr->end_pos, curr->next);
        curr = curr->next;
    }
}

void print_malloc_debug_info(char* title){
    alt_printf("%s\n", title);
    for(int i=0; i<get_num_heap_slots(); ++i){
        int block_size = get_heap_object_size(s3k_heap->objects[i]);
        alt_printf("Object pos: 0x%x --> 0x%x, size: %d, used: %d\n", s3k_heap->objects[i].start_pos, s3k_heap->objects[i].end_pos, block_size, s3k_heap->objects[i].is_used);
    }
}

void s3k_init_malloc(){
    init_random();
    // Set heap to point at
    s3k_heap = (MallocMatadata*)&__heap_metadata_pointer;
    //memset(s3k_heap, 0, sizeof(__heap_metadata_size));
    // Set number of metadata objects

    s3k_heap->number_of_objects = ((uint64_t)(&__heap_metadata_size)-sizeof(s3k_heap->number_of_objects))/sizeof(s3k_heap->objects[0]);
    alt_printf("-------------MALLOC INIT--------------\n");
    alt_printf("| Heap metadata objects: %d\n", s3k_heap->number_of_objects);
    alt_printf("| Heap pointer %x\n", &__heap_pointer);
    alt_printf("| Heap size %x\n", &__heap_size);
    alt_printf("| Heap metadata pointer 0x%x\n", (void*)s3k_heap);
    alt_printf("--------------------------------------\n");

    uint64_t heap_size = (uint64_t)&__heap_size;
    uint64_t heap_start = (uint64_t)&__heap_pointer;
    uint64_t object_size = heap_size / get_num_heap_slots();
    for(uint64_t i=0; i<s3k_heap->number_of_objects; i++){ // TO BE CHANGED
        s3k_heap->objects[i].start_pos = heap_start + i*object_size;
        s3k_heap->objects[i].end_pos = heap_start + (i+1)*object_size;
        s3k_heap->objects[i].is_used = false;
        // Set next and prev pointer of previous object
        if(i>0){
            s3k_heap->objects[i-1].next = &s3k_heap->objects[i];
            s3k_heap->objects[i].prev = &s3k_heap->objects[i-1];
        }
    }

    // set next of last object to null_ptr
    s3k_heap->objects[get_num_heap_slots()-1].next = (void*)0;
    
    // Debug print
    // print_malloc_debug_info("--- Initial Mallov Heap Blocks ---");
}

/*
Function to combine two adjecent blocks if, they 
are both unused and are together at least target_size in size.

Returns the address of the new block, or null_ptr if no blocks were combined
*/
HeapObject* s3k_try_combine(HeapObject* start_object, uint64_t target_size){
    // No need to combine if there is already enough space
    uint64_t first_block_size = get_heap_object_size(*start_object);
    if(first_block_size >= target_size){
        return start_object;
    }

    // If next block exists and is not used, try to combine it with the current one
    if(start_object->next && !start_object->next->is_used){
        HeapObject* next_object = start_object->next;
        
        //Combine the two blocks
        start_object->end_pos = start_object->next->end_pos;
        start_object->next = next_object->next;
        if(next_object->next)
            next_object->next->prev = start_object;

        //Kill the next object
        next_object->is_used = false;
        next_object->start_pos = 0;
        next_object->end_pos = 0;
        next_object->next = (HeapObject*)0;
        next_object->prev = (HeapObject*)0;
        #if MALLOC_DEEP_DEBUG_PRINT
        alt_printf("| COMBINED OBJECT TO SIZE: %d\n", get_heap_object_size(*start_object));
        #endif

        if(get_heap_object_size(*start_object)>=target_size)
            return start_object;
        // Run recursivly untill enough space is found (or it cannot continue)
        return s3k_try_combine(start_object, target_size);
    }
    return (HeapObject*)0;
}

/*
    If the next block is available, try to extend it by decreasing the 
    size of the current object. I.e. [---A---|---B---] ==> [-A-|-----B-----]
    TODO: Insert a new block if next block is used.
*/
void s3k_try_trim_extend(HeapObject* object, uint64_t target_size){
    uint64_t object_size = get_heap_object_size(*object);
    HeapObject* next_object = object->next;
    if (object_size <= target_size || !(next_object))
        return;
    if (object_size / 2 > target_size){
        #if MALLOC_DEEP_DEBUG_PRINT
        alt_printf("\t--- TRIMMING AN OBJEXT ---\n");
        alt_printf("\t| target size: %d\n", target_size);
        alt_printf("\t| object size: %d\n", object_size);
        #endif
        object->end_pos = object->start_pos + target_size;
        next_object->start_pos = object->end_pos;
    }    
}

void* s3k_simple_malloc(uint64_t size){
    size += CANARY_SIZE;

    HeapObject* next = &s3k_heap->objects[0];
    HeapObject* block_to_give = (HeapObject*)0;
    while(next){
        
        if(!next->is_used){
            // If it is free and fits the object, use it
            if(get_heap_object_size(*next) >= size){
                s3k_try_trim_extend(next, size);
                //next->is_used = true;
                block_to_give = next;
                break;
                //return (void*)next->start_pos; 
            }
            // Otherwise, try to combine with next block
            else{
                block_to_give = s3k_try_combine(next, size);
                if(block_to_give) break; //return (void*)combined->start_pos;
            }
        }
        next = next->next;
    }


    if(block_to_give != 0){
        block_to_give->is_used = true;
#if MALLOC_DEBUG_PRINT
        alt_printf("----- Found Heap Block -----\n");
        alt_printf("| sp:   0x%x\n", block_to_give->start_pos);
        alt_printf("| ep:   0x%x\n", block_to_give->end_pos);
        alt_printf("| next: 0x%x\n", block_to_give->next);
        alt_printf("----------------------------\n");
#endif
        add_canary((uint64_t*) (block_to_give->end_pos-CANARY_SIZE));
        return (void*)block_to_give->start_pos;
    }

    return (void*)0;
}

HeapObject* s3k_simple_find_empty_slot(HeapObject* next, uint64_t size, bool forward){
    #if MALLOC_DEEP_DEBUG_PRINT
    alt_printf("---- Finding empty slot of size %d sb 0x%x ----\n", size, next);
    #endif

    HeapObject* find_avalible_block = (HeapObject*)0;
    while(next){
        #if MALLOC_DEEP_DEBUG_PRINT
        alt_printf("| Block pos: 0x%x\n", next);
        alt_printf("| Is used? :   %d\n", next->is_used);
        alt_printf("| Obj size : %d\n", get_heap_object_size(*next));
        #endif
        // (Find first possible entry)

        // skip until nth entry
        
        
        if(!next->is_used){
            // If it is free and fits the object, use it
            if(get_heap_object_size(*next) >= size){
                s3k_try_trim_extend(next, size);
                //next->is_used = true;
                find_avalible_block = next;
                break;
                //return (void*)next->start_pos; 
            }
            // Otherwise, try to combine with next block
            else{
                find_avalible_block = s3k_try_combine(next, size);
                if(find_avalible_block) break; //return (void*)combined->start_pos;
            }
        }
        if (forward) 
            next = next->next;
        else next = next->prev;
    }
    #if MALLOC_DEEP_DEBUG_PRINT
    alt_printf("---------------------------------------------\n");
    #endif
    return find_avalible_block;
}
bool check_memory_is_zeroed(uint64_t size, uint64_t memory_address){    
    //memset((void*)memory_address, 1, size);
    uint8_t *ptr = (uint8_t*)memory_address;
    for (uint64_t i = 0; i<size; i++){
        if(ptr[i] != 0){
            return false;
        }
    }
    return true;
}

int get_num_used_heap_objects(){
    int num_used_blocks = 1;
    HeapObject* b = &s3k_heap->objects[0];
    while(b->next){
        b = b->next;
        num_used_blocks++;
    }
    return num_used_blocks;
}

void* s3k_simple_malloc_random(uint64_t size){ 
    size += CANARY_SIZE;

    int num_used_heap_objects = get_num_used_heap_objects();
    int rnd = next_random_int_v2(num_used_heap_objects);


    HeapObject* next = &s3k_heap->objects[0];
    HeapObject* block_to_give = (HeapObject*)0;
    uint64_t memory_address;
    int count = 1;
    while(next && count++ < rnd){
        next = next->next;
    }
    // Walk forward from rnd
    block_to_give = s3k_simple_find_empty_slot(next, size, true);
    // No empty slots after random. Loop back to 0 and check start
    if (!block_to_give){
        // Walk backwards from rnd
       block_to_give = s3k_simple_find_empty_slot(next, size, false);
    }
    if(block_to_give != 0){
        #if MALLOC_DEBUG_PRINT
        alt_printf("----- Found Heap Block -----\n");
        alt_printf("| sp:   0x%x\n", block_to_give->start_pos);
        alt_printf("| ep:   0x%x\n", block_to_give->end_pos);
        alt_printf("| next: 0x%x\n", block_to_give->next);
        alt_printf("----------------------------\n");
        #endif
        // Check that memory is zeroed before allocating (USE AFTER FREE MITIGATION).
        if (check_memory_is_zeroed(size-CANARY_SIZE, block_to_give->start_pos)){
            block_to_give->is_used = true;
            add_canary((uint64_t*)(block_to_give->end_pos - CANARY_SIZE));
            return (void*)block_to_give->start_pos; 
        }
        else {
            //DO SOMETHING?
            alt_printf("INTEGRITY CHECK FAILED");
        }
    }
    return (void*)0;
}

/* 
Slow and basic implementation of free 
Before freeing, memory is set to 0

*/
void s3k_simple_free(void* ptr){
    uint64_t heap_size = (uint64_t)&__heap_size;
    uint64_t object_size = heap_size / get_num_heap_slots();
    for(int i=0; i<get_num_heap_slots(); i++){
        if(s3k_heap->objects[i].is_used && (void*)s3k_heap->objects[i].start_pos == ptr){
            int size = (s3k_heap->objects[i].end_pos) - s3k_heap->objects[i].start_pos;
            // alt_printf("size: %d\n", size);
            s3k_heap->objects[i].is_used = false;
            remove_canary((uint64_t*)(s3k_heap->objects[i].end_pos-CANARY_SIZE));
            memset(ptr, 0, size);
            return;
        }
    }
}
