
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

#include "print.s"
#include "dumpregs.s"

    .section ".start"
    .globl exception_vector_table
    .globl FreeRTOS_SWI_Handler
    .globl info_msg2
    .globl info_msg3
    .globl info_msg4
    .globl info_msg5
    .globl info_msg6

@ 5 low-order zero bits, i.e., 32-byte aligned
.align 5
exception_vector_table:
    b		unused
    b		undef
    b		swi_handler
    b		pabt
    b		dabt
    b		unused2
    b		irq_handler
    b		firq_handler

unused:
    push    {r1}
    mov     r1, #0
    b       1f
undef:
    push    {r1}
    mov     r1, #1
    b       1f
svc:
    push    {r1}
    mov     r1, #2
    b       1f
pabt:
    push    {r1}
    mov     r1, #3
    b       1f
dabt:
    push    {r1}
    mov     r1, #4
    b       1f
unused2:
    push    {r1}
    mov     r1, #5
    b       1f
firq_handler:
    push    {r1}
    mov     r1, #7
    b       1f
1:
    push    {r0, lr}
    ldr     r0, =info_msg1
    bl      __printk
    pop     {r0, lr}
    pop     {r1}
    _dumpregs
    b       .
