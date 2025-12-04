#include "altc/altio.h"
#include "s3k/s3k.h"
#include <string.h>

#include "heap/utils.h"
#include "heap/malloc.h"
#include "heap/canary.h"

extern int __heap_pointer;
extern int __canary_pointer;
extern int _end;

extern int __canary_metadata_pointer;

void monitor_app1(){
	// Get the offset of the canary table for each app
	uint64_t canary_table_offset = (uint64_t)&__canary_metadata_pointer - APP0_BASE_ADDR;
	
	// Get app1's canary table location
	CanaryTable* app1_canary_table = (CanaryTable*)(APP1_BASE_ADDR + canary_table_offset);
	//alt_printf("App1 canary table located at address: 0x%x\n", app1_canary_table);


	bool same_canary = check_canary(app1_canary_table);
	if(same_canary){
		alt_printf("All canaries intact in app1's canary table\n");
	} else {
		alt_printf("Canary check failed! Buffer overflow detected in app1's canary table\n");
		// while(1){} // Stop monitorin. We should also KILL app1?
	}
}

void setup_apps()
{
	uint64_t uart_addr = s3k_napot_encode(UART0_BASE_ADDR, 0x8);
	// Address regions for each app
	uint64_t app1_addr = s3k_napot_encode(APP1_BASE_ADDR, APP1_LENGHT);
	uint64_t app1_text_mem = s3k_napot_encode(APP1_BASE_ADDR, 4096);
	// uint64_t app2_addr = s3k_napot_encode(APP2_BASE_ADDR, APP2_LENGHT);

	// MEMORY and PMP
	// Derive memory for APP1 
	
	// Create a memory segment for APP1 (APP1)
	uint32_t mem_free_cap_idx = find_free_cap();
	s3k_err_t mon_checking_err1 = s3k_cap_derive(RAM_MEM, mem_free_cap_idx, s3k_mk_memory(APP1_BASE_ADDR, APP1_BASE_ADDR + APP1_LENGHT, S3K_MEM_RWX));
	
	// Derive a PMP capability for the newly derive app1 main memory (APP1)
	uint32_t free_cap_idx = find_free_cap();
	s3k_err_t a1 = s3k_cap_derive(mem_free_cap_idx, free_cap_idx, s3k_mk_pmp(app1_addr, S3K_MEM_RWX));
	// [We make a copy of this pmp for APP0 (APP0)]
	uint32_t app0_cap_idx = find_free_cap();
	s3k_err_t app0_a1 = s3k_cap_derive(mem_free_cap_idx, app0_cap_idx, s3k_mk_pmp(app1_addr, S3K_MEM_RX));
	// [Finally load the pmp after the first pmp (BOOT_PMP) for APP0 (APP0)]
	s3k_pmp_load(app0_cap_idx, 2);
	//Also send and load the pmp in PID1
	s3k_err_t a2 = s3k_mon_cap_move(MONITOR, APP0_PID, free_cap_idx, APP1_PID, 0);
	s3k_err_t a3 = s3k_mon_pmp_load(MONITOR, APP1_PID, 0, 7);
	
	// Make .text non writable (must have higher priority ==> lower id than RWX pmp) (APP1)
	s3k_cap_derive(mem_free_cap_idx, free_cap_idx, s3k_mk_pmp(app1_text_mem, S3K_MEM_RX));
	s3k_mon_cap_move(MONITOR, APP0_PID, free_cap_idx, APP1_PID, 4);
	s3k_mon_pmp_load(MONITOR, APP1_PID, 4, 6);
	
	//Send memory AFTER pmp over of the same region as been created (APP1)
	// alt_printf("mem_free_cap_idx	");
	// debug_capability_from_idx(mem_free_cap_idx);
	s3k_err_t mem2 = s3k_mon_cap_move(MONITOR, APP0_PID, mem_free_cap_idx, APP1_PID, 2);

	// Derive a PMP capability for app2 main memory (APP2)
	// s3k_err_t a4 = s3k_cap_derive(RAM_MEM, free_cap_idx, s3k_mk_pmp(app2_addr, S3K_MEM_RWX));
	// s3k_err_t a5 = s3k_mon_cap_move(MONITOR, APP0_PID, free_cap_idx, APP2_PID, 0);
	// s3k_err_t a6 = s3k_mon_pmp_load(MONITOR, APP2_PID, 0, 7);


	// Derive a PMP capability for app1's heap and canary table for app2 (APP2)
	// [right now, a PMP capability over the entire memory region of app1 is given to app2]
	// s3k_err_t mon_checking_err1 = s3k_cap_derive(RAM_MEM, free_cap_idx, s3k_mk_pmp(app1_addr, S3K_MEM_RX));
	// s3k_err_t mon_checking_err2 = s3k_mon_cap_move(MONITOR, APP0_PID, free_cap_idx, APP2_PID, 2);
	// s3k_err_t mon_checking_err3 = s3k_mon_pmp_load(MONITOR, APP2_PID, 2, 2);

	/*
		We reserve PMP slot 0 for the canary metadata section.
		A better way would be to allocate PMP slots high-->low
		so that subsections can have higher priority (lower slot).
	*/
	

	// Derive a PMP capability for uart (APP1)
	s3k_err_t b1 = s3k_cap_derive(UART_MEM, free_cap_idx, s3k_mk_pmp(uart_addr, S3K_MEM_RW));
	s3k_err_t b2 = s3k_mon_cap_move(MONITOR, APP0_PID, free_cap_idx, APP1_PID, 1);
	s3k_err_t b3 = s3k_mon_pmp_load(MONITOR, APP1_PID, 1, 3);
	
	// Derive a PMP capability for uart (APP2)
	// s3k_err_t b4 = s3k_cap_derive(UART_MEM, free_cap_idx, s3k_mk_pmp(uart_addr, S3K_MEM_RW));
	// s3k_err_t b5 = s3k_mon_cap_move(MONITOR, APP0_PID, free_cap_idx, APP2_PID, 1);
	// s3k_err_t b6 = s3k_mon_pmp_load(MONITOR, APP2_PID, 1, 1);

	// if (a1 != S3K_SUCCESS || a2 != S3K_SUCCESS || a3 != S3K_SUCCESS ||
	// 	a4 != S3K_SUCCESS || a5 != S3K_SUCCESS || a6 != S3K_SUCCESS ||
	// 	b1 != S3K_SUCCESS || b2 != S3K_SUCCESS || b3 != S3K_SUCCESS ||
	// 	b4 != S3K_SUCCESS || b5 != S3K_SUCCESS || b6 != S3K_SUCCESS){
	// 		alt_printf("Failed app1/app2 PMP setup\n");
	// 	alt_printf("a1: %d, a2: %d, a3: %d\n", a1, a2, a3);
	// 	alt_printf("a4: %d, a5: %d, a6: %d\n", a4, a5, a6);
	// 	alt_printf("b1: %d, b2: %d, b3: %d\n", b1, b2, b3);
	// 	alt_printf("b4: %d, b5: %d, b6: %d\n", b4, b5, b6);
	// }


	// TIME
	// derive a time slice capability from APP1's time (HART1) 
	// On one core (HART1) will app1 and app2 where APP1 75% of time and APP2 25% of time
	s3k_cap_delete(HART2_TIME); 		// Not using core 3
	s3k_cap_delete(HART3_TIME);			// Not using core 4

	free_cap_idx = find_free_cap();
	// debug_capability_from_idx(HART1_TIME);
														  // s3k_mk_time(hart_idx, start_time, end_time)
	s3k_cap_derive(HART0_TIME, free_cap_idx, s3k_mk_time(0, 0, S3K_SLOT_CNT / 4));
	// debug_capability_from_idx(HART1_TIME);
	// debug_capability_from_idx(free_cap_idx);

	// Move the derived time slices to app1 and app2
	s3k_err_t c1 = s3k_mon_cap_move(MONITOR, APP0_PID, free_cap_idx, APP1_PID, 3);
	// And sync the times (as tutorial.04 does)
	s3k_sync();
	
	// MONITOR
	// derive a new monitor over app1 and app2 for app1
	// How do you make a correct monitor app0?
	// s3k_err_t worked_init = s3k_cap_derive(MONITOR, free_cap_idx, s3k_mk_monitor(APP0_PID, APP1_PID));
	// alt_printf("\n");
	// debug_capability_from_idx(free_cap_idx);
	// alt_printf("\n");
	// debug_capability_from_idx(MONITOR);
	// s3k_err_t worked = s3k_mon_cap_move(MONITOR, APP0_PID, free_cap_idx, APP1_PID, 8);

	// if (worked_init != S3K_SUCCESS || worked != S3K_SUCCESS){
	// 	alt_printf("Failed worked_init: error %d\n", worked_init);
	// 	alt_printf("Failed worked: error %d\n", worked);	
	// }

	// Write start PC of app1 and app2 to PC
	s3k_mon_reg_write(MONITOR, APP1_PID, S3K_REG_PC, APP1_BASE_ADDR);
	// s3k_mon_reg_write(MONITOR, APP2_PID, S3K_REG_PC, APP2_BASE_ADDR);

	// Start app1 and app2
	s3k_mon_resume(MONITOR, APP1_PID);
	// s3k_mon_resume(MONITOR, APP2_PID);
}

int main(void)
{
	// Setup UART access
	setup_uart_app0();
	alt_printf("hello from app0\n");

	// s3k_init_malloc();
	

	setup_apps();

	while(1){
		monitor_app1();
	}

	alt_printf("leaving app0\n");
	// alt_printf("Address for heap of app1: 0x%x", &__heap_pointer);
}



