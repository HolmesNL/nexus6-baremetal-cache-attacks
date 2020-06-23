#define CPU_BASE_PTR 0xF9088000  //base pointers of CPU IO memory
#define CPU0_SP 0x200000         //stack pointer of first CPU
#define CPU1_SP 0x300000         //stack pointer of second CPU
#define CPU2_SP 0x400000         //stack pointer of third CPU
#define SHARED_MEM_BASE 0x500000 //base of the shared memory, used for inter-cpu communication

#define CPU_VARS_BASE 0xaff00000 //location of all cpu-specific variables
#define CPU_VARS_SIZE 0x100      //size of cpu vars

#define get_vars_base(cpu_id)  ((cpu_vars_t*) (CPU_VARS_BASE + CPU_VARS_SIZE*cpu_id))

#ifndef __ASSEMBLY__
#include <uboot/drivers/serial/serial_msm.h>

typedef struct cpu_vars_s {
    u32 flag;
    u32 sp;
    void *function;
    u32 args[4];
} cpu_vars_t;

void _start1(void); //declaration of _start1 function from start.s

void enable_second_cpu(int cpu_id); 

int msm8974_release_secondary(unsigned long base, unsigned int cpu);

void execute_function(int cpu_id, void * function);
void execute_function_with_args(int cpu_id, void * function, u32 args[4]);

void wait_second_cpu(int cpu_id);
void print_vars(cpu_vars_t *cpu_vars);
void start_cpu1(u32 sp, u32 function);

inline int __attribute__((always_inline)) second_cpu_done(int cpu_id) 
{
    register cpu_vars_t *cpu_vars = get_vars_base(cpu_id);
    return cpu_vars->flag == 0;
}

#endif //__ASSEMBLY__
