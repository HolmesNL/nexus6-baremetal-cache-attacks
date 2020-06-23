#include <smp.h>
#include <serial.h>
#include <libflush/libflush.h>
#include <eviction.h>
#include <memory.h>
#include <delay.h>
#include <test.h>
#include <coprocessor.h>
#include "eviction_tests.h"

function_mapping_t function_mapping[] = {
  { "prime_probe",  prime_probe_hit,  prime_probe_miss },
  { "evict_reload", evict_reload_hit, evict_reload_miss },
  { "flush_reload", flush_reload_hit, flush_reload_miss },
};

function_mapping_instruction_t function_mapping_instruction[] = {
  { "flush_reload_instruction", flush_reload_instruction_hit, flush_reload_instruction_miss },
  { "prime_probe_instruction", prime_probe_instruction_hit, prime_probe_instruction_miss }
};

void eviction_test(void)
{
    print("Performing eviction test\r\n");
    unsigned int start;
    unsigned int stop;
    void* pointer = (void*) SHARED_MEM_BASE;
    int n = 1000;
    int sum = 0;
    volatile unsigned int value;

    sum = 0;
    for (int i = 0; i < n; i++) {
        sum += evict_reload_hit(pointer);
    }
    print("Hit time: %u\r\n", sum/n);

    sum = 0;
    for (int i = 0; i < n; i++) {
        sum += evict_reload_miss(pointer);
    }
    print("Miss time: %u\r\n", sum/n);
}

u32 evict_reload_hit(register void* pointer)
{
    access_memory(pointer);
    register u32 start = get_timing();
    access_memory(pointer);
    return get_timing() - start;
}

u32 evict_reload_miss(register void* pointer)
{
    evict(pointer);
    register u32 start = get_timing();
    access_memory(pointer);
    return get_timing() - start;
}

void eviction_test1(void)
{
    print("Performing eviction test1\r\n");
    unsigned int start;
    void* pointer = (void*) SHARED_MEM_BASE;

    for (int i=0;i<10;i++) {
        if (i % 2 == 0) {
            evict(pointer);
        }
        start = get_timing();
        access_memory(pointer);
        print("Time: %d\r\n", get_timing() - start);
    }
}

/*
 * Test whether neighbouring addresses are cached
 */
void eviction_test_neighbour(void)
{
    u32 start;
    u32 stop;
    u32 diff;
    for (int i=0;i<512;i++) {
        start = get_timing();
        asm volatile("ldrb r0, [%0]" :: "r"(HEAP_START+i));
        stop = get_timing();
        diff = stop-start;
        if (diff > EVICTION_THRESHOLD) 
            print("Address %d was not cached\r\n", i);
    }
}

/*
 * Test whether declaring a variable caches it
 */
void eviction_test_declaration(void)
{
    int test;
    writel(get_timing(), HEAP_START); //Write somewhere else to prevent potential effects of nearby addresses caching test
    access_memory(&test);
    if (get_timing() - readl(HEAP_START) > EVICTION_THRESHOLD) {
        print("Address was not cached\r\n");
    } else {
        print("Address was cached\r\n");
    }
}

void cpu1(void)
{
    void *pointer = (void*) SHARED_MEM_BASE;
    u32 start = get_timing();
    access_memory(pointer);
    u32 stop = get_timing();
    print("Time: %d\r\n", stop-start);
}

void cross_core_eviction_test(void)
{
    print("Performing cross core eviction test\r\n");
    void *pointer = (void*) SHARED_MEM_BASE;
    for (int i=0;i<10;i++) {
        evict(pointer);
        execute_function(1, &cpu1);
        wait_second_cpu(1);
        execute_function(1,&cpu1);
        wait_second_cpu(1);
    }
}

void prime_probe_test1()
{
    print("Performing prime+probe test 1\r\n");
    void *pointer = malloc(4);
    u32 index = get_index((u32) pointer);
    u32 start;
    u32 stop;

    for(int i=0;i<10;i++) {
        print("Accessed\r\n");
        prime(index);
        access_memory(pointer);
        start = get_timing();
        probe(index);
        stop = get_timing();
        print("Time: %d\r\n", stop-start);

        print("Not accessed\r\n");
        prime(index);
        start = get_timing();
        probe(index);
        stop = get_timing();
        print("Time: %d\r\n", stop-start);
    }
}

