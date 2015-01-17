
/*
 * Copyright (C) 2014-2015 Galois, Inc.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License** as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.

 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <freertos/mmu.h>

#include "stack.s"
#include "print.s"
#include "bss.s"

    .section ".start", "x"
    .align
    .globl    _start
    .globl    l1_page_table
    .globl    l2_page_table

_start:
    @ zImage header
.rept   8
    mov     r0, r0
.endr
    b       1f
    .word   0x016f2818      @ Magic numbers to help the loader
    .word   _start          @ absolute load/run zImage address
    .word   _end - _start   @ zImage size
    @ end of zImage header

1:
    zero_bss
    setup_stacks

    @ Install exception vector table address
    ldr    r0, =exception_vector_table
    mcr    p15, 0, r0, c12, c0, 0

    bl      flush_caches
    bl      invalidate_tlb
    bl      mmu_setup

    print boot_banner

    # NOTE: our page tables are set up to map virtual to physical 1-1, so the
    # virtual address of call_main is the same as its physical address.  If the
    # virtual memory setup ever changes we'll need to branch to the virtual
    # address of call_main after setup_page_table.  For now, just falling
    # through is sufficient because the value of PC will be the right virtual
    # address.

call_main:
    @ Make sure interrupts are disabled in the CPSR; we need to wait until the
    @ scheduler is ready before we turn them on.
    cpsid  i

    bl     fpu_enable
    bl     gic_init
    bl     platform_setup

    mov    r0, #0            @ arg 0: 0
    mov    r1, #0            @ arg 1: 0
    ldr    lr, =__exit       @ Set up LR so when main returns, it goes nowhere
    b      main

__exit:
    b      .

    .section ".data"

.align  14
l1_page_table:
    .fill (4*1024), 4, 0x0

.align  14
l2_page_table:
    .fill (L2_TOTAL*L2_ENTRIES), 4, 0x0

.align 2
boot_banner:
    .asciz "<<< FreeRTOS Startup >>>\n"
