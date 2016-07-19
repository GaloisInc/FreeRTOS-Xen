/* 
 ****************************************************************************
 * (C) 2006 - Cambridge University
 * (C) 2014 - Galois, Inc.
 ****************************************************************************
 *
 *        File: gnttab.c
 *      Author: Steven Smith (sos22@cam.ac.uk) 
 *     Changes: Grzegorz Milos (gm281@cam.ac.uk)
 *              
 *        Date: July 2006
 * 
 * Environment: Xen Minimal OS
 * Description: Simple grant tables implementation. About as stupid as it's
 *  possible to be and still work.
 *
 ****************************************************************************
 */

#include <FreeRTOS.h>
#include <semphr.h>
#include <task.h>
#include <string.h>

#include <stdint.h>
#include <stdlib.h>
#include <freertos/os.h>
#include <freertos/mmu.h>
#include <freertos/arch_mm.h>
#include <platform/gnttab.h>
#include <platform/hypervisor.h>
#include <platform/console.h>

grant_entry_t *gnttab_table;
static grant_ref_t gnttab_list[NR_GRANT_ENTRIES];
#ifdef GNT_DEBUG
static char inuse[NR_GRANT_ENTRIES];
#endif

xSemaphoreHandle gnttab_sem;

static const char * const gnttabop_error_msgs[] = GNTTABOP_error_msgs;

static void
put_free_entry(grant_ref_t ref)
{
    vTaskSuspendAll();
#ifdef GNT_DEBUG
    BUG_ON(!inuse[ref]);
    inuse[ref] = 0;
#endif
    gnttab_list[ref] = gnttab_list[0];
    gnttab_list[0]  = ref;
    xTaskResumeAll();
    xSemaphoreGive(gnttab_sem);
}

static grant_ref_t
get_free_entry(void)
{
    unsigned int ref;
    xSemaphoreTake(gnttab_sem, portMAX_DELAY);
    vTaskSuspendAll();
    ref = gnttab_list[0];
    BUG_ON(ref < NR_RESERVED_ENTRIES || ref >= NR_GRANT_ENTRIES);
    gnttab_list[0] = gnttab_list[ref];
#ifdef GNT_DEBUG
    BUG_ON(inuse[ref]);
    inuse[ref] = 1;
#endif
    xTaskResumeAll();
    return ref;
}

grant_ref_t
gnttab_grant_access(domid_t domid, unsigned long frame, int readonly)
{
    grant_ref_t ref;

    ref = get_free_entry();
    gnttab_table[ref].frame = frame;
    gnttab_table[ref].domid = domid;
    wmb();
    readonly *= GTF_readonly;
    gnttab_table[ref].flags = GTF_permit_access | readonly;
    wmb();

    dprintk("gnttab_grant_access: entry %d is now:\n", ref);
    dprintk("gnttab_grant_access:   frame = %lu\n", gnttab_table[ref].frame);
    dprintk("gnttab_grant_access:   domid = %lu\n", gnttab_table[ref].domid);
    dprintk("gnttab_grant_access:   flags = %u\n", gnttab_table[ref].flags);

    return ref;
}

grant_ref_t
gnttab_grant_transfer(domid_t domid, unsigned long pfn)
{
    grant_ref_t ref;

    ref = get_free_entry();
    gnttab_table[ref].frame = pfn;
    gnttab_table[ref].domid = domid;
    wmb();
    gnttab_table[ref].flags = GTF_accept_transfer;

    return ref;
}

int
gnttab_end_access(grant_ref_t ref)
{
    uint16_t flags, nflags;

    BUG_ON(ref >= NR_GRANT_ENTRIES || ref < NR_RESERVED_ENTRIES);

    nflags = gnttab_table[ref].flags;
    do {
        if ((flags = nflags) & (GTF_reading|GTF_writing)) {
            printk("WARNING: g.e. still in use! (%x)\n", flags);
            return 0;
        }
    } while ((nflags = synch_cmpxchg(&gnttab_table[ref].flags, flags, 0)) !=
            flags);

    put_free_entry(ref);
    return 1;
}

