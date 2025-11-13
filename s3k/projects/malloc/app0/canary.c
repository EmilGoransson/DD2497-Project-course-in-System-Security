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
        canarytable->value[i++].canary = -1;
    }
}

void read_canary(uint64_t index){
    alt_printf("The canary is '%d' and it's located at 0x%x\n", canarytable->value[index].canary, canarytable->value[index].heap_canary_pointer);
}

//Used by generate_canary, finds first available spot and adds it to the allocated space in the heap
//Canary = -1? Available space
//else canary in use
void add_canary(CanaryObject canary){
    int free_index = 0;
    while (canarytable->value[free_index].canary != -1) {
        if (free_index == CANARY_TABLE_ENTRIES){
            //No freeindex found, cannot add new entry to canary table
            return;
        }
        free_index++;
    }
    // alt_printf("Freeindex found by add_canary: %d\n", free_index);
    canarytable->value[free_index] = canary;
}

// (for now "canary_value" is just incremental values, not secure)
void generate_canary(uint64_t* heap_address, uint64_t heap_size){
    CanaryObject new_canary;
    canary_value += 1;
    
    new_canary.canary = canary_value;
    new_canary.heap_canary_pointer = (uint64_t)heap_address + heap_size - sizeof(CanaryObject); // Make some sort of s3k logic for this to write in heap || Ask about logic to put canary at end of heap
    
    add_canary(new_canary);
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