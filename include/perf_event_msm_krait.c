/*
 * Copyright (c) 2011-2013, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <asm/cp15.h>
#include <asm/vfp.h>
#include <perf_event_msm_krait.h>
#include <scm.h>
#include "vfpinstr.h"
#include <errno-base.h>
#include <serial.h>

#define KRAIT_EVT_PREFIX 1
#define KRAIT_VENUMEVT_PREFIX 2
/*
   event encoding:                nrccg
   n  = prefix (1 for Krait L1) (2 for Krait VeNum events)
   r  = register
   cc = code
   g  = group
*/

#define KRAIT_L1_ICACHE_ACCESS 0x10011
#define KRAIT_L1_ICACHE_MISS 0x10010

#define KRAIT_P2_L1_ITLB_ACCESS 0x12222
#define KRAIT_P2_L1_DTLB_ACCESS 0x12210

#define KRAIT_EVENT_MASK 0xfffff
#define KRAIT_MODE_EXCL_MASK 0xc0000000

#define CPU_NUM_COUNTERS 5

#define COUNT_MASK	0xffffffff

u32 evt_type_base[4] = {0xcc, 0xd0, 0xd4, 0xd8};

/*
 * This offset is used to calculate the index
 * into evt_type_base[] and krait_functions[]
 */
#define VENUM_BASE_OFFSET 3

#define KRAIT_MAX_L1_REG 3

/*
 * ARMv7 Cortex-A8 and Cortex-A9 Performance Events handling code.
 *
 * ARMv7 support: Jean Pihet <jpihet@mvista.com>
 * 2010 (c) MontaVista Software, LLC.
 *
 * Copied from ARMv6 code, with the low level code inspired
 *  by the ARMv7 Oprofile code.
 *
 * Cortex-A8 has up to 4 configurable performance counters and
 *  a single cycle counter.
 * Cortex-A9 has up to 31 configurable performance counters and
 *  a single cycle counter.
 *
 * All counters can be enabled/disabled and IRQ masked separately. The cycle
 *  counter and all 4 performance counters together can be reset separately.
 */

/*
 * Common ARMv7 event types
 *
 * Note: An implementation may not be able to count all of these events
 * but the encodings are considered to be `reserved' in the case that
 * they are not available.
 */
enum armv7_perf_types {
	ARMV7_PERFCTR_PMNC_SW_INCR			= 0x00,
	ARMV7_PERFCTR_L1_ICACHE_REFILL			= 0x01,
	ARMV7_PERFCTR_ITLB_REFILL			= 0x02,
	ARMV7_PERFCTR_L1_DCACHE_REFILL			= 0x03,
	ARMV7_PERFCTR_L1_DCACHE_ACCESS			= 0x04,
	ARMV7_PERFCTR_DTLB_REFILL			= 0x05,
	ARMV7_PERFCTR_MEM_READ				= 0x06,
	ARMV7_PERFCTR_MEM_WRITE				= 0x07,
	ARMV7_PERFCTR_INSTR_EXECUTED			= 0x08,
	ARMV7_PERFCTR_EXC_TAKEN				= 0x09,
	ARMV7_PERFCTR_EXC_EXECUTED			= 0x0A,
	ARMV7_PERFCTR_CID_WRITE				= 0x0B,

	/*
	 * ARMV7_PERFCTR_PC_WRITE is equivalent to HW_BRANCH_INSTRUCTIONS.
	 * It counts:
	 *  - all (taken) branch instructions,
	 *  - instructions that explicitly write the PC,
	 *  - exception generating instructions.
	 */
	ARMV7_PERFCTR_PC_WRITE				= 0x0C,
	ARMV7_PERFCTR_PC_IMM_BRANCH			= 0x0D,
	ARMV7_PERFCTR_PC_PROC_RETURN			= 0x0E,
	ARMV7_PERFCTR_MEM_UNALIGNED_ACCESS		= 0x0F,
	ARMV7_PERFCTR_PC_BRANCH_MIS_PRED		= 0x10,
	ARMV7_PERFCTR_CLOCK_CYCLES			= 0x11,
	ARMV7_PERFCTR_PC_BRANCH_PRED			= 0x12,

