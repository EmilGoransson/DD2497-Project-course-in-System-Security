#include "altc/altio.h"
#include "s3k/s3k.h"

uint64_t top_limit;
uint64_t bottom_limit;
// Last int or seed
uint64_t  seed;

void init_random(){

    // Defines limits
    // Grab seed from get time and set as seed
    
    seed = 0x555555555^s3k_get_time();
    alt_printf("Time, %d\n", s3k_get_time());

}
// Predictable max int = 4294967296 = 2^32, perhaps 2^64 instead
uint64_t next_random_int_v2(int top){
    alt_printf("seed, %d\n", seed);
    uint64_t a = 214013;
    uint64_t m = 4294967296;
    uint64_t c = 0;
    uint64_t number = (a * seed + c)%m;
    seed = number;
    return number % top;
}