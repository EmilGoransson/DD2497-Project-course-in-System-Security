#include "altc/altio.h"
#include "s3k/s3k.h"
#include <string.h>
#include "../../tutorial-commons/utils.h"

#include "canary.h"
#include "malloc.h"

#define RAM_MEMM 0
#define UART_MEMM 1

#define APP2_PID 2


int main(void)
{
	alt_printf("hellow from app1\n");
	
	alt_printf("PID1\n");
	alt_printf("	0: ");
	debug_capability_from_idx(0);
	alt_printf("	1: ");
	debug_capability_from_idx(1);
	alt_printf("	2: ");
	debug_capability_from_idx(2);
	alt_printf("	3: ");
	debug_capability_from_idx(3);
	alt_printf("	8: ");
	debug_capability_from_idx(8);
	
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