/* is-sub-test.S. l.sub instruction test of Or1ksim
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
 * Coding conventions are described in inst-set-test.S
 * ------------------------------------------------------------------------- */

/* ----------------------------------------------------------------------------
 * Test coverage
 *
 * The l.sub instruction should set the carry and overflow flags.
 *
 * Problems in this area were reported in Bugs 1782, 1783 and 1784. Having
 * fixed the problem, this is (in good software engineering style), a
 * regression test to go with the fix.
 *
 * This is not a comprehensive test of any instruction (yet).
 *
 * Of course what is really needed is a comprehensive instruction test...
 * ------------------------------------------------------------------------- */


#include "inst-set-test.h"

/* ----------------------------------------------------------------------------
 * A macro to carry out a test of subtraction in registers
 *
 *
 * Arguments
 *   set_flags: Flags to set in the SR
 *   clr_flags: Flags to clear in the SR
 *   op1:       First operand value
 *   op2:       Second operand value
 *   res:       Expected result
 *   cy:        Expected carry flag
 *   ov:        Expected overflow flag
 * ------------------------------------------------------------------------- */
#define TEST_SUB(set_flags, clr_flags, op1, op2, res, cy, ov)		 \
	l.mfspr	r3,r0,SPR_SR						;\
	LOAD_CONST (r2, set_flags)	/* Set flags */			;\
	l.or	r3,r3,r2						;\
	LOAD_CONST (r2, ~clr_flags)	/* Clear flags */		;\
	l.and	r3,r3,r2						;\
	l.mtspr	r0,r3,SPR_SR						;\
									;\
	LOAD_CONST (r5,op1)		/* Load numbers to subtract */	;\
	LOAD_CONST (r6,op2)						;\
	l.mtspr	r0,r0,SPR_EPCR_BASE	/* Clear record */		;\
50:	l.sub	r4,r5,r6						;\
	l.mfspr	r2,r0,SPR_SR		/* So we can examine flags */	;\
	l.mfspr	r5,r0,SPR_EPCR_BASE	/* What triggered exception */	;\
	PUSH (r5)			/* Save EPCR for later */	;\
	PUSH (r2)							;\
	PUSH (r4)			/* Save result for later */	;\
									;\
	PUTS ("  0x")							;\
	PUTH (op1)							;\
	PUTS (" - 0x")							;\
	PUTH (op2)							;\
	PUTS (" = 0x")							;\
	PUTH (res)							;\
	PUTS (": ")							;\
	POP (r4)							;\
	CHECK_RES1 (r4, res)						;\
									;\
	POP(r2)				/* Retrieve SR */		;\
	PUSH(r2)							;\
	LOAD_CONST (r4, SPR_SR_CY)	/* The carry bit */		;\
	l.and	r2,r2,r4						;\
	l.sfeq	r2,r4							;\
	CHECK_FLAG ("- carry flag set:      ", cy)			;\
									;\
	POP(r2)				/* Retrieve SR */		;\
	LOAD_CONST (r4, SPR_SR_OV)	/* The overflow bit */		;\
	l.and	r2,r2,r4						;\
	l.sfeq	r2,r4							;\
	CHECK_FLAG ("- overflow flag set:   ", ov)			;\
									;\
	POP (r2)			/* Retrieve EPCR */		;\
	LOAD_CONST (r4, 50b)		/* The opcode of interest */	;\
	l.and	r2,r2,r4						;\
	l.sfeq	r2,r4							;\
	l.bnf	53f							;\
									;\
	PUTS ("  - exception triggered: TRUE\n")			;\
	l.j	54f							;\
	l.nop								;\
									;\
53:	PUTS ("  - exception triggered: FALSE\n")			;\
54:	


/* ----------------------------------------------------------------------------
 * Start of code
 * ------------------------------------------------------------------------- */
	.section .text
	.global	_start
_start:
	l.mfspr	r3,r0,SPR_SR
	LOAD_CONST (r2, ~SPR_SR_OVE)	/* Clear OVE */
	l.and	r3,r3,r2
	l.mtspr	r0,r3,SPR_SR
	
	LOAD_STR (r3, "  ** OVE flag cleared **\n")
	l.jal	_puts
	l.nop

/* ----------------------------------------------------------------------------
 * Test of subtract signed, l.sub
 * ------------------------------------------------------------------------- */
