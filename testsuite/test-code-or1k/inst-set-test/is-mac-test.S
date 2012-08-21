/* is-mac-test.S. l.mac, l.maci, l.macrc and l.msb instruction test of Or1ksim
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
 * The l.mac, l.maci, l.macrc and l.msb instructions perform operations related
 * to combined signed multiply and addition/subtraction.
 *
 * The precise definition of these instructions is in flux. In addition there
 * are known problems with the assembler/disassembler (will not correctly
 * handle l.maci) and with the Verilog RTL implementation (not functional).
 *
 * Problems in this area were reported in Bugs 1773 and 1777. Having fixed the
 * problem, this is (in good software engineering style), a  regression test
 * to go with the fix.
 *
 * This is not a comprehensive test of any instruction (yet).
 *
 * Of course what is really needed is a comprehensive instruction test...
 * ------------------------------------------------------------------------- */


#include "inst-set-test.h"

/* ----------------------------------------------------------------------------
 * A macro to carry out a test of multiply accumulate read and clear
 *
 * Arguments
 *   machi:     Inital value of MACHI
 *   maclo:     Inital value of MACLO
 *   op1:       First operand value
 *   op2:       Second operand value
 *   res:       Expected result
 * ------------------------------------------------------------------------- */
#define TEST_MACRC(machi, maclo, op1, op2, res)				 \
	LOAD_CONST (r2,maclo)						;\
	l.mtspr	r0,r2,SPR_MACLO						;\
	LOAD_CONST (r2,machi)						;\
	l.mtspr	r0,r2,SPR_MACHI						;\
									;\
	LOAD_CONST (r5,op1)		/* Load numbers to add */	;\
	LOAD_CONST (r6,op2)						;\
	l.mac	r5,r6							;\
	l.macrc	r4							;\
	PUSH (r4)			/* Save for later */		;\
	PUTS ("  0x")							;\
	PUTH (machi)							;\
	PUTS (" ")							;\
	PUTH (maclo)							;\
	PUTS (" + 0x")							;\
	PUTH (op1)							;\
	PUTS (" * 0x")							;\
	PUTH (op2)							;\
	PUTS (" = 0x")							;\
	PUTH (res)							;\
	PUTS (": ")							;\
	POP (r4)							;\
	CHECK_RES ("", r4, res)						;\
									;\
	l.mfspr	r5,r0,SPR_MACHI						;\
	l.sfne	r5,r0							;\
	l.bf	50f							;\
									;\
	PUTS ("  - MACHI cleared\n")					;\
									;\
50:	l.mfspr	r6,r0,SPR_MACLO						;\
	l.sfne	r6,r0							;\
	l.bf	51f							;\
									;\
	PUTS ("  - MACLO cleared\n")					;\
51:	
	
/* ----------------------------------------------------------------------------
 * A macro to carry out a test of multiply accumulate in registers
 *
 * Arguments
 *   machi:     Inital value of MACHI
 *   maclo:     Inital value of MACLO
 *   op1:       First operand value
 *   op2:       Second operand value
 *   reshi:     Expected result
 *   reslo:     Expected result
 * ------------------------------------------------------------------------- */
#define TEST_MAC(machi, maclo, op1, op2, reshi, reslo)		 \
	LOAD_CONST (r2,maclo)						;\
	l.mtspr	r0,r2,SPR_MACLO						;\
	LOAD_CONST (r2,machi)						;\
	l.mtspr	r0,r2,SPR_MACHI						;\
									;\
	LOAD_CONST (r5,op1)		/* Load numbers to add */	;\
	LOAD_CONST (r6,op2)						;\
	l.mac	r5,r6							;\
	l.mfspr	r5,r0,SPR_MACHI						;\
	l.mfspr	r6,r0,SPR_MACLO						;\
	PUSH (r5)			/* Save for later */		;\
	PUSH (r6)							;\
	PUTS ("  0x")							;\
	PUTH (machi)							;\
	PUTS (" ")							;\
	PUTH (maclo)							;\
	PUTS (" + 0x")							;\
	PUTH (op1)							;\
	PUTS (" * 0x")							;\
	PUTH (op2)							;\
	PUTS (" = 0x")							;\
	PUTH (reshi)							;\
	PUTS (" ")							;\
	PUTH (reslo)							;\
	PUTS (": ")							;\
	POP (r6)							;\
	POP (r5)							;\
	CHECK_RES2 (r5, r6, reshi, reslo)
	
	
