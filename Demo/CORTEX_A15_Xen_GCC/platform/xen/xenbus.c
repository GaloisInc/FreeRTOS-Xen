/* 
 ****************************************************************************
 * (C) 2006 - Cambridge University
 * (C) 2014 - Galois, Inc.
 ****************************************************************************
 *
 *        File: xenbus.c
 *      Author: Steven Smith (sos22@cam.ac.uk) 
 *     Changes: Grzegorz Milos (gm281@cam.ac.uk)
 *     Changes: John D. Ramsdell
 *              
 *        Date: Jun 2006, chages Aug 2005
 * 
 * Environment: Xen Minimal OS
 * Description: Minimal implementation of xenbus
 *
 ****************************************************************************
 **/

#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

#include <freertos/os.h>
#include <freertos/spinlock.h>
#include <platform/xenbus.h>
#include <platform/events.h>
#include <platform/waittypes.h>
#include <platform/wait.h>
#include <platform/hypervisor.h>
#include <platform/console.h>
#include <platform/interrupts.h>

#include <xen/io/xs_wire.h>

void test_xenbus(void *);

#define min(x,y) ({                       \
        typeof(x) tmpx = (x);                 \
        typeof(y) tmpy = (y);                 \
        tmpx < tmpy ? tmpx : tmpy;            \
        })

static struct xenstore_domain_interface *xenstore_buf;
static uint32_t store_evtchn;
static DECLARE_WAIT_QUEUE_HEAD(xb_waitq);
DECLARE_WAIT_QUEUE_HEAD(xenbus_watch_queue);
xSemaphoreHandle xb_write_sem;

xenbus_event_queue xenbus_events;
static struct watch {
    char *token;
    xenbus_event_queue *events;
    struct watch *next;
} *watches;
struct xenbus_req_info 
{
    int in_use:1;
    int ready;
    DECLARE_WAIT_QUEUE_HEAD(waitq);
    void *reply;
};

#define NR_REQS 32
static struct xenbus_req_info req_info[NR_REQS];

static void memcpy_from_ring(const void *Ring,
        void *Dest,
        int off,
        int len)
{
    int c1, c2;
    const char *ring = Ring;
    char *dest = Dest;
    c1 = min(len, XENSTORE_RING_SIZE - off);
    c2 = len - c1;
    memcpy(dest, ring + off, c1);
    memcpy(dest + c1, ring, c2);
}

struct xenbus_event *xenbus_wait_for_watch_return(xenbus_event_queue *queue)
{
    struct xenbus_event *event;

    if (!queue)
        queue = &xenbus_events;
    while (!(event = *queue)) {
        wait_on(&xenbus_watch_queue);
    }
    *queue = event->next;
    return event;
}

void xenbus_wait_for_watch(xenbus_event_queue *queue)
{
    struct xenbus_event *event;

    if (!queue)
        queue = &xenbus_events;
    event = xenbus_wait_for_watch_return(queue);
    if (event) {
        vPortFree(event);
    }
    else
        printk("unexpected path returned by watch\n");
}

char* xenbus_wait_for_value(const char* path, const char* value, xenbus_event_queue *queue)
{
    if (!queue)
        queue = &xenbus_events;
    for(;;)
    {
        char *res, *msg;
        int r;

        msg = xenbus_read(XBT_NIL, path, &res);
        if(msg) return msg;

        r = strcmp(value,res);
        vPortFree(res);

        if(r==0) break;
        else xenbus_wait_for_watch(queue);
    }
    return NULL;
}

char *xenbus_switch_state(xenbus_transaction_t xbt, const char* path, XenbusState state)
{
    char *current_state;
    char *msg = NULL;
    char *msg2 = NULL;
    char value[2];
    XenbusState rs;
    int xbt_flag = 0;
    int retry = 0;

    do {
        if (xbt == XBT_NIL) {
            msg = xenbus_transaction_start(&xbt);
            if (msg) goto exit;
            xbt_flag = 1;
        }

        msg = xenbus_read(xbt, path, &current_state);
        if (msg) goto exit;

        rs = (XenbusState) (current_state[0] - '0');
        vPortFree(current_state);
        if (rs == state) {
            msg = NULL;
            goto exit;
        }

        snprintf(value, 2, "%d", state);
        msg = xenbus_write(xbt, path, value);

exit:
        if (xbt_flag) {
            msg2 = xenbus_transaction_end(xbt, 0, &retry);
            xbt = XBT_NIL;
        }
        if (msg == NULL && msg2 != NULL)
            msg = msg2;
    } while (retry);

    return msg;
}

