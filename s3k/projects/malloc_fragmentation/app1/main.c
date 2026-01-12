#include "altc/altio.h"
#include "s3k/s3k.h"
#include <string.h>
#include <stdlib.h>

#include "heap/canary.h"
#include "heap/malloc.h"
#include "heap/canary_trap.h"

//Fragmentation test
//Tests allocationg different amount of bytes to see how well malloc performs
//Performance impact = find big enough space for new malloc object 
bool test_malloc(){
	int sizes[] = {100, 100, 1, 99, 69, 1, 69, 1, 69, 1, 69, 0};
	void* malloc_blocks[sizeof(sizes)/sizeof(sizes[0]) -1];
	for(int i=0, size=sizes[0]; size != 0; i++, size=sizes[i]){
		//print_malloc_debug_info_list("--- Malloc heap blocks ---");
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
	s3k_init_malloc();
	init_canary_table();
	init_canary_trap();

	int i;
	for(i=0; i<100 && !test_malloc(); i++){}

	alt_printf("-------------------------------------\n");
	alt_printf("| Ran First Set of Malloc Tests\n");
    alt_printf("| Compleated %d tests\n", i);
	alt_printf("| Canary metadata pointer 0x%x\n", &__canary_metadata_pointer);
	alt_printf("-------------------------------------\n");
	alt_printf("--- Ran All Tests ---\n\n");
	print_malloc_debug_info("--- Malloc Heap Blocks After Tests ---");
}