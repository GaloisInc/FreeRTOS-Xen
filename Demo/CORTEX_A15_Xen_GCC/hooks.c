
#include <FreeRTOS.h>
#include <task.h>
#include <platform/console.h>

void vApplicationMallocFailedHook( void )
{
	/* Called if a call to pvPortMalloc() fails because there is insufficient
	free memory available in the FreeRTOS heap.  pvPortMalloc() is called
	internally by FreeRTOS API functions that create tasks, queues, software
	timers, and semaphores.  The size of the FreeRTOS heap is set by the
	configTOTAL_HEAP_SIZE configuration constant in FreeRTOSConfig.h. */
	printk("Malloc failed\n");
	__disable_irq();
	for( ;; );
}

void vApplicationStackOverflowHook( xTaskHandle pxTask, signed char *pcTaskName )
{
	( void ) pcTaskName;
	( void ) pxTask;

	/* Run time stack overflow checking is performed if
	configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
	function is called if a stack overflow is detected. */

	printk("Stack overflow in task (handle 0x%x, name '%s')\n", pxTask, pcTaskName);
	__disable_irq();
	for( ;; );
}

void vApplicationIdleHook( void )
{
}

void vApplicationTickHook( void )
{
}

