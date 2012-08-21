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
 * The l.div and l.divu instructions should set the carry flag as well as
 * triggering an event when divide by zero occurs.
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
 * A macro to carry out a test of divide signed or unsigned
 *
 * Arguments
 *   opc:       The opcode
 *   op1:       First operand value
 *   op2:       Second operand value
 *   res:       Expected result
 *   cy:        Expected carry flag
 *   ov:        Expected overflow flag
 * ------------------------------------------------------------------------- */
#define TEST_DIV(opc, op1, op2, res, cy, ov)				 \
	l.mfspr	r3,r0,SPR_SR						;\
	LOAD_CONST (r2, ~(SPR_SR_CY | SPR_SR_OV))			;\
	l.and	r3,r3,r2		/* Clear flags */		;\
	l.mtspr	r0,r3,SPR_SR						;\
									;\
	l.or	r4,r0,r0		/* Clear result reg */		;\
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
	PUTS (" / 0x")							;\
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
 * Test of divide signed, l.div
 * ------------------------------------------------------------------------- */
_div:	
	LOAD_STR (r3, "l.div\n")
	l.jal	_puts
	l.nop

	/* Divide two positive numbers and check rounding. Should set no
	   flags. */
	TEST_DIV (l.div, 0x0000000c, 0x00000003,
	          0x00000004, FALSE, FALSE)

	TEST_DIV (l.div, 0x0000000b, 0x00000003,
	          0x00000003, FALSE, FALSE)

	/* Divide two negative numbers and check rounding. Should set no
	   flags. */
	TEST_DIV (l.div, 0xfffffff4, 0xfffffffd,
	          0x00000004, FALSE, FALSE)

	TEST_DIV (l.div, 0xfffffff5, 0xfffffffd,
	          0x00000003, FALSE, FALSE)

	/* Divide a negative number by a positive number and check
	   rounding. Should set no flags. */
	TEST_DIV (l.div, 0xfffffff4, 0x00000003,
	          0xfffffffc, FALSE, FALSE)

	TEST_DIV (l.div, 0xfffffff5, 0x00000003,
	          0xfffffffd, FALSE, FALSE)

	/* Divide a positive number by a negative number and check
	   rounding. Should set no flags. */
	TEST_DIV (l.div, 0x0000000c, 0xfffffffd,
	          0xfffffffc, FALSE, FALSE)

	TEST_DIV (l.div, 0x0000000b, 0xfffffffd,
	          0xfffffffd, FALSE, FALSE)

	/* Divide by zero. Should set the overflow flag. */
	TEST_DIV (l.div, 0x0000000c, 0x00000000,
	          0x00000000, TRUE, FALSE)
	
	TEST_DIV (l.div, 0xfffffff4, 0x00000000,
	          0x00000000, TRUE, FALSE)

	/* Check that range exceptions are triggered */
	l.mfspr	r3,r0,SPR_SR
	LOAD_CONST (r2, SPR_SR_OVE)	/* Set OVE */
	l.or	r3,r3,r2
	l.mtspr	r0,r3,SPR_SR
	
	LOAD_STR (r3, "  ** OVE flag set **\n")
	l.jal	_puts
	l.nop

	/* Divide by zero. Should set the overflow flag and trigger an
	   exception. */
	TEST_DIV (l.div, 0x0000000c, 0x00000000,
	          0x00000000, TRUE, FALSE)
	
	TEST_DIV (l.div, 0xfffffff4, 0x00000000,
	          0x00000000, TRUE, FALSE)

	/* Finished checking range exceptions */
	l.mfspr	r3,r0,SPR_SR
	LOAD_CONST (r2, ~SPR_SR_OVE)	/* Clear OVE */
	l.and	r3,r3,r2
	l.mtspr	r0,r3,SPR_SR
	
	LOAD_STR (r3, "  ** OVE flag cleared **\n")
	l.jal	_puts
	l.nop

/* ----------------------------------------------------------------------------
 * Test of divide unsigned, l.divu
 * ------------------------------------------------------------------------- */
_divu:	
	LOAD_STR (r3, "l.divu\n")
	l.jal	_puts
	l.nop

	/* Divide two positive numbers and check rounding. Should set no
	   flags. */
	TEST_DIV (l.divu, 0x0000000c, 0x00000003,
	          0x00000004, FALSE, FALSE)

	TEST_DIV (l.divu, 0x0000000b, 0x00000003,
	          0x00000003, FALSE, FALSE)

	/* Divide two numbers that would be negative under 2's complement and
	   check rounding. Should set no flags. */
	TEST_DIV (l.divu, 0xfffffff4, 0xfffffffd,
	          0x00000000, FALSE, FALSE)

	TEST_DIV (l.divu, 0xfffffff5, 0xfffffffd,
	          0x00000000, FALSE, FALSE)

	/* Divide a number that would be negative under 2's complement by a
	   number that would be positive under 2's complement and check
	   rounding. Should set no flags. */
	TEST_DIV (l.divu, 0xfffffff4, 0x00000003,
	          0x55555551, FALSE, FALSE)

	TEST_DIV (l.divu, 0xfffffff5, 0x00000003,
	          0x55555551, FALSE, FALSE)

	/* Divide a number that would be positive under 2's complement by a
	   number that would be negative under 2's complement and check
	   rounding. Should set no flags. */
	TEST_DIV (l.divu, 0x0000000c, 0xfffffffd,
	          0x00000000, FALSE, FALSE)

	TEST_DIV (l.divu, 0x0000000b, 0xfffffffd,
	          0x00000000, FALSE, FALSE)

	/* Divide by zero. Should set the overflow flag. */
	TEST_DIV (l.divu, 0x0000000c, 0x00000000,
	          0x00000000, TRUE, FALSE)
	
	TEST_DIV (l.divu, 0xfffffff4, 0x00000000,
	          0x00000000, TRUE, FALSE)

	/* Check that range exceptions are triggered */
	l.mfspr	r3,r0,SPR_SR
	LOAD_CONST (r2, SPR_SR_OVE)	/* Set OVE */
	l.or	r3,r3,r2
	l.mtspr	r0,r3,SPR_SR
	
	LOAD_STR (r3, "  ** OVE flag set **\n")
	l.jal	_puts
	l.nop

	/* Divide by zero. Should set the overflow flag and trigger an
	   exception. */
	TEST_DIV (l.divu, 0x0000000c, 0x00000000,
	          0x00000000, TRUE, FALSE)
	
	TEST_DIV (l.divu, 0xfffffff4, 0x00000000,
	          0x00000000, TRUE, FALSE)

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
