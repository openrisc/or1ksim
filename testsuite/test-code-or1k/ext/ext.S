/* ext.S -- Tests the l.ext{b,h}{s,z} instructions

   Copyright (C) 2005 György `nog' Jeney <nog@sdf.lonestar.org>
   Copyright (C) 2010 Embecosm Limited

   Contributor György `nog' Jeney <nog@sdf.lonestar.org>
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
   This code is commented throughout for use with Doxygen.
   --------------------------------------------------------------------------*/

#include "spr-defs.h"

	.section .except,"ax"

	.org 0x100
	l.j	start_test
	l.nop
	.org 0x200
	l.jal	unhandled_except
	l.nop
	.org 0x300
	l.jal	unhandled_except
	l.nop
	.org 0x400
	l.jal	unhandled_except
	l.nop
	.org 0x500
	l.jal	unhandled_except
	l.nop
	.org 0x600
	l.jal	unhandled_except
	l.nop
	.org 0x700
	l.jal	unhandled_except
	l.nop
	.org 0x800
	l.jal	unhandled_except
	l.nop
	.org 0x900
	l.jal	unhandled_except
	l.nop
	.org 0xa00
	l.jal	unhandled_except
	l.nop
	.org 0xb00
	l.jal	unhandled_except
	l.nop
	.org 0xc00
	l.jal	unhandled_except
	l.nop
	.org 0xd00
	l.jal	unhandled_except
	l.nop
	.org 0xe00
	l.jal	unhandled_except
	l.nop
	.org 0xf00
	l.jal	unhandled_except
	l.nop

#define CHECK_EXT(insn, val, mask, high_mask) \
	l.movhi	r4,hi(val); \
	l.ori	r4,r4,lo(val); \
	l.ori	r3,r4,0; \
	l.nop	NOP_REPORT; \
	insn	r5,r4; \
	l.ori	r3,r5,0; \
	l.nop	NOP_REPORT; \
	\
	l.movhi	r6,hi(mask); \
	l.ori	r6,r6,lo(mask); \
	l.xori	r7,r6,-1; \
	\
	l.and	r8,r4,r6; \
	l.and	r9,r5,r6; \
	l.sfne	r8,r9; \
	l.bf	ext_fail; \
	l.nop; \
	l.and	r8,r5,r7; \
	\
	l.movhi	r7,hi(high_mask); \
	l.ori	r7,r7,lo(high_mask); \
	\
	l.sfne	r8,r7; \
	l.bf	ext_fail; \
	l.nop;

#define	CHECK_HIGH3_CLEAR(insn, val) CHECK_EXT(insn, val, 0x000000ff, 0)
#define	CHECK_HIGH3_SET(val) CHECK_EXT(l.extbs, val, 0x000000ff, 0xffffff00)
#define	CHECK_HIGH2_CLEAR(insn, val) CHECK_EXT(insn, val, 0x0000ffff, 0)
#define	CHECK_HIGH2_SET(val) CHECK_EXT(l.exths, val, 0x0000ffff, 0xffff0000)

#define CHECK_MOVE(insn, val) \
	l.movhi	r4,hi(val); \
	l.ori	r4,r4,lo(val); \
	l.ori	r3,r4,0; \
	l.nop	NOP_REPORT; \
	insn	r5,r4; \
	l.ori	r3,r5,0; \
	l.nop	NOP_REPORT; \
	l.sfne	r5,r4; \
	l.bf	ext_fail; \
	l.nop;

	.section .text
