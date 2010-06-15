/* is-or-test.S. l.or and l.ori instruction test of Or1ksim
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
 * The l.or and l.ori instructions should never set the carry and overflow
 * flags.
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
 * A macro to carry out a test of bitwise OR in registers
 *
 * This opcode should never set the flags. Result is compared with the native
 * computed value.
 *
 * Arguments
 *   op1:       First operand value
 *   op2:       Second operand value
 * ------------------------------------------------------------------------- */
#define TEST_OR(op1, op2)						 \
	l.mfspr	r3,r0,SPR_SR		/* Clear flags */		;\
	LOAD_CONST (r2, ~(SPR_SR_CY | SPR_SR_OV))			;\
	l.and	r3,r3,r2						;\
	l.mtspr	r0,r3,SPR_SR						;\
									;\
	LOAD_CONST (r5,op1)		/* Load operands */		;\
	LOAD_CONST (r6,op2)						;\
	l.mtspr	r0,r0,SPR_EPCR_BASE	/* Clear record */		;\
50:	l.or	r4,r5,r6						;\
	l.mfspr	r2,r0,SPR_SR		/* So we can examine flags */	;\
	l.mfspr	r5,r0,SPR_EPCR_BASE	/* What triggered exception */	;\
	PUSH (r5)			/* Save EPCR for later */	;\
	PUSH (r2)			/* Save SR for later */		;\
	PUSH (r4)			/* Save result for later */	;\
									;\
	PUTS ("  0x")							;\
	PUTH (op1)							;\
	PUTS (" | 0x")							;\
	PUTH (op2)							;\
	PUTS (" = 0x")							;\
	PUTH (op1 | op2)						;\
	PUTS (": ")							;\
	POP (r4)							;\
	CHECK_RES1 (r4, op1 | op2)					;\
									;\
	POP(r2)				/* Retrieve SR */		;\
	PUSH(r2)							;\
	LOAD_CONST (r4, SPR_SR_CY)	/* The carry bit */		;\
	l.and	r2,r2,r4						;\
	l.sfeq	r2,r4							;\
	CHECK_FLAG ("- carry flag set:      ", FALSE)			;\
									;\
	POP(r2)				/* Retrieve SR */		;\
	LOAD_CONST (r4, SPR_SR_OV)	/* The overflow bit */		;\
	l.and	r2,r2,r4						;\
	l.sfeq	r2,r4							;\
	CHECK_FLAG ("- overflow flag set:   ", FALSE)			;\
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
 * A macro to carry out a test of bitwise OR with an immediate operand
 *
 * This opcode should never set the flags. Result is compared with the native
 * computed value.
 *
 * Arguments
 *   op1:       First operand value
 *   op2:       Second operand value
 * ------------------------------------------------------------------------- */
#define TEST_ORI(op1, op2)						 \
	l.mfspr	r3,r0,SPR_SR		/* Clear flags */		;\
	LOAD_CONST (r2, ~(SPR_SR_CY | SPR_SR_OV))			;\
	l.and	r3,r3,r2						;\
	l.mtspr	r0,r3,SPR_SR						;\
									;\
	LOAD_CONST (r5,op1)		/* Load operands */		;\
	l.mtspr	r0,r0,SPR_EPCR_BASE	/* Clear record */		;\
53:	l.ori	r4,r5,op2						;\
	l.mfspr	r2,r0,SPR_SR		/* So we can examine flags */	;\
	l.mfspr	r5,r0,SPR_EPCR_BASE	/* What triggered exception */	;\
	PUSH (r5)			/* Save EPCR for later */	;\
	PUSH (r2)			/* Save SR for later */		;\
	PUSH (r4)			/* Save result for later */	;\
									;\
	PUTS ("  0x")							;\
	PUTH (op1)							;\
	PUTS (" | 0x")							;\
	PUTHH (op2)							;\
	PUTS (" = 0x")							;\
	PUTH (op1 | op2)						;\
	PUTS (": ")							;\
	POP (r4)							;\
	CHECK_RES1 (r4, op1 | op2)					;\
									;\
	POP(r2)				/* Retrieve SR */		;\
	PUSH(r2)							;\
	LOAD_CONST (r4, SPR_SR_CY)	/* The carry bit */		;\
	l.and	r2,r2,r4						;\
	l.sfeq	r2,r4							;\
	CHECK_FLAG ("- carry flag set:      ", FALSE)			;\
									;\
	POP(r2)				/* Retrieve SR */		;\
	LOAD_CONST (r4, SPR_SR_OV)	/* The overflow bit */		;\
	l.and	r2,r2,r4						;\
	l.sfeq	r2,r4							;\
	CHECK_FLAG ("- overflow flag set:   ", FALSE)			;\
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
	/* Always set OVE. We should never trigger an exception, even if this
	   bit is set. */
	l.mfspr	r3,r0,SPR_SR
	LOAD_CONST (r2, SPR_SR_OVE)	/* Set OVE */
	l.or	r3,r3,r2
	l.mtspr	r0,r3,SPR_SR
	
	LOAD_STR (r3, "  ** OVE flag set **\n")
	l.jal	_puts
	l.nop

/* ----------------------------------------------------------------------------
 * Test of or, l.or
 * ------------------------------------------------------------------------- */
_or:
	LOAD_STR (r3, "l.or\n")
	l.jal	_puts
	l.nop

	/* Test a range of operands */
	TEST_OR (0x00000000, 0x00000000)
	TEST_OR (0xffffffff, 0xffffffff)
	TEST_OR (0xaaaaaaaa, 0x00000000)
	TEST_OR (0xaaaaaaaa, 0xaaaaaaaa)
	TEST_OR (0x55555555, 0x00000000)
	TEST_OR (0x55555555, 0x55555555)
	TEST_OR (0xaaaaaaaa, 0x55555555)
	TEST_OR (0x4c70f07c, 0xb38f0f83)
	TEST_OR (0x4c70f07c, 0xc4c70f07)
	TEST_OR (0xb38f0f83, 0x38f0f83b)

/* ----------------------------------------------------------------------------
 * Test of or with immediate half word, l.ori
 * ------------------------------------------------------------------------- */
_ori:
	LOAD_STR (r3, "l.ori\n")
	l.jal	_puts
	l.nop

	/* Test a range of operands */
	TEST_ORI (0x00000000, 0x0000)
	TEST_ORI (0xffffffff, 0xffff)
	TEST_ORI (0xaaaaaaaa, 0x0000)
	TEST_ORI (0xaaaaaaaa, 0xaaaa)
	TEST_ORI (0x55555555, 0x0000)
	TEST_ORI (0x55555555, 0x5555)
	TEST_ORI (0xaaaaaaaa, 0x5555)
	TEST_ORI (0x4c70f07c, 0x0f83)
	TEST_ORI (0x4c70f07c, 0x0f07)
	TEST_ORI (0xb38f0f83, 0xf83b)

/* ----------------------------------------------------------------------------
 * All done
 * ------------------------------------------------------------------------- */
_exit:
	LOAD_STR (r3, "Test completed\n")
	l.jal	_puts
	l.nop

	TEST_EXIT
