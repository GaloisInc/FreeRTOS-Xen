
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
