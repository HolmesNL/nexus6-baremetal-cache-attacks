#include <aes.h>
#include <serial.h>
#include <memory.h>
#include <eviction.h>
#include <smp.h>
#include <libflush/timing.h>
#include <libflush/memory.h>
#include <openssl/aes.h>
#include "profiling.h"
#include <msm_rng.h>
#include <delay.h>
#include "eviction_tests.h"
#include <rsa.h>
#include <trustzone.h>
#include <coprocessor.h>
#include <address.h> //Includes address/index to monitor

#define AES_EXECUTION_TIME 23000
#define RSA_EXECUTION_TIME 56381964

//Encrypted from "This is a message"
char signature[] = {0x65,0x5,0xd4,0xd3,0x72,0x2,0x2c,0x8a,0x8,0xc8,0x3,0x89,0x5b,0x30,0xfd,0x39,0x4b,0x26,0x10,0x35,0x5b,0xb1,0x25,0x1d,0x60,0x59,0x5e,0x1f,0xb0,0x1e,0x7e,0x4f,0xd6,0x86,0x3,0x72,0xc8,0xc1,0x90,0x1,0x76,0x5,0xd4,0x76,0xc5,0xab,0x49,0x9e,0x22,0x58,0x43,0x60,0x72,0x46,0xe4,0xe4,0x8a,0x92,0xa6,0x66,0xae,0xe8,0x81,0x3d,0xc4,0xad,0x9f,0xf0,0x53,0xbb,0xe0,0x2b,0x3,0x74,0xd2,0x7f,0xd6,0xd7,0x6,0xbf,0x76,0x51,0x47,0x3b,0x87,0x45,0xe1,0xb4,0xbf,0xa0,0x23,0x67,0x51,0x9c,0x86,0x58,0xd3,0x65,0x84,0x97,0x87,0x14,0x2b,0xd1,0x7c,0x6f,0xc3,0x5,0x2c,0x38,0xc7,0x2c,0x7e,0x49,0x5f,0xf4,0xb1,0x9c,0x75,0x4b,0xd6,0xde,0x96,0xda,0x63,0x3,0x6a,0x24,0x3b,0xe9,0x23,0xb5,0x43,0x55,0x99,0xec,0x74,0x3c,0x4d,0x2b,0x7c,0x90,0x95,0xd0,0xd9,0xc6,0xab,0x61,0x7f,0xbb,0x21,0x6d,0x86,0x84,0x6f,0x67,0x14,0x8e,0x4a,0xb0,0xd2,0xf3,0x78,0x54,0x10,0xad,0xcc,0x79,0x3a,0x85,0xeb,0xc7,0x2d,0x9f,0x62,0x1,0xea,0x87,0x73,0xb5,0xc9,0x25,0xb6,0xed,0x3c,0xac,0x28,0x8d,0xf0,0x13,0x49,0x4,0xfe,0x65,0xd,0x99,0x1f,0xa2,0x59,0xf4,0x8b,0xfb,0x30,0xe9,0x15,0x78,0xb4,0xc6,0x9,0xf4,0x9f,0x63,0x34,0x2b,0xb5,0xee,0x41,0x5,0xc5,0x6e,0xad,0xef,0x7,0x7e,0xfe,0xf7,0x8c,0x4a,0x3c,0xb3,0xee,0xe6,0x7e,0xf6,0xae,0xa3,0xc8,0xb0,0xd2,0x61,0xba,0x32,0xaf,0x7f,0xb0,0x20,0x35,0x1e,0x6e,0xf6,0xca,0x14,0xcc,0x47,0x9d,0x87};

