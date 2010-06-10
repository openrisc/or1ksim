/* is-add-test.S. l.add, l.addc, l.addi and l.addic instruction test of Or1ksim
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
 * Test coverage
 *
 * The l.add, l.addc, l.addi and l.addic instructions should set the carry and
 * overflow flags.
 *
 * In addition the l.addc and l.addic instructions should add in the carry
 * bit.
 *
 * Problems in this area were reported in Bugs 1771 and 1776. Having fixed the
 * problem, this is (in good software engineering style), a  regression test
 * to go with the fix.
 *
 * This is not a comprehensive test of any instruction (yet).
 *
 * Of course what is really needed is a comprehensive instruction test...
 * ------------------------------------------------------------------------- */


#include "inst-set-test.h"

/* ----------------------------------------------------------------------------
 * A macro to carry out a test of addition in registers
 *
 *
 * Arguments
 *   set_flags: Flags to set in the SR
 *   clr_flags: Flags to clear in the SR
 *   opc:       The opcode
 *   op1:       First operand value
 *   op2:       Second operand value
 *   res:       Expected result
 *   str:       Output string
 *   cy:        Expected carry flag
 *   ov:        Expected overflow flag
 * ------------------------------------------------------------------------- */
#define TEST_ADD(set_flags, clr_flags, opc, op1, op2, res, str, cy, ov)	 \
	l.mfspr	r3,r0,SPR_SR						;\
	LOAD_CONST (r2, set_flags)	/* Set flags */			;\
	l.or	r3,r3,r2						;\
	LOAD_CONST (r2, ~clr_flags)	/* Clear flags */		;\
	l.and	r3,r3,r2						;\
	l.mtspr	r0,r3,SPR_SR						;\
									;\
	LOAD_CONST (r5,op1)		/* Load numbers to add */	;\
	LOAD_CONST (r6,op2)						;\
	opc	r4,r5,r6						;\
	l.mfspr	r2,r0,SPR_SR		/* So we can examine flags */	;\
	PUSH (r2)							;\
	CHECK_RES (str, r4, res)					;\
	POP(r2)				/* Retrieve SR */		;\
	PUSH(r2)							;\
	LOAD_CONST (r4, SPR_SR_CY)	/* The carry bit */		;\
	l.and	r2,r2,r4						;\
	l.sfeq	r2,r4							;\
	CHECK_FLAG ("- carry flag set:    ", cy)			;\
									;\
	POP(r2)				/* Retrieve SR */		;\
	PUSH(r2)							;\
	LOAD_CONST (r4, SPR_SR_OV)	/* The overflow bit */		;\
	l.and	r2,r2,r4						;\
	l.sfeq	r2,r4							;\
	CHECK_FLAG ("- overflow flag set: ", ov)

/* ----------------------------------------------------------------------------
 * A macro to carry out a test of addition with an immediate value
 *
 *
 * Arguments
 *   set_flags: Flags to set in the SR
 *   clr_flags: Flags to clear in the SR
 *   opc:       The opcode
 *   op1:       First operand value
 *   op2:       Second operand value (immediate)
 *   res:       Expected result
 *   str:       Output string
 *   cy:        Expected carry flag
 *   ov:        Expected overflow flag
 * ------------------------------------------------------------------------- */
#define TEST_ADDI(set_flags, clr_flags, opc, op1, op2, res, str, cy, ov) \
	l.mfspr	r3,r0,SPR_SR						;\
	LOAD_CONST (r2, set_flags)	/* Set flags */			;\
	l.or	r3,r3,r2						;\
	LOAD_CONST (r2, ~clr_flags)	/* Clear flags */		;\
	l.and	r3,r3,r2						;\
	l.mtspr	r0,r3,SPR_SR						;\
									;\
	LOAD_CONST (r5,op1)		/* Load first number to add */	;\
	opc	r4,r5,op2						;\
	l.mfspr	r2,r0,SPR_SR		/* So we can examine flags */	;\
	PUSH (r2)							;\
	CHECK_RES (str, r4, res)					;\
	POP(r2)				/* Retrieve SR */		;\
	PUSH(r2)							;\
	LOAD_CONST (r4, SPR_SR_CY)	/* The carry bit */		;\
	l.and	r2,r2,r4						;\
	l.sfeq	r2,r4							;\
	CHECK_FLAG ("- carry flag set:    ", cy)			;\
									;\
	POP(r2)				/* Retrieve SR */		;\
	PUSH(r2)							;\
	LOAD_CONST (r4, SPR_SR_OV)	/* The overflow bit */		;\
	l.and	r2,r2,r4						;\
	l.sfeq	r2,r4							;\
	CHECK_FLAG ("- overflow flag set: ", ov)

	
	.section .text
	.global	_start
