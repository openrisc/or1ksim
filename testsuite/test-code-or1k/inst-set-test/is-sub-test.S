/* is-sub-test.S. l.sub instruction test of Or1ksim
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
 * The l.sub instruction should set the carry and overflow flags.
 *
 * Problems in this area were reported in Bugs 1782, 1783 and 1784. Having
 * fixed the problem, this is (in good software engineering style), a
 * regression test to go with the fix.
 *
 * This is not a comprehensive test of any instruction (yet).
 *
 * Of course what is really needed is a comprehensive instruction test...
 * ------------------------------------------------------------------------- */


#include "inst-set-test.h"

/* ----------------------------------------------------------------------------
 * A macro to carry out a test of subtraction in registers
 *
 *
 * Arguments
 *   set_flags: Flags to set in the SR
 *   clr_flags: Flags to clear in the SR
 *   op1:       First operand value
 *   op2:       Second operand value
 *   res:       Expected result
 *   cy:        Expected carry flag
 *   ov:        Expected overflow flag
 * ------------------------------------------------------------------------- */
#define TEST_SUB(set_flags, clr_flags, op1, op2, res, cy, ov, aesr)	 \
	l.mfspr	r3,r0,SPR_SR						;\
	LOAD_CONST (r2, set_flags)	/* Set flags */			;\
	l.or	r3,r3,r2						;\
	LOAD_CONST (r2, ~clr_flags)	/* Clear flags */		;\
	l.and	r3,r3,r2						;\
	l.mtspr	r0,r3,SPR_SR						;\
									;\
	LOAD_CONST (r5,op1)		/* Load numbers to subtract */	;\
	LOAD_CONST (r6,op2)						;\
	l.mtspr	r0,r0,SPR_EPCR_BASE	/* Clear record */		;\
        l.mtspr	r0,r0,SPR_AESR						;\
50:	l.sub	r4,r5,r6						;\
	l.mfspr	r2,r0,SPR_SR		/* So we can examine flags */	;\
	l.mfspr	r5,r0,SPR_EPCR_BASE	/* What triggered exception */	;\
	l.mfspr	r6,r0,SPR_AESR						;\
									;\
        /* clear OVE so intervening l.add instructions don't trigger */	;\
        /* an exception */						;\
        LOAD_CONST(r7, ~SPR_SR_OVE)					;\
        l.and	r7,r2,r7						;\
        l.mtspr	r0,r7,SPR_SR						;\
									;\
        PUSH (r2)			/* Save SR for later */		;\
	PUSH (r6)			/* Save AESR for later */	;\
	PUSH (r5)			/* Save EPCR for later */	;\
	PUSH (r2)							;\
	PUSH (r4)			/* Save result for later */	;\
									;\
	PUTS ("  0x")							;\
	PUTH (op1)							;\
	PUTS (" - 0x")							;\
	PUTH (op2)							;\
	PUTS (" = 0x")							;\
	PUTH (res)							;\
	PUTS (": ")							;\
	POP (r4)							;\
	CHECK_RES1 (r4, res)						;\
									;\
	POP(r2)				/* Retrieve SR */		;\
	PUSH(r2)							;\
	LOAD_CONST (r4, SPR_SR_CY)	/* The carry bit */		;\
	l.and	r2,r2,r4						;\
	l.sfeq	r2,r4							;\
	CHECK_FLAG ("- carry flag set:      ", cy)			;\
									;\
	POP(r2)				/* Retrieve SR */		;\
	LOAD_CONST (r4, SPR_SR_OV)	/* The overflow bit */		;\
	l.and	r2,r2,r4						;\
	l.sfeq	r2,r4							;\
	CHECK_FLAG ("- overflow flag set:   ", ov)			;\
									;\
	POP (r2)			/* Retrieve EPCR */		;\
	LOAD_CONST (r4, 50b)		/* The opcode of interest */	;\
	l.and	r2,r2,r4						;\
	l.sfeq	r2,r4							;\
	OR1K_DELAYED_NOP(l.bnf	53f)					;\
									;\
	PUTS ("  - exception triggered: TRUE\n")			;\
									;\
	/* Check if AECSRP is set */					;\
	l.mfspr	r2,r0,SPR_CPUCFGR	/* Retrieve CPUCFGR */		;\
	LOAD_CONST (r4, SPR_CPUCFGR_AECSRP)				;\
	l.and	r2,r2,r4						;\
	l.sfeq	r2,r4							;\
	/* If AECSRP is not set skip to where AESR is popped at end */	;\
	OR1K_DELAYED_NOP(l.bnf	54f)					;\
									;\
        PUTS ("  - AESR ")						;\
        PUTH (aesr)							;\
        PUTS (":       ")						;\
	POP (r2)			/* Retrieve AESR */		;\
        CHECK_RES1 (r2, aesr)						;\
									;\
	l.j	55f							;\
	l.nop								;\
									;\