	/* These events are defined by the PMUv2 supplement (ARM DDI 0457A). */
	ARMV7_PERFCTR_MEM_ACCESS			= 0x13,
	ARMV7_PERFCTR_L1_ICACHE_ACCESS			= 0x14,
	ARMV7_PERFCTR_L1_DCACHE_WB			= 0x15,
	ARMV7_PERFCTR_L2_CACHE_ACCESS			= 0x16,
	ARMV7_PERFCTR_L2_CACHE_REFILL			= 0x17,
	ARMV7_PERFCTR_L2_CACHE_WB			= 0x18,
	ARMV7_PERFCTR_BUS_ACCESS			= 0x19,
	ARMV7_PERFCTR_MEM_ERROR				= 0x1A,
	ARMV7_PERFCTR_INSTR_SPEC			= 0x1B,
	ARMV7_PERFCTR_TTBR_WRITE			= 0x1C,
	ARMV7_PERFCTR_BUS_CYCLES			= 0x1D,

	ARMV7_PERFCTR_CPU_CYCLES			= 0xFF
};

/*
 * Perf Events' indices
 */
#define	ARMV7_IDX_CYCLE_COUNTER	0
#define	ARMV7_IDX_COUNTER0	1
#define	ARMV7_IDX_COUNTER_LAST \
	(ARMV7_IDX_CYCLE_COUNTER + CPU_NUM_COUNTERS - 1)

#define	ARMV7_MAX_COUNTERS	32
#define	ARMV7_COUNTER_MASK	(ARMV7_MAX_COUNTERS - 1)

/*
 * ARMv7 low level PMNC access
 */

/*
 * Perf Event to low level counters mapping
 */
#define	ARMV7_IDX_TO_COUNTER(x)	\
	(((x) - ARMV7_IDX_COUNTER0) & ARMV7_COUNTER_MASK)

/*
 * Per-CPU PMNC: config reg
 */
#define ARMV7_PMNC_E		(1 << 0) /* Enable all counters */
#define ARMV7_PMNC_P		(1 << 1) /* Reset all counters */
#define ARMV7_PMNC_C		(1 << 2) /* Cycle counter reset */
#define ARMV7_PMNC_D		(1 << 3) /* CCNT counts every 64th cpu cycle */
#define ARMV7_PMNC_X		(1 << 4) /* Export to ETM */
#define ARMV7_PMNC_DP		(1 << 5) /* Disable CCNT if non-invasive debug*/
#define	ARMV7_PMNC_N_SHIFT	11	 /* Number of counters supported */
#define	ARMV7_PMNC_N_MASK	0x1f
#define	ARMV7_PMNC_MASK		0x3f	 /* Mask for writable bits */

/*
 * FLAG: counters overflow flag status reg
 */
#define	ARMV7_FLAG_MASK		0xffffffff	/* Mask for writable bits */
#define	ARMV7_OVERFLOWED_MASK	ARMV7_FLAG_MASK

/*
 * PMXEVTYPER: Event selection reg
 */
#define	ARMV7_EVTYPE_MASK	0xc80000ff	/* Mask for writable bits */
#define	ARMV7_EVTYPE_EVENT	0xff		/* Mask for EVENT bits */

/*
 * Event filters for PMUv2
 */
#define	ARMV7_EXCLUDE_PL1	(1 << 31)
#define	ARMV7_EXCLUDE_USER	(1 << 30)
#define	ARMV7_INCLUDE_HYP	(1 << 27)

u32 armv7_pmnc_read(void)
{
	u32 val;
	asm volatile("mrc p15, 0, %0, c9, c12, 0" : "=r"(val));
	return val;
}

void armv7_pmnc_write(u32 val)
{
	val &= ARMV7_PMNC_MASK;
	isb();
	asm volatile("mcr p15, 0, %0, c9, c12, 0" : : "r"(val));
}

int armv7_pmnc_has_overflowed(u32 pmnc)
{
	return pmnc & ARMV7_OVERFLOWED_MASK;
}

int armv7_pmnc_counter_has_overflowed(u32 pmnc, int idx)
{
	return pmnc & BIT(ARMV7_IDX_TO_COUNTER(idx));
}

int armv7_pmnc_select_counter(int idx)
{
	u32 counter = ARMV7_IDX_TO_COUNTER(idx);
	asm volatile("mcr p15, 0, %0, c9, c12, 5" : : "r" (counter));
	isb();

	return idx;
}

