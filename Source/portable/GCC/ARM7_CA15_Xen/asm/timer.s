
#include "print.s"
#include "FreeRTOSConfig.h"

    .section ".start"
    .globl   schedule_timer
    .globl   setup_timer
    .globl   system_timer_frequency

# Set up the ARM generic timer initially.  Call this once, at boot time, after
# interrupts have been configured and enabled and after the stack has been
# configured.  This schedules an initial timer interrupt; once the interrupt is
# handled, further timer interrupts will be scheduled.
setup_timer:
    push     {r0, r1, r2, lr}

    print    timer_banner

    @ Read the system clock frequency
    mrc      p15, 0, r1, c14, c0, 0

    push     {r1}

    @ r1 must hold the clock frequency
    print    cntfrq_msg

    pop      {r1}

    ldr      r0, =system_timer_frequency
    str      r1, [r0]

    mov      r2, #configTICK_RATE_HZ_ASM
    udiv     r1, r1, r2

    ldr      r0, =tick_timervalue
    str      r1, [r0]

    print    timervalue_msg

    bl       schedule_timer

    pop      {r0, r1, r2, lr}
    bx       lr

# Schedule a timer interrupt.  Call this in an interrupt handler to further
# schedule timer interrupts; requires the stack to be configured.  For more
# information on the registers used, please see the ARM Architecture Reference
# Manual (ARM 7-A/7-R edition), Chapter B8.
schedule_timer:
    push     {r0, lr}

    @ Set TimerValue (CNTV_TVAL)
    ldr      r0, =tick_timervalue
    ldr      r0, [r0]
    mcr      p15, 0, r0, c14, c3, 0

    isb

    @ Write CNTV_CTL control register
    mov      r0, #0x1
    mcr      p15, 0, r0, c14, c3, 1
    isb

    pop      {r0, lr}
    bx       lr

    .section ".data"
    .align 2

system_timer_frequency:
    .long    0x0
tick_timervalue:
    .long    0x0
