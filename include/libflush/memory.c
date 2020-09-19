void jump_to_memory(register void* pointer)
{
    asm volatile(
        "mov r0, pc\n"
        "mov pc, %0\n"
        :: "r"(pointer));
}