_start:

/* ----------------------------------------------------------------------------
 * Test of add signed, l.add
 * ------------------------------------------------------------------------- */
_add:
	LOAD_STR (r3, "l.add\n")
	l.jal	_puts
	l.nop

	/* Add two small positive numbers */
	TEST_ADD (0, SPR_SR_CY | SPR_SR_OV,
	          l.add, 1, 2, 3,
	          "0x00000001 + 0x00000002 = 0x00000003: ",
	          FALSE, FALSE)

	/* Check carry in is ignored. */
	TEST_ADD (SPR_SR_CY, SPR_SR_OV,
	          l.add, 1, 2, 3,
	          "0x00000001 + 0x00000002 = 0x00000003: ",
	          FALSE, FALSE)

	/* Add two small negative numbers. Sets the carry flag but not the
	   overflow flag. */
	TEST_ADD (0, SPR_SR_CY | SPR_SR_OV,
	          l.add, 0xffffffff, 0xfffffffe, 0xfffffffd,
	          "0xffffffff + 0xfffffffe = 0xfffffffd: ",
	          TRUE, FALSE)

	/* Add two quite large positive numbers. Should set neither the
	   overflow nor the carry flag. */
	TEST_ADD (0, SPR_SR_CY | SPR_SR_OV,
	          l.add, 0x40000000, 0x3fffffff, 0x7fffffff,
	          "0x40000000 + 0x3fffffff = 0x7fffffff: ",
	          FALSE, FALSE)

	/* Add two large positive numbers. Should set the overflow, but not
	   the carry flag. */
	TEST_ADD (0, SPR_SR_CY | SPR_SR_OV,
	          l.add, 0x40000000, 0x40000000, 0x80000000,
	          "0x40000000 + 0x40000000 = 0x80000000: ",
	          FALSE, TRUE)

	/* Add two quite large negative numbers. Should set the carry, but not
	   the overflow flag. */
	TEST_ADD (0, SPR_SR_CY | SPR_SR_OV,
	          l.add, 0xc0000000, 0xc0000000, 0x80000000,
	          "0xc0000000 + 0xc0000000 = 0x80000000: ",
	          TRUE, FALSE)

	/* Add two large negative numbers. Should set both the overflow and
	   carry flags. */
	TEST_ADD (0, SPR_SR_CY | SPR_SR_OV,
	          l.add, 0xbfffffff, 0xbfffffff, 0x7ffffffe,
	          "0xbfffffff + 0xbfffffff = 0x7ffffffe: ",
	          TRUE, TRUE)

	/* Check that range exceptions are triggered */
	l.mfspr	r3,r0,SPR_SR
	LOAD_CONST (r2, SPR_SR_OVE)	/* Set OVE */
	l.or	r3,r3,r2
	l.mtspr	r0,r3,SPR_SR
	
	LOAD_STR (r3, "  ** OVE flag set **\n")
	l.jal	_puts
	l.nop

	/* Check that an overflow alone causes a RANGE Exception. */
	TEST_ADD (0, SPR_SR_CY | SPR_SR_OV,
	          l.add, 0x40000000, 0x40000000, 0x80000000,
	          "0x40000000 + 0x40000000 = 0x80000000: ",
	          FALSE, TRUE)

	/* Check that a carry alone does not cause a RANGE Exception. */
	TEST_ADD (0, SPR_SR_CY | SPR_SR_OV,
	          l.add, 0xffffffff, 0xfffffffe, 0xfffffffd,
	          "0xffffffff + 0xfffffffe = 0xfffffffd: ",
	          TRUE, FALSE)

	/* Check that carry and overflow together cause an exception. */
	TEST_ADD (0, SPR_SR_CY | SPR_SR_OV,
	          l.add, 0xbfffffff, 0xbfffffff, 0x7ffffffe,
	          "0xbfffffff + 0xbfffffff = 0x7ffffffe: ",
	          TRUE, TRUE)

	/* Finished checking range exceptions */
	l.mfspr	r3,r0,SPR_SR
	LOAD_CONST (r2, ~SPR_SR_OVE)	/* Clear OVE */
	l.and	r3,r3,r2
	l.mtspr	r0,r3,SPR_SR
	
	LOAD_STR (r3, "  ** OVE flag cleared **\n")
	l.jal	_puts
	l.nop

