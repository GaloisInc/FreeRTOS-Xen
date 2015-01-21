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


#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#ifndef __ASSEMBLER__
#include <stdlib.h>

/* Run time stats gathering definitions. */
unsigned long ulGetRunTimeCounterValue(void);
void vInitialiseRunTimeStats(void);

#define portCONFIGURE_TIMER_FOR_RUN_TIME_STATS() vInitialiseRunTimeStats()
#define portGET_RUN_TIME_COUNTER_VALUE() ulGetRunTimeCounterValue()

/* The size of the global output buffer that is available for use when there
are multiple command interpreters running at once (for example, one on a UART
and one on TCP/IP).  This is done to prevent an output buffer being defined by
each implementation - which would waste RAM.  In this case, there is only one
command interpreter running. */
#define configCOMMAND_INT_MAX_OUTPUT_SIZE 2096
#define configGENERATE_RUN_TIME_STATS	0

/* Normal assert() semantics without relying on the provision of an assert.h
header file. */
void vAssertCalled(const char * pcFile, unsigned long ulLine);
#define configASSERT( x ) if( ( x ) == 0 ) vAssertCalled( __FILE__, __LINE__ );

/****** Hardware specific settings. *******************************************/

void vConfigureTickInterrupt(void);
#define configSETUP_TICK_INTERRUPT() vConfigureTickInterrupt()

#endif

/*-----------------------------------------------------------
 * Application specific definitions.
 *
 * These definitions should be adjusted for your particular hardware and
 * application requirements.
 *
 * THESE PARAMETERS ARE DESCRIBED WITHIN THE 'CONFIGURATION' SECTION OF THE
 * FreeRTOS API DOCUMENTATION AVAILABLE ON THE FreeRTOS.org WEB SITE.
 *
 * See http://www.freertos.org/a00110.html.
 *----------------------------------------------------------*/

/*
 * The FreeRTOS Cortex-A port implements a full interrupt nesting model.
 *
 * Interrupts that are assigned a priority at or below
 * configMAX_API_CALL_INTERRUPT_PRIORITY (which counter-intuitively in the ARM
 * generic interrupt controller [GIC] means a priority that has a numerical
 * value above configMAX_API_CALL_INTERRUPT_PRIORITY) can call FreeRTOS safe API
 * functions and will nest.
 *
 * Interrupts that are assigned a priority above
 * configMAX_API_CALL_INTERRUPT_PRIORITY (which in the GIC means a numerical
 * value below configMAX_API_CALL_INTERRUPT_PRIORITY) cannot call any FreeRTOS
 * API functions, will nest, and will not be masked by FreeRTOS critical
 * sections (although it is necessary for interrupts to be globally disabled
 * extremely briefly as the interrupt mask is updated in the GIC).
 *
 * FreeRTOS functions that can be called from an interrupt are those that end in
 * "FromISR".  FreeRTOS maintains a separate interrupt safe API to enable
 * interrupt entry to be shorter, faster, simpler and smaller.
 */
#define configMAX_API_CALL_INTERRUPT_PRIORITY		25
#define configTICK_PRIORITY				30
#define configINTERRUPT_NESTING				0

// We need an assembly-safe version of the tick rate as well as the C version
#define configTICK_RATE_HZ_ASM				1000
#define configTICK_RATE_HZ				( ( portTickType ) configTICK_RATE_HZ_ASM )

#define configUSE_PORT_OPTIMISED_TASK_SELECTION		0
#define configUSE_TICKLESS_IDLE				0
#define configUSE_PREEMPTION				1
#define configUSE_IDLE_HOOK				0
#define configUSE_TICK_HOOK				0
#define configMAX_PRIORITIES				( 5 )
// This needs to be big enough to accomodate all general-purpose registers, FPU
// registers, and any other saved context.
#define configMINIMAL_STACK_SIZE			( ( unsigned short ) 1024 )
#define configTOTAL_HEAP_SIZE				( ( size_t ) ( 1048576 ) )
#define configMAX_TASK_NAME_LEN				( 10 )
#define configUSE_TRACE_FACILITY			1
#define configUSE_16_BIT_TICKS				0
#define configIDLE_SHOULD_YIELD				1
#define configUSE_MUTEXES				1
#define configQUEUE_REGISTRY_SIZE			8
#define configCHECK_FOR_STACK_OVERFLOW			2
#define configUSE_RECURSIVE_MUTEXES			1
#define configUSE_MALLOC_FAILED_HOOK			0
#define configUSE_APPLICATION_TASK_TAG			0
#define configUSE_COUNTING_SEMAPHORES			1

// Off means per-task FPU usage, i.e., tasks must activate the FPU explicitly.
// On means FPU usage is on for every task and need not be enabled explicitly.
#define configALL_TASKS_USE_FPU                         1
// Used to determine how much stack space is needed to save FPU state in
// context switch.
#define configFPU_NUM_REGISTERS				32
#define configFPU_BYTES_PER_REGISTER			8

/* Co-routine definitions. */
#define configUSE_CO_ROUTINES 				0
#define configMAX_CO_ROUTINE_PRIORITIES			( 2 )

/* Xen-related settings */
#define configXENBUS_TASK_PRIORITY			configTIMER_TASK_PRIORITY
#define configEVENT_IRQ_PRIORITY			configTICK_PRIORITY
#define configUSE_XEN_CONSOLE                           0

/* Software timer definitions. */
#define configUSE_TIMERS				0
#define configTIMER_TASK_PRIORITY			( configMAX_PRIORITIES - 1 )
#define configTIMER_QUEUE_LENGTH			5
#define configTIMER_TASK_STACK_DEPTH			( configMINIMAL_STACK_SIZE * 2 )

/* Set the following definitions to 1 to include the API function, or zero
to exclude the API function. */
#define INCLUDE_vTaskPrioritySet			1
#define INCLUDE_uxTaskPriorityGet			1
#define INCLUDE_vTaskDelete				1
#define INCLUDE_vTaskCleanUpResources			1
#define INCLUDE_vTaskSuspend				1
#define INCLUDE_vTaskDelayUntil				1
#define INCLUDE_vTaskDelay				1

/* This demo makes use of one or more example stats formatting functions.  These
format the raw data provided by the uxTaskGetSystemState() function in to human
readable ASCII form.  See the notes in the implementation of vTaskList() within
FreeRTOS/Source/tasks.c for limitations. */
#define configUSE_STATS_FORMATTING_FUNCTIONS		1

// We won't know how many unique priorities are supported until we interrogate
// the GIC.  Once that happens, this variable will contain the total number of
// interrupt priorities.
#define configUNIQUE_INTERRUPT_PRIORITIES		ulGICUniqueIntPriorities

#define FreeRTOS_IRQ_Handler IRQ_Handler
#define FreeRTOS_SWI_Handler SWI_Handler

#define vPortYieldFromISR() portYIELD_FROM_ISR(pdTRUE)

#endif /* FREERTOS_CONFIG_H */