/* ----------------------------------------------------------------------------
 * A macro to carry out a test of multiply accumulate with immediate arg
 *
 * There is currently a bug in the assembler, so we must hand construct
 * l.maci r5,op1.
 *
 * Arguments
 *   machi:     Inital value of MACHI
 *   maclo:     Inital value of MACLO
 *   op1:       First operand value
 *   op2:       Second operand value
 *   reshi:     Expected result
 *   reslo:     Expected result
 * ------------------------------------------------------------------------- */
#define TEST_MACI(machi, maclo, op1, op2, reshi, reslo)			 \
	LOAD_CONST (r2,maclo)						;\
	l.mtspr	r0,r2,SPR_MACLO						;\
	LOAD_CONST (r2,machi)						;\
	l.mtspr	r0,r2,SPR_MACHI						;\
									;\
	LOAD_CONST (r5,op1)		/* Load number to add */	;\
	.word	(0x4c050000|op2)	/* l.maci r5,op2 */		;\
	/* l.maci	r5,op2 */					;\
	l.mfspr	r5,r0,SPR_MACHI						;\
	l.mfspr	r6,r0,SPR_MACLO						;\
	PUSH (r5)			/* Save for later */		;\
	PUSH (r6)							;\
	PUTS ("  0x")							;\
	PUTH (machi)							;\
	PUTS (" ")							;\
	PUTH (maclo)							;\
	PUTS (" + 0x")							;\
	PUTH (op1)							;\
	PUTS (" * 0x")							;\
	PUTH (op2)							;\
	PUTS (" = 0x")							;\
	PUTH (reshi)							;\
	PUTS (" ")							;\
	PUTH (reslo)							;\
	PUTS (": ")							;\
	POP (r6)							;\
	POP (r5)							;\
	CHECK_RES2 (r5, r6, reshi, reslo)


/* ----------------------------------------------------------------------------
 * A macro to carry out a test of multiply and subract
 *
 * Arguments
 *   machi:     Inital value of MACHI
 *   maclo:     Inital value of MACLO
 *   op1:       First operand value
 *   op2:       Second operand value
 *   reshi:     Expected result
 *   reslo:     Expected result
 * ------------------------------------------------------------------------- */
#define TEST_MSB(machi, maclo, op1, op2, reshi, reslo)			 \
	LOAD_CONST (r2,maclo)						;\
	l.mtspr	r0,r2,SPR_MACLO						;\
	LOAD_CONST (r2,machi)						;\
	l.mtspr	r0,r2,SPR_MACHI						;\
									;\
	LOAD_CONST (r5,op1)		/* Load numbers to add */	;\
	LOAD_CONST (r6,op2)						;\
	l.msb	r5,r6							;\
	l.mfspr	r5,r0,SPR_MACHI						;\
	l.mfspr	r6,r0,SPR_MACLO						;\
	PUSH (r5)			/* Save for later */		;\
	PUSH (r6)							;\
	PUTS ("  0x")							;\
	PUTH (machi)							;\
	PUTS (" ")							;\
	PUTH (maclo)							;\
	PUTS (" - 0x")							;\
	PUTH (op1)							;\
	PUTS (" * 0x")							;\
	PUTH (op2)							;\
	PUTS (" = 0x")							;\
	PUTH (reshi)							;\
	PUTS (" ")							;\
	PUTH (reslo)							;\
	PUTS (": ")							;\
	POP (r6)							;\
	POP (r5)							;\
	CHECK_RES2 (r5, r6, reshi, reslo)
	
	
/* ----------------------------------------------------------------------------
 * Start of code
 * ------------------------------------------------------------------------- */
	.section .text
	.global	_start
_start:

/* ----------------------------------------------------------------------------
 * Test of multiply signed and accumulate, l.mac
 * ------------------------------------------------------------------------- */
