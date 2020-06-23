#include <asm.h>
#define ARMV7_PMCR_E       (1 << 0) /* Enable all counters */
#define ARMV7_PMCR_P       (1 << 1) /* Reset all counters */
#define ARMV7_PMCR_C       (1 << 2) /* Cycle counter reset */
#define ARMV7_PMCR_D       (1 << 3) /* Cycle counts every 64th cpu cycle */
#define ARMV7_PMCR_X       (1 << 4) /* Export to ETM */

#define ARMV7_PMCNTENSET_C (1 << 31) /* Enable cycle counter */

#define ARMV7_PMOVSR_C     (1 << 31) /* Overflow bit */


extern inline void timing_init(void)
{
    //Enable all counters and event counters
	asm volatile("mcr p15, 0, %0, c9, c12, 1" : : "r" (0x800000ff));
}

extern inline void timing_terminate(void)
{
  unsigned int value = 0;
  unsigned int mask = 0;

  // Performance Monitor Control Register
  asm volatile ("MRC p15, 0, %0, c9, c12, 0" :: "r" (value));

  mask = 0;
  mask |= ARMV7_PMCR_E; /* Enable */
  mask |= ARMV7_PMCR_C; /* Cycle counter reset */
  mask |= ARMV7_PMCR_P; /* Reset all counters */
  mask |= ARMV7_PMCR_X; /* Export */

  asm volatile ("MCR p15, 0, %0, c9, c12, 0" :: "r" (value & ~mask));
}
