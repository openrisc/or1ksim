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
 * All registers apart from r2, r9 and r11 are preserved across function calls.
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
    // Clear R0 on start-up. There is no guarantee that R0 is hardwired to zero,
    // and indeed it is not when simulating the or1200 Verilog core.
    l.andi  r0,r0,0x0
        
	l.movhi	r1,hi(_stack)		/* Set up the stack */
	l.ori	r1,r1,lo(_stack)

	l.j	_start			/* Jump to the start of code */
	l.nop

/* ----------------------------------------------------------------------------
 * Alignment exception
 *
 * Don't be tempted to use the LOAD_STR macro here, it will dump us back into
 * text section.
 *
 * The handling is a bit dubious at present. We just patch the instruction and
 * restart. This will go wrong in branch delay slots. Really we need to single
 * step past and then continue.
 *
 * Print a message identifying the exception type.
 * ------------------------------------------------------------------------- */
	.section .rodata
50:	.string	"  ALIGNMENT exception\n"

	.section .boot-text
	.org	0x600
	.global	_align
_align:
	/* Report exception */
	LOAD_CONST (r3, 50b)
	l.jal	_puts
	l.nop

	/* Patch with l.nop */
	l.mfspr	r2,r0,SPR_EPCR_BASE	/* Addr of problem instr */
	LOAD_CONST (r3, 0x15000000)	/* l.nop */
	l.sw	0(r2),r3

	/* All done */
	l.rfe
_align_end:	
	
/* ----------------------------------------------------------------------------
 * Illegal instruction exception
 *
 * Don't be tempted to use the LOAD_STR macro here, it will dump us back into
 * text section.
 *
 * The handling is a bit dubious at present. We just patch the instruction and
 * restart. This will go wrong in branch delay slots. Really we need to single
 * step past and then continue.
 *
 * Print a message identifying the exception type.
 * ------------------------------------------------------------------------- */
	.section .rodata
51:	.string	"  ILLEGAL INSTRUCTION exception\n"

	.section .boot-text
	.org	0x700
	.global	_illegal
_illegal:
	/* Report exception */
	LOAD_CONST (r3, 51b)
	l.jal	_puts
	l.nop

	/* Patch with l.nop */
	l.mfspr	r2,r0,SPR_EPCR_BASE	/* Addr of problem instr */
	LOAD_CONST (r3, 0x15000000)	/* l.nop */
	l.sw	0(r2),r3

	/* All done */
	l.rfe
_illegal_end:	
	
/* ----------------------------------------------------------------------------
 * Range exception
 *
 * Don't be tempted to use the LOAD_STR macro here, it will dump us back into
 * text section.
 *
 * The handling is a bit dubious at present. We just patch the instruction and
 * restart. This will go wrong in branch delay slots. Really we need to single
 * step past and then continue.
 *
 * Print a message identifying the exception type.
 * ------------------------------------------------------------------------- */
	.section .rodata
52:	.string	"  RANGE exception\n"

	.section .boot-text
	.org	0xb00
	.global	_range
_range:
	/* Report exception */
	LOAD_CONST (r3, 52b)
	l.jal	_puts
	l.nop

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
	PUSH (r3)
	l.add	r2,r0,r3		/* Copy the string pointer */
	
	/* Loop getting and printing each char until end of string */
60:	l.lbz	r3,0(r2)
	l.sfeq	r3,r0			/* NULL termination? */
	l.bf	61f

	l.addi	r2,r2,1			/* Delay slot, move to next char */
	l.j	60b			/* Repeat */
	l.nop	NOP_PUTC		/* Delay slot */

61:	POP (r3)
	l.jr	r9			/* Return */
	l.nop

/* ----------------------------------------------------------------------------
 * Subroutine to print out a register in hex
 *
 * Parameters:
 *  r3  The value to print
 * ------------------------------------------------------------------------- */
	.section .rodata
62:	.string "0123456789abcdef"
	.section .text

	.global	_puth
_puth:
	PUSH (r3)
	PUSH (r4)
	
	l.add	r2,r0,r3		/* Copy the value pointer */
	LOAD_CONST (r4,62b)		/* Ptr to digit chars */
	
	l.srli	r3,r2,28		/* Print each digit in turn. */
	l.add	r3,r4,r3
	l.lbz	r3,0(r3)
	l.nop	NOP_PUTC

	l.srli	r3,r2,24
	l.andi	r3,r3,0xf
	l.add	r3,r4,r3
	l.lbz	r3,0(r3)
	l.nop	NOP_PUTC

	l.srli	r3,r2,20
	l.andi	r3,r3,0xf
	l.add	r3,r4,r3
	l.lbz	r3,0(r3)
	l.nop	NOP_PUTC

	l.srli	r3,r2,16
	l.andi	r3,r3,0xf
	l.add	r3,r4,r3
	l.lbz	r3,0(r3)
	l.nop	NOP_PUTC

	l.srli	r3,r2,12
	l.andi	r3,r3,0xf
	l.add	r3,r4,r3
	l.lbz	r3,0(r3)
	l.nop	NOP_PUTC

	l.srli	r3,r2,8
	l.andi	r3,r3,0xf
	l.add	r3,r4,r3
	l.lbz	r3,0(r3)
	l.nop	NOP_PUTC

	l.srli	r3,r2,4
	l.andi	r3,r3,0xf
	l.add	r3,r4,r3
	l.lbz	r3,0(r3)
	l.nop	NOP_PUTC

	l.andi	r3,r2,0xf
	l.add	r3,r4,r3
	l.lbz	r3,0(r3)
	l.nop	NOP_PUTC

	POP (r4)			/* Return */
	POP (r3)
	l.jr	r9
	l.nop

