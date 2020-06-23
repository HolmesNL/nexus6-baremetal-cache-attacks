#ifndef memory_h_INCLUDED
#define memory_h_INCLUDED

#include <types.h>
#include <stddef.h> /* size_t */

#define HEAP_START 0x600000

//Round to next multiple of number
#define ROUND_UP(a, number)   ((a+number-1) & ~(number-1))
#define ROUND_DOWN(a, number) (a &~(number-1))

extern void *next_free;

void *memcpy(void *dest, const void *src, size_t n);
int memcmp(const void* s1, const void* s2,size_t n);
void *memset(void *s, int c, size_t n);

void *malloc(size_t size);

#endif //memory_h_INCLUDED