/* ----------------------------------------------------------------------------
 * Test of add signed and carry, l.addc
 * ------------------------------------------------------------------------- */
_addc:
	LOAD_STR (r3, "l.addc\n")
	l.jal	_puts
	l.nop

	/* Add two small positive numbers */
	TEST_ADD (0, SPR_SR_CY | SPR_SR_OV,
	          l.addc, 1, 2, 3,
	          "0x00000001 + 0x00000002     = 0x00000003: ",
	          FALSE, FALSE)

	/* Add two small negative numbers. Sets the carry flag but not the
	   overflow flag. */
	TEST_ADD (0, SPR_SR_CY | SPR_SR_OV,
	          l.addc, 0xffffffff, 0xfffffffe, 0xfffffffd,
	          "0xffffffff + 0xfffffffe     = 0xfffffffd: ",
	          TRUE, FALSE)

	/* Add two quite large positive numbers. Should set neither the
	   overflow nor the carry flag. */
	TEST_ADD (0, SPR_SR_CY | SPR_SR_OV,
	          l.addc, 0x40000000, 0x3fffffff, 0x7fffffff,
	          "0x40000000 + 0x3fffffff     = 0x7fffffff: ",
	          FALSE, FALSE)

	/* Add two quite large positive numbers with a carry in. Should set
	   the overflow but not the carry flag. */
	TEST_ADD (SPR_SR_CY, SPR_SR_OV,
	          l.addc, 0x40000000, 0x3fffffff, 0x80000000,
	          "0x40000000 + 0x3fffffff + c = 0x80000000: ",
	          FALSE, TRUE)

	/* Add two large positive numbers. Should set the overflow, but not
	   the carry flag. */
	TEST_ADD (0, SPR_SR_CY | SPR_SR_OV,
	          l.addc, 0x40000000, 0x40000000, 0x80000000,
	          "0x40000000 + 0x40000000     = 0x80000000: ",
	          FALSE, TRUE)

	/* Add the largest unsigned value to zero with a carry. This
	   potentially can break a simplistic test for carry that does not
	   consider the carry flag properly. Do it both ways around. */
	TEST_ADD (SPR_SR_CY, SPR_SR_OV,
	          l.addc, 0xffffffff, 0x00000000, 0x00000000,
	          "0xffffffff + 0x00000000 + c = 0x00000000: ",
	          TRUE, FALSE)

	TEST_ADD (SPR_SR_CY, SPR_SR_OV,
	          l.addc, 0x00000000, 0xffffffff, 0x00000000,
	          "0x00000000 + 0xffffffff + c = 0x00000000: ",
	          TRUE, FALSE)

	/* Add two quite large negative numbers. Should set the carry, but not
	   the overflow flag. flag. */
	TEST_ADD (0, SPR_SR_CY | SPR_SR_OV,
	          l.addc, 0xc0000000, 0xc0000000, 0x80000000,
	          "0xc0000000 + 0xc0000000     = 0x80000000: ",
	          TRUE, FALSE)

	/* Add two quite large negative numbers that would overflow, with a
	   carry that just avoids the overflow. Should set the carry, but not
	   the overflow flag. flag. */
	TEST_ADD (SPR_SR_CY, SPR_SR_OV,
	          l.addc, 0xc0000000, 0xbfffffff, 0x80000000,
	          "0xc0000000 + 0xbfffffff + c = 0x80000000: ",
	          TRUE, FALSE)

	/* Add two large negative numbers. Should set both the overflow and
	   carry flags. */
	TEST_ADD (0, SPR_SR_CY | SPR_SR_OV,
	          l.addc, 0xbfffffff, 0xbfffffff, 0x7ffffffe,
	          "0xbfffffff + 0xbfffffff     = 0x7ffffffe: ",
	          TRUE, TRUE)

	/* Check that range exceptions are triggered */
	l.mfspr	r3,r0,SPR_SR
	LOAD_CONST (r2, SPR_SR_OVE)	/* Set OVE */
	l.or	r3,r3,r2
	l.mtspr	r0,r3,SPR_SR
	
	LOAD_STR (r3, "  ** OVE flag set **\n")
	l.jal	_puts
	l.nop

	/* Check that an overflow alone causes a RANGE Exception, even when it
	   is the carry that causes the overflow. */
	TEST_ADD (0, SPR_SR_CY | SPR_SR_OV,
	          l.addc, 0x40000000, 0x40000000, 0x80000000,
	          "0x40000000 + 0x40000000     = 0x80000000: ",
	          FALSE, TRUE)

	TEST_ADD (SPR_SR_CY, SPR_SR_OV,
	          l.addc, 0x40000000, 0x3fffffff, 0x80000000,
	          "0x40000000 + 0x3fffffff + c = 0x80000000: ",
	          FALSE, TRUE)

	/* Check that a carry alone does not cause a RANGE Exception, even
	   when it is the carry that causes the overflow. */
	TEST_ADD (0, SPR_SR_CY | SPR_SR_OV,
	          l.addc, 0xffffffff, 0xfffffffe, 0xfffffffd,
	          "0xffffffff + 0xfffffffe     = 0xfffffffd: ",
	          TRUE, FALSE)

	TEST_ADD (SPR_SR_CY, SPR_SR_OV,
	          l.addc, 0x00000000, 0xffffffff, 0x00000000,
	          "0x00000000 + 0xffffffff + c = 0x00000000: ",
	          TRUE, FALSE)

	/* Check that carry and overflow together cause an exception. */
	TEST_ADD (0, SPR_SR_CY | SPR_SR_OV,
	          l.addc, 0xbfffffff, 0xbfffffff, 0x7ffffffe,
	          "0xbfffffff + 0xbfffffff     = 0x7ffffffe: ",
	          TRUE, TRUE)

	/* Finished checking range exceptions */
	l.mfspr	r3,r0,SPR_SR
	LOAD_CONST (r2, ~SPR_SR_OVE)	/* Clear OVE */
	l.and	r3,r3,r2
	l.mtspr	r0,r3,SPR_SR
	
	LOAD_STR (r3, "  ** OVE flag cleared **\n")
	l.jal	_puts
	l.nop

