
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