char *xenbus_wait_for_state_change(const char* path, XenbusState *state, xenbus_event_queue *queue)
{
    if (!queue)
        queue = &xenbus_events;
    for(;;)
    {
        char *res, *msg;
        XenbusState rs;

        msg = xenbus_read(XBT_NIL, path, &res);
        if(msg) return msg;

        rs = (XenbusState) (res[0] - 48);
        vPortFree(res);

        if (rs == *state)
            xenbus_wait_for_watch(queue);
        else {
            *state = rs;
            break;
        }
    }
    return NULL;
}


static void xenbus_thread_func(void *ign)
{
    struct xsd_sockmsg msg;
    unsigned prod;

    dprintk("thread: starting up; xenstore_buf = 0x%x\n", xenstore_buf);

    prod = xenstore_buf->rsp_prod;

    for (;;) 
    {
        wait_event(&xb_waitq, prod != xenstore_buf->rsp_prod);
        while (1) 
        {
            prod = xenstore_buf->rsp_prod;
            dprintk("Rsp_cons %d, rsp_prod %d.\n", xenstore_buf->rsp_cons,
                    xenstore_buf->rsp_prod);
            if (xenstore_buf->rsp_prod - xenstore_buf->rsp_cons < sizeof(msg))
                break;
            rmb();
            memcpy_from_ring(xenstore_buf->rsp,
                    &msg,
                    MASK_XENSTORE_IDX(xenstore_buf->rsp_cons),
                    sizeof(msg));
            dprintk("Msg len %d, %d avail, id %d.\n",
                    msg.len + sizeof(msg),
                    xenstore_buf->rsp_prod - xenstore_buf->rsp_cons,
                    msg.req_id);
            if (xenstore_buf->rsp_prod - xenstore_buf->rsp_cons <
                    sizeof(msg) + msg.len)
                break;

            dprintk("Message is good.\n");

            if(msg.type == XS_WATCH_EVENT)
            {
		struct xenbus_event *event = pvPortMalloc(sizeof(*event) + msg.len);
		if (event == NULL) {
			printk("ERROR: failed to allocate xenbus_event\n");
			BUG();
		}

                xenbus_event_queue *events = NULL;
		char *data = (char*)event + sizeof(*event);
                struct watch *watch;

                memcpy_from_ring(xenstore_buf->rsp,
		    data,
                    MASK_XENSTORE_IDX(xenstore_buf->rsp_cons + sizeof(msg)),
                    msg.len);

		event->path = data;
		event->token = event->path + strlen(event->path) + 1;

                xenstore_buf->rsp_cons += msg.len + sizeof(msg);

		if (watches)
			for (watch = watches; watch; watch = watch->next)
				if (!strcmp(watch->token, event->token)) {
					events = watch->events;
					break;
				}

                if (events) {
                    event->next = *events;
                    *events = event;
                    wake_up(&xenbus_watch_queue);
                } else {
                    printk("unexpected watch token %s\n", event->token);
                    vPortFree(event);
                }
            }

            else
            {
                req_info[msg.req_id].reply = pvPortMalloc(sizeof(msg) + msg.len);
		BUG_ON(!req_info[msg.req_id].in_use);
		if (req_info[msg.req_id].reply == NULL) {
			printk("ERROR: failed to allocate reply buffer\n");
			BUG();
		}

                memcpy_from_ring(xenstore_buf->rsp,
                    req_info[msg.req_id].reply,
                    MASK_XENSTORE_IDX(xenstore_buf->rsp_cons),
                    msg.len + sizeof(msg));
                xenstore_buf->rsp_cons += msg.len + sizeof(msg);
		req_info[msg.req_id].ready = 1;
		dprintk("waking up waiters for request %d\n", msg.req_id);
                wake_up(&req_info[msg.req_id].waitq);
            }
        }
    }
}

