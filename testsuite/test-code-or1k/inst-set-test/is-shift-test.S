/* is-shift-test.S. shift instructions test of Or1ksim
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
 * The shift instructions should never set the carry and overflow flags.
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
 * A macro to carry out a test of shift in registers
 *
 * This opcode should never set the flags.
 *
 * Arguments
 *   opc:       The operand
 *   op1:       First operand value
 *   op2:       Second operand value
 *   res:       The expected result
 * ------------------------------------------------------------------------- */
#define TEST_SHIFT(opc, op1, op2, res)					 \
	l.mfspr	r3,r0,SPR_SR		/* Clear flags */		;\
	LOAD_CONST (r2, ~(SPR_SR_CY | SPR_SR_OV))			;\
	l.and	r3,r3,r2						;\
	l.mtspr	r0,r3,SPR_SR						;\
									;\
	LOAD_CONST (r5,op1)		/* Load operands */		;\
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
	PUTS (" shifted by 0x")						;\
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
 * A macro to carry out a test of shift with an immediate operand
 *
 * This opcode should never set the flags.
 *
 * Arguments
 *   opc:       The operand
 *   op1:       First operand value
 *   op2:       Second operand value
 *   res:       The expected result
 * ------------------------------------------------------------------------- */
#define TEST_SHIFTI(opc, op1, op2, res)					 \
	l.mfspr	r3,r0,SPR_SR		/* Clear flags */		;\
	LOAD_CONST (r2, ~(SPR_SR_CY | SPR_SR_OV))			;\
	l.and	r3,r3,r2						;\
	l.mtspr	r0,r3,SPR_SR						;\
									;\
	LOAD_CONST (r5,op1)		/* Load operands */		;\
	l.mtspr	r0,r0,SPR_EPCR_BASE	/* Clear record */		;\
53:	opc	r4,r5,op2						;\
	l.mfspr	r2,r0,SPR_SR		/* So we can examine flags */	;\
	l.mfspr	r5,r0,SPR_EPCR_BASE	/* What triggered exception */	;\
	PUSH (r5)			/* Save EPCR for later */	;\
	PUSH (r2)			/* Save SR for later */		;\
	PUSH (r4)			/* Save result for later */	;\
									;\
	PUTS ("  0x")							;\
	PUTH (op1)							;\
	PUTS (" shifted by 0x")						;\
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
 * Test of shift left logical, l.sll
 * ------------------------------------------------------------------------- */
_sll:
	LOAD_STR (r3, "l.sll\n")
	l.jal	_puts
	l.nop

	/* Shift left by zero. */
	TEST_SHIFT (l.sll, 0xb38f0f83, 0x00000000, 0xb38f0f83)

	/* Shift left by amounts in the 1-31 range */
	TEST_SHIFT (l.sll, 0xb38f0f83, 0x00000001, 0x671e1f06)
	TEST_SHIFT (l.sll, 0xb38f0f83, 0x00000004, 0x38f0f830)
	TEST_SHIFT (l.sll, 0xb38f0f83, 0x00000010, 0x0f830000)
	TEST_SHIFT (l.sll, 0xb38f0f83, 0x0000001f, 0x80000000)

	/* Shift left by larger amounts - should be masked. */
	TEST_SHIFT (l.sll, 0xb38f0f83, 0x00000021, 0x671e1f06)
	TEST_SHIFT (l.sll, 0xb38f0f83, 0x00002224, 0x38f0f830)
	TEST_SHIFT (l.sll, 0xb38f0f83, 0x00f789f0, 0x0f830000)
	TEST_SHIFT (l.sll, 0xb38f0f83, 0xffffffff, 0x80000000)

/* ----------------------------------------------------------------------------
 * Test of shift left logical with immediate, l.slli
 * ------------------------------------------------------------------------- */
_slli:
	LOAD_STR (r3, "l.slli\n")
	l.jal	_puts
	l.nop

	/* Shift left by zero. */
	TEST_SHIFTI (l.slli, 0xb38f0f83, 0x0000, 0xb38f0f83)

	/* Shift left by amounts in the 1-31 range */
	TEST_SHIFTI (l.slli, 0xb38f0f83, 0x0001, 0x671e1f06)
	TEST_SHIFTI (l.slli, 0xb38f0f83, 0x0004, 0x38f0f830)
	TEST_SHIFTI (l.slli, 0xb38f0f83, 0x0010, 0x0f830000)
	TEST_SHIFTI (l.slli, 0xb38f0f83, 0x001f, 0x80000000)

	/* Shift left by larger amounts - should be masked. */
	TEST_SHIFTI (l.slli, 0xb38f0f83, 0x0021, 0x671e1f06)
	TEST_SHIFTI (l.slli, 0xb38f0f83, 0x0024, 0x38f0f830)
	TEST_SHIFTI (l.slli, 0xb38f0f83, 0x0030, 0x0f830000)
	TEST_SHIFTI (l.slli, 0xb38f0f83, 0x003f, 0x80000000)

