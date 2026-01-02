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

//Test1: Simultanious use of malloc and free
bool test1(){
	Rec rectangle ={
		.height = 10,
		.lenght = 20,
		.ratio = 2
	};
	
	for (size_t i = 0; i < 101; i++)
	{
		Rec* dynamic_ints_a = (Rec*) s3k_simple_malloc(sizeof(Rec)); 

		dynamic_ints_a->height = rectangle.height;
		dynamic_ints_a->lenght = rectangle.lenght;
		dynamic_ints_a->ratio = rectangle.ratio;

		if (i % 100 == 0)	
		{
			alt_printf("%d: Heap object\n", i);
			alt_printf("	rectangle height is: %d\n", dynamic_ints_a->height);
			alt_printf("	rectangle lenght is: %d\n", dynamic_ints_a->lenght);
			alt_printf("	rectangle ratio is: %f\n", dynamic_ints_a->ratio);
		}
		s3k_simple_free(dynamic_ints_a);
	}
	return true;
}

//Test2: Use-after-free
bool test2(){
	Rec rectangle ={
		.height = 10,
		.lenght = 20,
		.ratio = 2
	};

	Rec* dynamic_ints_a = (Rec*) s3k_simple_malloc(sizeof(Rec)); 
	dynamic_ints_a->height = rectangle.height;
	dynamic_ints_a->lenght = rectangle.lenght;
	dynamic_ints_a->ratio = rectangle.ratio;
	s3k_simple_free(dynamic_ints_a);

	// dynamic_ints_a->height = 50;
	// alt_printf("Lenght is: %d", dynamic_ints_a->height);
	return true;
}

//Test3: Excedding amount of canaries
bool test3(){
	int size = 100;
	int* data[size];
	for (size_t i = 0; i < size; i++)
	{
		data[i] = (int*) s3k_simple_malloc(sizeof(int*));
		alt_printf("%d: malloc: %d\n", i, data[i]);
	}
	for (size_t i = 0; i < size; i++)
	{
		alt_printf("%d: ", i);
		s3k_simple_free(data[i]);
	}
	return true;
}
	
//Test4: Is the canary index correct?
bool test4(){
	Rec rectangle ={
		.height = 10,
		.lenght = 20,
		.ratio = 2
	};

	Rec* rect = (Rec*) s3k_simple_malloc(sizeof(Rec));
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
	

bool test_malloc(){
	int sizes[] = {100, 100, 1, 99, 69, 1, 69, 1, 69, 1, 69, 0};
	void* malloc_blocks[sizeof(sizes)/sizeof(sizes[0]) -1];
	for(int i=0, size=sizes[0]; size != 0; i++, size=sizes[i]){
		print_malloc_debug_info("--- Malloc heap blocks ---");
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
	// Zero terminated int array.
	
	//char* dynamic_ints_a = s3k_simple_malloc_random(100);
	int i;
	for(i=0; i<100 && !test_malloc(); i++){
		alt_printf("TEST NUMBER %d\n", i);
	}

	alt_printf("-------------------------------------\n");
	alt_printf("| Ran First Set of Malloc Tests\n");
    alt_printf("| Compleated %d tests\n", i);
	alt_printf("| Canary metadata pointer 0x%x\n", &__canary_metadata_pointer);
	alt_printf("-------------------------------------\n");
	while(1){}
	if(test1())
	{
		alt_printf("Passed test1: Simultanious use of malloc and free\n");
	}
	if(test2())
	{
		alt_printf("Passed test2: Use-after-free\n");
	}
	if (test3())
	{
		alt_printf("Passed test3: Excedding amount of canaries\n");
	}
	if (test4())
	{
		alt_printf("Passed test4: Is the canary index correct\n");
	}
	


	
	//memset(dynamic_ints_a, 0, 16); // Artificiall buffer overflow

    // alt_printf("Canary metadata pointer 0x%x\n", &__canary_metadata_pointer);

	//To view the CanaryTable info
	//size((CanaryTable*)0x80023000);


	print_malloc_debug_info("--- After Malloc Heap Blocks ---");
}