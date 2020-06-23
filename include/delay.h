/* SPDX-License-Identifier: GPL-2.0+ */

void udelay(int usec);
inline  __attribute__((always_inline)) void cycle_delay(unsigned long long amount)
{
    for (register int i=0;i<amount/24;i++){
        asm volatile("nop");
    }
}
