/* inst-set-test.h. Macros for instruction set testing

   Copyright (C) 1999-2006 OpenCores
   Copyright (C) 2010 Embecosm Limited

   Contributors various OpenCores participants
   Contributor Jeremy Bennett <jeremy.bennett@embecosm.com>

   This file is part of OpenRISC 1000 Architectural Simulator.

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the Free
   Software Foundation; either version 3 of the License, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
   more details.

   You should have received a copy of the GNU General Public License along
   with this program.  If not, see <http:  www.gnu.org/licenses/>.  */


/* ----------------------------------------------------------------------------
 * Test coverage
 *
 * The l.lws instruction was omitted from Or1ksim originally. It is specified
 * for ORBIS32, even though it is functionally equivalent to l.lwz.
 *
 * Having fixed the problem, this is (in good software engineering style), a
 * regresison test to go with the fix.
 *
 * Of course what is really needed is a comprehensive instruction test...
 * ------------------------------------------------------------------------- */


#include "spr-defs.h"
#include "board.h"

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
 * Useful constants
 * ------------------------------------------------------------------------- */

/* Indicator of completion */
#define  ALL_DONE     (0xdeaddead)

/* Logical values */
#define TRUE   1
#define FALSE  0


/* ----------------------------------------------------------------------------
 * Macro to push a register onto the stack
 *
 * r1 points to the next free slot. Push the supplied register on, then
 * advance the stack pointer.
 *
 * Arguments:
 *   reg  The register to push
 *
 * Registers modified
 *   r1
 * ------------------------------------------------------------------------- */
#define PUSH(reg)							 \
	l.sw	0(r1),reg		/* Push */			;\
	l.addi	r1,r1,4			/* Advance the stack */
	
/* ----------------------------------------------------------------------------
 * Macro to pop a register off the stack
 *
 * r1 points to the next free slot. Decrement the stack pointer, then pop the
 * requested register.
 *
 * Arguments:
 *   reg  The register to pop
 *
 * Registers modified
 *   r1
 * ------------------------------------------------------------------------- */
#define POP(reg)							 \
	l.addi	r1,r1,-4		/* Decrement the stack */	;\
	l.lws	reg,0(r1)		/* Pop */
	
/* ----------------------------------------------------------------------------
 * Macro to load a 32-bit constant into a register
 *
 * Arguments:
 *   reg  The register to load
 *   val  The value to load
 *
 * ------------------------------------------------------------------------- */
#define LOAD_CONST(reg,val)						 \
	l.movhi	reg,hi(val)						;\
	l.ori	reg,reg,lo(val)
	
/* ----------------------------------------------------------------------------
 * Macro to define and load a pointer to a string
 *
 * Arguments:
 *   reg  The register to load
 *   str  The string
 *
 * ------------------------------------------------------------------------- */
#define LOAD_STR(reg,str)						 \
	.section .rodata						;\
1:									;\
	.string	str							;\
									;\
	.section .text							;\
	l.movhi	reg,hi(1b)						;\
	l.ori	reg,reg,lo(1b)
	
/* ----------------------------------------------------------------------------
 * Macro to print a character
 *
 * Arguments:
 *   c  The character to print
 * ------------------------------------------------------------------------- */
#define PUTC(c)								 \
	l.addi	r3,r0,c							;\
	l.nop	NOP_PUTC
	
/* ----------------------------------------------------------------------------
 * Macro for recording the result of a test
 *
 * The test result is in r4. Print out the name of test indented two spaces,
 * followed by  ": ", either "OK" or "Failed" and a newline.
 *
 * Arguments:
 *   str  Textual name of the test
 *   reg  The result to test (not r2)
 *   val  Desired result of the test
 * ------------------------------------------------------------------------- */
#define CHECK_RES(str,reg,val)						 \
	.section .rodata						;\
2:									;\
	.string	str							;\
									;\
	.section .text							;\
	PUSH (reg)			/* Save the register to test */	;\
									;\
	LOAD_CONST (r3,2b)		/* Print out the string */	;\
	l.jal	_ptest							;\
	l.nop								;\
									;\
	LOAD_CONST(r2,val)		/* The desired result */	;\
	POP (reg)			/* The register to test */	;\
	PUSH (reg)			/* May need again later */	;\
	l.sfeq	r2,reg			/* Does the result match? */	;\
	l.bf	3f							;\
	l.nop								;\
									;\
	l.jal	_pfail			/* Test failed */		;\
	l.nop								;\
	POP (reg)			/* Report the register */	;\
	l.add	r3,r0,reg						;\
	l.j	4f							;\
	l.nop	NOP_REPORT						;\
3:									;\
	POP (reg)			/* Discard the register */	;\
	l.jal	_pok			/* Test succeeded */		;\
	l.nop								;\
4:
	
/* ----------------------------------------------------------------------------
 * Macro for recording the result of a comparison
 *
 * If the flag is set print the string argument indented by 2 spaces, followed
 * by "TRUE" and a  newline, otherwise print the string argument indented by
 * two spaces, followed by "FALSE" and a newline.
 *
 * Arguments:
 *   str  Textual name of the test
 *   res  Expected result (TRUE or FALSE)
 * ------------------------------------------------------------------------- */
#define CHECK_FLAG(str,res)						 \
	.section .rodata						;\
5:									;\
	.string	str							;\
									;\
	.section .text							;\
	l.bnf	7f			/* Branch if result FALSE */	;\
									;\
	/* Branch for TRUE result */					;\
	LOAD_CONST (r3,5b)		/* The string to print */	;\
	l.jal	_ptest							;\
	l.nop								;\
									;\
	l.addi	r2,r0,TRUE		/* Was it expected? */		;\
	l.addi	r3,r0,res						;\
	l.sfeq	r2,r3							;\
	l.bnf	6f			/* Branch if not expected */	;\
									;\
	/* Sub-branch for TRUE found and expected */			;\
	l.jal	_ptrue							;\
	l.nop								;\
	PUTC ('\n')							;\
	l.j	9f							;\
	l.nop								;\
6:									;\
	/* Sub-branch for TRUE found and not expected */		;\
	l.jal	_ptrue							;\
	l.nop								;\
	l.jal	_punexpected						;\
	l.nop								;\
	l.j	9f							;\
	l.nop								;\
									;\
7:									;\
	/* Branch for FALSE result */					;\
	LOAD_CONST (r3,5b)		/* The string to print */	;\
	l.jal	_ptest							;\
	l.nop								;\
									;\
	l.addi	r2,r0,FALSE		/* Was it expected? */		;\
	l.addi	r3,r0,res						;\
	l.sfeq	r2,r3							;\
	l.bnf	8f			/* Branch if not expected */	;\
									;\
	/* Sub-branch for FALSE found and expected */			;\
	l.jal	_pfalse							;\
	l.nop								;\
	PUTC ('\n')							;\
	l.j	9f							;\
	l.nop								;\
8:									;\
	/* Sub-branch for FALSE found and not expected */		;\
	l.jal	_pfalse							;\
	l.nop								;\
	l.jal	_punexpected						;\
	l.nop								;\
9:

/* ----------------------------------------------------------------------------
 * Macro to report 0xdeaddead and then terminate
 * ------------------------------------------------------------------------- */
#define TEST_EXIT							 \
  l.movhi	r3,hi(ALL_DONE)						;\
	l.ori   r3,r3,lo(ALL_DONE)					;\
	l.nop	NOP_REPORT						;\
									;\
	l.addi	r3,r0,0							;\
	l.nop	NOP_EXIT
