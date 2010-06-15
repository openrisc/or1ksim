/* is-spr-test.S. l.mfspr and l.mtspr instruction test of Or1ksim
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
 * The l.mfspr and l.mtspr should OR the immdediate operand with the register
 * to determine the SPR address, not add it (Bug 1779).
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
 * A macro to carry out a test of l.mfspr
 *
 * MACLO is used as the SPR, since it can be read and cleared using l.macrc
 * and can be set using l.maci. op1 and op2 should be chosen to address this
 * register.
 *
 * The value placed in the register is entirely arbitrary - we use 0xdeadbeef.
 *
 * Arguments
 *   op1:       First l.mfspr operand value
 *   op2:       Second l.mfspr operand value
 * ------------------------------------------------------------------------- */
#define TEST_MFSPR(op1, op2)						 \
	l.macrc	r2							;\
	LOAD_CONST (r2,0xdeadbeef)					;\
	l.maci	r2,1							;\
									;\
	l.mfspr	r3,r0,SPR_SR						;\
	LOAD_CONST (r2, ~(SPR_SR_CY | SPR_SR_OV))			;\
	l.and	r3,r3,r2		/* Clear flags */		;\
	l.mtspr	r0,r3,SPR_SR						;\
									;\
	LOAD_CONST (r5,op1)		/* First operand in register */	;\
	l.mtspr	r0,r0,SPR_EPCR_BASE	/* Clear record */		;\
50:	l.mfspr	r4,r5,op2						;\
	l.mfspr	r2,r0,SPR_EPCR_BASE	/* What triggered exception */	;\
	PUSH (r2)			/* Save EPCR for later */	;\
	PUSH (r4)			/* Save result for later */	;\
									;\
	PUTS ("  l.mfspr  0x")						;\
	PUTH (op1)							;\
	PUTS (" | 0x")							;\
	PUTHH (op2)							;\
	PUTS (": ")							;\
	POP (r4)							;\
	CHECK_RES1 (r4, 0xdeadbeef)					;\
									;\
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
 * A macro to carry out a test of l.mtspr
 *
 * MACLO is used as the SPR, since it can be read and cleared using l.macrc.
 * op1 and op2 should be chosen to address this register.
 *
 * The value placed in the register is entirely arbitrary - we use 0xdeadbeef.
 *
 * Arguments
 *   op1:       First l.mfspr operand value
 *   op2:       Second l.mfspr operand value
 * ------------------------------------------------------------------------- */
#define TEST_MTSPR(op1, op2)						 \
	l.macrc	r2							;\
									;\
	l.mfspr	r3,r0,SPR_SR						;\
	LOAD_CONST (r2, ~(SPR_SR_CY | SPR_SR_OV))			;\
	l.and	r3,r3,r2		/* Clear flags */		;\
	l.mtspr	r0,r3,SPR_SR						;\
									;\
	LOAD_CONST (r5,op1)		/* First operand in register */	;\
	LOAD_CONST (r4,0xdeadbeef)	/* First operand in register */	;\
	l.mtspr	r0,r0,SPR_EPCR_BASE	/* Clear record */		;\
50:	l.mtspr	r5,r4,op2						;\
	l.mfspr	r2,r0,SPR_EPCR_BASE	/* What triggered exception */	;\
	PUSH (r2)			/* Save EPCR for later */	;\
	l.macrc	r4							;\
	PUSH (r4)			/* Save result for later */	;\
									;\
	PUTS ("  l.mtspr  0x")						;\
	PUTH (op1)							;\
	PUTS (" | 0x")							;\
	PUTHH (op2)							;\
	PUTS (": ")							;\
	POP (r4)							;\
	CHECK_RES1 (r4, 0xdeadbeef)					;\
									;\
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
 * Start of code
 * ------------------------------------------------------------------------- */
	.section .text
	.global	_start
_start:

/* ----------------------------------------------------------------------------
 * Test of move from SPR, l.mfspr
 *
 * MACLO (0x2801) is always used as the test register.
 * ------------------------------------------------------------------------- */
_mfspr:	
	LOAD_STR (r3, "l.mfspr\n")
	l.jal	_puts
	l.nop

	/* Move a test value using zero in the register */
	TEST_MFSPR (0x00000000, 0x2801)

	/* Move a test value using zero as the constant */
	TEST_MFSPR (0x00002801, 0x0000)

	/* Move a test value using non-zero in both register and constant.
	   Some of these values will not give the correct result if OR rather
	   than ADD is used to determine the SPR address */
	TEST_MFSPR (0x00002801, 0x2801)
	TEST_MFSPR (0x00000801, 0x2000)
	TEST_MFSPR (0x00002000, 0x0801)
	TEST_MFSPR (0x00002801, 0x0001)
	TEST_MFSPR (0x00000800, 0x2801)

/* ----------------------------------------------------------------------------
 * Test of move to SPR, l.mtspr
 *
 * MACLO (0x2801) is always used as the test register.
 * ------------------------------------------------------------------------- */
_mtspr:	
	LOAD_STR (r3, "l.mtspr\n")
	l.jal	_puts
	l.nop

	/* Move a test value using zero in the register */
	TEST_MTSPR (0x00000000, 0x2801)

	/* Move a test value using zero as the constant */
	TEST_MTSPR (0x00002801, 0x0000)

	/* Move a test value using non-zero in both register and constant.
	   Some of these values will not give the correct result if OR rather
	   than ADD is used to determine the SPR address */
	TEST_MTSPR (0x00002801, 0x2801)
	TEST_MTSPR (0x00000801, 0x2000)
	TEST_MTSPR (0x00002000, 0x0801)
	TEST_MTSPR (0x00002801, 0x0001)
	TEST_MTSPR (0x00000800, 0x2801)

/* ----------------------------------------------------------------------------
 * All done
 * ------------------------------------------------------------------------- */
_exit:
	LOAD_STR (r3, "Test completed\n")
	l.jal	_puts
	l.nop

	TEST_EXIT
