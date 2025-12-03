#include "altc/altio.h"
#include "s3k/s3k.h"
#include <string.h>

#include "heap/utils.h"
#include "heap/canary.h"
extern int __canary_metadata_pointer;


void monitor_app1(){
	// Get the offset of the canary table for each app
	uint64_t canary_table_offset = (uint64_t)&__canary_metadata_pointer - APP2_BASE_ADDR;

	// Get app1's canary table location
	CanaryTable* app1_canary_table = (CanaryTable*)(canary_table_offset + APP1_BASE_ADDR);
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
	while(1){
		monitor_app1();
	}

    return 0;
}
