/*
 * Copyright (C) 2006-2008, 2014-2015 Galois, Inc.
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

#ifndef HALVM_LIBIVC_H
#define HALVM_LIBIVC_H

#include <stdint.h>
#include "xen/event_channel.h"
#include "wait.h"

typedef struct ivc_connection   ivc_connection_t;
typedef enum   ivc_contype      ivc_contype_t;

enum ivc_contype
{
  ivcInputChannel,
  ivcOutputChannel,
  ivcInputOutputChannel
};

struct readbuf {
  void *buf;
  uint32_t size;
};

struct bufstate
{
  volatile uint32_t consumed;
  volatile uint32_t produced;
};

struct ivc_connection
{
  xList *waitq;
  uint32_t peer;
  ivc_contype_t type;
  evtchn_port_t port;
  uint32_t insize, outsize;
  uint32_t inmod, outmod;
  void *inbuf, *outbuf;
  struct bufstate *input;
  struct bufstate *output;
  void *buffer_base;
};

ivc_connection_t *makeConnection(char *, ivc_contype_t, xList *);
ivc_connection_t *acceptConnection(char *, ivc_contype_t,
                                   uint32_t, xList *);
void closeConnection(ivc_connection_t*);

struct readbuf *getData(ivc_connection_t*);
void putData(ivc_connection_t*, void *, size_t);
uint32_t connectionPeer(ivc_connection_t*);

int canRead(ivc_connection_t *con);
int canWrite(ivc_connection_t *con);

#endif
