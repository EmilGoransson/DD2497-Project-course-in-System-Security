#include "altc/altio.h"
#include "s3k/s3k.h"
#include <string.h>
#include <stdlib.h>

#include "heap/canary.h"
#include "heap/malloc.h"
#include "heap/canary_trap.h"


/*
	This file contains two
	types of tests, fragmentation tests for malloc
	and some other security tests. Change the value
	of the following define to run either of the tests.
*/
#define RUN_FRAGMENTATION_TEST 1
#define RUN_OTHER_TESTS 0

typedef struct
{
	uint8_t lenght;
	uint8_t height;
	_Float16 ratio;
} Rec;

//Test1: Simultaneous use of malloc and free
bool test1(){
	Rec rectangle ={
		.height = 10,
		.lenght = 20,
		.ratio = 2
	};
	
	for (size_t i = 0; i < 101; i++)
	{
		Rec* malloc_object = (Rec*) s3k_simple_malloc_random(sizeof(Rec)); 

		malloc_object->height = rectangle.height;
		malloc_object->lenght = rectangle.lenght;
		malloc_object->ratio = rectangle.ratio;

		if (i % 100 == 0)	
		{
			alt_printf("Heap object #%d:\n", i);
			alt_printf("	rectangle height is: %d\n", malloc_object->height);
			alt_printf("	rectangle lenght is: %d\n", malloc_object->lenght);
			alt_printf("	rectangle ratio is: %f\n", malloc_object->ratio);
		}
		s3k_simple_free(malloc_object);
	}
	return true;
}

//Test2: (run whole test twice) is the Canary randomly indexed?
//This will allocate one object with an associated canary and the canary should be randomly placed within the metadata/canary table
bool test2(){
	Rec rectangle ={
		.height = 10,
		.lenght = 20,
		.ratio = 2
	};

	Rec* rect = (Rec*) s3k_simple_malloc_random(sizeof(Rec));
	rect->height = rectangle.height;
	rect->lenght = rectangle.lenght;
	rect->ratio = rectangle.ratio;

	for (size_t i = 0; i < CANARY_TABLE_ENTRIES; i++)
	{
		alt_printf("%d:	", i);
		read_canary(i);
	}
	return true;
}

//Test3: Excedding amount of Malloc objects 
//(only 25, one malloc object will always persist so the output should show 24 indexes)
bool test3(){
	int size = 50;
	int* data[size];
	for (size_t i = 0; i < size; i++)
	{
		data[i] = (int*) s3k_simple_malloc_random(1);
		alt_printf("%d: given address: 0x%x\n", i, data[i]);
	}
	for (size_t i = 0; i < size; i++)
	{
		alt_printf("%d: ", i);
		s3k_simple_free(data[i]);
		alt_printf("\n");
	}
	return true;
}

//Test4: Use-after-free. 
//"INTEGRITY CHECK FAILED" is expected behaviour here
bool test4(){
	Rec rectangle ={
		.height = 10,
		.lenght = 20,
		.ratio = 2
	};

	//Create a malloc object
	Rec* malloc_object = (Rec*) s3k_simple_malloc_random(sizeof(Rec)); 
	malloc_object->height = rectangle.height;
	malloc_object->lenght = rectangle.lenght;
	malloc_object->ratio = rectangle.ratio;
	s3k_simple_free(malloc_object);

	//Use a malloc object after it is freed
	malloc_object->height = 50;
	alt_printf("Changed and read same object: %d\n", malloc_object->height);

	//Overwrite this malloc object (eventually) and trigger the check
	int size = 100;
	int* data[size];
	for (size_t i = 0; i < size; i++)
	{
		data[i] = (int*) s3k_simple_malloc_random(4);
	}
	//This test should not reach here since use-after-free check will occur
	for (size_t i = 0; i < size; i++)
	{
		alt_printf("%d: ", i);
		s3k_simple_free(data[i]);
		alt_printf("\n");
	}
	return true;
}

//Degramentation test
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
#if RUN_FRAGMENTATION_TEST
	int i;
	for(i=0; i<100 && !test_malloc(); i++){}

	print_malloc_debug_info_list("--- Malloc heap blocks after all tests ---");

	alt_printf("-------------------------------------\n");
	alt_printf("| Ran First Set of Malloc Tests\n");
    alt_printf("| Compleated %d tests\n", i);
	alt_printf("| Canary metadata pointer 0x%x\n", &__canary_metadata_pointer);
	alt_printf("-------------------------------------\n");
#endif
#if RUN_OTHER_TESTS
	if(test1()) alt_printf("Passed test1: Simultanious use of malloc and free\n");
	
	if(test2()) alt_printf("Passed test2: Is the canary index correct\n");

	if(test3()) alt_printf("Passed test3: Excedding amount of malloc objects\n");
	
	if(test4()) alt_printf("Passed test4: Use-after-free\n");
#endif
	alt_printf("--- Ran All Tests ---\n\n");
	print_malloc_debug_info("--- Malloc Heap Blocks After Tests ---");
}