#include "canary.h"

//In-progress
void check_canary(__uint64_t heap_start, __uint64_t given_canary){
    CanaryObject* getcanary;
    memcpy(getcanary, canarytable.index(heap_start), sizeof(CanaryObject));
    if (getcanary == given_canary){

    }
}