/* ----------------------------------------------------------------------------
 * Subroutine to print out the lower half of a register in hex
 *
 * Parameters:
 *  r3  The value to print
 * ------------------------------------------------------------------------- */
	.section .rodata
63:	.string "0123456789abcdef"
	.section .text

	.global	_puthh
_puthh:
	PUSH (r3)
	PUSH (r4)
	
	l.add	r2,r0,r3		/* Copy the value pointer */
	LOAD_CONST (r4,63b)		/* Ptr to digit chars */
	
	l.srli	r3,r2,12		/* Print each digit in turn. */
	l.andi	r3,r3,0xf
	l.add	r3,r4,r3
	l.lbz	r3,0(r3)
	l.nop	NOP_PUTC

	l.srli	r3,r2,8
	l.andi	r3,r3,0xf
	l.add	r3,r4,r3
	l.lbz	r3,0(r3)
	l.nop	NOP_PUTC

	l.srli	r3,r2,4
	l.andi	r3,r3,0xf
	l.add	r3,r4,r3
	l.lbz	r3,0(r3)
	l.nop	NOP_PUTC

	l.andi	r3,r2,0xf
	l.add	r3,r4,r3
	l.lbz	r3,0(r3)
	l.nop	NOP_PUTC

	POP (r4)			/* Return */
	POP (r3)
	l.jr	r9
	l.nop

/* ----------------------------------------------------------------------------
 * Subroutine to print out the lowest byte of a register in hex
 *
 * Parameters:
 *  r3  The value to print
 * ------------------------------------------------------------------------- */
	.section .rodata
63:	.string "0123456789abcdef"
	.section .text

	.global	_puthq
_puthq:
	PUSH (r3)
	PUSH (r4)
	
	l.add	r2,r0,r3		/* Copy the value pointer */
	LOAD_CONST (r4,63b)		/* Ptr to digit chars */
	
	l.srli	r3,r2,4			/* Print each digit in turn. */
	l.andi	r3,r3,0xf
	l.add	r3,r4,r3
	l.lbz	r3,0(r3)
	l.nop	NOP_PUTC

	l.andi	r3,r2,0xf
	l.add	r3,r4,r3
	l.lbz	r3,0(r3)
	l.nop	NOP_PUTC

	POP (r4)			/* Return */
	POP (r3)
	l.jr	r9
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
	PUSH (r9)			/* Save the return address */
	PUSH (r3)			/* Save the test name for later */

	LOAD_STR (r3, "  ")		/* Prefix */
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
	PUSH (r9)			/* Save the return address */
	PUSH (r3)

	LOAD_STR (r3, "OK\n")
	l.jal	_puts
	l.nop

	POP (r3)
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
	PUSH (r9)			/* Save the return address */
	PUSH (r3)

	LOAD_STR (r3, "Failed: ")
	l.jal	_puts
	l.nop

	POP (r3)
	POP (r9)
	l.jr	r9
	l.nop

/* ----------------------------------------------------------------------------
 * Subroutine to print out "TRUE"
 * ------------------------------------------------------------------------- */
	.global	_ptrue
_ptrue:
	PUSH (r9)			/* Save the return address */
	PUSH (r3)

	LOAD_STR (r3, "TRUE")
	l.jal	_puts
	l.nop

	POP (r3)
	POP (r9)
	l.jr	r9
	l.nop

/* ----------------------------------------------------------------------------
 * Subroutine to print out "FALSE"
 * ------------------------------------------------------------------------- */
	.global	_pfalse
_pfalse:
	PUSH (r9)			/* Save the return address */
	PUSH (r3)

	LOAD_STR (r3, "FALSE")
	l.jal	_puts
	l.nop

	POP (r3)
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
	PUSH (r9)			/* Save the return address */
	PUSH (r3)

	LOAD_STR (r3, " unexpected\n")
	l.jal	_puts
	l.nop

	POP (r3)
	POP (r9)
	l.jr	r9
	l.nop
