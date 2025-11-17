#include "canary.h"
#include <string.h>

extern int __canary_metadata_pointer;

static CanaryTable* canarytable;
static int canarytable_head = -1;
static int canarytable_free = 0;

int canary_value = 0;

void init_canary_table(){
    //Point to the memory area
    canarytable = (CanaryTable*) &__canary_metadata_pointer;

    //Change the current canaries to -1 as a starting value
    int i = 0;
    while(i != CANARY_TABLE_ENTRIES) {
        canarytable->entries[i++].canary = -1;
    }
}

//Used by generate_canary, finds first available spot in ther internal canary table and adds there
//Canary = -1? Available space
//else canary in use
void internal_add_canary(CanaryObject canary){
    int free_index = 0;
    while (canarytable->entries[free_index].canary != -1) {
        if (free_index == CANARY_TABLE_ENTRIES){
            //No freeindex found, cannot add new entry to canary table
            return;
        }
        free_index++;
    }
    // alt_printf("Freeindex found by add_canary: %d\n", free_index);
    canarytable->entries[free_index] = canary;
}

// (for now no randomizer. "canary_value" is just incremental values, not secure)
void add_canary(uint64_t* heap_canary_location){
    CanaryObject new_canary;
    randomizer();
    
    new_canary.canary = canary_value;
    new_canary.heap_canary_pointer = heap_canary_location;
    
    internal_add_canary(new_canary);
}

int randomizer(){
    canary_value += 1;
}

// Probably won't work for the monitor process. Will have to share the OG process canary table with the monitor process.
// Right now it's made as if the process is checking itself
bool check_canary(){
    bool same_canary = true;

    for (size_t i = 0; i < CANARY_TABLE_ENTRIES; i++){
        if(canarytable->entries[i].canary != -1){
            if(canarytable->entries[i].canary != *(canarytable->entries[i].heap_canary_pointer)){
                same_canary = true;
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
    rev_obj->heap_canary_pointer = (__uint64_t*) 0xDEADBEEF;
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