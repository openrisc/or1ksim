/* is-mul-test.S. l.mul, l.muli and l.mulu instruction test of Or1ksim
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
 * The l.mul, l.muli and l.mulu instructions should all be present and set the
 * carry and overflow flags.
 *
 * Problems in this area were reported in Bugs 1774, 1782, 1783 and 1784.
 * Having fixed the problem, this is (in good software engineering style), a
 * regression test to go with the fix.
 *
 * This is not a comprehensive test of any instruction (yet).
 *
 * Of course what is really needed is a comprehensive instruction test...
 * ------------------------------------------------------------------------- */


#include "inst-set-test.h"

/* ----------------------------------------------------------------------------
 * A macro to carry out a test of multiply signed or unsigned
 *
 * Arguments
 *   opc:       The opcode
 *   op1:       First operand value
 *   op2:       Second operand value
 *   res:       Expected result
 *   cy:        Expected carry flag
 *   ov:        Expected overflow flag
 * ------------------------------------------------------------------------- */
#define TEST_MUL(opc, op1, op2, res, cy, ov)				 \
	l.mfspr	r3,r0,SPR_SR						;\
	LOAD_CONST (r2, ~(SPR_SR_CY | SPR_SR_OV))			;\
	l.and	r3,r3,r2		/* Clear flags */		;\
	l.mtspr	r0,r3,SPR_SR						;\
									;\
	LOAD_CONST (r5,op1)		/* Load numbers to add */	;\
	LOAD_CONST (r6,op2)						;\
	l.mtspr	r0,r0,SPR_EPCR_BASE	/* Clear record */		;\
50:	opc	r4,r5,r6						;\
	l.mfspr	r2,r0,SPR_SR		/* So we can examine flags */	;\
	l.mfspr	r5,r0,SPR_EPCR_BASE	/* What triggered exception */	;\
	PUSH (r5)			/* Save EPCR for later */	;\
	PUSH (r2)			/* Save SR for later */		;\
	PUSH (r4)			/* Save result for later */	;\
									;\
	PUTS ("  0x")							;\
	PUTH (op1)							;\
	PUTS (" * 0x")							;\
	PUTH (op2)							;\
	PUTS (" = 0x")							;\
	PUTH (res)							;\
	PUTS (": ")							;\
	POP (r4)							;\
	CHECK_RES1 (r4, res)						;\
									;\
	POP (r2)			/* Retrieve SR */		;\
	PUSH (r2)							;\
	LOAD_CONST (r4, SPR_SR_CY)	/* The carry bit */		;\
	l.and	r2,r2,r4						;\
	l.sfeq	r2,r4							;\
	CHECK_FLAG ("- carry flag set:      ", cy)			;\
									;\
	POP (r2)			/* Retrieve SR */		;\
	LOAD_CONST (r4, SPR_SR_OV)	/* The overflow bit */		;\
	l.and	r2,r2,r4						;\
	l.sfeq	r2,r4							;\
	CHECK_FLAG ("- overflow flag set:   ", ov)			;\
									;\
	POP (r2)			/* Retrieve EPCR */		;\
	LOAD_CONST (r4, 50b)		/* The opcode of interest */	;\
	l.and	r2,r2,r4						;\
	l.sfeq	r2,r4							;\
	l.bnf	51f							;\
									;\
	PUTS ("  - exception triggered: TRUE\n")			;\
	l.j	52f							;\
	l.nop								;\
									;\
51:	PUTS ("  - exception triggered: FALSE\n")			;\
52:	
	
	
/* ----------------------------------------------------------------------------
 * A macro to carry out a test of multiply immediate
 *
 * Arguments
 *   op1:       First operand value
 *   op2:       Second operand value
 *   res:       Expected result
 *   cy:        Expected carry flag
 *   ov:        Expected overflow flag
 * ------------------------------------------------------------------------- */
#define TEST_MULI(op1, op2, res, cy, ov)				 \
	l.mfspr	r3,r0,SPR_SR						;\
	LOAD_CONST (r2, ~(SPR_SR_CY | SPR_SR_OV))			;\
	l.and	r3,r3,r2		/* Clear flags */		;\
	l.mtspr	r0,r3,SPR_SR						;\
									;\
	LOAD_CONST (r5,op1)		/* Load numbers to add */	;\
	l.mtspr	r0,r0,SPR_EPCR_BASE	/* Clear record */		;\
