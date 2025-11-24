#pragma once
#include "s3k/s3k.h"
#include "canary.h"
/*

    This code should maybe be in the canary files

*/

void init_canary_trap();
void canary_trap_handler();
void lock_metadata();
void open_metadata();
