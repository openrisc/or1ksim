/* fp.S. Floating point instruction set test of Or1ksim

   Copyright (C) 1999-2006 OpenCores
   Copyright (C) 2010 Embecosm Limited
   Copyright (C) 2010 ORSoC AB	

   Contributors various OpenCores participants
   Contributor Jeremy Bennett <jeremy.bennett@embecosm.com>
   Contributor Julius Baxter <julius.baxter@orsoc.se>

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
 * At present the tests are only reasonably comprehensive for lf.add.s and
 * lf.div.s. This is intended as a template for future work, and still has
 * omissions even so.
 *
 * Basic tests are provided for all single precision floating point
 * numbers. Since Or1ksim translates these operations into host FP, there is
 * little point in testing more of the subtleties of IEEE 745.
 *
 * However in future this test could be used with OpenRISC hardware, in which
 * case exhaustive testing would be essential.
 * ------------------------------------------------------------------------- */


#include "spr-defs.h"
#include "board.h"

/* ----------------------------------------------------------------------------
 * Coding conventions
 *
v * A simple rising stack is provided to starting at _stack and pointed to by
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

/* ----------------------------------------------------------------------------
 * Floating point constants (IEEE 754)
 *
 * For the record, the single precision format has 1 sign bit, 8 exponent bits
 * and 23 fraction bits.
 *
 *   seeeeeeeefffffffffffffffffffffff
 *
 * The exponent is biased by 2^7 - 1 (i.e 127 is subtracted from the value to
 * give the actual exponent). Normalized numbers have a non-zero exponent (up
 * to 2^8 - 2) and an implicit 1 before the fraction. Thus the value
 * represented is 
 *
 *   (-1)^s * 1.fffffffffffffffffffffff * 2^(eeeeeeee - (2^7 - 1))
 *
 * Special values used are:
 *
 *  Zeroes                Exponent is zero,    fraction also zero
 *  Denormalized numbers  Exponent is zero,    fraction non-zero
 *  Infinities            Exponent is 2^8 - 1, fraction is zero
 *  NaNs                  Exponent is 2^8 - 1, fraction is non-zero
 *
 * The top bit of the fraction in a NaN is often used to indicate the type of
 * NaN. If 1, it is a "quiet" NaN, if 0 it is a "signalling" NaN. Quiet NaN's
 * are generally propagated for FP errors. Signalling NaN's can be used for
 * more unusual purposes
 *
 * In general any other bits of the NaN fraction are just propagated unchanged.
 * ------------------------------------------------------------------------- */

#define FP_S_P_ZERO    (0x00000000)	/* Positive zero */
#define FP_S_N_ZERO    (0x80000000)	/* Positive zero */
#define FP_S_P_INF     (0x7f800000)	/* Positive infinity */
#define FP_S_N_INF     (0xff800000)	/* Negative infinity */
#define FP_S_P_NAN     (0x7fc00000)	/* Positive qNaN */
#define FP_S_N_NAN     (0xffc00000)	/* Negative qNaN */

/* Some NaNs without all fraction bits set (permitted under IEEE 754) */
#define FP_S_P_NAN_B   (0x7f800001)	/* Positive NaN, not all 1s in frac */
#define FP_S_N_NAN_B   (0xff800001)	/* Negative NaN, not all 1s in frac */

/* Some single precision normalized numbers */
#define  FP_S_0_5      (0x3f000000)	/* +0.5 */
#define  FP_S_ONE      (0x3f800000)	/* +1.0 */
#define  FP_S_1_5      (0x3fc00000)	/* +1.5 */
#define  FP_S_TWO      (0x40000000)	/* +2.0 */
#define  FP_S_THREE    (0x40400000)	/* +3.0 */
#define  FP_S_HUGE1    (0x7e800000)	/* +1.0 * 2^+126 */
#define  FP_S_HUGE2    (0x7f000000)	/* +1.0 * 2^+127 */
#define  FP_S_N_0_5    (0xbf000000)	/* -0.5 */
#define  FP_S_N_ONE    (0xbf800000)	/* -1.0 */
#define  FP_S_N_1_5    (0xbfc00000)	/* -1.5 */
#define  FP_S_N_TWO    (0xc0000000)	/* -2.0 */
#define  FP_S_N_THREE  (0xc0400000)	/* -3.0 */
#define  FP_S_N_HUGE1  (0xfe800000)	/* -1.0 * 2^+126 */
#define  FP_S_N_HUGE2  (0xff000000)	/* -1.0 * 2^+127 */

/* Some denormalized numbers */
#define  FP_S_SMALL1   (0x00200000)	/* +1.0 * 2^-129 */
#define  FP_S_SMALL2   (0x00400000)	/* +1.0 * 2^-128 */
#define  FP_S_N_SMALL1 (0x80200000)	/* -1.0 * 2^-129 */
#define  FP_S_N_SMALL2 (0x80400000)	/* -1.0 * 2^-128 */

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
 * Macro for setting rounding mode in FPCSR
 *
 * Arguments:
 *   rm  round mode macro from spr-defs.h
 * ------------------------------------------------------------------------- */
#define SET_RM(rm)						 \
	l.mfspr r4, r0, SPR_FPCSR 				;\
	/* Clear rounding mode bits */				;\
	l.ori 	r4, r4, SPR_FPCSR_RM				;\
	l.xori 	r4, r4, SPR_FPCSR_RM				;\
	/* Set desired rounding mode bits */			;\
	l.ori r4, r4, rm					;\
	l.mtspr r0, r4, SPR_FPCSR


	
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
	.section .except,"ax"

	.org 0x100
_reset:
    // Clear R0 on start-up. There is no guarantee that R0 is hardwired to zero,
    // and indeed it is not when simulating the or1200 Verilog core.
    l.andi  r0,r0,0x0
        
	l.movhi	r1,hi(_stack)		/* Set up the stack */
	l.ori	r1,r1,lo(_stack)
		
	l.movhi r3,hi(start)		/* Jump to test start */
	l.ori	r3,r3,lo(start)
	l.jr	r3
	l.nop

	.org 0x700
illegal_insn:
	l.nop
	.section .rodata						;\
1:									;\
	.string	"Illegal instruction. Enable hardware FPU.\n" 		;\
									;\
	.section .except,"ax"							;\
	l.movhi	r3,hi(1b)						;\
	l.ori	r3,r3,lo(1b)
	l.jal	_puts
	l.nop

	.section .rodata						;\
1:									;\
	.string	"Exiting.\n"						;\
									;\
	.section .except,"ax"							;\
	l.movhi	r3,hi(1b)						;\
	l.ori	r3,r3,lo(1b)
	l.jal	_puts
	l.nop

	l.addi	r3,r0,1
	l.nop	NOP_EXIT

	
	.section .text
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
 * Start of tests
 * ------------------------------------------------------------------------- */
	
start:	
	
/* ----------------------------------------------------------------------------
 * Test of single precision add: lf.add.s
 * ------------------------------------------------------------------------- */

_add_s:
	LOAD_STR (r3, "lf.add.s\n")
	l.jal	_puts
	l.nop

	/* Simple integer addition */
	LOAD_CONST (r5,FP_S_ONE)
	LOAD_CONST (r6,FP_S_P_ZERO)
	lf.add.s  r4,r5,r6
	CHECK_RES (" 1.0 +  0.0  =  1.0:  ", r4, FP_S_ONE)
	
	LOAD_CONST (r5,FP_S_ONE)
	LOAD_CONST (r6,FP_S_N_ZERO)
	lf.add.s  r4,r5,r6
	CHECK_RES (" 1.0 + -0.0  =  1.0:  ", r4, FP_S_ONE)
	
	LOAD_CONST (r5,FP_S_ONE)
	LOAD_CONST (r6,FP_S_ONE)
	lf.add.s  r4,r5,r6
	CHECK_RES (" 1.0 +  1.0  =  2.0:  ", r4, FP_S_TWO)

	LOAD_CONST (r5,FP_S_TWO)
	LOAD_CONST (r6,FP_S_ONE)
	lf.add.s  r4,r5,r6
	CHECK_RES (" 2.0 +  1.0  =  3.0:  ", r4, FP_S_THREE)

	/* Fractional addition */
	LOAD_CONST (r5,FP_S_1_5)
	LOAD_CONST (r6,FP_S_1_5)
	lf.add.s  r4,r5,r6
	CHECK_RES (" 1.5 +  1.5  =  3.0:  ", r4, FP_S_THREE)

	/* Addition with negative numbers */
	LOAD_CONST (r5,FP_S_TWO)
	LOAD_CONST (r6,FP_S_N_ONE)
	lf.add.s  r4,r5,r6
	CHECK_RES (" 2.0 + -1.0  =  1.0:  ", r4, FP_S_ONE)

	LOAD_CONST (r5,FP_S_ONE)
	LOAD_CONST (r6,FP_S_N_TWO)
	lf.add.s  r4,r5,r6
	CHECK_RES (" 1.0 + -2.0  = -1.0:  ", r4, FP_S_N_ONE)

	LOAD_CONST (r5,FP_S_ONE)
	LOAD_CONST (r6,FP_S_N_ONE)
	lf.add.s  r4,r5,r6
	CHECK_RES (" 1.0 + -1.0  = +0.0:  ", r4, FP_S_P_ZERO)

	/* Addition with infinities */
	LOAD_CONST (r5,FP_S_ONE)
	LOAD_CONST (r6,FP_S_P_INF)
	lf.add.s  r4,r5,r6
	CHECK_RES (" 1.0 + +inf  = +inf:  ", r4, FP_S_P_INF)

	LOAD_CONST (r5,FP_S_ONE)
	LOAD_CONST (r6,FP_S_N_INF)
	lf.add.s  r4,r5,r6
	CHECK_RES (" 1.0 + -inf  = -inf:  ", r4, FP_S_N_INF)

	LOAD_CONST (r5,FP_S_P_INF)
	LOAD_CONST (r6,FP_S_P_INF)
	lf.add.s  r4,r5,r6
	CHECK_RES ("+inf + +inf  = +inf:  ", r4, FP_S_P_INF)

	LOAD_CONST (r5,FP_S_P_INF)
	LOAD_CONST (r6,FP_S_N_INF)
	lf.add.s  r4,r5,r6
	CHECK_RES ("+inf + -inf  = -NaN:  ", r4, FP_S_N_NAN)

	LOAD_CONST (r5,FP_S_N_INF)
	LOAD_CONST (r6,FP_S_N_INF)
	lf.add.s  r4,r5,r6
	CHECK_RES ("-inf + -inf  = -inf:  ", r4, FP_S_N_INF)

	/* Addition with NaNs */
	LOAD_CONST (r5,FP_S_ONE)
	LOAD_CONST (r6,FP_S_P_NAN)
	lf.add.s  r4,r5,r6
	CHECK_RES (" 1.0 + +NaN  = +NaN:  ", r4, FP_S_P_NAN)

	LOAD_CONST (r5,FP_S_ONE)
	LOAD_CONST (r6,FP_S_N_NAN)
	lf.add.s  r4,r5,r6
	CHECK_RES (" 1.0 + -NaN  = -NaN:  ", r4, FP_S_N_NAN)

	LOAD_CONST (r5,FP_S_P_NAN)
	LOAD_CONST (r6,FP_S_P_NAN)
	lf.add.s  r4,r5,r6
	CHECK_RES ("+NaN + +NaN  = +NaN:  ", r4, FP_S_P_NAN)

	LOAD_CONST (r5,FP_S_P_NAN)
	LOAD_CONST (r6,FP_S_N_NAN)
	lf.add.s  r4,r5,r6
	CHECK_RES ("+NaN + -NaN  = +NaN:  ", r4, FP_S_P_NAN)

	LOAD_CONST (r5,FP_S_N_NAN)
	LOAD_CONST (r6,FP_S_N_NAN)
	lf.add.s  r4,r5,r6
	CHECK_RES ("-NaN + -NaN  = -NaN:  ", r4, FP_S_N_NAN)

	LOAD_CONST (r5,FP_S_ONE)
	LOAD_CONST (r6,FP_S_P_NAN_B)
	lf.add.s  r4,r5,r6
	CHECK_RES (" 1.0 + +sNaN = +qNaN: ", r4, 0x7fc00001)

	LOAD_CONST (r5,FP_S_ONE)
	LOAD_CONST (r6,FP_S_N_NAN_B)
	lf.add.s  r4,r5,r6
	CHECK_RES (" 1.0 + -sNaN = -qNaN: ", r4, 0xffc00001)

	/* Addition with overflow */
	LOAD_CONST (r5,FP_S_HUGE2)
	LOAD_CONST (r6,FP_S_HUGE2)
	lf.add.s  r4,r5,r6
	CHECK_RES (" 1.0 * 2^127 +  1.0 * 2^127  = +inf:  ", r4, FP_S_P_INF)

	LOAD_CONST (r5,FP_S_N_HUGE2)
	LOAD_CONST (r6,FP_S_N_HUGE2)
	lf.add.s  r4,r5,r6
	CHECK_RES ("-1.0 * 2^127 + -1.0 * 2^127  = -inf:  ", r4, FP_S_N_INF)