_mac:
	LOAD_STR (r3, "l.mac\n")
	l.jal	_puts
	l.nop

	/* MAC two small positive numbers on a zero total */
	TEST_MAC (0x00000000, 0x00000000,
	          0x00000002, 0x00000003,
	          0x00000000, 0x00000006)

	/* MAC two small positive numbers on a small positive total */
	TEST_MAC (0x00000000, 0x00000006,
	          0x00000002, 0x00000003,
	          0x00000000, 0x0000000c)

	/* MAC two small positive numbers on a moderate positive total */
	TEST_MAC (0x00000000, 0xfffffffa,
	          0x00000002, 0x00000003,
	          0x00000001, 0x00000000)

	/* MAC two small positive numbers on a large positive total */
	TEST_MAC (0x3fffffff, 0xfffffffa,
	          0x00000002, 0x00000003,
	          0x40000000, 0x00000000)

	/* MAC two small positive numbers on a small negative total */
	TEST_MAC (0xffffffff, 0xfffffffa,
	          0x00000002, 0x00000003,
	          0x00000000, 0x00000000)

	/* MAC two small positive numbers on a moderate negative total */
	TEST_MAC (0xffffffff, 0x00000000,
	          0x00000002, 0x00000003,
	          0xffffffff, 0x00000006)

	/* MAC two small positive numbers on a large negative total */
	TEST_MAC (0x80000000, 0x00000000,
	          0x00000002, 0x00000003,
	          0x80000000, 0x00000006)

	PUTC ('\n')

	/* MAC two moderate positive numbers on a zero total */
	TEST_MAC (0x00000000, 0x00000000,
	          0x00008001, 0x0000fffe,
	          0x00000000, 0x7ffffffe)

	/* MAC two moderate positive numbers on a small positive total */
	TEST_MAC (0x00000000, 0x00000002,
	          0x00008001, 0x0000fffe,
	          0x00000000, 0x80000000)

	/* MAC two moderate positive numbers on a moderate positive total */
	TEST_MAC (0x00000000, 0x80000002,
	          0x00008001, 0x0000fffe,
	          0x00000001, 0x00000000)

	/* MAC two moderate positive numbers on a large positive total */
	TEST_MAC (0x7fffffff, 0x80000001,
	          0x00008001, 0x0000fffe,
	          0x7fffffff, 0xffffffff)

	/* MAC two moderate positive numbers on a small negative total */
	TEST_MAC (0xffffffff, 0xffffffff,
	          0x00008001, 0x0000fffe,
	          0x00000000, 0x7ffffffd)

	/* MAC two moderate positive numbers on a moderate negative total */
	TEST_MAC (0xffffffff, 0x80000002,
	          0x00008001, 0x0000fffe,
	          0x00000000, 0x00000000)

	/* MAC two moderate positive numbers on a large negative total */
	TEST_MAC (0xfffffffe, 0x80000002,
	          0x00008001, 0x0000fffe,
	          0xffffffff, 0x00000000)

	PUTC ('\n')

	/* MAC two small negative numbers on a zero total */
	TEST_MAC (0x00000000, 0x00000000,
	          0xfffffffe, 0xfffffffd,
	          0x00000000, 0x00000006)

	/* MAC two small negative numbers on a small positive total */
	TEST_MAC (0x00000000, 0x00000006,
	          0xfffffffe, 0xfffffffd,
	          0x00000000, 0x0000000c)

	/* MAC two small negative numbers on a small negative total */
	TEST_MAC (0xffffffff, 0xffffffff,
	          0xfffffffe, 0xfffffffd,
	          0x00000000, 0x00000005)

	PUTC ('\n')

	/* MAC one small positive and one small negative number on a zero
	   total */
	TEST_MAC (0x00000000, 0x00000000,
	          0x00000002, 0xfffffffd,
	          0xffffffff, 0xfffffffa)

	/* MAC one small positive and one small negative number on a small
	   positive total */
	TEST_MAC (0x00000000, 0x0000000c,
	          0x00000002, 0xfffffffd,
	          0x00000000, 0x00000006)

	/* MAC one small positive and one small negative number on a moderate
	   positive total */
	TEST_MAC (0x00000001, 0x00000005,
	          0x00000002, 0xfffffffd,
	          0x00000000, 0xffffffff)

	/* MAC one small positive and one small negative number on a large
	   positive total */
	TEST_MAC (0x7fffffff, 0xffffffff,
	          0x00000002, 0xfffffffd,
	          0x7fffffff, 0xfffffff9)

	/* MAC one small positive and one small negative number on a small
	   negative total */
	TEST_MAC (0xffffffff, 0xffffffff,
	          0x00000002, 0xfffffffd,
	          0xffffffff, 0xfffffff9)

	/* MAC one small positive and one small negative number on a moderate
	   negative total */
	TEST_MAC (0xffffffff, 0x00000005,
	          0x00000002, 0xfffffffd,
	          0xfffffffe, 0xffffffff)

	/* MAC one small positive and one small negative number on a large
	   negative total */
	TEST_MAC (0x80000000, 0x00000006,
	          0x00000002, 0xfffffffd,
	          0x80000000, 0x00000000)

	PUTC ('\n')

	/* MAC one moderate positive and one moderate negative number on a
	   zero total */
	TEST_MAC (0x00000000, 0x00000000,
	          0x00008000, 0xffff0000,
	          0xffffffff, 0x80000000)

	/* MAC one moderate positive and one moderate negative number on a
	   small positive total */
	TEST_MAC (0x00000000, 0x00000006,
	          0x00008000, 0xffff0000,
	          0xffffffff, 0x80000006)

	/* MAC one moderate positive and one moderate negative number on a
	   moderate positive total */
	TEST_MAC (0x00000000, 0x80000000,
	          0x00008000, 0xffff0000,
	          0x00000000, 0x00000000)

	/* MAC one moderate positive and one moderate negative number on a
	   large positive total */
	TEST_MAC (0x7fffffff, 0xffffffff,
	          0x00008000, 0xffff0000,
	          0x7fffffff, 0x7fffffff)

	/* MAC one moderate positive and one moderate negative number on a
	   small negative total */
	TEST_MAC (0xffffffff, 0xffffffff,
	          0x00008000, 0xffff0000,
	          0xffffffff, 0x7fffffff)

	/* MAC one moderate positive and one moderate negative number on a
	   moderate negative total */
	TEST_MAC (0xffffffff, 0x7fffffff,
	          0x00008000, 0xffff0000,
	          0xfffffffe, 0xffffffff)

	/* MAC one moderate positive and one moderate negative number on a
	   large negative total */
	TEST_MAC (0x80000000, 0x80000000,
	          0x00008000, 0xffff0000,
	          0x80000000, 0x00000000)

	PUTC ('\n')

