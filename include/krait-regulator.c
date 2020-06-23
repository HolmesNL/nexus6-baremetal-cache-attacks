#include <scm.h>
#include <types.h>
#include <io.h>
#include <asm.h>

#define PMIC_VOLTAGE_MIN		350000
#define PMIC_VOLTAGE_MAX		1355000
#define LV_RANGE_STEP			5000

#define CORE_VOLTAGE_BOOTUP		900000

#define KRAIT_LDO_VOLTAGE_MIN		465000
#define KRAIT_LDO_VOLTAGE_OFFSET	465000
#define KRAIT_LDO_STEP			5000

#define BHS_SETTLING_DELAY_US		1
#define LDO_SETTLING_DELAY_US		1
#define MDD_SETTLING_DELAY_US		5

#define _KRAIT_MASK(BITS, POS)  (((u32)(1 << (BITS)) - 1) << POS)
#define KRAIT_MASK(LEFT_BIT_POS, RIGHT_BIT_POS) \
		_KRAIT_MASK(LEFT_BIT_POS - RIGHT_BIT_POS + 1, RIGHT_BIT_POS)

#define APC_SECURE		0x00000000
#define CPU_PWR_CTL		0x00000004
#define APC_PWR_STATUS		0x00000008
#define APC_TEST_BUS_SEL	0x0000000C
#define CPU_TRGTD_DBG_RST	0x00000010
#define APC_PWR_GATE_CTL	0x00000014
#define APC_LDO_VREF_SET	0x00000018
#define APC_PWR_GATE_MODE	0x0000001C
#define APC_PWR_GATE_DLY	0x00000020

#define PWR_GATE_CONFIG		0x00000044
#define VERSION			0x00000FD0

/* MDD register group */
#define MDD_CONFIG_CTL		0x00000000
#define MDD_MODE		0x00000010

#define PHASE_SCALING_REF	4

/* bit definitions for phase scaling eFuses */
#define PHASE_SCALING_EFUSE_VERSION_POS		26
#define PHASE_SCALING_EFUSE_VERSION_MASK	KRAIT_MASK(27, 26)
#define PHASE_SCALING_EFUSE_VERSION_SET		1

#define PHASE_SCALING_EFUSE_VALUE_POS		16
#define PHASE_SCALING_EFUSE_VALUE_MASK		KRAIT_MASK(18, 16)

/* bit definitions for APC_PWR_GATE_CTL */
#define BHS_CNT_BIT_POS		24
#define BHS_CNT_MASK		KRAIT_MASK(31, 24)
#define BHS_CNT_DEFAULT		64

#define CLK_SRC_SEL_BIT_POS	15
#define CLK_SRC_SEL_MASK	KRAIT_MASK(15, 15)
#define CLK_SRC_DEFAULT		0

#define LDO_PWR_DWN_BIT_POS	16
#define LDO_PWR_DWN_MASK	KRAIT_MASK(21, 16)

#define LDO_BYP_BIT_POS		8
#define LDO_BYP_MASK		KRAIT_MASK(13, 8)

#define BHS_SEG_EN_BIT_POS	1
#define BHS_SEG_EN_MASK		KRAIT_MASK(6, 1)
#define BHS_SEG_EN_DEFAULT	0x3F

#define BHS_EN_BIT_POS		0
#define BHS_EN_MASK		KRAIT_MASK(0, 0)

/* bit definitions for APC_LDO_VREF_SET register */
#define VREF_RET_POS		8
#define VREF_RET_MASK		KRAIT_MASK(14, 8)

#define VREF_LDO_BIT_POS	0
#define VREF_LDO_MASK		KRAIT_MASK(6, 0)

#define PWR_GATE_SWITCH_MODE_POS	4
#define PWR_GATE_SWITCH_MODE_MASK	KRAIT_MASK(6, 4)

#define PWR_GATE_SWITCH_MODE_PC		0
#define PWR_GATE_SWITCH_MODE_LDO	1
#define PWR_GATE_SWITCH_MODE_BHS	2
#define PWR_GATE_SWITCH_MODE_DT		3
#define PWR_GATE_SWITCH_MODE_RET	4

#define LDO_HDROOM_MIN		50000
#define LDO_HDROOM_MAX		250000

#define LDO_UV_MIN		465000
#define LDO_UV_MAX		750000

#define LDO_TH_MIN		600000
#define LDO_TH_MAX		900000

#define LDO_DELTA_MIN		10000
#define LDO_DELTA_MAX		100000

#define MSM_L2_SAW_PHYS		0xf9012000
#define MSM_MDD_BASE_PHYS	0xf908a800

#define KPSS_VERSION_2P0	0x20000000
#define KPSS_VERSION_2P2	0x20020000

#define PMIC_FTS_MODE_PFM   0x00
#define PMIC_FTS_MODE_PWM   0x80
#define PMIC_FTS_MODE_AUTO  0x40
#define ONE_PHASE_COEFF     1000000
#define TWO_PHASE_COEFF     2000000

#define PWM_SETTLING_TIME_US        50
#define PHASE_SETTLING_TIME_US      50

/**
 * secondary_cpu_hs_init - Initialize BHS and LDO registers
 *				for nonboot cpu
 *
 * @base_ptr: address pointer to APC registers of a cpu
 * @cpu: the cpu being brought out of reset
 *
 * seconday_cpu_hs_init() is called when a secondary cpu
 * is being brought online for the first time. It is not
 * called for boot cpu. It initializes power related
 * registers and makes the core run from BHS.
 * It also ends up turning on MDD which is required when the
 * core switches to LDO mode
 */
void secondary_cpu_hs_init(unsigned int base_ptr, int cpu)
{
	uint32_t reg_val;
	void *l2_saw_base;
	void *gcc_base_ptr;
	unsigned int mdd_base;
	//struct krait_power_vreg *kvreg;

	/* Turn on the BHS, turn off LDO Bypass and power down LDO */
	reg_val =  BHS_CNT_DEFAULT << BHS_CNT_BIT_POS
		| LDO_PWR_DWN_MASK
		| CLK_SRC_DEFAULT << CLK_SRC_SEL_BIT_POS
		| BHS_EN_MASK;
	writel(reg_val, base_ptr + APC_PWR_GATE_CTL);

	/* Turn on BHS segments */
	reg_val |= BHS_SEG_EN_DEFAULT << BHS_SEG_EN_BIT_POS;
	writel(reg_val, base_ptr + APC_PWR_GATE_CTL);

	/* Finally turn on the bypass so that BHS supplies power */
	reg_val |= LDO_BYP_MASK;
	writel(reg_val, base_ptr + APC_PWR_GATE_CTL);

	/*
	 * This nonboot cpu has not been probed yet. This cpu was
	 * brought out of reset as a part of maxcpus >= 2. Initialize
	 * its MDD and APC_PWR_GATE_MODE register here
	 */
	mdd_base = MSM_MDD_BASE_PHYS + cpu * 0x10000;
	/* setup the bandgap that configures the reference to the LDO */
	writel(0x00000190, mdd_base + MDD_CONFIG_CTL);
	/* Enable MDD */
	writel(0x00000002, mdd_base + MDD_MODE);

	writel(0x30430600, base_ptr + APC_PWR_GATE_DLY);
	writel(0x00000021, base_ptr + APC_PWR_GATE_MODE);

	/*
	 * If the driver has not yet started to manage phases then
	 * enable max phases.
	 */
	writel(0x10003, MSM_L2_SAW_PHYS + 0x1c);
}