/* ----------------------------------------------------------------------------
 * Test of single precision custom instruction: lf.cust1.s
 * ------------------------------------------------------------------------- */
_cust1_s:
	LOAD_STR (r3, "lf.cust1.s\n")
	l.jal	_puts
	l.nop

	/* Just test that the instruction does nothing */
	LOAD_CONST (r4,FP_S_ONE)
	LOAD_CONST (r5,FP_S_TWO)
	lf.cust1.s  r4,r5		/* Should do nothing */
	CHECK_RES ("l.cust1: ", r4, FP_S_ONE)
		
/* ----------------------------------------------------------------------------
 * Test of single precision divide instruction: lf.div.s
 * ------------------------------------------------------------------------- */
_div_s:
	LOAD_STR (r3, "lf.div.s\n")
	l.jal	_puts
	l.nop

	/* Tests of integer division */
	LOAD_CONST (r5,FP_S_ONE)
	LOAD_CONST (r6,FP_S_ONE)
	lf.div.s  r4,r5,r6
	CHECK_RES (" 1.0 /  1.0 =  1.0: ", r4, FP_S_ONE)

	LOAD_CONST (r5,FP_S_TWO)
	LOAD_CONST (r6,FP_S_ONE)
	lf.div.s  r4,r5,r6
	CHECK_RES (" 2.0 /  1.0 =  2.0: ", r4, FP_S_TWO)

	/* Test division with fractional result */
	LOAD_CONST (r5,FP_S_THREE)
	LOAD_CONST (r6,FP_S_TWO)
	lf.div.s  r4,r5,r6
	CHECK_RES (" 3.0 /  2.0 =  1.5: ", r4, FP_S_1_5)

	/* Test division of zero */
	LOAD_CONST (r5,FP_S_P_ZERO)
	LOAD_CONST (r6,FP_S_ONE)
	lf.div.s  r4,r5,r6
	CHECK_RES ("+0.0 /  1.0 = +0.0: ", r4, FP_S_P_ZERO)

	LOAD_CONST (r5,FP_S_N_ZERO)
	LOAD_CONST (r6,FP_S_ONE)
	lf.div.s  r4,r5,r6
	CHECK_RES ("-0.0 /  1.0 = -0.0: ", r4, FP_S_N_ZERO)

	/* Test signed division */
	LOAD_CONST (r5,FP_S_N_THREE)
	LOAD_CONST (r6,FP_S_TWO)
	lf.div.s  r4,r5,r6
	CHECK_RES ("-3.0 /  2.0 = -1.5: ", r4, FP_S_N_1_5)

	LOAD_CONST (r5,FP_S_THREE)
	LOAD_CONST (r6,FP_S_N_TWO)
	lf.div.s  r4,r5,r6
	CHECK_RES (" 3.0 / -2.0 = -1.5: ", r4, FP_S_N_1_5)

	LOAD_CONST (r5,FP_S_N_THREE)
	LOAD_CONST (r6,FP_S_N_TWO)
	lf.div.s  r4,r5,r6
	CHECK_RES ("-3.0 / -2.0 = -1.5: ", r4, FP_S_1_5)

	/* Division by zero */
	LOAD_CONST (r5,FP_S_ONE)
	LOAD_CONST (r6,FP_S_P_ZERO)
	lf.div.s  r4,r5,r6
	CHECK_RES (" 1.0 / +0.0 = +inf: ", r4, FP_S_P_INF)
	
	LOAD_CONST (r5,FP_S_ONE)
	LOAD_CONST (r6,FP_S_P_ZERO)
	lf.div.s  r4,r5,r6
	CHECK_RES (" 1.0 / -0.0 = +inf: ", r4, FP_S_P_INF)
	
	LOAD_CONST (r5,FP_S_N_ONE)
	LOAD_CONST (r6,FP_S_P_ZERO)
	lf.div.s  r4,r5,r6
	CHECK_RES ("-1.0 / +0.0 = -inf: ", r4, FP_S_N_INF)
	
	LOAD_CONST (r5,FP_S_N_ONE)
	LOAD_CONST (r6,FP_S_P_ZERO)
	lf.div.s  r4,r5,r6
	CHECK_RES ("-1.0 / -0.0 = -inf: ", r4, FP_S_N_INF)

	/* Division with infinities */
	LOAD_CONST (r5,FP_S_ONE)
	LOAD_CONST (r6,FP_S_P_INF)
	lf.div.s  r4,r5,r6
	CHECK_RES (" 1.0 / +inf = +0.0: ", r4, FP_S_P_ZERO)
	
	LOAD_CONST (r5,FP_S_ONE)
	LOAD_CONST (r6,FP_S_N_INF)
	lf.div.s  r4,r5,r6
	CHECK_RES (" 1.0 / -inf = -0.0: ", r4, FP_S_N_ZERO)
	
	LOAD_CONST (r5,FP_S_N_ONE)
	LOAD_CONST (r6,FP_S_P_INF)
	lf.div.s  r4,r5,r6
	CHECK_RES ("-1.0 / +inf = -0.0: ", r4, FP_S_N_ZERO)
	
	LOAD_CONST (r5,FP_S_N_ONE)
	LOAD_CONST (r6,FP_S_N_INF)
	lf.div.s  r4,r5,r6
	CHECK_RES ("-1.0 / -inf = +0.0: ", r4, FP_S_P_ZERO)
	
	LOAD_CONST (r5,FP_S_P_INF)
	LOAD_CONST (r6,FP_S_ONE)
	lf.div.s  r4,r5,r6
	CHECK_RES ("+inf /  1.0 = +inf: ", r4, FP_S_P_INF)
	
	LOAD_CONST (r5,FP_S_N_INF)
	LOAD_CONST (r6,FP_S_ONE)
	lf.div.s  r4,r5,r6
	CHECK_RES ("-inf /  1.0 = -inf: ", r4, FP_S_N_INF)
	
	LOAD_CONST (r5,FP_S_P_INF)
	LOAD_CONST (r6,FP_S_N_ONE)
	lf.div.s  r4,r5,r6
	CHECK_RES ("+inf / -1.0 = -inf: ", r4, FP_S_N_INF)
	
	LOAD_CONST (r5,FP_S_N_INF)
	LOAD_CONST (r6,FP_S_N_ONE)
	lf.div.s  r4,r5,r6
	CHECK_RES ("-inf / -1.0 = +inf: ", r4, FP_S_P_INF)
	
	LOAD_CONST (r5,FP_S_P_INF)
	LOAD_CONST (r6,FP_S_P_INF)
	lf.div.s  r4,r5,r6
	CHECK_RES ("+inf / +inf = -NaN: ", r4, FP_S_N_NAN)
	
	LOAD_CONST (r5,FP_S_P_INF)
	LOAD_CONST (r6,FP_S_N_INF)
	lf.div.s  r4,r5,r6
	CHECK_RES ("+inf / -inf = -NaN: ", r4, FP_S_N_NAN)
	
	LOAD_CONST (r5,FP_S_N_INF)
	LOAD_CONST (r6,FP_S_P_INF)
	lf.div.s  r4,r5,r6
	CHECK_RES ("-inf / +inf = -NaN: ", r4, FP_S_N_NAN)
	
	LOAD_CONST (r5,FP_S_N_INF)
	LOAD_CONST (r6,FP_S_N_INF)
	lf.div.s  r4,r5,r6
	CHECK_RES ("-inf / -inf = -NaN: ", r4, FP_S_N_NAN)
	
	/* Division with NaNs */
	LOAD_CONST (r5,FP_S_ONE)
	LOAD_CONST (r6,FP_S_P_NAN)
	lf.div.s  r4,r5,r6
	CHECK_RES (" 1.0 / +NaN = +NaN: ", r4, FP_S_P_NAN)
		
	LOAD_CONST (r5,FP_S_ONE)
	LOAD_CONST (r6,FP_S_N_NAN)
	lf.div.s  r4,r5,r6
	CHECK_RES (" 1.0 / -NaN = -NaN: ", r4, FP_S_N_NAN)
		
	LOAD_CONST (r5,FP_S_N_ONE)
	LOAD_CONST (r6,FP_S_P_NAN)
	lf.div.s  r4,r5,r6
	CHECK_RES ("-1.0 / +NaN = +NaN: ", r4, FP_S_P_NAN)
		
	LOAD_CONST (r5,FP_S_N_ONE)
	LOAD_CONST (r6,FP_S_N_NAN)
	lf.div.s  r4,r5,r6
	CHECK_RES ("-1.0 / -NaN = 1NaN: ", r4, FP_S_N_NAN)
		
	LOAD_CONST (r5,FP_S_P_NAN)
	LOAD_CONST (r6,FP_S_ONE)
	lf.div.s  r4,r5,r6
	CHECK_RES ("+NaN /  1.0 = +NaN: ", r4, FP_S_P_NAN)
		
	LOAD_CONST (r5,FP_S_N_NAN)
	LOAD_CONST (r6,FP_S_ONE)
	lf.div.s  r4,r5,r6
	CHECK_RES ("-NaN /  1.0 = -NaN: ", r4, FP_S_N_NAN)
		
	LOAD_CONST (r5,FP_S_P_NAN)
	LOAD_CONST (r6,FP_S_N_ONE)
	lf.div.s  r4,r5,r6
	CHECK_RES ("+NaN / -1.0 = +NaN: ", r4, FP_S_P_NAN)
		
	LOAD_CONST (r5,FP_S_N_NAN)
	LOAD_CONST (r6,FP_S_N_ONE)
	lf.div.s  r4,r5,r6
	CHECK_RES ("-NaN / -1.0 = -NaN: ", r4, FP_S_N_NAN)
		
	LOAD_CONST (r5,FP_S_P_NAN)
	LOAD_CONST (r6,FP_S_P_NAN)
	lf.div.s  r4,r5,r6
	CHECK_RES ("+NaN / +NaN = +NaN: ", r4, FP_S_P_NAN)
		
	LOAD_CONST (r5,FP_S_P_NAN)
	LOAD_CONST (r6,FP_S_N_NAN)
	lf.div.s  r4,r5,r6
	CHECK_RES ("+NaN / -NaN = +NaN: ", r4, FP_S_P_NAN)
		
	LOAD_CONST (r5,FP_S_N_NAN)
	LOAD_CONST (r6,FP_S_P_NAN)
	lf.div.s  r4,r5,r6
	CHECK_RES ("-NaN / +NaN = -NaN: ", r4, FP_S_N_NAN)
		
	LOAD_CONST (r5,FP_S_N_NAN)
	LOAD_CONST (r6,FP_S_N_NAN)
	lf.div.s  r4,r5,r6
	CHECK_RES ("-NaN / -NaN = -NaN: ", r4, FP_S_N_NAN)

	/* Division with overflow */
	LOAD_CONST (r5,FP_S_HUGE2)
	LOAD_CONST (r6,FP_S_SMALL1)
	lf.div.s  r4,r5,r6
	CHECK_RES (" 1.0 * 2^127  /  1.0 * 2^-129 = +inf: ", r4, FP_S_P_INF)
		
	LOAD_CONST (r5,FP_S_N_HUGE2)
	LOAD_CONST (r6,FP_S_SMALL1)
	lf.div.s  r4,r5,r6
	CHECK_RES ("-1.0 * 2^127  /  1.0 * 2^-129 = -inf: ", r4, FP_S_N_INF)
		
	LOAD_CONST (r5,FP_S_HUGE2)
	LOAD_CONST (r6,FP_S_N_SMALL1)
	lf.div.s  r4,r5,r6
	CHECK_RES (" 1.0 * 2^127  / -1.0 * 2^-129 = -inf: ", r4, FP_S_N_INF)
		
	LOAD_CONST (r5,FP_S_N_HUGE2)
	LOAD_CONST (r6,FP_S_N_SMALL1)
	lf.div.s  r4,r5,r6
	CHECK_RES ("-1.0 * 2^127  / -1.0 * 2^-129 = +inf: ", r4, FP_S_P_INF)

	/* Division with underflow */
	LOAD_CONST (r5,FP_S_SMALL1)
	LOAD_CONST (r6,FP_S_HUGE1)
	lf.div.s  r4,r5,r6
	CHECK_RES (" 1.0 * 2^-129 /  1.0 * 2^127  = +0.0: ", r4, FP_S_P_ZERO)

	LOAD_CONST (r5,FP_S_N_SMALL1)
	LOAD_CONST (r6,FP_S_HUGE1)
	lf.div.s  r4,r5,r6
	CHECK_RES ("-1.0 * 2^-129 /  1.0 * 2^127  = -0.0: ", r4, FP_S_N_ZERO)

	LOAD_CONST (r5,FP_S_SMALL1)
	LOAD_CONST (r6,FP_S_N_HUGE1)
	lf.div.s  r4,r5,r6
	CHECK_RES (" 1.0 * 2^-129 / -1.0 * 2^127  = -0.0: ", r4, FP_S_N_ZERO)

	LOAD_CONST (r5,FP_S_N_SMALL1)
	LOAD_CONST (r6,FP_S_N_HUGE1)
	lf.div.s  r4,r5,r6
	CHECK_RES ("-1.0 * 2^-129 / -1.0 * 2^127  = +0.0: ", r4, FP_S_P_ZERO)

	/* Needs tests of normalization to denormalization */
		
