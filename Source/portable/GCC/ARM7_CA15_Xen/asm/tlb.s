
    .section ".start"
    .globl   invalidate_tlb

invalidate_tlb:
    push   {r0, lr}

    /* Invalidate the TLB(s) */
    mov    r0, #0
    mcr    p15, 0, r0, c8, c7, 0 /* invalidate unified TLB */
    mcr    p15, 0, r0, c8, c6, 0 /* invalidate data TLB */
    mcr    p15, 0, r0, c8, c5, 0 /* invalidated instruction TLB */

    /* U-Boot does two operations on what the Cortex-A9 manual says are
       deprecated registers. I've left them in for now, but we might
       consider taking them out. -ACW */
    mcr    p15, 0, r0, c7, c10, 4 /* "data syncronization barrier" */
    mcr    p15, 0, r0, c7, c5, 4 /* "instruction syncronization barrier" */

    pop    {r0, lr}
    bx     lr
