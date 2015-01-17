
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
#include <string.h>

#include <freertos/os.h>
#include <freertos/mmu.h>
#include <platform/wait.h>
#include <platform/console.h>
#include <platform/xen_hvm.h>
#include <platform/hypervisor.h>
#include <platform/events.h>
#include <platform/xenbus.h>
#include <platform/gnttab.h>
#include <platform/console.h>

#include <xen/io/console.h>
#include <xen/io/protocols.h>
#include <xen/io/ring.h>
#include <xen/hvm/params.h>

DECLARE_WAIT_QUEUE_HEAD(console_queue);
void *console_ring;

static inline void notify_daemon(struct consfront_dev *dev)
{
    /* Use evtchn: this is called early, before irq is set up. */
    if (dev)
        notify_remote_via_evtchn(dev->evtchn);
    else
	printk("WARN: notify_daemon called with no console device!\n");
}

int xencons_ring_send_no_notify(struct consfront_dev *dev, const char *data, unsigned len)
{	
    int sent = 0;
	struct xencons_interface *intf;
	XENCONS_RING_IDX cons, prod;

        intf = dev->ring;
        if (!intf)
            return sent;

	cons = intf->out_cons;
	prod = intf->out_prod;

	mb();
	BUG_ON((prod - cons) > sizeof(intf->out));

	while ((sent < len) && ((prod - cons) < sizeof(intf->out)))
		intf->out[MASK_XENCONS_IDX(prod++, intf->out)] = data[sent++];

	wmb();
	intf->out_prod = prod;
    
    return sent;
}

int xencons_ring_send(struct consfront_dev *dev, const char *data, unsigned len)
{
    int sent;

    sent = xencons_ring_send_no_notify(dev, data, len);
    notify_daemon(dev);

    return sent;
}	

#if 0
void console_handle_input(evtchn_port_t port, struct pt_regs *regs, void *data)
{
	struct consfront_dev *dev = (struct consfront_dev *) data;
#ifdef HAVE_LIBC
        int fd = dev ? dev->fd : -1;

        if (fd != -1)
            files[fd].read = 1;

        wake_up(&console_queue);
#else
	struct xencons_interface *intf = xencons_interface();
	XENCONS_RING_IDX cons, prod;

	cons = intf->in_cons;
	prod = intf->in_prod;
	mb();
	BUG_ON((prod - cons) > sizeof(intf->in));

	while (cons != prod) {
		xencons_rx(intf->in+MASK_XENCONS_IDX(cons,intf->in), 1, regs);
		cons++;
	}

	mb();
	intf->in_cons = cons;

	notify_daemon(dev);

	xencons_tx();
#endif
}
#endif

#ifdef HAVE_LIBC
int xencons_ring_avail(struct consfront_dev *dev)
{
	struct xencons_interface *intf;
	XENCONS_RING_IDX cons, prod;

        if (!dev)
            intf = xencons_interface();
        else
            intf = dev->ring;

	cons = intf->in_cons;
	prod = intf->in_prod;
	mb();
	BUG_ON((prod - cons) > sizeof(intf->in));

        return prod - cons;
}

int xencons_ring_recv(struct consfront_dev *dev, char *data, unsigned len)
{
	struct xencons_interface *intf;
	XENCONS_RING_IDX cons, prod;
        unsigned filled = 0;

        if (!dev)
            intf = xencons_interface();
        else
            intf = dev->ring;

	cons = intf->in_cons;
	prod = intf->in_prod;
	mb();
	BUG_ON((prod - cons) > sizeof(intf->in));

        while (filled < len && cons + filled != prod) {
                data[filled] = *(intf->in + MASK_XENCONS_IDX(cons + filled, intf->in));
                filled++;
	}

	mb();
        intf->in_cons = cons + filled;

	notify_daemon(dev);

        return filled;
}
#endif

struct consfront_dev *xencons_ring_init(void)
{
	struct consfront_dev *dev;
	uint64_t c_pfn = 0, c_evtchn = 0;

	dev = pvPortMalloc(sizeof(struct consfront_dev));
	if (!dev) {
		printk("xencons_ring_init: failed to allocate memory for console device\n");
		BUG();
	}

	memset(dev, 0, sizeof(struct consfront_dev));
	dev->nodename = "device/console";
	dev->dom = 0;
	dev->backend = 0;
	dev->ring_ref = 0;

#ifdef HAVE_LIBC
	dev->fd = -1;
#endif

	init_waitqueue_head(&console_queue);

	/* Console probe */
	if (hvm_get_parameter(HVM_PARAM_CONSOLE_EVTCHN, &c_evtchn) != 0) {
		printk("Error getting console evtchn parameter\n");
		while (1) {};
	}

	if (hvm_get_parameter(HVM_PARAM_CONSOLE_PFN, &c_pfn) != 0) {
		printk("Error getting console pfn parameter\n");
		while (1) {};
	}

	dev->evtchn = (evtchn_port_t) c_evtchn;

	/* Map console ring buffer */
	dev->ring = (struct xencons_interface *) map_frame(c_pfn);
	if (dev->ring == 0) {
		printk("Error mapping console ring buffer frame\n");
		while (1) {};
	} else {
		printk("Console ring buffer PFN %llu mapped at 0x%p, event channel %lu\n",
				c_pfn, dev->ring, dev->evtchn);
	}

#if 0
	err = bind_evtchn(dev->evtchn, console_handle_input, dev);
	if (err <= 0) {
		printk("XEN console request chn bind failed %i\n", err);
                vPortFree(dev);
		return NULL;
	}
        unmask_evtchn(dev->evtchn);
#endif

	/* In case we have in-flight data after save/restore... */
	notify_daemon(dev);

	return dev;
}

void xencons_resume(void)
{
	(void)xencons_ring_init();
}