/* ----------------------------------------------------------------------------
 * Test of single precision fp to integer conversion: lf.ftoi.s
 * ------------------------------------------------------------------------- */
v_ftoi_s:
	LOAD_STR (r3, "lf.ftoi.s\n")
	l.jal	_puts
	l.nop

	/* Integer conversion */
	LOAD_CONST (r5,FP_S_ONE)
	lf.ftoi.s  r4,r5
	CHECK_RES ("(int)  1.0          =  1:    ", r4, 0x00000001)

	LOAD_CONST (r5,FP_S_N_ONE)
	lf.ftoi.s  r4,r5
	CHECK_RES ("(int) -1.0          = -1:    ", r4, 0xffffffff)

	LOAD_CONST (r5,FP_S_P_ZERO)
	lf.ftoi.s  r4,r5
	CHECK_RES ("(int) +0.0          =  0:    ", r4, 0x00000000)

	LOAD_CONST (r5,FP_S_N_ZERO)
	lf.ftoi.s  r4,r5
	CHECK_RES ("(int) -0.0          =  0:    ", r4, 0x00000000)

	/* Fractional conversion (round towards zero) */
	SET_RM	(FPCSR_RM_RZ)
	LOAD_CONST (r5,FP_S_1_5)
	lf.ftoi.s  r4,r5
	CHECK_RES ("(int)  1.5          =  1:    ", r4, 0x00000001)

	LOAD_CONST (r5,FP_S_N_1_5)
	lf.ftoi.s  r4,r5
	CHECK_RES ("(int) -1.5          = -1:    ", r4, 0xffffffff)

	/* Conversion of values too big */
	LOAD_CONST (r5,FP_S_HUGE1)
	lf.ftoi.s  r4,r5
	CHECK_RES ("(int)  1.0 * 2^126  = 2^31: ", r4, 0x7fffffff)

	LOAD_CONST (r5,FP_S_N_HUGE1)
	lf.ftoi.s  r4,r5
	CHECK_RES ("(int) -1.0 * 2^126  = -2^31: ", r4, 0x80000000)

	/* Conversion of very small values */
	LOAD_CONST (r5,FP_S_SMALL1)
	lf.ftoi.s  r4,r5
	CHECK_RES ("(int)  1.0 * 2^-129 =  0:    ", r4, 0x00000000)

	LOAD_CONST (r5,FP_S_N_SMALL1)
	lf.ftoi.s  r4,r5
	CHECK_RES ("(int) -1.0 * 2^-129 =  0:    ", r4, 0x00000000)

	/* Just basic check of Infinity & NaN */
	LOAD_CONST (r5,FP_S_P_INF)
	lf.ftoi.s  r4,r5
	CHECK_RES ("(int) +inf          = 2^31: ", r4, 0x7fffffff)

	LOAD_CONST (r5,FP_S_N_INF)
	lf.ftoi.s  r4,r5
	CHECK_RES ("(int) -inf          = -2^31: ", r4, 0x80000000)

	LOAD_CONST (r5,FP_S_P_NAN)
	lf.ftoi.s  r4,r5
	CHECK_RES ("(int) +NaN          = 2^31: ", r4, 0x7fffffff)

	LOAD_CONST (r5,FP_S_N_NAN)
	lf.ftoi.s  r4,r5
	CHECK_RES ("(int) -NaN          = 2^31: ", r4, 0x7fffffff)

/* ----------------------------------------------------------------------------
 * Test of single precision integer to fp conversion: lf.itof.s
 * ------------------------------------------------------------------------- */
_itof_s:
	LOAD_STR (r3, "lf.itof.s\n")
	l.jal	_puts
	l.nop

	/* Integer conversion */
	LOAD_CONST (r5,FP_S_ONE)
	lf.ftoi.s  r4,r5
	CHECK_RES ("(float)  1.0 =  1: ", r4, 0x00000001)

	LOAD_CONST (r5,FP_S_N_ONE)
	lf.ftoi.s  r4,r5
	CHECK_RES ("(float) -1.0 = -1: ", r4, 0xffffffff)

	LOAD_CONST (r5,FP_S_P_ZERO)
	lf.ftoi.s  r4,r5
	CHECK_RES ("(float) +0.0 =  0: ", r4, 0x00000000)

	LOAD_CONST (r5,FP_S_N_ZERO)
	lf.ftoi.s  r4,r5
	CHECK_RES ("(float) -0.0 =  0: ", r4, 0x00000000)

/* ----------------------------------------------------------------------------
 * Test of single precision multiply and add: lf.madd.s
 * ------------------------------------------------------------------------- */
_madd_s:
	LOAD_STR (r3, "lf.madd.s\n")
	l.jal	_puts
	l.nop

	/* Test of integer multiply and add */
	LOAD_CONST (r4,FP_S_ONE)
	LOAD_CONST (r5,FP_S_ONE)
	LOAD_CONST (r6,FP_S_ONE)
	lf.madd.s  r4,r5,r6
	CHECK_RES (" 1.0 +  1.0 *  1.0 =  2.0: ", r4, FP_S_TWO)

	/* Multiply and add with fractions */
	LOAD_CONST (r4,FP_S_0_5)
	LOAD_CONST (r5,FP_S_ONE)
	LOAD_CONST (r6,FP_S_ONE)
	lf.madd.s  r4,r5,r6
	CHECK_RES (" 0.5 +  1.0 *  1.0 =  1.5: ", r4, FP_S_1_5)

	LOAD_CONST (r4,FP_S_ONE)
	LOAD_CONST (r5,FP_S_0_5)
	LOAD_CONST (r6,FP_S_TWO)
	lf.madd.s  r4,r5,r6
	CHECK_RES (" 1.0 +  0.5 *  2.0 =  2.0: ", r4, FP_S_TWO)

	/* Multiply and add with negative numbers */
	LOAD_CONST (r4,FP_S_ONE)
	LOAD_CONST (r5,FP_S_N_ONE)
	LOAD_CONST (r6,FP_S_TWO)
	lf.madd.s  r4,r5,r6
	CHECK_RES (" 1.0 + -1.0 *  2.0 = -1.0: ", r4, FP_S_N_ONE)

	LOAD_CONST (r4,FP_S_N_TWO)
	LOAD_CONST (r5,FP_S_TWO)
	LOAD_CONST (r6,FP_S_0_5)
	lf.madd.s  r4,r5,r6
	CHECK_RES ("-2.0 +  2.0 *  0.5 = -1.0: ", r4, FP_S_N_ONE)

	LOAD_CONST (r4,FP_S_N_0_5)
	LOAD_CONST (r5,FP_S_N_0_5)
	LOAD_CONST (r6,FP_S_THREE)
	lf.madd.s  r4,r5,r6
	CHECK_RES ("-0.5 + -0.5 *  3.0 = -2.0: ", r4, FP_S_N_TWO)

	/* Multiply and add with zeros (more needed) */
	LOAD_CONST (r4,FP_S_P_ZERO)
	LOAD_CONST (r5,FP_S_P_ZERO)
	LOAD_CONST (r6,FP_S_P_ZERO)
	lf.madd.s  r4,r5,r6
	CHECK_RES ("+0.0 + +0.0 * +0.0 = +0.0: ", r4, FP_S_P_ZERO)

	LOAD_CONST (r4,FP_S_N_ZERO)
	LOAD_CONST (r5,FP_S_N_ZERO)
	LOAD_CONST (r6,FP_S_N_ZERO)
	lf.madd.s  r4,r5,r6
	CHECK_RES ("-0.0 + +0.0 * -0.0 = +0.0: ", r4, FP_S_P_ZERO)

	LOAD_CONST (r4,FP_S_N_ZERO)
	LOAD_CONST (r5,FP_S_N_ZERO)
	LOAD_CONST (r6,FP_S_N_ZERO)
	lf.madd.s  r4,r5,r6
	CHECK_RES ("-0.0 + -0.0 * -0.0 = +0.0: ", r4, FP_S_P_ZERO)

	LOAD_CONST (r4,FP_S_N_ONE)
	LOAD_CONST (r5,FP_S_ONE)
	LOAD_CONST (r6,FP_S_ONE)
	lf.madd.s  r4,r5,r6
	CHECK_RES ("-1.0 +  1.0 *  1.0 = +0.0: ", r4, FP_S_P_ZERO)

	/* Multiply and add with infinities (more needed) */
	LOAD_CONST (r4,FP_S_P_INF)
	LOAD_CONST (r5,FP_S_ONE)
	LOAD_CONST (r6,FP_S_ONE)
	lf.madd.s  r4,r5,r6
	CHECK_RES ("+inf +  1.0 *  1.0 = +inf: ", r4, FP_S_P_INF)

	LOAD_CONST (r4,FP_S_ONE)
	LOAD_CONST (r5,FP_S_ONE)
	LOAD_CONST (r6,FP_S_P_INF)
	lf.madd.s  r4,r5,r6
	CHECK_RES (" 1.0 +  1.0 * +inf = +inf: ", r4, FP_S_P_INF)

	LOAD_CONST (r4,FP_S_P_INF)
	LOAD_CONST (r5,FP_S_ONE)
	LOAD_CONST (r6,FP_S_P_INF)
	lf.madd.s  r4,r5,r6
	CHECK_RES ("+inf +  1.0 * +inf = +inf: ", r4, FP_S_P_INF)

	LOAD_CONST (r4,FP_S_N_INF)
	LOAD_CONST (r5,FP_S_ONE)
	LOAD_CONST (r6,FP_S_ONE)
	lf.madd.s  r4,r5,r6
	CHECK_RES ("-inf +  1.0 *  1.0 = -inf: ", r4, FP_S_N_INF)

	/* Multiply and add with NaNs (more needed) */
	LOAD_CONST (r4,FP_S_P_NAN)
	LOAD_CONST (r5,FP_S_ONE)
	LOAD_CONST (r6,FP_S_ONE)
	lf.madd.s  r4,r5,r6
	CHECK_RES ("+NaN +  1.0 *  1.0 = +NaN: ", r4, FP_S_P_NAN)

	LOAD_CONST (r4,FP_S_ONE)
	LOAD_CONST (r5,FP_S_ONE)
	LOAD_CONST (r6,FP_S_P_NAN)
	lf.madd.s  r4,r5,r6
	CHECK_RES (" 1.0 +  1.0 * +NaN = +NaN: ", r4, FP_S_P_NAN)

	LOAD_CONST (r4,FP_S_P_NAN)
	LOAD_CONST (r5,FP_S_ONE)
	LOAD_CONST (r6,FP_S_P_NAN)
	lf.madd.s  r4,r5,r6
	CHECK_RES ("+NaN +  1.0 * +NaN = +NaN: ", r4, FP_S_P_NAN)

	LOAD_CONST (r4,FP_S_N_NAN)
	LOAD_CONST (r5,FP_S_ONE)
	LOAD_CONST (r6,FP_S_ONE)
	lf.madd.s  r4,r5,r6
	CHECK_RES ("-NaN +  1.0 *  1.0 = -NaN: ", r4, FP_S_N_NAN)

	/* Multiply and add with overflow (more needed) */
	SET_RM	(FPCSR_RM_RIP)
	LOAD_CONST (r4,FP_S_HUGE2)
	LOAD_CONST (r5,FP_S_HUGE2)
	LOAD_CONST (r6,FP_S_ONE)
	lf.madd.s  r4,r5,r6
	CHECK_RES (" 1.0 * 2^127  +  1.0 * 2^127  *  1.0 = +inf: ",
	           r4, FP_S_P_INF)

	SET_RM	(FPCSR_RM_RIN)
	LOAD_CONST (r4,FP_S_HUGE2)
	LOAD_CONST (r5,FP_S_HUGE2)
	LOAD_CONST (r6,FP_S_SMALL1)
	lf.madd.s  r4,r5,r6
	CHECK_RES (" 1.0 * 2^127  +  1.0 * 2^127  *  1.0 * 2^-129 = +inf: ",
	           r4, FP_S_HUGE2)

