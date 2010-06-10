/* is-find.test.S. l.ff1 and l.fl1 instruction test of Or1ksim
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
 * The l.ff1 and l.fl1 should fine the first and last bits in a register
 * respectively.
 *
 * Problems in this area were reported in Bug 1772. Having fixed the problem,
 * this is (in good software engineering style), a  regression test to go with
 * the fix.
 *
 * This is not a comprehensive test of any instruction (yet).
 *
 * Of course what is really needed is a comprehensive instruction test...
 * ------------------------------------------------------------------------- */


#include "inst-set-test.h"

/* ----------------------------------------------------------------------------
 * A macro to carry out a test of find
 *
 * Arguments
 *   opc:       The opcode
 *   op:        First operand value
 *   res:       Expected result
 *   str:       Output string
 * ------------------------------------------------------------------------- */
#define TEST_FIND(opc, op, res, str)					 \
	LOAD_CONST (r4,33)		/* Invalid result */		;\
	LOAD_CONST (r5,op)		/* Load number to analyse */	;\
	opc	r4,r5						;\
	CHECK_RES (str, r4, res)

	
	.section .text
	.global	_start
_start:

/* ----------------------------------------------------------------------------
 * Test of find first 1, l.ff1
 * ------------------------------------------------------------------------- */
_ff1:
	LOAD_STR (r3, "l.ff1\n")
	l.jal	_puts
	l.nop

	/* Try a range of candidates. */
	TEST_FIND (l.ff1, 0x00000001,  1, "ff1 (0x00000001) =  1: ") 
	TEST_FIND (l.ff1, 0x80000000, 32, "ff1 (0x80000000) = 32: ")
	TEST_FIND (l.ff1, 0x55555555,  1, "ff1 (0x55555555) =  1: ")
	TEST_FIND (l.ff1, 0xaaaaaaaa,  2, "ff1 (0xaaaaaaaa) =  2: ")
	TEST_FIND (l.ff1, 0x00018000, 16, "ff1 (0x00018000) = 16: ")
	TEST_FIND (l.ff1, 0xc0000000, 31, "ff1 (0xc0000000) = 31: ")
	TEST_FIND (l.ff1, 0x00000000,  0, "ff1 (0x00000000) =  0: ")

/* ----------------------------------------------------------------------------
 * Test of find first 1, l.fl1
 * ------------------------------------------------------------------------- */
_fl1:
	LOAD_STR (r3, "l.fl1\n")
	l.jal	_puts
	l.nop

	/* Try a range of candidates. */
	TEST_FIND (l.fl1, 0x00000001,  1, "fl1 (0x00000001) =  1: ") 
	TEST_FIND (l.fl1, 0x80000000, 32, "fl1 (0x80000000) = 32: ")
	TEST_FIND (l.fl1, 0x55555555, 31, "fl1 (0x55555555) = 31: ")
	TEST_FIND (l.fl1, 0xaaaaaaaa, 32, "fl1 (0xaaaaaaaa) = 32: ")
	TEST_FIND (l.fl1, 0x00018000, 17, "fl1 (0x00018000) = 17: ")
	TEST_FIND (l.fl1, 0xc0000000, 32, "fl1 (0xc0000000) = 32: ")
	TEST_FIND (l.fl1, 0x00000000,  0, "fl1 (0x00000000) =  0: ")

/* ----------------------------------------------------------------------------
 * All done
 * ------------------------------------------------------------------------- */
_exit:
	LOAD_STR (r3, "Test completed\n")
	l.jal	_puts
	l.nop

	TEST_EXIT