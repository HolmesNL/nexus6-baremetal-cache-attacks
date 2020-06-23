/*
 * Copyright (c) 2011-2014, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU  General Public License for more details.
 *
 */
#include "msm_rng.h"
#include <io.h>
#include <serial.h>

#define DRIVER_NAME "msm_rng"

/* Device specific register offsets */
#define PRNG_DATA_OUT_OFFSET    0x0000
#define PRNG_STATUS_OFFSET	0x0004
#define PRNG_LFSR_CFG_OFFSET	0x0100
#define PRNG_CONFIG_OFFSET	0x0104

/* Device specific register masks and config values */
#define PRNG_LFSR_CFG_MASK	0xFFFF0000
#define PRNG_LFSR_CFG_CLOCKS	0x0000DDDD
#define PRNG_CONFIG_MASK	0xFFFFFFFD
#define PRNG_HW_ENABLE		0x00000002

#define MAX_HW_FIFO_DEPTH 16                     /* FIFO is 16 words deep */
#define MAX_HW_FIFO_SIZE (MAX_HW_FIFO_DEPTH * 4) /* FIFO is 32 bits wide  */

#define GCC_CC_PHYS     0xFC400000
#define PRNG_AHB_CBCR                    0x0D04
#define APCS_CLOCK_BRANCH_ENA_VOTE       0x1484

#define BASE 0xF9BFF000

int msm_rng_read(void *data, size_t max)
{
	struct msm_rng_device *msm_rng_dev;
	struct platform_device *pdev;
	size_t maxsize;
	size_t currsize = 0;
	unsigned long val;
	unsigned long *retdata = data;
	int ret, ret1;

	u32 base = BASE;

	/* calculate max size bytes to transfer back to caller */
	/*maxsize = MAX_HW_FIFO_SIZE;
	if (max < maxsize) {
    	maxsize = max;
	}*/
	maxsize = max;

	/* no room for word data */
	if (maxsize < 4)
		return 0;

	/* read random data from h/w */
	/* enable PRNG clock */
	prng_clk_enable();

	/* read random data from h/w */
	do {
		/* check status bit if data is available */
		if (!(readl(base + PRNG_STATUS_OFFSET) & 0x00000001)) {
			break;	/* no data to read so just bail */
		}

		/* read FIFO */
		val = readl(base + PRNG_DATA_OUT_OFFSET);
		if (!val) {
			break;	/* no data to read so just bail */
		}

		/* write data back to callers pointer */
		*(retdata++) = val;
		currsize += 4;

		/* make sure we stay on 32bit boundary */
		if ((maxsize - currsize) < 4)
			break;
	} while (currsize < maxsize);
	/* vote to turn off clock */
	prng_clk_disable();

	return currsize;
}

int msm_rng_enable_hw()
{
	unsigned long val = 0;
	unsigned long reg_val = 0;
	int ret = 0;
	u32 base = BASE;

	/*if (msm_rng_dev->qrng_perf_client) {
		ret = msm_bus_scale_client_update_request(
				msm_rng_dev->qrng_perf_client, 1);
		if (ret)
			pr_err("bus_scale_client_update_req failed!\n");
	}*/
	/* Enable the PRNG CLK */
	prng_clk_enable();
	/* Enable PRNG h/w only if it is NOT ON */
	val = readl(base + PRNG_CONFIG_OFFSET) &
					PRNG_HW_ENABLE;
	/* PRNG H/W is not ON */
	if (val != PRNG_HW_ENABLE) {
		val = readl(base + PRNG_LFSR_CFG_OFFSET);
		val &= PRNG_LFSR_CFG_MASK;
		val |= PRNG_LFSR_CFG_CLOCKS;
		writel(val, base + PRNG_LFSR_CFG_OFFSET);

		/* The PRNG CONFIG register should be first written */
		mb();

		reg_val = readl(base + PRNG_CONFIG_OFFSET)
						& PRNG_CONFIG_MASK;
		reg_val |= PRNG_HW_ENABLE;
		writel(reg_val, base + PRNG_CONFIG_OFFSET);

		/* The PRNG clk should be disabled only after we enable the
		* PRNG h/w by writing to the PRNG CONFIG register.
		*/
		mb();
	}
	prng_clk_disable();
	return 0;
}

void prng_clk_enable()
{
    u32 vote_reg = GCC_CC_PHYS + APCS_CLOCK_BRANCH_ENA_VOTE;
	u32 ena = readl(vote_reg);
	ena |= BIT(13);
	writel(ena, vote_reg);

	//branch_clk_halt_check(c, vclk->halt_check, CBCR_REG(vclk), BRANCH_ON);

}

void prng_clk_disable()
{
    u32 vote_reg = GCC_CC_PHYS + APCS_CLOCK_BRANCH_ENA_VOTE;
	u32 ena = readl(vote_reg);
	ena &= ~BIT(13);
	writel(ena, vote_reg);
}


int RAND_bytes(unsigned char* buf, int num) 
{
	//Make sure that enough random data is read
	char *randomness_pointer = (char *) buf;
	int read = 0;
	while (read < num) {
        read += msm_rng_read(randomness_pointer, num);
        randomness_pointer += read;
	}
	return 1;
}