/* ----------------------------------------------------------------------------
 * Test of single precision multiply: lf.mul.s
 * ------------------------------------------------------------------------- */
_mul_s:
	LOAD_STR (r3, "lf.mul.s\n")
	l.jal	_puts
	l.nop

	/* Tests of integer multiplication */
	LOAD_CONST (r5,FP_S_ONE)
	LOAD_CONST (r6,FP_S_ONE)
	lf.mul.s  r4,r5,r6
	CHECK_RES (" 1.0 *  1.0 =  1.0: ", r4, FP_S_ONE)

	LOAD_CONST (r5,FP_S_THREE)
	LOAD_CONST (r6,FP_S_ONE)
	lf.mul.s  r4,r5,r6
	CHECK_RES (" 3.0 *  1.0 =  3.0: ", r4, FP_S_THREE)

	/* Multiplication with fractions */
	LOAD_CONST (r5,FP_S_1_5)
	LOAD_CONST (r6,FP_S_TWO)
	lf.mul.s  r4,r5,r6
	CHECK_RES (" 1.5 *  2.0 =  3.0: ", r4, FP_S_THREE)

	/* Multiplication with negative numbers */
	LOAD_CONST (r5,FP_S_N_ONE)
	LOAD_CONST (r6,FP_S_TWO)
	lf.mul.s  r4,r5,r6
	CHECK_RES ("-1.0 *  2.0 = -2.0: ", r4, FP_S_N_TWO)

	LOAD_CONST (r5,FP_S_N_ONE)
	LOAD_CONST (r6,FP_S_N_TWO)
	lf.mul.s  r4,r5,r6
	CHECK_RES ("-1.0 * -2.0 = +2.0: ", r4, FP_S_TWO)

	/* Multiplication with zeros */
	LOAD_CONST (r5,FP_S_P_ZERO)
	LOAD_CONST (r6,FP_S_TWO)
	lf.mul.s  r4,r5,r6
	CHECK_RES ("+0.0 *  2.0 = +0.0: ", r4, FP_S_P_ZERO)

	LOAD_CONST (r5,FP_S_N_ZERO)
	LOAD_CONST (r6,FP_S_TWO)
	lf.mul.s  r4,r5,r6
	CHECK_RES ("+0.0 *  2.0 = +0.0: ", r4, FP_S_N_ZERO)

	/* Multiplication with infinities (more needed) */
	LOAD_CONST (r5,FP_S_P_INF)
	LOAD_CONST (r6,FP_S_N_TWO)
	lf.mul.s  r4,r5,r6
	CHECK_RES ("+inf * -2.0 = -inf: ", r4, FP_S_N_INF)

	/* Multiplication with NaNs (more needed) */
	LOAD_CONST (r5,FP_S_P_NAN)
	LOAD_CONST (r6,FP_S_TWO)
	lf.mul.s  r4,r5,r6
	CHECK_RES ("+NaN *  2.0 = +NaN: ", r4, FP_S_P_NAN)

	/* Multiplication overflow */
	SET_RM	(FPCSR_RM_RIP)
	
	LOAD_CONST (r5,FP_S_HUGE1)
	LOAD_CONST (r6,FP_S_HUGE1)
	lf.mul.s  r4,r5,r6
	CHECK_RES (" 1.0 * 2^127 *  1.0 * 2^127 = +inf: ", r4, FP_S_P_INF)

	LOAD_CONST (r5,FP_S_N_HUGE1)
	LOAD_CONST (r6,FP_S_N_HUGE1)
	lf.mul.s  r4,r5,r6
	CHECK_RES ("-1.0 * 2^127 * -1.0 * 2^127 = +inf: ", r4, FP_S_P_INF)

	SET_RM	(FPCSR_RM_RIN)
	LOAD_CONST (r5,FP_S_N_HUGE1)
	LOAD_CONST (r6,FP_S_HUGE1)
	lf.mul.s  r4,r5,r6
	CHECK_RES ("-1.0 * 2^127 *  1.0 * 2^127 = -inf: ", r4, FP_S_N_INF)

	LOAD_CONST (r5,FP_S_HUGE1)
	LOAD_CONST (r6,FP_S_N_HUGE1)
	lf.mul.s  r4,r5,r6
	CHECK_RES (" 1.0 * 2^127 * -1.0 * 2^127 = -inf: ", r4, FP_S_N_INF)


/* ----------------------------------------------------------------------------
 * Test of single precision remainder: lf.rem.s
 * ------------------------------------------------------------------------- */
_rem_s:
	LOAD_STR (r3, "lf.rem.s\n")
	l.jal	_puts
	l.nop

	/* Tests of integer remainder */
	LOAD_CONST (r5,FP_S_ONE)
	LOAD_CONST (r6,FP_S_ONE)
	lf.rem.s  r4,r5,r6
	CHECK_RES (" 1.0 %  1.0 = +0.0: ", r4, FP_S_P_ZERO)

	LOAD_CONST (r5,FP_S_ONE)
	LOAD_CONST (r6,FP_S_TWO)
	lf.rem.s  r4,r5,r6
	CHECK_RES (" 1.0 %  2.0 =  1.0: ", r4, FP_S_ONE)

	/* Remainder with fractions */
	// FIXME: This is failing, giving rem = -0.5 with softfloat
	/*
	LOAD_CONST (r5,FP_S_1_5)
	LOAD_CONST (r6,FP_S_TWO)
	lf.rem.s  r4,r5,r6
	CHECK_RES (" 1.5 %  2.0 =  1.5: ", r4, FP_S_1_5)
	*/
	LOAD_CONST (r5,FP_S_TWO)
	LOAD_CONST (r6,FP_S_1_5)
	lf.rem.s  r4,r5,r6
	CHECK_RES (" 2.0 %  1.5 =  0.5: ", r4, FP_S_0_5)

	LOAD_CONST (r5,FP_S_THREE)
	LOAD_CONST (r6,FP_S_1_5)
	lf.rem.s  r4,r5,r6
	CHECK_RES (" 3.0 %  1.5 = +0.0: ", r4, FP_S_P_ZERO)

	/* Remainder with negative numbers */
	// FIXME: These next 4 are failing with the wrong signs on the results!
	/*
	LOAD_CONST (r5,FP_S_N_THREE)
	LOAD_CONST (r6,FP_S_TWO)
	lf.rem.s  r4,r5,r6
	CHECK_RES ("-3.0 %  2.0 = -1.0: ", r4, FP_S_N_ONE)

	LOAD_CONST (r5,FP_S_N_THREE)
	LOAD_CONST (r6,FP_S_TWO)
	lf.rem.s  r4,r5,r6
	CHECK_RES ("-3.0 %  2.0 = -1.0: ", r4, FP_S_N_ONE)

	LOAD_CONST (r5,FP_S_THREE)
	LOAD_CONST (r6,FP_S_N_TWO)
	lf.rem.s  r4,r5,r6
	CHECK_RES (" 3.0 % -2.0 =  1.0: ", r4, FP_S_ONE)

	LOAD_CONST (r5,FP_S_N_THREE)
	LOAD_CONST (r6,FP_S_N_TWO)
	lf.rem.s  r4,r5,r6
	CHECK_RES ("-3.0 % -2.0 = -1.0: ", r4, FP_S_N_ONE)
	*/
	/* Remainder with zeros (more are needed) */
	LOAD_CONST (r5,FP_S_P_ZERO)
	LOAD_CONST (r6,FP_S_TWO)
	lf.rem.s  r4,r5,r6
	CHECK_RES ("+0.0 %  2.0 = +0.0: ", r4, FP_S_P_ZERO)

	LOAD_CONST (r5,FP_S_N_ZERO)
	LOAD_CONST (r6,FP_S_TWO)
	lf.rem.s  r4,r5,r6
	CHECK_RES ("-0.0 %  2.0 = -0.0: ", r4, FP_S_N_ZERO)

	LOAD_CONST (r5,FP_S_TWO)
	LOAD_CONST (r6,FP_S_P_ZERO)
	lf.rem.s  r4,r5,r6
	CHECK_RES (" 2.0 % +0.0 = -NaN: ", r4, FP_S_N_NAN)

	LOAD_CONST (r5,FP_S_N_TWO)
	LOAD_CONST (r6,FP_S_P_ZERO)
	lf.rem.s  r4,r5,r6
	CHECK_RES ("-2.0 % +0.0 = -NaN: ", r4, FP_S_N_NAN)

	LOAD_CONST (r5,FP_S_TWO)
	LOAD_CONST (r6,FP_S_N_ZERO)
	lf.rem.s  r4,r5,r6
	CHECK_RES (" 2.0 % -0.0 = -NaN: ", r4, FP_S_N_NAN)

	/* Remainder with infinities (more are needed) */
	LOAD_CONST (r5,FP_S_TWO)
	LOAD_CONST (r6,FP_S_P_INF)
	lf.rem.s  r4,r5,r6
	CHECK_RES (" 2.0 % +inf =  2.0: ", r4, FP_S_TWO)

	LOAD_CONST (r5,FP_S_P_INF)
	LOAD_CONST (r6,FP_S_TWO)
	lf.rem.s  r4,r5,r6
	CHECK_RES ("+inf %  2.0 = -NaN: ", r4, FP_S_N_NAN)

	/* Remainder with NaNs (more are needed) */
	LOAD_CONST (r5,FP_S_TWO)
	LOAD_CONST (r6,FP_S_P_NAN)
	lf.rem.s  r4,r5,r6
	CHECK_RES (" 2.0 % +NaN = +NaN: ", r4, FP_S_P_NAN)

	LOAD_CONST (r5,FP_S_P_NAN)
	LOAD_CONST (r6,FP_S_TWO)
	lf.rem.s  r4,r5,r6
	CHECK_RES ("+NaN %  2.0 = +NaN: ", r4, FP_S_P_NAN)

	/* Remainder with overflow of division (more are needed) */
	LOAD_CONST (r5,FP_S_HUGE2)
	LOAD_CONST (r6,FP_S_SMALL1)
	lf.rem.s  r4,r5,r6
	CHECK_RES (" 1.0 * 2^127  % 1.0 * 2^-129 = +0.0: ", r4, FP_S_P_ZERO)

	/* Remainder with underflow (more are needed) */
	LOAD_CONST (r5,FP_S_SMALL1)
	LOAD_CONST (r6,FP_S_HUGE2)
	lf.rem.s  r4,r5,r6
	CHECK_RES (" 1.0 * 2^-129 % 1.0 * 2^127  = +0.0: ", r4, FP_S_SMALL1)

	/* Remainder with denormalization (more are needed) */
	
/* ----------------------------------------------------------------------------
 * Test of single precision set flag if equal: lf.sfeq.s
 * ------------------------------------------------------------------------- */
