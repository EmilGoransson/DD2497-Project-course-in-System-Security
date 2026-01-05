#include <string.h>
#include "altc/altio.h"
#include "s3k/s3k.h"
//#include "s3k/kernel/inc/offsets.h"

#include "heap/canary_trap.h"
#include "heap/canary.h"
#include "heap/utils.h"

extern int __canary_metadata_pointer;
extern int __canaryTable_size;

extern uint8_t __critical_func_start;
extern uint8_t __critical_func_end;

//Defined in trap.S
extern void canary_trap(void);

/* CANARY TRAP CODE */
#define RAM_CAP 2               // Update to RAM_MEM (which is defined in utils.h)
static char trap_stack[TRAP_STACK_SIZE];
static uint32_t pmp_cap_idx;

void canary_trap_handler();

/* 
    Initializes the trap
*/
void init_canary_trap(){

    //alt_printf("TRAP HANDLER ADDRESS: 0x%x\n", trap_handler);
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
    // THIS FUNCTINO ACTIUALLY DOES TAKE ARUGMENTS, BUT WE ARE NOT ALLOWED TO 
    // DEFINE IT AS SUCH!!!
    alt_printf("MEMORY LOCATION OF CANARY_TRAP: 0x%x\n", canary_trap);
    setup_trap(canary_trap, trap_stack, TRAP_STACK_SIZE);

}

// https://www2.eecs.berkeley.edu/Pubs/TechRpts/2016/EECS-2016-118.pdf 
SD_Instruction parse_sd_instruction(uint32_t data){
    // We only support C.SD and SD instructions.
    // There are probably other store instructions
    // in risc-v, but it would be too much effort to 
    // support all of them. Hopefully the compiler will 
    // not randomly start using other types of SD instructions.

    // In riscv, compressed (16 bit) instructions do not have 11 
    // as the 2 least significant bits.
    bool is_compressed = (data&3) != 3;
    SD_Instruction intr;
    // From page 83 of the manual
    typedef struct {
        uint32_t op     : 2;
        uint32_t rs2    : 3;
        uint32_t imm1   : 2;
        uint32_t rs1    : 3;
        uint32_t imm2   : 3;
        uint32_t funct3 : 3;
    } C_SD_Intr;
    // Page 29
    typedef struct {
        uint32_t op     : 7;
        uint32_t imm1   : 5;
        uint32_t funct3 : 3;
        uint32_t rs1    : 5;
        uint32_t rs2    : 5;
        uint32_t imm2   : 7;
    } S_Instr;


    if(is_compressed){
        C_SD_Intr* csd_intr = (C_SD_Intr*)&data;

        alt_printf("\t------ Parsing Compressed Instruction: 0x%x -------\n", (uint16_t)data);
        alt_printf("\t| OP:     0x%x\n", csd_intr->op);
        alt_printf("\t| rs2:    0x%x\n", csd_intr->rs2);
        alt_printf("\t| imm1:   0x%x\n", csd_intr->imm1);
        alt_printf("\t| rs1:    0x%x\n", csd_intr->rs1);
        alt_printf("\t| imm2:   0x%x\n", csd_intr->imm2);
        alt_printf("\t| funct3: 0x%x\n", csd_intr->funct3);
        alt_printf("\t--------------------------------------------\n");

        
        SD_Instruction intr_compressd = {
            .valid      = csd_intr->op==0?1:0, // C.SD has OP code 0
            .offset     = csd_intr->imm2<<3 | csd_intr->imm1<<6,
            .dest_reg   = csd_intr->rs1+8, 
            .source_reg = csd_intr->rs2+8,
            .compressed = true,
        };
        intr = intr_compressd;
    }
    else{
        S_Instr* s_instr = (S_Instr*)&data;

        alt_printf("\t------ Parsing S-Instruction: 0x%x -------\n", data);
        alt_printf("\t| OP:     0x%x\n", s_instr->op);
        alt_printf("\t| imm1:   0x%x\n", s_instr->imm1);
        alt_printf("\t| funct3: 0x%x\n", s_instr->funct3);
        alt_printf("\t| rs1:    0x%x\n", s_instr->rs1);
        alt_printf("\t| rs2:    0x%x\n", s_instr->rs2);
        alt_printf("\t| imm2:   0x%x\n", s_instr->imm2);
        alt_printf("\t--------------------------------------------\n");

        SD_Instruction intr_stype = {
            .valid      = 1, // C.SD has OP code 51?
            .offset     = s_instr->imm1 | s_instr->imm2 << 5,
            .dest_reg   = s_instr->rs1, 
            .source_reg = s_instr->rs2,
            .compressed = false,
        };
        intr = intr_stype;
    }

    alt_printf("\t------ Parsed Instruction -------\n");
    alt_printf("\t| valid:  0x%x\n", intr.valid);
    alt_printf("\t| Source: x%d\n", intr.source_reg);
    alt_printf("\t| Dest:   x%d\n", intr.dest_reg);
    alt_printf("\t| Offset: 0x%x\n", intr.offset);
    alt_printf("\t-----------------------------------\n");

    return intr;

}