/* ----------------------------------------------------------------------------
 * Test of multiply signed and accumulate, l.maci
 * ------------------------------------------------------------------------- */
_maci:
	LOAD_STR (r3, "l.maci\n")
	l.jal	_puts
	l.nop

	/* MAC two small positive numbers on a zero total */
	TEST_MACI (0x00000000, 0x00000000,
	           0x00000002, 0x0003,
	           0x00000000, 0x00000006)

	/* MAC two small positive numbers on a small positive total */
	TEST_MACI (0x00000000, 0x00000006,
	           0x00000002, 0x0003,
	           0x00000000, 0x0000000c)

	/* MAC two small positive numbers on a moderate positive total */
	TEST_MACI (0x00000000, 0xfffffffa,
	           0x00000002, 0x0003,
	           0x00000001, 0x00000000)

	/* MAC two small positive numbers on a large positive total */
	TEST_MACI (0x3fffffff, 0xfffffffa,
	           0x00000002, 0x0003,
	           0x40000000, 0x00000000)

	/* MAC two small positive numbers on a small negative total */
	TEST_MACI (0xffffffff, 0xfffffffa,
	           0x00000002, 0x0003,
	           0x00000000, 0x00000000)

	/* MAC two small positive numbers on a moderate negative total */
	TEST_MACI (0xffffffff, 0x00000000,
	           0x00000002, 0x0003,
	           0xffffffff, 0x00000006)

	/* MAC two small positive numbers on a large negative total */
	TEST_MACI (0x80000000, 0x00000000,
	           0x00000002, 0x0003,
	           0x80000000, 0x00000006)

	PUTC ('\n')