53:	PUTS ("  - exception triggered: FALSE\n")			;\
	/* If exception wasn't triggered we still need to pop AESR */	;\
54:	POP (r2)			/* Retrieve AESR */		;\
55:     /* Restore SR (we cleared OVE) */				;\
	POP (r2)			/* Retrive SR */		;\
        l.mtspr	r0,r2,SPR_SR



/* ----------------------------------------------------------------------------
 * Start of code
 * ------------------------------------------------------------------------- */
	.section .text
	.global	_start
_start:
	l.mfspr	r3,r0,SPR_SR
	LOAD_CONST (r2, ~SPR_SR_OVE)	/* Clear OVE */
	l.and	r3,r3,r2
	l.mtspr	r0,r3,SPR_SR
	
	LOAD_STR (r3, "  ** OVE flag cleared **\n")
	l.jal	_puts
	l.nop

/* ----------------------------------------------------------------------------
 * Test of subtract signed, l.sub
 * ------------------------------------------------------------------------- */
_sub:
	LOAD_STR (r3, "l.sub\n")
	l.jal	_puts
	l.nop

	/* Subtract two small positive numbers. Sets the carry, but never the
	   overflow if the result is negative. */
	TEST_SUB (0, SPR_SR_CY | SPR_SR_OV,
	          0x00000003, 0x00000002, 0x00000001,
	          FALSE, FALSE, 0)

	TEST_SUB (0, SPR_SR_CY | SPR_SR_OV,
	          0x00000001, 0x00000002, 0xffffffff,
	          TRUE, FALSE, 0)

	/* Check carry in is ignored. */
	TEST_SUB (SPR_SR_CY, SPR_SR_OV,
	          0x00000003, 0x00000002, 0x00000001,
	          FALSE, FALSE, 0)

	/* Subtract two small negative numbers. Sets the carry flag if the
	   result is negative, but never the overflow flag. */
	TEST_SUB (0, SPR_SR_CY | SPR_SR_OV,
	          0xfffffffd, 0xfffffffe, 0xffffffff,
	          TRUE, FALSE, 0)

	TEST_SUB (0, SPR_SR_CY | SPR_SR_OV,
	          0xffffffff, 0xfffffffe, 0x00000001,
	          FALSE, FALSE, 0)

	/* Subtract two quite large positive numbers. Should set neither the
	   overflow nor the carry flag. */
	TEST_SUB (0, SPR_SR_CY | SPR_SR_OV,
	          0x7fffffff, 0x3fffffff, 0x40000000,
	          FALSE, FALSE, 0)

	/* Subtract two quite large negative numbers. Should set neither the
	   overflow nor the carry flag. */
	TEST_SUB (0, SPR_SR_CY | SPR_SR_OV,
	          0x40000000, 0x40000000, 0x00000000,
	          FALSE, FALSE, 0)

	/* Subtract two large positive numbers with a negative result. Should
	   set the carry, but not the overflow flag. */
	TEST_SUB (0, SPR_SR_CY | SPR_SR_OV,
	          0x3fffffff, 0x40000000, 0xffffffff,
	          TRUE, FALSE, 0)

	/* Subtract two large negative numbers with a positive result. Should
	   set niether the carry nor the overflow flag. */
	TEST_SUB (0, SPR_SR_CY | SPR_SR_OV,
	          0x40000000, 0x3fffffff, 0x00000001,
	          FALSE, FALSE, 0)

	/* Subtract a large positive from a large negative number. Should set
	   overflow but not the carry flag. */
	TEST_SUB (0, SPR_SR_CY | SPR_SR_OV,
	          0x80000000, 0x7fffffff, 0x00000001,
	          FALSE, TRUE, 0)

	/* Subtract a large negative from a large positive number. Should set
	   both the overflow and carry flags. */
	TEST_SUB (0, SPR_SR_CY | SPR_SR_OV,
	          0x7fffffff, 0x80000000, 0xffffffff,
	          TRUE, TRUE, 0)

        /* check if AECSRP is set */
        l.mfspr r3,r0,SPR_CPUCFGR
        LOAD_CONST(r2, SPR_CPUCFGR_AECSRP)
        l.and r3,r3,r2
        l.sfeq r3,r2
        OR1K_DELAYED_NOP(l.bf _sub_aecsr)

