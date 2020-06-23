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
#include <linux/errno.h>
#include <types.h>
#include <io.h>
#include <asm.h>

#ifndef MSM_SERIAL_H
#define MSM_SERIAL_H

/* Serial registers - this driver works in uartdm mode*/

#define UARTDM_DMRX             0x34 /* Max RX transfer length */
#define UARTDM_NCF_TX           0x40 /* Number of chars to TX */

#define UARTDM_RXFS             0x50 /* RX channel status register */
#define UARTDM_RXFS_BUF_SHIFT   0x7  /* Number of bytes in the packing buffer */
#define UARTDM_RXFS_BUF_MASK    0x7
#define UARTDM_MR1				 0x00
#define UARTDM_MR2				 0x04
#define UARTDM_CSR				 0xA0

#define UARTDM_SR                0xA4 /* Status register */
#define UARTDM_SR_RX_READY       (1 << 0) /* Word is the receiver FIFO */
#define UARTDM_SR_TX_EMPTY       (1 << 3) /* Transmitter underrun */
#define UARTDM_SR_UART_OVERRUN   (1 << 4) /* Receive overrun */

#define UARTDM_CR                         0xA8 /* Command register */
#define UARTDM_CR_CMD_RESET_ERR           (3 << 4) /* Clear overrun error */
#define UARTDM_CR_CMD_RESET_STALE_INT     (8 << 4) /* Clears stale irq */
#define UARTDM_CR_CMD_RESET_TX_READY      (3 << 8) /* Clears TX Ready irq*/
#define UARTDM_CR_CMD_FORCE_STALE         (4 << 8) /* Causes stale event */
#define UARTDM_CR_CMD_STALE_EVENT_DISABLE (6 << 8) /* Disable stale event */

#define UARTDM_IMR                0xB0 /* Interrupt mask register */
#define UARTDM_ISR                0xB4 /* Interrupt status register */
#define UARTDM_ISR_TX_READY       0x80 /* TX FIFO empty */

#define UARTDM_TF               0x100 /* UART Transmit FIFO register */
#define UARTDM_RF               0x140 /* UART Receive FIFO register */

#define UART_DM_CLK_RX_TX_BIT_RATE 0xCC
#define MSM_BOOT_UART_DM_8_N_1_MODE 0x34
#define MSM_BOOT_UART_DM_CMD_RESET_RX 0x10
#define MSM_BOOT_UART_DM_CMD_RESET_TX 0x20

//DECLARE_GLOBAL_DATA_PTR;

struct msm_serial_data {
	phys_addr_t base;
	unsigned chars_cnt; /* number of buffered chars */
	uint32_t chars_buf; /* buffered chars */
};

int msm_serial_fetch(struct msm_serial_data *priv);
int msm_serial_getc(struct msm_serial_data *priv);
int msm_serial_putc(struct msm_serial_data *priv, const char ch);
int msm_serial_pending(struct msm_serial_data *priv, bool input);

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
static int msm_uart_clk_init(struct udevice *dev)
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

void uart_dm_init(struct msm_serial_data *priv);

//We know our device, no need for probing
/*
static int msm_serial_probe(struct udevice *dev)
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
static int msm_serial_ofdata_to_platdata(struct udevice *dev)
{
	struct msm_serial_data *priv = dev_get_priv(dev);

	priv->base = devfdt_get_addr(dev);
	if (priv->base == FDT_ADDR_T_NONE)
		return -EINVAL;

	return 0;
}
*/

#endif // MSM_SERIAL_H
