#include <serial.h>
#include <printf.h>
#include <smp.h>
#include <memory.h>
#include <libflush/memory.h>
#include <libflush/timing.h>
#include <coprocessor.h>
#include <eviction.h>
#include <mmu.h>
#include <aes.h>
#include "profiling.h"
#include "eviction_tests.h"
#include <msm_rng.h>
#include <delay.h>
#include <rsa.h>
#include <trustzone.h>

#define MPM2_MPM_PS_HOLD 0xFC4AB000

#define get_bits(x, start, stop)    ((x >> start) & ((1 << (stop-start+1)) -1))
#define get_bit(x, bit) ((x>>bit) & 1)

#include <stddef.h> /* size_t */

void _print_cache_info(u32 value)
{
    print("\tWrite-through: %d\r\n", get_bit(value, 31));
    print("\tWrite-back: %d\r\n", get_bit(value, 30));
    print("\tRead-allocation: %d\r\n", get_bit(value, 29));
    print("\tWrite-allocation: %d\r\n", get_bit(value, 28));
    print("\tNumber of sets: %d\r\n", get_bits(value, 13, 27) + 1);
    print("\tAssociativity: %d\r\n", get_bits(value, 3, 12) + 1);
    print("\tLine size: %d Bytes\r\n", (1 << ((value & 7) + 2))*4);
}

void print_cache_info()
{
    u32 value;
    for (int level=0;level<=1;level++) {
        value = cache_info(level,0);
        print("L%d Data/unified cache\r\n", level+1);
        _print_cache_info(value);
    }
    value = cache_info(0,1);
    print("L1 Instruction cache\r\n");
    _print_cache_info(value);
}

u32 read_cpsr()
{
    u32 val;
    asm volatile("MRS %0, CPSR" : "=r" (val));
    return val;
}

void print_values()
{
    print("SCTLR: %x\r\n", read_sctlr());
    print("TTBR0: %x\r\n", read_ttbr0());
    print("TTBCR: %x\r\n", read_ttbcr());
    print("MPIDR: %x\r\n", read_mpidr());
    print("ID_PFR1: %x\r\n", read_features());
}

void random_wait(int mask)
{
    u32 start = get_timing();
    while (get_timing() & mask);
    print("waited %d, %d\r\n", mask, get_timing()-start);
}

void prng_init(void)
{
    msm_rng_enable_hw();
}

void test_prng(void)
{
    u32 randomness_size = 1000;
	u32 *randomness = malloc(randomness_size * sizeof(u32));
    msm_rng_read(randomness, randomness_size*sizeof(u32));
    print_array(randomness, randomness_size);
    print("\r\n");
}

void rsa(void)
{
    RSA* r = rsa_keygen(1);
    /*print("P: %s\r\n",BN_bn2hex(r->p)); 
    print("Q: %s\r\n",BN_bn2hex(r->q)); 
    print("N: %s\r\n",BN_bn2hex(r->n)); 
    print("d: %s\r\n",BN_bn2hex(r->d)); 
    print("e: %s\r\n",BN_bn2hex(r->e));*/ 
    rsa_test(r);
}

int notmain(void *dtb, void *keymaster_bin, void *cmnlib_bin) {
    set_keymaster_bin(keymaster_bin);

    enable_second_cpu(1);
    enable_second_cpu(2);

    init_page_table();

    // CPU0
    enable_mmu();
	init_event_counters();

    //CPUs 1 and 2
    for (int i=1;i<=2;i++) {
    	execute_function(i, &enable_mmu);
    	wait_second_cpu(i);
    	execute_function(i, &init_event_counters);
    	wait_second_cpu(i);
    }

    struct msm_serial_data dev;
	dev.base = MSM_SERIAL_BASE;
	struct qseecom_command_scm_resp resp;
	
	char c;
	unsigned int rcv_sz = 0;
	int app_id = 0;

	int32_t ret = -1;
	struct qseecom_load_img_req load_img_req;
	
	ret = tz_init_sec_region(&resp);	
	check_result(ret, resp);
	
	ret = load_srv_img(0x1a5c, 0x206e0, (unsigned int)cmnlib_bin, "cmnlib", &resp);
	check_result(ret, resp);

    eviction_init(); //create the congruent addresses
    prng_init();
    print("System Booted\r\n");

	while(1) {
	
		while(!msm_serial_fetch(&dev));	//wait for character
		c = msm_serial_getc(&dev); //Get character

        //Print value
        if (c == 'p') {
            profiling();
            //execution_times();
            print("EOF\r\n");
        }
        if (c == 'a') {
            test_prng();
            print("EOF\r\n");
        }
        if (c == 'e') {
            flush_reload_instruction_test();
            prime_probe_instruction_test();
            //prime_probe_test1();
            //eviction_test();
            //flush_test();
            //L2_inclusive_data_test();
            //L2_inclusive_instruction_test();
        }
        if (c == 'x') {
            execute_function(1, &eviction_test);
            wait_second_cpu(1);
            print("execution done\r\n");
        }
        if (c == 'c') {
            print("CPU0:\r\n");
            eviction_test1();
            print("CPU1:\r\n");
            execute_function(1, &eviction_test1);
            wait_second_cpu(1);
            print("CPU2:\r\n");
            execute_function(2, &eviction_test1);
            wait_second_cpu(2);
        }
        if (c == 'f') {
            register u32 start;
            register u32 stop;
            int num_executions = 1000;
            int sum = 0;
            for (int i=0;i<num_executions;i++) {
                start = get_timing();
                flush_set(0);
                stop = get_timing();
                sum += stop - start;
            }
            print("Average flush length: %d\r\n", sum/num_executions);
        }
        if (c == 'm') {
            monitor_cache_sets();
            print("EOF\r\n");
        }
        if (c == 's') {
            rsa();
        }
        if (c == 'u') {
            //cycle_delay test
            register u32 start, stop;
            u32 num_measurements = 1000;
            u32 start_measurement = 0;
            u32 end_measurement = 5000;
            u32 step = (end_measurement-start_measurement)/num_measurements;
            int j = start_measurement;
            for (int i=0;i<=num_measurements;i++) {
                start = get_timing();
                cycle_delay(j);
                stop = get_timing();
                print("%d, %d\r\n", j, stop-start);
                j += step;
            }
            print("EOF\r\n");
        }
        if (c == 't') {
            histogram(1, 1000);
            void* pointer = malloc(4);
            u32 sum = 0;
            for (int i=0;i<1000;i++) {
                u32 start = get_timing();
                evict(pointer);
                u32 stop = get_timing();
                sum += stop - start;
            }
            print("Eviction_time:\r\n");
            print("%d\r\n", sum/1000);
            
            //histogram_instruction(1, 1000);
            print("EOF\r\n");
        }
        if (c == 'i') {
            print_values();
            execute_function(1, &print_values);
            wait_second_cpu(1);
        }
        if (c == 'w') {
            for (int i=10;i<20;i++) {
                random_wait(1 << i);
            }
        }
		//Reboot
		if (c == 'r') {
			unsigned int* hold = (unsigned int*)MPM2_MPM_PS_HOLD;
			*hold = 0;
		}
				
	} 
	return 0;
}
