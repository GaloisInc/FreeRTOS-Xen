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


#ifndef _MMU_H
#define _MMU_H

#ifndef __ASSEMBLER__
#include <stdint.h>
void * map_frames(unsigned long *lst, unsigned long count);
void * map_frame(unsigned long pfn);
void * map_frames_direct(unsigned long count, unsigned long *start_mfn);
void mmu_setup(void);
extern uint32_t physical_address_offset;
extern void * device_tree;
#endif

// Set up 1MB descriptors for the 1 MB sections with these page table indices.
// Everything at or above DIRECT_MAP_END will be unmapped initially.
#define DIRECT_MAP_START    0x0
#define DIRECT_MAP_END      0xc00

// Entries at or above this page table index should be marked cacheable.
// Entries below this index should be marked uncacheable due to being device
// I/O memory.
#define CACHEABLE_START     0x0

// Descriptor flags.
#define FLAGS_BUFFERABLE    (1 << 2)
#define FLAGS_CACHEABLE     (1 << 3)
#define FLAGS_READWRITE     ((1 << 10) | (1 << 11))
#define SP_FLAGS_READWRITE  ((1 << 4) | (1 << 5))

// Set these bits on the TTBR0 register value: IRGN0/1, RGN0/1
// ((1 << 6) | (1 << 0) | (1 << 4) | (1 << 3))
#define TTBR_CACHEABLE_FLAGS 0x59

// Bits to be set in each type of translation table descriptor we are likely to
// use.
#define DESC_FAULT          0x0

// Page table descriptor: take the top 22 bits of the page table base address,
// set bit 0
#define _PAGE_TABLE(addr)                 (((addr) & (~0x3ff)) | 0x1)
#define DESC_PAGE_TABLE(addr)             (_PAGE_TABLE(addr))

// Section descriptor: given a section (megabyte) number, shift it into the top
// 12 bits of the descriptor, set bit 1.
#define _SECTION(sec_num)                 ((sec_num << 20) | 0x2)
#define DESC_SECTION(sec_num, flags)      (_SECTION(sec_num) | flags)

// Small page descriptor: shift the PFN up 12 bits, set bit 1.
#define _SMALL_PAGE(pfn)                  ((pfn << 12) | 0x2)
#define DESC_SMALL_PAGE(pfn, flags)       (_SMALL_PAGE(pfn) | flags)

#define L2_ENTRIES             256
#define L1_ENTRIES             4096
#define L2_BASE_INDEX          DIRECT_MAP_END
#define L2_TOTAL               (L1_ENTRIES - DIRECT_MAP_END)

#endif
