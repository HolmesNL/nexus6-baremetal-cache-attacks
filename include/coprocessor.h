//Events
#define EVENT_L1D_REFILL 0x3
#define EVENT_L2D_REFILL 0x17
#define EVENT_L1D_CACHE 0x4
#define EVENT_L2D_CACHE 0x16

//Counters
#define L1D_CACHE 1
#define L2D_CACHE 2
#define L1D_REFILL 3
#define L2D_REFILL 4

#include <types.h>

void set_event_counter(u32 event, u32 counter);
unsigned int get_event_counter(u32 counter);
void reset_event_counters();
unsigned int enabled_counters();
unsigned int selected_event_counter();
void init_event_counters();
void write_flags(u32 flags);
void reset_cycle_counter(int div64);
unsigned int implemented_features(void);
unsigned int read_sctlr(void);
void write_sctlr(unsigned int val);
void enable_cache(void);
void disable_cache(void);
u32 cache_info(u32 level, u32 type);
u32 read_mpidr();
void write_ttbr0(u32 value);
void write_ttbr1(u32 value);
void disable_mmu(void);
u32 read_ttbcr();
void write_dacr(u32 value);
u32 read_ttbr0();
u32 read_ttbr1();
u32 read_dacr();
void flush_tlbs(void);
void flush_I_BTB(void);
u32 read_features();

/*
 * Invalidate instruction caches (ICIALLU) 
 */
inline __attribute__((always_inline)) void flush_instruction()
{
    asm volatile (
        "MCR p15, 0, r0, c7, c5, 0\r\n"
        "ISB\r\n"
    );
}
