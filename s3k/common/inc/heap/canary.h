#pragma once
#include "s3k/s3k.h"
#include "altc/altio.h"

extern int __canaryTable_size;
extern int __canary_metadata_pointer;

#define CANARY_TABLE_ENTRIES 256

#define USE_TRAP 1

typedef struct{
    //8 Bytes
    volatile uint64_t canary;
    //8 Bytes
    uint64_t* volatile heap_canary_pointer;
} CanaryObject;

typedef struct {
    //16 Bytes * 256 = 4096
    volatile CanaryObject entries[CANARY_TABLE_ENTRIES];
} CanaryTable;


// Verify all canaries in the given canary table
bool check_canary(CanaryTable* target_table);

// Remove CanaryObject with canary from canary table
int remove_canary(uint64_t* canary_address);

// Add process canary to canaryTable 
int internal_add_canary(CanaryObject canary);

// Generate a new canary and place it in the heap
int add_canary(uint64_t* heap_address);

//Initialize the canary table
void init_canary_table();

//Read a CanaryObject from CanaryTable
void read_canary(uint64_t read_canary);