/* ----------------------------------------------------------------------------
 * Test of add signed immediate, l.addi
 * ------------------------------------------------------------------------- */
_addi:
	LOAD_STR (r3, "l.addi\n")
	l.jal	_puts
	l.nop

	/* Add two small positive numbers */
	TEST_ADDI (0, SPR_SR_CY | SPR_SR_OV,
	           l.addi, 1, 2, 3,
	           "0x00000001 + 0x00000002 = 0x00000003: ",
	           FALSE, FALSE)

	/* Check carry in is ignored. */
	TEST_ADDI (SPR_SR_CY, SPR_SR_OV,
	           l.addi, 1, 2, 3,
	           "0x00000001 + 0x00000002 = 0x00000003: ",
	           FALSE, FALSE)

	/* Add two small negative numbers. Sets the carry flag but not the
	   overflow flag. */
	TEST_ADDI (0, SPR_SR_CY | SPR_SR_OV,
	           l.addi, 0xffffffff, 0xfffe, 0xfffffffd,
	           "0xffffffff + 0xfffffffe = 0xfffffffd: ",
	           TRUE, FALSE)

	/* Add two quite large positive numbers. Should set neither the
	   overflow nor the carry flag. */
	TEST_ADDI (0, SPR_SR_CY | SPR_SR_OV,
	           l.addi, 0x7fff8000, 0x7fff, 0x7fffffff,
	           "0x7fff8000 + 0x00007fff = 0x7fffffff: ",
	           FALSE, FALSE)

	/* Add two large positive numbers. Should set the overflow, but not
	   the carry flag. */
	TEST_ADDI (0, SPR_SR_CY | SPR_SR_OV,
	           l.addi, 0x7fffc000, 0x4000, 0x80000000,
	           "0x7fffc000 + 0x00004000 = 0x80000000: ",
	           FALSE, TRUE)

	/* Add two quite large negative numbers. Should set the carry, but not
	   the overflow flag. */
	TEST_ADDI (0, SPR_SR_CY | SPR_SR_OV,
	           l.addi, 0x80008000, 0x8000, 0x80000000,
	           "0x80008000 + 0xffff8000 = 0x80000000: ",
	           TRUE, FALSE)

	/* Add two large negative numbers. Should set both the overflow and
	   carry flags. */
	TEST_ADDI (0, SPR_SR_CY | SPR_SR_OV,
	           l.addi, 0x80007fff, 0x8000, 0x7fffffff,
	           "0x80007fff + 0xffff8000 = 0x7fffffff: ",
	           TRUE, TRUE)

	/* Check that range exceptions are triggered */
	l.mfspr	r3,r0,SPR_SR
	LOAD_CONST (r2, SPR_SR_OVE)	/* Set OVE */
	l.or	r3,r3,r2
	l.mtspr	r0,r3,SPR_SR
	
	LOAD_STR (r3, "  ** OVE flag set **\n")
	l.jal	_puts
	l.nop

	/* Check that an overflow alone causes a RANGE Exception. */
	TEST_ADDI (0, SPR_SR_CY | SPR_SR_OV,
	           l.addi, 0x7fffc000, 0x4000, 0x80000000,
	           "0x7fffc000 + 0x00004000 = 0x80000000: ",
	           FALSE, TRUE)

	/* Check that a carry alone does not cause a RANGE Exception. */
	TEST_ADDI (0, SPR_SR_CY | SPR_SR_OV,
	           l.addi, 0xffffffff, 0xfffe, 0xfffffffd,
	           "0xffffffff + 0xfffffffe = 0xfffffffd: ",
	           TRUE, FALSE)

	/* Check that carry and overflow together cause an exception. */
	TEST_ADDI (0, SPR_SR_CY | SPR_SR_OV,
	           l.addi, 0x80007fff, 0x8000, 0x7fffffff,
	           "0x80007fff + 0xffff8000 = 0x7ffffffe: ",
	           TRUE, TRUE)

	/* Finished checking range exceptions */
	l.mfspr	r3,r0,SPR_SR
	LOAD_CONST (r2, ~SPR_SR_OVE)	/* Clear OVE */
	l.and	r3,r3,r2
	l.mtspr	r0,r3,SPR_SR
	
	LOAD_STR (r3, "  ** OVE flag cleared **\n")
	l.jal	_puts
	l.nop

