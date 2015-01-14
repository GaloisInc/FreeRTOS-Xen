// BANNERSTART
// - Copyright 2006-2008, Galois, Inc.
// - This software is distributed under a standard, three-clause BSD license.
// - Please see the file LICENSE, distributed with this software, for specific
// - terms and conditions.
// Author: Adam Wick <awick@galois.com>
// BANNEREND
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
