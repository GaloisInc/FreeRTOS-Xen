
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

    .section    ".start"
    .globl      install_pt_address
    .globl      set_domain_permissions
    .globl      enable_mmu

install_pt_address:
    push   {r0}

    ldr    r0, =l1_page_table
    orr    r0, r0, #TTBR_CACHEABLE_FLAGS
    mcr    p15, 0, r0, c2, c0, 0

    pop    {r0}
    bx     lr

set_domain_permissions:
    push   {r0}

    mov    r0, #3
    mcr    p15, 0, r0, c3, c0, 0
    isb

    pop    {r0}
    bx     lr

enable_mmu:
    push   {r0, r1}

    /* Turn on the L1 instruction cache (I), branch predictor (Z),
       data cache (C), and MMU */
    mrc    p15, 0, r0, c1, c0, 0

    /* 00FTN0E0XU000000LRVIZ0RS01111CAM */
    /*   ARM E P       4R               */
    /* 00000000000000000000000000000100  <-- turn on data cache */
    /* 00000000000000000001000000000000  <-- turn on instruction cache */
    /* 00000000000000000000100000000000  <-- branch predictor */
    /* 00000000000000000000000000000001  <-- MMU */
    /* 00000000000000000001100000000101  <- everything */
    movw   r1, #0x1805
    orr    r0, r0, r1
    mcr    p15, 0, r0, c1, c0, 0

    pop    {r0, r1}
    bx     lr
