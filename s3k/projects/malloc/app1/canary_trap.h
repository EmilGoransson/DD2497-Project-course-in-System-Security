#pragma once
#include "s3k/s3k.h"
#include "canary.h"
/*

    We don't use the trap to temporary unlock the canary metadata
    since there is not simple and clear way to automatically unlock it
    after use. 
    
    For now, we manually call open_canary_metadata and lock_canary_metadata
    when it should be unlocked. I don't see how this has any more security holes
    than the trap method.

*/

void init_canary_trap();
//void canary_trap_handler();
void lock_canary_metadata();
void open_canary_metadata();


