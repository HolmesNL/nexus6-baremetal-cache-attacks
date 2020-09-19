#include <device_configuration.h>
#include <eviction.h>
#include <memory.h>
#include <serial.h>
#include <coprocessor.h>

u32 get_index(u32 address)
{
    return (address >> LINE_LENGTH_LOG2) % NUMBER_OF_SETS;
}

void eviction_init()
{
    eviction = malloc(sizeof(eviction_t));
    //print("Eviction: %x to %x\r\n", &eviction, &eviction + sizeof(eviction_t));
    for (int index=0;index<NUMBER_OF_SETS;index++) {
        congruent_address_cache_entry_t *congruent = &eviction->congruent_address_cache[index];
        for (int i=0;i<ADDRESS_COUNT;i++) {
            congruent->congruent_addresses[i] = (void *) (EVICTION_ADDRESSES_BASE + ((index + NUMBER_OF_SETS*i) << LINE_LENGTH_LOG2));
            if (get_index((u32) congruent->congruent_addresses[i]) != index) {
                print("eviction set is incorrect!\r\n");
            }
        }
    }
}

void eviction_instruction_init()
{
    eviction = malloc(sizeof(eviction_t));
    for (int index=0;index<NUMBER_OF_SETS;index++) {
        congruent_address_cache_entry_t *congruent = &eviction->congruent_address_cache[index];
        for (int i=0;i<ADDRESS_COUNT;i++) {
            u32 *address = (u32 *) (EVICTION_ADDRESSES_BASE + ((index + NUMBER_OF_SETS*i) << LINE_LENGTH_LOG2));
            *address = 0xe1a0f000; //mov pc, r0
            congruent->congruent_addresses[i] = (void *) address;
            if (get_index((u32) congruent->congruent_addresses[i]) != index) {
                print("eviction set is incorrect!\r\n");
            }
        }
    }
    dsb(); //make sure the data is written before continueing 
    flush_instruction(); //Flush instruction cache so the new value is used
}

void *get_address_in_set(u32 set)
{
    return (void *) (HEAP_START+ (set << LINE_LENGTH_LOG2));
}

void prime(register u32 index)
{
    register congruent_address_cache_entry_t *congruent_address_cache_entry = &eviction->congruent_address_cache[index];
    for (register unsigned int i = 0; i < ES_EVICTION_COUNTER; i++) {
        for (register unsigned int j = 0; j < ES_NUMBER_OF_ACCESSES_IN_LOOP; j++) {
            for (register unsigned int k = 0; k < ES_DIFFERENT_ADDRESSES_IN_LOOP; k++) {
                access_memory(congruent_address_cache_entry->congruent_addresses[i+k]);
            }
        }
    }
}

void probe(register u32 index)
{
    register congruent_address_cache_entry_t *congruent_address_cache_entry = &eviction->congruent_address_cache[index];
    for (int i=ADDRESS_COUNT -1; i>=0;i--) {
        access_memory(congruent_address_cache_entry->congruent_addresses[i]);
    }
}

void prime_instruction(register u32 index)
{
    register congruent_address_cache_entry_t *congruent_address_cache_entry = &eviction->congruent_address_cache[index];
    for (register unsigned int i = 0; i < ES_EVICTION_COUNTER; i++) {
        for (register unsigned int j = 0; j < ES_NUMBER_OF_ACCESSES_IN_LOOP; j++) {
            for (register unsigned int k = 0; k < ES_DIFFERENT_ADDRESSES_IN_LOOP; k++) {
                jump_to_memory(congruent_address_cache_entry->congruent_addresses[i+k]);
            }
        }
    }
}

void probe_instruction(register u32 index)
{
    register congruent_address_cache_entry_t *congruent_address_cache_entry = &eviction->congruent_address_cache[index];
    for (int i=ADDRESS_COUNT -1; i>=0;i--) {
        jump_to_memory(congruent_address_cache_entry->congruent_addresses[i]);
    }
}


void evict(register void* pointer)
{
    register congruent_address_cache_entry_t *congruent_address_cache_entry = &eviction->congruent_address_cache[get_index((u32) pointer)];
    for (register unsigned int i = 0; i < ES_EVICTION_COUNTER; i++) {
        for (register unsigned int j = 0; j < ES_NUMBER_OF_ACCESSES_IN_LOOP; j++) {
            for (register unsigned int k = 0; k < ES_DIFFERENT_ADDRESSES_IN_LOOP; k++) {
                access_memory(congruent_address_cache_entry->congruent_addresses[i+k]);
            }
        }
    }
}

/*
 * Flush the specified set in the L2 cache.
 * Due to the inclusiveness of the APQ8084 processor, this should also flush the same lines from the L1 cache.
 */
void flush_set1(int set) 
{
    asm volatile(
        "mov r0, #8                         \n\t"
        "mov r1, %0                         \n\t"
        "flush_loop_l2:                     \n\t"
        "subs r0, #1                        \n\t"
		"MOV  r3, #2					    \n\t"
		"ADD  r3, r3, r0, lsl #29		    \n\t"  // 31-A = 31-3 = 29
		"ADD  r3, r3, r1, lsl #7		    \n\t"  // L
		"ADD  r3, r3, #4                    \n\t"  // set cache level to L2 (a 1 in the second bit of the level field (3:1))
		"MCR p15, 0, r3, c7, c14, 2		    \n\t"  // clean and invalidate by set way
		"cmp r0, #0							\n\t"
		"BNE flush_loop_l2					\n\t"
		"DSB                                \n\t"
		:: "r"(set)
	);
}

