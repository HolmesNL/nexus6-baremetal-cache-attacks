/*
 * MODIFIED by Jan-Jaap Korpershoek in 2020 for compatibility with a bare-metal environment
 */

// SPDX-License-Identifier: GPL-2.0+
/*
 * Qualcomm UART driver
 *
 * (C) Copyright 2015 Mateusz Kulikowski <mateusz.kulikowski@gmail.com>
 *
 * UART will work in Data Mover mode.
 * Based on Linux driver.
 */

#include "serial_msm.h"

/* Serial registers - this driver works in uartdm mode*/

//DECLARE_GLOBAL_DATA_PTR;

int msm_serial_fetch(struct msm_serial_data *priv)
{
	//struct msm_serial_data *priv = dev_get_priv(dev);
	unsigned sr;

	if (priv->chars_cnt)
		return priv->chars_cnt;

	/* Clear error in case of buffer overrun */
	if (readl(priv->base + UARTDM_SR) & UARTDM_SR_UART_OVERRUN)
		writel(UARTDM_CR_CMD_RESET_ERR, priv->base + UARTDM_CR);

	/* We need to fetch new character */
	sr = readl(priv->base + UARTDM_SR);

	if (sr & UARTDM_SR_RX_READY) {
		/* There are at least 4 bytes in fifo */
		priv->chars_buf = readl(priv->base + UARTDM_RF);
		priv->chars_cnt = 4;
	} else {
		/* Check if there is anything in fifo */
		priv->chars_cnt = readl(priv->base + UARTDM_RXFS);
		/* Extract number of characters in UART packing buffer*/
		priv->chars_cnt = (priv->chars_cnt >>
				   UARTDM_RXFS_BUF_SHIFT) &
				  UARTDM_RXFS_BUF_MASK;
		if (!priv->chars_cnt)
			return 0;

		/* There is at least one charcter, move it to fifo */
		writel(UARTDM_CR_CMD_FORCE_STALE,
		       priv->base + UARTDM_CR);

		priv->chars_buf = readl(priv->base + UARTDM_RF);
		writel(UARTDM_CR_CMD_RESET_STALE_INT,
		       priv->base + UARTDM_CR);
		writel(0x7, priv->base + UARTDM_DMRX);
	}

	return priv->chars_cnt;
}

int msm_serial_getc(struct msm_serial_data *priv)
{
	//struct msm_serial_data *priv = dev_get_priv(dev);
	char c;

	if (!msm_serial_fetch(priv))
		return -EAGAIN;

	c = priv->chars_buf & 0xFF;
	priv->chars_buf >>= 8;
	priv->chars_cnt--;

	return c;
}

int msm_serial_putc(struct msm_serial_data *priv, const char ch)
{
	//struct msm_serial_data *priv = dev_get_priv(dev);

	if (!(readl(priv->base + UARTDM_SR) & UARTDM_SR_TX_EMPTY) &&
	    !(readl(priv->base + UARTDM_ISR) & UARTDM_ISR_TX_READY))
		return -EAGAIN;

	writel(UARTDM_CR_CMD_RESET_TX_READY, priv->base + UARTDM_CR);

	writel(1, priv->base + UARTDM_NCF_TX);
	writel(ch, priv->base + UARTDM_TF);

	return 0;
}

int msm_serial_pending(struct msm_serial_data *priv, bool input)
{
	if (input) {
		if (msm_serial_fetch(priv))
			return 1;
	}

	return 0;
}

//This might come in handy later, but I don't want to depend on the whole dm_serial_ops struct right now
/*
static const struct dm_serial_ops msm_serial_ops = {
	.putc = msm_serial_putc,
	.pending = msm_serial_pending,
	.getc = msm_serial_getc,
};
*/

//Clock is already initialized, so no need for reinit (for now)
/*
int msm_uart_clk_init(struct udevice *dev)
{
	uint clk_rate = fdtdec_get_uint(gd->fdt_blob, dev_of_offset(dev),
					"clock-frequency", 115200);
	uint clkd[2]; //clk_id and clk_no
	int clk_offset;
	struct udevice *clk_dev;
	struct clk clk;
	int ret;

	ret = fdtdec_get_int_array(gd->fdt_blob, dev_of_offset(dev), "clock",
				   clkd, 2);
	if (ret)
		return ret;

	clk_offset = fdt_node_offset_by_phandle(gd->fdt_blob, clkd[0]);
	if (clk_offset < 0)
		return clk_offset;

	ret = uclass_get_device_by_of_offset(UCLASS_CLK, clk_offset, &clk_dev);
	if (ret)
		return ret;

	clk.id = clkd[1];
	ret = clk_request(clk_dev, &clk);
	if (ret < 0)
		return ret;

	ret = clk_set_rate(&clk, clk_rate);
	clk_free(&clk);
	if (ret < 0)
		return ret;

	return 0;
}
*/

void uart_dm_init(struct msm_serial_data *priv)
{
	writel(UART_DM_CLK_RX_TX_BIT_RATE, priv->base + UARTDM_CSR);
	writel(0x0, priv->base + UARTDM_MR1);
	writel(MSM_BOOT_UART_DM_8_N_1_MODE, priv->base + UARTDM_MR2);
	writel(MSM_BOOT_UART_DM_CMD_RESET_RX, priv->base + UARTDM_CR);
	writel(MSM_BOOT_UART_DM_CMD_RESET_TX, priv->base + UARTDM_CR);
}

//We know our device, no need for probing
/*
int msm_serial_probe(struct udevice *dev)
{
	int ret;
	struct msm_serial_data *priv = dev_get_priv(dev);

	// No need to reinitialize the UART after relocation
	if (gd->flags & GD_FLG_RELOC)
		return 0;

	ret = msm_uart_clk_init(dev);
	if (ret)
		return ret;

	pinctrl_select_state(dev, "uart");
	uart_dm_init(priv);

	return 0;
}
*/

//Uboot specific conversion function
/*
int msm_serial_ofdata_to_platdata(struct udevice *dev)
{
	struct msm_serial_data *priv = dev_get_priv(dev);

	priv->base = devfdt_get_addr(dev);
	if (priv->base == FDT_ADDR_T_NONE)
		return -EINVAL;

	return 0;
}
*/
