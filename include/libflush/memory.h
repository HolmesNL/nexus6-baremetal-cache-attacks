#ifndef libflush_memory_included
#define libflush_memory_included

inline __attribute__((always_inline)) void access_memory(void* pointer)
{
  register unsigned int value;
  asm volatile ("LDR %0, [%1]\n\t"
      : "=r" (value)
      : "r" (pointer)
      );
}

inline __attribute__((always_inline)) void memory_barrier(void)
{
    asm volatile ("DSB");
    asm volatile ("ISB");
}

#endif //libflush_memory_included
