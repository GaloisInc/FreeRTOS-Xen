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

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <assert.h>

#include <xenstore_lib.h>

#include <platform/libIVC.h>
#include <freertos/arch_mm.h>
#include <freertos/mmu.h>
#include <platform/xenbus.h>
#include <platform/events.h>
#include <platform/console.h>
#include <platform/gnttab.h>
#include <platform/hypervisor.h>

#include <FreeRTOS.h>
#include <task.h>

#define PROT_READWRITE (PROT_READ | PROT_WRITE)
#define malloc pvPortMalloc
#define free vPortFree

/*****************************************************************************/
/** Channel Setup ************************************************************/
/*****************************************************************************/

static uint32_t *parseRefs(char *str, uint32_t *len);
#ifdef HAVE_XENSTORE_H
static char     *showGrantRefList(uint32_t *grants, uint32_t num);
#endif
static ivc_connection_t *buildChannel(uint32_t, ivc_contype_t,
                                      evtchn_port_t, void *, void *, uint32_t,
                                      int, xList *);
static uint32_t computeModulus(uint32_t);

void *malloc_pages(int num, void **real_base)
{
	void *ptr = malloc((num + 1) * PAGE_SIZE);

	if (!ptr)
		return NULL;

	void *base = (void *) (((uint32_t) ptr) & (~(PAGE_SIZE - 1)));
	*real_base = ptr;

	dprintk("malloc_pages: ptr = 0x%x, base = 0x%x, base + PAGE_SIZE = 0x%x\n",
			ptr, base, base + PAGE_SIZE);

	if (base == ptr) {
		return ptr;
	} else {
		return (base + PAGE_SIZE);
	}
}

static char *showGrantRefList(uint32_t *grants, uint32_t num)
{
  int itemLen = 32;
  char *eachref = malloc(num * itemLen);
  uint32_t i, idx, total_len;
  char *retval, *ptr;

  for (i = 0, total_len = 0; i < num; i++) {
    ptr = eachref + (itemLen * i);
    int grant_int = (int) grants[i];
    snprintf(ptr, itemLen, "grant:%d", grant_int);
    total_len += strlen(ptr);
  }

  total_len += 2 /* braces */ + (num - 1) /* commas */ + 1 /* null */;
  retval = malloc(total_len * sizeof(char));

  retval[0]           = '[';
  retval[total_len-2] = ']';
  retval[total_len-1] = 0;

  for(i = 0, idx = 1; i < num; i++) {
    ptr = eachref + (itemLen * i);
    size_t size = strlen(ptr);
    memcpy(&(retval[idx]), ptr, size);
    idx += size;
    if(i != (num - 1)) retval[idx++] = ',';
  }

  free(eachref);

  return retval;
}

int xenbus_wait_for_key(const char *key, char **dest)
{
  int found;
  char *err = NULL;
  xenbus_event_queue xenbus_events = NULL;

  err = xenbus_watch_path_token(0, key, key, &xenbus_events);
  if (err) {
	  printk("%s: error watching for '%s': %s\n", __func__, key, err);
	  vPortFree(err);
	  return 1;
  }

  found = 0;
  while (!found) {
	  xenbus_wait_for_watch(&xenbus_events);
	  err = xenbus_read(0, key, dest);
	  if (err) {
		  dprintk("%s: error reading '%s': %s\n", __func__, key, err);
		  vPortFree(err);
	  } else {
		  found = 1;
	  }
  }

  xenbus_unwatch_path_token(0, key, key);
  return 0;
}

