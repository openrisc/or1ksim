/* is-div-test.S. l.div and l.divu instruction test of Or1ksim
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
 * The l.div and l.divu instructions should set the carry flag as well as
 * triggering an event when divide by zero occurs.
 *
 * Having fixed the problem, this is (in good software engineering style), a
 * regresison test to go with the fix.
 *
 * This is not a comprehensive test of either instruction (yet).
 *
 * Of course what is really needed is a comprehensive instruction test...
 * ------------------------------------------------------------------------- */


#include "inst-set-test.h"

/* ----------------------------------------------------------------------------
 * Test of divide l.div
 * ------------------------------------------------------------------------- */

	.section .text
	.global	_start
_start:
	/* Signed divide by zero */
_div:	
	LOAD_STR (r3, "l.div\n")
	l.jal	_puts
	l.nop

	LOAD_CONST (r2, ~SPR_SR_CY)	/* Clear the carry flag */
	l.mfspr	r3,r0,SPR_SR
	l.and	r3,r3,r2
	l.mtspr	r0,r3,SPR_SR

	LOAD_CONST (r5,1)		/* Set up args for division */
	LOAD_CONST (r6,0)
	l.div	r4,r5,r6

	l.mfspr	r2,r0,SPR_SR		/* So we can examine the carry flag */
	LOAD_CONST (r4, SPR_SR_CY)	/* The carry bit */
	l.and	r2,r2,r4
	l.sfeq	r2,r4
	CHECK_FLAG ("1 / 0 (with error) carry flag set: ", TRUE)

	/* Signed divide by zero */
_divu:	
	LOAD_STR (r3, "l.divu\n")
	l.jal	_puts
	l.nop

	LOAD_CONST (r2, ~SPR_SR_CY)	/* Clear the carry flag */
	l.mfspr	r3,r0,SPR_SR
	l.and	r3,r3,r2
	l.mtspr	r0,r3,SPR_SR

	LOAD_CONST (r5,1)		/* Set up args for division */
	LOAD_CONST (r6,0)
	l.divu	r4,r5,r6

	l.mfspr	r2,r0,SPR_SR		/* So we can examine the carry flag */
	LOAD_CONST (r4, SPR_SR_CY)	/* The carry bit */
	l.and	r2,r2,r4
	l.sfeq	r2,r4
	CHECK_FLAG ("1 / 0 (with error) carry flag set: ", TRUE)

/* ----------------------------------------------------------------------------
 * All done
 * ------------------------------------------------------------------------- */
_exit:
	LOAD_STR (r3, "Test completed\n")
	l.jal	_puts
	l.nop

	TEST_EXIT