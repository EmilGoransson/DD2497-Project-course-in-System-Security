#include <string.h>
#include "altc/altio.h"
#include "s3k/s3k.h"
//#include "s3k/kernel/inc/offsets.h"

#include "heap/canary_trap.h"
#include "heap/canary.h"
#include "heap/utils.h"

extern int __canary_metadata_pointer;
extern int __canaryTable_size;

/* CANARY TRAP CODE */
#define RAM_CAP 2               // Update to RAM_MEM (which is defined in utils.h)
static char trap_stack[TRAP_STACK_SIZE];
static uint32_t pmp_cap_idx;

void canary_trap_handler() __attribute__((interrupt("machine")));

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
    setup_trap(canary_trap_handler, trap_stack, TRAP_STACK_SIZE);
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
        We don't have any clear way to access all the 
        general purpose registers as they were before the exception.
        Luckely most of them are not changed at this point. 
        We use inline assembly since it is easier, but this should
        really be done fully in assembly.
    */

    __asm__ volatile (
        "addi sp, sp, -192\n"
        "sd x8,    0(sp)\n"
        "sd x9,    8(sp)\n"
        "sd x10,  16(sp)\n"
        "sd x11,  24(sp)\n"
        "sd x12,  32(sp)\n"
        "sd x13,  40(sp)\n"
        "sd x14,  48(sp)\n"
        "sd x15,  56(sp)\n"
        "sd x16,  64(sp)\n"
        "sd x17,  72(sp)\n"
        "sd x18,  80(sp)\n"
        "sd x19,  88(sp)\n"
        "sd x20,  96(sp)\n"
        "sd x21, 104(sp)\n"
        "sd x22, 112(sp)\n"
        "sd x23, 120(sp)\n"
        "sd x24, 128(sp)\n"
        "sd x25, 136(sp)\n"
        "sd x26, 144(sp)\n"
        "sd x27, 152(sp)\n"
        "sd x28, 160(sp)\n"
        "sd x29, 168(sp)\n"
        "sd x30, 176(sp)\n"
        "sd x31, 184(sp)\n"
        ::: "memory"
    );
    volatile uint64_t* reg_ptr = (uint64_t*)s3k_reg_read(S3K_REG_SP);
    volatile uint64_t registers[32];
    for(int i=8; i<32; i++)
    {
        registers[i] = reg_ptr[i-8];
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
        open_canary_metadata();
        /*for(int i=8; i<16; i++){
            alt_printf("REG: x%d has value: 0x%x\n", i, registers[i]);
        }*/
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

    __asm__ volatile (
        "ld x8,    0(sp)\n\t"
        "ld x9,    8(sp)\n\t"
        "ld x10,  16(sp)\n\t"
        "ld x11,  24(sp)\n\t"
        "ld x12,  32(sp)\n\t"
        "ld x13,  40(sp)\n\t"
        "ld x14,  48(sp)\n\t"
        "ld x15,  56(sp)\n\t"
        "ld x16,  64(sp)\n\t"
        "ld x17,  72(sp)\n\t"
        "ld x18,  80(sp)\n\t"
        "ld x19,  88(sp)\n\t"
        "ld x20,  96(sp)\n\t"
        "ld x21, 104(sp)\n\t"
        "ld x22, 112(sp)\n\t"
        "ld x23, 120(sp)\n\t"
        "ld x24, 128(sp)\n\t"
        "ld x25, 136(sp)\n\t"
        "ld x26, 144(sp)\n\t"
        "ld x27, 152(sp)\n\t"
        "ld x28, 160(sp)\n\t"
        "ld x29, 168(sp)\n\t"
        "ld x30, 176(sp)\n\t"
        "ld x31, 184(sp)\n\t"
        "addi sp, sp, 192\n\t"
        ::: "x8",  "x9",  "x10", "x11", "x12", "x13", "x14", "x15",
            "x16", "x17", "x18", "x19", "x20", "x21", "x22", "x23",
            "x24", "x25", "x26", "x27", "x28", "x29", "x30", "x31",
            "memory"
    );
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
