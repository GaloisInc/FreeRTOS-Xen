
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

    .section ".start"
    .globl   fpu_enable

fpu_enable:
    MRC     p15, 0, r0, c1, c0, 2
    ORR     r0, r0, #(3<<20)
    ORR     r0, r0, #(3<<22)
    BIC     r0, r0, #(3<<30)
    MCR     p15, 0, r0, c1, c0, 2

    ISB
    MOV     r0, #(1<<30)
    VMSR    FPEXC, r0

    print   fpu_enabled

    bx      lr
