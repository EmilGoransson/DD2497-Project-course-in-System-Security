#include "altc/altio.h"
#include "s3k/s3k.h"
#include <string.h>
#include "../utils.h"

// #include "../app1/malloc.h"
#include "../app1/canary.h"
extern int __canary_metadata_pointer;

// Created since I couldn't access the canary table from app1 directly
bool check_canary(CanaryTable* target_table){
    bool same_canary = true;

    for (size_t i = 0; i < CANARY_TABLE_ENTRIES; i++){
		// alt_printf("Checking canary at index %d\n", i);
        if(target_table->entries[i].heap_canary_pointer != 0){
            int current_val = *(target_table->entries[i].heap_canary_pointer);
            int expected_val = target_table->entries[i].canary;

            if(expected_val != current_val){
                same_canary = false;
                //alt_printf("BUFFER OVERFLOW. The canary was '%d', but now it's '%d' \n", target_table->entries[i].canary, *(target_table->entries[i].heap_canary_pointer));
            }

			//alt_printf("Checked canary at address 0x%x: expected %d, found %d\n", target_table->entries[i].heap_canary_pointer, expected_val, current_val);
        }
    }


    return same_canary;
}

void monitor_app1(){
	// Get the offset of the canary table for each app
	uint64_t canary_table_offset = (uint64_t)&__canary_metadata_pointer - APP2_BASE_ADDR;

	// Get app1's canary table location
	CanaryTable* app1_canary_table = (CanaryTable*)(APP1_BASE_ADDR + canary_table_offset);
	//alt_printf("App1 canary table located at address: 0x%x\n", app1_canary_table);

	bool same_canary = check_canary(app1_canary_table);
	if(same_canary){
		//alt_printf("All canaries intact in app1's canary table\n");
	} else {
		alt_printf("Canary check failed! Buffer overflow detected in app1's canary table\n");
		while(1){} // Stop monitorin. We should also KILL app1?
	}
}

int main(void)
{
    alt_printf("PID2\n");
	alt_printf("	0: ");
	debug_capability_from_idx(0);
	alt_printf("	1: ");
	debug_capability_from_idx(1);
	// alt_printf("	2: ");
	// debug_capability_from_idx(2);
	alt_printf("	3: ");
	debug_capability_from_idx(3);
	// alt_printf("	8: ");
	// debug_capability_from_idx(8);
    alt_printf("APP2 PRINTS\n");

	// init_canary_table();
	// s3k_init_malloc();
	while(1){
		monitor_app1();
	}

    return 0;
}
