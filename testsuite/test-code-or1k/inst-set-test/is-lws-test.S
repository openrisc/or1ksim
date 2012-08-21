/* is-lws-test.S. l.lws instruction test of Or1ksim
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
 * The l.lws instruction was omitted from Or1ksim originally. It is specified
 * for ORBIS32, even though it is functionally equivalent to l.lwz.
 *
 * Having fixed the problem, this is (in good software engineering style), a
 * regresison test to go with the fix.
 *
 * Of course what is really needed is a comprehensive instruction test...
 * ------------------------------------------------------------------------- */


#include "inst-set-test.h"

/* ----------------------------------------------------------------------------
 * Test of load single word and extend with sign: l.lws
 * ------------------------------------------------------------------------- */
	.section .rodata
	.balign	4
50:	.word	0xdeadbeef
51:	.word	0x00000000
52:	.word	0x7fffffff
53:	.word	0x80000000
54:	.word	0xffffffff

	.section .text
	.global	_start
_start:
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

	TEST_EXIT