/* ----------------------------------------------------------------------------
 * Test of shift right arithmetic, l.sra
 * ------------------------------------------------------------------------- */
_sra:
	LOAD_STR (r3, "l.sra\n")
	l.jal	_puts
	l.nop

	/* Shift right by zero. */
	TEST_SHIFT (l.sra, 0xb38f0f83, 0x00000000, 0xb38f0f83)

	/* Shift right by amounts in the 1-31 range */
	TEST_SHIFT (l.sra, 0xb38f0f83, 0x00000001, 0xd9c787c1)
	TEST_SHIFT (l.sra, 0xb38f0f83, 0x00000004, 0xfb38f0f8)
	TEST_SHIFT (l.sra, 0xb38f0f83, 0x00000010, 0xffffb38f)
	TEST_SHIFT (l.sra, 0xb38f0f83, 0x0000001f, 0xffffffff)

	TEST_SHIFT (l.sra, 0x4c70f07c, 0x00000001, 0x2638783e)
	TEST_SHIFT (l.sra, 0x4c70f07c, 0x00000004, 0x04c70f07)
	TEST_SHIFT (l.sra, 0x4c70f07c, 0x00000010, 0x00004c70)
	TEST_SHIFT (l.sra, 0x4c70f07c, 0x0000001f, 0x00000000)

	/* Shift right by larger amounts - should be masked. */
	TEST_SHIFT (l.sra, 0xb38f0f83, 0x00000021, 0xd9c787c1)
	TEST_SHIFT (l.sra, 0xb38f0f83, 0x00002224, 0xfb38f0f8)
	TEST_SHIFT (l.sra, 0xb38f0f83, 0x00f789f0, 0xffffb38f)
	TEST_SHIFT (l.sra, 0xb38f0f83, 0xffffffff, 0xffffffff)

	TEST_SHIFT (l.sra, 0x4c70f07c, 0x00000021, 0x2638783e)
	TEST_SHIFT (l.sra, 0x4c70f07c, 0x00002224, 0x04c70f07)
	TEST_SHIFT (l.sra, 0x4c70f07c, 0x00f789f0, 0x00004c70)
	TEST_SHIFT (l.sra, 0x4c70f07c, 0xffffffff, 0x00000000)


/* ----------------------------------------------------------------------------
 * Test of shift right arithmetic with immediate, l.srai
 * ------------------------------------------------------------------------- */
_srai:
	LOAD_STR (r3, "l.srai\n")
	l.jal	_puts
	l.nop

	/* Shift right by zero. */
	TEST_SHIFTI (l.srai, 0xb38f0f83, 0x0000, 0xb38f0f83)

	/* Shift right by amounts in the 1-31 range */
	TEST_SHIFTI (l.srai, 0xb38f0f83, 0x0001, 0xd9c787c1)
	TEST_SHIFTI (l.srai, 0xb38f0f83, 0x0004, 0xfb38f0f8)
	TEST_SHIFTI (l.srai, 0xb38f0f83, 0x0010, 0xffffb38f)
	TEST_SHIFTI (l.srai, 0xb38f0f83, 0x001f, 0xffffffff)

	TEST_SHIFTI (l.srai, 0x4c70f07c, 0x0001, 0x2638783e)
	TEST_SHIFTI (l.srai, 0x4c70f07c, 0x0004, 0x04c70f07)
	TEST_SHIFTI (l.srai, 0x4c70f07c, 0x0010, 0x00004c70)
	TEST_SHIFTI (l.srai, 0x4c70f07c, 0x001f, 0x00000000)

	/* Shift right by larger amounts - should be masked. */
	TEST_SHIFTI (l.srai, 0xb38f0f83, 0x0021, 0xd9c787c1)
	TEST_SHIFTI (l.srai, 0xb38f0f83, 0x0024, 0xfb38f0f8)
	TEST_SHIFTI (l.srai, 0xb38f0f83, 0x0030, 0xffffb38f)
	TEST_SHIFTI (l.srai, 0xb38f0f83, 0x003f, 0xffffffff)

	TEST_SHIFTI (l.srai, 0x4c70f07c, 0x0021, 0x2638783e)
	TEST_SHIFTI (l.srai, 0x4c70f07c, 0x0024, 0x04c70f07)
	TEST_SHIFTI (l.srai, 0x4c70f07c, 0x0030, 0x00004c70)
	TEST_SHIFTI (l.srai, 0x4c70f07c, 0x003f, 0x00000000)

