
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

#include <stdlib.h>
#include <stdint.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "configcheck.h"
#include <portmacro.h>
#include <port/gic.h>

#include <platform/console.h>

/*
 * Starts the first task executing.  This function is necessarily written in
 * assembly code so is implemented in portASM.s.
 */
extern void vPortRestoreTaskContext( void );

/*-----------------------------------------------------------*/

/* A variable is used to keep track of the critical section nesting.  This
variable has to be stored as part of the task context and must be initialised to
a non zero value to ensure interrupts don't inadvertently become unmasked before
the scheduler starts.  As it is stored as part of the task context it will
automatically be set to 0 when the first task is started. */
volatile unsigned long ulCriticalNesting = 9999UL;

/* Saved as part of the task context.  If ulPortTaskHasFPUContext is non-zero then
a floating point context must be saved and restored for the task. */
#if configALL_TASKS_USE_FPU
unsigned long ulPortTaskHasFPUContext = pdTRUE;
#else
unsigned long ulPortTaskHasFPUContext = pdFALSE;
#endif

/* Set to 1 to pend a context switch from an ISR. */
unsigned long ulPortYieldRequired = pdFALSE;

/* Counts the interrupt nesting depth.  A context switch is only performed if
if the nesting depth is 0. */
unsigned long ulPortInterruptNesting = 0UL;

/*-----------------------------------------------------------*/

/*
 * See header file for description.
 */
portSTACK_TYPE *pxPortInitialiseStack( portSTACK_TYPE *pxTopOfStack, pdTASK_CODE pxCode, void *pvParameters )
{
	/* Setup the initial stack of the task.  The stack is set exactly as
	expected by the portRESTORE_CONTEXT() macro.

	The fist real value on the stack is the status register, which is set for
	system mode, with interrupts enabled.  A few NULLs are added first to ensure
	GDB does not try decoding a non-existent return address. */
	*pxTopOfStack = NULL;
	pxTopOfStack--;
	*pxTopOfStack = NULL;
	pxTopOfStack--;
	*pxTopOfStack = NULL;
	pxTopOfStack--;
	*pxTopOfStack = ( portSTACK_TYPE ) portINITIAL_SPSR;

	if( ( ( unsigned long ) pxCode & portTHUMB_MODE_ADDRESS ) != 0x00UL )
	{
		/* The task will start in THUMB mode. */
		*pxTopOfStack |= portTHUMB_MODE_BIT;
	}

	pxTopOfStack--;

	/* Next the return address, which in this case is the start of the task. */
	*pxTopOfStack = ( portSTACK_TYPE ) pxCode;
	pxTopOfStack--;

	/* Next all the registers other than the stack pointer. */
	*pxTopOfStack = ( portSTACK_TYPE ) 0x00000000;	/* R14 */
	pxTopOfStack--;
	*pxTopOfStack = ( portSTACK_TYPE ) 0x12121212;	/* R12 */
	pxTopOfStack--;
	*pxTopOfStack = ( portSTACK_TYPE ) 0x11111111;	/* R11 */
	pxTopOfStack--;
	*pxTopOfStack = ( portSTACK_TYPE ) 0x10101010;	/* R10 */
	pxTopOfStack--;
	*pxTopOfStack = ( portSTACK_TYPE ) 0x09090909;	/* R9 */
	pxTopOfStack--;
	*pxTopOfStack = ( portSTACK_TYPE ) 0x08080808;	/* R8 */
	pxTopOfStack--;
	*pxTopOfStack = ( portSTACK_TYPE ) 0x07070707;	/* R7 */
	pxTopOfStack--;
	*pxTopOfStack = ( portSTACK_TYPE ) 0x06060606;	/* R6 */
	pxTopOfStack--;
	*pxTopOfStack = ( portSTACK_TYPE ) 0x05050505;	/* R5 */
	pxTopOfStack--;
	*pxTopOfStack = ( portSTACK_TYPE ) 0x04040404;	/* R4 */
	pxTopOfStack--;
	*pxTopOfStack = ( portSTACK_TYPE ) 0x03030303;	/* R3 */
	pxTopOfStack--;
	*pxTopOfStack = ( portSTACK_TYPE ) 0x02020202;	/* R2 */
	pxTopOfStack--;
	*pxTopOfStack = ( portSTACK_TYPE ) 0x01010101;	/* R1 */
	pxTopOfStack--;
	*pxTopOfStack = ( portSTACK_TYPE ) pvParameters; /* R0 */
	pxTopOfStack--;

	/* The task will start with a critical nesting count of 0 as interrupts are
	enabled. */
	*pxTopOfStack = portNO_CRITICAL_NESTING;
	pxTopOfStack--;

#if configALL_TASKS_USE_FPU
    int i;
    int steps = (configFPU_NUM_REGISTERS * configFPU_BYTES_PER_REGISTER)/sizeof(portSTACK_TYPE);

	/* The task will start with a floating point context. */

    // For more info, see
    // http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dht0002a/ch01s03s02.html
    //
    // Load initial values for D0-D31 onto the stack; there are 32 64-bit
    // floating point registers to save (so 8*32 bytes).  See also
    // asm/irq.s:portRESTORE_CONTEXT for an equivalent implementation.
    for (i = 0; i < steps; i++) {
        *pxTopOfStack = 0x0;
        pxTopOfStack--;
    }

    // FPSCR initial value
	*pxTopOfStack = 0x0;
    pxTopOfStack--;

    // Push ulPortTaskHasFPUContext = pdTRUE
    *pxTopOfStack = pdTRUE;
#else
    // Push ulPortTaskHasFPUContext = pdFALSE
    *pxTopOfStack = portNO_FLOATING_POINT_CONTEXT;
#endif

	return pxTopOfStack;
}
/*-----------------------------------------------------------*/