tmp:	
	/* MAC two moderate positive numbers on a zero total */
	TEST_MACI (0x00000000, 0x00000000,
	           0x00010002, 0x7fff,
	           0x00000000, 0x7ffffffe)

	/* MAC two moderate positive numbers on a small positive total */
	TEST_MACI (0x00000000, 0x00000002,
	           0x00010002, 0x7fff,
	           0x00000000, 0x80000000)

	/* MAC two moderate positive numbers on a moderate positive total */
	TEST_MACI (0x00000000, 0x80000002,
	           0x00010002, 0x7fff,
	           0x00000001, 0x00000000)

	/* MAC two moderate positive numbers on a large positive total */
	TEST_MACI (0x7fffffff, 0x80000001,
	           0x00010002, 0x7fff,
	           0x7fffffff, 0xffffffff)

	/* MAC two moderate positive numbers on a small negative total */
	TEST_MACI (0xffffffff, 0xffffffff,
	           0x00010002, 0x7fff,
	           0x00000000, 0x7ffffffd)

	/* MAC two moderate positive numbers on a moderate negative total */
	TEST_MACI (0xffffffff, 0x80000002,
	           0x00010002, 0x7fff,
	           0x00000000, 0x00000000)

	/* MAC two moderate positive numbers on a large negative total */
	TEST_MACI (0xfffffffe, 0x80000002,
	           0x00010002, 0x7fff,
	           0xffffffff, 0x00000000)

	PUTC ('\n')

	/* MAC two small negative numbers on a zero total */
	TEST_MACI (0x00000000, 0x00000000,
	           0xfffffffe, 0xfffd,
	           0x00000000, 0x00000006)

	/* MAC two small negative numbers on a small positive total */
	TEST_MACI (0x00000000, 0x00000006,
	           0xfffffffe, 0xfffd,
	           0x00000000, 0x0000000c)

	/* MAC two small negative numbers on a small negative total */
	TEST_MACI (0xffffffff, 0xffffffff,
	           0xfffffffe, 0xfffd,
	           0x00000000, 0x00000005)

	PUTC ('\n')

	/* MAC one small positive and one small negative number on a zero
	   total */
	TEST_MACI (0x00000000, 0x00000000,
	           0x00000002, 0xfffd,
	           0xffffffff, 0xfffffffa)

	/* MAC one small positive and one small negative number on a small
	   positive total */
	TEST_MACI (0x00000000, 0x0000000c,
	           0x00000002, 0xfffd,
	           0x00000000, 0x00000006)

	/* MAC one small positive and one small negative number on a moderate
	   positive total */
	TEST_MACI (0x00000001, 0x00000005,
	           0x00000002, 0xfffd,
	           0x00000000, 0xffffffff)

	/* MAC one small positive and one small negative number on a large
	   positive total */
	TEST_MACI (0x7fffffff, 0xffffffff,
	           0x00000002, 0xfffd,
	           0x7fffffff, 0xfffffff9)

	/* MAC one small positive and one small negative number on a small
	   negative total */
	TEST_MACI (0xffffffff, 0xffffffff,
	           0x00000002, 0xfffd,
	           0xffffffff, 0xfffffff9)

	/* MAC one small positive and one small negative number on a moderate
	   negative total */
	TEST_MACI (0xffffffff, 0x00000005,
	           0x00000002, 0xfffd,
	           0xfffffffe, 0xffffffff)

	/* MAC one small positive and one small negative number on a large
	   negative total */
	TEST_MACI (0x80000000, 0x00000006,
	           0x00000002, 0xfffd,
	           0x80000000, 0x00000000)

	PUTC ('\n')

	/* MAC one moderate positive and one moderate negative number on a
	   zero total */
	TEST_MACI (0x00000000, 0x00000000,
	           0x00010000, 0x8000,
	           0xffffffff, 0x80000000)

	/* MAC one moderate positive and one moderate negative number on a
	   small positive total */
	TEST_MACI (0x00000000, 0x00000006,
	           0x00010000, 0x8000,
	           0xffffffff, 0x80000006)

	/* MAC one moderate positive and one moderate negative number on a
	   moderate positive total */
	TEST_MACI (0x00000000, 0x80000000,
	           0x00010000, 0x8000,
	           0x00000000, 0x00000000)

	/* MAC one moderate positive and one moderate negative number on a
	   large positive total */
	TEST_MACI (0x7fffffff, 0xffffffff,
	           0x00010000, 0x8000,
	           0x7fffffff, 0x7fffffff)

	/* MAC one moderate positive and one moderate negative number on a
	   small negative total */
	TEST_MACI (0xffffffff, 0xffffffff,
	           0x00010000, 0x8000,
	           0xffffffff, 0x7fffffff)

	/* MAC one moderate positive and one moderate negative number on a
	   moderate negative total */
	TEST_MACI (0xffffffff, 0x7fffffff,
	           0x00010000, 0x8000,
	           0xfffffffe, 0xffffffff)

	/* MAC one moderate positive and one moderate negative number on a
	   large negative total */
	TEST_MACI (0x80000000, 0x80000000,
	           0x00010000, 0x8000,
	           0x80000000, 0x00000000)

	PUTC ('\n')

/* ----------------------------------------------------------------------------
 * Test of multiply signed and accumulate, read and clear l.macrc
 * ------------------------------------------------------------------------- */