_sfeq_s:
	LOAD_STR (r3, "lf.sfeq.s\n")
	l.jal	_puts
	l.nop

	/* Tests of integer equality */
	LOAD_CONST (r4,FP_S_ONE)
	LOAD_CONST (r5,FP_S_ONE)
	lf.sfeq.s  r4,r5
	CHECK_FLAG (" 1.0 ==  1.0: ", TRUE)

	LOAD_CONST (r4,FP_S_ONE)
	LOAD_CONST (r5,FP_S_TWO)
	lf.sfeq.s  r4,r5
	CHECK_FLAG (" 1.0 ==  2.0: ", FALSE)

	/* Fractional equality */
	LOAD_CONST (r4,FP_S_1_5)
	LOAD_CONST (r5,FP_S_1_5)
	lf.sfeq.s  r4,r5
	CHECK_FLAG (" 1.5 ==  1.5: ", TRUE)

	LOAD_CONST (r4,FP_S_1_5)
	LOAD_CONST (r5,FP_S_0_5)
	lf.sfeq.s  r4,r5
	CHECK_FLAG (" 1.5 ==  0.5: ", FALSE)

	/* Signed equality */
	LOAD_CONST (r4,FP_S_N_1_5)
	LOAD_CONST (r5,FP_S_N_1_5)
	lf.sfeq.s  r4,r5
	CHECK_FLAG ("-1.5 == -1.5: ", TRUE)
	
	LOAD_CONST (r4,FP_S_1_5)
	LOAD_CONST (r5,FP_S_N_1_5)
	lf.sfeq.s  r4,r5
	CHECK_FLAG (" 1.5 == -1.5: ", FALSE)

	/* Equality of zeros */
	LOAD_CONST (r4,FP_S_P_ZERO)
	LOAD_CONST (r5,FP_S_P_ZERO)
	lf.sfeq.s  r4,r5
	CHECK_FLAG ("+0.0 == +0.0: ", TRUE)
	
	LOAD_CONST (r4,FP_S_N_ZERO)
	LOAD_CONST (r5,FP_S_N_ZERO)
	lf.sfeq.s  r4,r5
	CHECK_FLAG ("-0.0 == -0.0: ", TRUE)
	
	LOAD_CONST (r4,FP_S_P_ZERO)
	LOAD_CONST (r5,FP_S_N_ZERO)
	lf.sfeq.s  r4,r5
	CHECK_FLAG ("+0.0 == -0.0: ", TRUE)

	/* Equality with infinities (more needed) */
	LOAD_CONST (r4,FP_S_ONE)
	LOAD_CONST (r5,FP_S_P_INF)
	lf.sfeq.s  r4,r5
	CHECK_FLAG (" 1.0 == +inf: ", FALSE)
	
	LOAD_CONST (r4,FP_S_P_INF)
	LOAD_CONST (r5,FP_S_P_INF)
	lf.sfeq.s  r4,r5
	CHECK_FLAG ("+inf == +inf: ", TRUE)
	
	LOAD_CONST (r4,FP_S_N_INF)
	LOAD_CONST (r5,FP_S_N_INF)
	lf.sfeq.s  r4,r5
	CHECK_FLAG ("-inf == -inf: ", TRUE)
	
	LOAD_CONST (r4,FP_S_P_INF)
	LOAD_CONST (r5,FP_S_N_INF)
	lf.sfeq.s  r4,r5
	CHECK_FLAG ("+inf == -inf: ", FALSE)
	
	/* Equality with NaNs (more needed) */
	LOAD_CONST (r4,FP_S_ONE)
	LOAD_CONST (r5,FP_S_P_NAN)
	lf.sfeq.s  r4,r5
	CHECK_FLAG (" 1.0 == +NaN: ", FALSE)
	
	LOAD_CONST (r4,FP_S_P_NAN)
	LOAD_CONST (r5,FP_S_P_NAN)
	lf.sfeq.s  r4,r5
	CHECK_FLAG ("+NaN == +NaN: ", FALSE)
	
	LOAD_CONST (r4,FP_S_N_NAN)
	LOAD_CONST (r5,FP_S_N_NAN)
	lf.sfeq.s  r4,r5
	CHECK_FLAG ("-NaN == -NaN: ", FALSE)
	
	LOAD_CONST (r4,FP_S_P_NAN)
	LOAD_CONST (r5,FP_S_N_NAN)
	lf.sfeq.s  r4,r5
	CHECK_FLAG ("+NaN == -NaN: ", FALSE)

	/* Equality with denormalized numbers (more needed) */
	LOAD_CONST (r4,FP_S_SMALL1)
	LOAD_CONST (r5,FP_S_SMALL1)
	lf.sfeq.s  r4,r5
	CHECK_FLAG (" 1.0 * 2^-129 == 1.0 * 2^-129: ", TRUE)

	LOAD_CONST (r4,FP_S_SMALL1)
	LOAD_CONST (r5,FP_S_SMALL2)
	lf.sfeq.s  r4,r5
	CHECK_FLAG (" 1.0 * 2^-129 == 1.0 * 2^-128: ", FALSE)

	LOAD_CONST (r4,FP_S_HUGE1)
	LOAD_CONST (r5,FP_S_SMALL2)
	lf.sfeq.s  r4,r5
	CHECK_FLAG (" 1.0 * 2^126  == 1.0 * 2^-128: ", FALSE)

	
/* ----------------------------------------------------------------------------
 * Test of single precision set flag if greater than or equal: lf.sfge.s
 * ------------------------------------------------------------------------- */
_sfge_s:
	LOAD_STR (r3, "lf.sfge.s\n")
	l.jal	_puts
	l.nop

	/* Tests of integer greater than or equality */
	LOAD_CONST (r4,FP_S_ONE)
	LOAD_CONST (r5,FP_S_ONE)
	lf.sfge.s  r4,r5
	CHECK_FLAG (" 1.0 >=  1.0: ", TRUE)

	LOAD_CONST (r4,FP_S_TWO)
	LOAD_CONST (r5,FP_S_ONE)
	lf.sfge.s  r4,r5
	CHECK_FLAG (" 2.0 >=  1.0: ", TRUE)

	LOAD_CONST (r4,FP_S_ONE)
	LOAD_CONST (r5,FP_S_TWO)
	lf.sfge.s  r4,r5
	CHECK_FLAG (" 1.0 >=  2.0: ", FALSE)

	/* Fractional greater than or equality */
	LOAD_CONST (r4,FP_S_1_5)
	LOAD_CONST (r5,FP_S_1_5)
	lf.sfge.s  r4,r5
	CHECK_FLAG (" 1.5 >=  1.5: ", TRUE)

	LOAD_CONST (r4,FP_S_1_5)
	LOAD_CONST (r5,FP_S_0_5)
	lf.sfge.s  r4,r5
	CHECK_FLAG (" 1.5 >=  0.5: ", TRUE)

	LOAD_CONST (r4,FP_S_0_5)
	LOAD_CONST (r5,FP_S_1_5)
	lf.sfge.s  r4,r5
	CHECK_FLAG (" 0.5 >=  1.5: ", FALSE)

	/* Signed greater than or equality */
	LOAD_CONST (r4,FP_S_N_1_5)
	LOAD_CONST (r5,FP_S_N_1_5)
	lf.sfge.s  r4,r5
	CHECK_FLAG ("-1.5 >= -1.5: ", TRUE)
	
	LOAD_CONST (r4,FP_S_N_1_5)
	LOAD_CONST (r5,FP_S_N_0_5)
	lf.sfge.s  r4,r5
	CHECK_FLAG ("-1.5 >= -0.5: ", FALSE)

	LOAD_CONST (r4,FP_S_N_0_5)
	LOAD_CONST (r5,FP_S_N_1_5)
	lf.sfge.s  r4,r5
	CHECK_FLAG ("-0.5 >= -1.5: ", TRUE)
	
	LOAD_CONST (r4,FP_S_1_5)
	LOAD_CONST (r5,FP_S_N_1_5)
	lf.sfge.s  r4,r5
	CHECK_FLAG (" 1.5 >= -1.5: ", TRUE)

	LOAD_CONST (r4,FP_S_N_1_5)
	LOAD_CONST (r5,FP_S_1_5)
	lf.sfge.s  r4,r5
	CHECK_FLAG ("-1.5 >=  1.5: ", FALSE)

	/* Greater than or equality of zeros */
	LOAD_CONST (r4,FP_S_P_ZERO)
	LOAD_CONST (r5,FP_S_P_ZERO)
	lf.sfge.s  r4,r5
	CHECK_FLAG ("+0.0 >= +0.0: ", TRUE)
	
	LOAD_CONST (r4,FP_S_N_ZERO)
	LOAD_CONST (r5,FP_S_N_ZERO)
	lf.sfge.s  r4,r5
	CHECK_FLAG ("-0.0 >= -0.0: ", TRUE)
	
	LOAD_CONST (r4,FP_S_P_ZERO)
	LOAD_CONST (r5,FP_S_N_ZERO)
	lf.sfge.s  r4,r5
	CHECK_FLAG ("+0.0 >= -0.0: ", TRUE)

	LOAD_CONST (r4,FP_S_N_ZERO)
	LOAD_CONST (r5,FP_S_P_ZERO)
	lf.sfge.s  r4,r5
	CHECK_FLAG ("-0.0 >= +0.0: ", TRUE)

	/* Greater than or equality with infinities (more needed) */
	LOAD_CONST (r4,FP_S_ONE)
	LOAD_CONST (r5,FP_S_P_INF)
	lf.sfge.s  r4,r5
	CHECK_FLAG (" 1.0 >= +inf: ", FALSE)
	
	LOAD_CONST (r4,FP_S_P_INF)
	LOAD_CONST (r5,FP_S_ONE)
	lf.sfge.s  r4,r5
	CHECK_FLAG ("+inf >=  1.0: ", TRUE)
	
	LOAD_CONST (r4,FP_S_ONE)
	LOAD_CONST (r5,FP_S_N_INF)
	lf.sfge.s  r4,r5
	CHECK_FLAG (" 1.0 >= -inf: ", TRUE)
	
	LOAD_CONST (r4,FP_S_N_INF)
	LOAD_CONST (r5,FP_S_ONE)
	lf.sfge.s  r4,r5
	CHECK_FLAG ("-inf >=  1.0: ", FALSE)
	
	LOAD_CONST (r4,FP_S_P_INF)
	LOAD_CONST (r5,FP_S_P_INF)
	lf.sfge.s  r4,r5
	CHECK_FLAG ("+inf >= +inf: ", TRUE)
	
	LOAD_CONST (r4,FP_S_N_INF)
	LOAD_CONST (r5,FP_S_N_INF)
	lf.sfge.s  r4,r5
	CHECK_FLAG ("-inf >= -inf: ", TRUE)
	
	LOAD_CONST (r4,FP_S_P_INF)
	LOAD_CONST (r5,FP_S_N_INF)
	lf.sfge.s  r4,r5
	CHECK_FLAG ("+inf >= -inf: ", TRUE)
	
	LOAD_CONST (r4,FP_S_N_INF)
	LOAD_CONST (r5,FP_S_P_INF)
	lf.sfge.s  r4,r5
	CHECK_FLAG ("-inf >= +inf: ", FALSE)
	
	/* Greater than or equality with NaNs (more needed) */
	LOAD_CONST (r4,FP_S_ONE)
	LOAD_CONST (r5,FP_S_P_NAN)
	lf.sfge.s  r4,r5
	CHECK_FLAG (" 1.0 >= +NaN: ", FALSE)
	
	LOAD_CONST (r4,FP_S_P_NAN)
	LOAD_CONST (r5,FP_S_P_NAN)
	lf.sfge.s  r4,r5
	CHECK_FLAG ("+NaN >= +NaN: ", FALSE)
	
	LOAD_CONST (r4,FP_S_N_NAN)
	LOAD_CONST (r5,FP_S_N_NAN)
	lf.sfge.s  r4,r5
	CHECK_FLAG ("-NaN >= -NaN: ", FALSE)
	
	LOAD_CONST (r4,FP_S_P_NAN)
	LOAD_CONST (r5,FP_S_N_NAN)
	lf.sfge.s  r4,r5
	CHECK_FLAG ("+NaN >= -NaN: ", FALSE)

	LOAD_CONST (r4,FP_S_N_NAN)
	LOAD_CONST (r5,FP_S_P_NAN)
	lf.sfge.s  r4,r5
	CHECK_FLAG ("-NaN >= +NaN: ", FALSE)

	/* Greater than or equality with denormalized numbers (more needed) */
	LOAD_CONST (r4,FP_S_SMALL1)
	LOAD_CONST (r5,FP_S_SMALL1)
	lf.sfge.s  r4,r5
	CHECK_FLAG (" 1.0 * 2^-129 >= 1.0 * 2^-129: ", TRUE)

	LOAD_CONST (r4,FP_S_SMALL1)
	LOAD_CONST (r5,FP_S_SMALL2)
	lf.sfge.s  r4,r5
	CHECK_FLAG (" 1.0 * 2^-129 >= 1.0 * 2^-128: ", FALSE)

	LOAD_CONST (r4,FP_S_SMALL2)
	LOAD_CONST (r5,FP_S_SMALL1)
	lf.sfge.s  r4,r5
	CHECK_FLAG (" 1.0 * 2^-128 >= 1.0 * 2^-129: ", TRUE)

	LOAD_CONST (r4,FP_S_HUGE1)
	LOAD_CONST (r5,FP_S_SMALL2)
	lf.sfge.s  r4,r5
	CHECK_FLAG (" 1.0 * 2^126  >= 1.0 * 2^-128: ", TRUE)

	LOAD_CONST (r4,FP_S_SMALL2)
	LOAD_CONST (r5,FP_S_HUGE1)
	lf.sfge.s  r4,r5
	CHECK_FLAG (" 1.0 * 2^-128 >= 1.0 * 2^126:  ", FALSE)

	
/* ----------------------------------------------------------------------------
 * Test of single precision set flag if greater than: lf.sfgt.s
 * ------------------------------------------------------------------------- */
