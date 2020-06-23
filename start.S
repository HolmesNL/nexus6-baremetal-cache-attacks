#define __ASSEMBLY__

#include <smp.h>

.globl _start
_start:
    ldr sp,=CPU0_SP
	ldr r1,=(_binary_keymaster_tlt_start)     //make keymaster_mdt the second argument of notmain
	ldr r2,=(_binary_cmnlib_tlt_start)        //make cmnlib_tlt the second argument of notmain
    bl notmain
	.int 0                                    // 0x80008: image load offset
    .int 0                                    // 0x80010: effective image size
    .quad 0                                   // 0x80018: kernel flags
    .quad 0                                   // 0x80020: reserved
    .int 0                                    // 0x80028: reserved
    .int (_binary_0x6a1f58_dtb_start - _start)// 0x8002c
    .int (_binary_keymaster_tlt_start- _start)// 0x80030
    .quad 0                                   // 0x80030: reserved
    .int 0x644d5241                           // 0x80038
    .int 0                                    // 0x8003c

.globl _start1
_start1:
    mrc p15, 0, r5, c0, c0, 5                 // read MPIDR
    and r5, r5, #15                           // get CPU id
    ldr r1, =CPU_VARS_SIZE                    // calculate the cpu_vars_base == flag address
    mul r5, r5, r1
    ldr r1, =CPU_VARS_BASE
    add r5, r5, r1                            //r5 = flag address
    mov r1, #0
    str r1, [r5]                              //store 0 in the flag
_check_flag:
    ldr r1, [r5]                              //read the flag
    cmp r1, #0                                //check if it is 0
    beq _check_flag                           //If flag is 0 we do not have a function to execute
    add r5, r5, #4                            //Add 4 to start reading the sp instead of flag
    ldr sp, [r5]
    ldr r4, [r5, #4]                          //read function
    ldr r0, [r5, #8]                          //read arguments
    ldr r1, [r5, #12]
    ldr r2, [r5, #16]
    ldr r3, [r5, #20]
    blx r4                                    //execute function
    b _start1                                 //wait for new function to execute