ivc_connection_t *makeConnection(char *name,
                                 ivc_contype_t type,
				 xList *waitq)
{
  char *rdomstr = NULL, *rrefstr = NULL, *rpstr = NULL, *err = NULL;
  char key[1024], val[1024];
  uint32_t other, num_refs, *grants;
  domid_t me;
  ivc_connection_t *res;
  evtchn_port_t remote_port, local_port;
  void *buffer;
  char perms;
  unsigned long start_mfn = 0;
  int i;

  dprintk("makeConnection: called\n");

  me = xenbus_get_self_id();

  dprintk("makeConnection: me = %d\n", me);

  sprintf(key, "/rendezvous/%s", name);
  xenbus_rm(0, key);
  xenbus_mkdir(0, key);
  perms = XS_PERM_READ | XS_PERM_WRITE;
  xenbus_set_perms(0, key, me, perms);

  dprintk("makeConnection: writing LeftDomId\n");

  sprintf(key, "/rendezvous/%s/LeftDomId", name);
  sprintf(val, "dom%d", me);
  xenbus_write(0, key, val);

  sprintf(key, "/rendezvous/%s/RightDomId", name);
  if (xenbus_wait_for_key(key, &rdomstr)) {
	  printk("Error waiting for key '%s'\n", key);
	  return NULL;
  }

  int other_int = 0;
  sscanf(rdomstr, "dom%d", &other_int);
  other = (uint32_t) other_int;
  vPortFree(rdomstr);

  dprintk("makeConnection: waiting for grant reference(s)\n");

  sprintf(key, "/rendezvous/%s/RightGrantRefs", name);
  if (xenbus_wait_for_key(key, &rrefstr)) {
	  printk("Error waiting for key '%s'\n", key);
	  return NULL;
  }

  grants = parseRefs(rrefstr, &num_refs);
  vPortFree(rrefstr);

  // For each grant reference from the other domain, map the grant reference.
  // For this we just map these 1:1 since we get to pick the MFNs.
  buffer = map_frames_direct(num_refs, &start_mfn);
  assert(buffer);

  dprintk("makeConnection: start_mfn = 0x%x\n", start_mfn);

  // Now for each grant, request a mapping at the MFNs starting at start_mfn.
  for (i = 0; i < num_refs; i++) {
	if (gnttab_map_ref(other, grants[i], start_mfn + i) != 0) {
		dprintk("makeConnection: could not map grant ref %d (i = %d) (domain %d) to MFN %d\n",
				grants[i], i, other, start_mfn + i);
		return NULL;
	} else {
		dprintk("makeConnection: mapped grant ref %d\n", grants[i]);
	}
  }

  free(grants);

  sprintf(key, "/rendezvous/%s/RightPorts", name);
  if (xenbus_wait_for_key(key, &rpstr)) {
	  printk("Error waiting for key '%s'\n", key);
	  return NULL;
  }

  int remote_port_int = 0;
  sscanf(rpstr, "[echan:%d]", &remote_port_int);
  remote_port = (evtchn_port_t) remote_port_int;
  vPortFree(rpstr);

  if (evtchn_bind_interdomain(other, remote_port, &local_port) != 0) {
	  dprintk("makeConnection: could not bind to remote port!\n");
	  return NULL;
  }

  dprintk("makeConnection: port = %d\n", local_port);

  res = buildChannel(other, type, local_port, buffer, buffer, num_refs * 4096, 0, waitq);

  dprintk("makeConnection: confirming connection\n");

  sprintf(key, "/rendezvous/%s/LeftConnectionConfirmed", name);
  sprintf(val, "True");
  err = xenbus_write(0, key, val);
  if (err) {
	  dprintk("makeConnection: failed to write confirmation: %s\n", err);
	  return NULL;
  }

  dprintk("makeConnection: done\n");

  return res;
}

