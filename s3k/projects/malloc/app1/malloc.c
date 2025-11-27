#include "malloc.h"


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

void print_malloc_debug_info(char* title){
    alt_printf("%s\n", title);
    for(int i=0; i<get_num_heap_slots(); ++i){
        alt_printf("Object pos: 0x%x --> 0x%x, NP: 0x%x\n", s3k_heap->objects[i].start_pos, s3k_heap->objects[i].end_pos, s3k_heap->objects[i].next);
    }
}

void s3k_init_malloc(){

    init_random();

    // Set heap to point at
    s3k_heap = (MallocMatadata*)&__heap_metadata_pointer;
    //memset(s3k_heap, 0, sizeof(__heap_metadata_size));
    // Set number of metadata objects

    s3k_heap->number_of_objects = ((uint64_t)(&__heap_metadata_size)-sizeof(s3k_heap->number_of_objects))/sizeof(s3k_heap->objects[0]);
    alt_printf("Heap metadata objects: %d\n", s3k_heap->number_of_objects);
    alt_printf("Heap pointer %x\n", &__heap_pointer);
    alt_printf("Heap size %x\n", &__heap_size);
    alt_printf("Heap size %x\n", &__heap_size);
    alt_printf("Heap metadat pointer 0x%x\n", (void*)s3k_heap);
    uint64_t heap_size = (uint64_t)&__heap_size;
    uint64_t heap_start = (uint64_t)&__heap_pointer;
    uint64_t object_size = heap_size / get_num_heap_slots();
    alt_printf("Object size: %d\n", object_size);
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
    print_malloc_debug_info("--- Initial Mallov Heap Blocks ---");
}

/*
Function to combine two adjecent blocks if, they 
are both unused and are together at least target_size in size.

Returns the address of the new block, or null_ptr if no blocks were combined
*/
HeapObject* s3k_try_combine(HeapObject* start_object, uint64_t target_size){
    // If next block exists and is not used, try to combine it with the current one
    if(start_object->next && !start_object->next->is_used){
        HeapObject* next_object = start_object->next;
        uint64_t first_block_size = get_heap_object_size(*start_object);
        uint64_t second_block_size = get_heap_object_size(*start_object->next);
        
        // Later on we might want this to be recursive, e.g. 4 adjecent blocks are combined
        if(first_block_size + second_block_size < target_size){
            return (HeapObject*)0;
        }

        //Combine the two blocks
        start_object->end_pos = start_object->next->end_pos;
        start_object->next = next_object->next;

        //Kill the next object
        next_object->is_used = false;
        next_object->start_pos = 0;
        next_object->end_pos = 0;

        return start_object;
    }
    return (HeapObject*)0;
}

/*
    If the next block is available, try to extend it by decreasing the 
    size of the current object. I.e. [---A---|---B---] ==> [-A-|-----B-----]
*/
void s3k_try_trim_extend(HeapObject* object, uint64_t target_size){
    uint64_t object_size = get_heap_object_size(*object);
    alt_printf("Object size: %d\n", object_size);
    alt_printf("Object pos: 0x%x\n", object);
    HeapObject* next_object = object->next;
    if (object_size <= target_size || !(next_object))
        return;
    if (object_size / 2 > target_size){
        object->end_pos = object->start_pos + target_size;
        next_object->start_pos = object->end_pos;
    }    
}

void* s3k_simple_malloc(uint64_t size){
    size += CANARY_SIZE;
    if (size > (uint64_t)&__heap_size / get_num_heap_slots()){
        (void*)0;
    }

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
    /*
    for(int i = 0; i < get_num_heap_slots(); i++){
        if (!s3k_heap.objects[i].is_used){
            s3k_heap.objects[i].is_used = true;
            return (void*)s3k_heap.objects[i].start_pos;
        }
    }
    */
    if(block_to_give != 0){
        block_to_give->is_used = true;
        add_canary((uint64_t*) (block_to_give->end_pos-CANARY_SIZE));
        return (void*)block_to_give->start_pos;
    }
    return (void*)0;
}

HeapObject* s3k_simple_find_empty_slot(HeapObject* next, uint64_t size, bool forward){
    HeapObject* find_avalible_block = (HeapObject*)0;
    while(next){

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
    return find_avalible_block;
}

void* s3k_simple_malloc_random(uint64_t size){
    size += CANARY_SIZE;
    if (size > (uint64_t)&__heap_size / get_num_heap_slots()){
        (void*)0;
    }
    int rnd = next_random_int_v2(get_num_heap_slots());
    alt_printf("Random start for heap: %d\n", rnd);

    HeapObject* next = &s3k_heap->objects[0];
    HeapObject* block_to_give = (HeapObject*)0;

    int count = 1;
    while(next && count++ < rnd){
        next = next->next;
    }
    // Walk forward from rnd
    block_to_give = s3k_simple_find_empty_slot(next, size, true);
    alt_printf("Block to give: %d", block_to_give);
    // No empty slots after random. Loop back to 0 and check start
    if (!block_to_give){
        // Walk backwards from rnd
       block_to_give = s3k_simple_find_empty_slot(next, size, false); 
    }
    if(block_to_give != 0){
        block_to_give->is_used = true;
        add_canary((uint64_t*) (block_to_give->end_pos-CANARY_SIZE));
        return (void*)block_to_give->start_pos;
    }
    return (void*)0;
}

/* Slow and basic implementation of free */
void s3k_simple_free(void* ptr){
    for(int i=0; i<get_num_heap_slots(); i++){
        uint64_t heap_size = (uint64_t)&__heap_size;
        uint64_t object_size = heap_size / get_num_heap_slots();
        if((void*)s3k_heap->objects[i].start_pos == ptr){
            s3k_heap->objects[i].is_used = false;
            remove_canary((uint64_t*)(s3k_heap->objects[i].end_pos-CANARY_SIZE));
            return;
        }
    }        
}
