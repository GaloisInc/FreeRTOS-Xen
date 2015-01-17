
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

.macro zero_bss
	@ BSS start and end MUST be word-aligned.
	ldr	r0, =_bss_start
	ldr	r1, =_bss_end
	add	r1, r1, #4
	mov	r2, #0

1:
	str	r2, [r0]

	add	r0, r0, #4
	cmp	r0, r1
	blt	1b
.endm
