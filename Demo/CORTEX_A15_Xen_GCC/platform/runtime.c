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
#include <runtime_reqs.h>
#include <platform/console.h>
#include <platform/hypercalls.h>
#include <freertos/os.h>
#include <FreeRTOS.h>
#include <task.h>

void *pvPortMalloc(size_t);

void runtime_write(size_t len, char *buffer)
{
	(void)HYPERVISOR_console_io(CONSOLEIO_WRITE, len, buffer);
}

void runtime_exit(void)
{
	printk("<<< FreeRTOS exit >>>\n");
	vTaskSuspendAll();
	taskDISABLE_INTERRUPTS();
	BUG();
}

inline void *runtime_libc_malloc(size_t len)
{
	return pvPortMalloc(len);
}
