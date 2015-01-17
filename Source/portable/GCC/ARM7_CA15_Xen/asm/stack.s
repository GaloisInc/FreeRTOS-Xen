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
