#include <string.h>
#include "altc/altio.h"
#include "s3k/s3k.h"

#include "heap/canary_trap.h"
#include "heap/canary.h"
#include "heap/utils.h"

extern int __canary_metadata_pointer;
extern int __canaryTable_size;

/* CANARY TRAP CODE */
#define RAM_CAP 2               // Update to RAM_MEM (which is defined in utils.h)
static char trap_stack[TRAP_STACK_SIZE];
static uint32_t pmp_cap_idx;

// Look up what this does. Does it tell the linker that this function 
// will be used as a trap handler and make it store / load all registers on stack?
void canary_trap_handler(void) __attribute__((interrupt("machine")));

/* 
    Initializes the trap
*/
void init_canary_trap(){
    // Create the PMP capability
    pmp_cap_idx = find_free_cap();
    uint64_t canary_meta_start  = (uint64_t)&__canary_metadata_pointer;
    uint64_t canary_meta_size   = (uint64_t)&__canaryTable_size;
    alt_printf("CANARY METADATA: 0x%x, LEN: 0x%x\n", canary_meta_start, canary_meta_size);
    uint64_t pmp_addr = s3k_napot_encode(canary_meta_start, canary_meta_size);
	s3k_err_t err = s3k_cap_derive(RAM_CAP, pmp_cap_idx, s3k_mk_pmp(pmp_addr, S3K_MEM_RX));
    if(err){
        alt_printf("Could not derive PMP capability, error code: %x\n", err);
    }
    lock_canary_metadata();
	// debug_capability_from_idx(pmp_cap_idx);
    setup_trap(canary_trap_handler, trap_stack, TRAP_STACK_SIZE);
}


/*
    When program tries to write to canary, this handler will be invoked.

    Wait, how do should be revert the PMP capablity? The handler will only run one time!
*/
void canary_trap_handler(){
    // Set reg to 1 such that we can verify that we are in the trap handler
    uint32_t* exception_address = (uint32_t*)s3k_reg_read(S3K_REG_EPC);
    s3k_reg_write(S3K_REG_EPC, TRAP_EPC_CONSTANT);

    // Fix security function??? However that's supposed to be?? Check it's own trap handler stack??? Check that we're in the stack pointer
    //
    // if(s3k_reg_read(S3K_REG_SP) != s3k_reg_read(S3K_REG_TSP)){
    //     alt_printf("ERROR: Canary trap handler executing from illegal context\n");
    //     while(true){}
    // }
    
    alt_printf("SP adress in trap handler: 0x%x\n", s3k_reg_read(S3K_REG_SP));
    alt_printf("TSP adress in trap handler: 0x%x\n", s3k_reg_read(S3K_REG_TSP));
    alt_printf("ESP adress in trap handler: 0x%x\n", s3k_reg_read(S3K_REG_ESP));

    open_canary_metadata();

    // Part that either adds or removes canaries depending on instruction
    //
    // <==
    CanaryObject canary = {
        .canary = 0xDEEDBEEF,
        .heap_canary_pointer = 0,
    };
    internal_add_canary(canary);

    lock_canary_metadata();

    // Return the EPC to the instruction after the exception
    s3k_reg_write(S3K_REG_EPC, (uint64_t)exception_address + INSTRUCTION_SIZE);

    // Debug print crashed instruction
    alt_printf("INSTRUCTION THAT CRASHED: 0x%x\n", *exception_address);

    while(1){}
}

// Sets the metadata to read only
void lock_canary_metadata(){
    s3k_err_t err = s3k_pmp_load(pmp_cap_idx, 0);
    if(err){
        alt_printf("Error, could not load canaray metadata PMP region\n");
    }
    s3k_sync_mem();
    // We need to decide what should happen when something goes wrong, (process termination?)
    // for now, I will just trap it in an infinite loop
    while(err){}
}

// Sets the metadata to read-write
void open_canary_metadata(){
    s3k_err_t err = s3k_pmp_unload(pmp_cap_idx);
    if(err){
        alt_printf("ERROR: Could not unlock canary metadata pmp region\n");
    }
    s3k_sync_mem();
    if(s3k_reg_read(S3K_REG_EPC) != TRAP_EPC_CONSTANT){
        alt_printf("ERROR: Open canary metadata executing from illegal context\n");
        while(true){}
    }
    
}