static void xenbus_evtchn_handler(evtchn_port_t port, struct pt_regs *regs,
				  void *ign)
{
    wake_up_from_isr(&xb_waitq);
}

static int nr_live_reqs;
static DEFINE_SPINLOCK(req_lock);
static DECLARE_WAIT_QUEUE_HEAD(req_wq);

/* Release a xenbus identifier */
void release_xenbus_id(int id)
{
    BUG_ON(!req_info[id].in_use);
    spin_lock(&req_lock);
    req_info[id].in_use = 0;
    nr_live_reqs--;
    req_info[id].in_use = 0;
    if (nr_live_reqs == NR_REQS - 1)
        wake_up(&req_wq);
    spin_unlock(&req_lock);
}

/* Allocate an identifier for a xenbus request.  Blocks if none are
   available. */
int allocate_xenbus_id(void)
{
    static int probe;
    int o_probe;

    while (1) 
    {
        spin_lock(&req_lock);
        if (nr_live_reqs < NR_REQS)
            break;
        spin_unlock(&req_lock);
        wait_event(&req_wq, (nr_live_reqs < NR_REQS));
    }

    o_probe = probe;
    for (;;) 
    {
        if (!req_info[o_probe].in_use)
            break;
        o_probe = (o_probe + 1) % NR_REQS;
        BUG_ON(o_probe == probe);
    }
    nr_live_reqs++;
    req_info[o_probe].in_use = 1;
    req_info[o_probe].ready = 0;
    probe = (o_probe + 1) % NR_REQS;
    spin_unlock(&req_lock);
    init_waitqueue_head(&req_info[o_probe].waitq);

    return o_probe;
}

void arch_init_xenbus(struct xenstore_domain_interface **xenstore_buf, uint32_t *store_evtchn);

/* Initialise xenbus. */
void init_xenbus(void)
{
    portBASE_TYPE ret;

    dprintk("init_xenbus called.\n");

    init_waitqueue_head(&xb_waitq);
    init_waitqueue_head(&req_wq);
    init_waitqueue_head(&xenbus_watch_queue);
    watches = NULL;
    xb_write_sem = xSemaphoreCreateMutex();

    arch_init_xenbus(&xenstore_buf, &store_evtchn);

    dprintk("init_xenbus: buf = 0x%x, evtchn = %d\n", xenstore_buf, store_evtchn);

    ret = xTaskCreate(xenbus_thread_func, ( signed portCHAR * ) "xenbusTask", 4096,
		    NULL, configXENBUS_TASK_PRIORITY, NULL);
    if (ret != pdPASS) {
	    printk("Error creating xenbus task, status was %d\n", ret);
	    BUG();
    }

    bind_evtchn(store_evtchn, xenbus_evtchn_handler, NULL);

    unmask_evtchn(store_evtchn);

    gic_register_handler(EVENT_IRQ, handle_event);
    gic_enable_interrupt(EVENT_IRQ /* interrupt number */,
		    0x1 /*cpu_set*/, 1 /*level_sensitive*/, 0 /* ppi */);
    gic_set_priority(EVENT_IRQ, configEVENT_IRQ_PRIORITY << portPRIORITY_SHIFT);
    printk("Xen event IRQ enabled at priority %d\n", configEVENT_IRQ_PRIORITY);

    printk("xenbus initialised on event channel %d\n", store_evtchn);
}

void fini_xenbus(void)
{
}

/* Send data to xenbus.  This can block.  All of the requests are seen
   by xenbus as if sent atomically.  The header is added
   automatically, using type %type, req_id %req_id, and trans_id
   %trans_id. */