/* ----------------------------------------------------------------------------
 * Test of add signed and carry, l.addic
 * ------------------------------------------------------------------------- */
_addic:
	LOAD_STR (r3, "l.addic\n")
	l.jal	_puts
	l.nop

	/* Add two small positive numbers */
	TEST_ADDI (0, SPR_SR_CY | SPR_SR_OV,
	           l.addic, 1, 2, 3,
	           "0x00000001 + 0x00000002     = 0x00000003: ",
	           FALSE, FALSE)

	/* Add two small negative numbers. Sets the carry flag but not the
	   overflow flag. */
	TEST_ADDI (0, SPR_SR_CY | SPR_SR_OV,
	           l.addic, 0xffffffff, 0xfffe, 0xfffffffd,
	           "0xffffffff + 0xfffffffe     = 0xfffffffd: ",
	           TRUE, FALSE)

	/* Add two quite large positive numbers. Should set neither the
	   overflow nor the carry flag. */
	TEST_ADDI (0, SPR_SR_CY | SPR_SR_OV,
	           l.addic, 0x7fff8000, 0x7fff, 0x7fffffff,
	           "0x7fff8000 + 0x00007fff     = 0x7fffffff: ",
	           FALSE, FALSE)

	/* Add two quite large positive numbers with a carry in. Should set
	   the overflow but not the carry flag. */
	TEST_ADDI (SPR_SR_CY, SPR_SR_OV,
	           l.addic, 0x7fff8000, 0x7fff, 0x80000000,
	           "0x7fff8000 + 0x00007fff + c = 0x80000000: ",
	           FALSE, TRUE)

	/* Add two large positive numbers. Should set the overflow, but not
	   the carry flag. */
	TEST_ADDI (0, SPR_SR_CY | SPR_SR_OV,
	           l.addic, 0x7fffc000, 0x4000, 0x80000000,
	           "0x7fffc000 + 0x00004000     = 0x80000000: ",
	           FALSE, TRUE)

	/* Add the largest unsigned value to zero with a carry. This
	   potentially can break a simplistic test for carry that does not
	   consider the carry flag properly. Do it both ways around. */
	TEST_ADDI (SPR_SR_CY, SPR_SR_OV,
	           l.addic, 0xffffffff, 0x0000, 0x00000000,
	           "0xffffffff + 0x00000000 + c = 0x00000000: ",
	           TRUE, FALSE)

	TEST_ADDI (SPR_SR_CY, SPR_SR_OV,
	           l.addic, 0x00000000, 0xffff, 0x00000000,
	           "0x00000000 + 0xffffffff + c = 0x00000000: ",
	           TRUE, FALSE)

	/* Add two quite large negative numbers. Should set the carry, but not
	   the overflow flag. flag. */
	TEST_ADDI (0, SPR_SR_CY | SPR_SR_OV,
	           l.addic, 0x80008000, 0x8000, 0x80000000,
	           "0x80008000 + 0xffff8000     = 0x80000000: ",
	           TRUE, FALSE)

	/* Add two quite large negative numbers that would overflow, with a
	   carry that just avoids the overflow. Should set the carry, but not
	   the overflow flag. flag. */
	TEST_ADDI (SPR_SR_CY, SPR_SR_OV,
	           l.addic, 0x80007fff, 0x8000, 0x80000000,
	           "0x80007fff + 0xffff8000 + c = 0x80000000: ",
	           TRUE, FALSE)

	/* Add two large negative numbers. Should set both the overflow and
	   carry flags. */
	TEST_ADDI (0, SPR_SR_CY | SPR_SR_OV,
	           l.addic, 0x80007fff, 0x8000, 0x7fffffff,
	           "0x80007fff + 0xffff8000     = 0x7fffffff: ",
	           TRUE, TRUE)

	/* Check that range exceptions are triggered */
	l.mfspr	r3,r0,SPR_SR
	LOAD_CONST (r2, SPR_SR_OVE)	/* Set OVE */
	l.or	r3,r3,r2
	l.mtspr	r0,r3,SPR_SR
	
	LOAD_STR (r3, "  ** OVE flag set **\n")
	l.jal	_puts
	l.nop

	/* Check that an overflow alone causes a RANGE Exception, even when it
	   is the carry that causes the overflow. */
	TEST_ADDI (0, SPR_SR_CY | SPR_SR_OV,
	           l.addic, 0x7fffc000, 0x4000, 0x80000000,
	           "0x7fffc000 + 0x00004000     = 0x80000000: ",
	           FALSE, TRUE)

	TEST_ADDI (SPR_SR_CY, SPR_SR_OV,
	           l.addic, 0x7fffc000, 0x3fff, 0x80000000,
	           "0x7fffc000 + 0x00003fff + c = 0x80000000: ",
	           FALSE, TRUE)

	/* Check that a carry alone does not cause a RANGE Exception, even
	   when it is the carry that causes the overflow. */
	TEST_ADDI (0, SPR_SR_CY | SPR_SR_OV,
	           l.addic, 0xffffffff, 0xfffe, 0xfffffffd,
	           "0xffffffff + 0xfffffffe     = 0xfffffffd: ",
	           TRUE, FALSE)

	TEST_ADDI (SPR_SR_CY, SPR_SR_OV,
	           l.addic, 0x00000000, 0xffff, 0x00000000,
	           "0x00000000 + 0xffffffff + c = 0x00000000: ",
	           TRUE, FALSE)

	/* Check that carry and overflow together cause an exception. */
	TEST_ADDI (0, SPR_SR_CY | SPR_SR_OV,
	           l.addic, 0x80007fff, 0x8000, 0x7fffffff,
	           "0x80007fff + 0xffff8000     = 0x7ffffffe: ",
	           TRUE, TRUE)

	/* Finished checking range exceptions */
	l.mfspr	r3,r0,SPR_SR
	LOAD_CONST (r2, ~SPR_SR_OVE)	/* Clear OVE */
	l.and	r3,r3,r2
	l.mtspr	r0,r3,SPR_SR
	
	LOAD_STR (r3, "  ** OVE flag cleared **\n")
	l.jal	_puts
	l.nop

/* ----------------------------------------------------------------------------
 * All done
 * ------------------------------------------------------------------------- */
_exit:
	LOAD_STR (r3, "Test completed\n")
	l.jal	_puts
	l.nop

	TEST_EXIT