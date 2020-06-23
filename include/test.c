#define five(a) a;a;a;a;a;
#define ten(a) five(a)five(a)
#define hundred(a) ten(ten(a))
#define twohundredfifty(a) hundred(a)hundred(a)ten(five(a))

void do_nothing() {
    //1KB of NOP
    twohundredfifty(asm volatile("mov r0,r0"))
}