void xb_write(int type, int req_id, xenbus_transaction_t trans_id,
		     const struct write_req *req, int nr_reqs)
{
    XENSTORE_RING_IDX prod;
    int r;
    int len = 0;
    const struct write_req *cur_req;
    int req_off;
    int total_off;
    int this_chunk;
    struct xsd_sockmsg m = {.type = type, .req_id = req_id,
        .tx_id = trans_id };
    struct write_req header_req = { &m, sizeof(m) };

    xSemaphoreTake(xb_write_sem, portMAX_DELAY);
    dprintk("xb_write: started\n");

    for (r = 0; r < nr_reqs; r++)
        len += req[r].len;
    m.len = len;
    len += sizeof(m);

    cur_req = &header_req;

    BUG_ON(len > XENSTORE_RING_SIZE);

    /* Wait for the ring to drain to the point where we can send the
       message. */
    prod = xenstore_buf->req_prod;
    dprintk("xb_write: prod initially 0x%x\n", prod);
    if (prod + len - xenstore_buf->req_cons > XENSTORE_RING_SIZE) 
    {
        /* Wait for there to be space on the ring */
        dprintk("prod %d, len %d, cons %d, size %d; waiting.\n",
                prod, len, xenstore_buf->req_cons, XENSTORE_RING_SIZE);
        wait_event(&xb_waitq,
                xenstore_buf->req_prod + len - xenstore_buf->req_cons <=
                XENSTORE_RING_SIZE);
        dprintk("Back from wait.\n");
        prod = xenstore_buf->req_prod;
    }
    dprintk("xb_write: prod then 0x%x\n", prod);

    /* We're now guaranteed to be able to send the message without
       overflowing the ring.  Do so. */
    total_off = 0;
    req_off = 0;
    while (total_off < len) 
    {
        this_chunk = min(cur_req->len - req_off,
                XENSTORE_RING_SIZE - MASK_XENSTORE_IDX(prod));

	dprintk("xb_write: copying into ring buffer at 0x%x\n",
			(char *)xenstore_buf->req + MASK_XENSTORE_IDX(prod));

        memcpy((char *)xenstore_buf->req + MASK_XENSTORE_IDX(prod),
                (char *)cur_req->data + req_off, this_chunk);
        prod += this_chunk;
        req_off += this_chunk;
        total_off += this_chunk;
        if (req_off == cur_req->len) 
        {
            req_off = 0;
            if (cur_req == &header_req)
                cur_req = req;
            else
                cur_req++;
        }
    }

    dprintk("Complete main loop of xb_write.\n");
    BUG_ON(req_off != 0);
    BUG_ON(total_off != len);
    BUG_ON(prod > xenstore_buf->req_cons + XENSTORE_RING_SIZE);

    /* Remote must see entire message before updating indexes */
    wmb();

    xenstore_buf->req_prod += len;
    dprintk("xb_write: req_prod finally 0x%x\n", xenstore_buf->req_prod);

    xSemaphoreGive(xb_write_sem);

    /* Send evtchn to notify remote */
    notify_remote_via_evtchn(store_evtchn);
}

/* Send a mesasge to xenbus, in the same fashion as xb_write, and
   block waiting for a reply.  The reply is malloced and should be
   freed by the caller. */
struct xsd_sockmsg *
xenbus_msg_reply(int type,
		 xenbus_transaction_t trans,
		 struct write_req *io,
		 int nr_reqs)
{
    int id;
    struct xsd_sockmsg *rep;

    id = allocate_xenbus_id();
    xb_write(type, id, trans, io, nr_reqs);

    dprintk("waiting on response to request %d\n", id);
    wait_event(&req_info[id].waitq, req_info[id].ready);
    dprintk("woke up on response to request %d\n", id);

    rmb();
    rep = req_info[id].reply;
    BUG_ON(rep->req_id != id);
    release_xenbus_id(id);
    return rep;
}

static char *errmsg(struct xsd_sockmsg *rep)
{
    char *res;
    if (!rep) {
	char msg[] = "No reply";
	size_t len = strlen(msg) + 1;
	return memcpy(pvPortMalloc(len), msg, len);
    }
    if (rep->type != XS_ERROR)
	return NULL;
    res = pvPortMalloc(rep->len + 1);
    memcpy(res, rep + 1, rep->len);
    res[rep->len] = 0;
    vPortFree(rep);
    return res;
}	

