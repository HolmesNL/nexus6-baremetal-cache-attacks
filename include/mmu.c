#include <types.h>
#include <coprocessor.h>
#include <io.h>
#include <serial.h>
#include <libflush/memory.h>
#include <mmu.h>

#define PAGE_TABLE_ADDR 0xb0000000
#define PAGE_TABLE_ENTRIES 0xaff
#define PAGE_TABLE_IO_ENTRIES 100

/*
 * Short-descriptor translation table first-level descriptor formats (p1326 ARM reference manual)
 */ 
void init_page_table()
{
    u32 section;
    //Every dword describes a section in memory of 1Mb.
    for(u32 i=0;i<PAGE_TABLE_ENTRIES;i++) {
        section = (i << 20); //Base address
        section |= 2; //Section or supersection with PXN == 0
        //bit[18] == 0, this is a section
        section |= BIT(19); //Set Non-Secure bit
        section |= BIT(16); //Shareable
        // AP[2] == 0, write access enabled
        section |= BIT(11); // AP[1] == 1, unpriviliged access
        section |= BIT(10); // AP[0] == 1 since SCTLR.AFE flag is off
        // Domain == 0
        section |= (6 << 12) | (2 << 2); // TEX = 1BB CB=AA where AA == BB == 10 to make everything cacheable memory and write-through, no write-allocate (p1368 ARM reference manual)
        // XN = 0, everything executable

        u32 address = PAGE_TABLE_ADDR + (i*4);
        writel(section, address);
    }

    //communiction with second CPU (treated same as IO)
    for (u32 i=0;i<1;i++) {
        u32 base_address = 0xaff+i;
        u32 address = PAGE_TABLE_ADDR + base_address * 4;
        section = (base_address << 20); //Base address
        section |= 3; //Section or supersection with PXN == 1
        //bit[18] == 0, this is a section
        section |= BIT(19); //Set Non-Secure bit
        section |= BIT(16); //Shareable
        // AP[2] == 0, write access enabled
        section |= BIT(11); // AP[1] == 1, unpriviliged access
        section |= BIT(10); // AP[0] == 1 since SCTLR.AFE flag is off
        // Domain == 0
        // TEX == 000, CB == 00
        section |= BIT(4); // XN = 1, not executable
        writel(section, address);
    }

    //IO memory
    //Every dword describes a section in memory of 1Mb.
    for (u32 i=0;i<PAGE_TABLE_IO_ENTRIES;i++) {
        u32 base_address = 0xF90+i;
        u32 address = PAGE_TABLE_ADDR + base_address * 4;
        section = (base_address << 20); //Base address
        section |= 3; //Section or supersection with PXN == 1
        //bit[18] == 0, this is a section
        section |= BIT(19); //Set Non-Secure bit
        section |= BIT(16); //Shareable
        // AP[2] == 0, write access enabled
        section |= BIT(11); // AP[1] == 1, unpriviliged access
        section |= BIT(10); // AP[0] == 1 since SCTLR.AFE flag is off
        // Domain == 0
        // TEX == 000, CB == 00
        section |= BIT(4); // XN = 1, not executable
        writel(section, address);
    }

}

void print_page_table()
{
    for(int i=0;i<PAGE_TABLE_ENTRIES;i++) {
        u32 address = PAGE_TABLE_ADDR + (i*4);
        print("%x = %x\r\n", address, readl(address));
    }
    for (u32 i=0;i<PAGE_TABLE_IO_ENTRIES;i++) {
        u32 base_address = 0xF99+i;
        u32 address = PAGE_TABLE_ADDR + base_address * 4;
        print("%x = %x\r\n", address, readl(address));
    }
}

void init_mmu()
{

    init_page_table();
    enable_mmu();

}

void enable_mmu()
{
    write_dacr(3); //Set D0 access permissions to 0b11 for Manager

    //Write pagetable location to TTBR0 and TTBR1
    u32 ttbr0 = PAGE_TABLE_ADDR;
    ttbr0 |= BIT(6); // IRGN = 01, write-back, write-allocate, Cacheable
    ttbr0 |= BIT(3); // RGN = 01,  Normal memory, Outer Write-Back Write-Allocate Cacheable.
    //ttbr0[5] == 0, outer Shareable
    ttbr0 |= BIT(1); // Shareable

    disable_cache();
    flush_tlbs();
    flush_I_BTB();

    write_ttbr0(ttbr0);
    // TTBCR.EAE == 0, use the short-descriptor format
    // TTBCR.N == 0 so only TTBR0 is used
    register u32 r3;
    asm volatile(
        "MRC p15, 7, %0, c15, c0, 2\n"
        "BIC %0, %0, #0x400\n"
        "MCR p15, 7, %0, c15, c0, 2\n"
        : "=r"(r3)
    );

    isb();
    enable_cache();
}