void profile_flush_reload(int use_eviction)
{
    print("Starting profiling.\r\n");
	uint8_t in[]  = { 0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96, 0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a };
	uint8_t key[] = { 0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6, 0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c };
	uint8_t out[] = { 0x3a, 0xd7, 0x7b, 0xb4, 0x0d, 0x7a, 0x36, 0x60, 0xa8, 0x9e, 0xca, 0xf3, 0x24, 0x66, 0xef, 0x97 };
    struct AES_ctx* ctx = malloc(sizeof(struct AES_ctx));
	AES_init_ctx(ctx, key);

	//Profiling stuff
	u32 start;
	void* pointer = &ctx->RoundKey;
	void* pointer1 = (void *) ROUND_DOWN((u32) pointer,128)+128; 
	u32 index = get_index((u32) pointer);
	u32 index1 = get_index((u32) pointer1);
	u32 result[1000];
	u32 result1[1000];
    int i = 0;

    if (use_eviction) {
        evict(pointer);
        evict(pointer1);
    } else {
        flush_set(index);
        flush_set(index1);
    }

    u32 args[4] = {(u32) ctx, (u32) in};
    execute_function_with_args(2, &AES_ECB_encrypt, args);

    while (!second_cpu_done(2)) {
        start = get_timing();
        access_memory(pointer);
        result[i] = get_timing() - start < EVICTION_THRESHOLD;

        start = get_timing();
        access_memory(pointer1);
        result1[i] = get_timing() - start < EVICTION_THRESHOLD;

        i++;
        if (use_eviction) {
            evict(pointer);
            evict(pointer1);
        } else {
            flush_set(index);
            flush_set(index1);
        }
    }
    for (int j=0;j<i;j++) {
        print("Results: %d, %d\r\n", result[j], result1[j]);
    }

    if (0 == memcmp((char*) out, (char*) in, 16)) {
        print("Success!\r\n");
    } else {
        print("Failure!\r\n");
        print("Expected:\r\n");
        print_aes(out);
        print("Actual:\r\n");
        print_aes(in);
    }

}

void test()
{
    //Time of AES on second cpu
    cycle_delay(AES_EXECUTION_TIME/5);
}

void rsa_replacement()
{
    cycle_delay(RSA_EXECUTION_TIME/2);
}

void test1(const unsigned char *in, unsigned char *out, const AES_KEY *key)
{
    AES_encrypt(in, out, key);
    AES_encrypt(in, out, key);
}

void profile_flush_reload_openssl()
{
    print("Starting profiling.\r\n");
	uint8_t in[]  = { 0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96, 0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a };
	uint8_t userkey[] = { 0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6, 0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c };
	uint8_t out[] = { 0x3a, 0xd7, 0x7b, 0xb4, 0x0d, 0x7a, 0x36, 0x60, 0xa8, 0x9e, 0xca, 0xf3, 0x24, 0x66, 0xef, 0x97 };
	u32 bits = 128;
	u32 num_samples = 100;
	u32 num_profiles = 1000;

    AES_KEY* key = malloc(sizeof(AES_KEY));
	AES_set_encrypt_key(userkey, bits, key);

	u32 randomness_size = num_profiles;
	u32 *randomness = malloc(randomness_size * sizeof(u32));
	//Make sure that enough random data is read
	char *randomness_pointer = (char *) randomness;
	int read = 0;
	while (read < randomness_size * sizeof(u32)) {
        read += msm_rng_read(randomness_pointer, randomness_size * sizeof(u32));
        randomness_pointer += read;
	}
    u32 random_index = 0;

	//Profiling stuff
	register u32 start;
	register u32 stop;
	register u32 start1;
	register u32 stop1;
	register u32 start_execution;
	u32 stop_execution;
	void* pointer = &key->rd_key;
	void* pointer1 = (void *) ROUND_DOWN((u32) pointer,128)+128; 
	print("pointer: %x, pointer1: %x, keys in first cache line: %d\r\n", pointer, pointer1, ((u32) pointer1 - (u32) pointer)/16);
	register u32 index = get_index((u32) pointer);
	register u32 index1 = get_index((u32) pointer1);
	u32 result[num_samples];
	u32 result1[num_samples];
	register u32 measure_time;
	register u32 avg_execution_time = AES_EXECUTION_TIME;
	u32 avg_window = 2844;
	register u32 sample_index;
	memset(result, 0, num_samples*4);
	memset(result1, 0, num_samples*4);

    for (int times=0;times<num_profiles;times++) {
        int i = 0;
    	uint8_t in[]  = { 0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96, 0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a };

        flush_set(index);
        flush_set(index1);

        u32 args[4] = {(u32) in, (u32) in, (u32) key};
        execute_function_with_args(2, &AES_encrypt, args);
        start_execution = get_timing();

        cycle_delay(randomness[random_index++] % 2000); // (avg_execution_time / (num_measurements per execution))
        do {
            start = get_timing();
            access_memory(pointer);
            stop = get_timing();

            start1 = get_timing();
            access_memory(pointer1);
            stop1 = get_timing();

            measure_time = stop1 - start_execution;
            sample_index = measure_time*num_samples/avg_execution_time;
            if (sample_index >= num_samples) 
                sample_index = num_samples-1;

            result[sample_index]  += (stop  - start  < EVICTION_THRESHOLD);
            result1[sample_index] += (stop1 - start1 < EVICTION_THRESHOLD);

            i++;

            flush_set(index);
            flush_set(index1);
        } while (!second_cpu_done(2));
        stop_execution = get_timing();
        //avg_execution_time = (avg_execution_time*(times+1) + stop_execution - start_execution) / (times + 2);

    }
    //print("avg_window: %d\r\n", avg_window);
    for (u32 j=0;j<num_samples;j++) {
        print("%d, %d\r\n", result[j], result1[j]);
    }
    print("Average execution time: %d\r\n", avg_execution_time);

}

