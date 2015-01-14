
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