portBASE_TYPE xPortStartScheduler( void )
{
	unsigned long ulAPSR;

	dprintk("xPortStartScheduler\n");

	/* Only continue if the CPU is not in User mode.  The CPU must be in a
	Privileged mode for the scheduler to start. */
	__asm volatile ( "MRS %0, APSR" : "=r" ( ulAPSR ) );
	ulAPSR &= portAPSR_MODE_BITS_MASK;
	configASSERT( ulAPSR != portAPSR_USER_MODE );

	if( ulAPSR != portAPSR_USER_MODE )
	{
		/* Only continue if the binary point value is set to its lowest possible
		setting.  See the comments in vPortValidateInterruptPriority() below for
		more information. */
		dprintk("Checking binary point\n");
		configASSERT( ( portICCBPR_BINARY_POINT_REGISTER_VALUE & portBINARY_POINT_BITS ) <= portMAX_BINARY_POINT_VALUE );

		if( ( portICCBPR_BINARY_POINT_REGISTER_VALUE & portBINARY_POINT_BITS ) <= portMAX_BINARY_POINT_VALUE )
		{
			dprintk("Setting up tick interrupt\n");
			ASSERT(global_interrupts_disabled());
			/* Start the timer that generates the tick ISR. */
			configSETUP_TICK_INTERRUPT();

			dprintk("Enabling interrupts\n");
			__enable_irq();
			dprintk("Restoring task context for first task\n");
			vPortRestoreTaskContext();
		}
	}

	/* Will only get here if xTaskStartScheduler() was called with the CPU in
	a non-privileged mode or the binary point register was not set to its lowest
	possible value. */
	return 0;
}
/*-----------------------------------------------------------*/

void vPortEndScheduler( void )
{
	/* It is unlikely that the ARM port will require this function as there
	is nothing to return to. */
}
/*-----------------------------------------------------------*/

void vPortEnterCritical( void )
{
	/* Disable interrupts as per portDISABLE_INTERRUPTS(); 	*/
	ulPortSetInterruptMask();

	/* Now interrupts are disabled ulCriticalNesting can be accessed
	directly.  Increment ulCriticalNesting to keep a count of how many times
	portENTER_CRITICAL() has been called. */
	ulCriticalNesting++;
}
/*-----------------------------------------------------------*/

void vPortExitCritical( void )
{
	if( ulCriticalNesting > portNO_CRITICAL_NESTING )
	{
		/* Decrement the nesting count as the critical section is being
		exited. */
		ulCriticalNesting--;

		/* If the nesting level has reached zero then all interrupt
		priorities must be re-enabled. */
		if( ulCriticalNesting == portNO_CRITICAL_NESTING )
		{
			/* Critical nesting has reached zero so all interrupt priorities
			should be unmasked. */
			portCLEAR_INTERRUPT_MASK();
		}
	}
}
/*-----------------------------------------------------------*/