ivc_connection_t *acceptConnection(char *name,
                                   ivc_contype_t type,
                                   uint32_t num_pages,
				   xList *waitq)
{
  char *greflist = NULL, *ival = NULL, *ldomstr = NULL;
  char key[1024], val[1024];
  uint32_t other, *grefs, local_port;
  domid_t me;
  void *buffer, *buffer_base;
  int i;
  uint32_t pfn;

  dprintk("acceptConnection: called, waiting for LeftDomId\n");

  other = 0;
  sprintf(key, "/rendezvous/%s/LeftDomId", name);
  if (xenbus_wait_for_key(key, &ldomstr)) {
	  printk("Error waiting for key '%s'\n", key);
	  return NULL;
  }

  int other_int = 0;
  sscanf(ldomstr, "dom%d", &other_int);
  other = (uint32_t) other_int;
  vPortFree(ldomstr);

  dprintk("acceptConnection: other = %lu\n", other);

  me = xenbus_get_self_id();

  dprintk("acceptConnection: self id = %d\n", me);

  buffer = malloc_pages(num_pages, &buffer_base);
  assert(buffer);

  dprintk("acceptConnection: buffer = 0x%x, buffer_base = 0x%x\n", buffer, buffer_base);

  grefs = malloc(num_pages * sizeof(uint32_t));
  assert(grefs);

  dprintk("acceptConnection: grefs = 0x%x\n", grefs);

  dprintk("acceptConnection: allocating %d pages\n", num_pages);

  // For each page in the buffer (assuming the buffer is 4k-aligned, which it
  // should be if the comments in heap_2 are any indication), add a grant table
  // entry for that page's PFN.  We don't have to do a PA->VA translation here
  // because the memory containing the heap is mapped 1:1 VA:PA.
  for (i = 0; i < num_pages; i++) {
	  pfn = (((uint32_t) buffer) + (i * PAGE_SIZE)) >> PAGE_SHIFT;
	  // XXX this should be a PA, not a VA; we'll need to look at the page
	  // table to find out where the buffer was allocated.  Since we are
	  // using the heap here, we happen to get pages in a 1:1 mapped
	  // address range, so we are getting _lucky_ because the VA page and
	  // PA page numbers are the same.  This isn't going to work in the
	  // long run.
	  grefs[i] = gnttab_grant_access(other, pfn, 0);
	  dprintk("acceptConnection: grant ref %d allocated for page %llu\n", grefs[i], pfn);
  }

  dprintk("acceptConnection: allocating unbound evtchn port\n");

  if (evtchn_alloc_unbound(other, &local_port) != 0) {
	  dprintk("acceptConnection: evtchn_alloc_unbound failed\n");
	  free(grefs);
	  free(buffer_base);
	  return NULL;
  }
  assert(local_port);

  dprintk("acceptConnection: port = %d\n", local_port);

  dprintk("acceptConnection: writing parameters to xenstore\n");

  sprintf(key, "/rendezvous/%s/RightDomId", name);
  sprintf(val, "dom%d", me);
  xenbus_write(0, key, val);

  sprintf(key, "/rendezvous/%s/RightGrantRefs", name);
  greflist = showGrantRefList(grefs, num_pages);
  xenbus_write(0, key, greflist);
  free(greflist);
  free(grefs);

  sprintf(key, "/rendezvous/%s/RightPorts", name);
  int local_port_int = (int) local_port;
  sprintf(val, "[echan:%d]", local_port_int);
  xenbus_write(0, key, val);

  dprintk("acceptConnection: waiting for connection confirmation\n");

  sprintf(key, "/rendezvous/%s/LeftConnectionConfirmed", name);
  if (xenbus_wait_for_key(key, &ival)) {
	  printk("Error waiting for key '%s'\n", key);
	  return NULL;
  }

  dprintk("acceptConnection: got connection confirmation\n");
  free(ival);

  sprintf(key, "/rendezvous/%s", name);
  xenbus_rm(0, key);

  dprintk("acceptConnection: building channel\n");

  return buildChannel(other, type, local_port, buffer, buffer_base, 4096 * num_pages, 1, waitq);
}

void closeConnection(ivc_connection_t *con)
{
  void *ptr = con->inbuf ? con->inbuf : con->outbuf;
  uint32_t size;

  switch(con->type) {
    case ivcInputChannel:
      size = con->insize + sizeof(struct bufstate);
      break;
    case ivcOutputChannel:
      size = con->outsize + sizeof(struct bufstate);
      break;
    case ivcInputOutputChannel:
      size = con->insize + con->outsize + (2 * sizeof(struct bufstate));
      break;
    default:
      assert(0);
  }

  // XXX TOOD: grant table unmap: xc_gnttab_munmap(iface->gt, ptr, (size + 4095) / 4096);
  // And store grant reference mapping handles so we can use those here
  dprintk("closeConnection: ptr = 0x%x, size = %d\n", ptr, size);
  free(con->buffer_base);
  free(con);
}