53:	l.muli	r4,r5,op2						;\
	l.mfspr	r2,r0,SPR_SR		/* So we can examine flags */	;\
	l.mfspr	r5,r0,SPR_EPCR_BASE	/* What triggered exception */	;\
	PUSH (r5)			/* Save EPCR for later */	;\
	PUSH (r2)			/* Save SR for later */		;\
	PUSH (r4)			/* Save result for later */	;\
									;\
	PUTS ("  0x")							;\
	PUTH (op1)							;\
	PUTS (" * 0x")							;\
	PUTHH (op2)							;\
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
	LOAD_CONST (r4, 53b)		/* The opcode of interest */	;\
	l.and	r2,r2,r4						;\
	l.sfeq	r2,r4							;\
	l.bnf	54f							;\
									;\
	PUTS ("  - exception triggered: TRUE\n")			;\
	l.j	55f							;\
	l.nop								;\
									;\
54:	PUTS ("  - exception triggered: FALSE\n")			;\
55:	
	

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
 * Test of multiply signed, l.mul
 * ------------------------------------------------------------------------- */
_mul:
	LOAD_STR (r3, "l.mul\n")
	l.jal	_puts
	l.nop

	/* Multiply two small positive numbers. Should set no flags. */
	TEST_MUL (l.mul, 0x00000002, 0x00000003,
	          0x00000006, FALSE, FALSE)

	/* Multiply two quite large positive numbers. Should set no flags */
	TEST_MUL (l.mul, 0x00008001, 0x0000fffe,
	          0x7ffffffe, FALSE, FALSE)

	/* Multiply two slightly too large positive numbers. Should set the
	   overflow, but not the carry flag */
	TEST_MUL (l.mul, 0x00008000, 0x00010000,
	          0x80000000, FALSE, TRUE)

	/* Multiply two large positive numbers. Should set both the carry and
	   overflow flags (even though the result is not a negative number. */
	TEST_MUL (l.mul, 0x00010000, 0x00010000, 0x00000000, TRUE, TRUE)

	/* Multiply two small negative numbers. Should set the overflow, but not
	   the carry flag. */
	TEST_MUL (l.mul, 0xfffffffe, 0xfffffffd,
	          0x00000006, TRUE, FALSE)

	/* Multiply two quite large negative numbers. Should set the overflow,
	   but not the carry flag. */
	TEST_MUL (l.mul, 0xffff7fff, 0xffff0002,
	          0x7ffffffe, TRUE, FALSE)

	/* Multiply two slightly too large negative numbers. Should set both the
	   overflow, and the carry flags */
	TEST_MUL (l.mul, 0xffff7fff, 0xffff0000,
	          0x80010000, TRUE, TRUE)

	/* Multiply two large negative numbers. Should set the
	   both the carry and overflow flags (even though the result is a
	   positive number. */
	TEST_MUL (l.mul, 0xffff0000, 0xfffeffff,
	          0x00010000, TRUE, TRUE)

	/* Multiply one small negative number and one small positive number.
	   Should set the overflow, but not the carry flag. */
	TEST_MUL (l.mul, 0x00000002, 0xfffffffd,
	          0xfffffffa, TRUE, FALSE)

	/* Multiply one quite large negative number and one quite large
	   positive number. Should set the overflow, but not the carry flag. */
	TEST_MUL (l.mul, 0xffff8000, 0x00010000,
	          0x80000000, TRUE, FALSE)

	/* Multiply one slightly too large negative number and one slightly
	   too large positive number. Should set both the carry and overflow
	   flags. */
	TEST_MUL (l.mul, 0xffff7fff, 0x00010000,
	          0x7fff0000, TRUE, TRUE)

	/* Multiply the largest negative number by positive unity. Should set
	   neither carry, nor overflow flag. */
	TEST_MUL (l.mul, 0x80000000, 0x00000001,
	          0x80000000, FALSE, FALSE)

	/* Check that range exceptions are triggered */
	l.mfspr	r3,r0,SPR_SR
	LOAD_CONST (r2, SPR_SR_OVE)	/* Set OVE */
	l.or	r3,r3,r2
	l.mtspr	r0,r3,SPR_SR
	
	LOAD_STR (r3, "  ** OVE flag set **\n")
	l.jal	_puts
	l.nop

	/* Check that an overflow alone causes a RANGE Exception. */
	TEST_MUL (l.mul, 0x00008000, 0x00010000,
	          0x80000000, FALSE, TRUE)

	/* Check that a carry alone does not cause a RANGE Exception. */
	TEST_MUL (l.mul, 0x00000002, 0xfffffffd,
	          0xfffffffa, TRUE, FALSE)

	/* Check that carry and overflow together cause an exception. */
	TEST_MUL (l.mul, 0xffff7fff, 0xffff0000,
	          0x80010000, TRUE, TRUE)

	/* Finished checking range exceptions */
	l.mfspr	r3,r0,SPR_SR
	LOAD_CONST (r2, ~SPR_SR_OVE)	/* Clear OVE */
	l.and	r3,r3,r2
	l.mtspr	r0,r3,SPR_SR
	
	LOAD_STR (r3, "  ** OVE flag cleared **\n")
	l.jal	_puts
	l.nop

