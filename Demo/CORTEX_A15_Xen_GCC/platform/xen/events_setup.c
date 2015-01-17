
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

#include <platform/events.h>
#include <platform/hypervisor.h>
#include <platform/console.h>

evtchn_port_t debug_port = -1;

static void virq_debug(evtchn_port_t port, struct pt_regs *regs, void *params)
{
	printk("Received a virq_debug event\n");
}

void arch_init_events(void) {
	printk("events: binding debug VIRQ\n");
	debug_port = bind_virq(VIRQ_DEBUG, (evtchn_handler_t)virq_debug, 0);

	if(debug_port == -1) {
		printk("events: could not bind VIRQ!\n");
		while (1) {};
	} else {
		printk("events: bound debug port (%d)\n", debug_port);
	}

	unmask_evtchn(debug_port);
}

void arch_fini_events(void) {
	if(debug_port != -1)
	{
		mask_evtchn(debug_port);
		unbind_evtchn(debug_port);
	}
}
