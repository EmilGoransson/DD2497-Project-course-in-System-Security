#include "altc/altio.h"
#include "s3k/s3k.h"
#include <string.h>
#include <stdlib.h>

#include "heap/canary.h"
#include "heap/malloc.h"
#include "heap/canary_trap.h"


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

		/*
		if (i % 100 == 0)	
		{
			alt_printf("Heap object #%d:\n", i);
			alt_printf("	rectangle height is: %d\n", malloc_object->height);
			alt_printf("	rectangle lenght is: %d\n", malloc_object->lenght);
			alt_printf("	rectangle ratio is: %f\n", malloc_object->ratio);
		}*/
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
	/*
	for (size_t i = 0; i < CANARY_TABLE_ENTRIES; i++)
	{
		alt_printf("%d:	", i);
		read_canary(i);
	}
	*/
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
		//alt_printf("%d: given address: 0x%x\n", i, data[i]);
	}
	for (size_t i = 0; i < size; i++)
	{
		//alt_printf("%d: ", i);
		s3k_simple_free(data[i]);
		//alt_printf("\n");
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
	// We allocate a large block to make it more likely to be allocated
	// later, showing that the integrity check works
	Rec* malloc_object = (Rec*) s3k_simple_malloc_random(100); 
	malloc_object->height = rectangle.height;
	malloc_object->lenght = rectangle.lenght;
	malloc_object->ratio = rectangle.ratio;
	s3k_simple_free(malloc_object);

	//Use a malloc object after it is freed
	malloc_object->height = 50;
	alt_printf("Changed and read same object: %d\n", malloc_object->height);

	alt_printf("Allocating memory that has been written to, expecting integrity check to fail\n");

	//Overwrite this malloc object (eventually) and trigger the check
	int size = 100;
	int* data[size];
	for (size_t i = 0; i < size; i++)
	{
		data[i] = (int*) s3k_simple_malloc_random(100);
	}
	//This test should not reach here since use-after-free check will occur
	for (size_t i = 0; i < size; i++)
	{
		//alt_printf("%d: ", i);
		s3k_simple_free(data[i]);
		//alt_printf("\n");
	}
	return true;
}

int main(void)
{
	s3k_init_malloc();
	init_canary_table();
	init_canary_trap();

	if(test1()) alt_printf("Passed test1: Simultanious use of malloc and free\n");
	
	if(test2()) alt_printf("Passed test2: Is the canary index correct\n");

	if(test3()) alt_printf("Passed test3: Excedding amount of malloc objects\n");
	
	if(test4()) alt_printf("Passed test4: Use-after-free\n");

	alt_printf("--- Ran All Tests ---\n\n");
	print_malloc_debug_info("--- Malloc Heap Blocks After Tests ---");
}