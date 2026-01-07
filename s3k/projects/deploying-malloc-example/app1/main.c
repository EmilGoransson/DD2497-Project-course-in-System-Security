#include "altc/altio.h"
#include "s3k/s3k.h"
#include <string.h>

#include "heap/canary.h"
#include "heap/malloc.h"
#include "heap/canary_trap.h"

typedef struct
{
	uint32_t lenght;
	uint32_t height;
	uint32_t area;
} Rec;

int main(void)
{
	s3k_init_malloc();
	init_canary_table();
	init_canary_trap();

	//Make a struct of data e.g a rectangle
	Rec rectangle ={
		.lenght = 20,
		.height = 10,
		.area = 200
	};

	//Allocate enough bytes on the heap for our struct
	Rec* heap_rectangle = (Rec*) s3k_simple_malloc_random(sizeof(Rec));

	//Put the stack-allocated object on the heap
	memcpy(heap_rectangle, &rectangle, sizeof(Rec));

	//Read from heap object
	alt_printf("Rectangle\n	Lenght: %d\n	Height: %d\n	Area: %d\n", heap_rectangle->lenght, heap_rectangle->height, heap_rectangle->area);

	//Release the heap object
	s3k_simple_free((void*)heap_rectangle);
}