
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

void map_grant_table()
{
    struct xen_add_to_physmap xatp;
    int i;

    /* Map grant table pages */
    for (i = (NR_GRANT_FRAMES - 1); i >= 0; i--) {
	    xatp.domid = DOMID_SELF;
	    xatp.idx = i;
	    xatp.space = XENMAPSPACE_grant_table;
	    xatp.gpfn = GNTTAB_BASE_PFN + i;

	    if (HYPERVISOR_memory_op(XENMEM_add_to_physmap, &xatp) != 0) {
		    printk("xen_setup: failed to map grant table page %d\n", i);
		    while (1) {};
	    } else {
		    printk("xen_setup: mapped grant table page %d\n", i);
	    }
    }

    gnttab_table = (grant_entry_t *) (GNTTAB_BASE_PFN << PAGE_SHIFT);
}

void xen_setup()
{
    printk("xen_setup: setting up shared info\n");
    init_shared_info();
    printk("xen_setup: setting up console\n");
    init_console();
    printk("xen_setup: setting up grant table\n");
    map_grant_table();
    init_gnttab();
    printk("xen_setup: setting up events system\n");
    init_events();
    printk("xen_setup: setting up xenbus\n");
    init_xenbus();

    printk("xen_setup: done.\n");
}