_sfgt_s:
	LOAD_STR (r3, "lf.sfgt.s\n")
	l.jal	_puts
	l.nop

	/* Tests of integer greater than */
	LOAD_CONST (r4,FP_S_ONE)
	LOAD_CONST (r5,FP_S_ONE)
	lf.sfgt.s  r4,r5
	CHECK_FLAG (" 1.0 >  1.0: ", FALSE)

	LOAD_CONST (r4,FP_S_TWO)
	LOAD_CONST (r5,FP_S_ONE)
	lf.sfgt.s  r4,r5
	CHECK_FLAG (" 2.0 >  1.0: ", TRUE)

	LOAD_CONST (r4,FP_S_ONE)
	LOAD_CONST (r5,FP_S_TWO)
	lf.sfgt.s  r4,r5
	CHECK_FLAG (" 1.0 >  2.0: ", FALSE)

	/* Fractional greater than */
	LOAD_CONST (r4,FP_S_1_5)
	LOAD_CONST (r5,FP_S_1_5)
	lf.sfgt.s  r4,r5
	CHECK_FLAG (" 1.5 >  1.5: ", FALSE)

	LOAD_CONST (r4,FP_S_1_5)
	LOAD_CONST (r5,FP_S_0_5)
	lf.sfgt.s  r4,r5
	CHECK_FLAG (" 1.5 >  0.5: ", TRUE)

	LOAD_CONST (r4,FP_S_0_5)
	LOAD_CONST (r5,FP_S_1_5)
	lf.sfgt.s  r4,r5
	CHECK_FLAG (" 0.5 >  1.5: ", FALSE)

	/* Signed greater than */
	LOAD_CONST (r4,FP_S_N_1_5)
	LOAD_CONST (r5,FP_S_N_1_5)
	lf.sfgt.s  r4,r5
	CHECK_FLAG ("-1.5 > -1.5: ", FALSE)
	
	LOAD_CONST (r4,FP_S_N_1_5)
	LOAD_CONST (r5,FP_S_N_0_5)
	lf.sfgt.s  r4,r5
	CHECK_FLAG ("-1.5 > -0.5: ", FALSE)

	LOAD_CONST (r4,FP_S_N_0_5)
	LOAD_CONST (r5,FP_S_N_1_5)
	lf.sfgt.s  r4,r5
	CHECK_FLAG ("-0.5 > -1.5: ", TRUE)
	
	LOAD_CONST (r4,FP_S_1_5)
	LOAD_CONST (r5,FP_S_N_1_5)
	lf.sfgt.s  r4,r5
	CHECK_FLAG (" 1.5 > -1.5: ", TRUE)

	LOAD_CONST (r4,FP_S_N_1_5)
	LOAD_CONST (r5,FP_S_1_5)
	lf.sfgt.s  r4,r5
	CHECK_FLAG ("-1.5 >  1.5: ", FALSE)

	/* Greater than of zeros */
	LOAD_CONST (r4,FP_S_P_ZERO)
	LOAD_CONST (r5,FP_S_P_ZERO)
	lf.sfgt.s  r4,r5
	CHECK_FLAG ("+0.0 > +0.0: ", FALSE)
	
	LOAD_CONST (r4,FP_S_N_ZERO)
	LOAD_CONST (r5,FP_S_N_ZERO)
	lf.sfgt.s  r4,r5
	CHECK_FLAG ("-0.0 > -0.0: ", FALSE)
	
	LOAD_CONST (r4,FP_S_P_ZERO)
	LOAD_CONST (r5,FP_S_N_ZERO)
	lf.sfgt.s  r4,r5
	CHECK_FLAG ("+0.0 > -0.0: ", FALSE)

	LOAD_CONST (r4,FP_S_N_ZERO)
	LOAD_CONST (r5,FP_S_P_ZERO)
	lf.sfgt.s  r4,r5
	CHECK_FLAG ("-0.0 > +0.0: ", FALSE)

	/* Greater than with infinities (more needed) */
	LOAD_CONST (r4,FP_S_ONE)
	LOAD_CONST (r5,FP_S_P_INF)
	lf.sfgt.s  r4,r5
	CHECK_FLAG (" 1.0 > +inf: ", FALSE)
	
	LOAD_CONST (r4,FP_S_P_INF)
	LOAD_CONST (r5,FP_S_ONE)
	lf.sfgt.s  r4,r5
	CHECK_FLAG ("+inf >  1.0: ", TRUE)
	
	LOAD_CONST (r4,FP_S_ONE)
	LOAD_CONST (r5,FP_S_N_INF)
	lf.sfgt.s  r4,r5
	CHECK_FLAG (" 1.0 > -inf: ", TRUE)
	
	LOAD_CONST (r4,FP_S_N_INF)
	LOAD_CONST (r5,FP_S_ONE)
	lf.sfgt.s  r4,r5
	CHECK_FLAG ("-inf >  1.0: ", FALSE)
	
	LOAD_CONST (r4,FP_S_P_INF)
	LOAD_CONST (r5,FP_S_P_INF)
	lf.sfgt.s  r4,r5
	CHECK_FLAG ("+inf > +inf: ", FALSE)
	
	LOAD_CONST (r4,FP_S_N_INF)
	LOAD_CONST (r5,FP_S_N_INF)
	lf.sfgt.s  r4,r5
	CHECK_FLAG ("-inf > -inf: ", FALSE)
	
	LOAD_CONST (r4,FP_S_P_INF)
	LOAD_CONST (r5,FP_S_N_INF)
	lf.sfgt.s  r4,r5
	CHECK_FLAG ("+inf > -inf: ", TRUE)
	
	LOAD_CONST (r4,FP_S_N_INF)
	LOAD_CONST (r5,FP_S_P_INF)
	lf.sfgt.s  r4,r5
	CHECK_FLAG ("-inf > +inf: ", FALSE)
	
	/* Greater than with NaNs (more needed) */
	LOAD_CONST (r4,FP_S_ONE)
	LOAD_CONST (r5,FP_S_P_NAN)
	lf.sfgt.s  r4,r5
	CHECK_FLAG (" 1.0 > +NaN: ", FALSE)
	
	LOAD_CONST (r4,FP_S_P_NAN)
	LOAD_CONST (r5,FP_S_P_NAN)
	lf.sfgt.s  r4,r5
	CHECK_FLAG ("+NaN > +NaN: ", FALSE)
	
	LOAD_CONST (r4,FP_S_N_NAN)
	LOAD_CONST (r5,FP_S_N_NAN)
	lf.sfgt.s  r4,r5
	CHECK_FLAG ("-NaN > -NaN: ", FALSE)
	
	LOAD_CONST (r4,FP_S_P_NAN)
	LOAD_CONST (r5,FP_S_N_NAN)
	lf.sfgt.s  r4,r5
	CHECK_FLAG ("+NaN > -NaN: ", FALSE)

	LOAD_CONST (r4,FP_S_N_NAN)
	LOAD_CONST (r5,FP_S_P_NAN)
	lf.sfgt.s  r4,r5
	CHECK_FLAG ("-NaN > +NaN: ", FALSE)

	/* Greater than with denormalized numbers (more needed) */
	LOAD_CONST (r4,FP_S_SMALL1)
	LOAD_CONST (r5,FP_S_SMALL1)
	lf.sfgt.s  r4,r5
	CHECK_FLAG (" 1.0 * 2^-129 > 1.0 * 2^-129: ", FALSE)

	LOAD_CONST (r4,FP_S_SMALL1)
	LOAD_CONST (r5,FP_S_SMALL2)
	lf.sfgt.s  r4,r5
	CHECK_FLAG (" 1.0 * 2^-129 > 1.0 * 2^-128: ", FALSE)

	LOAD_CONST (r4,FP_S_SMALL2)
	LOAD_CONST (r5,FP_S_SMALL1)
	lf.sfgt.s  r4,r5
	CHECK_FLAG (" 1.0 * 2^-128 > 1.0 * 2^-129: ", TRUE)

	LOAD_CONST (r4,FP_S_HUGE1)
	LOAD_CONST (r5,FP_S_SMALL2)
	lf.sfgt.s  r4,r5
	CHECK_FLAG (" 1.0 * 2^126  > 1.0 * 2^-128: ", TRUE)

	LOAD_CONST (r4,FP_S_SMALL2)
	LOAD_CONST (r5,FP_S_HUGE1)
	lf.sfgt.s  r4,r5
	CHECK_FLAG (" 1.0 * 2^-128 > 1.0 * 2^126:  ", FALSE)

/* ----------------------------------------------------------------------------
 * Test of single precision set flag if less than or equal: lf.sfle.s
 * ------------------------------------------------------------------------- */
_sfle_s:
	LOAD_STR (r3, "lf.sfle.s\n")
	l.jal	_puts
	l.nop

	/* Tests of integer less than or equality */
	LOAD_CONST (r4,FP_S_ONE)
	LOAD_CONST (r5,FP_S_ONE)
	lf.sfle.s  r4,r5
	CHECK_FLAG (" 1.0 <=  1.0: ", TRUE)

	LOAD_CONST (r4,FP_S_TWO)
	LOAD_CONST (r5,FP_S_ONE)
	lf.sfle.s  r4,r5
	CHECK_FLAG (" 2.0 <=  1.0: ", FALSE)

	LOAD_CONST (r4,FP_S_ONE)
	LOAD_CONST (r5,FP_S_TWO)
	lf.sfle.s  r4,r5
	CHECK_FLAG (" 1.0 <=  2.0: ", TRUE)

	/* Fractional less than or equality */
	LOAD_CONST (r4,FP_S_1_5)
	LOAD_CONST (r5,FP_S_1_5)
	lf.sfle.s  r4,r5
	CHECK_FLAG (" 1.5 <=  1.5: ", TRUE)

	LOAD_CONST (r4,FP_S_1_5)
	LOAD_CONST (r5,FP_S_0_5)
	lf.sfle.s  r4,r5
	CHECK_FLAG (" 1.5 <=  0.5: ", FALSE)

	LOAD_CONST (r4,FP_S_0_5)
	LOAD_CONST (r5,FP_S_1_5)
	lf.sfle.s  r4,r5
	CHECK_FLAG (" 0.5 <=  1.5: ", TRUE)

	/* Signed less than or equality */
	LOAD_CONST (r4,FP_S_N_1_5)
	LOAD_CONST (r5,FP_S_N_1_5)
	lf.sfle.s  r4,r5
	CHECK_FLAG ("-1.5 <= -1.5: ", TRUE)
	
	LOAD_CONST (r4,FP_S_N_1_5)
	LOAD_CONST (r5,FP_S_N_0_5)
	lf.sfle.s  r4,r5
	CHECK_FLAG ("-1.5 <= -0.5: ", TRUE)

	LOAD_CONST (r4,FP_S_N_0_5)
	LOAD_CONST (r5,FP_S_N_1_5)
	lf.sfle.s  r4,r5
	CHECK_FLAG ("-0.5 <= -1.5: ", FALSE)
	
	LOAD_CONST (r4,FP_S_1_5)
	LOAD_CONST (r5,FP_S_N_1_5)
	lf.sfle.s  r4,r5
	CHECK_FLAG (" 1.5 <= -1.5: ", FALSE)

	LOAD_CONST (r4,FP_S_N_1_5)
	LOAD_CONST (r5,FP_S_1_5)
	lf.sfle.s  r4,r5
	CHECK_FLAG ("-1.5 <=  1.5: ", TRUE)

	/* Less than or equality of zeros */
	LOAD_CONST (r4,FP_S_P_ZERO)
	LOAD_CONST (r5,FP_S_P_ZERO)
	lf.sfle.s  r4,r5
	CHECK_FLAG ("+0.0 <= +0.0: ", TRUE)
	
	LOAD_CONST (r4,FP_S_N_ZERO)
	LOAD_CONST (r5,FP_S_N_ZERO)
	lf.sfle.s  r4,r5
	CHECK_FLAG ("-0.0 <= -0.0: ", TRUE)
	
	LOAD_CONST (r4,FP_S_P_ZERO)
	LOAD_CONST (r5,FP_S_N_ZERO)
	lf.sfle.s  r4,r5
	CHECK_FLAG ("+0.0 <= -0.0: ", TRUE)

	LOAD_CONST (r4,FP_S_N_ZERO)
	LOAD_CONST (r5,FP_S_P_ZERO)
	lf.sfle.s  r4,r5
	CHECK_FLAG ("-0.0 <= +0.0: ", TRUE)

	/* Less than or equality with infinities (more needed) */
	LOAD_CONST (r4,FP_S_ONE)
	LOAD_CONST (r5,FP_S_P_INF)
	lf.sfle.s  r4,r5
	CHECK_FLAG (" 1.0 <= +inf: ", TRUE)
	
	LOAD_CONST (r4,FP_S_P_INF)
	LOAD_CONST (r5,FP_S_ONE)
	lf.sfle.s  r4,r5
	CHECK_FLAG ("+inf <=  1.0: ", FALSE)
	
	LOAD_CONST (r4,FP_S_ONE)
	LOAD_CONST (r5,FP_S_N_INF)
	lf.sfle.s  r4,r5
	CHECK_FLAG (" 1.0 <= -inf: ", FALSE)
	
	LOAD_CONST (r4,FP_S_N_INF)
	LOAD_CONST (r5,FP_S_ONE)
	lf.sfle.s  r4,r5
	CHECK_FLAG ("-inf <=  1.0: ", TRUE)
	
	LOAD_CONST (r4,FP_S_P_INF)
	LOAD_CONST (r5,FP_S_P_INF)
	lf.sfle.s  r4,r5
	CHECK_FLAG ("+inf <= +inf: ", TRUE)
	
	LOAD_CONST (r4,FP_S_N_INF)
	LOAD_CONST (r5,FP_S_N_INF)
	lf.sfle.s  r4,r5
	CHECK_FLAG ("-inf <= -inf: ", TRUE)
	
	LOAD_CONST (r4,FP_S_P_INF)
	LOAD_CONST (r5,FP_S_N_INF)
	lf.sfle.s  r4,r5
	CHECK_FLAG ("+inf <= -inf: ", FALSE)
	
	LOAD_CONST (r4,FP_S_N_INF)
	LOAD_CONST (r5,FP_S_P_INF)
	lf.sfle.s  r4,r5
	CHECK_FLAG ("-inf <= +inf: ", TRUE)
	
	/* Less than or equality with NaNs (more needed) */
	LOAD_CONST (r4,FP_S_ONE)
	LOAD_CONST (r5,FP_S_P_NAN)
	lf.sfle.s  r4,r5
	CHECK_FLAG (" 1.0 <= +NaN: ", FALSE)
	
	LOAD_CONST (r4,FP_S_P_NAN)
	LOAD_CONST (r5,FP_S_P_NAN)
	lf.sfle.s  r4,r5
	CHECK_FLAG ("+NaN <= +NaN: ", FALSE)
	
	LOAD_CONST (r4,FP_S_N_NAN)
	LOAD_CONST (r5,FP_S_N_NAN)
	lf.sfle.s  r4,r5
	CHECK_FLAG ("-NaN <= -NaN: ", FALSE)
	
	LOAD_CONST (r4,FP_S_P_NAN)
	LOAD_CONST (r5,FP_S_N_NAN)
	lf.sfle.s  r4,r5
	CHECK_FLAG ("+NaN <= -NaN: ", FALSE)

	LOAD_CONST (r4,FP_S_N_NAN)
	LOAD_CONST (r5,FP_S_P_NAN)
	lf.sfle.s  r4,r5
	CHECK_FLAG ("-NaN <= +NaN: ", FALSE)

	/* Less than or equality with denormalized numbers (more needed) */
	LOAD_CONST (r4,FP_S_SMALL1)
	LOAD_CONST (r5,FP_S_SMALL1)
	lf.sfle.s  r4,r5
	CHECK_FLAG (" 1.0 * 2^-129 <= 1.0 * 2^-129: ", TRUE)

	LOAD_CONST (r4,FP_S_SMALL1)
	LOAD_CONST (r5,FP_S_SMALL2)
	lf.sfle.s  r4,r5
	CHECK_FLAG (" 1.0 * 2^-129 <= 1.0 * 2^-128: ", TRUE)

	LOAD_CONST (r4,FP_S_SMALL2)
	LOAD_CONST (r5,FP_S_SMALL1)
	lf.sfle.s  r4,r5
	CHECK_FLAG (" 1.0 * 2^-128 <= 1.0 * 2^-129: ", FALSE)

	LOAD_CONST (r4,FP_S_HUGE1)
	LOAD_CONST (r5,FP_S_SMALL2)
	lf.sfle.s  r4,r5
	CHECK_FLAG (" 1.0 * 2^126  <= 1.0 * 2^-128: ", FALSE)

	LOAD_CONST (r4,FP_S_SMALL2)
	LOAD_CONST (r5,FP_S_HUGE1)
	lf.sfle.s  r4,r5
	CHECK_FLAG (" 1.0 * 2^-128 <= 1.0 * 2^126:  ", TRUE)

	