/* Send a debug message to xenbus.  Can block. */
static void xenbus_debug_msg(const char *msg)
{
    int len = strlen(msg);
    struct write_req req[] = {
        { "print", sizeof("print") },
        { msg, len },
        { "", 1 }};
    struct xsd_sockmsg *reply;

    reply = xenbus_msg_reply(XS_DEBUG, 0, req, ARRAY_SIZE(req));
    printk("Got a reply, type %d, id %d, len %d.\n",
            reply->type, reply->req_id, reply->len);
}

/* List the contents of a directory.  Returns a malloc()ed array of
   pointers to malloc()ed strings.  The array is NULL terminated.  May
   block. */
char *xenbus_ls(xenbus_transaction_t xbt, const char *pre, char ***contents)
{
    struct xsd_sockmsg *reply, *repmsg;
    struct write_req req[] = { { pre, strlen(pre)+1 } };
    int nr_elems, x, i;
    char **res, *msg;

    dprintk("xenbus_ls: called\n");
    repmsg = xenbus_msg_reply(XS_DIRECTORY, xbt, req, ARRAY_SIZE(req));
    msg = errmsg(repmsg);
    if (msg) {
	*contents = NULL;
	return msg;
    }
    reply = repmsg + 1;
    for (x = nr_elems = 0; x < repmsg->len; x++)
        nr_elems += (((char *)reply)[x] == 0);
    res = pvPortMalloc(sizeof(res[0]) * (nr_elems + 1));
    for (x = i = 0; i < nr_elems; i++) {
        int l = strlen((char *)reply + x);
        res[i] = pvPortMalloc(l + 1);
        memcpy(res[i], (char *)reply + x, l + 1);
        x += l + 1;
    }
    res[i] = NULL;
    vPortFree(repmsg);
    *contents = res;
    return NULL;
}

char *xenbus_read(xenbus_transaction_t xbt, const char *path, char **value)
{
    struct write_req req[] = { {path, strlen(path) + 1} };
    struct xsd_sockmsg *rep;
    char *res, *msg;

    dprintk("xenbus_read: called\n");
    rep = xenbus_msg_reply(XS_READ, xbt, req, ARRAY_SIZE(req));
    msg = errmsg(rep);
    if (msg) {
	*value = NULL;
	return msg;
    }
    res = pvPortMalloc(rep->len + 1);
    memcpy(res, rep + 1, rep->len);
    res[rep->len] = 0;
    vPortFree(rep);
    *value = res;
    return NULL;
}

char *xenbus_mkdir(xenbus_transaction_t xbt, const char *path)
{
     return xenbus_write(xbt, path, "");
}

char *xenbus_write(xenbus_transaction_t xbt, const char *path, const char *value)
{
    struct write_req req[] = { 
	{path, strlen(path) + 1},
	{value, strlen(value)},
    };
    struct xsd_sockmsg *rep;
    char *msg;

    dprintk("xenbus_write: called\n");
    rep = xenbus_msg_reply(XS_WRITE, xbt, req, ARRAY_SIZE(req));
    msg = errmsg(rep);
    if (msg) return msg;
    vPortFree(rep);
    return NULL;
}

char* xenbus_watch_path_token( xenbus_transaction_t xbt, const char *path, const char *token, xenbus_event_queue *events)
{
    struct xsd_sockmsg *rep;

    struct write_req req[] = { 
        {path, strlen(path) + 1},
	{token, strlen(token) + 1},
    };

    struct watch *watch = pvPortMalloc(sizeof(*watch));

    char *msg;

    dprintk("xenbus_watch_path_token: called\n");

    if (!events)
        events = &xenbus_events;

    watch->token = strdup(token);
    watch->events = events;
    watch->next = watches;
    watches = watch;

    rep = xenbus_msg_reply(XS_WATCH, xbt, req, ARRAY_SIZE(req));

    msg = errmsg(rep);
    if (msg) return msg;
    vPortFree(rep);

    return NULL;
}

