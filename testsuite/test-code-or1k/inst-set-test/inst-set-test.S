/* inst-set-test.S. Instruction set test library for Or1ksim
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
 * This library contains the stack implementation and reset sequence and a set
 * of library functions.
 *
 * The functions provided here provide simple utilities that are useful when
 * writing tests in assembler.
 * ------------------------------------------------------------------------- */

#include "inst-set-test.h"

/* ----------------------------------------------------------------------------
 * Simple stack, will be pointed to by r1, which is the next empty slot
 * ------------------------------------------------------------------------- */
	.section .stack
	.balign	4
	.global	_stack
_stack:
	.space	0x1000,0x0

/* ----------------------------------------------------------------------------
 * Exception handling
 * ------------------------------------------------------------------------- */
	.section .boot-text

/* ----------------------------------------------------------------------------
 * Reset exception
 *
 * Set up the stack and jump to _start
 * ------------------------------------------------------------------------- */
	.org 0x100
	.global	_reset
_reset:
	l.movhi	r1,hi(_stack)		/* Set up the stack */
	l.ori	r1,r1,lo(_stack)

	l.j	_start			/* Jump to the start of code */
	l.nop

/* ----------------------------------------------------------------------------
 * Range exception
 *
 * Don't be tempted to use the LOAD_STR macro here, it will dump us back into
 * text space.
 *	
 * Print a message about the instruction triggering the exception. Then
 * replace it by l.nop and return.
 * ------------------------------------------------------------------------- */
	.section .rodata
50:	.string	"  RANGE exception\n"
51:	.string	"  - caused by: "
52:	.string	"  - SR value:  "

	.section .boot-text
	.org	0xb00
	.global	_range
_range:
	/* Note exception */
	LOAD_CONST (r3, 50b)
	l.jal	_puts
	l.nop

	/* Report problem instruction */
	LOAD_CONST (r3, 51b)
	l.jal	_puts
	l.nop

	l.mfspr	r2,r0,SPR_EPCR_BASE	/* Addr of problem instr */
	l.lws	r3,0(r2)		/* The actual instruction */
	l.nop	NOP_REPORT

	/* Report status register */
	LOAD_CONST (r3, 52b)
	l.jal	_puts
	l.nop

	l.mfspr	r3,r0,SPR_ESR_BASE	/* Status reg */
	l.nop	NOP_REPORT

	/* Patch with l.nop */
	l.mfspr	r2,r0,SPR_EPCR_BASE	/* Addr of problem instr */
	LOAD_CONST (r3, 0x15000000)	/* l.nop */
	l.sw	0(r2),r3

	/* All done */
	l.rfe
_range_end:	
	
/* ----------------------------------------------------------------------------
 * End of exception vectors
 *
 * Guarantee the exception vector space does not have general purpose code
 * ------------------------------------------------------------------------- */
	.org	0xffc
	l.nop				

/* ----------------------------------------------------------------------------
 * All subroutines are in the text section.
 * ------------------------------------------------------------------------- */
	.section .text

/* ----------------------------------------------------------------------------
 * Subroutine to print out a string
 *
 * The string is followed by a newline
 *
 * Parameters:
 *  r3  Pointer to the string to print
 * ------------------------------------------------------------------------- */
	.global	_puts
_puts:
	l.add	r2,r0,r3		/* Copy the string pointer */
	
	/* Loop getting and printing each char until end of string */
60:	l.lbz	r3,0(r2)
	l.sfeq	r3,r0			/* NULL termination? */
	l.bf	61f

	l.addi	r2,r2,1			/* Delay slot, move to next char */
	l.j	60b			/* Repeat */
	l.nop	NOP_PUTC		/* Delay slot */

61:	l.jr	r9			/* Return */
	l.nop

/* ----------------------------------------------------------------------------
 * Subroutine to print out a test name prompt
 *
 * The string is preceded by two spaces
 *
 * Parameters:
 *  r3  Pointer to the test name to print
 * ------------------------------------------------------------------------- */
	.global	_ptest
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
	l.nop

/* ----------------------------------------------------------------------------
 * Subroutine to print out "OK"
 *
 * The string is followed by a newline
 * ------------------------------------------------------------------------- */
	.global	_pok
_pok:
	PUSH(r9)			/* Save the return address */

	LOAD_STR(r3, "OK\n")
	l.jal	_puts
	l.nop

	POP (r9)
	l.jr	r9
	l.nop

/* ----------------------------------------------------------------------------
 * Subroutine to print out "Failed"
 *
 * The string is followed by a ": ", which will then allow a report
 * ------------------------------------------------------------------------- */
	.global	_pfail
_pfail:
	PUSH(r9)			/* Save the return address */

	LOAD_STR(r3, "Failed: ")
	l.jal	_puts
	l.nop

	POP (r9)
	l.jr	r9
	l.nop

/* ----------------------------------------------------------------------------
 * Subroutine to print out "TRUE"
 * ------------------------------------------------------------------------- */
	.global	_ptrue
_ptrue:
	PUSH(r9)			/* Save the return address */

	LOAD_STR(r3, "TRUE")
	l.jal	_puts
	l.nop

	POP (r9)
	l.jr	r9
	l.nop

/* ----------------------------------------------------------------------------
 * Subroutine to print out "FALSE"
 * ------------------------------------------------------------------------- */
	.global	_pfalse
_pfalse:
	PUSH(r9)			/* Save the return address */

	LOAD_STR(r3, "FALSE")
	l.jal	_puts
	l.nop

	POP (r9)
	l.jr	r9
	l.nop

/* ----------------------------------------------------------------------------
 * Subroutine to print out "unexpected"
 *
 * Preceded by a space and followed by a newline
 * ------------------------------------------------------------------------- */
	.global	_punexpected
_punexpected:
	PUSH(r9)			/* Save the return address */

	LOAD_STR(r3, " unexpected\n")
	l.jal	_puts
	l.nop

	POP (r9)
	l.jr	r9
	l.nop