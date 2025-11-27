#include "canary.h"
#include <string.h>
#include "canary_trap.h"
#include "randomize.h"
#include "../utils.h"

extern int __canary_metadata_pointer;

// For initiliziing canary table in a specific section (I think this is it, now we initilize it in that section of memory instead of in .data)
__attribute__((section(".canary_metadata"), used))
// REMOVE THIS(?):
// CanaryObject ctable[CANARY_TABLE_ENTRIES];

static CanaryTable* canarytable;
static int canarytable_head = -1;
static int canarytable_free = 0;


/*
Initiate canarytable with the "unused" value (-1)

*/
void init_canary_table(){
    //Point to the memory area
    canarytable = (CanaryTable*) &__canary_metadata_pointer;
    
    //Change the current canaries to -1 as a starting value
    int i = 0;
    while(i != CANARY_TABLE_ENTRIES) {
        canarytable->entries[i++].canary = -1;
    }
}

/*
Finds first available spot in ther internal canary table and adds there
Canary = -1? Available space
else canary in use

Used by add_canary
*/
void internal_add_canary(CanaryObject canary){
    int free_index = 0;
    while (canarytable->entries[free_index].canary != -1) {
        if (free_index == CANARY_TABLE_ENTRIES){
            //No freeindex found, cannot add new entry to canary table
            alt_printf("Error: could not add new canary to canarytable");
            return;
        }
        free_index++;
    }
    // Temporarely unlock the metadata section
    open_canary_metadata();
    canarytable->entries[free_index] = canary;
    lock_canary_metadata();
    *canary.heap_canary_pointer = canary.canary;
}

/* 
Creates a new entry into the "canarytable".
Associates a canary with heap_canary_location (a memory address)).

(for now no randomizer. "canary_value" is just incremental values, not secure)
*/
void add_canary(uint64_t* heap_canary_location){
    CanaryObject new_canary;
    //some random number (2^16)
    init_random();
    new_canary.canary = next_random_int_v2(65536);
    new_canary.heap_canary_pointer = heap_canary_location;
    internal_add_canary(new_canary);
}

/*
Randomizer for creating canary values

TODO: need a dedicated PRNG or similar maybe?
*/
uint64_t next_random_int(){
    static uint64_t canary_value = 0;
    canary_value += 1;
    return canary_value;
}

// Probably won't work for the monitor process. Will have to share the OG process canary table with the monitor process.
// Right now it's made as if the process is checking itself
bool check_canary(CanaryTable* target_table){
    bool same_canary = true;

    for (size_t i = 0; i < CANARY_TABLE_ENTRIES; i++){
        if(canarytable->entries[i].canary != -1){
            if(canarytable->entries[i].canary != *(canarytable->entries[i].heap_canary_pointer)){
                same_canary = true;
                alt_printf("ERROR CANARY AT: 0x%x\n", canarytable->entries[i].heap_canary_pointer);
                alt_printf("BUFFER OVERFLOW. The canary was '%d', but now it's '%d' \n", canarytable->entries[i].canary, *(canarytable->entries[i].heap_canary_pointer));
            }
        }
    }


    return same_canary;
}

void remove_canary(__uint64_t* heap_start){
    CanaryObject* rev_obj;
    __uint8_t i = 0;
    
    //Find CanaryObject in canarytable
    //iterate the list until heap_start indicator is found
    while (canarytable->entries[i].heap_canary_pointer != heap_start)
    {
        i++;
        if (i > CANARY_TABLE_ENTRIES)
        {
            alt_printf("Object not found, cant remove\nReturning...\n");
            return;
        }
        
    }
    rev_obj = &(canarytable->entries[i]);
    alt_printf("rev_obj canary: %d\n", rev_obj->canary);

    //Clear information about the object (Done by reference)
    rev_obj->canary = -1;
    rev_obj->heap_canary_pointer = (__uint64_t*) 0xDEADBEEF; // <-- BORDE BARA NULLPTR?
    alt_printf("The in-memory object's canary: %d\n", canarytable->entries[i].canary);
}

void read_canary(__uint64_t index){
    alt_printf("The canary is '%d' and it's located at 0x%x\n", canarytable->entries[index].canary, canarytable->entries[index].heap_canary_pointer);
}

void size(CanaryTable* node){
    alt_printf("uint16 size in bytes %d\n", sizeof(uint16_t));
    alt_printf("uint64 size in bytes %d\n", sizeof(uint64_t));
    alt_printf("CanaryObject size in bytes %d\n", sizeof(CanaryObject));
    alt_printf("CanaryTable size in bytes %d\n", sizeof(CanaryTable));
}

void test(){
    alt_printf("Canary metadata pointer 0x%x\n", &__canary_metadata_pointer);
    alt_printf("First byte of canarytable: %d\n", *canarytable);
    alt_printf("First byte of canarytable: %d\n", __canary_metadata_pointer);

    alt_printf("CanaryTable size %x\n", &__canaryTable_size);
    alt_printf("CanaryTable size in bytes %d\n", CANARY_TABLE_ENTRIES);

    alt_printf("Canary table initliazed\n");
    alt_printf("Size of canary table is %d bytes\n", sizeof(canarytable));
}


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
