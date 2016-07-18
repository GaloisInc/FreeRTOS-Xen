
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

#include <stdint.h>
#include <platform/console.h>
#include <freertos/mmu.h>
#include <freertos/asm/mm.h>

extern uint32_t l1_page_table[L1_ENTRIES];
extern uint32_t l2_page_table[L2_TOTAL][L2_ENTRIES];

struct mmu_state {
	int cur_l1_idx;
	int next_l2_idx;
} mmu_state;

void setup_mmu_state()
{
    // Set up state to prepare for 4k L2 page table mappings; this way, the
    // first call to map_frame() will start by calling alloc_l2() to allocate
    // the first L2 page table.
    mmu_state.cur_l1_idx = -1;
    mmu_state.next_l2_idx = L2_ENTRIES;

    dprintk("mmu state intialized: l1 base = 0x%x, l2 base = 0x%x, cur_l1_idx = %d, next_l2_idx = %d\n",
		    l1_page_table, l2_page_table, mmu_state.cur_l1_idx,
		    mmu_state.next_l2_idx);
}

void setup_direct_map(void)
{
    uint32_t i, flags, desc;

    dprintk("Setting up direct mapped pages: l1 base = 0x%x, start = 0x%x, end = 0x%x\n",
		    l1_page_table, DIRECT_MAP_START, DIRECT_MAP_END);

    for (i = DIRECT_MAP_START; i < DIRECT_MAP_END; i++) {
	    flags = FLAGS_READWRITE;

	    if (i >= CACHEABLE_START) {
		    flags |= FLAGS_CACHEABLE | FLAGS_BUFFERABLE;
	    }

	    // Map the page table index to itself since this is the direct map
	    // (1:1); use a 1 MB descriptor
	    desc = DESC_SECTION(i, flags);

        // If the page table entry is already present, that's because
        // boot.s put it there to get the code segment's virtual and
        // physical address mappings set up. Skip this entry if so.
        if (l1_page_table[i]) {
            dprintk("Skipping preexisting L1 page table entry %d\n", i);
        } else {
            l1_page_table[i] = desc;
        }
    }

    dprintk("Done setting up direct map\n");
}

static int alloc_l2()
{
    uint32_t desc;
    uint32_t *addr;

    dprintk("mmu: alloc_l2 called\n");

    if (mmu_state.next_l2_idx != L2_ENTRIES) {
	    printk("mmu: BUG! refused to allocate with L2 entries remaining (next_l2_idx = 0x%x, cur_l1_idx = 0x%x)\n",
			    mmu_state.next_l2_idx, mmu_state.cur_l1_idx);
	    return -1;
    }

    if (mmu_state.cur_l1_idx + 1 >= L2_TOTAL) {
	    dprintk("mmu: refused to allocate L2, no space remaining (cur_l1_idx = 0x%x)\n",
			    mmu_state.next_l2_idx, mmu_state.cur_l1_idx);
	    return -1;
    }

    dprintk("mmu: cur_l1_idx initially 0x%x, next_l2_idx 0x%x\n", mmu_state.cur_l1_idx,
		    mmu_state.next_l2_idx);

    mmu_state.cur_l1_idx++;
    mmu_state.next_l2_idx = 0;

    dprintk("mmu: alloc_l2 sees l2 = 0x%x\n", l2_page_table);

    addr = (uint32_t *) (&(l2_page_table[mmu_state.cur_l1_idx]));

    dprintk("mmu: alloc_l2 using 0x%x as l2 page table base address for cur_l1_idx = 0x%x\n",
		    addr, mmu_state.cur_l1_idx);

    desc = DESC_PAGE_TABLE((uint32_t)addr + physical_address_offset);

    dprintk("mmu: l1[0x%x + 0x%x] = 0x%x\n", L2_BASE_INDEX, mmu_state.cur_l1_idx, desc);

    l1_page_table[L2_BASE_INDEX + mmu_state.cur_l1_idx] = desc;

    return 0;
}

static inline uint32_t free_mappings()
{
    return (L2_ENTRIES - mmu_state.next_l2_idx) + ((L2_TOTAL - (mmu_state.cur_l1_idx + 1)) * L2_ENTRIES);
}

void * map_frame(unsigned long pfn)
{
    int res = 0;
    uint32_t desc;
    void * va;

    if (mmu_state.next_l2_idx == L2_ENTRIES)
	    res = alloc_l2();

    if (res) {
	    dprintk("mmu: map_frame alloc_l2 failed, res %d\n", res);
	    return 0;
    }

    if (free_mappings() == 0) {
	    dprintk("mmu: could not map page, no free mappings\n");
	    return 0;
    }

    dprintk("mmu: free_mappings() = 0x%x\n", free_mappings());

    desc = DESC_SMALL_PAGE(pfn, FLAGS_CACHEABLE | FLAGS_BUFFERABLE | SP_FLAGS_READWRITE);

    dprintk("mmu: l2[0x%x][0x%x] = 0x%x\n",
		    mmu_state.cur_l1_idx, mmu_state.next_l2_idx, desc);

    dprintk("mmu: l2 base ptr = 0x%x\n", l2_page_table);

    l2_page_table[mmu_state.cur_l1_idx][mmu_state.next_l2_idx] = desc;

    va = (void *) (((L2_BASE_INDEX + mmu_state.cur_l1_idx) << 20) | (mmu_state.next_l2_idx << 12));

    dprintk("mmu: mapped page, pfn = %ld, l1_idx = 0x%x, l2_idx = 0x%x, va = 0x%x\n",
		    pfn, mmu_state.cur_l1_idx, mmu_state.next_l2_idx, va);

    mmu_state.next_l2_idx++;

    return va;
}

void * map_frames_direct(unsigned long count, unsigned long *start_mfn)
{
    int i;
    unsigned long start_frame;
    void *start_va = NULL, *cur_va = NULL;

    if (count == 0)
        return NULL;

    if (free_mappings() < count)
        return NULL;

    start_frame = ((L2_BASE_INDEX + mmu_state.cur_l1_idx) << 8) | mmu_state.next_l2_idx;

    for (i = 0; i < count; i++) {
        cur_va = map_frame(start_frame + i);

        if (i == 0)
            start_va = cur_va;
    }

    *start_mfn = start_frame;

    return start_va;
}

void * map_frames(unsigned long *lst, unsigned long count)
{
    int i;
    void *start_va = NULL;
    void *cur_va = NULL;

    if (count == 0)
        return NULL;

    if (free_mappings() < count)
        return NULL;

    dprintk("map_frames: mapping %d frames\n", count);

    for (i = 0; i < count; i++) {
        cur_va = map_frame(lst[i]);

        if (i == 0)
            start_va = cur_va;
    }

    return start_va;
}

void mmu_setup(void)
{
	setup_mmu_state();
	setup_direct_map();
	invalidate_tlb();
	flush_caches();
	printk("MMU setup complete.\n");
}