void flushL2()
{
    asm volatile(
            "MOV R2, #8                      \n\t" //Ways counter
            "flush_loop_l2outer1:            \n\t"
            "SUBS R2,#1                      \n\t"
            "MOV R1, #2048                   \n\t" //Sets counter
                "flush_loop_l2inner1:        \n\t"
                "SUBS R1,#1                  \n\t"
                // now figure out the masking for level 2, that is
                // way 31:29, set 14:6, level 3:1
                // A = log2(associativity) = log2(8) = 3
                // L = log2(LINE_LENGTH) = 7
                // S = log2(NUMBER_OF_SETS) = 11
                // B = L + S = 18
                "MOV  R3, #2                 \n\t"
                "ADD  R3, R3, R2, lsl #29    \n\t"  // 31-A = 31-3 = 29
                "ADD  R3, R3, R1, lsl #7     \n\t"  // L
                "ADD  R3, R3, #4             \n\t"  // set cache level to L2 (a 1 in the second bit of the level field (3:1))
//              "MCR p15, 0, R3, c7, c6, 2   \n\t"   // invalidate by set way
                "MCR p15, 0, R3, c7, c14, 2  \n\t"   // clean and invalidate by set way
//              "MCR p15, 0, R3, c7, c10, 2  \n\t"   // clean by set/way
                "cmp R1, #0                  \n\t"
                "BNE flush_loop_l2inner1     \n\t"
            "cmp R2, #0                      \n\t"
            "BNE flush_loop_l2outer1         \n\t"
            "   DSB                          \n\t"
            "   ISB                          \n\t"
        );
}

void flushL1L2()
{

	// L1 16 kb, L2 2 mb
	// and also the geometry for
	// L1 is 4 way 64 set 64 byte per line a total of 16kb
	// L2 is 8 way 2048 set 128 byte per line a total of 2 mb

	//printing c9, L2 cache lock down register
	asm volatile(
			//clear the L1 first
			"MOV R2, #4							\n\t" //Ways counter
			"flush_loop_l1outer:				\n\t"
			"SUBS R2,#1							\n\t"
			"MOV R1, #64						\n\t" //Sets counter
				"flush_loop_l1inner:					\n\t"
				"SUBS R1,#1						\n\t"
				// now figure out the masking for level 0, that is
			    // way 31:30, set 12:6, level 3:1
			    // A = log2(associativity) = log2(4) = 2
			    // L = log2(LINE_LENGTH) = 6
			    // S = log2(NUMBER_OF_SETS) = 6
			    // B = L + S = 12
				"MOV R3, R2, lsl #30			\n\t" //set way on bits 31:32-A
				"ADD R3, R3, R1, lsl #6			\n\t" // 6 = LOG2(LINE_LENGTH)
				                                      // levels is kept 0
//				"MCR p15, 0, R3, c7, c6, 2  	\n\t"   // invalidate by set way
				"MCR p15, 0, R3, c7, c14, 2		\n\t"	// clean and invalidate by set way
//				"MCR p15, 0, R3, c7, c10, 2		\n\t"	// clean by set/way
				"cmp R1, #0						\n\t"
				"BNE flush_loop_l1inner				\n\t"
			"cmp R2, #0							\n\t"
			"BNE flush_loop_l1outer					\n\t"

			/*//decide to proceed to next level or not
			"MOV R1, #0							\n\t"
			"LDR R2, %0							\n\t"
			"CMP R1, R2							\n\t"
			"BEQ flush_endCacheFlush					\n\t"*/


			"MOV R2, #8							\n\t" //Ways counter
			"flush_loop_l2outer:						\n\t"
			"SUBS R2,#1							\n\t"
			"MOV R1, #2048					\n\t" //Sets counter
				"flush_loop_l2inner:					\n\t"
				"SUBS R1,#1						\n\t"
				// now figure out the masking for level 2, that is
			    // way 31:29, set 14:6, level 3:1
			    // A = log2(associativity) = log2(8) = 3
			    // L = log2(LINE_LENGTH) = 7
			    // S = log2(NUMBER_OF_SETS) = 11
			    // B = L + S = 18
				"MOV  R3, #2					\n\t"
				"ADD  R3, R3, R2, lsl #29		\n\t"  // 31-A = 31-3 = 29
				"ADD  R3, R3, R1, lsl #7		\n\t"  // L
				"ADD  R3, R3, #4                \n\t"  // set cache level to L2 (a 1 in the second bit of the level field (3:1))
//				"MCR p15, 0, R3, c7, c6, 2  	\n\t"   // invalidate by set way
				"MCR p15, 0, R3, c7, c14, 2		\n\t"	// clean and invalidate by set way
//				"MCR p15, 0, R3, c7, c10, 2		\n\t"	// clean by set/way
				"cmp R1, #0						\n\t"
				"BNE flush_loop_l2inner				\n\t"
			"cmp R2, #0							\n\t"
			"BNE flush_loop_l2outer					\n\t"
			"flush_endCacheFlush:						\n\t"
			"	DSB								\n\t"
			"	ISB								\n\t"
			:  				// output
			//:  "m"(level)	// input
			//: "r1","r2","r3","r4"
	);

}

