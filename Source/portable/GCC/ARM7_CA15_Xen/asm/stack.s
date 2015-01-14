#include "arm_modes.s"

.macro setup_stacks
    @ Set up stack pointers
    CPS  #ABT_MODE
    LDR  sp, =abt_stack

    CPS  #FIQ_MODE
    LDR  sp, =firq_stack

    CPS  #IRQ_MODE
    LDR  sp, =irq_stack

    CPS  #UND_MODE
    LDR  sp, =und_stack

    @ Set up SVC mode stack and continue in this mode.
    CPS  #SVC_MODE
    LDR  sp, =svc_stack
.endm