unsigned long
gnttab_end_transfer(grant_ref_t ref)
{
    unsigned long frame;
    uint16_t flags;

    BUG_ON(ref >= NR_GRANT_ENTRIES || ref < NR_RESERVED_ENTRIES);

    while (!((flags = gnttab_table[ref].flags) & GTF_transfer_committed)) {
        if (synch_cmpxchg(&gnttab_table[ref].flags, flags, 0) == flags) {
            printk("Release unused transfer grant.\n");
            put_free_entry(ref);
            return 0;
        }
    }

    /* If a transfer is in progress then wait until it is completed. */
    while (!(flags & GTF_transfer_completed)) {
        flags = gnttab_table[ref].flags;
    }

    /* Read the frame number /after/ reading completion status. */
    rmb();
    frame = gnttab_table[ref].frame;

    put_free_entry(ref);

    return frame;
}

const char *
gnttabop_error(int16_t status)
{
    status = -status;
    if (status < 0 || status >= ARRAY_SIZE(gnttabop_error_msgs))
	return "bad status";
    else
        return gnttabop_error_msgs[status];
}

int gnttab_map_ref(domid_t domid, grant_ref_t ref, unsigned long mfn)
{
	struct gnttab_map_grant_ref req;
	int result;

	req.host_addr = (mfn << PAGE_SHIFT);
	req.flags = GNTMAP_host_map;
	req.ref = ref;
	req.dom = domid;

	dprintk("gnttab_map_ref: mfn = 0x%x\n", mfn);
	dprintk("gnttab_map_ref: req.host_addr = %llx\n", req.host_addr);
	dprintk("gnttab_map_ref: req.dom = %d\n", req.dom);
	dprintk("gnttab_map_ref: req.ref = %d\n", req.ref);
	dprintk("gnttab_map_ref: req.flags = 0x%x\n", req.flags);

	result = HYPERVISOR_grant_table_op(GNTTABOP_map_grant_ref, &req, 1);

	if (result != 0) {
		dprintk("gnttab_map_ref: could not map ref, got result %d\n", result);
		return result;
	} else {
		dprintk("gnttab_map_ref: req.status = %d (%s)\n", req.status, gnttabop_error(req.status));
		if (req.status == 0) {
			dprintk("gnttab_map_ref: req.handle = %d\n", req.handle);
		}
		return req.status;
	}
}

void
init_gnttab(void)
{
    struct gnttab_setup_table setup;
    unsigned long frames[NR_GRANT_FRAMES];
    long result;
    int i;

    printk("Setting up grant table\n");

    gnttab_sem = xSemaphoreCreateCounting(NR_GRANT_ENTRIES, 0);
    if (gnttab_sem == NULL) {
	    printk("init_gnttab: could not create semaphore\n");
	    BUG();
    }

    printk("Grant table mapped at %p\n", gnttab_table);

#ifdef GNT_DEBUG
    memset(inuse, 1, sizeof(inuse));
#endif
    for (i = NR_RESERVED_ENTRIES; i < NR_GRANT_ENTRIES; i++)
        put_free_entry(i);

    return;

    setup.dom = DOMID_SELF;
    setup.nr_frames = NR_GRANT_FRAMES;
    set_xen_guest_handle(setup.frame_list, frames);
    
    result = HYPERVISOR_grant_table_op(GNTTABOP_setup_table, &setup, 1);
    if (result != 0) {
	    printk("Grant table setup failed, result status %d!\n", result);
	    BUG();
    }

    if (setup.status != GNTST_okay) {
	    printk("Grant table setup failed, op_status %d!\n",
			    setup.status);
	    BUG();
    }

    for (i = 0; i < NR_GRANT_FRAMES; i++) {
        dprintk("frames[%d] = %ld\n", i, frames[i]);
    }

    gnttab_table = (grant_entry_t *) map_frames(frames, NR_GRANT_FRAMES);
    printk("Grant table mapped at 0x%p\n", gnttab_table);
}

void
fini_gnttab(void)
{
    struct gnttab_setup_table setup;

    setup.dom = DOMID_SELF;
    setup.nr_frames = 0;

    HYPERVISOR_grant_table_op(GNTTABOP_setup_table, &setup, 1);
}
