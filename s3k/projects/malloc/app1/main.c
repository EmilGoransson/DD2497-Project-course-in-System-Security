#include "altc/altio.h"
#include "s3k/s3k.h"
#include <string.h>

#include "heap/canary.h"
#include "heap/malloc.h"
#include "heap/canary_trap.h"

bool test_malloc(){
	int sizes[] = {1, 2, 0};
	void* malloc_blocks[sizeof(sizes)/sizeof(sizes[0]) -1];
	for(int i=0, size=sizes[0]; size != 0; i++, size=sizes[i]){
		void* ptr = s3k_simple_malloc_random(size);
		malloc_blocks[i] = ptr;
		if(ptr==(void*)0){
			alt_printf("MALLOC ERROR, GOT NULLPTR!\n");
			return true;
		}
	}
	for(int i=0; i<sizeof(malloc_blocks)/sizeof(malloc_blocks[0]); i++){
		s3k_simple_free(malloc_blocks[i]);
	}
	return false;
}

int main(void)
{
	init_canary_table();
	s3k_init_malloc();
	init_canary_trap();
	// Zero terminated int array.
	
	//char* dynamic_ints_a = s3k_simple_malloc_random(100);
	int i;
	for(i=0; i<1000 && !test_malloc(); i++){}
    alt_printf("Ran %d tests of malloc\n", i);
	alt_printf("Canary metadata pointer 0x%x\n", &__canary_metadata_pointer);

	//To view the CanaryTable info
	//size((CanaryTable*)0x80023000);

	//print_malloc_debug_info("--- After Mallov Heap Blocks ---");
}