_sub:
	LOAD_STR (r3, "l.sub\n")
	l.jal	_puts
	l.nop

	/* Subtract two small positive numbers. Sets the carry, but never the
	   overflow if the result is negative. */
	TEST_SUB (0, SPR_SR_CY | SPR_SR_OV,
	          0x00000003, 0x00000002, 0x00000001,
	          FALSE, FALSE)

	TEST_SUB (0, SPR_SR_CY | SPR_SR_OV,
	          0x00000001, 0x00000002, 0xffffffff,
	          TRUE, FALSE)

	/* Check carry in is ignored. */
	TEST_SUB (SPR_SR_CY, SPR_SR_OV,
	          0x00000003, 0x00000002, 0x00000001,
	          FALSE, FALSE)

	/* Subtract two small negative numbers. Sets the carry flag if the
	   result is negative, but never the overflow flag. */
	TEST_SUB (0, SPR_SR_CY | SPR_SR_OV,
	          0xfffffffd, 0xfffffffe, 0xffffffff,
	          TRUE, FALSE)

	TEST_SUB (0, SPR_SR_CY | SPR_SR_OV,
	          0xffffffff, 0xfffffffe, 0x00000001,
	          FALSE, FALSE)

	/* Subtract two quite large positive numbers. Should set neither the
	   overflow nor the carry flag. */
	TEST_SUB (0, SPR_SR_CY | SPR_SR_OV,
	          0x7fffffff, 0x3fffffff, 0x40000000,
	          FALSE, FALSE)

	/* Subtract two quite large negative numbers. Should set neither the
	   overflow nor the carry flag. */
	TEST_SUB (0, SPR_SR_CY | SPR_SR_OV,
	          0x40000000, 0x40000000, 0x00000000,
	          FALSE, FALSE)

	/* Subtract two large positive numbers with a negative result. Should
	   set the carry, but not the overflow flag. */
	TEST_SUB (0, SPR_SR_CY | SPR_SR_OV,
	          0x3fffffff, 0x40000000, 0xffffffff,
	          TRUE, FALSE)

	/* Subtract two large negative numbers with a positive result. Should
	   set niether the carry nor the overflow flag. */
	TEST_SUB (0, SPR_SR_CY | SPR_SR_OV,
	          0x40000000, 0x3fffffff, 0x00000001,
	          FALSE, FALSE)

	/* Subtract a large positive from a large negative number. Should set
	   overflow but not the carry flag. */
	TEST_SUB (0, SPR_SR_CY | SPR_SR_OV,
	          0x80000000, 0x7fffffff, 0x00000001,
	          FALSE, TRUE)

	/* Subtract a large negative from a large positive number. Should set
	   both the overflow and carry flags. */
	TEST_SUB (0, SPR_SR_CY | SPR_SR_OV,
	          0x7fffffff, 0x80000000, 0xffffffff,
	          TRUE, TRUE)

	/* Check that range exceptions are triggered */
	l.mfspr	r3,r0,SPR_SR
	LOAD_CONST (r2, SPR_SR_OVE)	/* Set OVE */
	l.or	r3,r3,r2
	l.mtspr	r0,r3,SPR_SR
	
	LOAD_STR (r3, "  ** OVE flag set **\n")
	l.jal	_puts
	l.nop

	/* Check that an overflow alone causes a RANGE Exception. */
	TEST_SUB (0, SPR_SR_CY | SPR_SR_OV,
	          0x80000000, 0x7fffffff, 0x00000001,
	          FALSE, TRUE)

	/* Check that a carry alone does not cause a RANGE Exception. */
	TEST_SUB (0, SPR_SR_CY | SPR_SR_OV,
	          0x3fffffff, 0x40000000, 0xffffffff,
	          TRUE, FALSE)

	/* Check that carry and overflow together cause an exception. */
	TEST_SUB (0, SPR_SR_CY | SPR_SR_OV,
	          0x7fffffff, 0x80000000, 0xffffffff,
	          TRUE, TRUE)

	/* Finished checking range exceptions */
	l.mfspr	r3,r0,SPR_SR
	LOAD_CONST (r2, ~SPR_SR_OVE)	/* Clear OVE */
	l.and	r3,r3,r2
	l.mtspr	r0,r3,SPR_SR
	
	LOAD_STR (r3, "  ** OVE flag cleared **\n")
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