
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

#include <FreeRTOS.h>
#include <freertos/asm/timer.h>
#include <port/gic.h>
#include <port/interrupts.h>
#include <platform/console.h>

/*
 * The application must provide a function that configures a peripheral to
 * create the FreeRTOS tick interrupt, then define configSETUP_TICK_INTERRUPT()
 * in FreeRTOSConfig.h to call the function.
 */
void handle_timer_interrupt(uint32_t unused)
{
		schedule_timer();
		FreeRTOS_Tick_Handler();
}

void vConfigureTickInterrupt( void )
{
	setup_timer();
    gic_register_handler(VIRTUAL_TIMER_IRQ, handle_timer_interrupt);

	gic_enable_interrupt(VIRTUAL_TIMER_IRQ /* interrupt number */,
			0x1 /*cpu_set*/, 1 /*level_sensitive*/, 1 /* ppi */);
	gic_set_priority(VIRTUAL_TIMER_IRQ, configTICK_PRIORITY << portPRIORITY_SHIFT);
	printk("Timer event IRQ enabled at priority %d\n", configTICK_PRIORITY);
}
