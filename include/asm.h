#define ISB     asm volatile ("isb sy" : : : "memory")
#define DSB     asm volatile ("dsb sy" : : : "memory")
#define DMB     asm volatile ("dmb sy" : : : "memory")

#define isb()   ISB
#define dsb()   DSB
#define dmb()   DMB
