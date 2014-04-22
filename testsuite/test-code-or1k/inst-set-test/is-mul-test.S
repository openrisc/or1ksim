/* is-mul-test.S. l.mul, l.muli and l.mulu instruction test of Or1ksim
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
 * The l.mul, l.muli and l.mulu instructions should all be present and set the
 * carry and overflow flags.
 *
 * Problems in this area were reported in Bugs 1774, 1782, 1783 and 1784.
 * Having fixed the problem, this is (in good software engineering style), a
 * regression test to go with the fix.
 *
 * This is not a comprehensive test of any instruction (yet).
 *
 * Of course what is really needed is a comprehensive instruction test...
 * ------------------------------------------------------------------------- */


#include "inst-set-test.h"

/* ----------------------------------------------------------------------------
 * A macro to carry out a test of multiply signed
 *
 * Arguments
 *   op1:       First operand value
 *   op2:       Second operand value
 *   res:       Expected result
 *   ov:        Expected overflow flag
 * ------------------------------------------------------------------------- */
#define TEST_MUL(op1, op2, res, ov)				 	\
	l.mfspr	r3,r0,SPR_SR						;\
	LOAD_CONST (r2, ~SPR_SR_OV)					;\
	l.and	r3,r3,r2		/* Clear flags */		;\
	l.mtspr	r0,r3,SPR_SR						;\
									;\
	LOAD_CONST (r5,op1)		/* Load numbers to multiply */	;\
	LOAD_CONST (r6,op2)						;\
	l.mtspr	r0,r0,SPR_EPCR_BASE	/* Clear record */		;\
        l.mtspr	r0,r0,SPR_AESR						;\
50:	l.mul	r4,r5,r6						;\
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
	PUTS (" * 0x")							;\
	PUTH (op2)							;\
	PUTS (" = 0x")							;\
	PUTH (res)							;\
	PUTS (": ")							;\
	POP (r4)							;\
	CHECK_RES1 (r4, res)						;\
									;\
	POP (r2)			/* Retrieve SR */		;\
	LOAD_CONST (r4, SPR_SR_OV)	/* The overflow bit */		;\
	l.and	r2,r2,r4						;\
	l.sfeq	r2,r4							;\
	CHECK_FLAG ("- overflow flag set:   ", ov)			;\
									;\
	POP (r2)			/* Retrieve EPCR */		;\
	LOAD_CONST (r4, 50b)		/* The opcode of interest */	;\
	l.and	r2,r2,r4						;\
	l.sfeq	r2,r4							;\
	l.bnf	51f							;\
									;\
	PUTS ("  - exception triggered: TRUE\n")			;\
									;\
	/* Check if AECSRP is set */					;\
	l.mfspr	r2,r0,SPR_CPUCFGR	/* Retrieve CPUCFGR */		;\
	LOAD_CONST (r4, SPR_CPUCFGR_AECSRP)				;\
	l.and	r2,r2,r4						;\
	l.sfeq	r2,r4							;\
	/* If AECSRP is not set skip to where AESR is popped at end */	;\
	OR1K_DELAYED_NOP(l.bnf	52f)					;\
									;\
	/* if l.mul ever sets an exception in, OVMULE will be the */	;\
	/* only bit set */						;\
        PUTS ("  - AESR ")						;\
        PUTH (SPR_AESR_OVMULE)						;\
        PUTS (":       ")						;\
	POP (r2)			/* Retrieve AESR */		;\
        CHECK_RES1 (r2, SPR_AESR_OVMULE)				;\
									;\
	OR1K_DELAYED_NOP(l.j	53f)					;\
									;\
51:	PUTS ("  - exception triggered: FALSE\n")			;\
	/* If exception wasn't triggered we still need to pop AESR */	;\
52:	POP (r2)			/* Retrieve AESR */		;\
53:     /* Restore SR (we cleared OVE) */				;\
	POP (r2)			/* Retrive SR */		;\
        l.mtspr	r0,r2,SPR_SR

	
	
/* ----------------------------------------------------------------------------
 * A macro to carry out a test of multiply unsigned
 *
 * Arguments
 *   op1:       First operand value
 *   op2:       Second operand value
 *   res:       Expected result
 *   cy:        Expected carry flag
 * ------------------------------------------------------------------------- */