void profile_prime_probe_openssl1()
{
    print("Starting profiling.\r\n");
	uint8_t in[]  = { 0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96, 0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a };
	uint8_t userkey[] = { 0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6, 0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c };
	uint8_t out[] = { 0x3a, 0xd7, 0x7b, 0xb4, 0x0d, 0x7a, 0x36, 0x60, 0xa8, 0x9e, 0xca, 0xf3, 0x24, 0x66, 0xef, 0x97 };
	u32 bits = 128;
	u32 num_samples = 100;
	u32 num_profiles = 1000;

    AES_KEY* key = malloc(sizeof(AES_KEY));
	AES_set_encrypt_key(userkey, bits, key);

	u32 randomness_size = num_profiles;
	u32 *randomness = malloc(randomness_size * sizeof(u32));
	char *randomness_pointer = (char *) randomness;
	//Make sure that enough random data is read
	int read = 0;
	while (read < randomness_size * sizeof(u32)) {
        read += msm_rng_read(randomness_pointer, randomness_size * sizeof(u32) - read);
        randomness_pointer += read;
	}
    u32 random_index = 0;

	//Profiling stuff
	register u32 start;
	register u32 stop;
	register u32 start1;
	register u32 stop1;
	register u32 start_execution;
	u32 stop_execution;
	void* pointer = &key->rd_key;
	void* pointer1 = (void *) ROUND_DOWN((u32) pointer,128)+128; 
	print("pointer: %x, pointer1: %x, keys in first cache line: %d\r\n", pointer, pointer1, ((u32) pointer1 - (u32) pointer)/16);
	register u32 index = get_index((u32) pointer);
	register u32 index1 = get_index((u32) pointer1);
	u32 result[num_samples];
	u32 result1[num_samples];
	register u32 measure_time;
	register u32 avg_execution_time = AES_EXECUTION_TIME;
	u32 avg_window = 2844;
	register u32 sample_index;
	memset(result, 0, num_samples*4);
	memset(result1, 0, num_samples*4);
    for (int times=0;times<num_profiles;times++) {
        int i = 0;
    	uint8_t in[]  = { 0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96, 0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a };


        prime(index);
        prime(index1);

        u32 args[4] = {(u32) in, (u32) in, (u32) key};
        execute_function_with_args(2, &AES_encrypt, args);
        start_execution = get_timing();

        cycle_delay(randomness[random_index++] % 4000); // (avg_execution_time / (num_measurements per execution))
        do {
            start = get_timing();
            probe(index);
            stop = get_timing();

            start1 = get_timing();
            probe(index1);
            stop1 = get_timing();

            measure_time = stop1 - start_execution;
            sample_index = measure_time*num_samples/avg_execution_time;
            if (sample_index >= num_samples) 
                sample_index = num_samples-1;

            result[sample_index]  += (stop  - start  > PROBE_THRESHOLD);
            result1[sample_index] += (stop1 - start1 > PROBE_THRESHOLD);

            i++;

            prime(index);
            prime(index1);
        } while (!second_cpu_done(2));
        stop_execution = get_timing();
        //avg_execution_time = (avg_execution_time*(times+1) + stop_execution - start_execution) / (times + 2);

    }
    //print("avg_window: %d\r\n", avg_window);
    for (u32 j=0;j<num_samples;j++) {
        print("%d, %d\r\n", result[j], result1[j]);
    }
    print("Average execution time: %d\r\n", avg_execution_time);

}