/* ----------------------------------------------------------------------------
 * Test of multiply signed, l.muli
 * ------------------------------------------------------------------------- */
_muli:
	LOAD_STR (r3, "l.muli\n")
	l.jal	_puts
	l.nop

	/* Multiply two small positive numbers. Should set no flags. */
	TEST_MULI (0x00000002, 0x0003,
	          0x00000006, FALSE, FALSE)

	/* Multiply two quite large positive numbers. Should set no flags */
	TEST_MULI (0x00010002, 0x7fff,
	          0x7ffffffe, FALSE, FALSE)

	/* Multiply two slightly too large positive numbers. Should set the
	   overflow, but not the carry flag */
	TEST_MULI (0x00020000, 0x4000,
	          0x80000000, FALSE, TRUE)

	/* Multiply two large positive numbers. Should set both the carry and
	   overflow flags (even though the result is not a negative number. */
	TEST_MULI (0x00040000, 0x4000,
	           0x00000000, TRUE, TRUE)

	/* Multiply two small negative numbers. Should set the overflow, but not
	   the carry flag. */
	TEST_MULI (0xfffffffe, 0xfffd,
	          0x00000006, TRUE, FALSE)

	/* Multiply two quite large negative numbers. Should set the overflow,
	   but not the carry flag. */
	TEST_MULI (0xfffefffe, 0x8001,
	          0x7ffffffe, TRUE, FALSE)

	/* Multiply two slightly too large negative numbers. Should set both the
	   overflow, and the carry flags */
	TEST_MULI (0xfffe0000, 0xbfff,
	          0x80020000, TRUE, TRUE)

	/* Multiply two large negative numbers. Should set the
	   both the carry and overflow flags (even though the result is a
	   positive number. */
	TEST_MULI (0xfffdfffe, 0x8000,
	          0x00010000, TRUE, TRUE)

	/* Multiply one small negative number and one small positive number.
	   Should set the overflow, but not the carry flag. */
	TEST_MULI (0x00000002, 0xfffd,
	          0xfffffffa, TRUE, FALSE)

	/* Multiply one quite large negative number and one quite large
	   positive number. Should set the overflow, but not the carry flag. */
	TEST_MULI (0x00010000, 0x8000,
	          0x80000000, TRUE, FALSE)

	/* Multiply one slightly too large negative number and one slightly
	   too large positive number. Should set both the carry and overflow
	   flags. */
	TEST_MULI (0xfffdfffc, 0x4000,
	          0x7fff0000, TRUE, TRUE)

	/* Multiply the largest negative number by positive unity. Should set
	   neither carry, nor overflow flag. */
	TEST_MULI (0x80000000, 0x0001,
	          0x80000000, FALSE, FALSE)

	/* Check that range exceptions are triggered */
	l.mfspr	r3,r0,SPR_SR
	LOAD_CONST (r2, SPR_SR_OVE)	/* Set OVE */
	l.or	r3,r3,r2
	l.mtspr	r0,r3,SPR_SR
	
	LOAD_STR (r3, "  ** OVE flag set **\n")
	l.jal	_puts
	l.nop

	/* Check that an overflow alone causes a RANGE Exception. */
	TEST_MULI (0x00020000, 0x4000,
	          0x80000000, FALSE, TRUE)

	/* Check that a carry alone does not cause a RANGE Exception. */
	TEST_MULI (0xfffffffe, 0xfffd,
	          0x00000006, TRUE, FALSE)

	/* Check that carry and overflow together cause an exception. */
	TEST_MULI (0xfffdfffe, 0x8000,
	          0x00010000, TRUE, TRUE)

	/* Finished checking range exceptions */
	l.mfspr	r3,r0,SPR_SR
	LOAD_CONST (r2, ~SPR_SR_OVE)	/* Clear OVE */
	l.and	r3,r3,r2
	l.mtspr	r0,r3,SPR_SR
	
	LOAD_STR (r3, "  ** OVE flag cleared **\n")
	l.jal	_puts
	l.nop

