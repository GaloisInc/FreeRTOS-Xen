
    .section ".data"
    .globl timer_banner
    .globl cntfrq_msg
    .globl timervalue_msg
    .globl interrupts_enabled
    .globl interrupts_disabled
    .globl jumpmsg
    .globl stack_start
    .globl stack_entry
    .globl info_msg1
    .globl info_msg2
    .globl info_msg3
    .globl info_msg4
    .globl info_msg5
    .globl info_msg6
    .globl info_msg7
    .globl fpu_enabled

timer_banner:
    .asciz "Enabling virtual timer...\n"
cntfrq_msg:
    .asciz "System counter frequency: %ld hz\n"
timervalue_msg:
    .asciz "Tick timervalue: %ld\n"
interrupts_enabled:
    .asciz "Interrupts enabled, disabling\n"
interrupts_disabled:
    .asciz "Interrupts already disabled\n"
jumpmsg:
    .asciz "Restored context, resuming task\n"
stack_start:
    .asciz "--- STACK ---\n"
stack_entry:
    .asciz " 0x%.8x: 0x%.8x\n"
info_msg1:
    .asciz "Abort! vector %d\n"
info_msg2:
    .asciz "  r0 = 0x%.8x    r1 = 0x%.8x    r2 = 0x%.8x\n"
info_msg3:
    .asciz "  r3 = 0x%.8x    r4 = 0x%.8x    r5 = 0x%.8x\n"
info_msg4:
    .asciz "  r6 = 0x%.8x    r7 = 0x%.8x    r8 = 0x%.8x\n"
info_msg5:
    .asciz "  r9 = 0x%.8x   r10 = 0x%.8x   r11 = 0x%.8x\n"
info_msg6:
    .asciz " r12 = 0x%.8x    lr = 0x%.8x  cpsr = 0x%.8x\n"
info_msg7:
    .asciz "spsr = 0x%.8x\n"
fpu_enabled:
    .asciz "FPU enabled\n"
