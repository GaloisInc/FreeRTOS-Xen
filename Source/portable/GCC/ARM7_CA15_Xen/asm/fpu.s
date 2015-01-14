
#include "print.s"

    .section ".start"
    .globl   fpu_enable

fpu_enable:
    MRC     p15, 0, r0, c1, c0, 2
    ORR     r0, r0, #(3<<20)
    ORR     r0, r0, #(3<<22)
    BIC     r0, r0, #(3<<30)
    MCR     p15, 0, r0, c1, c0, 2

    ISB
    MOV     r0, #(1<<30)
    VMSR    FPEXC, r0

    print   fpu_enabled

    bx      lr
