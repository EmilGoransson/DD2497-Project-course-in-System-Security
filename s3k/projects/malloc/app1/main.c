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

	uint64_t* canary_meta = (uint64_t*)(80023000);
	// Try an illigal write here
	//*canary_meta = 69;

	alt_printf("VALUE!!!!\n");
    alt_printf("OUTSIDE. SP adress in trap handler: 0x%x\n", s3k_reg_read(S3K_REG_SP));
    alt_printf("OUTSIDE. TSP adress in trap handler: 0x%x\n", s3k_reg_read(S3K_REG_TSP));
    alt_printf("OUTSIDE. ESP adress in trap handler: 0x%x\n", s3k_reg_read(S3K_REG_ESP));

	char* dynamic_ints_a = s3k_simple_malloc_random(10); // 10 104+90 = 194
	memset(dynamic_ints_a, 0, 10); // Artificiall buffer overflow
	
 	alt_printf("ALL TESTS DONE, TERMINATING \n");
}