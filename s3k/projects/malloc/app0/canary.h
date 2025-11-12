#pragma once
#include "s3k/s3k.h"

//look at these again and change as necessary
typedef struct{
    uint64_t heap_start;
    uint64_t heap_end_canary;
}CanaryObject;
typedef struct {
    CanaryObject value;
    struct CanarNode* next;
} CanaryNode;



//Canary table entry: index with heap_stary 
//Compare Canary table entry with given_canary
void check_canary(__uint64_t heap_start, __uint64_t given_canary);

//remove heap_start index from canary table
void remove_canary(__uint64_t heap_start);

// Add process canary to table
void add_canary(__uint64_t heap_start);

void init_canary_table();