/* ----------------------------------------------------------------------------
 * Test of shift right logical, l.srl
 * ------------------------------------------------------------------------- */
_srl:
	LOAD_STR (r3, "l.srl\n")
	l.jal	_puts
	l.nop

	/* Shift right by zero. */
	TEST_SHIFT (l.srl, 0xb38f0f83, 0x00000000, 0xb38f0f83)

	/* Shift right by amounts in the 1-31 range */
	TEST_SHIFT (l.srl, 0xb38f0f83, 0x00000001, 0x59c787c1)
	TEST_SHIFT (l.srl, 0xb38f0f83, 0x00000004, 0x0b38f0f8)
	TEST_SHIFT (l.srl, 0xb38f0f83, 0x00000010, 0x0000b38f)
	TEST_SHIFT (l.srl, 0xb38f0f83, 0x0000001f, 0x00000001)

	TEST_SHIFT (l.srl, 0x4c70f07c, 0x00000001, 0x2638783e)
	TEST_SHIFT (l.srl, 0x4c70f07c, 0x00000004, 0x04c70f07)
	TEST_SHIFT (l.srl, 0x4c70f07c, 0x00000010, 0x00004c70)
	TEST_SHIFT (l.srl, 0x4c70f07c, 0x0000001f, 0x00000000)

	/* Shift right by larger amounts - should be masked. */
	TEST_SHIFT (l.srl, 0xb38f0f83, 0x00000021, 0x59c787c1)
	TEST_SHIFT (l.srl, 0xb38f0f83, 0x00002224, 0x0b38f0f8)
	TEST_SHIFT (l.srl, 0xb38f0f83, 0x00f789f0, 0x0000b38f)
	TEST_SHIFT (l.srl, 0xb38f0f83, 0xffffffff, 0x00000001)

	TEST_SHIFT (l.srl, 0x4c70f07c, 0x00000021, 0x2638783e)
	TEST_SHIFT (l.srl, 0x4c70f07c, 0x00002224, 0x04c70f07)
	TEST_SHIFT (l.srl, 0x4c70f07c, 0x00f789f0, 0x00004c70)
	TEST_SHIFT (l.srl, 0x4c70f07c, 0xffffffff, 0x00000000)


/* ----------------------------------------------------------------------------
 * Test of shift right logical with immediate, l.srli
 * ------------------------------------------------------------------------- */
_srli:
	LOAD_STR (r3, "l.srli\n")
	l.jal	_puts
	l.nop

	/* Shift right by zero. */
	TEST_SHIFTI (l.srli, 0xb38f0f83, 0x0000, 0xb38f0f83)

	/* Shift right by amounts in the 1-31 range */
	TEST_SHIFTI (l.srli, 0xb38f0f83, 0x0001, 0x59c787c1)
	TEST_SHIFTI (l.srli, 0xb38f0f83, 0x0004, 0x0b38f0f8)
	TEST_SHIFTI (l.srli, 0xb38f0f83, 0x0010, 0x0000b38f)
	TEST_SHIFTI (l.srli, 0xb38f0f83, 0x001f, 0x00000001)

	TEST_SHIFTI (l.srli, 0x4c70f07c, 0x0001, 0x2638783e)
	TEST_SHIFTI (l.srli, 0x4c70f07c, 0x0004, 0x04c70f07)
	TEST_SHIFTI (l.srli, 0x4c70f07c, 0x0010, 0x00004c70)
	TEST_SHIFTI (l.srli, 0x4c70f07c, 0x001f, 0x00000000)

	/* Shift right by larger amounts - should be masked. */
	TEST_SHIFTI (l.srli, 0xb38f0f83, 0x0021, 0x59c787c1)
	TEST_SHIFTI (l.srli, 0xb38f0f83, 0x0024, 0x0b38f0f8)
	TEST_SHIFTI (l.srli, 0xb38f0f83, 0x0030, 0x0000b38f)
	TEST_SHIFTI (l.srli, 0xb38f0f83, 0x003f, 0x00000001)

	TEST_SHIFTI (l.srli, 0x4c70f07c, 0x0021, 0x2638783e)
	TEST_SHIFTI (l.srli, 0x4c70f07c, 0x0024, 0x04c70f07)
	TEST_SHIFTI (l.srli, 0x4c70f07c, 0x0030, 0x00004c70)
	TEST_SHIFTI (l.srli, 0x4c70f07c, 0x003f, 0x00000000)

/* ----------------------------------------------------------------------------
 * All done
 * ------------------------------------------------------------------------- */
_exit:
	LOAD_STR (r3, "Test completed\n")
	l.jal	_puts
	l.nop

	TEST_EXIT