char* xenbus_unwatch_path_token( xenbus_transaction_t xbt, const char *path, const char *token)
{
    struct xsd_sockmsg *rep;

    struct write_req req[] = { 
        {path, strlen(path) + 1},
	{token, strlen(token) + 1},
    };

    struct watch *watch, **prev;

    char *msg;

    dprintk("xenbus_unwatch_path_token: called\n");
    rep = xenbus_msg_reply(XS_UNWATCH, xbt, req, ARRAY_SIZE(req));

    msg = errmsg(rep);
    if (msg) return msg;
    vPortFree(rep);

    for (prev = &watches, watch = *prev; watch; prev = &watch->next, watch = *prev)
        if (!strcmp(watch->token, token)) {
            vPortFree(watch->token);
            *prev = watch->next;
            vPortFree(watch);
            break;
        }

    return NULL;
}

char *xenbus_rm(xenbus_transaction_t xbt, const char *path)
{
    struct write_req req[] = { {path, strlen(path) + 1} };
    struct xsd_sockmsg *rep;
    char *msg;

    dprintk("xenbus_rm: called\n");
    rep = xenbus_msg_reply(XS_RM, xbt, req, ARRAY_SIZE(req));
    msg = errmsg(rep);
    if (msg)
	return msg;
    vPortFree(rep);
    return NULL;
}

char *xenbus_get_perms(xenbus_transaction_t xbt, const char *path, char **value)
{
    struct write_req req[] = { {path, strlen(path) + 1} };
    struct xsd_sockmsg *rep;
    char *res, *msg;

    dprintk("xenbus_get_perms: called\n");
    rep = xenbus_msg_reply(XS_GET_PERMS, xbt, req, ARRAY_SIZE(req));
    msg = errmsg(rep);
    if (msg) {
	*value = NULL;
	return msg;
    }
    res = pvPortMalloc(rep->len + 1);
    memcpy(res, rep + 1, rep->len);
    res[rep->len] = 0;
    vPortFree(rep);
    *value = res;
    return NULL;
}

#define PERM_MAX_SIZE 32
char *xenbus_set_perms(xenbus_transaction_t xbt, const char *path, domid_t dom, char perm)
{
    char value[PERM_MAX_SIZE];
    struct write_req req[] = { 
	{path, strlen(path) + 1},
	{value, 0},
    };
    struct xsd_sockmsg *rep;
    char *msg;

    dprintk("xenbus_set_perms: called\n");

    snprintf(value, PERM_MAX_SIZE, "%c%hu", perm, dom);
    req[1].len = strlen(value) + 1;

    rep = xenbus_msg_reply(XS_SET_PERMS, xbt, req, ARRAY_SIZE(req));
    msg = errmsg(rep);
    if (msg)
	return msg;
    vPortFree(rep);
    return NULL;
}

char *xenbus_transaction_start(xenbus_transaction_t *xbt)
{
    /* xenstored becomes angry if you send a length 0 message, so just
       shove a nul terminator on the end */
    struct write_req req = { "", 1};
    struct xsd_sockmsg *rep;
    char *err;

    dprintk("xenbus_transaction_start: called\n");
    rep = xenbus_msg_reply(XS_TRANSACTION_START, 0, &req, 1);
    err = errmsg(rep);
    if (err)
	return err;
    sscanf((const char *)(rep + 1), "%lu", xbt);
    vPortFree(rep);
    return NULL;
}

char *
xenbus_transaction_end(xenbus_transaction_t t, int abort, int *retry)
{
    struct xsd_sockmsg *rep;
    struct write_req req;
    char *err;

    *retry = 0;

    req.data = abort ? "F" : "T";
    req.len = 2;

    dprintk("xenbus_transaction_end: called\n");
    rep = xenbus_msg_reply(XS_TRANSACTION_END, t, &req, 1);
    err = errmsg(rep);
    if (err) {
	if (!strcmp(err, "EAGAIN")) {
	    *retry = 1;
	    vPortFree(err);
	    return NULL;
	} else {
	    return err;
	}
    }
    vPortFree(rep);
    return NULL;
}

int xenbus_read_integer(const char *path)
{
    char *res, *buf;
    int t;

    res = xenbus_read(XBT_NIL, path, &buf);
    if (res) {
	vPortFree(res);
	return -1;
    }
    sscanf(buf, "%d", &t);
    vPortFree(buf);
    return t;
}

