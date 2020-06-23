
ARMGNU = /usr/bin/arm-linux-gnueabihf
CC = gcc

I_DIR = ./include
CFLAGS = -Wall -Wno-unused-variable -Wno-comment -O0 -nostdlib -nostartfiles -ffreestanding -std=gnu99 -I$(I_DIR) -mfloat-abi=hard -march=armv7-a -fno-stack-protector -finline-functions -DOPENSSL_NO_ENGINE

OBJS = start.o bootloader05.o dtb.o keymaster.o cmnlib.o $(I_DIR)/uboot/drivers/serial/serial_msm.o $(I_DIR)/lib1funcs.o $(I_DIR)/uldivmod.o eviction_tests.o profiling.o
OBJS += $(patsubst %.c, %.o, $(wildcard $(I_DIR)/libflush/*.c)) $(patsubst %.c, %.o, $(wildcard $(I_DIR)/*.c)) $(patsubst %.c, %.o, $(wildcard $(I_DIR)/openssl/*.c))
DEPS = $(wildcard $(I_DIR)/*.h) $(wildcard $(I_DIR)/libflush/*.h) $(wildcard $(I_DIR)/asm/*.h) $(wildcard *.h) $(wildcard $(I_DIR)/openssl/*.h) $(wildcard $(I_DIR)/openssl/crypto/*.h)

ifdef STRATEGY
    CFLAGS += -DSTRATEGY=${STRATEGY}
    DEPS += ${STRATEGY}
else
    CFLAGS += -DSTRATEGY=$(abspath include/strategy.h)
endif

$(info $(DEPS))
$(info $(OBJS))

all : kernel.img

clean :
	find -name "*.o" -exec rm {} \;
	rm -f *.bin
	rm -f *.hex
	rm -f *.elf
	rm -f *.list
	rm -f *.img
	rm -f *.bc
	rm -f *.clang.s

%.o : %.c $(DEPS)
	$(ARMGNU)-$(CC) -c $< -o $@ $(CFLAGS) 

%.o : %.s
	$(ARMGNU)-$(CC) -c $< -o $@ $(CFLAGS) 

%.o : %.S $(DEPS)
	$(ARMGNU)-$(CC) -c $< -o $@ $(CFLAGS) 

dtb.o: 0x6a1f58.dtb
	$(ARMGNU)-objcopy -B arm -O elf32-littlearm -I binary $^ $@

keymaster.o: keymaster.tlt
	$(ARMGNU)-objcopy -B arm -O elf32-littlearm -I binary $^ $@

cmnlib.o: cmnlib.tlt
	$(ARMGNU)-objcopy -B arm -O elf32-littlearm -I binary $^ $@

used: $(OBJS)
	cat $(OBJS) > files_used

kernel.img : loader $(OBJS)
	$(ARMGNU)-ld $(OBJS) $(LIBGCC) -T loader -o bootloader05.elf
	$(ARMGNU)-objdump -D bootloader05.elf > bootloader05.list
	$(ARMGNU)-objcopy bootloader05.elf -O ihex bootloader05.hex
	$(ARMGNU)-objcopy bootloader05.elf -O binary kernel.img