#define TEST_MULU(op1, op2, res, cy)				 	\
	l.mfspr	r3,r0,SPR_SR						;\
	LOAD_CONST (r2, ~SPR_SR_CY)					;\
	l.and	r3,r3,r2		/* Clear flags */		;\
	l.mtspr	r0,r3,SPR_SR						;\
									;\
	LOAD_CONST (r5,op1)		/* Load numbers to multiply */	;\
	LOAD_CONST (r6,op2)						;\
	l.mtspr	r0,r0,SPR_EPCR_BASE	/* Clear record */		;\
        l.mtspr	r0,r0,SPR_AESR						;\
50:	l.mulu	r4,r5,r6						;\
	l.mfspr	r2,r0,SPR_SR		/* So we can examine flags */	;\
	l.mfspr	r5,r0,SPR_EPCR_BASE	/* What triggered exception */	;\
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
	PUSH (r2)			/* Save SR for later */		;\
	PUSH (r4)			/* Save result for later */	;\
									;\
	PUTS ("  0x")							;\
	PUTH (op1)							;\
	PUTS (" * 0x")							;\
	PUTH (op2)							;\
	PUTS (" = 0x")							;\
	PUTH (res)							;\
	PUTS (": ")							;\
	POP (r4)							;\
	CHECK_RES1 (r4, res)						;\
									;\
	POP (r2)			/* Retrieve SR */		;\
	LOAD_CONST (r4, SPR_SR_CY)	/* The carry bit */		;\
	l.and	r2,r2,r4						;\
	l.sfeq	r2,r4							;\
	CHECK_FLAG ("- carry flag set:      ", cy)			;\
									;\
	POP (r2)			/* Retrieve EPCR */		;\
	LOAD_CONST (r4, 50b)		/* The opcode of interest */	;\
	l.and	r2,r2,r4						;\
	l.sfeq	r2,r4							;\
	l.bnf	51f							;\
									;\
	PUTS ("  - exception triggered: TRUE\n")			;\
									;\
	/* Check if AECSRP is set */					;\
	l.mfspr	r2,r0,SPR_CPUCFGR	/* Retrieve CPUCFGR */		;\
	LOAD_CONST (r4, SPR_CPUCFGR_AECSRP)				;\
	l.and	r2,r2,r4						;\
	l.sfeq	r2,r4							;\
	/* If AECSRP is not set skip to where AESR is popped at end */	;\
	OR1K_DELAYED_NOP(l.bnf	52f)					;\
									;\
	/* if l.mulu ever sets an exception in, CYMULE will be the */	;\
	/* only bit set */						;\
        PUTS ("  - AESR ")						;\
        PUTH (SPR_AESR_CYMULE)						;\
        PUTS (":       ")						;\
	POP (r2)			/* Retrieve AESR */		;\
        CHECK_RES1 (r2, SPR_AESR_CYMULE)				;\
									;\
	OR1K_DELAYED_NOP(l.j	53f)					;\
									;\
51:	PUTS ("  - exception triggered: FALSE\n")			;\
	/* If exception wasn't triggered we still need to pop AESR */	;\
52:	POP (r2)			/* Retrieve AESR */		;\
53:     /* Restore SR (we cleared OVE) */				;\
	POP (r2)			/* Retrive SR */		;\
        l.mtspr	r0,r2,SPR_SR

	
	
/* ----------------------------------------------------------------------------
 * A macro to carry out a test of multiply immediate
 *
 * Arguments
 *   op1:       First operand value
 *   op2:       Second operand value
 *   res:       Expected result
 *   cy:        Expected carry flag
 *   ov:        Expected overflow flag
 * ------------------------------------------------------------------------- */
#define TEST_MULI(op1, op2, res, ov)				 	\
	l.mfspr	r3,r0,SPR_SR						;\
	LOAD_CONST (r2, ~SPR_SR_OV)					;\
	l.and	r3,r3,r2		/* Clear flags */		;\
	l.mtspr	r0,r3,SPR_SR						;\
									;\
	LOAD_CONST (r5,op1)		/* Load numbers to multiply */	;\
	l.mtspr	r0,r0,SPR_EPCR_BASE	/* Clear record */		;\
        l.mtspr	r0,r0,SPR_AESR						;\
