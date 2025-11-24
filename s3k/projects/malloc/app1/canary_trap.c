#include "canary_trap.h"
#include "../../tutorial-commons/utils.h"
#define APP0_PID 0
#define RAM_CAP 3
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
    lock_metadata();
	debug_capability_from_idx(pmp_cap_idx);
    setup_trap(canary_trap_handler, trap_stack, TRAP_STACK_SIZE);
}


/*
    When program tries to write to canary, this handler will be invoked.

    Wait, how do should be revert the PMP capablity? The handler will only run one time!
*/
void canary_trap_handler(){
    register uint64_t *caller_address asm ("ra");
    alt_printf("Crashed at: 0x%x\n", caller_address);
    /*
    // Test if caller is allowed to write to the canaries metadata
    bool is_allowed_to_write = caller_address >= internal_add_canary && 
                                caller_address <= internal_canary_end_addr;

    if(is_allowed_to_write){
        // Set capability to be writable
    }
    */
    //default_trap_handler(); // Defined?
}

// Sets the metadata to read only
s3k_err_t lock_metadata(){
    s3k_err_t err = s3k_pmp_load(pmp_cap_idx, 0);
    s3k_sync_mem();
    return err;
}

// Sets the metadata to read-write
void open_metadata(){
    s3k_err_t err = s3k_pmp_unload(pmp_cap_idx);
    s3k_sync_mem();
    return err;
}