int xenbus_read_uuid(const char* path, unsigned char uuid[16]) {
   char * res, *buf;
   res = xenbus_read(XBT_NIL, path, &buf);
   if(res) {
      vPortFree(res);
      return 0;
   }
   if(strlen(buf) != ((2*16)+4) /* 16 hex bytes and 4 hyphens */
         || sscanf(buf,
            "%2hhx%2hhx%2hhx%2hhx-"
            "%2hhx%2hhx-"
            "%2hhx%2hhx-"
            "%2hhx%2hhx-"
            "%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx",
            uuid, uuid + 1, uuid + 2, uuid + 3,
            uuid + 4, uuid + 5, uuid + 6, uuid + 7,
            uuid + 8, uuid + 9, uuid + 10, uuid + 11,
            uuid + 12, uuid + 13, uuid + 14, uuid + 15) != 16) {
      printk("Xenbus path %s value %s is not a uuid!\n", path, buf);
      vPortFree(buf);
      return 0;
   }
   vPortFree(buf);
   return 1;
}

char* xenbus_printf(xenbus_transaction_t xbt,
                                  const char* node, const char* path,
                                  const char* fmt, ...)
{
#define BUFFER_SIZE 256
    char fullpath[BUFFER_SIZE];
    char val[BUFFER_SIZE];
    va_list args;
  
    BUG_ON(strlen(node) + strlen(path) + 1 >= BUFFER_SIZE);
    sprintf(fullpath,"%s/%s", node, path);
    va_start(args, fmt);
    vsnprintf(val, BUFFER_SIZE, fmt, args);
    va_end(args);
    return xenbus_write(xbt,fullpath,val);
}

domid_t xenbus_get_self_id(void)
{
    char *dom_id, *msg;
    domid_t ret;

    msg = xenbus_read(XBT_NIL, "domid", &dom_id);
    if (msg) {
	    printk("BUG: xenbus_read of domid failed with '%s'\n", msg);
	    BUG();
    }

    sscanf(dom_id, "%u", (unsigned int *) &ret);

    return ret;
}

static void do_ls_test(const char *pre)
{
    char **dirs, *msg;
    int x;

    printk("ls %s...\n", pre);
    msg = xenbus_ls(XBT_NIL, pre, &dirs);
    if (msg) {
	printk("Error in xenbus ls: %s\n", msg);
	vPortFree(msg);
	return;
    }
    for (x = 0; dirs[x]; x++) 
    {
        printk("ls %s[%d] -> %s\n", pre, x, dirs[x]);
        vPortFree(dirs[x]);
    }
    vPortFree(dirs);
}

static void do_read_test(const char *path)
{
    char *res, *msg;
    printk("Read %s...\n", path);
    msg = xenbus_read(XBT_NIL, path, &res);
    if (msg) {
	printk("Error in xenbus read: %s\n", msg);
	vPortFree(msg);
	return;
    }
    printk("Read %s -> %s.\n", path, res);
    vPortFree(res);
}

static void do_write_test(const char *path, const char *val)
{
    char *msg;
    printk("Write %s to %s...\n", val, path);
    msg = xenbus_write(XBT_NIL, path, val);
    if (msg) {
	printk("Result %s\n", msg);
	vPortFree(msg);
    } else {
	printk("Success.\n");
    }
}

static void do_rm_test(const char *path)
{
    char *msg;
    printk("rm %s...\n", path);
    msg = xenbus_rm(XBT_NIL, path);
    if (msg) {
	printk("Result %s\n", msg);
	vPortFree(msg);
    } else {
	printk("Success.\n");
    }
}

/* Simple testing thing */
void test_xenbus(void *ignored)
{
    printk("Doing xenbus test.\n");
    xenbus_debug_msg("Testing xenbus...\n");

    printk("Doing write test.\n");
    do_write_test("data/stuff", "flobble");
    do_read_test("data/stuff");

    printk("Doing ls test.\n");
    do_ls_test("data");
    do_ls_test("data/nonexistent");

    printk("Doing rm test.\n");
    do_rm_test("data/stuff");
    do_read_test("data/stuff");
    printk("(Should have said ENOENT)\n");

    vTaskDelete(NULL);
}

/*
 * Local variables:
 * mode: C
 * c-basic-offset: 4
 * End:
 */
