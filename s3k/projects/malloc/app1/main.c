#include "altc/altio.h"
#include "s3k/s3k.h"
#include <string.h>
#include "../../tutorial-commons/utils.h"

#include "canary.h"
#include "malloc.h"

int heap_pointer = 0x80020267;

#define RAM_MEMM 0
#define UART_MEMM 1

#define APP2_PID 2

void setup_app2()
{
	uint64_t uart_addr = s3k_napot_encode(UART0_BASE_ADDR, 0x8);
	uint64_t app2_addr = s3k_napot_encode(0x80020000, 0x10000);
	uint64_t app1_heap_addr = s3k_napot_encode((uint64_t)&__heap_pointer, (uint64_t)&__heap_size);

	// Derive a PMP capability for app2 main memory
	uint32_t free_cap_idx = find_free_cap();
	s3k_err_t a1 = s3k_cap_derive(RAM_MEMM, free_cap_idx, s3k_mk_pmp(app2_addr, S3K_MEM_RWX));
	s3k_err_t a2 = s3k_mon_cap_move(MONITOR, APP1_PID, free_cap_idx, APP2_PID, 0);
	s3k_err_t a3 = s3k_mon_pmp_load(MONITOR, APP2_PID, 0, 0);

	// Derive a PMP capability for uart
	s3k_err_t b1 = s3k_cap_derive(UART_MEMM, free_cap_idx, s3k_mk_pmp(uart_addr, S3K_MEM_RW));
	s3k_err_t b2 = s3k_mon_cap_move(MONITOR, APP1_PID, free_cap_idx, APP2_PID, 1);
	s3k_err_t b3 = s3k_mon_pmp_load(MONITOR, APP2_PID, 1, 1);
	
	// Derive a PMP capability for reading in heap memory of app1
	s3k_err_t c1 = s3k_cap_derive(RAM_MEMM, free_cap_idx, s3k_mk_pmp(app1_heap_addr, S3K_MEM_R));
	s3k_err_t c2 = s3k_mon_cap_move(MONITOR, APP1_PID, free_cap_idx, APP2_PID, 2);
	s3k_err_t c3 = s3k_mon_pmp_load(MONITOR, APP2_PID, 2, 2);

	if (a1 != S3K_SUCCESS || a2 != S3K_SUCCESS || a3 != S3K_SUCCESS ||
		b1 != S3K_SUCCESS || b2 != S3K_SUCCESS || b3 != S3K_SUCCESS ||
		c1 != S3K_SUCCESS || c2 != S3K_SUCCESS || c3 != S3K_SUCCESS){
		alt_printf("Failed app2 PMP setup\n");
		alt_printf("a1: %d, a2: %d, a3: %d\n", a1, a2, a3);
		alt_printf("b1: %d, b2: %d, b3: %d\n", b1, b2, b3);
		alt_printf("c1: %d, c2: %d, c3: %d\n", c1, c2, c3);
	}

	// derive a time slice capability
	s3k_mon_cap_move(MONITOR, APP1_PID, 3, APP2_PID, 3);

	// // derive a new monitor over app1 for app2
	// s3k_cap_derive(MONITOR, free_cap_idx,s3k_mk_monitor(APP1_PID, APP1_PID));
	// s3k_mon_cap_move(MONITOR, APP1_PID, free_cap_idx, APP2_PID, MONITOR);

	// Write start PC of app2 to PC
	s3k_mon_reg_write(MONITOR, APP2_PID, S3K_REG_PC, 0x80030000);

	// Start app2
	s3k_mon_resume(MONITOR, APP2_PID);
}

int main(void)
{
	alt_printf("hellow from app1\n");
	
	// setup_app2();

	alt_printf("leaving app1\n");

	// s3k_init_malloc();

	// char* dynamic_ints_a = s3k_simple_malloc(10); // 10 104+90 = 194
	// print_malloc_debug_info("--- BLOCKS AFTER A ---");
	// char* dynamic_ints_b = s3k_simple_malloc(200*sizeof(char));
	// print_malloc_debug_info("--- BLOCKS AFTER B ---");
	// s3k_simple_free(dynamic_ints_b);
	// int* dynamic_ints_c = s3k_simple_malloc(4*sizeof(int));
	// print_malloc_debug_info("--- BLOCKS AFTER C ---");

	// alt_printf("Position of dyn int a: 0x%x\n\n", dynamic_ints_a);
	// alt_printf("Position of dyn int b: 0x%x\n\n", dynamic_ints_b);
	// alt_printf("Position of dyn int c: 0x%x\n\n", dynamic_ints_c);


    // alt_printf("Canary metadata pointer 0x%x\n", &__canary_metadata_pointer);
	
	// *(&__canary_metadata_pointer + 984) = 3;
	// *(&__canary_metadata_pointer + 1484) = 2;

	// init_canary_table();
	// add_canary((uint64_t*) (&__canary_metadata_pointer + 984));
	// add_canary((uint64_t*) (&__canary_metadata_pointer + 1484));
	// check_canary();
	// remove_canary((uint64_t*) (&__canary_metadata_pointer + 1484));
	// read_canary(0);

}

	// char* dynamic_ints_a = s3k_simple_malloc(10); // 10 104+90 = 194
	// print_malloc_debug_info("--- BLOCKS AFTER A ---");
	// char* dynamic_ints_b = s3k_simple_malloc(200*sizeof(char));
	// print_malloc_debug_info("--- BLOCKS AFTER B ---");
	// s3k_simple_free(dynamic_ints_b);
	// int* dynamic_ints_c = s3k_simple_malloc(4*sizeof(int));
	// print_malloc_debug_info("--- BLOCKS AFTER C ---");

	// alt_printf("Position of dyn int a: 0x%x\n\n", dynamic_ints_a);
	// alt_printf("Position of dyn int b: 0x%x\n\n", dynamic_ints_b);
	// alt_printf("Position of dyn int c: 0x%x\n\n", dynamic_ints_c);


    // alt_printf("Canary metadata pointer 0x%x\n", &__canary_metadata_pointer);
	
	// *(&__canary_metadata_pointer + 984) = 3;
	// *(&__canary_metadata_pointer + 1484) = 2;

	// init_canary_table();
	// add_canary((uint64_t*) (&__canary_metadata_pointer + 984));
	// add_canary((uint64_t*) (&__canary_metadata_pointer + 1484));
	// check_canary();
	// remove_canary((uint64_t*) (&__canary_metadata_pointer + 1484));
	// // read_canary(0);