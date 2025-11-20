#include "altc/altio.h"
#include "s3k/s3k.h"
#include "canary.h"
#include <string.h>

#include "malloc.h"

#define APP0_PID 0
#define APP1_PID 1

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

extern int __heap_pointer;
extern int __canary_pointer;
extern int _end;

void setup_uart(uint64_t uart_idx)
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

void setup_app1(uint64_t tmp)
{
	uint64_t uart_addr = s3k_napot_encode(UART0_BASE_ADDR, 0x8);
	uint64_t app1_addr = s3k_napot_encode(0x80020000, 0x10000);

	// Derive a PMP capability for app1 main memory
	s3k_cap_derive(RAM_MEM, tmp, s3k_mk_pmp(app1_addr, S3K_MEM_RWX));
	s3k_mon_cap_move(MONITOR, APP0_PID, tmp, APP1_PID, 0);
	s3k_mon_pmp_load(MONITOR, APP1_PID, 0, 0);

	// Derive a PMP capability for uart
	s3k_cap_derive(UART_MEM, tmp, s3k_mk_pmp(uart_addr, S3K_MEM_RW));
	s3k_mon_cap_move(MONITOR, APP0_PID, tmp, APP1_PID, 1);
	s3k_mon_pmp_load(MONITOR, APP1_PID, 1, 1);

	// derive a time slice capability
	s3k_mon_cap_move(MONITOR, APP0_PID, HART1_TIME, APP1_PID, 2);

	// Write start PC of app1 to PC
	s3k_mon_reg_write(MONITOR, APP1_PID, S3K_REG_PC, 0x80020000);

	// Start app1
	s3k_mon_resume(MONITOR, APP1_PID);
}

int main(void)
{
	// Setup UART access
	setup_uart(10);

	s3k_init_malloc();
	
	char* dynamic_ints_a = s3k_simple_malloc(10); // 10 104+90 = 194
	print_malloc_debug_info("--- BLOCKS AFTER A ---");
	char* dynamic_ints_b = s3k_simple_malloc(200*sizeof(char));
	print_malloc_debug_info("--- BLOCKS AFTER B ---");
	s3k_simple_free(dynamic_ints_b);
	int* dynamic_ints_c = s3k_simple_malloc(4*sizeof(int));
	print_malloc_debug_info("--- BLOCKS AFTER C ---");

	alt_printf("Position of dyn int a: 0x%x\n\n", dynamic_ints_a);
	alt_printf("Position of dyn int b: 0x%x\n\n", dynamic_ints_b);
	alt_printf("Position of dyn int c: 0x%x\n\n", dynamic_ints_c);


    alt_printf("Canary metadata pointer 0x%x\n", &__canary_metadata_pointer);
	
	*(&__canary_metadata_pointer + 984) = 3;
	*(&__canary_metadata_pointer + 1484) = 2;

	init_canary_table();
	add_canary((uint64_t*) (&__canary_metadata_pointer + 984));
	add_canary((uint64_t*) (&__canary_metadata_pointer + 1484));
	check_canary();
	remove_canary((uint64_t*) (&__canary_metadata_pointer + 1484));
	// read_canary(0);
}
