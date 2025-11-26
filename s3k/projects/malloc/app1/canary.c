#include "canary.h"
#include <string.h>

extern int __canary_metadata_pointer;

// For initiliziing canary table in a specific section
// __attribute__((section(".canary_metadata"), used))
// CanaryObject ctable[CANARY_TABLE_ENTRIES];
static CanaryTable* canarytable;
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
        i++;
    }
}

/*
Finds first available spot in ther internal canary table and adds there
Canary = -1? Available space
else canary in use

Used by add_canary
*/
void internal_add_canary(CanaryObject canary){
    int free_index = 0;
    while (canarytable->entries[free_index].heap_canary_pointer) {
        if (free_index == CANARY_TABLE_ENTRIES){
            //No freeindex found, cannot add new entry to canary table
            alt_printf("Error: could not add new canary to canarytable");
            return;
        }
        free_index++;
    }
    // Temporarely unlock the metadata section
#if USE_TRAP
    open_canary_metadata();
#endif
    canarytable->entries[free_index] = canary;
#if USE_TRAP
    lock_canary_metadata();
#endif
    *canary.heap_canary_pointer = canary.canary;
}

/* 
Creates a new entry into the "canarytable".
Associates a canary with heap_canary_location (a memory address)).

(for now no randomizer. "canary_value" is just incremental values, not secure)
*/
void add_canary(uint64_t* heap_canary_location){
    CanaryObject new_canary;
    new_canary.canary = next_random_int();
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

// Probably won't work for the monitor process. Will have to share the OG process canary table with the monitor process.
// Right now it's made as if the process is checking itself
bool check_canary(){
    bool same_canary = true;

    for (size_t i = 0; i < CANARY_TABLE_ENTRIES; i++){
        if(canarytable->entries[i].heap_canary_pointer){
            if(canarytable->entries[i].canary != *(canarytable->entries[i].heap_canary_pointer)){
                same_canary = true;
                alt_printf("ERROR CANARY AT: 0x%x\n", canarytable->entries[i].heap_canary_pointer);
                alt_printf("BUFFER OVERFLOW. The canary was '%d', but now it's '%d' \n", canarytable->entries[i].canary, *(canarytable->entries[i].heap_canary_pointer));
            }
        }
    }


    return same_canary;
}

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
    rev_obj = &(canarytable->entries[i]);
    alt_printf("rev_obj canary: %d\n", rev_obj->canary);

    //Clear information about the object (Done by reference)
    rev_obj->canary = -1;
    rev_obj->heap_canary_pointer = (__uint64_t*)0;
    alt_printf("The in-memory object's canary: %d\n", canarytable->entries[i].canary);
}

void read_canary(__uint64_t index){
    alt_printf("The canary is '%d' and it's located at 0x%x\n", canarytable->entries[index].canary, canarytable->entries[index].heap_canary_pointer);
}

void size(CanaryTable* node){
    alt_printf("uint16 size in bytes %d\n", sizeof(uint16_t));
    alt_printf("uint64 size in bytes %d\n", sizeof(uint64_t));
    alt_printf("CanaryObject size in bytes %d\n", sizeof(CanaryObject));
    alt_printf("CanaryTable size in bytes %d\n", sizeof(CanaryTable));
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