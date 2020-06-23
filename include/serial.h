#include <uboot/drivers/serial/serial_msm.h>
#include <types.h>

#define MSM_SERIAL_BASE 0xF995E000

int geti(struct msm_serial_data *dev);
void print(const char* format, ...);
void _putchar(char c);	
void print_aes(uint8_t* buf);
void print_array(u32* buf, u32 size);
void print_byte_array(char* buf, u32 size);
