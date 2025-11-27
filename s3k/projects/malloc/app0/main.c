#include "altc/altio.h"
#include "s3k/s3k.h"
#include <string.h>
#include "../../tutorial-commons/utils.h"
#include "../app1/malloc.h"
#include "../app1/canary.h"

extern int __heap_pointer;
extern int __canary_pointer;
extern int _end;

void setup_apps()
{
	uint64_t uart_addr = s3k_napot_encode(UART0_BASE_ADDR, 0x8);
	// Address regions for each app
	uint64_t app1_addr = s3k_napot_encode(APP1_BASE_ADDR, APP1_LENGHT);
	uint64_t app1_text_mem = s3k_napot_encode(APP1_BASE_ADDR, 4096);
	uint64_t app2_addr = s3k_napot_encode(APP2_BASE_ADDR, APP2_LENGHT);

	// MEMORY and PMP
	// Derive memory for APP1 to create the monitor process
	// This creates memory for app1 to the size of both app1 and app2
	
	// Derive a PMP capability for app1 main memory (APP1)
	uint32_t free_cap_idx = find_free_cap();
	s3k_err_t a1 = s3k_cap_derive(RAM_MEM, free_cap_idx, s3k_mk_pmp(app1_addr, S3K_MEM_RWX));
	s3k_err_t a2 = s3k_mon_cap_move(MONITOR, APP0_PID, free_cap_idx, APP1_PID, 0);
	s3k_err_t a3 = s3k_mon_pmp_load(MONITOR, APP1_PID, 0, 15);
	
	// Make .text non writable (must have higher priority ==> lower id than RWX pmp)
	s3k_cap_derive(RAM_MEM, free_cap_idx, s3k_mk_pmp(app1_text_mem, S3K_MEM_RX));
	s3k_mon_cap_move(MONITOR, APP0_PID, free_cap_idx, APP1_PID, 4);
	s3k_mon_pmp_load(MONITOR, APP1_PID, 4, 14);

	// Derive a PMP capability for app2 main memory (APP2)
	s3k_err_t a4 = s3k_cap_derive(RAM_MEM, free_cap_idx, s3k_mk_pmp(app2_addr, S3K_MEM_RWX));
	s3k_err_t a5 = s3k_mon_cap_move(MONITOR, APP0_PID, free_cap_idx, APP2_PID, 0);
	s3k_err_t a6 = s3k_mon_pmp_load(MONITOR, APP2_PID, 0, 15);


	// Derive a PMP capability for app1's heap and canary table for app2 (APP2)
	// [right now, a PMP capability over the entire memory region of app1 is given to app2]
	s3k_err_t mon_checking_err1 = s3k_cap_derive(RAM_MEM, free_cap_idx, s3k_mk_pmp(app1_addr, S3K_MEM_RX));
	s3k_err_t mon_checking_err2 = s3k_mon_cap_move(MONITOR, APP0_PID, free_cap_idx, APP2_PID, 2);
	s3k_err_t mon_checking_err3 = s3k_mon_pmp_load(MONITOR, APP2_PID, 2, 2);

	/*
		We reserve PMP slot 0 for the canary metadata section.
		A better way would be to allocate PMP slots high-->low
		so that subsections can have higher priority (lower slot).
	*/

	

	// Send ALL of RAM to APP1 (APP1)
	s3k_err_t mem2 = s3k_mon_cap_move(MONITOR, APP0_PID, RAM_MEM, APP1_PID, 2);
	

	// Derive a PMP capability for uart (APP1)
	s3k_err_t b1 = s3k_cap_derive(UART_MEM, free_cap_idx, s3k_mk_pmp(uart_addr, S3K_MEM_RW));
	s3k_err_t b2 = s3k_mon_cap_move(MONITOR, APP0_PID, free_cap_idx, APP1_PID, 1);
	s3k_err_t b3 = s3k_mon_pmp_load(MONITOR, APP1_PID, 1, 3);
	
	// Derive a PMP capability for uart (APP2)
	s3k_err_t b4 = s3k_cap_derive(UART_MEM, free_cap_idx, s3k_mk_pmp(uart_addr, S3K_MEM_RW));
	s3k_err_t b5 = s3k_mon_cap_move(MONITOR, APP0_PID, free_cap_idx, APP2_PID, 1);
	s3k_err_t b6 = s3k_mon_pmp_load(MONITOR, APP2_PID, 1, 1);

	if (a1 != S3K_SUCCESS || a2 != S3K_SUCCESS || a3 != S3K_SUCCESS ||
		a4 != S3K_SUCCESS || a5 != S3K_SUCCESS || a6 != S3K_SUCCESS ||
		b1 != S3K_SUCCESS || b2 != S3K_SUCCESS || b3 != S3K_SUCCESS ||
		b4 != S3K_SUCCESS || b5 != S3K_SUCCESS || b6 != S3K_SUCCESS){
			alt_printf("Failed app1/app2 PMP setup\n");
		alt_printf("a1: %d, a2: %d, a3: %d\n", a1, a2, a3);
		alt_printf("a4: %d, a5: %d, a6: %d\n", a4, a5, a6);
		alt_printf("b1: %d, b2: %d, b3: %d\n", b1, b2, b3);
		alt_printf("b4: %d, b5: %d, b6: %d\n", b4, b5, b6);
	}


	// TIME
	// derive a time slice capability from APP1's time (HART1) 
	// On one core (HART1) will app1 and app2 where APP1 75% of time and APP2 25% of time
	s3k_cap_delete(HART2_TIME); 		// Not using core 3
	s3k_cap_delete(HART3_TIME);			// Not using core 4

	free_cap_idx = find_free_cap();
	// debug_capability_from_idx(HART1_TIME);
														  // s3k_mk_time(hart_idx, start_time, end_time)
	s3k_cap_derive(HART1_TIME, free_cap_idx, s3k_mk_time(1, 0, S3K_SLOT_CNT / 4));
	// debug_capability_from_idx(HART1_TIME);
	// debug_capability_from_idx(free_cap_idx);

	// Move the derived time slices to app1 and app2
	s3k_err_t c1 = s3k_mon_cap_move(MONITOR, APP0_PID, HART1_TIME, APP1_PID, 3);
	s3k_err_t c4 = s3k_mon_cap_move(MONITOR, APP0_PID, free_cap_idx, APP2_PID, 3);
	// And sync the times (as tutorial.04 does)
	s3k_sync();
	
	// MONITOR
	// derive a new monitor over app1 and app2 for app1
	// How do you make a correct monitor app0?
	s3k_err_t worked_init = s3k_cap_derive(MONITOR, free_cap_idx, s3k_mk_monitor(APP0_PID, APP1_PID));
	// alt_printf("\n");
	// debug_capability_from_idx(free_cap_idx);
	// alt_printf("\n");
	// debug_capability_from_idx(MONITOR);
	s3k_err_t worked = s3k_mon_cap_move(MONITOR, APP0_PID, free_cap_idx, APP1_PID, 8);

	if (worked_init != S3K_SUCCESS || worked != S3K_SUCCESS){
		alt_printf("Failed worked_init: error %d\n", worked_init);
		alt_printf("Failed worked: error %d\n", worked);	
	}

	// Write start PC of app1 and app2 to PC
	s3k_mon_reg_write(MONITOR, APP1_PID, S3K_REG_PC, APP1_BASE_ADDR);
	s3k_mon_reg_write(MONITOR, APP2_PID, S3K_REG_PC, APP2_BASE_ADDR);

	// Start app1 and app2
	s3k_mon_resume(MONITOR, APP1_PID);
	s3k_mon_resume(MONITOR, APP2_PID);
}

int main(void)
{
	// Setup UART access
	setup_uart_app0();
	alt_printf("hello from app0\n");

	// s3k_init_malloc();

	setup_apps();
	alt_printf("leaving app0\n");
	// alt_printf("Address for heap of app1: 0x%x", &__heap_pointer);
}