start_test:
    // Clear R0 on start-up. There is no guarantee that R0 is hardwired to zero,
    // and indeed it is not when simulating the or1200 Verilog core.
    l.andi  r0,r0,0x0
        
	/* Test l.extbs */
	CHECK_HIGH3_CLEAR(l.extbs, 0x7f)
	CHECK_HIGH3_CLEAR(l.extbs, 0x53)
	CHECK_HIGH3_CLEAR(l.extbs, 0xff53)
	CHECK_HIGH3_CLEAR(l.extbs, 0x1234)

	CHECK_HIGH3_SET(0xff)
	CHECK_HIGH3_SET(0x80)
	CHECK_HIGH3_SET(0xff80)
	CHECK_HIGH3_SET(0x7f80)
	CHECK_HIGH3_SET(0x7fff)

	/* Test l.extbz */
	CHECK_HIGH3_CLEAR(l.extbz, 0x7f)
	CHECK_HIGH3_CLEAR(l.extbz, 0x53)
	CHECK_HIGH3_CLEAR(l.extbz, 0xff53)
	CHECK_HIGH3_CLEAR(l.extbz, 0x1234)

	CHECK_HIGH3_CLEAR(l.extbz, 0xff)
	CHECK_HIGH3_CLEAR(l.extbz, 0x80)
	CHECK_HIGH3_CLEAR(l.extbz, 0xff80)
	CHECK_HIGH3_CLEAR(l.extbz, 0x7f80)
	CHECK_HIGH3_CLEAR(l.extbz, 0x7fff)

	/* Test l.exths */
	CHECK_HIGH2_CLEAR(l.exths, 0x7fff)
	CHECK_HIGH2_CLEAR(l.exths, 0x5233)
	CHECK_HIGH2_CLEAR(l.exths, 0xffff2f53)
	CHECK_HIGH2_CLEAR(l.exths, 0x12345678)

	CHECK_HIGH2_SET(0xffff)
	CHECK_HIGH2_SET(0x8000)
	CHECK_HIGH2_SET(0xff80)
	CHECK_HIGH2_SET(0x80008000)
	CHECK_HIGH2_SET(0x7fffffff)

	/* Test l.exthz */
	CHECK_HIGH2_CLEAR(l.exthz, 0x7fff)
	CHECK_HIGH2_CLEAR(l.exthz, 0x5233)
	CHECK_HIGH2_CLEAR(l.exthz, 0xffff2f53)
	CHECK_HIGH2_CLEAR(l.exthz, 0x12345678)

	CHECK_HIGH2_CLEAR(l.exthz, 0xffff)
	CHECK_HIGH2_CLEAR(l.exthz, 0x8000)
	CHECK_HIGH2_CLEAR(l.exthz, 0xff80)
	CHECK_HIGH2_CLEAR(l.exthz, 0x80008000)
	CHECK_HIGH2_CLEAR(l.exthz, 0x7fffffff)

	/* Test l.extws */
	CHECK_MOVE(l.extws, 0xffffffff)
	CHECK_MOVE(l.extws, 0x7fffffff)
	CHECK_MOVE(l.extws, 0x7fff7fff)
	CHECK_MOVE(l.extws, 0xffff7f7f)
	CHECK_MOVE(l.extws, 0xffffff7f)
	CHECK_MOVE(l.extws, 0xffff7fff)
	CHECK_MOVE(l.extws, 0x7fff7f7f)
	CHECK_MOVE(l.extws, 0x12345678)

	/* Test l.extwz */
	CHECK_MOVE(l.extwz, 0xffffffff)
	CHECK_MOVE(l.extwz, 0x7fffffff)
	CHECK_MOVE(l.extwz, 0x7fff7fff)
	CHECK_MOVE(l.extwz, 0xffff7f7f)
	CHECK_MOVE(l.extwz, 0xffffff7f)
	CHECK_MOVE(l.extwz, 0xffff7fff)
	CHECK_MOVE(l.extwz, 0x7fff7f7f)
	CHECK_MOVE(l.extwz, 0x12345678)

	l.movhi	r3,0xdead
	l.ori	r3,r3,0xdead
	l.nop	NOP_REPORT
	l.ori	r3,r0,0
	l.nop	NOP_EXIT

ext_fail:
	l.ori	r3,r0,0x1234
	l.nop	NOP_EXIT

unhandled_except:
	l.addi	r3,r9,-8
	l.nop	NOP_REPORT
	l.nop	NOP_EXIT

