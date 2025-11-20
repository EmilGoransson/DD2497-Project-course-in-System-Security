#include "altc/altio.h"
#include "s3k/s3k.h"
#include <string.h>
#include "../../tutorial-commons/utils.h"

#include "../app1/malloc.h"

#define APP0_PID 0
#define APP0_BASE_ADDR 0x80010000
#define APP0_LENGHT 0x10000

#define APP1_PID 1
#define APP1_BASE_ADDR 0x80020000
#define APP1_LENGHT 0x10000

#define APP2_PID 2
#define APP2_BASE_ADDR 0x80030000
#define APP2_LENGHT 0x10000


// See plat_conf.h
#define BOOT_PMP 0
#define RAM_MEM 1
#define UART_MEM 2
#define TIME_MEM 3
#define HART0_TIME 4
#define HART1_TIME 5
#define HART2_TIME 6
#define HART3_TIME 7
#define MONITOR 8
#define CHANNEL 9

extern int _end;

void setup_uart1(uint64_t uart_idx)
{
	uint64_t uart_addr = s3k_napot_encode(UART0_BASE_ADDR, 0x8);
	// Derive a PMP capability for accessing UART
	s3k_cap_derive(UART_MEM, uart_idx, s3k_mk_pmp(uart_addr, S3K_MEM_RW));
	// Load the derive PMP capability to PMP configuration
	s3k_pmp_load(uart_idx, 1);
	// Synchronize PMP unit (hardware) with PMP configuration
	// false => not full synchronization.
	s3k_sync_mem();
}

void setup_app1()
{
	uint64_t uart_addr = s3k_napot_encode(UART0_BASE_ADDR, 0x8);
	uint64_t app1_addr = s3k_napot_encode(APP1_BASE_ADDR, APP1_LENGHT);
	uint64_t app2_addr = s3k_napot_encode(APP2_BASE_ADDR, APP2_LENGHT);
	uint64_t app1_heap_addr = s3k_napot_encode((uint64_t)&__heap_pointer, (uint64_t)&__heap_size);

	// Derive a PMP capability for app1 main memory
	uint32_t free_cap_idx = find_free_cap();
	s3k_err_t a1 = s3k_cap_derive(RAM_MEM, free_cap_idx, s3k_mk_pmp(app1_addr, S3K_MEM_RWX));
	s3k_err_t a2 = s3k_mon_cap_move(MONITOR, APP0_PID, free_cap_idx, APP1_PID, 0);
	s3k_err_t a3 = s3k_mon_pmp_load(MONITOR, APP1_PID, 0, 0);
	
	// Derive a PMP capability for app2 main memory
	s3k_err_t a4 = s3k_cap_derive(RAM_MEM, free_cap_idx, s3k_mk_pmp(app2_addr, S3K_MEM_RWX));
	s3k_err_t a5 = s3k_mon_cap_move(MONITOR, APP0_PID, free_cap_idx, APP2_PID, 0);
	s3k_err_t a6 = s3k_mon_pmp_load(MONITOR, APP2_PID, 0, 0);

	
	
	// Derive a PMP capability for uart
	s3k_err_t b1 = s3k_cap_derive(UART_MEM, free_cap_idx, s3k_mk_pmp(uart_addr, S3K_MEM_RW));
	s3k_err_t b2 = s3k_mon_cap_move(MONITOR, APP0_PID, free_cap_idx, APP1_PID, 1);
	s3k_err_t b3 = s3k_mon_pmp_load(MONITOR, APP1_PID, 1, 1);

	// Derive a PMP capability for uart
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

	// derive a time slice capability app1
	s3k_err_t c1 = s3k_mon_cap_move(MONITOR, APP0_PID, HART1_TIME, APP1_PID, 3);
	// derive a time slice capability app2
	s3k_err_t c4 = s3k_mon_cap_move(MONITOR, APP0_PID, HART2_TIME, APP2_PID, 3);


	// // derive a new monitor over app1 and app2 for app1
	// s3k_err_t worked_init = s3k_cap_derive(MONITOR, free_cap_idx, s3k_mk_monitor(APP0_PID, APP1_PID));
	// s3k_err_t worked = s3k_mon_cap_move(MONITOR, APP0_PID, free_cap_idx, APP1_PID, 8);
	


	// if (worked_init != S3K_SUCCESS || worked != S3K_SUCCESS){
	// 	alt_printf("Failed worked_init: error %d\n", worked_init);
	// 	alt_printf("Failed worked: error %d\n", worked);	
	// }

	// Write start PC of app1 and app2 to PC
	s3k_mon_reg_write(MONITOR, APP1_PID, S3K_REG_PC, 0x80020000);
	s3k_mon_reg_write(MONITOR, APP2_PID, S3K_REG_PC, 0x80030000);

	// Start app1 and app2
	s3k_mon_resume(MONITOR, APP1_PID);
	s3k_mon_resume(MONITOR, APP2_PID);
}

int main(void)
{
	// Setup UART access
	setup_uart1(10);
	alt_printf("hello from app0\n");

	// s3k_init_malloc();

	setup_app1();
	alt_printf("leaving app0\n");
	// alt_printf("Address for heap of app1: 0x%x", &__heap_pointer);
}



