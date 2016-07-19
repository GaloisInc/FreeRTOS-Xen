
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
#include <xen/xen.h>
#include <xen/hvm/params.h>
#include <xen/memory.h>
#include <freertos/arch_mm.h>
#include <platform/console.h>
#include <platform/events.h>
#include <platform/hypervisor.h>
#include <platform/gnttab.h>
#include <port/gic.h>

#include <libfdt.h>

union start_info_union start_info_union;
shared_info_t *HYPERVISOR_shared_info;
extern char shared_info_page[PAGE_SIZE];
extern grant_entry_t *gnttab_table;

void init_shared_info()
{
    struct xen_add_to_physmap xatp;

    /* Map shared_info page */
    xatp.domid = DOMID_SELF;
    xatp.idx = 0;
    xatp.space = XENMAPSPACE_shared_info;
    xatp.gpfn = virt_to_pfn(shared_info_page);

    if (HYPERVISOR_memory_op(XENMEM_add_to_physmap, &xatp) != 0) {
    	printk("xen_setup: failed to map shared info page!\n");
	while (1) {};
    } else {
    	printk("xen_setup: mapped shared info page\n");
    }

    HYPERVISOR_shared_info = (struct shared_info *)shared_info_page;
}

static uint64_t get_gnttab_base(void *device_tree)
{
    int hypervisor;
    int len = 0;
    const uint64_t *regs;
    uint64_t gnttab_base;

    hypervisor = fdt_node_offset_by_compatible(device_tree, -1, "xen,xen");
    BUG_ON(hypervisor < 0);

    regs = fdt_getprop(device_tree, hypervisor, "reg", &len);
    /* The property contains the address and size, 8-bytes each. */
    if (regs == NULL || len < 16) {
        printk("Bad 'reg' property: %p %d\n", regs, len);
        while (1) {};
    }

    gnttab_base = fdt64_to_cpu(regs[0]);

    printk("FDT suggests grant table base 0x%llx\n", (unsigned long long) gnttab_base);

    return gnttab_base;
}

void map_grant_table(void *device_tree)
{
    struct xen_add_to_physmap xatp;
    int i;

    gnttab_table = (void*)(uint32_t)get_gnttab_base(device_tree);

    /* Map grant table pages */
    for (i = (NR_GRANT_FRAMES - 1); i >= 0; i--) {
	    xatp.domid = DOMID_SELF;
	    xatp.idx = i;
	    xatp.space = XENMAPSPACE_grant_table;
	    xatp.gpfn = ((uint32_t)gnttab_table >> PAGE_SHIFT) + i;

	    if (HYPERVISOR_memory_op(XENMEM_add_to_physmap, &xatp) != 0) {
		    printk("xen_setup: failed to map grant table page %d\n", i);
		    while (1) {};
	    } else {
		    printk("xen_setup: mapped grant table page %d\n", i);
	    }
    }
}

void xen_setup(void *device_tree)
{
    printk("xen_setup: setting up shared info\n");
    init_shared_info();
    printk("xen_setup: setting up console\n");
    init_console();
    printk("xen_setup: setting up grant table\n");
    map_grant_table(device_tree);
    init_gnttab();
    printk("xen_setup: setting up events system\n");
    init_events();
    printk("xen_setup: setting up xenbus\n");
    init_xenbus();

    printk("xen_setup: done.\n");
}
