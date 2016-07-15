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

OUTPUT_ARCH(arm)
ENTRY(_start)
SECTIONS 
{
  os_start = 0x10008000;

  . = os_start;

  .text : {
    *(.start);
    *(.text);
  }

  . = ALIGN(4);

  .rodata : { *(.rodata); }
  .data : { *(.data); }

  . = ALIGN(4);

  /* NOTE: The space specified in the zImage header (_end - _start) MUST be no
   * bigger than the binary image size of the entire kernel. */
  _end = .;

  . = ALIGN(8);
  _start_stacks = .;
  svc_stack = _start_stacks + 16384;
  irq_stack = _start_stacks + 32768;
  firq_stack = _start_stacks + 49152;
  abt_stack = _start_stacks + 65536;
  und_stack = _start_stacks + 81920;

  . = und_stack;

  . = ALIGN(4);
  _bss_start = .;
  .bss : { *(.bss); }
  . = ALIGN(4);
  _bss_end = .;
}
