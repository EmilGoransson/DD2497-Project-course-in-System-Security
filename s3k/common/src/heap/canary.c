#include <string.h>
#include <stdatomic.h>
#include "heap/canary.h"
#include "heap/canary_trap.h"
#include "heap/randomize.h"
#include "heap/utils.h"


extern int __canary_metadata_pointer;

// For initiliziing canary table in a specific section (I think this is it, now we initilize it in that section of memory instead of in .data)
__attribute__((section(".canary_metadata"), used))
// REMOVE THIS(?):
// CanaryObject ctable[CANARY_TABLE_ENTRIES];

static CanaryTable* canarytable;
static int active_canaries = 0;
static bool used_index[CANARY_TABLE_ENTRIES] = {false};
uint16_t available_slots[CANARY_TABLE_ENTRIES];
static int canarytable_head = -1;
static int canarytable_free = 0;


/*
Initiate canarytable with the "unused" value (-1)

*/
void init_canary_table(){
    //Point to the memory area
    canarytable = (CanaryTable*) &__canary_metadata_pointer;
    
    //Change the current canaries to -1 as a starting value
    int i = 0;
    while(i != CANARY_TABLE_ENTRIES) {
        canarytable->entries[i].canary = -1;
        canarytable->entries[i].heap_canary_pointer = 0;
        available_slots[i] = i;
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
Finds first available spot in ther internal canary table and adds there
Canary = -1? Available space
else canary in use

Used by add_canary
*/
void internal_add_canary(CanaryObject canary){
    int total_available_slots = CANARY_TABLE_ENTRIES-active_canaries;
    int free_index = next_random_int_v2(total_available_slots); //0-251, 0-128, etc
    while (canarytable->entries[available_slots[free_index]].heap_canary_pointer) {
        if (active_canaries == CANARY_TABLE_ENTRIES){
            //No freeindex found, cannot add new entry to canary table
            alt_printf("Error: could not add new canary to canarytable");
            return;
        }
        free_index = next_random_int_v2(total_available_slots);
    }
#if CANARY_DEBUG_PRINT
    alt_printf("-------------Adding canary--------------\n");
    alt_printf("| Pointer: 0x%x\n", canary.heap_canary_pointer);
    alt_printf("| Value:   0x%x\n", canary.canary);
    alt_printf("--------------------------------------\n");
#endif
    active_canaries++;
    // Temporarely unlock the metadata section
    // MAKE SURE TO WRITE BEFORE POINTING
    //Put canary value at the given adress 
    *canary.heap_canary_pointer = canary.canary;
    #if USE_TRAP
        open_canary_metadata();
    #endif
        //Add canary object to the heap
        canarytable->entries[free_index] = canary;
    #if USE_TRAP
        lock_canary_metadata();
    #endif
    active_canaries++;

    //Add index to used_list
    used_index[free_index] = true;
    
    alt_printf("Added index%d to the table, active canaries=%d\n", available_slots[free_index], active_canaries);

    //Redefine available slots (the values in this array is the available indexes)
    uint8_t final_slot = CANARY_TABLE_ENTRIES-1;
    for (size_t i,j = 0; i < CANARY_TABLE_ENTRIES; i++)
    {
        //index in use? continue with next index
        if(used_index[i]){
            //Set the last at the end to 0
            available_slots[final_slot--] = 0;
            continue;
        }
        //For each used_index == true, this should reach total_available_slots eventually (always <=CANARY_TABLE_ENTRIES)
        available_slots[j++] = i;
    }
}

/* 
Creates a new entry into the "canarytable".
Associates a canary with heap_canary_location (a memory address)).
*/
void add_canary(uint64_t* heap_canary_location){
    CanaryObject new_canary;
    //some random number (2^16)
    init_random();
    
    //Create canary object on stack 
    new_canary.canary = next_random_int_v2(65536);
    new_canary.heap_canary_pointer = heap_canary_location;
    internal_add_canary(new_canary);
}

/*
Randomizer for creating canary values

TODO: need a dedicated PRNG or similar maybe?
*/
uint64_t next_random_int(){
    static uint64_t canary_value = 0;
    canary_value += 1;
    return canary_value;
}

bool check_canary(CanaryTable* target_table){
    if (active_canaries == 0)
    {
        return true;
    }
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
                // alt_printf("Problem at index%d: curr val at %x: %d, expect:%d", i, target_table->entries[i].heap_canary_pointer, current_val, expected_val);
                return false;
            }
        }
    }
    return true;
}

// Does not work, needs to unlock canary-heap location before writing
void remove_canary(__uint64_t* heap_start){
    CanaryObject* rev_obj;
    __uint8_t i = 0;
    
    //Find CanaryObject in canarytable
    //iterate the list until heap_start indicator is found
    while (canarytable->entries[i].heap_canary_pointer != heap_start)
    {
        i++;
        if (i > CANARY_TABLE_ENTRIES)
        {
            alt_printf("Object not found, cant remove\nReturning...\n");
            return;
        }
        
    }

    //Clear information about the object (Done by reference)
    open_canary_metadata();
    canarytable->entries[i].heap_canary_pointer = (__uint64_t*) 0;
    active_canaries--;
    lock_canary_metadata();

    //Add index to used_list
    used_index[i] = false;
    
    alt_printf("Removed index%d from the table, active canaries=%d\n", i, active_canaries);
    //Redefine available slots (the values in this array is the available indexes in the table )
    uint8_t final_slot = CANARY_TABLE_ENTRIES-1;
    for (size_t i,j = 0; i < CANARY_TABLE_ENTRIES; i++)
    {
        //index in use? continue with next index
        if(used_index[i]){
            //Set the last at the end to 0
            available_slots[final_slot--] = 0;
            continue;
        }
        //For each used_index == true, this should reach total_available_slots eventually (always <=CANARY_TABLE_ENTRIES)
        available_slots[j++] = i;
    }

    // alt_printf("The in-memory object's canary: %d\n", canarytable->entries[i].canary);
}

void read_canary(__uint64_t index){
    alt_printf("The canary is '%d' and it's located at 0x%x\n", canarytable->entries[index].canary, canarytable->entries[index].heap_canary_pointer);
}

void size(CanaryTable* node){
    alt_printf("uint16 size in bytes: %d\n", sizeof(uint16_t));
    alt_printf("uint64 size in bytes: %d\n", sizeof(uint64_t));
    alt_printf("CanaryObject size in bytes: %d\n", sizeof(CanaryObject));
    alt_printf("CanaryTable size in bytes: %d\n", sizeof(CanaryTable));
    alt_printf("Currently used canaries: %d", active_canaries);
}

void test(){
    alt_printf("Canary metadata pointer 0x%x\n", &__canary_metadata_pointer);
    alt_printf("First byte of canarytable: %d\n", *canarytable);
    alt_printf("First byte of canarytable: %d\n", __canary_metadata_pointer);

    alt_printf("CanaryTable size %x\n", &__canaryTable_size);
    alt_printf("CanaryTable size in bytes %d\n", CANARY_TABLE_ENTRIES);

    alt_printf("Canary table initliazed\n");
    alt_printf("Size of canary table is %d bytes\n", sizeof(canarytable));
}


