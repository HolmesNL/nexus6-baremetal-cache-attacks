#ifndef eviction_tests_h_INCLUDED
#define eviction_tests_h_INCLUDED

#include <types.h>

#define LENGTH(x) (sizeof(x)/sizeof((x)[0]))

typedef u32 (*function_t)(void* pointer);
typedef u32 (*function_instruction_t)(void (*pointer)(void));

typedef struct function_mapping_s {
  char* name;
  function_t hit_function;
  function_t miss_function;
} function_mapping_t;

typedef struct function_mapping_instruction_s {
  char* name;
  function_instruction_t hit_function;
  function_instruction_t miss_function;
} function_mapping_instruction_t;

void eviction_test(void);
void eviction_test1(void);
void eviction_test_workings(void);
u32 evict_reload_hit(void* pointer);
u32 evict_reload_miss(void* pointer);
void cross_core_eviction_test(void);
void eviction_test_declaration(void);
void eviction_test_neighbour(void);
void prime_probe_test();
void prime_probe_instruction_test();
void flush_reload_instruction_test();
void prime_probe_test1();
void flush_test();
void flush_flush();
void histogram(int method, int histogram_size);
void histogram_instruction(int method, int histogram_size);
void L2_inclusive_instruction_test();
void L2_inclusive_data_test();
u32 prime_probe_hit(void* pointer);
u32 prime_probe_miss(void* pointer);
u32 prime_probe_instruction_miss(register void (*pointer)(void));
u32 prime_probe_instruction_hit(register void (*pointer)(void));
u32 flush_reload_instruction_hit(register void (*pointer)(void));
u32 flush_reload_instruction_miss(register void (*pointer)(void));
u32 flush_reload_hit(register void *pointer);
u32 flush_reload_miss(register void *pointer);

#endif // eviction_tests_h_INCLUDED