u32 armv7pmu_read_counter(int idx)
{
	u32 value = 0;

	if (armv7_pmnc_select_counter(idx) == idx)
		asm volatile("mrc p15, 0, %0, c9, c13, 2" : "=r" (value));

	return value;
}

void armv7pmu_write_counter(int idx, u32 value)
{
	if (armv7_pmnc_select_counter(idx) == idx)
		asm volatile("mcr p15, 0, %0, c9, c13, 2" : : "r" (value));
}

void armv7_pmnc_write_evtsel(int idx, u32 val)
{
	if (armv7_pmnc_select_counter(idx) == idx) {
		val &= ARMV7_EVTYPE_MASK;
		asm volatile("mcr p15, 0, %0, c9, c13, 1" : : "r" (val));
	}
}

int armv7_pmnc_enable_counter(int idx)
{
	u32 counter = ARMV7_IDX_TO_COUNTER(idx);
	asm volatile("mcr p15, 0, %0, c9, c12, 1" : : "r" (BIT(counter)));
	return idx;
}

int armv7_pmnc_disable_counter(int idx)
{
	u32 counter = ARMV7_IDX_TO_COUNTER(idx);
	asm volatile("mcr p15, 0, %0, c9, c12, 2" : : "r" (BIT(counter)));
	return idx;
}

int armv7_pmnc_enable_intens(int idx)
{
	u32 counter = ARMV7_IDX_TO_COUNTER(idx);
	asm volatile("mcr p15, 0, %0, c9, c14, 1" : : "r" (BIT(counter)));
	return idx;
}

int armv7_pmnc_disable_intens(int idx)
{
	u32 counter = ARMV7_IDX_TO_COUNTER(idx);
	asm volatile("mcr p15, 0, %0, c9, c14, 2" : : "r" (BIT(counter)));
	isb();
	/* Clear the overflow flag in case an interrupt is pending. */
	asm volatile("mcr p15, 0, %0, c9, c12, 3" : : "r" (BIT(counter)));
	isb();

	return idx;
}

u32 armv7_pmnc_getreset_flags(void)
{
	u32 val;

	/* Read */
	asm volatile("mrc p15, 0, %0, c9, c12, 3" : "=r" (val));

	/* Write to clear flags */
	val &= ARMV7_FLAG_MASK;
	asm volatile("mcr p15, 0, %0, c9, c12, 3" : : "r" (val));

	return val;
}

/*void armv7_pmnc_dump_regs(struct arm_pmu *cpu_pmu)
{
	u32 val;
	unsigned int cnt;

	printk(KERN_INFO "PMNC registers dump:\n");

	asm volatile("mrc p15, 0, %0, c9, c12, 0" : "=r" (val));
	printk(KERN_INFO "PMNC  =0x%08x\n", val);

	asm volatile("mrc p15, 0, %0, c9, c12, 1" : "=r" (val));
	printk(KERN_INFO "CNTENS=0x%08x\n", val);

	asm volatile("mrc p15, 0, %0, c9, c14, 1" : "=r" (val));
	printk(KERN_INFO "INTENS=0x%08x\n", val);

	asm volatile("mrc p15, 0, %0, c9, c12, 3" : "=r" (val));
	printk(KERN_INFO "FLAGS =0x%08x\n", val);

	asm volatile("mrc p15, 0, %0, c9, c12, 5" : "=r" (val));
	printk(KERN_INFO "SELECT=0x%08x\n", val);

	asm volatile("mrc p15, 0, %0, c9, c13, 0" : "=r" (val));
	printk(KERN_INFO "CCNT  =0x%08x\n", val);

	for (cnt = ARMV7_IDX_COUNTER0;
			cnt <= ARMV7_IDX_COUNTER_LAST(cpu_pmu); cnt++) {
		armv7_pmnc_select_counter(cnt);
		asm volatile("mrc p15, 0, %0, c9, c13, 2" : "=r" (val));
		printk(KERN_INFO "CNT[%d] count =0x%08x\n",
			ARMV7_IDX_TO_COUNTER(cnt), val);
		asm volatile("mrc p15, 0, %0, c9, c13, 1" : "=r" (val));
		printk(KERN_INFO "CNT[%d] evtsel=0x%08x\n",
			ARMV7_IDX_TO_COUNTER(cnt), val);
	}
}*/

