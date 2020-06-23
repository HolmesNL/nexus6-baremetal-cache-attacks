#include "coprocessor.h"
#include <libflush/memory.h>
#include <libflush/timing.h>
#include <serial.h>
#include <perf_event_msm_krait.h>
#include <io.h>

#define ARMV7_PMCR_E       (1 << 0) /* Enable all counters */
#define ARMV7_PMCR_P       (1 << 1) /* Reset all counters */
#define ARMV7_PMCR_C       (1 << 2) /* Cycle counter reset */
#define ARMV7_PMCR_D       (1 << 3) /* Cycle counts every 64th cpu cycle */
#define ARMV7_PMCR_X       (1 << 4) /* Export to ETM */

#define ARMV7_PMCNTENSET_C (1 << 31) /* Enable cycle counter */

#define ARMV7_PMOVSR_C     (1 << 31) /* Overflow bit */

/*
 * @event: Should be one of the EVENT_* types defined in coprocessor.h
 * @index: Should be a one of the counters defined in coprocessor.h
 */
void set_event_counter(u32 event, u32 counter)
{
    // Select index PMNx counter
    asm volatile ("MCR p15, 0, %0, c9, c12, 5" :: "r" (counter)); 

    // Set the event type of the counter
	asm volatile ("mcr p15, 0, %0, c9, c13, 1\n\t" :: "r" (event));	
}

unsigned int selected_event_counter()
{
    unsigned int selected;
    asm volatile("MRC p15, 0, %0, c9, c12, 5" : "=r" (selected));
    return selected;
}

void init_event_counters()
{
    krait_pmu_init();
    krait_pmu_reset();

    krait_pmu_stop();

    krait_pmu_enable_event(L1D_CACHE, EVENT_L1D_CACHE | 0x8000000);
    krait_pmu_enable_event(L1D_REFILL, EVENT_L1D_REFILL | 0x8000000);

    timing_init();

    krait_pmu_start();
}

void reset_event_counters() 
{
    // Read current PMCR value
    unsigned int value;
    asm volatile ("MRC p15, 0, %0, c9, c12, 0" : "=r" (value));

    // Reset the counters
    value |= ARMV7_PMCR_P;
    asm volatile ("MCR p15, 0, %0, c9, c12, 0" :: "r" (value));
}

void reset_cycle_counter(int div64)
{
    unsigned int value;
    asm volatile ("MRC p15, 0, %0, c9, c12, 0" : "=r" (value));
    value |= ARMV7_PMCR_C; // Reset cycle counter to zero

    if (div64 == 1) {
    value |= ARMV7_PMCR_D; // Enable cycle count divider
    }

    // Performance Monitor Control Register
    asm volatile ("MCR p15, 0, %0, c9, c12, 0" :: "r" (value));
}

unsigned int get_event_counter(u32 counter)
{
    return armv7pmu_read_counter(counter);
}

unsigned int enabled_counters() 
{
    unsigned int status;
    asm volatile( "MRC p15, 0, %0, c9, c12, 1" : "=r"(status));
    return status;
}

void write_flags(u32 flags)
{
    asm volatile( "MCR p15, 0, r0, c9, c12, 3" :: "r" (flags));
}

/*
 * These instructions do not have any effect, so the bit is probably RAZ/WI
 * 
void data_cache_enable(void)
{
	unsigned int val;
	asm volatile ("MRC 	p15, 0, %0, c1, c0, 0\n\t": "=r" (val));
	asm volatile (
		"orr %0, %0, #4\n"
		"dsb\n"
		:: "r" (val)
	);
	asm volatile ("MCR 	p15, 0, %0, c1, c0, 0\n\t" :: "r"(val));
}

void data_cache_disable(void)
{
	unsigned int val;
	asm volatile ("MRC 	p15, 0, %0, c1, c0, 0\n\t": "=r" (val));
	asm volatile (
		"bic %0, %0, #4\n"
		"dsb\n"
		:: "r" (val)
	);
	asm volatile ("MCR 	p15, 0, %0, c1, c0, 0\n\t" :: "r"(val));
}*/

//
unsigned int read_sctlr(void)
{
	unsigned int val;
	asm volatile ("MRC 	p15, 0, %0, c1, c0, 0\n\t": "=r" (val));
	return val;
}

void write_sctlr(unsigned int val)
{
	asm volatile ("MCR 	p15, 0, %0, c1, c0, 0\n\t":: "r" (val));
}

unsigned int implemented_features(void)
{
    unsigned int val;
    asm volatile ("MRC p15, 0, %0, c9, c12, 6": "=r" (val));
    return val;
}

/*
 * Flush Data and Instruction TLBs
 */
void flush_tlbs(void)
{
    asm volatile("MCR p15, 0, %0, c8, c7, 0" :: "r" (0));
}

/*
 * ICIALLU
 */
void flush_I_BTB(void)
{
    asm volatile("MCR p15, 0, %0, c7, c5, 0" :: "r" (0));
}

void enable_cache(void)
{
    u32 val = read_sctlr();
    // Set C bit of sctlr to enable cache
    val |= (1<<2);
    // Set I bit to enable instruction cache
    val |= (1<<12);
    // Set Z bit to enable branch prediction
    val |= (1<<11);
    val |= (1<<1); //Check alignment faults
    // Set M bit of sctlr to enable MMU
    val |= 1;
    write_sctlr(val);

}

void disable_cache(void)
{
    u32 val = read_sctlr();
    
    val &= ~BIT(2); // Unset C bit
    val &= ~BIT(12);// Unset I bit
    write_sctlr(val);
}

void disable_mmu(void)
{
    u32 val = read_sctlr();
    // Unset M bit of sctlr to disable MMU
    val &= ~(1UL);
    write_sctlr(val);
}

/*
 * Show cache info
 * @param level 0 for lowest level (L1), 1 for L2
 * @param type 
 *      0 for data or unified, 
 *      1 for instruction cache
 */
u32 cache_info(u32 level, u32 type)
{
    u32 value = (level << 1) | type;
    asm volatile("MCR p15, 2, %0, c0, c0, 0" :: "r" (value));
    asm volatile("MRC p15, 1, %0, c0, c0, 0" : "=r" (value));
    return value;
}

u32 read_mpidr()
{
    u32 value;
    asm volatile("MRC p15, 0, %0, c0, c0, 5" : "=r" (value));
    return value;
}

u32 read_ttbr0()
{
    u32 value;
    asm volatile("MRC p15, 0, %0, c2, c0, 0" : "=r" (value));
    return value;
}

u32 read_ttbr1()
{
    u32 value;
    asm volatile("MRC p15, 0, %0, c2, c0, 1" : "=r" (value));
    return value;
}

void write_ttbr0(u32 value)
{
    asm volatile("MCR p15, 0, %0, c2, c0, 0" :: "r" (value));
}

void write_ttbr1(u32 value)
{
    asm volatile("MCR p15, 0, %0, c2, c0, 1" :: "r" (value));
}

u32 read_ttbcr()
{
    u32 value;
    asm volatile("MRC p15, 0, %0, c2, c0, 2" : "=r" (value));
    return value;
}

u32 read_dacr()
{
    u32 value;
    asm volatile("MRC p15, 0, %0, c3, c0, 0" : "=r" (value));
    return value;
}

void write_dacr(u32 value)
{
    asm volatile("MCR p15, 0, %0, c3, c0, 0" :: "r" (value));
}

/*
 * ID_PFR1
 */
u32 read_features()
{
    u32 value;
    asm volatile("MRC p15, 0, %0, c0, c1, 1" : "=r" (value));
    return value;
}

