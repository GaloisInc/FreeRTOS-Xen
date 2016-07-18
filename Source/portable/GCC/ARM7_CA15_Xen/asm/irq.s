
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

#include "portmacro.h"
#include <port/gic.h>
#include "print.s"
#include "dumpregs.s"
#include "arm_modes.s"

	.section ".start"
	.globl	irq_handler
	.globl  swi_handler
        .globl  vPortRestoreTaskContext
	.globl  global_interrupts_disabled

.macro portSAVE_CONTEXT
	@ Start by saving the current LR_irq and SPSR_irq values on the system
	@ mode stack.  This saves the address of the task resume PC
	@ (decremented at the beginning of the IRQ handler) and the task CPSR.
	SRSDB	SP!, #SYS_MODE
	CPS	#SYS_MODE

	@ Save the non-banked registers and system mode LR on the system mode
	@ stack.  This saves the task register state.
	PUSH	{R0-R12, R14}

	/* Push the critical nesting count */
	LDR		R2, =ulCriticalNesting
	LDR		R1, [R2]
	PUSH	{R1}

	/* Does the task have a floating point context that needs saving?  If */
	/* ulPortTaskHasFPUContext is 0 then no. */
	LDR		R2, =ulPortTaskHasFPUContext
	LDR		R3, [R2]
	CMP		R3, #0
        BEQ             1f

	/* Save the floating point context, if any */
	FMRX            R1,  FPSCR
	VPUSH           {D0-D15}
	VPUSH	        {D16-D31}
	PUSH         	{R1}

1:
	/* Save ulPortTaskHasFPUContext itself */
	PUSH	        {R3}

	/* Save the task stack pointer in the TCB */
	LDR		R0, =pxCurrentTCB
	LDR		R1, [R0]
	STR		SP, [R1]
.endm

.macro portRESTORE_CONTEXT
	/* Switch to system mode, since that is the mode in which tasks run */
	CPS		#SYS_MODE

	/* Set the system mode SP to point to the stack of the task being
	 * restored. */
	LDR		R0, =pxCurrentTCB
	LDR		R1, [R0]
	LDR		SP, [R1]

	/* Is there a floating point context to restore?  If the restored */
	/* ulPortTaskHasFPUContext is zero then no. */
	LDR		R0, =ulPortTaskHasFPUContext
	POP		{R1}
	STR		R1, [R0]
	CMP		R1, #0
	BEQ             1f

	/* Restore the floating point context, if any */
	LDMFD           SP!, {R0}
	VPOP	        {D16-D31}
	VPOP	        {D0-D15}
	VMSR            FPSCR, R0

1:
	/* Restore the critical section nesting depth */
	LDR		R0, =ulCriticalNesting
	POP		{R1}
	STR		R1, [R0]

	/* Ensure the priority mask is correct for the critical nesting depth */
        MOV             R5, #configMAX_API_CALL_INTERRUPT_PRIORITY
        LDR             R6, =portPRIORITY_SHIFT
	LDR		R6, [R6]
        LSL             R5, R5, R6
	CMP		R1, #0
	MOVEQ	        R0, #255
	MOVNE	        R0, R5
    bl      gic_cpu_set_priority_mask

	/* Restore all system mode registers other than the SP (which is
	 * already being used) */
	POP		{R0-R12, R14}

	/* Return to the task code, loading the PC and CPSR from the task
	 * stack. */
	RFEIA		sp!
.endm

global_interrupts_disabled:
        MRS    r0, cpsr
        and    r0, r0, #CPSR_I
	bx     lr

swi_handler:
	/* Save the context of the current task and select a new task to run. */
	portSAVE_CONTEXT
	LDR R0, =vTaskSwitchContext
	BLX	R0

vPortRestoreTaskContext:
	portRESTORE_CONTEXT

# Interrupt handler.  Place this in the exception vector table in the IRQ
# handler slot.  Requires an irq_stack pointer which points to the bottom of
# the IRQ stack.  Calls the gic_handler function to handle all interrupts.
irq_handler:
	/* Decrement the interrupted PC value (now in LR) by 4 since it points
	   to the instruction *after* the one which was interrupted.  To resume
	   correctly we need to back the PC up to the previous instruction. */
        SUB             lr, lr, #4

	@ Save LR, SPSR of interrupted code on stack; save non-banked registers, too
        push    {lr}
	MRS	lr, SPSR
	PUSH	{lr}

	cps	#SVC_MODE
        push    {r0-r12, lr}

#if configINTERRUPT_NESTING
	/* Increment nesting count.  r3 holds the address of
           ulPortInterruptNesting for future use.  r1 holds the original
           ulPortInterruptNesting value for future use. */
	LDR	r3, =ulPortInterruptNesting
	LDR	r1, [r3]
	ADD	r4, r1, #1
	STR	r4, [r3]
#endif

        push    {r0-r4}

	@ Ack the interrupt; IRQ ID will be in r0
	bl	gic_readiar

	push	{r0}

#if configINTERRUPT_NESTING
	cpsie   i
#endif

	@ Handle the interrupt
	bl      gic_handler

#if configINTERRUPT_NESTING
	cpsid   i
#endif

	@ Signal EOI
	pop	{r0}
	bl	gic_eoir

        pop     {r0-r4}

#if configINTERRUPT_NESTING
	@ Restore the old nesting count
	STR	r1, [r3]

	@ A context switch is never performed if the nesting count is not 0
	CMP	r1, #0
	BNE	exit_without_switch
#endif

	/* Did the interrupt request a context switch?  r1 holds the address of */
	/* ulPortYieldRequired and r0 the value of ulPortYieldRequired for future */
	/* use. */
	LDR	r1, =ulPortYieldRequired
	LDR	r0, [r1]
	CMP	r0, #0
	BNE	switch_before_exit

exit_without_switch:
	/* No context switch.  Restore used registers but do NOT restore SPSR
	 * and LR_irq because we need to leave them there for RFE below. */
	POP	{r0-r12, lr}

	CPS	#IRQ_MODE

	/* Return to the interrupted code. RFE restores CPSR and PC from the
	 * IRQ mode stack. The value of PC on the stack is the LR that we
	 * decremented at the start of the interrupt handler. */
        POP     {LR}
        MSR     SPSR_cxsf, LR
        POP     {LR}
        MOVS    PC, LR

switch_before_exit:
	/* A context switch is to be performed.  Clear the context switch pending */
	/* flag. */
	LDR	r1, =ulPortYieldRequired
	MOV	r0, #0
	STR	r0, [r1]

	/* Restore used registers, LR-irq and SPSR before saving the context */
	/* to the task stack.   We pop LR_irq and SPSR here because they're
	 * going to be needed by the portSAVE_CONTEXT routine to store them on
	 * the stack again with SRS. */
	POP	{r0-r12, lr}
	cps     #IRQ_MODE

	POP	{LR}
	MSR	SPSR_cxsf, LR
	POP	{LR}

	portSAVE_CONTEXT

	/* Call the function that selects the new task to execute. */
	/* vTaskSwitchContext() if vTaskSwitchContext() uses LDRD or STRD */
	/* instructions, or 8 byte aligned stack allocated data.  LR does not need */
	/* saving as a new LR will be loaded by portRESTORE_CONTEXT anyway. */
	bl	vTaskSwitchContext

	/* Restore the context of, and branch to, the task selected to execute next. */
	portRESTORE_CONTEXT