53:	l.muli	r4,r5,op2						;\
	l.mfspr	r2,r0,SPR_SR		/* So we can examine flags */	;\
	l.mfspr	r5,r0,SPR_EPCR_BASE	/* What triggered exception */	;\
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
	PUSH (r2)			/* Save SR for later */		;\
	PUSH (r4)			/* Save result for later */	;\
									;\
	PUTS ("  0x")							;\
	PUTH (op1)							;\
	PUTS (" * 0x")							;\
	PUTHH (op2)							;\
	PUTS (" = 0x")							;\
	PUTH (res)							;\
	PUTS (": ")							;\
	POP (r4)							;\
	CHECK_RES1 (r4, res)						;\
									;\
	POP(r2)				/* Retrieve SR */		;\
	LOAD_CONST (r4, SPR_SR_OV)	/* The overflow bit */		;\
	l.and	r2,r2,r4						;\
	l.sfeq	r2,r4							;\
	CHECK_FLAG ("- overflow flag set:   ", ov)			;\
									;\
	POP (r2)			/* Retrieve EPCR */		;\
	LOAD_CONST (r4, 53b)		/* The opcode of interest */	;\
	l.and	r2,r2,r4						;\
	l.sfeq	r2,r4							;\
	l.bnf	54f							;\
									;\
	PUTS ("  - exception triggered: TRUE\n")			;\
									;\
	/* Check if AECSRP is set */					;\
	l.mfspr	r2,r0,SPR_CPUCFGR	/* Retrieve CPUCFGR */		;\
	LOAD_CONST (r4, SPR_CPUCFGR_AECSRP)				;\
	l.and	r2,r2,r4						;\
	l.sfeq	r2,r4							;\
	/* If AECSRP is not set skip to where AESR is popped at end */	;\
	OR1K_DELAYED_NOP(l.bnf	55f)					;\
									;\
        PUTS ("  - AESR ")						;\
        PUTH (SPR_AESR_OVMULE)						;\
        PUTS (":       ")						;\
	POP (r2)			/* Retrieve AESR */		;\
        CHECK_RES1 (r2, SPR_AESR_OVMULE)				;\
									;\
	OR1K_DELAYED_NOP(l.j	56f)					;\
									;\
54:	PUTS ("  - exception triggered: FALSE\n")			;\
	/* If exception wasn't triggered we still need to pop AESR */	;\
55:	POP (r2)			/* Retrieve AESR */		;\
56:	/* Restore SR (we cleared OVE) */				;\
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
 * Test of multiply signed, l.mul
 * ------------------------------------------------------------------------- */
_mul:
	LOAD_STR (r3, "l.mul\n")
	l.jal	_puts
	l.nop

	/* Multiply two small positive numbers. Should set no flags. */
	TEST_MUL (0x00000002, 0x00000003,
	          0x00000006, FALSE)

	/* Multiply two quite large positive numbers. Should set no flags */
	TEST_MUL (0x00008001, 0x0000fffe,
	          0x7ffffffe, FALSE)

	/* Multiply two slightly too large positive numbers. Should set the
	   overflow flag */
	TEST_MUL (0x00008000, 0x00010000,
	          0x80000000, TRUE)

	/* Multiply two large positive numbers. Should set the overflow flag */
	TEST_MUL (0x00010000, 0x00010000, 0x00000000, TRUE)

	/* Multiply two small negative numbers. Should not set the overflow
           flag */
	TEST_MUL (0xfffffffe, 0xfffffffd,
	          0x00000006, FALSE)

	/* Multiply two quite large negative numbers. Should not set the
           overflow flag */
	TEST_MUL (0xffff7fff, 0xffff0002,
	          0x7ffffffe, FALSE)

	/* Multiply two slightly too large negative numbers. Should set the
	   overflow flag */
	TEST_MUL (0xffff7fff, 0xffff0000,
	          0x80010000, TRUE)

	/* Multiply two large negative numbers. Should set the
	   overflow flag even though the result is a positive number. */
	TEST_MUL (0xffff0000, 0xfffeffff,
	          0x00010000, TRUE)

	/* Multiply one small negative number and one small positive number.
	   Should not set the overflow flag. */
	TEST_MUL (0x00000002, 0xfffffffd,
	          0xfffffffa, FALSE)

	/* Multiply one quite large negative number and one quite large
	   positive number. Should not set the overflow flag. */
	TEST_MUL (0xffff8000, 0x00010000,
	          0x80000000, FALSE)

	/* Multiply one slightly too large negative number and one slightly
	   too large positive number. Should set the overflow flag. */
	TEST_MUL (0xffff7fff, 0x00010000,
	          0x7fff0000, TRUE)

	/* Multiply the largest negative number by positive unity. Should not
	   set the overflow flag. */
	TEST_MUL (0x80000000, 0x00000001,
	          0x80000000, FALSE)

        /* check if AECSRP is set */
        l.mfspr r3,r0,SPR_CPUCFGR
        LOAD_CONST(r2, SPR_CPUCFGR_AECSRP)
        l.and r3,r3,r2
        l.sfeq r3,r2
        OR1K_DELAYED_NOP(l.bf _mul_aecsr)