void canary_trap_handler(){
    /*
        The registers are saved in order on the stack
        in the assembly function canary_trap. We obtain a 
        pointer to it like this: Take initial SP (end of trap stack)
        and subtrack 512 bytes from it (space reserverd for registers).
    */
    volatile uint64_t* reg_ptr = (uint64_t*)trap_stack + (TRAP_STACK_SIZE-512)/sizeof(uint64_t);
    
    volatile uint64_t registers[32];
    for(int i=0; i<32; i++)
    {
        registers[i] = reg_ptr[i+1];
    }

    alt_printf("---- TRAP HANDLER INVOKED ----\n");
    volatile uint64_t* sp = (uint64_t*)s3k_reg_read(S3K_REG_ESP);
    int64_t offset = 0;
    alt_printf("| PROC PID: %d\n", 0);

    // Set reg to 1 such that we can verify that we are in the trap handler
    uint64_t* exception_address = (uint64_t*)s3k_reg_read(S3K_REG_EPC);

    uint32_t exception_instruction = (uint32_t)*exception_address;
    alt_printf("| Exception Address: 0x%x ---\n", exception_address);
    SD_Instruction instr = parse_sd_instruction(exception_instruction);
    
    s3k_reg_write(S3K_REG_EPC, TRAP_EPC_CONSTANT);

    uint64_t return_address;
    if(instr.valid){
        /* 
            Check if the caller is allowed to open meta-data
        */
        if(exception_address < &__critical_func_start 
            || exception_address > &__critical_func_end){
            alt_printf("Illigal Canary Write Attempted From 0x%x\n", exception_address);
            while(1){}
        }

        open_canary_metadata();
        for(int i=8; i<24; i++){
            alt_printf("REG: x%d has value: 0x%x\n", i, registers[i]);
        }
        // Run the assembly C.SD assembly instrcution
        uint64_t src_reg_value = registers[instr.source_reg];
        uint64_t dst_reg_value = registers[instr.dest_reg];
        uint64_t* target_position = (uint64_t*)(instr.offset + dst_reg_value);
        alt_printf("| WRITING VALUE 0x%x to position 0x%x\n", src_reg_value, target_position);
        *target_position = src_reg_value;
        lock_canary_metadata();

        // Return the EPC to the instruction after the exception
        // Assume that we only use compressed instructions (2 bytes)
        return_address = (uint64_t)exception_address + (instr.compressed ? 2 : 4);
        alt_printf("| RETURNING TO:     0x%x\n", return_address);
        alt_printf("| Next Instruction: 0x%x\n", *((uint16_t*)return_address));
        s3k_reg_write(S3K_REG_EPC, return_address);
        alt_printf("| ---- RETURNING FROM TRAP HANDLER -----\n");
    }
    else{
        // Debug print crashed instruction
        alt_printf("INSTRUCTION POINTER THAT CRASHED: 0x%x\n", exception_address);

        while(1){}
    }

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
    alt_printf("OPEN CANARY ERRNO: %d\n", err);
    if(err){
        alt_printf("ERROR: Could not unlock canary metadata pmp region\n");
    }
    s3k_sync_mem();
    if(s3k_reg_read(S3K_REG_EPC) != TRAP_EPC_CONSTANT){
        alt_printf("ERROR: Open canary metadata executing from illegal context\n");
        while(true){}
    }
    
}