_macrc:
	LOAD_STR (r3, "l.macrc\n")
	l.jal	_puts
	l.nop

	/* MAC two small positive numbers on a zero total */
	TEST_MACRC (0x00000000, 0x00000000,
	            0x00000002, 0x00000003,
	            0x00000006)

	/* MAC two small positive numbers on a small positive total */
	TEST_MACRC (0x00000000, 0x00000006,
	            0x00000002, 0x00000003,
	            0x0000000c)

	/* MAC two small positive numbers on a moderate positive total */
	TEST_MACRC (0x00000000, 0xfffffffa,
	          0x00000002, 0x00000003,
	          0x00000000)

	/* MAC two small positive numbers on a large positive total */
	TEST_MACRC (0x3fffffff, 0xfffffffa,
	            0x00000002, 0x00000003,
	            0x00000000)

	/* MAC two small positive numbers on a small negative total */
	TEST_MACRC (0xffffffff, 0xfffffffa,
	            0x00000002, 0x00000003,
	            0x00000000)

	/* MAC two small positive numbers on a moderate negative total */
	TEST_MACRC (0xffffffff, 0x00000000,
	            0x00000002, 0x00000003,
	            0x00000006)

	/* MAC two small positive numbers on a large negative total */
	TEST_MACRC (0x80000000, 0x00000000,
	            0x00000002, 0x00000003,
	            0x00000006)

	PUTC ('\n')

	/* MAC two moderate positive numbers on a zero total */
	TEST_MACRC (0x00000000, 0x00000000,
	            0x00008001, 0x0000fffe,
	            0x7ffffffe)

	/* MAC two moderate positive numbers on a small positive total */
	TEST_MACRC (0x00000000, 0x00000002,
	            0x00008001, 0x0000fffe,
	            0x80000000)

	/* MAC two moderate positive numbers on a moderate positive total */
	TEST_MACRC (0x00000000, 0x80000002,
	            0x00008001, 0x0000fffe,
	            0x00000000)

	/* MAC two moderate positive numbers on a large positive total */
	TEST_MACRC (0x7fffffff, 0x80000001,
	            0x00008001, 0x0000fffe,
	            0xffffffff)

	/* MAC two moderate positive numbers on a small negative total */
	TEST_MACRC (0xffffffff, 0xffffffff,
	            0x00008001, 0x0000fffe,
	            0x7ffffffd)

	/* MAC two moderate positive numbers on a moderate negative total */
	TEST_MACRC (0xffffffff, 0x80000002,
	            0x00008001, 0x0000fffe,
	            0x00000000)

	/* MAC two moderate positive numbers on a large negative total */
	TEST_MACRC (0xfffffffe, 0x80000002,
	            0x00008001, 0x0000fffe,
	            0x00000000)

	PUTC ('\n')

	/* MAC two small negative numbers on a zero total */
	TEST_MACRC (0x00000000, 0x00000000,
	            0xfffffffe, 0xfffffffd,
	            0x00000006)

	/* MAC two small negative numbers on a small positive total */
	TEST_MACRC (0x00000000, 0x00000006,
	            0xfffffffe, 0xfffffffd,
	            0x0000000c)

	/* MAC two small negative numbers on a small negative total */
	TEST_MACRC (0xffffffff, 0xffffffff,
	            0xfffffffe, 0xfffffffd,
	            0x00000005)

	PUTC ('\n')

	/* MAC one small positive and one small negative number on a zero
	   total */
	TEST_MACRC (0x00000000, 0x00000000,
	            0x00000002, 0xfffffffd,
	            0xfffffffa)

	/* MAC one small positive and one small negative number on a small
	   positive total */
	TEST_MACRC (0x00000000, 0x0000000c,
	            0x00000002, 0xfffffffd,
	            0x00000006)

	/* MAC one small positive and one small negative number on a moderate
	   positive total */
	TEST_MACRC (0x00000001, 0x00000005,
	            0x00000002, 0xfffffffd,
	            0xffffffff)

	/* MAC one small positive and one small negative number on a large
	   positive total */
	TEST_MACRC (0x7fffffff, 0xffffffff,
	            0x00000002, 0xfffffffd,
	            0xfffffff9)

	/* MAC one small positive and one small negative number on a small
	   negative total */
	TEST_MACRC (0xffffffff, 0xffffffff,
	            0x00000002, 0xfffffffd,
	            0xfffffff9)

	/* MAC one small positive and one small negative number on a moderate
	   negative total */
	TEST_MACRC (0xffffffff, 0x00000005,
	            0x00000002, 0xfffffffd,
	            0xffffffff)

	/* MAC one small positive and one small negative number on a large
	   negative total */
	TEST_MACRC (0x80000000, 0x00000006,
	            0x00000002, 0xfffffffd,
	            0x00000000)

	PUTC ('\n')

	/* MAC one moderate positive and one moderate negative number on a
	   zero total */
	TEST_MACRC (0x00000000, 0x00000000,
	            0x00008000, 0xffff0000,
	            0x80000000)

	/* MAC one moderate positive and one moderate negative number on a
	   small positive total */
	TEST_MACRC (0x00000000, 0x00000006,
	            0x00008000, 0xffff0000,
	            0x80000006)

	/* MAC one moderate positive and one moderate negative number on a
	   moderate positive total */
	TEST_MACRC (0x00000000, 0x80000000,
	            0x00008000, 0xffff0000,
	            0x00000000)

	/* MAC one moderate positive and one moderate negative number on a
	   large positive total */
	TEST_MACRC (0x7fffffff, 0xffffffff,
	            0x00008000, 0xffff0000,
	            0x7fffffff)

	/* MAC one moderate positive and one moderate negative number on a
	   small negative total */
	TEST_MACRC (0xffffffff, 0xffffffff,
	            0x00008000, 0xffff0000,
	            0x7fffffff)

	/* MAC one moderate positive and one moderate negative number on a
	   moderate negative total */
	TEST_MACRC (0xffffffff, 0x7fffffff,
	            0x00008000, 0xffff0000,
	            0xffffffff)

	/* MAC one moderate positive and one moderate negative number on a
	   large negative total */
	TEST_MACRC (0x80000000, 0x80000000,
	            0x00008000, 0xffff0000,
	            0x00000000)

	PUTC ('\n')

