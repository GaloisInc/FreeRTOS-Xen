
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

#ifndef PORTMACRO_H
#define PORTMACRO_H

#include <FreeRTOSConfig.h>
#include <port/gic.h>

#ifdef __cplusplus
	extern "C" {
#endif

#define portSTACK_GROWTH			( -1 )
#define portTICK_RATE_MS			( ( portTickType ) 1000 / configTICK_RATE_HZ )
#define portBYTE_ALIGNMENT			8

#define portINTERRUPT_CONTROLLER_CPU_INTERFACE_ADDRESS 		( gic.gicc_base )
#define portICCIAR_INTERRUPT_ACKNOWLEDGE_REGISTER_ADDRESS 	( ( gic.gicc_base + GICC_IAR ) )
#define portICCEOIR_END_OF_INTERRUPT_REGISTER_ADDRESS 		( ( gic.gicc_base + GICC_EOIR ) )
#define portICCPMR_PRIORITY_MASK_REGISTER_ADDRESS 		( ( gic.gicc_base + GICC_PMR ) )
#define portICCBPR_BINARY_POINT_REGISTER_ADDRESS		( ( gic.gicc_base + GICC_BPR ) )
#define portICCRPR_RUNNING_PRIORITY_REGISTER_ADDRESS		( ( gic.gicc_base + GICC_RPR ) )

#define portICCPMR_PRIORITY_MASK_REGISTER_VALUE			( * (uint32_t *) portICCPMR_PRIORITY_MASK_REGISTER_ADDRESS )
#define portICCBPR_BINARY_POINT_REGISTER_VALUE 			( * (uint32_t *) portICCBPR_BINARY_POINT_REGISTER_ADDRESS )
#define portICCRPR_RUNNING_PRIORITY_REGISTER_VALUE 		( * (uint32_t *) portICCRPR_RUNNING_PRIORITY_REGISTER_ADDRESS )
#define portPRIORITY_SHIFT					ulPriorityShift
#define portLOWEST_PRIORITY					ulLowestPriority

/* A critical section is exited when the critical section nesting count reaches
this value. */
#define portNO_CRITICAL_NESTING			( ( unsigned long ) 0 )

/* In all GICs 255 can be written to the priority mask register to unmask all
(but the lowest) interrupt priority. */
#define portUNMASK_VALUE				( 0xFF )

/* Tasks are not created with a floating point context, but can be given a
floating point context after they have been created.  A variable is stored as
part of the tasks context that holds portNO_FLOATING_POINT_CONTEXT if the task
does not have an FPU context, or any other value if the task does have an FPU
context. */
#define portNO_FLOATING_POINT_CONTEXT	( ( portSTACK_TYPE ) 0 )

/* Constants required to setup the initial task context. */
#define portINITIAL_SPSR				( ( portSTACK_TYPE ) 0x1f ) /* System mode, ARM mode, interrupts enabled. */
#define portTHUMB_MODE_BIT				( ( portSTACK_TYPE ) 0x20 )
#define portINTERRUPT_ENABLE_BIT		( 0x80UL )
#define portTHUMB_MODE_ADDRESS			( 0x01UL )

/* Used by portASSERT_IF_INTERRUPT_PRIORITY_INVALID() when ensuring the binary
point is zero. */
#define portBINARY_POINT_BITS			( ( unsigned char ) 0x03 )
#define portMAX_BINARY_POINT_VALUE		0x7

/* Masks all bits in the APSR other than the mode bits. */
#define portAPSR_MODE_BITS_MASK			( 0x1F )

/* The value of the mode bits in the APSR when the CPU is executing in user
mode. */
#define portAPSR_USER_MODE				( 0x10 )

/*-----------------------------------------------------------*/
#ifndef __ASSEMBLER__

uint32_t global_interrupts_disabled(void);

#define __enable_irq()               do { asm volatile ("cpsie i"); } while (0);
#define __disable_irq()              do { asm volatile ("cpsid i"); } while (0);

/* Macro to unmask all interrupt priorities. */
#define portCLEAR_INTERRUPT_MASK()											\
{																			\
	portICCPMR_PRIORITY_MASK_REGISTER_VALUE = portUNMASK_VALUE;					\
	__asm(	"DSB		\n"													\
		"ISB		\n" );												\
}

/*-----------------------------------------------------------
 * Port specific definitions.
 *
 * The settings in this file configure FreeRTOS correctly for the given hardware
 * and compiler.
 *
 * These settings should not be altered.
 *-----------------------------------------------------------
 */

/* Type definitions. */
#define portCHAR		char
#define portFLOAT		float
#define portDOUBLE		double
#define portLONG		long
#define portSHORT		short
#define portSTACK_TYPE	unsigned long
#define portBASE_TYPE	portLONG
// NB: this was 'unsigned long' and requires further testing.
typedef uint32_t portTickType;
#define portMAX_DELAY ( portTickType ) 0xffffffff

/*-----------------------------------------------------------*/

/* Task utilities. */

/* Called at the end of an ISR that can cause a context switch. */
#define portEND_SWITCHING_ISR( xSwitchRequired )\
{												\
extern unsigned long ulPortYieldRequired;		\
												\
	if( xSwitchRequired != pdFALSE )			\
	{											\
		ulPortYieldRequired = pdTRUE;			\
	}											\
}

#define portYIELD_FROM_ISR( x ) portEND_SWITCHING_ISR( x )
#define portYIELD() __asm( "SWI 0" );


/*-----------------------------------------------------------
 * Critical section control
 *----------------------------------------------------------*/

extern void vPortEnterCritical( void );
extern void vPortExitCritical( void );
extern unsigned long ulPortSetInterruptMask( void );
extern void vPortClearInterruptMask( unsigned long ulNewMaskValue );

/* These macros do not globally disable/enable interrupts.  They do mask off
interrupts that have a priority below configMAX_API_CALL_INTERRUPT_PRIORITY. */
#define portENTER_CRITICAL()		vPortEnterCritical();
#define portEXIT_CRITICAL()			vPortExitCritical();
#define portDISABLE_INTERRUPTS()	ulPortSetInterruptMask()
#define portENABLE_INTERRUPTS()		vPortClearInterruptMask( 0 )
#define portSET_INTERRUPT_MASK_FROM_ISR()		ulPortSetInterruptMask()
#define portCLEAR_INTERRUPT_MASK_FROM_ISR(x)	vPortClearInterruptMask(x)

/*-----------------------------------------------------------*/

/* Task function macros as described on the FreeRTOS.org WEB site.  These are
not required for this port but included in case common demo code that uses these
macros is used. */
#define portTASK_FUNCTION_PROTO( vFunction, pvParameters )	void vFunction( void *pvParameters )
#define portTASK_FUNCTION( vFunction, pvParameters )	void vFunction( void *pvParameters )

/* Prototype of the FreeRTOS tick handler.  This must be installed as the
handler for whichever peripheral is used to generate the RTOS tick. */
void FreeRTOS_Tick_Handler( void );

/* Any task that uses the floating point unit MUST call vPortTaskUsesFPU()
before any floating point instructions are executed. */
void vPortTaskUsesFPU( void );
#define portTASK_USES_FLOATING_POINT() vPortTaskUsesFPU()

#define portLOWEST_INTERRUPT_PRIORITY ( ( ( unsigned long ) configUNIQUE_INTERRUPT_PRIORITIES ) - 1UL )
#define portLOWEST_USABLE_INTERRUPT_PRIORITY ( portLOWEST_INTERRUPT_PRIORITY - 1UL )

/* Architecture specific optimisations. */
#if configUSE_PORT_OPTIMISED_TASK_SELECTION == 1

	/* Store/clear the ready priorities in a bit map. */
	#define portRECORD_READY_PRIORITY( uxPriority, uxReadyPriorities ) ( uxReadyPriorities ) |= ( 1UL << ( uxPriority ) )
	#define portRESET_READY_PRIORITY( uxPriority, uxReadyPriorities ) ( uxReadyPriorities ) &= ~( 1UL << ( uxPriority ) )

	/*-----------------------------------------------------------*/

	#define portGET_HIGHEST_PRIORITY( uxTopPriority, uxReadyPriorities ) uxTopPriority = ( 31 - __CLZ( uxReadyPriorities ) )

#endif /* configUSE_PORT_OPTIMISED_TASK_SELECTION */

#ifdef configASSERT
	void vPortValidateInterruptPriority( void );
	#define portASSERT_IF_INTERRUPT_PRIORITY_INVALID() 	vPortValidateInterruptPriority()
#endif /* configASSERT */

#define portNOP() __asm volatile( "NOP" )

#endif /* __ASSEMBLER__ */

#ifdef __cplusplus
	} /* extern C */
#endif

#endif /* PORTMACRO_H */