void FreeRTOS_Tick_Handler( void )
{
	/* Set interrupt mask before altering scheduler structures.   The tick
	handler runs at the lowest priority, so interrupts cannot already be masked,
	so there is no need to save and restore the current mask value. */

	portICCPMR_PRIORITY_MASK_REGISTER_VALUE = ( configMAX_API_CALL_INTERRUPT_PRIORITY << portPRIORITY_SHIFT );
	__asm(	"DSB		\n"
	     	"ISB		\n" );

	/* Increment the RTOS tick. */
	if( xTaskIncrementTick() != pdFALSE )
	{
		ulPortYieldRequired = pdTRUE;
	}

	/* Ensure all interrupt priorities are active again. */
	portCLEAR_INTERRUPT_MASK();
}
/*-----------------------------------------------------------*/

#if configALL_TASKS_USE_FPU
void vPortTaskUsesFPU( void )
{
    // This just returns because all tasks use the FPU by default.
    return;
}
#else
void vPortTaskUsesFPU( void )
{
	unsigned long ulInitialFPSCR = 0;

	/* A task is registering the fact that it needs an FPU context.  Set the
	FPU flag (which is saved as part of the task context). */
	ulPortTaskHasFPUContext = pdTRUE;

	/* Initialise the floating point status register. */
	__asm( "FMXR 	FPSCR, %0" :: "r" (ulInitialFPSCR) );
}
#endif
/*-----------------------------------------------------------*/

void vPortClearInterruptMask( unsigned long ulNewMaskValue )
{
	if( ulNewMaskValue == pdFALSE )
	{
		portCLEAR_INTERRUPT_MASK();
	}
}
/*-----------------------------------------------------------*/

unsigned long ulPortSetInterruptMask( void )
{
	unsigned long ulReturn;
	uint32_t mask = configMAX_API_CALL_INTERRUPT_PRIORITY << portPRIORITY_SHIFT;

	uint32_t inten = !global_interrupts_disabled();

	if (inten)
		__disable_irq();

	if (portICCPMR_PRIORITY_MASK_REGISTER_VALUE == mask)
	{
		/* Interrupts were already masked. */
		ulReturn = pdTRUE;
	}
	else
	{
		ulReturn = pdFALSE;
		portICCPMR_PRIORITY_MASK_REGISTER_VALUE = mask;
		__asm(	"DSB		\n"
			"ISB		\n" );
	}

	if (inten)
		__enable_irq();

	return ulReturn;
}
/*-----------------------------------------------------------*/

#if( configASSERT_DEFINED == 1 )

	void vPortValidateInterruptPriority( void )
	{
		/* The following assertion will fail if a service routine (ISR) for
		an interrupt that has been assigned a priority above
		configMAX_SYSCALL_INTERRUPT_PRIORITY calls an ISR safe FreeRTOS API
		function.  ISR safe FreeRTOS API functions must *only* be called
		from interrupts that have been assigned a priority at or below
		configMAX_SYSCALL_INTERRUPT_PRIORITY.

		Numerically low interrupt priority numbers represent logically high
		interrupt priorities, therefore the priority of the interrupt must
		be set to a value equal to or numerically *higher* than
		configMAX_SYSCALL_INTERRUPT_PRIORITY.

		FreeRTOS maintains separate thread and ISR API functions to ensure
		interrupt entry is as fast and simple as possible.

		The following links provide detailed information:
		http://www.freertos.org/RTOS-Cortex-M3-M4.html
		http://www.freertos.org/FAQHelp.html */
		configASSERT( portICCRPR_RUNNING_PRIORITY_REGISTER_VALUE >= ( configMAX_API_CALL_INTERRUPT_PRIORITY << portPRIORITY_SHIFT ) );

		/* Priority grouping:  The interrupt controller (GIC) allows the bits
		that define each interrupt's priority to be split between bits that
		define the interrupt's pre-emption priority bits and bits that define
		the interrupt's sub-priority.  For simplicity all bits must be defined
		to be pre-emption priority bits.  The following assertion will fail if
		this is not the case (if some bits represent a sub-priority).

		The priority grouping is configured by the GIC's binary point register
		(ICCBPR).  Writting 0 to ICCBPR will ensure it is set to its lowest
		possible value (which may be above 0). */
		configASSERT( ( portICCBPR_BINARY_POINT_REGISTER_VALUE & portBINARY_POINT_BITS ) <= portMAX_BINARY_POINT_VALUE );
	}

#endif /* configASSERT_DEFINED */
