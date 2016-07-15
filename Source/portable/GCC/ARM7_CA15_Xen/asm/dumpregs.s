
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

.macro _dumpregs
    push    {r0-r12,lr}
    push    {r12, lr}

    push    {r3}
    mov     r3, r2
    mov     r2, r1
    mov     r1, r0
    ldr     r0, =info_msg2
    bl      __printk
    pop     {r3}

    mov     r1, r3
    mov     r2, r4
    mov     r3, r5
    ldr     r0, =info_msg3
    bl      __printk

    mov     r1, r6
    mov     r2, r7
    mov     r3, r8
    ldr     r0, =info_msg4
    bl      __printk

    mov     r1, r9
    mov     r2, r10
    mov     r3, r11
    ldr     r0, =info_msg5
    bl      __printk

    pop     {r12, lr}
    mov     r1, r12
    mov     r2, lr
    mrs     r3, cpsr
    ldr     r0, =info_msg6
    bl      __printk

    mrs     r1, spsr
    ldr     r0, =info_msg7
    bl      __printk

    mrc     p15, 0, r1, c5, c0, 1 @ Read IFSR into r1
    ldr     r0, =info_msg8
    bl      __printk

    mrc     p15, 0, r1, c5, c0, 0 @ Read DFSR into r1
    ldr     r0, =info_msg9
    bl      __printk

    mrc     p15, 0, r1, c6, c0, 0 @ Read DFAR into r1
    ldr     r0, =info_msg10
    bl      __printk

    pop     {r0-r12,lr}
.endm

.macro _dumpstack
    push    {r0-r12, lr}
    add     r4, sp, #56 @ 14 * 4 bytes to compensate for the push above
    
    ldr     r0, =stack_start
    bl      __printk

    ldr     r0, =stack_entry
    mov     r1, r4
    ldr     r2, [r4]
    bl      __printk

    ldr     r0, =stack_entry
    add     r1, r4, #4
    ldr     r2, [r4, #4]
    bl      __printk

    ldr     r0, =stack_entry
    add     r1, r4, #8
    ldr     r2, [r4, #8]
    bl      __printk

    ldr     r0, =stack_entry
    add     r1, r4, #12
    ldr     r2, [r4, #12]
    bl      __printk

    ldr     r0, =stack_entry
    add     r1, r4, #16
    ldr     r2, [r4, #16]
    bl      __printk

    ldr     r0, =stack_entry
    add     r1, r4, #20
    ldr     r2, [r4, #20]
    bl      __printk

    ldr     r0, =stack_entry
    add     r1, r4, #24
    ldr     r2, [r4, #24]
    bl      __printk

    ldr     r0, =stack_entry
    add     r1, r4, #28
    ldr     r2, [r4, #28]
    bl      __printk

    pop     {r0-r12, lr}
.endm
