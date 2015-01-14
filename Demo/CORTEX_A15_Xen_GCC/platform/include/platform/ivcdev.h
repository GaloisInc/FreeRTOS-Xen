
#ifndef _IVCDEV_H
#define _IVCDEV_H

#include "libIVC.h"

struct ivcdev {
	ivc_connection_t *rconn;
	ivc_connection_t *wconn;
	xList *waitq;
};

struct ivcdev *ivc_makeConnection(const char *base_name, xList *waitq);
struct ivcdev *ivc_acceptConnection(const char *base_name, int num_pages, xList *waitq);

void ivc_putData(struct ivcdev *con, void *buffer, size_t size);
struct readbuf *ivc_getData(struct ivcdev *con);
int ivc_canRead(struct ivcdev *con);
int ivc_canWrite(struct ivcdev *con);
void ivc_wait_until_ready(struct ivcdev *con);

#endif