void prime_probe_test()
{
    print("Performing prime+probe test\r\n");
    u32 sum;
    u32 n = 1000;
    void *pointer = malloc(128);
    pointer = malloc(4);
    print("Cache set: %d\r\n", get_index((u32) pointer));

    sum = 0;
    print("Accessed\r\n");
    for (int i=0;i<n;i++) {
        sum += prime_probe_miss(pointer);
    }
    print("Time: %d\r\n", sum/n);

    sum = 0;
    print("Not accessed\r\n");
    for (int i=0;i<n;i++) {
        sum += prime_probe_hit(pointer);
    }
    print("Time: %d\r\n", sum/n);
}

void prime_probe_instruction_test()
{
    print("Performing prime+probe instruction test\r\n");
    u32 sum;
    u32 n = 1000;
    void (*pointer)(void) = &do_nothing;

    sum = 0;
    print("Accessed\r\n");
    for (int i=0;i<n;i++) {
        sum += prime_probe_instruction_miss(pointer);
    }
    print("Time: %d\r\n", sum/n);

    sum = 0;
    print("Not accessed\r\n");
    for (int i=0;i<n;i++) {
        sum += prime_probe_instruction_hit(pointer);
    }
    print("Time: %d\r\n", sum/n);
}

void flush_reload_instruction_test()
{
    print("Performing flush+reload instruction test\r\n");
    u32 sum;
    u32 n = 1000;
    void (*pointer)(void) = &do_nothing;

    sum = 0;
    print("Accessed\r\n");
    for (int i=0;i<n;i++) {
        sum += flush_reload_instruction_hit(pointer);
    }
    print("Time: %d\r\n", sum/n);

    sum = 0;
    print("Not accessed\r\n");
    for (int i=0;i<n;i++) {
        sum += flush_reload_instruction_miss(pointer);
    }
    print("Time: %d\r\n", sum/n);
}

void flush_test()
{
    print("Performing flush test\r\n");
    register int start, stop;
    void* pointer = malloc(4);
    int n = 1000;
    int sum = 0;

    sum = 0;
    for (int i = 0; i < n; i++) {
        start = get_timing();
        access_memory(pointer);
        stop = get_timing();
        sum += stop-start;
    }
    print("Hit time: %u\r\n", sum/n);

    sum = 0;
    for (int i = 0; i < n; i++) {
        flush_set(get_index((u32) pointer));
        start = get_timing();
        access_memory(pointer);
        stop = get_timing();
        sum += stop-start;
    }
    print("Miss time: %u\r\n", sum/n);
}

void flush_flush()
{
    void* pointer = malloc(4);
    int index = get_index((u32) pointer);
    u32 start;
    u32 stop;
    u32 sum;
    u32 n = 1000;

    for (int j=0;j<2;j++) {
        sum = 0;
        for (int i=0; i<n;i++) {
            access_memory(pointer);
            start = get_timing();
            flush_set(index);
            stop = get_timing();
            sum += stop-start;
        }
        print("Flush after access: %d\r\n", sum/n);

        sum = 0;
        for (int i=0; i<n;i++) {
            start = get_timing();
            flush_set(index);
            stop = get_timing();
            sum += stop - start;
        }
        print("Flush after flush: %d\r\n", sum/n);
    }
}

void print_histogram(int* histogram, int histogram_size)
{
    u32 sum=0;
    for (int i=0;i<histogram_size;i++) {
        print("%d,", histogram[i]);
        sum += histogram[i];
    }
    print("\r\n");
    print("avg: %d\r\n", sum/histogram_size);
}

/*
 * @param method: index of function_mapping
 */
void histogram(int method, int histogram_size)
{
    char* name = function_mapping[method].name;
    function_t hit_function = function_mapping[method].hit_function;
    function_t miss_function = function_mapping[method].miss_function;

    void* pointer = malloc(4);
    u32 n = histogram_size;
    int histogram[histogram_size];

    for (int i=0; i<n;i++) {
        histogram[i] = hit_function(pointer);
    }
    print("%s hit\r\n", name);
    print_histogram(histogram, histogram_size);

    for (int i=0; i<n;i++) {
        histogram[i] = miss_function(pointer);
    }
    print("%s miss\r\n", name);
    print_histogram(histogram, histogram_size);

}

/*
 * @param method: index of function_mapping_instruction
 */