static uint32_t *parseRefs(char *str, uint32_t *count)
{
  uint32_t *retval, index, i;

  for(i = 0, *count = 0; str[i]; i++)
    if(str[i] == ':')
      *count += 1;
  assert(count > 0);

  dprintk("parseRefs: count = %lu\n", *count);

  retval = malloc(*count * sizeof(uint32_t));
  assert(retval);

  for(i = 0, index = 0; i < *count; i++) {
    int res;

    while(str[index] != ':') index += 1;
    int retval_int = 0;
    res = sscanf(&(str[index]), ":%d", &retval_int);
    retval[i] = (uint32_t) retval_int;
    assert(res == 1);

    dprintk("parseRefs: scanned ref %d, value %d\n", i, retval[i]);

    index += 1;
  }

  dprintk("parseRefs: done\n");

  return retval;
}

void handle_evtchn(evtchn_port_t port, struct pt_regs *regs, void *ptr)
{
	ivc_connection_t *conn = (ivc_connection_t *) ptr;
	dprintk("handle_evtchn: waking up any waiters on this connection\n");
	unmask_evtchn(port);
	wake_up_from_isr(conn->waitq);
}

static ivc_connection_t *buildChannel(uint32_t peer,
                                      ivc_contype_t type,
                                      evtchn_port_t port,
                                      void *buffer,
                                      void *buffer_base,
                                      uint32_t buffer_size,
                                      int doClear,
				      xList *waitq)
{
  ivc_connection_t *res = malloc(sizeof(ivc_connection_t));

  if ((uint32_t)buffer & (PAGE_SIZE - 1)) {
	  dprintk("BUG: buildChannel got a buffer that was not page-aligned\n");
	  return NULL;
  }

  res->peer = peer;
  res->type = type;
  res->port = port;
  res->buffer_base = buffer_base;

  if (waitq)
	  res->waitq = waitq;
  else {
	  res->waitq = pvPortMalloc(sizeof(xList));
	  vListInitialise(res->waitq);
  }

  if(doClear) memset(buffer, 0, buffer_size);

  switch(type) {
    case ivcInputChannel:
      res->insize = buffer_size - sizeof(struct bufstate);
      res->outsize = 0;
      res->inmod = computeModulus(res->insize);
      res->outmod = 0;
      res->inbuf = buffer;
      res->outbuf = NULL;
      res->input  = (struct bufstate *)((uintptr_t)buffer + res->insize);
      res->output = NULL;
      break;
    case ivcOutputChannel:
      res->insize = 0;
      res->outsize = buffer_size - sizeof(struct bufstate);
      res->inmod = 0;
      res->outmod = computeModulus(res->outsize);
      res->inbuf = NULL;
      res->outbuf = buffer;
      res->input = NULL;
      res->output = (struct bufstate *)((uintptr_t)buffer + res->outsize);
      break;
    case ivcInputOutputChannel:
    {
      dprintk("BUG: ivcInputOutputChannel not supported!\n");
      return NULL;
    }
    default:
      assert(0);
  }

  // When the event channel fires, wake up any readers or writers.
  bind_evtchn(port, handle_evtchn, res);
  unmask_evtchn(port);

  dprintk("buildChannel: done\n");

  return res;
}

static uint32_t computeModulus(uint32_t size)
{
  uint64_t size64 = size;
  uint64_t q = 0x100000000ULL / size64;
  uint32_t base = (uint32_t)(q * size64);

  return (base == 0) ? (uint32_t)(q * (size64 - 1)) : base;
}

/*****************************************************************************/
/** Reading and writing from channels ****************************************/
/*****************************************************************************/

static uint32_t getAvail(uint32_t, struct bufstate *);
static void     readRaw(ivc_connection_t *, void *, uint32_t);
static void     writeRaw(ivc_connection_t *, void *, uint32_t);

int canRead(ivc_connection_t *con)
{
  return getAvail(con->inmod, con->input) >= sizeof(unsigned long);
}

int canWrite(ivc_connection_t *con)
{
  return ((con->outsize - getAvail(con->outmod, con->output)) >= sizeof(unsigned long));
}

