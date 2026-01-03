#pragma once


typedef struct SD_Instruction{
    bool valid;
    bool compressed;
    uint8_t source_reg;
    uint8_t dest_reg;
    // 12 bits signed immidiate
    int16_t offset; 
} SD_Instruction;

void init_canary_trap();
//void canary_trap_handler();
void lock_canary_metadata();
void open_canary_metadata();