//
u32 armv7_read_num_pmnc_events(void)
{
	u32 nb_cnt;

	/* Read the nb of CNTx counters supported from PMNC */
	nb_cnt = (armv7_pmnc_read() >> ARMV7_PMNC_N_SHIFT) & ARMV7_PMNC_N_MASK;

	/* Add the CPU cycles counter and return */
	return nb_cnt + 1;
}

struct krait_evt {
	/*
	 * The group_setval field corresponds to the value that the group
	 * register needs to be set to. This value is calculated from the row
	 * and column that the event belongs to in the event table
	 */
	u32 group_setval;

	/*
	 * The groupcode corresponds to the group that the event belongs to.
	 * Krait has 3 groups of events PMRESR0, 1, 2
	 * going from 0 to 2 in terms of the codes used
	 */
	u8 groupcode;

	/*
	 * The armv7_evt_type field corresponds to the armv7 defined event
	 * code that the Krait events map to
	 */
	u32 armv7_evt_type;
};

u32 krait_read_pmresr0(void)
{
	u32 val;

	asm volatile("mrc p15, 1, %0, c9, c15, 0" : "=r" (val));
	return val;
}

void krait_write_pmresr0(u32 val)
{
	asm volatile("mcr p15, 1, %0, c9, c15, 0" : : "r" (val));
}

u32 krait_read_pmresr1(void)
{
	u32 val;

	asm volatile("mrc p15, 1, %0, c9, c15, 1" : "=r" (val));
	return val;
}

void krait_write_pmresr1(u32 val)
{
	asm volatile("mcr p15, 1, %0, c9, c15, 1" : : "r" (val));
}

u32 krait_read_pmresr2(void)
{
	u32 val;

	asm volatile("mrc p15, 1, %0, c9, c15, 2" : "=r" (val));
	return val;
}

void krait_write_pmresr2(u32 val)
{
	asm volatile("mcr p15, 1, %0, c9, c15, 2" : : "r" (val));
}

u32 krait_read_vmresr0(void)
{
	u32 val;

	asm volatile ("mrc p10, 7, %0, c11, c0, 0" : "=r" (val));
	return val;
}

void krait_write_vmresr0(u32 val)
{
	asm volatile ("mcr p10, 7, %0, c11, c0, 0" : : "r" (val));
}

u32 venum_orig_val;
u32 fp_orig_val;

void krait_pre_vmresr0(void)
{
	u32 venum_new_val;
	u32 fp_new_val;
	u32 v_orig_val;
	u32 f_orig_val;

	/* CPACR Enable CP10 and CP11 access */
	v_orig_val = get_copro_access();
	venum_new_val = v_orig_val | CPACC_SVC(10) | CPACC_SVC(11);
	set_copro_access(venum_new_val);
	/* Store orig venum val */
	venum_orig_val = v_orig_val;

	/* Enable FPEXC */
	f_orig_val = fmrx(FPEXC);
	fp_new_val = f_orig_val | FPEXC_EN;
	fmxr(FPEXC, fp_new_val);
	/* Store orig fp val */
	fp_orig_val = f_orig_val;

}

void krait_post_vmresr0(void)
{
	/* Restore FPEXC */
	fmxr(FPEXC, fp_orig_val);
	isb();
	/* Restore CPACR */
	set_copro_access(venum_orig_val);
}

struct krait_access_funcs {
	u32 (*read) (void);
	void (*write) (u32);
	void (*pre) (void);
	void (*post) (void);
};

/*
 * The krait_functions array is used to set up the event register codes
 * based on the group to which an event belongs.
 * Having the following array modularizes the code for doing that.
 */
struct krait_access_funcs krait_functions[] = {
	{krait_read_pmresr0, krait_write_pmresr0, NULL, NULL},
	{krait_read_pmresr1, krait_write_pmresr1, NULL, NULL},
	{krait_read_pmresr2, krait_write_pmresr2, NULL, NULL},
	{krait_read_vmresr0, krait_write_vmresr0, krait_pre_vmresr0,
	 krait_post_vmresr0},
};

u32 krait_get_columnmask(u32 evt_code)
{
	const u32 columnmasks[] = {0xffffff00, 0xffff00ff, 0xff00ffff,
					0x80ffffff};

	return columnmasks[evt_code & 0x3];
}

