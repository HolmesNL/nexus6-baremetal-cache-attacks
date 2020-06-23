#include <serial.h>

void udelay(int amount)
{
    for (int i=0;i<amount;i++) {
        for (int j=0;j<100000;j++) {
            asm volatile ("nop");
        }
    }
}

