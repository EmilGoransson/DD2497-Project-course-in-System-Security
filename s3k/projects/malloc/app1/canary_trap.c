#include <string.h>
#include "altc/altio.h"
#include "s3k/s3k.h"

#include "canary_trap.h"
#include "../utils.h"

extern int __canary_metadata_pointer;
extern int __canaryTable_size;
/* CANARY TRAP CODE */
#define RAM_CAP 2
#define TRAP_STACK_SIZE 1024
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
	debug_capability_from_idx(pmp_cap_idx);
    setup_trap(canary_trap_handler, trap_stack, TRAP_STACK_SIZE);
}


/*
    When program tries to write to canary, this handler will be invoked.

    Wait, how do should be revert the PMP capablity? The handler will only run one time!
*/
void canary_trap_handler(){
    alt_printf("ERROR: Tried to write to canary metadata without unlocking first\n");
    while(true){};
    /*
    uint64_t exception_address = s3k_reg_read(S3K_REG_EPC);

    // For now, assume that this function is 200 bytes large. Will need a better way
    // of getting its size later. Probably have to do something with linker scripts.
    uint64_t internal_canary_end_addr = 200 + (uint64_t)internal_add_canary;
    // Test if caller is allowed to write to the canaries metadata
    bool is_allowed_to_write = exception_address >= (uint64_t)internal_add_canary && 
                                exception_address <= internal_canary_end_addr;

    if(is_allowed_to_write){
        // Set capability to be writable
        alt_printf("OPENING PMP LOCK FOR CANARY METADATA \n");
        open_metadata();
    }
    else{
        alt_printf("NOT ALLOWED TO WRITE TO CANARY METADATA FROM 0x%x\n", exception_address);
    }
    */
}

// Sets the metadata to read only
void lock_canary_metadata(){
    s3k_err_t err = s3k_pmp_load(pmp_cap_idx, 0);
    s3k_sync_mem();
    // We need to decide what should happen when something goes wrong, (process termination?)
    // for now, I will just trap it in an infinite loop
    while(err){}
}

// Sets the metadata to read-write
void open_canary_metadata(){
    s3k_err_t err = s3k_pmp_unload(pmp_cap_idx);
    s3k_sync_mem();
    if(err){
        alt_printf("ERROR: Could not unlock canary metadata pmp region");
    }
}
