#pragma once
#include "s3k/s3k.h"
#include "altc/altio.h"

extern int __canaryTable_size;
extern int __canary_metadata_pointer;

#define CANARY_TABLE_ENTRIES 256

#define USE_TRAP 0 //maybe move to common .h file where each feature can be toggled

// look at these again and change as necessary
typedef struct{
    //8 Bytes (can be 2 bytes, don't need more space rn)
    uint64_t canary;
    //8 Bytes
    uint64_t* heap_canary_pointer;
} CanaryObject;

typedef struct {
    //16 Bytes 
    CanaryObject entries[CANARY_TABLE_ENTRIES];
} CanaryTable;


//Compare Canary table entry with given_canary
bool check_canary(CanaryTable* target_table);

//remove CanaryObject with heap_start from canary table
void remove_canary(__uint64_t* heap_start);

// Add process canary to canaryTable 
void internal_add_canary(CanaryObject canary);

// Generate a new canary and place it in the heap
void add_canary(__uint64_t* heap_address);

//Initialize the canary table
void init_canary_table();


//Read a CanaryObject from CanaryTable
void read_canary(__uint64_t read_canary);
void size(CanaryTable* node);
void test();

// Temporary solution, we need a linker to solve this.
// Exported variable
//uint64_t internal_canary_end_addr;

