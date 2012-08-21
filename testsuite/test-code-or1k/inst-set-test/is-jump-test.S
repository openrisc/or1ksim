/* is-jump-test.S. l.j, l.jal, l.jalr and l.jr instruction test of Or1ksim
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
 * The l.jalr and l.jr instructions should trigger an alignment exception if
 * the register does not holde an aligned address (Bug 1775).
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
 * A macro to carry out a test of a jump using a register destination
 *
 * We manually construct the opcode, to allow us to force r9 into the
 * destination field, to test exception handling. Usually the assembler would
 * prevent this.
 *
 * Arguments
 *   opc_mask:  The opcode mask
 *   regno:     Register number to use
 *   offset:    Offset in bytes forward of target (testing alignment
 * ------------------------------------------------------------------------- */
#define TEST_JUMP(opc_mask, dest, offset)				 \
	LOAD_CONST (r31,51f + offset)					;\
	.word	(0xe01f0004|(dest << 21))	/* l.ori rD,r31,r0 */	;\
	l.mtspr	r0,r0,SPR_EPCR_BASE	/* Clear record */		;\
50:	.word	(opc_mask|(dest << 11))	/* Jump opcode */		;\
	l.nop								;\
									;\
	/* Jump failed, we drop through to here */			;\
	l.mfspr	r2,r0,SPR_EPCR_BASE	/* What triggered exception */	;\
	PUSH (r2)			/* Save EPCR for later */	;\
	PUTS ("  Jump to 0x")						;\
	PUTH (51f + offset)						;\
	PUTS (" using register 0x")					;\
	PUTHQ (dest)							;\
	PUTS (" failed\n")						;\
	l.j	52f							;\
	l.nop								;\
									;\
	/* Jump succeeded we get here */				;\
51:	l.mfspr	r2,r0,SPR_EPCR_BASE	/* What triggered exception */	;\
	PUSH (r2)			/* Save EPCR for later */	;\
	PUTS ("  Jump to 0x")						;\
	PUTH (51b + offset)						;\
	PUTS (" using register 0x")					;\
	PUTHQ (dest)							;\
	PUTS (" OK\n")							;\
									;\
	/* Report if we got exception */				;\
52:	POP (r2)			/* Retrieve EPCR */		;\
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

/* ----------------------------------------------------------------------------
 * Test of jump and link register, l.jalr
 * ------------------------------------------------------------------------- */
_jalr:	
	LOAD_STR (r3, "l.jalr\n")
	l.jal	_puts
	l.nop

	/* Test with various alignment offsets */
	TEST_JUMP (0x48000000, 5, 0)		/* No offset */
	TEST_JUMP (0x48000000, 5, 1)		/* No offset */
	TEST_JUMP (0x48000000, 5, 2)		/* No offset */
	TEST_JUMP (0x48000000, 5, 3)		/* No offset */

	/* Test with link register as the destination */
	TEST_JUMP (0x48000000, 9, 0)		/* No offset */

/* ----------------------------------------------------------------------------
 * Test of jump register, l.jr
 * ------------------------------------------------------------------------- */
_jr:	
	LOAD_STR (r3, "l.jr\n")
	l.jal	_puts
	l.nop

	/* Test with various alignment offsets */
	TEST_JUMP (0x44000000, 5, 0)			/* No offset */
	TEST_JUMP (0x44000000, 5, 1)			/* No offset */
	TEST_JUMP (0x44000000, 5, 2)			/* No offset */
	TEST_JUMP (0x44000000, 5, 3)			/* No offset */

	/* Test with link register as the destination (OK here) */
	TEST_JUMP (0x44000000, 9, 0)			/* No offset */

/* ----------------------------------------------------------------------------
 * All done
 * ------------------------------------------------------------------------- */
_exit:
	LOAD_STR (r3, "Test completed\n")
	l.jal	_puts
	l.nop

	TEST_EXIT