_mul_no_aecsr:
	LOAD_STR (r3, "  ** OVE flag set **\n")
	OR1K_DELAYED_NOP(l.jal	_puts)

	/* Check that range exceptions are triggered */
	l.mfspr	r3,r0,SPR_SR
	LOAD_CONST (r2, SPR_SR_OVE)	/* Set OVE */
	l.or	r3,r3,r2
	l.mtspr	r0,r3,SPR_SR
	
	/* Check that an overflow alone causes a RANGE Exception. */
	TEST_MUL (0x00008000, 0x00010000,
	          0x80000000, TRUE)

	/* Check that a numbers that would cause unsigned overflow (but not
           signed overflow) do not cause a RANGE Exception. */
	TEST_MUL (0x00000002, 0xfffffffd,
	          0xfffffffa, FALSE)

	/* Check that numbers that cause both signed and unsigned overflow
           together cause an exception. */
	TEST_MUL (0xffff7fff, 0xffff0000,
	          0x80010000, TRUE)

        OR1K_DELAYED_NOP(l.j _mul_except_done)

_mul_aecsr:
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
	TEST_MUL (0x00008000, 0x00010000,
	          0x80000000, TRUE)

	/* Check that a numbers that would cause unsigned overflow (but not
           signed overflow) do not cause a RANGE Exception. */
	TEST_MUL (0x00000002, 0xfffffffd,
	          0xfffffffa, FALSE)

	/* Check that numbers that cause both signed and unsigned overflow
           together does not cause an exception. */
	TEST_MUL (0xffff7fff, 0xffff0000,
	          0x80010000, TRUE)

        /* Clear OVE so following printout won't raise an exception */
        l.mfspr r3,r0,SPR_SR
        LOAD_CONST (r2, ~SPR_SR_OVE)
        l.and r3,r2,r3
        l.mtspr r0,r3,SPR_SR
        
	LOAD_STR (r3, "  ** AECR set to OVMULE **\n")
	OR1K_DELAYED_NOP(l.jal	_puts)
        
        /* Set OVE again */
	l.mfspr	r3,r0,SPR_SR
	LOAD_CONST (r2, SPR_SR_OVE)	/* Set OVE */
	l.or	r3,r3,r2
	l.mtspr	r0,r3,SPR_SR
	
        /* set AECR to OVMULE */
        LOAD_CONST(r2, SPR_AECR_OVMULE)
        l.mtspr r0,r2,SPR_AECR

	/* Check that an overflow alone causes a RANGE Exception. */
	TEST_MUL (0x00008000, 0x00010000,
	          0x80000000, TRUE)

	/* Check that a numbers that would cause unsigned overflow (but not
           signed overflow) do not cause a RANGE Exception. */
	TEST_MUL (0x00000002, 0xfffffffd,
	          0xfffffffa, FALSE)

	/* Check that numbers that cause both signed and unsigned overflow
           together causes an exception. */
	TEST_MUL (0xffff7fff, 0xffff0000,
	          0x80010000, TRUE)

        /* Clear OVE so following printout won't raise an exception */
        l.mfspr r3,r0,SPR_SR
        LOAD_CONST (r2, ~SPR_SR_OVE)
        l.and r3,r2,r3
        l.mtspr r0,r3,SPR_SR
        
	LOAD_STR (r3, "  ** AECR set to CYMULE **\n")
	OR1K_DELAYED_NOP(l.jal	_puts)
        
        /* Set OVE again */
	l.mfspr	r3,r0,SPR_SR
	LOAD_CONST (r2, SPR_SR_OVE)	/* Set OVE */
	l.or	r3,r3,r2
	l.mtspr	r0,r3,SPR_SR
	
        /* set AECR to CYMULE */
        LOAD_CONST(r2, SPR_AECR_CYMULE)
        l.mtspr r0,r2,SPR_AECR

	/* Check that an overflow alone does not cause a RANGE Exception. */
	TEST_MUL (0x00008000, 0x00010000,
	          0x80000000, TRUE)

	/* Check that a numbers that would cause unsigned overflow (but not
           signed overflow) do not cause a RANGE Exception. */
	TEST_MUL (0x00000002, 0xfffffffd,
	          0xfffffffa, FALSE)

	/* Check that numbers that cause both signed and unsigned overflow
           together does not cause an exception. */
	TEST_MUL (0xffff7fff, 0xffff0000,
	          0x80010000, TRUE)