void krait_evt_setup(u32 gr, u32 setval, u32 evt_code)
{
	u32 val;

	if (krait_functions[gr].pre)
		krait_functions[gr].pre();

	val = krait_get_columnmask(evt_code) & krait_functions[gr].read();
	val = val | setval;
	krait_functions[gr].write(val);

	if (krait_functions[gr].post)
		krait_functions[gr].post();
}

void krait_clear_pmuregs(void)
{
	krait_write_pmresr0(0);
	krait_write_pmresr1(0);
	krait_write_pmresr2(0);

	krait_pre_vmresr0();
	krait_write_vmresr0(0);
	krait_post_vmresr0();
}

void krait_clearpmu(u32 grp, u32 val, u32 evt_code)
{
	u32 new_pmuval;

	if (krait_functions[grp].pre)
		krait_functions[grp].pre();

	new_pmuval = krait_functions[grp].read() &
		krait_get_columnmask(evt_code);
	krait_functions[grp].write(new_pmuval);

	if (krait_functions[grp].post)
		krait_functions[grp].post();
}

void krait_pmu_reset(void)
{
    int idx;

	/* Stop all counters and their interrupts */
	for (idx = 1; idx < CPU_NUM_COUNTERS; ++idx) {
		armv7_pmnc_disable_counter(idx);
		armv7_pmnc_disable_intens(idx);
	}

	/* Clear all pmresrs */
	krait_clear_pmuregs();

	/* Reset irq stat reg */
	armv7_pmnc_getreset_flags();

	/* Reset all ctrs to 0 */
	armv7_pmnc_write(ARMV7_PMNC_P | ARMV7_PMNC_C);
}

void krait_pmu_init(void)
{
	krait_clear_pmuregs();
}

void krait_pmu_start(void)
{
	armv7_pmnc_write(armv7_pmnc_read() | ARMV7_PMNC_E);
}

void krait_pmu_stop(void)
{
	armv7_pmnc_write(armv7_pmnc_read() & ~ARMV7_PMNC_E);
}

void krait_pmu_enable_event(int idx, int event)
{
	/* Disable counter */
	armv7_pmnc_disable_counter(idx);

	armv7_pmnc_write_evtsel(idx, event);

	/* Enable interrupt for this counter */
	armv7_pmnc_enable_intens(idx);

	/* Restore prev val */
	armv7pmu_write_counter(idx, 0);

	/* Enable counter */
	armv7_pmnc_enable_counter(idx);
}

void armv7_pmnc_dump_regs()
{
	u32 val;
	unsigned int cnt;

	print("PMNC registers dump:\r\n");

	asm volatile("mrc p15, 0, %0, c9, c12, 0" : "=r" (val));
	print("PMNC  =0x%08x\r\n", val);

	asm volatile("mrc p15, 0, %0, c9, c12, 1" : "=r" (val));
	print("CNTENS=0x%08x\r\n", val);

	asm volatile("mrc p15, 0, %0, c9, c14, 1" : "=r" (val));
	print("INTENS=0x%08x\r\n", val);

	asm volatile("mrc p15, 0, %0, c9, c12, 3" : "=r" (val));
	print("FLAGS =0x%08x\r\n", val);

	asm volatile("mrc p15, 0, %0, c9, c12, 5" : "=r" (val));
	print("SELECT=0x%08x\r\n", val);

	asm volatile("mrc p15, 0, %0, c9, c13, 0" : "=r" (val));
	print("CCNT  =0x%08x\r\n", val);

	asm volatile("MRC p15, 0, %0, c1, c0, 0" : "=r" (val));
	print("SCTLR  =0x%08x\r\n", val);

	for (cnt = ARMV7_IDX_COUNTER0;
			cnt <= ARMV7_IDX_COUNTER_LAST; cnt++) {
		armv7_pmnc_select_counter(cnt);
		asm volatile("mrc p15, 0, %0, c9, c13, 2" : "=r" (val));
		print("CNT[%d] count =0x%08x\r\n",
			ARMV7_IDX_TO_COUNTER(cnt), val);
		asm volatile("mrc p15, 0, %0, c9, c13, 1" : "=r" (val));
		print("CNT[%d] evtsel=0x%08x\r\n",
			ARMV7_IDX_TO_COUNTER(cnt), val);
	}
}