void profile_flush_reload_openssl1()
{
    print("Starting profiling.\r\n");
	uint8_t in[]  = { 0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96, 0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a };
	uint8_t userkey[] = { 0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6, 0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c };
	uint8_t out[] = { 0x3a, 0xd7, 0x7b, 0xb4, 0x0d, 0x7a, 0x36, 0x60, 0xa8, 0x9e, 0xca, 0xf3, 0x24, 0x66, 0xef, 0x97 };
	u32 bits = 128;
    AES_KEY* key = malloc(sizeof(AES_KEY));
	AES_set_encrypt_key(userkey, bits, key);

	//Profiling stuff
	register u32 start, stop;
	void* pointers[2];
	pointers[0] = &key->rd_key;
	pointers[1] = (void *) ROUND_DOWN((u32) pointers[0],128)+128; 
	u32 indexes[2];
	indexes[0] = get_index((u32) pointers[0]);
	indexes[1] = get_index((u32) pointers[1]);
	
	u32 result[1000];
	u32 result1[1000];
	memset(result, 0, 1000*4);
	memset(result1, 0, 1000*4);

	u32 *results[2] = {result, result1};
	u32 index[2] = {0, 0};
    u32 args[4] = {(u32) in, (u32) in, (u32) key};

    u32 diffs[1000];
    u32 diff_index = 0;

    for (int j=0;j<=1;j++) {
        int i = index[j];
        void* pointer = pointers[j];
        u32 set = indexes[j];
        flush_set(set);

        execute_function_with_args(1, &AES_encrypt, args);

        while (!second_cpu_done(1)) {
            start = get_timing();
            access_memory(pointer);
            stop = get_timing();
            //diffs[diff_index++] = stop - start;
            results[j][i] = stop - start < EVICTION_THRESHOLD;

            i++;
            flush_set(set);
        }
        index[j] = i;
    }
    int num_results = index[0];
    if (index[1] > num_results) {
        num_results = index[1];
    }
    for (int j=0;j<num_results;j++) {
        print("Results: %d, %d\r\n", result[j], result1[j]);
    }
    print("Num results: %d, %d\r\n", index[0], index[1]);
    //print_array(diffs, 1000);
    //print("\r\n");

}
void profile_prime_probe_openssl()
{
    print("Starting profiling.\r\n");
	uint8_t in[]  = { 0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96, 0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a };
	uint8_t userkey[] = { 0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6, 0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c };
	uint8_t out[] = { 0x3a, 0xd7, 0x7b, 0xb4, 0x0d, 0x7a, 0x36, 0x60, 0xa8, 0x9e, 0xca, 0xf3, 0x24, 0x66, 0xef, 0x97 };
	u32 bits = 128;
    AES_KEY* key = malloc(sizeof(AES_KEY));
	AES_set_encrypt_key(userkey, bits, key);

	//Profiling stuff
	u32 start, stop;
	void* pointers[2];
	pointers[0] = &key->rd_key;
	pointers[1] = (void *) ROUND_DOWN((u32) pointers[0],128)+128; 
	
	u32 result[1000];
	u32 result1[1000];
	memset(result, 0, 1000*4);
	memset(result1, 0, 1000*4);

	u32 *results[2] = {result, result1};
	u32 index[2] = {0, 0};
    u32 args[4] = {(u32) in, (u32) in, (u32) key};

    u32 diffs[1000];
    u32 diff_index = 0;

    for (int j=0;j<=1;j++) {
        int i = index[j];
        void* pointer = pointers[j];
        u32 set = get_index((u32) pointer);
        prime(set);

        execute_function_with_args(1, &AES_encrypt, args);

        while (!second_cpu_done(1)) {
            start = get_timing();
            probe(set);
            stop = get_timing();
            //diffs[diff_index++] = stop - start;
            results[j][i] = stop - start > PROBE_THRESHOLD;

            i++;
            prime(set);
        }
        index[j] = i;
    }
    int num_results = index[0];
    if (index[1] > num_results) {
        num_results = index[1];
    }
    for (int j=0;j<num_results;j++) {
        print("Results: %d, %d\r\n", result[j], result1[j]);
    }
    print("Num results: %d, %d\r\n", index[0], index[1]);
    //print_array(diffs, 1000);
    //print("\r\n");
}

