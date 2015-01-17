
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

# print msg prints the string referred to by msg to the Xen console.  Example:
#
# .section ".data"
# foo:
#   .asciz "bar\n"
# 
#   print foo
#
# Note that the string MUST be declared using 'asciz', not 'ascii', to be
# null-terminated; otherwise the call to printk will overrun the end of the
# string.
.macro print msg
    push   {r0, r1, lr}
    ldr    r0, =\msg
    bl     printk
    pop    {r0, r1, lr}
.endm
