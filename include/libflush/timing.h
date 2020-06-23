#ifndef libflush_timing_included
#define libflush_timing_included

#include <asm.h>
inline __attribute__((always_inline)) unsigned int get_timing(void)
{
  register unsigned int result = 0;

  isb();dsb();
  asm volatile ("MRC p15, 0, %0, c9, c13, 0" : "=r" (result));
  isb();dsb();

  return result;
}
void timing_terminate(void);
void timing_init(void);

#endif //libflush_timing_included