_mul_except_done:
	/* Finished checking range exceptions */
	l.mfspr	r3,r0,SPR_SR
	LOAD_CONST (r2, ~SPR_SR_OVE)	/* Clear OVE */
	l.and	r3,r3,r2
	l.mtspr	r0,r3,SPR_SR
	
	LOAD_STR (r3, "  ** OVE flag cleared **\n")
	l.jal	_puts
	l.nop

/* ----------------------------------------------------------------------------
 * Test of multiply signed, l.muli
 * ------------------------------------------------------------------------- */
_muli:
	LOAD_STR (r3, "l.muli\n")
	l.jal	_puts
	l.nop

	/* Multiply two small positive numbers. Should set no flags. */
	TEST_MULI (0x00000002, 0x0003,
	          0x00000006, FALSE)

	/* Multiply two quite large positive numbers. Should set no flags */
	TEST_MULI (0x00010002, 0x7fff,
	          0x7ffffffe, FALSE)

	/* Multiply two slightly too large positive numbers. Should set the
	   overflow flag */
	TEST_MULI (0x00020000, 0x4000,
	          0x80000000, TRUE)

	/* Multiply two large positive numbers. Should set the overflow flag. */
	TEST_MULI (0x00040000, 0x4000,
	           0x00000000, TRUE)

	/* Multiply two small negative numbers. Should set the overflow flag. */
	TEST_MULI (0xfffffffe, 0xfffd,
	          0x00000006, FALSE)

	/* Multiply two quite large negative numbers. Should not set the overflow flag. */
	TEST_MULI (0xfffefffe, 0x8001,
	          0x7ffffffe, FALSE)

	/* Multiply two slightly too large negative numbers. Should set the overflow flag. */
	TEST_MULI (0xfffe0000, 0xbfff,
	          0x80020000, TRUE)

	/* Multiply two large negative numbers. Should set the overflow flag
           (even though the result is a positive number). */
	TEST_MULI (0xfffdfffe, 0x8000,
	          0x00010000, TRUE)

	/* Multiply one small negative number and one small positive number.
	   Should not set the overflow flag. */
	TEST_MULI (0x00000002, 0xfffd,
	          0xfffffffa, FALSE)

	/* Multiply one quite large negative number and one quite large
	   positive number. Should not set the overflow flag. */
	TEST_MULI (0x00010000, 0x8000,
	          0x80000000, FALSE)

	/* Multiply one slightly too large negative number and one slightly
	   too large positive number. Should set the overflow flag. */
	TEST_MULI (0xfffdfffc, 0x4000,
	          0x7fff0000, TRUE)

	/* Multiply the largest negative number by positive unity. Should not
	   set the overflow flag. */
	TEST_MULI (0x80000000, 0x0001,
	          0x80000000, FALSE)

        /* check if AECSRP is set */
        l.mfspr r3,r0,SPR_CPUCFGR
        LOAD_CONST(r2, SPR_CPUCFGR_AECSRP)
        l.and r3,r3,r2
        l.sfeq r3,r2
        OR1K_DELAYED_NOP(l.bf _muli_aecsr)