void histogram_instruction(int method, int histogram_size)
{
    char* name = function_mapping_instruction[method].name;
    function_instruction_t hit_function = function_mapping_instruction[method].hit_function;
    function_instruction_t miss_function = function_mapping_instruction[method].miss_function;

    void (*pointer)(void) = &do_nothing;
    u32 n = histogram_size;
    int histogram[histogram_size];

    for (int i=0; i<n;i++) {
        histogram[i] = hit_function(pointer);
    }
    print("%s hit\r\n", name);
    print_histogram(histogram, histogram_size);

    for (int i=0; i<n;i++) {
        histogram[i] = miss_function(pointer);
    }
    print("%s miss\r\n", name);
    print_histogram(histogram, histogram_size);

}

void L2_inclusive_instruction_test()
{
    print("Performing L2 inclusiveness to instructions test\r\n");
    u32 sum;
    u32 n = 1000;
    register u32 start;
    register u32 stop;
    register void (*pointer)(void) = &do_nothing;

    sum = 0;
    print("Not cleared (T1)\r\n");
    for (int i=0;i<n;i++) {
        start = get_timing();
        pointer();
        stop = get_timing();
        sum += stop-start;
    }
    print("Time: %d\r\n", sum/n);

    sum = 0;
    print("Cleared (T2)\r\n");
    for (int i=0;i<n;i++) {
        flushL2();
        //flush_instruction();
        start = get_timing();
        pointer();
        stop = get_timing();
        sum += stop-start;
    }
    print("Time: %d\r\n", sum/n);
}

void L2_inclusive_data_test()
{
    print("Performing L2 inclusiveness to data test\r\n");
    u32 sum;
    u32 n = 1000;
    register u32 start;
    register u32 stop;
    register void (*pointer)(void) = &do_nothing;

    sum = 0;
    print("Not cleared (T1)\r\n");
    for (int i=0;i<n;i++) {
        start = get_timing();
        access_memory(pointer);
        stop = get_timing();
        sum += stop-start;
    }
    print("Time: %d\r\n", sum/n);

    sum = 0;
    print("Cleared (T2)\r\n");
    for (int i=0;i<n;i++) {
        flushL2();
        start = get_timing();
        access_memory(pointer);
        stop = get_timing();
        sum += stop-start;
    }
    print("Time: %d\r\n", sum/n);
}

u32 prime_probe_hit(register void* pointer)
{
    register u32 index = get_index((u32) pointer);
    register u32 start;
    access_memory(pointer);
    prime(index);
    start = get_timing();
    probe(index);
    return get_timing() - start;
}

u32 prime_probe_miss(register void* pointer)
{
    register u32 index = get_index((u32) pointer);
    register u32 start;
    access_memory(pointer);
    prime(index);
    access_memory(pointer);
    start = get_timing();
    probe(index);
    return get_timing() - start;
}

u32 prime_probe_instruction_hit(register void (*pointer)(void))
{
    register u32 index = get_index((u32) pointer);
    register u32 start;
    pointer();
    prime(index);
    start = get_timing();
    probe(index);
    return get_timing() - start;
}

u32 prime_probe_instruction_miss(register void (*pointer)(void))
{
    register u32 index = get_index((u32) pointer);
    register u32 start;
    pointer();
    prime(index);
    pointer();
    start = get_timing();
    probe(index);
    return get_timing() - start;
}

u32 flush_reload_instruction_hit(register void (*pointer)(void))
{
    register u32 index = get_index((u32) pointer);
    register u32 start;
    flush_set(index);
    pointer();
    start = get_timing();
    pointer();
    return get_timing() - start;
}

u32 flush_reload_instruction_miss(register void (*pointer)(void))
{
    register u32 index = get_index((u32) pointer);
    register u32 start;
    flush_set(index);
    start = get_timing();
    pointer();
    return get_timing() - start;
}

u32 flush_reload_hit(register void *pointer)
{
    register u32 index = get_index((u32) pointer);
    register u32 start;
    flush_set(index);
    access_memory(pointer);
    start = get_timing();
    access_memory(pointer);
    return get_timing() - start;
}

u32 flush_reload_miss(register void *pointer)
{
    register u32 index = get_index((u32) pointer);
    register u32 start;
    flush_set(index);
    start = get_timing();
    access_memory(pointer);
    return get_timing() - start;
}