_sub_no_aecsr:
	LOAD_STR (r3, "  ** OVE flag set **\n")
	OR1K_DELAYED_NOP(l.jal	_puts)

	/* Check that range exceptions are triggered */
	l.mfspr	r3,r0,SPR_SR
	LOAD_CONST (r2, SPR_SR_OVE)	/* Set OVE */
	l.or	r3,r3,r2
	l.mtspr	r0,r3,SPR_SR
	
	/* Check that an overflow alone causes a RANGE Exception. */
	TEST_SUB (0, SPR_SR_CY | SPR_SR_OV,
	          0x80000000, 0x7fffffff, 0x00000001,
	          FALSE, TRUE, 0)

	/* Check that a carry alone does not cause a RANGE Exception. */
	TEST_SUB (0, SPR_SR_CY | SPR_SR_OV,
	          0x3fffffff, 0x40000000, 0xffffffff,
	          TRUE, FALSE, 0)

	/* Check that carry and overflow together cause an exception. */
	TEST_SUB (0, SPR_SR_CY | SPR_SR_OV,
	          0x7fffffff, 0x80000000, 0xffffffff,
	          TRUE, TRUE, 0)

        OR1K_DELAYED_NOP(l.j _sub_except_done)

_sub_aecsr:
	LOAD_STR (r3, "  ** OVE flag set **\n")
	OR1K_DELAYED_NOP(l.jal	_puts)

	LOAD_STR (r3, "  ** AECR set to 0 **\n")
	OR1K_DELAYED_NOP(l.jal	_puts)
        
	/* Check that range exceptions are triggered */
	l.mfspr	r3,r0,SPR_SR
	LOAD_CONST (r2, SPR_SR_OVE)	/* Set OVE */
	l.or	r3,r3,r2
	l.mtspr	r0,r3,SPR_SR
	
        /* set AECR to 0 */
        l.mtspr r0,r0,SPR_AECR

	/* Check that an overflow alone does not cause a RANGE Exception. */
	TEST_SUB (0, SPR_SR_CY | SPR_SR_OV,
	          0x80000000, 0x7fffffff, 0x00000001,
	          FALSE, TRUE, 0)

	/* Check that a carry alone does not cause a RANGE Exception. */
	TEST_SUB (0, SPR_SR_CY | SPR_SR_OV,
	          0x3fffffff, 0x40000000, 0xffffffff,
	          TRUE, FALSE, 0)

	/* Check that carry and overflow together does not cause an exception. */
	TEST_SUB (0, SPR_SR_CY | SPR_SR_OV,
	          0x7fffffff, 0x80000000, 0xffffffff,
	          TRUE, TRUE, 0)

        /* Clear OVE so following printout won't raise an exception */
        l.mfspr r3,r0,SPR_SR
        LOAD_CONST (r2, ~SPR_SR_OVE)
        l.and r3,r2,r3
        l.mtspr r0,r3,SPR_SR
        
	LOAD_STR (r3, "  ** AECR set to OVADDE only **\n")
	OR1K_DELAYED_NOP(l.jal	_puts)
        
        /* Set OVE again */
	l.mfspr	r3,r0,SPR_SR
	LOAD_CONST (r2, SPR_SR_OVE)	/* Set OVE */
	l.or	r3,r3,r2
	l.mtspr	r0,r3,SPR_SR
	
        /* set AECR to OVADDE */
        LOAD_CONST(r2, SPR_AECR_OVADDE)
        l.mtspr r0,r2,SPR_AECR

	/* Check that an overflow alone causes a RANGE Exception. */
	TEST_SUB (0, SPR_SR_CY | SPR_SR_OV,
	          0x80000000, 0x7fffffff, 0x00000001,
	          FALSE, TRUE, SPR_AESR_OVADDE)

	/* Check that a carry alone does not cause a RANGE Exception. */
	TEST_SUB (0, SPR_SR_CY | SPR_SR_OV,
	          0x3fffffff, 0x40000000, 0xffffffff,
	          TRUE, FALSE, 0)

	/* Check that carry and overflow together cause an exception. */
	TEST_SUB (0, SPR_SR_CY | SPR_SR_OV,
	          0x7fffffff, 0x80000000, 0xffffffff,
	          TRUE, TRUE, SPR_AESR_OVADDE)

        /* Clear OVE so following printout won't raise an exception */
        l.mfspr r3,r0,SPR_SR
        LOAD_CONST (r2, ~SPR_SR_OVE)
        l.and r3,r2,r3
        l.mtspr r0,r3,SPR_SR
        
	LOAD_STR (r3, "  ** AECR set to CYADDE only **\n")
	OR1K_DELAYED_NOP(l.jal	_puts)

        /* Set OVE again */
	l.mfspr	r3,r0,SPR_SR
	LOAD_CONST (r2, SPR_SR_OVE)	/* Set OVE */
	l.or	r3,r3,r2
	l.mtspr	r0,r3,SPR_SR
	
        /* set AECR to CYADDE */
        LOAD_CONST(r2, SPR_AECR_CYADDE)
        l.mtspr r0,r2,SPR_AECR

	/* Check that an overflow alone does not cause a RANGE Exception. */
	TEST_SUB (0, SPR_SR_CY | SPR_SR_OV,
	          0x80000000, 0x7fffffff, 0x00000001,
	          FALSE, TRUE, 0)

	/* Check that a carry alone causes a RANGE Exception. */
	TEST_SUB (0, SPR_SR_CY | SPR_SR_OV,
	          0x3fffffff, 0x40000000, 0xffffffff,
	          TRUE, FALSE, SPR_AESR_CYADDE)

	/* Check that carry and overflow together cause an exception. */
	TEST_SUB (0, SPR_SR_CY | SPR_SR_OV,
	          0x7fffffff, 0x80000000, 0xffffffff,
	          TRUE, TRUE, SPR_AESR_CYADDE)
        
        /* Clear OVE so following printout won't raise an exception */
        l.mfspr r3,r0,SPR_SR
        LOAD_CONST (r2, ~SPR_SR_OVE)
        l.and r3,r2,r3
        l.mtspr r0,r3,SPR_SR
        
	LOAD_STR (r3, "  ** AECR set to OVADDE | CYADDE **\n")
	OR1K_DELAYED_NOP(l.jal	_puts)
        
        /* Set OVE again */
	l.mfspr	r3,r0,SPR_SR
	LOAD_CONST (r2, SPR_SR_OVE)	/* Set OVE */
	l.or	r3,r3,r2
	l.mtspr	r0,r3,SPR_SR
	
        /* set AECR to OVADDE */
        LOAD_CONST(r2, SPR_AECR_OVADDE | SPR_AECR_CYADDE)
        l.mtspr r0,r2,SPR_AECR

	/* Check that an overflow alone causes a RANGE Exception. */
	TEST_SUB (0, SPR_SR_CY | SPR_SR_OV,
	          0x80000000, 0x7fffffff, 0x00000001,
	          FALSE, TRUE, SPR_AECR_OVADDE)

	/* Check that a carry alone causes a RANGE Exception. */
	TEST_SUB (0, SPR_SR_CY | SPR_SR_OV,
	          0x3fffffff, 0x40000000, 0xffffffff,
	          TRUE, FALSE, SPR_AESR_CYADDE)

	/* Check that carry and overflow together cause an exception. */
	TEST_SUB (0, SPR_SR_CY | SPR_SR_OV,
	          0x7fffffff, 0x80000000, 0xffffffff,
	          TRUE, TRUE, SPR_AESR_OVADDE | SPR_AESR_CYADDE)

_sub_except_done:
	/* Finished checking range exceptions */
	l.mfspr	r3,r0,SPR_SR
	LOAD_CONST (r2, ~SPR_SR_OVE)	/* Clear OVE */
	l.and	r3,r3,r2
	l.mtspr	r0,r3,SPR_SR
	
	LOAD_STR (r3, "  ** OVE flag cleared **\n")
	OR1K_DELAYED_NOP(l.jal	_puts)

/* ----------------------------------------------------------------------------
 * All done
 * ------------------------------------------------------------------------- */
_exit:
	LOAD_STR (r3, "Test completed\n")
	l.jal	_puts
	l.nop

	TEST_EXIT