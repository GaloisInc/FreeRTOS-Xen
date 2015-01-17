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
