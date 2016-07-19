
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

#include "stack.s"
#include "print.s"
#include "bss.s"

    .section ".start", "x"
    .align
    .globl    _start
    .globl    l1_page_table
    .globl    l2_page_table
    .globl    physical_address_offset
    .globl    device_tree

_start:
    @ zImage header
.rept   8
    mov     r0, r0
.endr
    b       1f
    .word   0x016f2818      @ Magic numbers to help the loader
    .word   0               @ absolute load/run zImage address (0 = use start of RAM + 0x8000)
    .word   _end - _start   @ zImage size
    @ end of zImage header

1:
    @ We don't know where Xen will load us, and since the executable
    @ must be run at a known location (_start) we need to set up the MMU
    @ just enough to start running the program in virtual address space.
    @ This technique works because the hard-coded virtual address of the
    @ binary is low enough that the physical runtime address is likely
    @ to be higher, so the subtraction here yields the right offset.
    adr	    r1, _start		    @ r1 = physical address of _start
    ldr	    r3, =_start		    @ r3 = (desired) virtual address of _start
    sub 	r9, r1, r3		    @ r9 = (physical - virtual) offset

    ldr	    r7, =l1_page_table	@ r7 = (desired) virtual addr of translation table
    add	    r1, r7, r9		    @ r1 = physical addr of translation table

    @ Save the DTB pointer and address offset
    mov     r10, r2
    mov     r11, r9

    @ Set the page table base address register
	// orr	r0, r1, #0b0001011	@ Sharable, Inner/Outer Write-Back Write-Allocate Cacheable
	orr	r0, r1, #0x59
	mcr	p15, 0, r0, c2, c0, 0	@ set TTBR0

	@ Set access permission for domains.
	@ Domains are deprecated, but we have to configure them anyway.
	@ We mark every page as being domain 0 and set domain 0 to "client mode"
	@ (client mode = use access flags in page table).
	mov	r0, #1			@ 1 = client
	mcr	p15, 0, r0, c3, c0, 0	@ DACR

	@ Template (flags) for a 1 MB page-table entry.
	@ C B = 1 1 (outer and inner write-back, write-allocate)
	@ After this, r8 = template page table entry
	ldr	r8, =(0x2 +  		/* Section entry */ \
		      0xc +  		/* C B */ \
		      (3 << 10) + 	/* Read/write */ \
		      (1 << 19))	/* Non-secure */

    @ Store the virtual end address of the kernel so we can map enough
    @ 1MB sections to cover all of it in case it is bigger than 1MB
    ldr r5, =_bss_end

    @ Load the physical start MB for the code segment
	mov	r0, pc, lsr#20

    @ Map the desired virtual address (from r4) to this physical location
	ldr	r4, =_start		        @ r4 = desired virtual address of this section
1:
    @ Set up the descriptor for this page table entry: combine the current
    @ megabyte counter (r0) with the descriptor template (r8) into r3
    mov r3, r0, lsl#20
	orr	r3, r3, r8

    @ Store the descriptor (r3) using the current virtual address (r4)
    @ as the page table index into to the page table at (r1)
	str	r3, [r1, r4, lsr#18]

    @ Advance the virtual mapping address by 1MB
    add r4, r4, #1<<20
    @ Advance the physical address by 1MB
    add r0, r0, #1
    @ Compare the virtual address mapped to the virtual _end; If the
    @ address we just mapped is before end of the kernel, we have more
    @ mappings to create
    cmp r4, r5
    blt 1b
    @ Done with virtual->physical map.

    @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
    @ Now, map the current physical address, too, so we can keep
    @ executing after the MMU is turned on

    @ Load the physical start MB address for the code segment
	mov	r0, pc, lsr#20
	lsl r0, r0, #20

    @ First, adjust r5 so that it refers to the physical _end instead of
    @ virtual _bss_end using the precomputed virtual/physical offset (r9)
    add r5, r5, r9

1:
    @ Set up the descriptor for this page table entry: combine the current
    @ physical address (r0) with the descriptor template (r8) into r3
	orr	r3, r0, r8

    @ Store a physical->physical mapping into the page table at (r1)
	str	r3, [r1, r0, lsr#18]

    @ Advance the phsyical address (and virtual, since they are the
    @ same) by 1MB
    add r0, r0, #1<<20
    @ If the address we just mapped is before end of the kernel, we have
    @ more mappings to create
    cmp r0, r5
    blt 1b

	@ Invalidate TLB
	dsb				@ Caching is off, but must still prevent reordering
	mcr	p15, 0, r1, c8, c7, 0	@ TLBIALL

	@ Enable MMU / SCTLR
	mrc	p15, 0, r1, c1, c0, 0	@ SCTLR
	orr	r1, r1, #3 << 11	@ enable icache, branch prediction
	orr	r1, r1, #4 + 1		@ enable dcache, MMU
	mcr	p15, 0, r1, c1, c0, 0	@ SCTLR
	isb

    @ Branch to virtual address of stage2 now that the MMU is on
	ldr	r1, =stage2
	bx	r1

stage2:
    zero_bss
    setup_stacks

    @ Install exception vector table address
    ldr    r0, =exception_vector_table
    mcr    p15, 0, r0, c12, c0, 0

    bl      flush_caches
    bl      invalidate_tlb
    bl      mmu_setup

    print boot_banner

    # NOTE: our page tables are set up to map virtual to physical 1-1, so the
    # virtual address of call_main is the same as its physical address.  If the
    # virtual memory setup ever changes we'll need to branch to the virtual
    # address of call_main after setup_page_table.  For now, just falling
    # through is sufficient because the value of PC will be the right virtual
    # address.

call_main:
    @ Make sure interrupts are disabled in the CPSR; we need to wait until the
    @ scheduler is ready before we turn them on.
    cpsid  i

    bl     fpu_enable

    @ Store the address offset in a global
    ldr    r0, =physical_address_offset
    str    r11, [r0]

    @ Store the device tree virtual address
    ldr    r0, =device_tree
    str    r10, [r0]

    bl     gic_init
    bl     platform_setup

    mov    r0, #0            @ arg 0: 0
    mov    r1, #0            @ arg 1: 0
    ldr    lr, =__exit       @ Set up LR so when main returns, it goes nowhere
    b      main

__exit:
    b      .

    .section ".data"
.align 2
physical_address_offset:
    .word 0x0

.align 2
device_tree:
    .word 0x0

.align  14
l1_page_table:
    .fill (4*1024), 4, 0x0

.align  14
l2_page_table:
    .fill (L2_TOTAL*L2_ENTRIES), 4, 0x0

.align 2
boot_banner:
    .asciz "<<< FreeRTOS Startup >>>\n"
