#include "altc/altio.h"
#include "s3k/s3k.h"
#include <string.h>
#include "../../tutorial-commons/utils.h"

extern int __heap_pointer;
extern int __stack_pointer;
extern int __stack_size;

int main(void)
{
    alt_printf("PID2\n");
	alt_printf("	0: ");
	debug_capability_from_idx(0);
	alt_printf("	1: ");
	debug_capability_from_idx(1);
	alt_printf("	2: ");
	debug_capability_from_idx(2);
	alt_printf("	3: ");
	debug_capability_from_idx(3);
	// alt_printf("	8: ");
	// debug_capability_from_idx(8);
    alt_printf("APP2 PRINTS\n");


    alt_printf("heap pointer in app2: 0x%x\n", (uint64_t) &__heap_pointer);
    alt_printf("stack pointer in app2: 0x%x\n", (uint64_t) &__stack_pointer - (uint64_t) &__stack_size);
    return 0;
}
