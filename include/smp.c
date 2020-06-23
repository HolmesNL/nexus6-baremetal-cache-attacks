#include <smp.h>
#include <scm.h>
#include <krait-regulator.h>
#include <memory.h>
#include <coprocessor.h>
#include <serial.h>

extern void _start1(void); //declaration of _start1 function from start.s

void enable_second_cpu(int cpu_id) 
{
	struct {
		unsigned int flags;
		unsigned long addr;
	} cmd;

	cmd.addr = (unsigned long) &_start1;
	cmd.flags = 1 | 8; //Enable CPU1 and CPU2 in cold boot mode

	struct scm_response scm_response_buf;

	int res = scm_call(1, 1,
			&cmd, sizeof(cmd), &scm_response_buf, sizeof(scm_response_buf));

    cpu_vars_t *cpu_vars = get_vars_base(cpu_id);
    switch (cpu_id) {
        case 0:
            cpu_vars->sp = CPU0_SP;
            break;
        case 1:
            cpu_vars->sp = CPU1_SP;
            break;
        case 2:
            cpu_vars->sp = CPU2_SP;
            break;
    }
	msm8974_release_secondary(CPU_BASE_PTR, cpu_id);
}

int msm8974_release_secondary(unsigned long base, unsigned int cpu)
{

    unsigned int base_ptr = base + (cpu * 0x10000);

	secondary_cpu_hs_init(base_ptr, cpu);

    unsigned int write_address = base_ptr + 4;

	writel(0x021,write_address);
	writel(0x020,write_address);
	writel(0x000,write_address);
	writel(0x080,write_address);
	return 0;
}

void execute_function(int cpu_id, void * function) 
{
    cpu_vars_t *cpu_vars = get_vars_base(cpu_id);
    cpu_vars->function = function;
    cpu_vars->flag = 1;
}

void execute_function_with_args(int cpu_id, void * function, u32 args[4])
{
    cpu_vars_t *cpu_vars = get_vars_base(cpu_id);
    memcpy(cpu_vars->args, args, 4*sizeof(u32));
    execute_function(cpu_id, function);
}

void wait_second_cpu(int cpu_id) 
{
    while (!second_cpu_done(cpu_id));
}

void start_cpu1(u32 sp, u32 function)
{
    print("sp: %x, function: %x\r\n", sp, function);
}

void print_vars(cpu_vars_t *cpu_vars)
{
    print("address: %x\r\n", (u32) cpu_vars);
    print(
        "cpu_vars:\r\n"
        "\tflag: %x\r\n"
        "\tsp: %x\r\n"
        "\tfunction: %x\r\n"
        , cpu_vars->flag
        , cpu_vars->sp
        , cpu_vars->function
    );
}
