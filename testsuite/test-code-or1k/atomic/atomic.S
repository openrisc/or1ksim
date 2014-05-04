/* atomic.S. Tests for Or1ksim atomic operation instructions.

   Copyright (C) 2014 Stefan Kristainsson <stefan.kristiansson@saunalahti.fi>

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
   This code is commented throughout for use with Doxygen.
   --------------------------------------------------------------------------*/

#include "or1k-asm.h"
#include "spr-defs.h"
#include "board.h"

#define MEM_RAM 0x00000000

        .extern excpt_syscall
	.section .text

/*
 * custom syscall exception handler - does nothing but return.
 * Used as a convinient way to create nop exceptions.
 */
excpt_atomic_syscall:
	l.rfe

/* Main entry */
	.global main
main:
	/* init - insert our custom syscall exception handler */
	l.movhi	r3,hi(excpt_syscall)
	l.ori	r3,r3,lo(excpt_syscall)
	l.movhi	r4,hi(excpt_atomic_syscall)
	l.ori	r4,r4,lo(excpt_atomic_syscall)
	l.sw	0(r3),r4

test1:
	/* 1st test - l.lwa load */
	l.ori	r3,r0,0x1
	l.nop	NOP_REPORT
	l.movhi	r5,hi(0x600d0001)
	l.ori	r5,r5,lo(0x600d0001)
	l.sw	0(r0),r5
	l.lwa	r4,0(r0)
	l.ori	r3,r4,0
	l.nop	NOP_REPORT
	l.sfeq	r4,r5
	l.bnf	fail
	 l.nop

test2:
	/* 2nd test - l.swa store that succeeds */
	l.ori	r3,r0,0x2
	l.nop	NOP_REPORT
	l.movhi	r5,hi(0x600d0002)
	l.ori	r5,r5,lo(0x600d0002)
	l.ori	r6,r5,0
	l.swa	0(r0),r5
	/* store the l.swa result in r5 */
	l.ori	r5,r0,0
	l.bnf	1f
	 l.nop
	l.ori	r5,r0,1
1:	l.lwz	r4,0(r0)
	l.ori	r3,r4,0
	l.nop	NOP_REPORT
	l.sfeq	r4,r6
	l.bnf	fail
	 l.nop

test3:
	/* 3rd test - check l.swa result (expect 1) */
	l.ori	r3,r0,0x3
	l.nop	NOP_REPORT
	l.ori	r3,r0,1
	l.sfeq	r5,r3 /* r5 holds the l.swa result from test 2 */
	l.bnf	fail
	 l.nop
	l.movhi	r3,hi(0x600d0003)
	l.ori	r3,r3,lo(0x600d0003)
	l.nop	NOP_REPORT

test4:
	/* 4th test - clear lock with a l.sw */
	l.ori	r3,r0,0x4
	l.nop	NOP_REPORT
	l.movhi	r5,hi(0x600d0004)
	l.ori	r5,r5,lo(0x600d0004)
	l.sw	0(r0),r5
	l.lwz	r3,0(r0)
	l.nop	NOP_REPORT
	l.sfeq	r5,r3
	l.bnf	fail
	 l.nop

test5:
	/* 5th test - l.swa store that fails */
	l.ori	r3,r0,0x5
	l.nop	NOP_REPORT
	l.movhi	r5,hi(0xdeadbeef)
	l.ori	r5,r5,lo(0xdeadbeef)
	l.ori	r6,r5,0
	l.swa	0(r0),r5
	/* store the l.swa result in r5 */
	l.ori	r5,r0,0
	l.bnf	1f
	 l.nop
	l.ori	r5,r0,1
1:	l.lwz	r4,0(r0) /* 0(r0) should contain: 0x600d0004 */
	l.sfeq	r4,r6
	l.bf	fail
	 l.nop
	l.movhi	r3,hi(0x600d0005)
	l.ori	r3,r3,lo(0x600d0005)
	l.nop	NOP_REPORT