struct readbuf *getData(ivc_connection_t *con)
{
  void *item_cur, *item_end;
  uint32_t avail, rd_amount;
  unsigned long item_size;
  struct readbuf *rb;

  assert( con->inbuf );

  /* wait until we have enough data to read the size */
  wait_event(con->waitq, getAvail(con->inmod, con->input) >= sizeof(unsigned long));

  rb = malloc(sizeof(struct readbuf));

  /* read the size, allocate the buffer, compute various pointers */
  readRaw(con, &item_size, sizeof(unsigned long));
  rb->size = item_size;

  assert(item_size <= con->insize - sizeof(unsigned long));

  rb->buf = item_cur = malloc(item_size);
  item_end = (void*)((uintptr_t)rb->buf + item_size);

  /* read in the data */
  while(item_cur < item_end) {
    avail = getAvail(con->inmod, con->input);
    rd_amount = (avail > item_size) ? item_size : avail;
    if(rd_amount > 0) {
      readRaw(con, item_cur, rd_amount);
      item_cur = (void*)((uintptr_t)item_cur + rd_amount);
      item_size -= rd_amount;
    }
  }
  assert(item_size == 0);

  return rb;
}

void putData(ivc_connection_t *con, void *buffer, size_t size)
{
  void *buf_cur = buffer, *buf_end = (void*)((uintptr_t)buffer + size);
  unsigned long item_size = size;
  uint32_t avail, wrt_amount;

  assert( con->outbuf );

  /* wait until we have enough data to write the size, then do so */
  wait_event(con->waitq,
		  con->outsize - getAvail(con->outmod, con->output) >=
		  sizeof(unsigned long));

  writeRaw(con, &item_size, sizeof(unsigned long));

  /* write out the data */
  while(buf_cur < buf_end) {
    avail = con->outsize - getAvail(con->outmod, con->output);
    wrt_amount = (avail > item_size) ? item_size : avail;
    if(wrt_amount > 0) {
      writeRaw(con, buf_cur, wrt_amount);
      buf_cur = (void*)((uintptr_t)buf_cur + wrt_amount);
      item_size -= wrt_amount;
    }
  }

  assert(item_size == 0);
}

uint32_t connectionPeer(ivc_connection_t *con)
{
  return con->peer;
}

static uint32_t getAvail(uint32_t mod, struct bufstate *state)
{
  uint32_t prod = state->produced;
  uint32_t cons = state->consumed;
  return (prod >= cons) ? (prod - cons) : (prod + (mod - cons));
}

static void readRaw(ivc_connection_t *con, void *buffer, uint32_t size)
{
  uint32_t off = con->input->consumed % con->insize;
  void *basep = (void*)((uintptr_t)con->inbuf + off);

  if(off + size > con->insize) {
    /* we need to split this up, as we're wrapping around */
    uint32_t part1sz = con->insize - off;
    memcpy(buffer, basep, part1sz);
    memcpy((void*)((uintptr_t)buffer + part1sz), con->inbuf, size - part1sz);
  } else {
    /* we can just write it in directly here */
    memcpy(buffer, basep, size);
  }
  con->input->consumed = (con->input->consumed + size) % con->inmod;
  __sync_synchronize();
  dprintk("readRaw: notifying evtchn port %d\n", con->port);
  notify_remote_via_evtchn(con->port);
}

static void writeRaw(ivc_connection_t *con, void *buffer, uint32_t size)
{
  uint32_t off = con->output->produced % con->outsize;
  void *basep = (void*)((uintptr_t)con->outbuf + off);

  if(off + size > con->outsize) {
    uint32_t part1sz = con->outsize - off;
    memcpy(basep, buffer, part1sz);
    memcpy(con->outbuf, (void*)((uintptr_t)buffer + part1sz), size - part1sz);
  } else {
    memcpy(basep, buffer, size);
  }
  con->output->produced = (con->output->produced + size) % con->outmod;
  __sync_synchronize();
  dprintk("writeRaw: notifying evtchn port %d\n", con->port);
  notify_remote_via_evtchn(con->port);
}

