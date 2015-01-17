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


#include <string.h>

#include <platform/libIVC.h>
#include <platform/ivcdev.h>
#include <platform/console.h>

#define MAX_NAME_LEN         64

struct ivcdev *ivc_makeConnection(const char *base_name, xList *waitq)
{
	char ch0_name[MAX_NAME_LEN];
	char ch1_name[MAX_NAME_LEN];
	struct ivcdev *dev = NULL;
       
	dev = pvPortMalloc(sizeof(struct ivcdev));
	if (!dev) {
		printk("ivcdev: could not allocate memory for device\n");
		return NULL;
	}

	if (!waitq) {
		waitq = pvPortMalloc(sizeof(xList));
		if (!waitq) {
			vPortFree(dev);
			printk("ivcdev: could not allocate memory for wait queue\n");
			return NULL;
		}

		vListInitialise(waitq);
	}

	dev->waitq = waitq;

	snprintf(ch0_name, MAX_NAME_LEN, "%s_0", base_name);
	snprintf(ch1_name, MAX_NAME_LEN, "%s_1", base_name);

	dev->wconn = makeConnection(ch0_name, ivcOutputChannel, waitq);
	if (!dev->wconn) {
		printk("ivcdev: could not accept write connection '%s'\n",
				ch1_name);
		closeConnection(dev->rconn);
		goto err;
	}

	dev->rconn = makeConnection(ch1_name, ivcInputChannel, waitq);
	if (!dev->rconn) {
		printk("ivcdev: could not make read connection '%s'\n",
				ch0_name);
		goto err;
	}

	return dev;
err:
	vPortFree(dev);
	return NULL;
}

struct ivcdev *ivc_acceptConnection(const char *base_name, int num_pages, xList *waitq)
{
	char ch0_name[MAX_NAME_LEN];
	char ch1_name[MAX_NAME_LEN];
	struct ivcdev *dev = NULL;
       
	dev = pvPortMalloc(sizeof(struct ivcdev));
	if (!dev) {
		printk("ivcdev: could not allocate memory for device\n");
		return NULL;
	}

	if (!waitq) {
		waitq = pvPortMalloc(sizeof(xList));
		if (!waitq) {
			vPortFree(dev);
			printk("ivcdev: could not allocate memory for wait queue\n");
			return NULL;
		}

		vListInitialise(waitq);
	}

	dev->waitq = waitq;

	snprintf(ch0_name, MAX_NAME_LEN, "%s_0", base_name);
	snprintf(ch1_name, MAX_NAME_LEN, "%s_1", base_name);

	dev->rconn = acceptConnection(ch0_name, ivcInputChannel, num_pages, waitq);
	if (!dev->rconn) {
		printk("ivcdev: could not accept read connection '%s'\n",
				ch0_name);
		goto err;
	}

	dev->wconn = acceptConnection(ch1_name, ivcOutputChannel, num_pages, waitq);
	if (!dev->wconn) {
		printk("ivcdev: could not make write connection '%s'\n",
				ch1_name);
		closeConnection(dev->rconn);
		goto err;
	}

	return dev;
err:
	vPortFree(dev);
	return NULL;
}

void ivc_putData(struct ivcdev *con, void *buffer, size_t size)
{
	return putData(con->wconn, buffer, size);
}

struct readbuf *ivc_getData(struct ivcdev *con)
{
	return getData(con->rconn);
}

int ivc_canRead(struct ivcdev *con)
{
	return canRead(con->rconn);
}

int ivc_canWrite(struct ivcdev *con)
{
	return canWrite(con->wconn);
}

void ivc_wait_until_ready(struct ivcdev *con)
{
	wait_event(con->waitq, ivc_canRead(con) || ivc_canWrite(con));
}
