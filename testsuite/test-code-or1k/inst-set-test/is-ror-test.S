/* is-div-test.S. l.div and l.divu instruction test of Or1ksim
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
 * The l.ror and l.rori instructions were missing from Or1ksim.
 *
 * Having fixed the problem, this is (in good software engineering style), a
 * regresison test to go with the fix.
 *
 * This is not a comprehensive test of either instruction (yet).
 *
 * Of course what is really needed is a comprehensive instruction test...
 * ------------------------------------------------------------------------- */


#include "inst-set-test.h"

/* ----------------------------------------------------------------------------
 * A macro to carry out a test of rotate right
 *
 * Arguments
 *   op1:       First operand value
 *   op2:       Second operand value
 *   res:       Expected result
 * ------------------------------------------------------------------------- */
#define TEST_ROR(op1, op2, res)						 \
	LOAD_CONST (r5,op1)		/* Load numbers to rotate */	;\
	LOAD_CONST (r6,op2)						;\
	l.mtspr	r0,r0,SPR_EPCR_BASE	/* Clear record */		;\
50:	l.ror	r4,r5,r6						;\
	l.mfspr	r5,r0,SPR_EPCR_BASE	/* What triggered exception */	;\
	PUSH (r5)			/* Save EPCR for later */	;\
	PUSH (r4)			/* Save result for later */	;\
									;\
	PUTS ("  0x")							;\
	PUTH (op1)							;\
	PUTS (" ROR 0x")						;\
	PUTH (op2)							;\
	PUTS (" = 0x")							;\
	PUTH (res)							;\
	PUTS (": ")							;\
	POP (r4)							;\
	CHECK_RES1 (r4, res)						;\
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
 * A macro to carry out a test of rotate right immediate
 *
 * Arguments
 *   op1:       First operand value
 *   op2:       Second operand value
 *   res:       Expected result
 * ------------------------------------------------------------------------- */
#define TEST_RORI(op1, op2, res)					 \
	LOAD_CONST (r5,op1)		/* Load numbers to rotate */	;\
	l.mtspr	r0,r0,SPR_EPCR_BASE	/* Clear record */		;\
53:	l.rori	r4,r5,op2						;\
	l.mfspr	r5,r0,SPR_EPCR_BASE	/* What triggered exception */	;\
	PUSH (r5)			/* Save EPCR for later */	;\
	PUSH (r4)			/* Save result for later */	;\
									;\
	PUTS ("  0x")							;\
	PUTH (op1)							;\
	PUTS (" RORI 0x")						;\
	PUTHQ (op2)							;\
	PUTS (" = 0x")							;\
	PUTH (res)							;\
	PUTS (": ")							;\
	POP (r4)							;\
	CHECK_RES1 (r4, res)						;\
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

/* ----------------------------------------------------------------------------
 * Test of rotate right, l.ror
 * ------------------------------------------------------------------------- */
_ror:	
	LOAD_STR (r3, "l.ror\n")
	l.jal	_puts
	l.nop

	/* Rotate by zero */
	TEST_ROR (0xb38f0f83, 0x00000000, 0xb38f0f83)

	/* Rotate by amounts in the 1 - 31 range. */
	TEST_ROR (0xb38f0f83, 0x00000001, 0xd9c787c1)
	TEST_ROR (0xb38f0f83, 0x00000004, 0x3b38f0f8)
	TEST_ROR (0xb38f0f83, 0x00000010, 0x0f83b38f)
	TEST_ROR (0xb38f0f83, 0x0000001f, 0x671e1f07)

	/* Rotate by larger amounts - should be masked. */
	TEST_ROR (0xb38f0f83, 0x00000021, 0xd9c787c1)
	TEST_ROR (0xb38f0f83, 0x00002224, 0x3b38f0f8)
	TEST_ROR (0xb38f0f83, 0x00f789f0, 0x0f83b38f)
	TEST_ROR (0xb38f0f83, 0xffffffff, 0x671e1f07)
	

/* ----------------------------------------------------------------------------
 * Test of rotate right immediate, l.rori
 * ------------------------------------------------------------------------- */
_rori:	
	LOAD_STR (r3, "l.rori\n")
	l.jal	_puts
	l.nop

	/* Rotate by zero */
	TEST_RORI (0xb38f0f83, 0x00000000, 0xb38f0f83)

	/* Rotate by amounts in the 1 - 31 range. */
	TEST_RORI (0xb38f0f83, 0x01, 0xd9c787c1)
	TEST_RORI (0xb38f0f83, 0x04, 0x3b38f0f8)
	TEST_RORI (0xb38f0f83, 0x10, 0x0f83b38f)
	TEST_RORI (0xb38f0f83, 0x1f, 0x671e1f07)

	/* Rotate by larger amounts (32 - 63) - should be masked. */
	TEST_RORI (0xb38f0f83, 0x21, 0xd9c787c1)
	TEST_RORI (0xb38f0f83, 0x24, 0x3b38f0f8)
	TEST_RORI (0xb38f0f83, 0x30, 0x0f83b38f)
	TEST_RORI (0xb38f0f83, 0x3f, 0x671e1f07)
	

/* ----------------------------------------------------------------------------
 * All done
 * ------------------------------------------------------------------------- */
_exit:
	LOAD_STR (r3, "Test completed\n")
	l.jal	_puts
	l.nop

	TEST_EXIT