_muli_no_aecsr:
	LOAD_STR (r3, "  ** OVE flag set **\n")
	OR1K_DELAYED_NOP(l.jal	_puts)

	/* Check that range exceptions are triggered */
	l.mfspr	r3,r0,SPR_SR
	LOAD_CONST (r2, SPR_SR_OVE)	/* Set OVE */
	l.or	r3,r3,r2
	l.mtspr	r0,r3,SPR_SR
	
	/* Check that an overflow alone causes a RANGE Exception. */
	TEST_MULI (0x00020000, 0x4000,
	          0x80000000, TRUE)

	/* Check that a multiply that would cause unsigned overflow but not
           signed overflow does not cause a RANGE Exception. */
	TEST_MULI (0xfffffffe, 0xfffd,
	          0x00000006, FALSE)

	/* Check that a multiply that would cause both signed and
	   unsigned overflow together causes an exception. */
	TEST_MULI (0xfffdfffe, 0x8000,
	          0x00010000, TRUE)

        OR1K_DELAYED_NOP(l.j _muli_except_done)

_muli_aecsr:
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
	TEST_MULI (0x00020000, 0x4000,
	          0x80000000, TRUE)

	/* Check that a multiply that would cause unsigned overflow but not
           signed overflow does not cause a RANGE Exception. */
	TEST_MULI (0xfffffffe, 0xfffd,
	          0x00000006, FALSE)

	/* Check that a multiply that would cause both signed and
	   unsigned overflow together does not cause an exception. */
	TEST_MULI (0xfffdfffe, 0x8000,
	          0x00010000, TRUE)

        /* Clear OVE so following printout won't raise an exception */
        l.mfspr r3,r0,SPR_SR
        LOAD_CONST (r2, ~SPR_SR_OVE)
        l.and r3,r2,r3
        l.mtspr r0,r3,SPR_SR
        
	LOAD_STR (r3, "  ** AECR set to OVMULE **\n")
	OR1K_DELAYED_NOP(l.jal	_puts)
        
        /* Set OVE again */
	l.mfspr	r3,r0,SPR_SR
	LOAD_CONST (r2, SPR_SR_OVE)	/* Set OVE */
	l.or	r3,r3,r2
	l.mtspr	r0,r3,SPR_SR
	
        /* set AECR to OVMULE */
        LOAD_CONST(r2, SPR_AECR_OVMULE)
        l.mtspr r0,r2,SPR_AECR

	/* Check that an overflow alone causes a RANGE Exception. */
	TEST_MULI (0x00020000, 0x4000,
	          0x80000000, TRUE)

	/* Check that a multiply that would cause unsigned overflow but not
           signed overflow does not cause a RANGE Exception. */
	TEST_MULI (0xfffffffe, 0xfffd,
	          0x00000006, FALSE)

	/* Check that a multiply that would cause both signed and
	   unsigned overflow together cause an exception. */
	TEST_MULI (0xfffdfffe, 0x8000,
	          0x00010000, TRUE)

        /* Clear OVE so following printout won't raise an exception */
        l.mfspr r3,r0,SPR_SR
        LOAD_CONST (r2, ~SPR_SR_OVE)
        l.and r3,r2,r3
        l.mtspr r0,r3,SPR_SR
        
	LOAD_STR (r3, "  ** AECR set to CYMULE **\n")
	OR1K_DELAYED_NOP(l.jal	_puts)
        
        /* Set OVE again */
	l.mfspr	r3,r0,SPR_SR
	LOAD_CONST (r2, SPR_SR_OVE)	/* Set OVE */
	l.or	r3,r3,r2
	l.mtspr	r0,r3,SPR_SR
	
        /* set AECR to OVMULE */
        LOAD_CONST(r2, SPR_AECR_CYMULE)
        l.mtspr r0,r2,SPR_AECR

	/* Check that an overflow alone does not cause a RANGE Exception. */
	TEST_MULI (0x00020000, 0x4000,
	          0x80000000, TRUE)

	/* Check that a multiply that would cause unsigned overflow but not
           signed overflow does not cause a RANGE Exception. */
	TEST_MULI (0xfffffffe, 0xfffd,
	          0x00000006, FALSE)

	/* Check that a multiply that would cause both signed and
	   unsigned overflow together does not cause an exception. */
	TEST_MULI (0xfffdfffe, 0x8000,
	          0x00010000, TRUE)