test6:
	/* 6th test - check l.swa result (expect 0) */
	l.ori	r3,r0,0x6
	l.nop	NOP_REPORT
	l.ori	r3,r0,0
	l.sfeq	r5,r3 /* r5 holds the l.swa result from test 5 */
	l.bnf	fail
	 l.nop
	l.movhi	r3,hi(0x600d0006)
	l.ori	r3,r3,lo(0x600d0006)
	l.nop	NOP_REPORT

test7:
	/* 7th test - l.lwa/l.swa sequence aborted by an exception */
	l.ori	r3,r0,0x7
	l.nop	NOP_REPORT
	l.movhi	r5,hi(0x600d0007)
	l.ori	r5,r5,lo(0x600d0007)
	l.sw	0(r0),r5
	l.lwa	r4,0(r0)
	l.sfeq	r4,r5
	l.bnf	fail
	 l.nop
	/* force an exception, which will abort the l.lwa/l.swa */
	l.sys	0
	/* check that a l.swa fails after the exception */
	l.movhi	r5,hi(0xdeadbeef)
	l.ori	r5,r5,lo(0xdeadbeef)
	l.ori	r6,r5,0
	l.swa	0(r0),r5
	l.bf	fail /* l.swa result expected to be 0 */
	 l.nop
	l.lwz	r3,0(r0) /* 0(r0) should contain: 0x600d0007 */
	l.sfeq	r3,r6
	l.bf	fail
	 l.nop
	l.movhi	r3,hi(0x600d0007)
	l.ori	r3,r3,lo(0x600d0007)
	l.nop	NOP_REPORT

test8:
	/* 8th test - l.lwa/l.swa sequence aborted by an l.lwa */
	l.ori	r3,r0,0x8
	l.nop	NOP_REPORT
	l.movhi	r5,hi(0x600d0008)
	l.ori	r5,r5,lo(0x600d0008)
	l.sw	0(r0),r5
	l.sw	4(r0),r5
	l.lwa	r4,0(r0)
	l.lwa	r4,4(r0) /* abort the 0(r0) l.lwa */
	l.sfeq	r4,r5
	l.bnf	fail
	 l.nop
	/* check that a l.swa to 0(r0) fail */
	l.movhi	r5,hi(0xdeadbeef)
	l.ori	r5,r5,lo(0xdeadbeef)
	l.ori	r6,r5,0
	l.swa	0(r0),r5
	l.bf	fail /* l.swa result expected to be 0 */
	 l.nop
	l.lwz	r3,0(r0) /* 0(r0) should contain: 0x600d0008 */
	l.sfeq	r3,r6
	l.bf	fail
	 l.nop
	l.movhi	r3,hi(0x600d0008)
	l.ori	r3,r3,lo(0x600d0008)
	l.nop	NOP_REPORT

test9:
	/* 9th test - abort l.lwa/l.swa sequence by an l.swa (to other addr) */
	l.ori	r3,r0,0x9
	l.nop	NOP_REPORT
	l.movhi	r5,hi(0x600d0009)
	l.ori	r5,r5,lo(0x600d0009)
	l.sw	0(r0),r5
	l.lwa	r4,0(r0)
	l.sfeq	r4,r5
	l.bnf	fail
	 l.nop
	/* abort the l.lwa to 0x0 by an l.swa to 0x4 (and check that it fail) */
	l.swa	4(r0),r5
	l.bf	fail /* l.swa result expected to be 0 */
	 l.nop
	/* check that a l.swa to 0(r0) fail */
	l.movhi	r5,hi(0xdeadbeef)
	l.ori	r5,r5,lo(0xdeadbeef)
	l.ori	r6,r5,0
	l.swa	0(r0),r5
	l.bf	fail /* l.swa result expected to be 0 */
	 l.nop
	l.lwz	r3,0(r0) /* 0(r0) should contain: 0x600d0009 */
	l.sfeq	r3,r6
	l.bf	fail
	 l.nop
	l.movhi	r3,hi(0x600d0009)
	l.ori	r3,r3,lo(0x600d0009)
	l.nop	NOP_REPORT

success:
	/* exit(0) */
	l.ori	r3,r0,0
	l.nop	NOP_EXIT
	l.j	.
	 l.nop

fail:
	/* exit(1) */
	l.ori	r3,r0,1
	l.nop	NOP_EXIT
	l.j	.
	 l.nop
