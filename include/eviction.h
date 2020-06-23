#ifndef eviction_included
#define eviction_included

#define EVICTION_THRESHOLD 260
#define PROBE_THRESHOLD 1420
#define EVICTION_THRESHOLD_CPU1 260

//Location of the probing memory
#define EVICTION_ADDRESSES_BASE 0xaf000000

#include <device_configuration.h>

#define ADDRESS_COUNT ((ES_EVICTION_COUNTER) + (ES_DIFFERENT_ADDRESSES_IN_LOOP) -1)

#include <types.h>
#include <libflush/libflush.h>


typedef struct congruent_address_cache_entry_s {
  void* congruent_addresses[ADDRESS_COUNT];
} congruent_address_cache_entry_t;

typedef struct eviction_s {
  congruent_address_cache_entry_t congruent_address_cache[NUMBER_OF_SETS];
} eviction_t;

eviction_t *eviction;

void evict(void* pointer);
u32 get_index(u32 address);
void flushL2();
void flushL1L2();
void prime(u32 index);
void probe(u32 index);

/*
 * Flush the specified set in the L2 cache.
 * Due to the inclusiveness of the APQ8084 processor, this should also flush the same lines from the L1 cache.
 */
inline __attribute__((always_inline)) void flush_set(int set) 
{
    register int set_index = 8;
    while (set_index--) {
        asm volatile (
    		"MOV  r3, #2					    \n\t"
    		"ADD  r3, r3, %1, lsl #29		    \n\t"  // 31-A = 31-3 = 29
    		"ADD  r3, r3, %0, lsl #7		    \n\t"  // L
    		"ADD  r3, r3, #4                    \n\t"  // set cache level to L2 (a 1 in the second bit of the level field (3:1))
    		"MCR p15, 0, r3, c7, c14, 2		    \n\t"  // clean and invalidate by set way
    		:: "r"(set), "r"(set_index) : "r3"
    	);
    }
    asm volatile ("DSB");
}

void flush_set1(int set);

void eviction_init();
void *get_address_in_set(u32 set);

#endif //eviction_included
