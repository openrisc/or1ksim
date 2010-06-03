/* lws-test.S. l.lws instruction test of Or1ksim

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
 * Temporary labels up to 49 are reserved for macros and subroutines. Each is
 * used only once in any macro or subroutine. You can get in a serious mess if
 * you get local label clashing in macros. 
 *
 * Arguments to functions are passed in r3 through r8.
 * r9 is the link (return address)
 * r11 is for returning results
 * r2 through r11 are not preserved across calls. All other registers are.
 * ------------------------------------------------------------------------- */


/* ----------------------------------------------------------------------------
 * Memory controller constants
 * ------------------------------------------------------------------------- */

#define MEM_RAM 0x00000000

#define MC_CSR		(0x00)
#define MC_POC		(0x04)
#define MC_BA_MASK	(0x08)
#define MC_CSC(i)	(0x10 + (i) * 8)
#define MC_TMS(i)	(0x14 + (i) * 8)

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
 * Simple stack, will be pointed to by r1, which is the next empty slot
 * ------------------------------------------------------------------------- */
	.section .data
	.balign	4
	.global	_stack
_stack:
	.space	0x1000,0x0

/* ----------------------------------------------------------------------------
 * reset exception
 * ------------------------------------------------------------------------- */
	.section .except, "ax"
	l.addi	  r1,r0,0
	
	.section .text
	.org 0x100
_reset:
	l.movhi	r1,hi(_stack)		/* Set up the stack */
	l.ori	r1,r1,lo(_stack)
	
	l.movhi r3,hi(_init_mc)		/* Code starts with MC setup */
	l.ori	r3,r3,lo(_init_mc)
	l.jr	r3
	l.nop
	
/* ----------------------------------------------------------------------------
 * Subroutine to print out a string
 *
 * The string is followed by a newline
 *
 * Parameters:
 *  r3  Pointer to the string to print
 * ------------------------------------------------------------------------- */
_puts:
	l.add	r2,r0,r3		/* Copy the string pointer */
	
	/* Loop getting and printing each char until end of string */
10:	
	l.lbz	r3,0(r2)
	l.sfeq	r3,r0			/* NULL termination? */
	l.bf	11f

	l.addi	r2,r2,1			/* Delay slot, move to next char */
	l.j	10b			/* Repeat */
	l.nop	NOP_PUTC		/* Delay slot */

11:
	l.jr	r9			/* Return */
	l.nop

/* ----------------------------------------------------------------------------
 * Subroutine to print out a test name prompt
 *
 * The string is preceded by two spaces
 *
 * Parameters:
 *  r3  Pointer to the test name to print
 * ------------------------------------------------------------------------- */
_ptest:
	PUSH(r9)			/* Save the return address */
	PUSH(r3)			/* Save the test name for later */

	LOAD_STR(r3, "  ")		/* Prefix */
	l.jal	_puts
	l.nop

	POP(r3)				/* Test name */
	l.jal	_puts
	l.nop
	
	POP (r9)
	l.jr	r9


/* ----------------------------------------------------------------------------
 * Subroutine to print out "OK"
 *
 * The string is followed by a newline
 * ------------------------------------------------------------------------- */
_pok:
	PUSH(r9)			/* Save the return address */

	LOAD_STR(r3, "OK\n")
	l.jal	_puts
	l.nop

	POP (r9)
	l.jr	r9


/* ----------------------------------------------------------------------------
 * Subroutine to print out "Failed"
 *
 * The string is followed by a ": ", which will then allow a report
 * ------------------------------------------------------------------------- */
_pfail:
	PUSH(r9)			/* Save the return address */

	LOAD_STR(r3, "Failed: ")
	l.jal	_puts
	l.nop

	POP (r9)
	l.jr	r9

/* ----------------------------------------------------------------------------
 * Subroutine to print out "TRUE"
 * ------------------------------------------------------------------------- */
_ptrue:
	PUSH(r9)			/* Save the return address */

	LOAD_STR(r3, "TRUE")
	l.jal	_puts
	l.nop

	POP (r9)
	l.jr	r9


/* ----------------------------------------------------------------------------
 * Subroutine to print out "FALSE"
 * ------------------------------------------------------------------------- */
_pfalse:
	PUSH(r9)			/* Save the return address */

	LOAD_STR(r3, "FALSE")
	l.jal	_puts
	l.nop

	POP (r9)
	l.jr	r9

/* ----------------------------------------------------------------------------
 * Subroutine to print out "unexpected"
 *
 * Preceded by a space and followed by a newline
 * ------------------------------------------------------------------------- */
_punexpected:
	PUSH(r9)			/* Save the return address */

	LOAD_STR(r3, " unexpected\n")
	l.jal	_puts
	l.nop

	POP (r9)
	l.jr	r9

/* ----------------------------------------------------------------------------
 * Memory controller initialization initialization
 * ------------------------------------------------------------------------- */