void monitor_cache_sets()
{
	uint8_t in[]  = { 0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96, 0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a };
	uint8_t userkey[] = { 0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6, 0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c };
	u32 bits = 128;

    AES_KEY* key = malloc(sizeof(AES_KEY));
	AES_set_encrypt_key(userkey, bits, key);
    u32 args[4] = {(u32) in, (u32) in, (u32) key};
    void* function[2] = {&test, &AES_encrypt};

	u32 start;
	u32 diff;
	u32 used_by_process;

    u32 num_samples = 200;
    u32 used[NUMBER_OF_SETS];
    u32 used_aes[NUMBER_OF_SETS];
    memset(used, 0, NUMBER_OF_SETS*sizeof(u32));
    memset(used_aes, 0, NUMBER_OF_SETS*sizeof(u32));
    for (int function_index=0;function_index <= 1; function_index++) {
        for (int i=0;i<NUMBER_OF_SETS;i++) {
            void *address = get_address_in_set(i);
            u32 index = get_index((u32) address);
            for (int j=0;j<num_samples;j++) {
                prime(index);

                execute_function_with_args(1, function[function_index], args);
                wait_second_cpu(1);

                start = get_timing();
                probe(index);
                diff = get_timing() - start;
                //print("Diff: %d\r\n", diff);
                used_by_process = diff > PROBE_THRESHOLD; //address was used by other function
                if (function_index == 0) {
                    used[i] += used_by_process;
                } else {
                    used_aes[i] += used_by_process;
                }
            }
        }
    }
    print_array(used, NUMBER_OF_SETS);
    print("\r\n");
    print_array(used_aes, NUMBER_OF_SETS);
    print("\r\n");
}

void execution_times()
{
	uint8_t in[]  = { 0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96, 0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a };
	uint8_t userkey[] = { 0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6, 0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c };
	uint8_t out[] = { 0x3a, 0xd7, 0x7b, 0xb4, 0x0d, 0x7a, 0x36, 0x60, 0xa8, 0x9e, 0xca, 0xf3, 0x24, 0x66, 0xef, 0x97 };
	u32 bits = 128;
	char second_cpu = 1;
	u32 num_samples = 10000;

	register u32 start;
	u32 times[num_samples];

    AES_KEY* key = malloc(sizeof(AES_KEY));
	AES_set_encrypt_key(userkey, bits, key);

    for (int i=0;i<num_samples;i++) {
        if (second_cpu) {
            u32 args[4] = {(u32) in, (u32) in, (u32) key};
            execute_function_with_args(2, &AES_encrypt, args);
            start = get_timing();
            wait_second_cpu(2);
            times[i] = get_timing() - start;
        } else {
            start = get_timing();
            AES_encrypt(in, in, key);
            times[i] = get_timing() - start;
        }
    }
    print_array(times, num_samples);
    print("\r\n");
}

void rsa_execution_time(u32 second_cpu)
{
    RSA *rsa = rsa_keygen(1);
    unsigned char result[RSA_size(rsa)];
    unsigned char to[RSA_size(rsa)];
    memset(result, 0, RSA_size(rsa));
    memcpy(to, signature, RSA_size(rsa));

    u32 num_executions = 1000;
	register u32 start;
	u32 args[4] = {sizeof(to), (u32) to, (u32) result, (u32) rsa};
	u32 times[num_executions];
    //RSA_private_encrypt(sizeof(from), from, to, rsa, 0);
    //print_byte_array((char *) to, sizeof(to));
    //print("\r\n");

	for (int i=0; i< num_executions; i++) {
    	if (second_cpu) {
        	start = get_timing();
            execute_function_with_args(1, &RSA_public_decrypt, args);
            wait_second_cpu(1);
            times[i] = get_timing() - start;
    	} else {
        	start = get_timing();
            RSA_public_decrypt(sizeof(to), to, result, rsa, 0);
            times[i] = get_timing() - start;
    	}
        //print("Result: %s\r\n", result);
	}
	print_array(times, num_executions);
	print("\r\n");
}