/* ----------------------------------------------------------------------------
 * Test of multiply signed and accumulate, l.msb
 * ------------------------------------------------------------------------- */
_msb:
	LOAD_STR (r3, "l.msb\n")
	l.jal	_puts
	l.nop

	/* MSB two small positive numbers on a zero total */
	TEST_MSB (0x00000000, 0x00000000,
	          0x00000002, 0x00000003,
	          0xffffffff, 0xfffffffa)

	/* MSB two small positive numbers on a small positive total */
	TEST_MSB (0x00000000, 0x0000000c,
	          0x00000002, 0x00000003,
	          0x00000000, 0x00000006)

	/* MSB two small positive numbers on a moderate positive total */
	TEST_MSB (0x00000001, 0x00000000,
	          0x00000002, 0x00000003,
	          0x00000000, 0xfffffffa)

	/* MSB two small positive numbers on a large positive total */
	TEST_MSB (0x40000000, 0x00000000,
	          0x00000002, 0x00000003,
	          0x3fffffff, 0xfffffffa)

	/* MSB two small positive numbers on a small negative total */
	TEST_MSB (0xffffffff, 0xfffffffa,
	          0x00000002, 0x00000003,
	          0xffffffff, 0xfffffff4)

	/* MSB two small positive numbers on a moderate negative total */
	TEST_MSB (0xffffffff, 0x00000005,
	          0x00000002, 0x00000003,
	          0xfffffffe, 0xffffffff)

	/* MSB two small positive numbers on a large negative total */
	TEST_MSB (0x80000000, 0x00000006,
	          0x00000002, 0x00000003,
	          0x80000000, 0x00000000)

	PUTC ('\n')

	/* MSB two moderate positive numbers on a zero total */
	TEST_MSB (0x00000000, 0x00000000,
	          0x00008001, 0x0000fffe,
	          0xffffffff, 0x80000002)

	/* MSB two moderate positive numbers on a small positive total */
	TEST_MSB (0x00000000, 0x00000002,
	          0x00008001, 0x0000fffe,
	          0xffffffff, 0x80000004)

	/* MSB two moderate positive numbers on a moderate positive total */
	TEST_MSB (0x00000000, 0x80000002,
	          0x00008001, 0x0000fffe,
	          0x00000000, 0x00000004)

	/* MSB two moderate positive numbers on a large positive total */
	TEST_MSB (0x7fffffff, 0x7ffffffd,
	          0x00008001, 0x0000fffe,
	          0x7ffffffe, 0xffffffff)

	/* MSB two moderate positive numbers on a small negative total */
	TEST_MSB (0xffffffff, 0xffffffff,
	          0x00008001, 0x0000fffe,
	          0xffffffff, 0x80000001)

	/* MSB two moderate positive numbers on a moderate negative total */
	TEST_MSB (0xffffffff, 0x80000002,
	          0x00008001, 0x0000fffe,
	          0xffffffff, 0x00000004)

	/* MSB two moderate positive numbers on a large negative total */
	TEST_MSB (0xfffffffe, 0x80000002,
	          0x00008001, 0x0000fffe,
	          0xfffffffe, 0x00000004)

	PUTC ('\n')

	/* MSB two small negative numbers on a zero total */
	TEST_MSB (0x00000000, 0x00000006,
	          0xfffffffe, 0xfffffffd,
	          0x00000000, 0x00000000)

	/* MSB two small negative numbers on a small positive total */
	TEST_MSB (0x00000000, 0x0000000c,
	          0xfffffffe, 0xfffffffd,
	          0x00000000, 0x00000006)

	/* MSB two small negative numbers on a small negative total */
	TEST_MSB (0xffffffff, 0xffffffff,
	          0xfffffffe, 0xfffffffd,
	          0xffffffff, 0xfffffff9)

	PUTC ('\n')

	/* MSB one small positive and one small negative number on a zero
	   total */
	TEST_MSB (0x00000000, 0x00000000,
	          0x00000002, 0xfffffffd,
	          0x00000000, 0x00000006)

	/* MSB one small positive and one small negative number on a small
	   positive total */
	TEST_MSB (0x00000000, 0x00000006,
	          0x00000002, 0xfffffffd,
	          0x00000000, 0x0000000c)

	/* MSB one small positive and one small negative number on a moderate
	   positive total */
	TEST_MSB (0x00000000, 0xffffffff,
	          0x00000002, 0xfffffffd,
	          0x00000001, 0x00000005)

	/* MSB one small positive and one small negative number on a large
	   positive total */
	TEST_MSB (0x7fffffff, 0xfffffff9,
	          0x00000002, 0xfffffffd,
	          0x7fffffff, 0xffffffff)

	/* MSB one small positive and one small negative number on a small
	   negative total */
	TEST_MSB (0xffffffff, 0xfffffff9,
	          0x00000002, 0xfffffffd,
	          0xffffffff, 0xffffffff)

	/* MSB one small positive and one small negative number on a moderate
	   negative total */
	TEST_MSB (0xfffffffe, 0xffffffff,
	          0x00000002, 0xfffffffd,
	          0xffffffff, 0x00000005)

	/* MSB one small positive and one small negative number on a large
	   negative total */
	TEST_MSB (0x80000000, 0x00000000,
	          0x00000002, 0xfffffffd,
	          0x80000000, 0x00000006)

	PUTC ('\n')

	/* MSB one moderate positive and one moderate negative number on a
	   zero total */
	TEST_MSB (0x00000000, 0x00000000,
	          0x00008000, 0xffff0000,
	          0x00000000, 0x80000000)

	/* MSB one moderate positive and one moderate negative number on a
	   small positive total */
	TEST_MSB (0x00000000, 0x00000006,
	          0x00008000, 0xffff0000,
	          0x00000000, 0x80000006)

	/* MSB one moderate positive and one moderate negative number on a
	   moderate positive total */
	TEST_MSB (0x00000000, 0x80000000,
	          0x00008000, 0xffff0000,
	          0x00000001, 0x00000000)

	/* MSB one moderate positive and one moderate negative number on a
	   large positive total */
	TEST_MSB (0x7fffffff, 0x7fffffff,
	          0x00008000, 0xffff0000,
	          0x7fffffff, 0xffffffff)

	/* MSB one moderate positive and one moderate negative number on a
	   small negative total */
	TEST_MSB (0xffffffff, 0xffffffff,
	          0x00008000, 0xffff0000,
	          0x00000000, 0x7fffffff)

	/* MSB one moderate positive and one moderate negative number on a
	   moderate negative total */
	TEST_MSB (0xfffffffe, 0xffffffff,
	          0x00008000, 0xffff0000,
	          0xffffffff, 0x7fffffff)

	/* MSB one moderate positive and one moderate negative number on a
	   large negative total */
	TEST_MSB (0x80000000, 0x00000000,
	          0x00008000, 0xffff0000,
	          0x80000000, 0x80000000)

	PUTC ('\n')

/* ----------------------------------------------------------------------------
 * All done
 * ------------------------------------------------------------------------- */
_exit:
	LOAD_STR (r3, "Test completed\n")
	l.jal	_puts
	l.nop

	TEST_EXIT
