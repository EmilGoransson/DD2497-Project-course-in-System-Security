#include <string.h>
#include <stdatomic.h>
#include "heap/canary.h"
#include "heap/randomize.h"
#include "heap/utils.h"


extern int __canary_metadata_pointer;

__attribute__((section(".canary_metadata"), used))
CanaryTable canarytable;

static int active_canaries = 0;


/*
Initiate canarytable. Slots with nullptr count as unused
*/
void init_canary_table(){
    //Change the current canaries to -1 as a starting value
    int i = 0;
    while(i != CANARY_TABLE_ENTRIES) {
        canarytable.entries[i].canary = -1;
        canarytable.entries[i].heap_canary_pointer = 0;
        i++;
    }
#if CANARY_DEBUG_PRINT
    alt_printf("-------------CANARY INIT--------------\n");
    alt_printf("| Canary pointer: %x\n", &__canary_metadata_pointer);
    alt_printf("| Canary objects: %d\n", CANARY_TABLE_ENTRIES);
    alt_printf("--------------------------------------\n");
#endif
}

/*
Add canary to Canary metadata

Used by add_canary
*/

__attribute__((section(".text.critical_func"), used, noinline)) 
int internal_add_canary(CanaryObject canary){
    int free_index = next_random_int_v2(CANARY_TABLE_ENTRIES);
    for (int i = 0; i < CANARY_TABLE_ENTRIES; i++){
        if(canarytable.entries[free_index].heap_canary_pointer == 0){
            break;
        }
        free_index = (free_index+1)%CANARY_TABLE_ENTRIES;
        if(i==CANARY_TABLE_ENTRIES-1){
            alt_printf("ERROR: Failed to find a free canary slot\n");

            return 1;
        }
    }

#if CANARY_DEBUG_PRINT
    alt_printf("-------------Adding canary--------------\n");
    alt_printf("| Pointer:      0x%x\n", canary.heap_canary_pointer);
    alt_printf("| Value:        0x%x\n", canary.canary);
    alt_printf("| Metadata pos: 0x%x\n", &canarytable->entries[free_index]);
    alt_printf("| --- Previous Data At Pos ---\n");
    alt_printf("| Pointer:      0x%x\n", canarytable->entries[free_index].heap_canary_pointer);
    alt_printf("| Value:        0x%x\n", canarytable->entries[free_index].canary);
    alt_printf("| Added index%d to the table, active canaries=%d\n", free_index, active_canaries);
    alt_printf("--------------------------------------\n");
#endif

    // Temporarely unlock the metadata section
    *canary.heap_canary_pointer = canary.canary;
    canarytable->entries[free_index] = canary;
    active_canaries++;
    return 0;
}

/* 
Creates a new entry into the "canarytable".
Associates a canary with heap_canary_location (a memory address)).
*/
int add_canary(uint64_t* canary){
    init_random();
    
    //Create canary object on stack 
    CanaryObject new_canary;
    new_canary.canary = next_random_int_v2(65536);
    new_canary.heap_canary_pointer = canary;
    return internal_add_canary(new_canary);

}

/* 
Check all canaries in the given canary table are correct.
*/
bool check_canary(CanaryTable* target_table){
    for (size_t i = 0; i < CANARY_TABLE_ENTRIES; i++){
        volatile uint64_t* heap_canary_pointer = target_table->entries[i].heap_canary_pointer;
        if(heap_canary_pointer != 0){
            uint64_t current_val = *(heap_canary_pointer);
            uint64_t expected_val = target_table->entries[i].canary;
            if(expected_val != current_val){
#if CANARY_DEBUG_PRINT
                alt_printf("-------------CANARY ERROR--------------\n");
                alt_printf("| Pointer:  0x%x\n", heap_canary_pointer);
                alt_printf("| Expected: 0x%x\n", expected_val);
                alt_printf("| Actual:   0x%x\n", current_val);
                alt_printf("--------------------------------------\n");
#endif
                return false;
            }
        }
    }
    return true;
}

/*
Remove canary from canarytable
*/
__attribute__((section(".text.critical_func"), used, noinline)) 
int remove_canary(uint64_t* canary_adress){
    #if CANARY_DEBUG_PRINT
    alt_printf("Removing Canary at 0x%x\n", heap_start);
    #endif
    CanaryObject* rev_obj;
    int i = 0;
    
    //Find CanaryObject in canarytable
    //iterate the list until heap_start indicator is found
    while (canarytable.entries[i].heap_canary_pointer != canary_adress)
    {
        i++;
        if (i > CANARY_TABLE_ENTRIES)
        {
            return 1;
        }
    }

    //Clear information about the object (Done by reference)
    open_canary_metadata();
    canarytable.entries[i].heap_canary_pointer = (uint64_t*) 0;
    active_canaries--;
    lock_canary_metadata();
    #if CANARY_DEBUG_PRINT
    alt_printf("Removed index %d from the table, active canaries=%d\n", i, active_canaries);
    #endif
    return 0;
}

void read_canary(uint64_t index){
    alt_printf("The canary is '%d' and it's located at 0x%x\n", canarytable.entries[index].canary, canarytable.entries[index].heap_canary_pointer);
}

