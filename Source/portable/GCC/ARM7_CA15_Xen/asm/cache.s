
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
    .globl flush_caches

.macro flush_dcache level
    dsb
    isb
    /* Figure out the cache size information */
    mov    r0, \level
    sub    r0, r0, #1
    lsl    r0, r0, #1
    mcr    p15, 2, r0, c0, c0, 0 /* select the cache */
    mrc    p15, 1, r0, c0, c0, 0 /* read its information */
    lsr    r9, r0, #13
    movw   r7, 0x7FFF
    and    r9, r9, r7 /* r9 = number of sets */
    lsr    r8, r0, #3
    movw   r7, 0x3FF
    and    r8, r8, r7 /* r8 = number of ways */
    /* INVARIANT:
        total number of ways = r8
        total number of sets = r9 */
    mov    r0, #0 /* curway = r0 = 0 */
    mov    r1, #0 /* curset = r1 = 0 */
1:
    lsl    r2, r0, #30  /* shift way into position */
    lsl    r3, r1, #6   /* shift set into position */
    mov    r4, \level
    sub    r4, r4, #1
    lsl    r4, r4, #1   /* shift level into position */
    orr    r5, r2, r3   /* combine them */
    orr    r5, r5, r4   /* and again */
    mcr    p15, 0, r5, c7, c14, 2  /* flush it! */
    add    r0, r0, #1   /* increment way counter */
    cmp    r0, r8       /* is curway == maxways? */
    bne    1b           /* if not, go back */
    mov    r0, #0       /* otherwise, reset curway to 0 */
    add    r1, r1, #1   /* increment set counter */
    cmp    r1, r9       /* is curset == maxsets? */
    bne    1b           /* if not, go back */
    movw   r0, #0
    mcr    p15, 0, r0, c7, c10, 4
.endm

flush_caches:
    push   {r0-r9, lr}
    /* Paranoia! */
    eor    r0, r0, r0 /* i.e., r0 = 0*/

    /* Flush the instruction cache. */
    mcr    p15, 0, r0, cr7, cr5, 0

    /* Flush the data cache */
    flush_dcache #1
    flush_dcache #2

    /* Flush the BTAC (branch predictor cache) */
    mcr    p15, 0, r0, cr7, cr5, 6 /* this may be covered by IC flush */

    pop    {r0-r9, lr}
    bx     lr
