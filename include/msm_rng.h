/*
 * Copyright (c) 2014, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#ifndef __MSM_RNG_HEADER__
#define __MSM_RNG_HEADER__

#include <types.h>

#define FIPS140_DRBG_ENABLED  (1)
#define FIPS140_DRBG_DISABLED (0)

#define Q_HW_DRBG_BLOCK_BYTES (32)

/*
 *
 *  This function calls hardware random bit generator
 *  directory and retuns it back to caller.
 *
 */
int msm_rng_enable_hw();
int msm_rng_read(void *data, size_t max);
void prng_clk_enable();
void prng_clk_disable();
int RAND_bytes(unsigned char* buf, int num);

#endif
