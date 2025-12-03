#include "altc/altio.h"
#include "s3k/s3k.h"
#include <string.h>

#include "heap/canary.h"
#include "heap/malloc.h"
#include "heap/canary_trap.h"



int main(void)
{
	init_canary_table();
	s3k_init_malloc();
	init_canary_trap();

	char* dynamic_ints_a = s3k_simple_malloc_random(10); // 10 104+90 = 194
	char* dynamic_ints_b = s3k_simple_malloc_random(200);
	char* dynamic_ints_c = s3k_simple_malloc_random(4);
	char* dynamic_ints_d = s3k_simple_malloc_random(4);
	char* dynamic_ints_e = s3k_simple_malloc_random(200);
	memset(dynamic_ints_a, 0, 16); // Artificiall buffer overflow

    alt_printf("Canary metadata pointer 0x%x\n", &__canary_metadata_pointer);

	//print_malloc_debug_info("--- After Mallov Heap Blocks ---");
}