/* ----------------------------------------------------------------------------
 * Test of multiply unsigned, l.mulu
 * ------------------------------------------------------------------------- */
_mulu:
	LOAD_STR (r3, "l.mulu\n")
	l.jal	_puts
	l.nop

	/* Multiply two small positive numbers. Should set no flags. */
	TEST_MUL (l.mulu, 0x00000002, 0x00000003,
	          0x00000006, FALSE, FALSE)

	/* Multiply two quite large positive numbers. Should set no flags */
	TEST_MUL (l.mulu, 0x00008001, 0x0000fffe,
	          0x7ffffffe, FALSE, FALSE)

	/* Multiply two slightly too large positive numbers. Should set the
	   overflow, but not the carry flag */
	TEST_MUL (l.mulu, 0x00008000, 0x00010000,
	          0x80000000, FALSE, FALSE)

	/* Multiply two large positive numbers. Should set both the carry and
	   overflow flags (even though the result is not a negative number. */
	TEST_MUL (l.mulu, 0x00010000, 0x00010000,
		  0x00000000, TRUE, FALSE)

	/* Multiply two small negative numbers. Should set the overflow, but not
	   the carry flag. */
	TEST_MUL (l.mulu, 0xfffffffe, 0xfffffffd,
	          0x00000006, TRUE, FALSE)

	/* Multiply two quite large negative numbers. Should set the overflow,
	   but not the carry flag. */
	TEST_MUL (l.mulu, 0xffff7fff, 0xffff0002,
	          0x7ffffffe, TRUE, FALSE)

	/* Multiply two slightly too large negative numbers. Should set both the
	   overflow, and the carry flags */
	TEST_MUL (l.mulu, 0xffff7fff, 0xffff0000,
	          0x80010000, TRUE, FALSE)

	/* Multiply two large negative numbers. Should set the
	   both the carry and overflow flags (even though the result is a
	   positive number. */
	TEST_MUL (l.mulu, 0xffff0000, 0xfffeffff,
	          0x00010000, TRUE, FALSE)

	/* Multiply one small negative number and one small positive number.
	   Should set the overflow, but not the carry flag. */
	TEST_MUL (l.mulu, 0x00000002, 0xfffffffd,
	          0xfffffffa, TRUE, FALSE)

	/* Multiply one quite large negative number and one quite large
	   positive number. Should set the overflow, but not the carry flag. */
	TEST_MUL (l.mulu, 0xffff8000, 0x00010000,
	          0x80000000, TRUE, FALSE)

	/* Multiply one slightly too large negative number and one slightly
	   too large positive number. Should set both the carry and overflow
	   flags. */
	TEST_MUL (l.mulu, 0xffff7fff, 0x00010000,
	          0x7fff0000, TRUE, FALSE)

	/* Multiply the largest negative number by positive unity. Should set
	   neither carry, nor overflow flag. */
	TEST_MUL (l.mulu, 0x80000000, 0x00000001,
	          0x80000000, FALSE, FALSE)

	/* Check that range exceptions are never triggered */
	l.mfspr	r3,r0,SPR_SR
	LOAD_CONST (r2, SPR_SR_OVE)	/* Set OVE */
	l.or	r3,r3,r2
	l.mtspr	r0,r3,SPR_SR
	
	LOAD_STR (r3, "  ** OVE flag set **\n")
	l.jal	_puts
	l.nop

	/* Check that what would cause an overflow alone in 2's complement does
	   not cause a RANGE Exception. */
	TEST_MUL (l.mulu, 0x00008000, 0x00010000,
	          0x80000000, FALSE, FALSE)

	/* Check that a carry alone does not cause a RANGE Exception. */
	TEST_MUL (l.mulu, 0x00000002, 0xfffffffd,
	          0xfffffffa, TRUE, FALSE)

	/* Check that what would cause an overflow and carry in 2's complement
	   does not cause a RANGE Exception. */
	TEST_MUL (l.mulu, 0xffff7fff, 0xffff0000,
	          0x80010000, TRUE, FALSE)

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