_init_mc:
 
	l.movhi r3,hi(MC_BASE_ADDR)
	l.ori	r3,r3,lo(MC_BASE_ADDR)
 
	l.addi	r4,r3,MC_CSC(0)
	l.movhi r5,hi(FLASH_BASE_ADDR)
	l.srai	r5,r5,6
	l.ori	r5,r5,0x0025
	l.sw	0(r4),r5
 
	l.addi	r4,r3,MC_TMS(0)
	l.movhi r5,hi(FLASH_TMS_VAL)
	l.ori	r5,r5,lo(FLASH_TMS_VAL)
	l.sw	0(r4),r5
 
	l.addi	r4,r3,MC_BA_MASK
	l.addi	r5,r0,MC_MASK_VAL
	l.sw	0(r4),r5
 
	l.addi	r4,r3,MC_CSR
	l.movhi r5,hi(MC_CSR_VAL)
	l.ori	r5,r5,lo(MC_CSR_VAL)
	l.sw	0(r4),r5
 
	l.addi	r4,r3,MC_TMS(1)
	l.movhi r5,hi(SDRAM_TMS_VAL)
	l.ori	r5,r5,lo(SDRAM_TMS_VAL)
	l.sw	0(r4),r5
 
	l.addi	r4,r3,MC_CSC(1)
	l.movhi r5,hi(SDRAM_BASE_ADDR)
	l.srai	r5,r5,6
	l.ori	r5,r5,0x0411
	l.sw	0(r4),r5
 
/* ----------------------------------------------------------------------------
 * Test of load single word and extend with sign: l.lws
 * ------------------------------------------------------------------------- */
	.section .rodata
50:	.word	0xdeadbeef
51:	.word	0x00000000
52:	.word	0x7fffffff
53:	.word	0x80000000
54:	.word	0xffffffff

	.section .text
_lws:
	LOAD_STR (r3, "l.lws\n")
	l.jal	_puts
	l.nop

	/* Load with zero offset */
	LOAD_CONST (r5,50b)
	l.lws	r4,0(r5)
	CHECK_RES (" l.lws r4,0(r5): r4=0xdeadbeef:  ", r4, 0xdeadbeef)
	
	LOAD_CONST (r5,51b)
	l.lws	r4,0(r5)
	CHECK_RES (" l.lws r4,0(r5): r4=0x00000000:  ", r4, 0x00000000)
	
	LOAD_CONST (r5,52b)
	l.lws	r4,0(r5)
	CHECK_RES (" l.lws r4,0(r5): r4=0x7fffffff:  ", r4, 0x7fffffff)
	
	LOAD_CONST (r5,53b)
	l.lws	r4,0(r5)
	CHECK_RES (" l.lws r4,0(r5): r4=0x80000000:  ", r4, 0x80000000)
	
	LOAD_CONST (r5,54b)
	l.lws	r4,0(r5)
	CHECK_RES (" l.lws r4,0(r5): r4=0xffffffff:  ", r4, 0xffffffff)

	/* Load with positive offset */
	LOAD_CONST (r5,50b)
	l.lws	r4,4(r5)
	CHECK_RES (" l.lws r4,0(r5): r4=0x00000000:  ", r4, 0x00000000)
	
	LOAD_CONST (r5,50b)
	l.lws	r4,8(r5)
	CHECK_RES (" l.lws r4,0(r5): r4=0x7fffffff:  ", r4, 0x7fffffff)
	
	LOAD_CONST (r5,50b)
	l.lws	r4,12(r5)
	CHECK_RES (" l.lws r4,0(r5): r4=0x80000000:  ", r4, 0x80000000)
	
	LOAD_CONST (r5,50b)
	l.lws	r4,16(r5)
	CHECK_RES (" l.lws r4,0(r5): r4=0xffffffff:  ", r4, 0xffffffff)

	/* Load with negative offset */
	LOAD_CONST (r5,54b)
	l.lws	r4,-16(r5)
	CHECK_RES (" l.lws r4,0(r5): r4=0xdeadbeef:  ", r4, 0xdeadbeef)
	
	LOAD_CONST (r5,54b)
	l.lws	r4,-12(r5)
	CHECK_RES (" l.lws r4,0(r5): r4=0x00000000:  ", r4, 0x00000000)
	
	LOAD_CONST (r5,54b)
	l.lws	r4,-8(r5)
	CHECK_RES (" l.lws r4,0(r5): r4=0x7fffffff:  ", r4, 0x7fffffff)
	
	LOAD_CONST (r5,54b)
	l.lws	r4,-4(r5)
	CHECK_RES (" l.lws r4,0(r5): r4=0x80000000:  ", r4, 0x80000000)

/* ----------------------------------------------------------------------------
 * All done
 * ------------------------------------------------------------------------- */
_exit:
	LOAD_STR (r3, "Test completed\n")
	l.jal	_puts
	l.nop

	l.movhi	r3,hi(ALL_DONE)
	l.ori   r3,r3,lo(ALL_DONE)
	l.nop	NOP_REPORT		/* Should be 0xdeaddead */

	l.addi	r3,r0,0
	l.nop	NOP_EXIT