_muli_except_done:
	/* Finished checking range exceptions */
	l.mfspr	r3,r0,SPR_SR
	LOAD_CONST (r2, ~SPR_SR_OVE)	/* Clear OVE */
	l.and	r3,r3,r2
	l.mtspr	r0,r3,SPR_SR
	
	LOAD_STR (r3, "  ** OVE flag cleared **\n")
	l.jal	_puts
	l.nop

/* ----------------------------------------------------------------------------
 * Test of multiply unsigned, l.mulu
 * ------------------------------------------------------------------------- */
_mulu:
	LOAD_STR (r3, "l.mulu\n")
	l.jal	_puts
	l.nop

	/* Multiply two small positive numbers. Should set no flags. */
	TEST_MULU (0x00000002, 0x00000003,
	           0x00000006, FALSE)

	/* Multiply two quite large positive numbers. Should set no flags */
	TEST_MULU (0x00008001, 0x0000fffe,
	           0x7ffffffe, FALSE)

	/* Multiply two slightly too large positive numbers. Should not set the
	   carry flag */
	TEST_MULU (0x00008000, 0x00010000,
	           0x80000000, FALSE)

	/* Multiply two large positive numbers. Should the carry flag
	   (even though the result is not a negative number). */
	TEST_MULU (0x00010000, 0x00010000,
		   0x00000000, TRUE)

	/* Multiply two small "negative" numbers. Should set the carry flag. */
	TEST_MULU (0xfffffffe, 0xfffffffd,
	           0x00000006, TRUE)

	/* Multiply two quite large negative numbers. Should set the carry flag. */
	TEST_MULU (0xffff7fff, 0xffff0002,
	           0x7ffffffe, TRUE)

	/* Multiply two slightly too large "negative" numbers. Should set the carry flag. */
	TEST_MULU (0xffff7fff, 0xffff0000,
	           0x80010000, TRUE)

	/* Multiply two large negative numbers. Should set the carry flag. */
	TEST_MULU (0xffff0000, 0xfffeffff,
	           0x00010000, TRUE)

	/* Multiply one small "negative" number and one small positive number.
	   Should set the carry flag. */
	TEST_MULU (0x00000002, 0xfffffffd,
	           0xfffffffa, TRUE)

	/* Multiply one quite large "negative" number and one quite large
	   positive number. Should set the carry flag. */
	TEST_MULU (0xffff8000, 0x00010000,
	           0x80000000, TRUE)

	/* Multiply one slightly too large "negative" number and one slightly
	   too large positive number. Should set the carry flag. */
	TEST_MULU (0xffff7fff, 0x00010000,
	           0x7fff0000, TRUE)

	/* Multiply the largest "negative" number by positive unity. Should not set
	   the carry flag. */
	TEST_MULU (0x80000000, 0x00000001,
	           0x80000000, FALSE)

        /* check if AECSRP is set */
        l.mfspr r3,r0,SPR_CPUCFGR
        LOAD_CONST(r2, SPR_CPUCFGR_AECSRP)
        l.and r3,r3,r2
        l.sfeq r3,r2
        OR1K_DELAYED_NOP(l.bf _mulu_aecsr)

