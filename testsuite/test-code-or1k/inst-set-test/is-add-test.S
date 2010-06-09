/* is-add-test.S. l.add, l.addc, l.addi and l.addic instruction test of Or1ksim
 * 
 * Copyright (C) 1999-2006 OpenCores
 * Copyright (C) 2010 Embecosm Limited
 * 
 * Contributors various OpenCores participants
 * Contributor Jeremy Bennett <jeremy.bennett@embecosm.com>
 * 
 * This file is part of OpenRISC 1000 Architectural Simulator.
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 3 of the License, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http:  www.gnu.org/licenses/>.
 */

/* ----------------------------------------------------------------------------
 * Coding conventions
 *
 * A simple rising stack is provided starting at _stack and pointed to by
 * r1. r1 points to the next free word. Only 32-bit registers may be pushed
 * onto the stack.
 *
 * Local labels up to 49 are reserved for macros. Each is used only once in
 * all macros. You can get in a serious mess if you get local label clashing
 * in macros.
 *
 * Arguments to functions are passed in r3 through r8.
 * r9 is the link (return address)
 * r11 is for returning results
 *
 * Only r1 and r2 are preserved across function calls. It is up to the callee
 * to save any other registers required.
 * ------------------------------------------------------------------------- */

/* ----------------------------------------------------------------------------
 * Test coverage
 *
 * The l.add, l.addc, l.addi and l.addic instructions should set the carry and
 * overflow flags.
 *
 * In addition the l.addc and l.addic instructions should add in the carry
 * bit.
 *
 * Problems in this area were reported in Bug 1771. Having fixed the problem,
 * this is (in good software engineering style), a  regression test to go with
 * the fix.
 *
 * This is not a comprehensive test of any instruction (yet).
 *
 * Of course what is really needed is a comprehensive instruction test...
 * ------------------------------------------------------------------------- */


#include "inst-set-test.h"

	.section .text
	.global	_start
_start:

/* ----------------------------------------------------------------------------
 * Test of add signed, l.add
 * ------------------------------------------------------------------------- */
