
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
#include <inttypes.h>
#include <xen/hvm/params.h>
#include <xen/io/xs_wire.h>

#include <freertos/os.h>
#include <freertos/arch_mm.h>
#include <freertos/mmu.h>
#include <platform/hypervisor.h>
#include <platform/console.h>
#include <platform/xen_hvm.h>

void arch_init_xenbus(struct xenstore_domain_interface **xenstore_buf, uint32_t *store_evtchn) {
	uint64_t value;
	uint64_t xenstore_pfn;

	if (hvm_get_parameter(HVM_PARAM_STORE_EVTCHN, &value))
		BUG();

	*store_evtchn = (int)value;

	if(hvm_get_parameter(HVM_PARAM_STORE_PFN, &xenstore_pfn))
		BUG();

	printk("arch_init_xenbus: xenstore pfn = %llu\n", xenstore_pfn);

	*xenstore_buf = (struct xenstore_domain_interface *) map_frame(xenstore_pfn);
}