_mulu_no_aecsr:
	LOAD_STR (r3, "  ** OVE flag set **\n")
	OR1K_DELAYED_NOP(l.jal	_puts)

	/* Check that range exceptions are triggered */
	l.mfspr	r3,r0,SPR_SR
	LOAD_CONST (r2, SPR_SR_OVE)	/* Set OVE */
	l.or	r3,r3,r2
	l.mtspr	r0,r3,SPR_SR

	/* Check that what would cause a signed overflow does
	   not cause a RANGE Exception. */
	TEST_MULU (0x00008000, 0x00010000,
	           0x80000000, FALSE)

	/* Check that unsigned overflow causes a RANGE Exception. */
	TEST_MULU (0x00000002, 0xfffffffd,
	           0xfffffffa, TRUE)

	/* Check that what would cause a both signed and unsigned
	   overflow causes a RANGE Exception. */
	TEST_MULU (0xffff7fff, 0xffff0000,
	           0x80010000, TRUE)

        OR1K_DELAYED_NOP(l.j _mulu_except_done)

_mulu_aecsr:
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

	/* Check that what would cause a signed overflow does
	   not cause a RANGE Exception. */
	TEST_MULU (0x00008000, 0x00010000,
	           0x80000000, FALSE)

	/* Check that unsigned overflow does not cause a RANGE Exception. */
	TEST_MULU (0x00000002, 0xfffffffd,
	           0xfffffffa, TRUE)

	/* Check that what would cause a both signed and unsigned
	   overflow does not cause a RANGE Exception. */
	TEST_MULU (0xffff7fff, 0xffff0000,
	           0x80010000, TRUE)

        /* Clear OVE so following printout won't raise an exception */
        l.mfspr r3,r0,SPR_SR
        LOAD_CONST (r2, ~SPR_SR_OVE)
        l.and r3,r2,r3
        l.mtspr r0,r3,SPR_SR
        
	LOAD_STR (r3, "  ** AECR set to OVMULE **\n")
	OR1K_DELAYED_NOP(l.jal	_puts)
        
        /* Set OVE again */
	l.mfspr	r3,r0,SPR_SR
	LOAD_CONST (r2, SPR_SR_OVE)	/* Set OVE */
	l.or	r3,r3,r2
	l.mtspr	r0,r3,SPR_SR
	
        /* set AECR to OVMULE */
        LOAD_CONST(r2, SPR_AECR_OVMULE)
        l.mtspr r0,r2,SPR_AECR

	/* Check that what would cause a signed overflow does
	   not cause a RANGE Exception. */
	TEST_MULU (0x00008000, 0x00010000,
	           0x80000000, FALSE)

	/* Check that unsigned overflow does not cause a RANGE Exception. */
	TEST_MULU (0x00000002, 0xfffffffd,
	           0xfffffffa, TRUE)

	/* Check that what would cause a both signed and unsigned
	   overflow does not cause a RANGE Exception. */
	TEST_MULU (0xffff7fff, 0xffff0000,
	           0x80010000, TRUE)
        
        /* Clear OVE so following printout won't raise an exception */
        l.mfspr r3,r0,SPR_SR
        LOAD_CONST (r2, ~SPR_SR_OVE)
        l.and r3,r2,r3
        l.mtspr r0,r3,SPR_SR
        
	LOAD_STR (r3, "  ** AECR set to CYMULE **\n")
	OR1K_DELAYED_NOP(l.jal	_puts)
        
        /* Set OVE again */
	l.mfspr	r3,r0,SPR_SR
	LOAD_CONST (r2, SPR_SR_OVE)	/* Set OVE */
	l.or	r3,r3,r2
	l.mtspr	r0,r3,SPR_SR
	
        /* set AECR to CYMULE */
        LOAD_CONST(r2, SPR_AECR_CYMULE)
        l.mtspr r0,r2,SPR_AECR

	/* Check that what would cause a signed overflow does
	   not cause a RANGE Exception. */
	TEST_MULU (0x00008000, 0x00010000,
	           0x80000000, FALSE)

	/* Check that unsigned overflow causes a RANGE Exception. */
	TEST_MULU (0x00000002, 0xfffffffd,
	           0xfffffffa, TRUE)

	/* Check that what would cause a both signed and unsigned
	   overflow causes a RANGE Exception. */
	TEST_MULU (0xffff7fff, 0xffff0000,
	           0x80010000, TRUE)
        
_mulu_except_done:
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
