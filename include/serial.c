#include <serial.h>
#include <stdarg.h>
#include <uboot/drivers/serial/serial_msm.h>
#include <printf.h>

int geti(struct msm_serial_data *dev) {
	int rcv = 0;
	int cnt = 3;
	unsigned char c;
	
	while (cnt >= 0) {
		while(!msm_serial_fetch(dev));
		while(dev->chars_cnt) {
			c = msm_serial_getc(dev);
			//msm_serial_putc(dev, c);
			rcv |= (c << (8*(3-cnt)));
			cnt--;
		}
	}
	return rcv;
}

void *get_blob(unsigned int sz, char *buf, struct msm_serial_data *dev) {
	unsigned char c;
	unsigned int cnt = 0;

	while (cnt < sz) {
		while(!msm_serial_fetch(dev));
		while(dev->chars_cnt) {
			c = msm_serial_getc(dev);
			msm_serial_putc(dev, c);
			buf[cnt] = c;
			cnt++;
		}
	}
	return (void *)buf;
}

void _print(char buf[], struct msm_serial_data *dev)
{
	unsigned int i = 0;
	while (buf[i] != 0x0) {
		while (msm_serial_putc(dev, buf[i]) != 0); // we thought this should be without ! and with <0, but this didnt work.
		i++;
	}
	for (i=0;i<64;i++) {buf[i] = 0;};
}

void _printn(char buf[], struct msm_serial_data *dev, uint8_t sz)
{
	unsigned static int i = 0;
	while (i < sz) {
		unsigned static int j = 0;
		for(j=0;j<1000;j++){asm("nop");}
		while(msm_serial_putc(dev, buf[i]) != 0);
		i++;
	}
}


void print(const char* format, ...)
{
    char buf[1024];
    struct msm_serial_data dev;
	dev.base = MSM_SERIAL_BASE;

    va_list va;
    va_start(va, format);
	vsnprintf_(buf, -1, format, va);
	_print(buf, &dev);
	va_end(va);
}

void _putchar(char c) {	
	struct msm_serial_data dev;
	dev.base = 0xF995E000;
    while(msm_serial_putc(&dev, c) != 0);
}

void print_aes(uint8_t* buf)
{
	print("%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\n", 
    	buf[0],  buf[1],  buf[2],  buf[3], 
    	buf[4],  buf[5],  buf[6],  buf[7], 
    	buf[8],  buf[9],  buf[10],  buf[11], 
    	buf[12],  buf[13],  buf[14],  buf[15]);
}

void print_array(u32* buf, u32 size)
{
    print("[");
    for (u32 j=0;j<size;j++) {
        print("%u", buf[j]);
        if (j != size-1) {
            print(",");
        }
    }
    print("]");
}

void print_byte_array(char* buf, u32 size)
{
    print("[");
    for (u32 j=0;j<size;j++) {
        print("%x", buf[j]);
        if (j != size-1) {
            print(",");
        }
    }
    print("]");
}