void profile_rsa() 
{
    void *address = &BN_mod_exp_mont + 1188; //Start of the for loop
    u32 index = get_index((u32) address);
    print("Address: %x, Index: %d\r\n", address, index);

    RSA *rsa = rsa_keygen(1);
    unsigned char result[RSA_size(rsa)];
    unsigned char to[RSA_size(rsa)];
    memset(result, 0, RSA_size(rsa));
    memcpy(to, signature, RSA_size(rsa));

    register u32 start;
    register u32 stop;
	u32 args[4] = {sizeof(to), (u32) to, (u32) result, (u32) rsa};
	u32 num_samples = 100000;
	u32 samples[num_samples];
	u32 i = 0;

    prime(index);
    execute_function_with_args(1, &RSA_public_decrypt, args);

    do {
        start = get_timing();
        probe(index);
        stop = get_timing();

        samples[i] = stop - start > PROBE_THRESHOLD;

        prime(index);
        i++;
    } while(!second_cpu_done(1));

    print_array(samples, i);
    print("\r\n");
    //print("i: %d\r\n", i);
}

void profiling_load_trustlet()
{
	struct qseecom_command_scm_resp resp;
    int ret;
    ret = load_trustlet(0x1a5c, 0x7f1c, (unsigned int)keymaster_bin, KEYMASTER_NAME, &resp);
    //check_result(ret, resp);
}

void execution_time_rsa_trustzone()
{
    execute_function(1, &profiling_load_trustlet);
    u32 start = get_timing();
    wait_second_cpu(1);
    u32 stop = get_timing();
    print("Loading trustlet time: %d\r\n", stop - start);
}

void load_trustlet_replacement() 
{
    cycle_delay(10025192/2);
}

void profile_rsa_trustzone()
{
    register u32 index = INDEX_TO_MONITOR;

    register u32 start;
    register u32 stop;
    u32 num_samples = 10000;
    u32 samples[num_samples];
    //print("samples: %x to %x\r\n", samples, samples + num_samples);
    register u32 i = 0;

    prime(index);
    execute_function(1, &profiling_load_trustlet);
    //execute_function(1, &load_trustlet_replacement);

    do {
        start = get_timing();
        probe(index);
        stop = get_timing();

        samples[i] = stop - start;

        prime(index);
        //cycle_delay(1200); //Prevent doing more than 100000 measurements
        i++;
    } while(!second_cpu_done(1));

    print_array(samples, i);
    print("\r\n");
    //print("i: %d\r\n", i);
}

void profile_rsa_trustzone_gaps()
{
    register u32 index = INDEX_TO_MONITOR;

    register u32 start;
    register u32 stop;
	u32 num_samples = 10000;
	u32 samples[num_samples];
	//print("samples: %x to %x\r\n", samples, samples + num_samples);
	register u32 i = 0;
	register u32 prev;

    prime(index);
    execute_function(1, &profiling_load_trustlet);
    //execute_function(1, &load_trustlet_replacement);
    //cycle_delay(30000000);
    prev = get_timing();

    do {
        start = get_timing();
        probe(index);
        stop = get_timing();

        if (stop - start > 1200) {
            samples[i++] = stop - prev;
            prev = stop;
        }
        prime(index);
        //cycle_delay(1500); //Prevent doing more than 100000 measurements
    } while(!second_cpu_done(1));

    print_array(samples, i);
    print("\r\n");
    //print("i: %d\r\n", i);
}

void profiling()
{
    //profile_flush_reload_openssl1();
    //profile_prime_probe_openssl1();
    //profile_flush_reload_openssl();
    //profile_flush_reload(0);
    //rsa_execution_time(1);
    //profile_rsa();
    //execution_time_rsa_trustzone();
    //profile_rsa_trustzone();
    profile_rsa_trustzone_gaps();
}