/* ----------------------------------------------------------------------------
 * Test of single precision set flag if less than: lf.sflt.s
 * ------------------------------------------------------------------------- */
_sflt_s:
	LOAD_STR (r3, "lf.sflt.s\n")
	l.jal	_puts
	l.nop

	/* Tests of integer less than */
	LOAD_CONST (r4,FP_S_ONE)
	LOAD_CONST (r5,FP_S_ONE)
	lf.sflt.s  r4,r5
	CHECK_FLAG (" 1.0 <  1.0: ", FALSE)

	LOAD_CONST (r4,FP_S_TWO)
	LOAD_CONST (r5,FP_S_ONE)
	lf.sflt.s  r4,r5
	CHECK_FLAG (" 2.0 <  1.0: ", FALSE)

	LOAD_CONST (r4,FP_S_ONE)
	LOAD_CONST (r5,FP_S_TWO)
	lf.sflt.s  r4,r5
	CHECK_FLAG (" 1.0 <  2.0: ", TRUE)

	/* Fractional less than */
	LOAD_CONST (r4,FP_S_1_5)
	LOAD_CONST (r5,FP_S_1_5)
	lf.sflt.s  r4,r5
	CHECK_FLAG (" 1.5 <  1.5: ", FALSE)

	LOAD_CONST (r4,FP_S_1_5)
	LOAD_CONST (r5,FP_S_0_5)
	lf.sflt.s  r4,r5
	CHECK_FLAG (" 1.5 <  0.5: ", FALSE)

	LOAD_CONST (r4,FP_S_0_5)
	LOAD_CONST (r5,FP_S_1_5)
	lf.sflt.s  r4,r5
	CHECK_FLAG (" 0.5 <  1.5: ", TRUE)

	/* Signed less than */
	LOAD_CONST (r4,FP_S_N_1_5)
	LOAD_CONST (r5,FP_S_N_1_5)
	lf.sflt.s  r4,r5
	CHECK_FLAG ("-1.5 < -1.5: ", FALSE)
	
	LOAD_CONST (r4,FP_S_N_1_5)
	LOAD_CONST (r5,FP_S_N_0_5)
	lf.sflt.s  r4,r5
	CHECK_FLAG ("-1.5 < -0.5: ", TRUE)

	LOAD_CONST (r4,FP_S_N_0_5)
	LOAD_CONST (r5,FP_S_N_1_5)
	lf.sflt.s  r4,r5
	CHECK_FLAG ("-0.5 < -1.5: ", FALSE)
	
	LOAD_CONST (r4,FP_S_1_5)
	LOAD_CONST (r5,FP_S_N_1_5)
	lf.sflt.s  r4,r5
	CHECK_FLAG (" 1.5 < -1.5: ", FALSE)

	LOAD_CONST (r4,FP_S_N_1_5)
	LOAD_CONST (r5,FP_S_1_5)
	lf.sflt.s  r4,r5
	CHECK_FLAG ("-1.5 <  1.5: ", TRUE)

	/* Less than of zeros */
	LOAD_CONST (r4,FP_S_P_ZERO)
	LOAD_CONST (r5,FP_S_P_ZERO)
	lf.sflt.s  r4,r5
	CHECK_FLAG ("+0.0 < +0.0: ", FALSE)
	
	LOAD_CONST (r4,FP_S_N_ZERO)
	LOAD_CONST (r5,FP_S_N_ZERO)
	lf.sflt.s  r4,r5
	CHECK_FLAG ("-0.0 < -0.0: ", FALSE)
	
	LOAD_CONST (r4,FP_S_P_ZERO)
	LOAD_CONST (r5,FP_S_N_ZERO)
	lf.sflt.s  r4,r5
	CHECK_FLAG ("+0.0 < -0.0: ", FALSE)

	LOAD_CONST (r4,FP_S_N_ZERO)
	LOAD_CONST (r5,FP_S_P_ZERO)
	lf.sflt.s  r4,r5
	CHECK_FLAG ("-0.0 < +0.0: ", FALSE)

	/* Less than with infinities (more needed) */
	LOAD_CONST (r4,FP_S_ONE)
	LOAD_CONST (r5,FP_S_P_INF)
	lf.sflt.s  r4,r5
	CHECK_FLAG (" 1.0 < +inf: ", TRUE)
	
	LOAD_CONST (r4,FP_S_P_INF)
	LOAD_CONST (r5,FP_S_ONE)
	lf.sflt.s  r4,r5
	CHECK_FLAG ("+inf <  1.0: ", FALSE)
	
	LOAD_CONST (r4,FP_S_ONE)
	LOAD_CONST (r5,FP_S_N_INF)
	lf.sflt.s  r4,r5
	CHECK_FLAG (" 1.0 < -inf: ", FALSE)
	
	LOAD_CONST (r4,FP_S_N_INF)
	LOAD_CONST (r5,FP_S_ONE)
	lf.sflt.s  r4,r5
	CHECK_FLAG ("-inf <  1.0: ", TRUE)
	
	LOAD_CONST (r4,FP_S_P_INF)
	LOAD_CONST (r5,FP_S_P_INF)
	lf.sflt.s  r4,r5
	CHECK_FLAG ("+inf < +inf: ", FALSE)
	
	LOAD_CONST (r4,FP_S_N_INF)
	LOAD_CONST (r5,FP_S_N_INF)
	lf.sflt.s  r4,r5
	CHECK_FLAG ("-inf < -inf: ", FALSE)
	
	LOAD_CONST (r4,FP_S_P_INF)
	LOAD_CONST (r5,FP_S_N_INF)
	lf.sflt.s  r4,r5
	CHECK_FLAG ("+inf < -inf: ", FALSE)
	
	LOAD_CONST (r4,FP_S_N_INF)
	LOAD_CONST (r5,FP_S_P_INF)
	lf.sflt.s  r4,r5
	CHECK_FLAG ("-inf < +inf: ", TRUE)
	
	/* Less than with NaNs (more needed) */
	LOAD_CONST (r4,FP_S_ONE)
	LOAD_CONST (r5,FP_S_P_NAN)
	lf.sflt.s  r4,r5
	CHECK_FLAG (" 1.0 < +NaN: ", FALSE)
	
	LOAD_CONST (r4,FP_S_P_NAN)
	LOAD_CONST (r5,FP_S_P_NAN)
	lf.sflt.s  r4,r5
	CHECK_FLAG ("+NaN < +NaN: ", FALSE)
	
	LOAD_CONST (r4,FP_S_N_NAN)
	LOAD_CONST (r5,FP_S_N_NAN)
	lf.sflt.s  r4,r5
	CHECK_FLAG ("-NaN < -NaN: ", FALSE)
	
	LOAD_CONST (r4,FP_S_P_NAN)
	LOAD_CONST (r5,FP_S_N_NAN)
	lf.sflt.s  r4,r5
	CHECK_FLAG ("+NaN < -NaN: ", FALSE)

	LOAD_CONST (r4,FP_S_N_NAN)
	LOAD_CONST (r5,FP_S_P_NAN)
	lf.sflt.s  r4,r5
	CHECK_FLAG ("-NaN < +NaN: ", FALSE)

	/* Less than with denormalized numbers (more needed) */
	LOAD_CONST (r4,FP_S_SMALL1)
	LOAD_CONST (r5,FP_S_SMALL1)
	lf.sflt.s  r4,r5
	CHECK_FLAG (" 1.0 * 2^-129 < 1.0 * 2^-129: ", FALSE)

	LOAD_CONST (r4,FP_S_SMALL1)
	LOAD_CONST (r5,FP_S_SMALL2)
	lf.sflt.s  r4,r5
	CHECK_FLAG (" 1.0 * 2^-129 < 1.0 * 2^-128: ", TRUE)

	LOAD_CONST (r4,FP_S_SMALL2)
	LOAD_CONST (r5,FP_S_SMALL1)
	lf.sflt.s  r4,r5
	CHECK_FLAG (" 1.0 * 2^-128 < 1.0 * 2^-129: ", FALSE)

	LOAD_CONST (r4,FP_S_HUGE1)
	LOAD_CONST (r5,FP_S_SMALL2)
	lf.sflt.s  r4,r5
	CHECK_FLAG (" 1.0 * 2^126  < 1.0 * 2^-128: ", FALSE)

	LOAD_CONST (r4,FP_S_SMALL2)
	LOAD_CONST (r5,FP_S_HUGE1)
	lf.sflt.s  r4,r5
	CHECK_FLAG (" 1.0 * 2^-128 < 1.0 * 2^126:  ", TRUE)
	
/* ----------------------------------------------------------------------------
 * Test of single precision set flag if not equal: lf.sfne.s
 * ------------------------------------------------------------------------- */