_add:
	LOAD_STR (r3, "l.add\n")
	l.jal	_puts
	l.nop

	/* Add two small positive numbers */
	LOAD_CONST (r2, ~(SPR_SR_CY | SPR_SR_OV))	/* Clear flags */
	l.mfspr	r3,r0,SPR_SR
	l.and	r3,r3,r2
	l.mtspr	r0,r3,SPR_SR

	LOAD_CONST (r5,1)		/* Add two small positive numbers */
	LOAD_CONST (r6,2)
	l.add	r4,r5,r6
	l.mfspr	r2,r0,SPR_SR		/* So we can examine the carry flag */
	PUSH (r2)
	CHECK_RES ("0x00000001 + 0x00000002 = 0x00000003: ", r4, 0x00000003)

	POP(r2)				/* Retrieve SR */
	PUSH(r2)
	LOAD_CONST (r4, SPR_SR_CY)	/* The carry bit */
	l.and	r2,r2,r4
	l.sfeq	r2,r4
	CHECK_FLAG ("- carry flag set:    ", FALSE)

	POP(r2)				/* Retrieve SR */
	PUSH(r2)
	LOAD_CONST (r4, SPR_SR_OV)	/* The carry bit */
	l.and	r2,r2,r4
	l.sfeq	r2,r4
	CHECK_FLAG ("- overflow flag set: ", FALSE)

	/* Add two small negative numbers */
	LOAD_CONST (r2, ~(SPR_SR_CY | SPR_SR_OV))	/* Clear flags */
	l.mfspr	r3,r0,SPR_SR
	l.and	r3,r3,r2
	l.mtspr	r0,r3,SPR_SR

	LOAD_CONST (r5,0xffffffff)	/* Add two small negative numbers */
	LOAD_CONST (r6,0xfffffffe)
	l.add	r4,r5,r6
	l.mfspr	r2,r0,SPR_SR		/* So we can examine the carry flag */
	PUSH (r2)
	CHECK_RES ("0xffffffff + 0xfffffffe = 0xfffffffd: ", r4, 0xfffffffd)

	POP(r2)				/* Retrieve SR */
	PUSH(r2)
	LOAD_CONST (r4, SPR_SR_CY)	/* The carry bit */
	l.and	r2,r2,r4
	l.sfeq	r2,r4
	CHECK_FLAG ("- carry flag set:    ", TRUE)

	POP(r2)				/* Retrieve SR */
	PUSH(r2)
	LOAD_CONST (r4, SPR_SR_OV)	/* The carry bit */
	l.and	r2,r2,r4
	l.sfeq	r2,r4
	CHECK_FLAG ("- overflow flag set: ", FALSE)

	/* Add two quite large positive numbers. Should set neither the
	   overflow nor the carry flag. */
	LOAD_CONST (r2, ~(SPR_SR_CY | SPR_SR_OV))	/* Clear flags */
	l.mfspr	r3,r0,SPR_SR
	l.and	r3,r3,r2
	l.mtspr	r0,r3,SPR_SR

	LOAD_CONST (r5,0x40000000)	/* Add two large positive numbers */
	LOAD_CONST (r6,0x3fffffff)
	l.add	r4,r5,r6
	l.mfspr	r2,r0,SPR_SR		/* So we can examine the carry flag */
	PUSH (r2)
	CHECK_RES ("0x40000000 + 0x3fffffff = 0x7fffffff: ", r4, 0x7fffffff)

	POP(r2)				/* Retrieve SR */
	PUSH(r2)
	LOAD_CONST (r4, SPR_SR_CY)	/* The carry bit */
	l.and	r2,r2,r4
	l.sfeq	r2,r4
	CHECK_FLAG ("- carry flag set:    ", FALSE)

	POP(r2)				/* Retrieve SR */
	PUSH(r2)
	LOAD_CONST (r4, SPR_SR_OV)	/* The carry bit */
	l.and	r2,r2,r4
	l.sfeq	r2,r4
	CHECK_FLAG ("- overflow flag set: ", FALSE)

	/* Add two large positive numbers. Should set the overflow, but not
	   the carry flag. */
	LOAD_CONST (r2, ~(SPR_SR_CY | SPR_SR_OV))	/* Clear flags */
	l.mfspr	r3,r0,SPR_SR
	l.and	r3,r3,r2
	l.mtspr	r0,r3,SPR_SR

	LOAD_CONST (r5,0x40000000)	/* Add two large positive numbers */
	LOAD_CONST (r6,0x40000000)
	l.add	r4,r5,r6
	l.mfspr	r2,r0,SPR_SR		/* So we can examine the carry flag */
	PUSH (r2)
	CHECK_RES ("0x40000000 + 0x40000000 = 0x80000000: ", r4, 0x80000000)

	POP(r2)				/* Retrieve SR */
	PUSH(r2)
	LOAD_CONST (r4, SPR_SR_CY)	/* The carry bit */
	l.and	r2,r2,r4
	l.sfeq	r2,r4
	CHECK_FLAG ("- carry flag set:    ", FALSE)

	POP(r2)				/* Retrieve SR */
	PUSH(r2)
	LOAD_CONST (r4, SPR_SR_OV)	/* The carry bit */
	l.and	r2,r2,r4
	l.sfeq	r2,r4
	CHECK_FLAG ("- overflow flag set: ", TRUE)

	/* Add two quite large negative numbers. Should set the carry, but not
	   the overflow flag. flag. */
	LOAD_CONST (r2, ~(SPR_SR_CY | SPR_SR_OV))	/* Clear flags */
	l.mfspr	r3,r0,SPR_SR
	l.and	r3,r3,r2
	l.mtspr	r0,r3,SPR_SR

	LOAD_CONST (r5,0xc0000000)	/* Add two large positive numbers */
	LOAD_CONST (r6,0xc0000000)
	l.add	r4,r5,r6
	l.mfspr	r2,r0,SPR_SR		/* So we can examine the carry flag */
	PUSH (r2)
	CHECK_RES ("0xc0000000 + 0xc0000000 = 0x80000000: ", r4, 0x80000000)

	POP(r2)				/* Retrieve SR */
	PUSH(r2)
	LOAD_CONST (r4, SPR_SR_CY)	/* The carry bit */
	l.and	r2,r2,r4
	l.sfeq	r2,r4
	CHECK_FLAG ("- carry flag set:    ", TRUE)

	POP(r2)				/* Retrieve SR */
	PUSH(r2)
	LOAD_CONST (r4, SPR_SR_OV)	/* The carry bit */
	l.and	r2,r2,r4
	l.sfeq	r2,r4
	CHECK_FLAG ("- overflow flag set: ", FALSE)

	/* Add two large negative numbers. Should set both the overflow and
	   carry flags. */
	LOAD_CONST (r2, ~(SPR_SR_CY | SPR_SR_OV))	/* Clear flags */
	l.mfspr	r3,r0,SPR_SR
	l.and	r3,r3,r2
	l.mtspr	r0,r3,SPR_SR

	LOAD_CONST (r5,0xbfffffff)	/* Add two large negative numbers */
	LOAD_CONST (r6,0xbfffffff)
	l.add	r4,r5,r6
	l.mfspr	r2,r0,SPR_SR		/* So we can examine the carry flag */
	PUSH (r2)
	CHECK_RES ("0xbfffffff + 0xbfffffff = 0x7ffffffe: ", r4, 0x7ffffffe)

	POP(r2)				/* Retrieve SR */
	PUSH(r2)
	LOAD_CONST (r4, SPR_SR_CY)	/* The carry bit */
	l.and	r2,r2,r4
	l.sfeq	r2,r4
	CHECK_FLAG ("- carry flag set:    ", TRUE)

	POP(r2)				/* Retrieve SR */
	PUSH(r2)
	LOAD_CONST (r4, SPR_SR_OV)	/* The carry bit */
	l.and	r2,r2,r4
	l.sfeq	r2,r4
	CHECK_FLAG ("- overflow flag set: ", TRUE)

	/* Check that range exceptions are triggered */
	LOAD_CONST (r2, SPR_SR_OVE)	/* Set OVE */
	l.mfspr	r3,r0,SPR_SR
	l.or	r3,r3,r2
	l.mtspr	r0,r3,SPR_SR
	
	LOAD_STR (r3, "  OVE flag set\n")
	l.jal	_puts
	l.nop

	/* Check that an overflow alone causes a RANGE Exception. */
	LOAD_CONST (r2, ~(SPR_SR_CY | SPR_SR_OV))	/* Clear flags */
	l.mfspr	r3,r0,SPR_SR
	l.and	r3,r3,r2
	l.mtspr	r0,r3,SPR_SR

	LOAD_CONST (r5,0x40000000)	/* Add two large positive numbers */
	LOAD_CONST (r6,0x40000000)
	l.add	r4,r5,r6
	l.mfspr	r2,r0,SPR_SR		/* So we can examine the carry flag */
	PUSH (r2)
	CHECK_RES ("0x40000000 + 0x40000000 = 0x80000000: ", r4, 0x80000000)

	POP(r2)				/* Retrieve SR */
	PUSH(r2)
	LOAD_CONST (r4, SPR_SR_CY)	/* The carry bit */
	l.and	r2,r2,r4
	l.sfeq	r2,r4
	CHECK_FLAG ("- carry flag set:    ", FALSE)

	POP(r2)				/* Retrieve SR */
	PUSH(r2)
	LOAD_CONST (r4, SPR_SR_OV)	/* The carry bit */
	l.and	r2,r2,r4
	l.sfeq	r2,r4
	CHECK_FLAG ("- overflow flag set: ", TRUE)

	/* Check that a carry alone does not cause a RANGE Exception. */
	LOAD_CONST (r2, ~(SPR_SR_CY | SPR_SR_OV))	/* Clear flags */
	l.mfspr	r3,r0,SPR_SR
	l.and	r3,r3,r2
	l.mtspr	r0,r3,SPR_SR

	LOAD_CONST (r5,0xffffffff)	/* Add two small negative numbers */
	LOAD_CONST (r6,0xfffffffe)
	l.add	r4,r5,r6
	l.mfspr	r2,r0,SPR_SR		/* So we can examine the carry flag */
	PUSH (r2)
	CHECK_RES ("0xffffffff + 0xfffffffe = 0xfffffffd: ", r4, 0xfffffffd)

	POP(r2)				/* Retrieve SR */
	PUSH(r2)
	LOAD_CONST (r4, SPR_SR_CY)	/* The carry bit */
	l.and	r2,r2,r4
	l.sfeq	r2,r4
	CHECK_FLAG ("- carry flag set:    ", TRUE)

	POP(r2)				/* Retrieve SR */
	PUSH(r2)
	LOAD_CONST (r4, SPR_SR_OV)	/* The carry bit */
	l.and	r2,r2,r4
	l.sfeq	r2,r4
	CHECK_FLAG ("- overflow flag set: ", FALSE)

	/* Check that carry and overflow together cause an exception. */
	LOAD_CONST (r2, ~(SPR_SR_CY | SPR_SR_OV))	/* Clear flags */
	l.mfspr	r3,r0,SPR_SR
	l.and	r3,r3,r2
	l.mtspr	r0,r3,SPR_SR

	LOAD_CONST (r5,0xbfffffff)	/* Add two large negative numbers */
	LOAD_CONST (r6,0xbfffffff)
	l.add	r4,r5,r6
	l.mfspr	r2,r0,SPR_SR		/* So we can examine the carry flag */
	PUSH (r2)
	CHECK_RES ("0xbfffffff + 0xbfffffff = 0x7ffffffe: ", r4, 0x7ffffffe)

	POP(r2)				/* Retrieve SR */
	PUSH(r2)
	LOAD_CONST (r4, SPR_SR_CY)	/* The carry bit */
	l.and	r2,r2,r4
	l.sfeq	r2,r4
	CHECK_FLAG ("- carry flag set:    ", TRUE)

	POP(r2)				/* Retrieve SR */
	PUSH(r2)
	LOAD_CONST (r4, SPR_SR_OV)	/* The carry bit */
	l.and	r2,r2,r4
	l.sfeq	r2,r4
	CHECK_FLAG ("- overflow flag set: ", TRUE)

	/* Finished checking range exceptions */
	LOAD_CONST (r2, ~SPR_SR_OVE)	/* Clear OVE */
	l.mfspr	r3,r0,SPR_SR
	l.and	r3,r3,r2
	l.mtspr	r0,r3,SPR_SR
	
	LOAD_STR (r3, "  OVE flag cleared\n")
	l.jal	_puts
	l.nop

/* ----------------------------------------------------------------------------
 * All done
 * ------------------------------------------------------------------------- */
_exit:
	LOAD_STR (r3, "Test completed\n")
	l.jal	_puts
	l.nop

	TEST_EXIT