_sfne_s:
	LOAD_STR (r3, "lf.sfne.s\n")
	l.jal	_puts
	l.nop

	/* Tests of integer inequality */
	LOAD_CONST (r4,FP_S_ONE)
	LOAD_CONST (r5,FP_S_ONE)
	lf.sfne.s  r4,r5
	CHECK_FLAG (" 1.0 !=  1.0: ", FALSE)

	LOAD_CONST (r4,FP_S_ONE)
	LOAD_CONST (r5,FP_S_TWO)
	lf.sfne.s  r4,r5
	CHECK_FLAG (" 1.0 !=  2.0: ", TRUE)

	/* Fractional inequality */
	LOAD_CONST (r4,FP_S_1_5)
	LOAD_CONST (r5,FP_S_1_5)
	lf.sfne.s  r4,r5
	CHECK_FLAG (" 1.5 !=  1.5: ", FALSE)

	LOAD_CONST (r4,FP_S_1_5)
	LOAD_CONST (r5,FP_S_0_5)
	lf.sfne.s  r4,r5
	CHECK_FLAG (" 1.5 !=  0.5: ", TRUE)

	/* Signed inequality */
	LOAD_CONST (r4,FP_S_N_1_5)
	LOAD_CONST (r5,FP_S_N_1_5)
	lf.sfne.s  r4,r5
	CHECK_FLAG ("-1.5 != -1.5: ", FALSE)
	
	LOAD_CONST (r4,FP_S_1_5)
	LOAD_CONST (r5,FP_S_N_1_5)
	lf.sfne.s  r4,r5
	CHECK_FLAG (" 1.5 != -1.5: ", TRUE)

	/* Inequality of zeros */
	LOAD_CONST (r4,FP_S_P_ZERO)
	LOAD_CONST (r5,FP_S_P_ZERO)
	lf.sfne.s  r4,r5
	CHECK_FLAG ("+0.0 != +0.0: ", FALSE)
	
	LOAD_CONST (r4,FP_S_N_ZERO)
	LOAD_CONST (r5,FP_S_N_ZERO)
	lf.sfne.s  r4,r5
	CHECK_FLAG ("-0.0 != -0.0: ", FALSE)
	
	LOAD_CONST (r4,FP_S_P_ZERO)
	LOAD_CONST (r5,FP_S_N_ZERO)
	lf.sfne.s  r4,r5
	CHECK_FLAG ("+0.0 != -0.0: ", FALSE)

	/* Inequality with infinities (more needed) */
	LOAD_CONST (r4,FP_S_ONE)
	LOAD_CONST (r5,FP_S_P_INF)
	lf.sfne.s  r4,r5
	CHECK_FLAG (" 1.0 != +inf: ", TRUE)
	
	LOAD_CONST (r4,FP_S_P_INF)
	LOAD_CONST (r5,FP_S_P_INF)
	lf.sfne.s  r4,r5
	CHECK_FLAG ("+inf != +inf: ", FALSE)
	
	LOAD_CONST (r4,FP_S_N_INF)
	LOAD_CONST (r5,FP_S_N_INF)
	lf.sfne.s  r4,r5
	CHECK_FLAG ("-inf != -inf: ", FALSE)
	
	LOAD_CONST (r4,FP_S_P_INF)
	LOAD_CONST (r5,FP_S_N_INF)
	lf.sfne.s  r4,r5
	CHECK_FLAG ("+inf != -inf: ", TRUE)
	
	/* Inequality with NaNs (more needed) */
	LOAD_CONST (r4,FP_S_ONE)
	LOAD_CONST (r5,FP_S_P_NAN)
	lf.sfne.s  r4,r5
	CHECK_FLAG (" 1.0 != +NaN: ", TRUE)
	
	LOAD_CONST (r4,FP_S_P_NAN)
	LOAD_CONST (r5,FP_S_P_NAN)
	lf.sfne.s  r4,r5
	CHECK_FLAG ("+NaN != +NaN: ", TRUE)
	
	LOAD_CONST (r4,FP_S_N_NAN)
	LOAD_CONST (r5,FP_S_N_NAN)
	lf.sfne.s  r4,r5
	CHECK_FLAG ("-NaN != -NaN: ", TRUE)
	
	LOAD_CONST (r4,FP_S_P_NAN)
	LOAD_CONST (r5,FP_S_N_NAN)
	lf.sfne.s  r4,r5
	CHECK_FLAG ("+NaN != -NaN: ", TRUE)

	/* Inequality with denormalized numbers (more needed) */
	LOAD_CONST (r4,FP_S_SMALL1)
	LOAD_CONST (r5,FP_S_SMALL1)
	lf.sfne.s  r4,r5
	CHECK_FLAG (" 1.0 * 2^-129 != 1.0 * 2^-129: ", FALSE)

	LOAD_CONST (r4,FP_S_SMALL1)
	LOAD_CONST (r5,FP_S_SMALL2)
	lf.sfne.s  r4,r5
	CHECK_FLAG (" 1.0 * 2^-129 != 1.0 * 2^-128: ", TRUE)

	LOAD_CONST (r4,FP_S_HUGE1)
	LOAD_CONST (r5,FP_S_SMALL2)
	lf.sfne.s  r4,r5
	CHECK_FLAG (" 1.0 * 2^126  != 1.0 * 2^-128: ", TRUE)

/* ----------------------------------------------------------------------------
 * Test of single precision subtract: lf.sub.s
 * ------------------------------------------------------------------------- */
	.section .text
_sub_s:
	LOAD_STR (r3, "lf.sub.s\n")
	l.jal	_puts
	l.nop

	SET_RM	(FPCSR_RM_RIP)
	
	/* Simple integer subtraction */
	LOAD_CONST (r5,FP_S_ONE)
	LOAD_CONST (r6,FP_S_P_ZERO)
	lf.sub.s  r4,r5,r6
	CHECK_RES (" 1.0 -  0.0  =  1.0:  ", r4, FP_S_ONE)
	
	LOAD_CONST (r5,FP_S_ONE)
	LOAD_CONST (r6,FP_S_N_ZERO)
	lf.sub.s  r4,r5,r6
	CHECK_RES (" 1.0 - -0.0  =  1.0:  ", r4, FP_S_ONE)
	
	LOAD_CONST (r5,FP_S_ONE)
	LOAD_CONST (r6,FP_S_ONE)
	lf.sub.s  r4,r5,r6
	CHECK_RES (" 1.0 -  1.0  = +0.0:  ", r4, FP_S_P_ZERO)

	LOAD_CONST (r5,FP_S_TWO)
	LOAD_CONST (r6,FP_S_ONE)
	lf.sub.s  r4,r5,r6
	CHECK_RES (" 2.0 -  1.0  =  1.0:  ", r4, FP_S_ONE)

	/* Fractional subtraction */
	LOAD_CONST (r5,FP_S_1_5)
	LOAD_CONST (r6,FP_S_1_5)
	lf.sub.s  r4,r5,r6
	CHECK_RES (" 1.5 -  1.5  = +0.0:  ", r4, FP_S_P_ZERO)

	LOAD_CONST (r5,FP_S_1_5)
	LOAD_CONST (r6,FP_S_0_5)
	lf.sub.s  r4,r5,r6
	CHECK_RES (" 1.5 -  0.5  =  1.0:  ", r4, FP_S_ONE)

	LOAD_CONST (r5,FP_S_1_5)
	LOAD_CONST (r6,FP_S_ONE)
	lf.sub.s  r4,r5,r6
	CHECK_RES (" 1.5 -  1.5  =  0.5:  ", r4, FP_S_0_5)

	/* Subtraction with negative numbers */
	LOAD_CONST (r5,FP_S_TWO)
	LOAD_CONST (r6,FP_S_N_ONE)
	lf.sub.s  r4,r5,r6
	CHECK_RES (" 2.0 - -1.0  =  3.0:  ", r4, FP_S_THREE)

	LOAD_CONST (r5,FP_S_ONE)
	LOAD_CONST (r6,FP_S_N_TWO)
	lf.sub.s  r4,r5,r6
	CHECK_RES (" 1.0 - -2.0  =  3.0:  ", r4, FP_S_THREE)

	LOAD_CONST (r5,FP_S_ONE)
	LOAD_CONST (r6,FP_S_N_ONE)
	lf.sub.s  r4,r5,r6
	CHECK_RES (" 1.0 - -1.0  =  2.0:  ", r4, FP_S_TWO)

	LOAD_CONST (r5,FP_S_N_ONE)
	LOAD_CONST (r6,FP_S_TWO)
	lf.sub.s  r4,r5,r6
	CHECK_RES ("-1.0 -  2.0  = -3.0:  ", r4, FP_S_N_THREE)

	LOAD_CONST (r5,FP_S_ONE)
	LOAD_CONST (r6,FP_S_TWO)
	lf.sub.s  r4,r5,r6
	CHECK_RES (" 1.0 -  2.0  = -1.0:  ", r4, FP_S_N_ONE)

	LOAD_CONST (r5,FP_S_N_ONE)
	LOAD_CONST (r6,FP_S_N_ONE)
	lf.sub.s  r4,r5,r6
	CHECK_RES ("-1.0 - -1.0  =  0.0:  ", r4, FP_S_P_ZERO)

	/* Subtraction with infinities (more needed) */
	LOAD_CONST (r5,FP_S_ONE)
	LOAD_CONST (r6,FP_S_P_INF)
	lf.sub.s  r4,r5,r6
	CHECK_RES (" 1.0 - +inf  = -inf:  ", r4, FP_S_N_INF)

	LOAD_CONST (r5,FP_S_ONE)
	LOAD_CONST (r6,FP_S_N_INF)
	lf.sub.s  r4,r5,r6
	CHECK_RES (" 1.0 - -inf  = +inf:  ", r4, FP_S_P_INF)

	LOAD_CONST (r5,FP_S_P_INF)
	LOAD_CONST (r6,FP_S_P_INF)
	lf.sub.s  r4,r5,r6
	CHECK_RES ("+inf - +inf  = -NaN:  ", r4, FP_S_N_NAN)

	LOAD_CONST (r5,FP_S_P_INF)
	LOAD_CONST (r6,FP_S_N_INF)
	lf.sub.s  r4,r5,r6
	CHECK_RES ("+inf - -inf  = +inf:  ", r4, FP_S_P_INF)

	LOAD_CONST (r5,FP_S_N_INF)
	LOAD_CONST (r6,FP_S_N_INF)
	lf.sub.s  r4,r5,r6
	CHECK_RES ("-inf - -inf  = -NaN:  ", r4, FP_S_N_NAN)

	/* Subtraction with NaNs (more needed) */
	LOAD_CONST (r5,FP_S_ONE)
	LOAD_CONST (r6,FP_S_P_NAN)
	lf.sub.s  r4,r5,r6
	CHECK_RES (" 1.0 - +NaN  = +NaN:  ", r4, FP_S_P_NAN)

	LOAD_CONST (r5,FP_S_ONE)
	LOAD_CONST (r6,FP_S_N_NAN)
	lf.sub.s  r4,r5,r6
	CHECK_RES (" 1.0 - -NaN  = -NaN:  ", r4, FP_S_N_NAN)

	LOAD_CONST (r5,FP_S_P_NAN)
	LOAD_CONST (r6,FP_S_P_NAN)
	lf.sub.s  r4,r5,r6
	CHECK_RES ("+NaN - +NaN  = +NaN:  ", r4, FP_S_P_NAN)

	LOAD_CONST (r5,FP_S_P_NAN)
	LOAD_CONST (r6,FP_S_N_NAN)
	lf.sub.s  r4,r5,r6
	CHECK_RES ("+NaN - -NaN  = +NaN:  ", r4, FP_S_P_NAN)

	LOAD_CONST (r5,FP_S_N_NAN)
	LOAD_CONST (r6,FP_S_N_NAN)
	lf.sub.s  r4,r5,r6
	CHECK_RES ("-NaN - -NaN  = -NaN:  ", r4, FP_S_N_NAN)

	LOAD_CONST (r5,FP_S_ONE)
	LOAD_CONST (r6,FP_S_P_NAN_B)
	lf.sub.s  r4,r5,r6
	CHECK_RES (" 1.0 - +sNaN = +qNaN: ", r4, 0x7fc00001)

	LOAD_CONST (r5,FP_S_ONE)
	LOAD_CONST (r6,FP_S_N_NAN_B)
	lf.sub.s  r4,r5,r6
	CHECK_RES (" 1.0 - -sNaN = -qNaN: ", r4, 0xffc00001)

	/* Subtraction with overflow (more neeeded) */
	SET_RM	(FPCSR_RM_RIN)
	
	LOAD_CONST (r5,FP_S_N_HUGE2)
	LOAD_CONST (r6,FP_S_HUGE2)
	lf.sub.s  r4,r5,r6
	CHECK_RES ("-1.0 * 2^127 -  1.0 * 2^127  = +inf:  ", r4, FP_S_N_INF)

	SET_RM	(FPCSR_RM_RIP)
	LOAD_CONST (r5,FP_S_HUGE2)
	LOAD_CONST (r6,FP_S_N_HUGE2)
	lf.sub.s  r4,r5,r6
	CHECK_RES (" 1.0 * 2^127 - -1.0 * 2^127  = -inf:  ", r4, FP_S_P_INF)


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
