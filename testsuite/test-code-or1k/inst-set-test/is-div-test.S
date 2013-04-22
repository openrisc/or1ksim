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
 * A macro to carry out a test of divide signed or unsigned
 *
 * Arguments
 *   opc:       The opcode
 *   op1:       First operand value
 *   op2:       Second operand value
 *   res:       Expected result
 *   cy:        Expected carry flag
 *   ov:        Expected overflow flag
 * ------------------------------------------------------------------------- */
#define TEST_DIV(opc, op1, op2, res, ov)				 \
	l.mfspr	r3,r0,SPR_SR						;\
	LOAD_CONST (r2, ~SPR_SR_OV)					;\
	l.and	r3,r3,r2		/* Clear flags */		;\
	l.mtspr	r0,r3,SPR_SR						;\
									;\
	l.or	r4,r0,r0		/* Clear result reg */		;\
	LOAD_CONST (r5,op1)		/* Load numbers to add */	;\
	LOAD_CONST (r6,op2)						;\
	l.mtspr	r0,r0,SPR_EPCR_BASE	/* Clear record */		;\
        l.mtspr	r0,r0,SPR_AESR						;\
50:	opc	r4,r5,r6						;\
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
	PUTS (" / 0x")							;\
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
	OR1K_DELAYED_NOP(l.bnf	51f)					;\
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
	/* if divide ever sets an exception in, DBZE will be the */	;\
	/* only bit set */						;\
        PUTS ("  - AESR ")						;\
        PUTH (SPR_AESR_DBZE)						;\
        PUTS (":       ")						;\
	POP (r2)			/* Retrieve AESR */		;\
        CHECK_RES1 (r2, SPR_AESR_DBZE)					;\
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
 * Test of divide signed, l.div
 * ------------------------------------------------------------------------- */
_div:	
	LOAD_STR (r3, "l.div\n")
	l.jal	_puts
	l.nop

	/* Divide two positive numbers and check rounding. Should set no
	   flags. */
	TEST_DIV (l.div, 0x0000000c, 0x00000003,
	          0x00000004, FALSE)

	TEST_DIV (l.div, 0x0000000b, 0x00000003,
	          0x00000003, FALSE)

	/* Divide two negative numbers and check rounding. Should set no
	   flags. */
	TEST_DIV (l.div, 0xfffffff4, 0xfffffffd,
	          0x00000004, FALSE)

	TEST_DIV (l.div, 0xfffffff5, 0xfffffffd,
	          0x00000003, FALSE)

	/* Divide a negative number by a positive number and check
	   rounding. Should set no flags. */
	TEST_DIV (l.div, 0xfffffff4, 0x00000003,
	          0xfffffffc, FALSE)

	TEST_DIV (l.div, 0xfffffff5, 0x00000003,
	          0xfffffffd, FALSE)

	/* Divide a positive number by a negative number and check
	   rounding. Should set no flags. */
	TEST_DIV (l.div, 0x0000000c, 0xfffffffd,
	          0xfffffffc, FALSE)

	TEST_DIV (l.div, 0x0000000b, 0xfffffffd,
	          0xfffffffd, FALSE)

	/* Divide by zero. Should set the overflow flag. */
	TEST_DIV (l.div, 0x0000000c, 0x00000000,
	          0x00000000, TRUE)
	
	TEST_DIV (l.div, 0xfffffff4, 0x00000000,
	          0x00000000, TRUE)

        /* INT32_MIN / -1 should set the overflow flag. */
        TEST_DIV (l.div, 0x80000000, 0xffffffff,
                  0x00000000, TRUE)

        /* check if AECSRP is set */
        l.mfspr r3,r0,SPR_CPUCFGR
        LOAD_CONST(r2, SPR_CPUCFGR_AECSRP)
        l.and r3,r3,r2
        l.sfeq r3,r2
        OR1K_DELAYED_NOP(l.bf _div_aecsr)

_div_no_aecsr:
	LOAD_STR (r3, "  ** OVE flag set **\n")
	OR1K_DELAYED_NOP(l.jal	_puts)

	/* Check that range exceptions are triggered */
	l.mfspr	r3,r0,SPR_SR
	LOAD_CONST (r2, SPR_SR_OVE)	/* Set OVE */
	l.or	r3,r3,r2
	l.mtspr	r0,r3,SPR_SR
	
	/* Divide by zero. Should set the overflow flag and trigger an
	   exception. */
	TEST_DIV (l.div, 0x0000000c, 0x00000000,
	          0x00000000, TRUE)
	
	TEST_DIV (l.div, 0xfffffff4, 0x00000000,
	          0x00000000, TRUE)

        /* INT32_MIN / -1 should set the overflow flag and trigger an exception */
        TEST_DIV (l.div, 0x80000000, 0xffffffff,
                  0x00000000, TRUE)

        OR1K_DELAYED_NOP(l.j _div_except_done)

_div_aecsr:
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

	/* Divide by zero. Should set the overflow flag and not trigger an
	   exception. */
	TEST_DIV (l.div, 0x0000000c, 0x00000000,
	          0x00000000, TRUE)
	
	TEST_DIV (l.div, 0xfffffff4, 0x00000000,
	          0x00000000, TRUE)

        /* INT32_MIN / -1 should set the overflow flag and not trigger
	   an exception */
        TEST_DIV (l.div, 0x80000000, 0xffffffff,
                  0x00000000, TRUE)

        /* Clear OVE so following printout won't raise an exception */
        l.mfspr r3,r0,SPR_SR
        LOAD_CONST (r2, ~SPR_SR_OVE)
        l.and r3,r2,r3
        l.mtspr r0,r3,SPR_SR
        
	LOAD_STR (r3, "  ** AECR set to DBZE **\n")
	OR1K_DELAYED_NOP(l.jal	_puts)
        
	/* Check that range exceptions are triggered */
	l.mfspr	r3,r0,SPR_SR
	LOAD_CONST (r2, SPR_SR_OVE)	/* Set OVE */
	l.or	r3,r3,r2
	l.mtspr	r0,r3,SPR_SR
	
        /* set AECR to DBZE */
        LOAD_CONST (r2, SPR_AECR_DBZE)
        l.mtspr r0,r2,SPR_AECR

	/* Divide by zero. Should set the overflow flag and trigger an
	   exception. */
	TEST_DIV (l.div, 0x0000000c, 0x00000000,
	          0x00000000, TRUE)
	
	TEST_DIV (l.div, 0xfffffff4, 0x00000000,
	          0x00000000, TRUE)

        /* INT32_MIN / -1 should set the overflow flag and trigger an
	   exception */
        TEST_DIV (l.div, 0x80000000, 0xffffffff,
                  0x00000000, TRUE)


_div_except_done:
	/* Finished checking range exceptions */
	l.mfspr	r3,r0,SPR_SR
	LOAD_CONST (r2, ~SPR_SR_OVE)	/* Clear OVE */
	l.and	r3,r3,r2
	l.mtspr	r0,r3,SPR_SR

	LOAD_STR (r3, "  ** OVE flag cleared **\n")
	OR1K_DELAYED_NOP(l.jal	_puts)

/* ----------------------------------------------------------------------------
 * Test of divide unsigned, l.divu
 * ------------------------------------------------------------------------- */
_divu:	
	LOAD_STR (r3, "l.divu\n")
	l.jal	_puts
	l.nop

	/* Divide two positive numbers and check rounding. Should set no
	   flags. */
	TEST_DIV (l.divu, 0x0000000c, 0x00000003,
	          0x00000004, FALSE)

	TEST_DIV (l.divu, 0x0000000b, 0x00000003,
	          0x00000003, FALSE)

	/* Divide two numbers that would be negative under 2's complement and
	   check rounding. Should set no flags. */
	TEST_DIV (l.divu, 0xfffffff4, 0xfffffffd,
	          0x00000000, FALSE)

	TEST_DIV (l.divu, 0xfffffff5, 0xfffffffd,
	          0x00000000, FALSE)

	/* Divide a number that would be negative under 2's complement by a
	   number that would be positive under 2's complement and check
	   rounding. Should set no flags. */
	TEST_DIV (l.divu, 0xfffffff4, 0x00000003,
	          0x55555551, FALSE)

	TEST_DIV (l.divu, 0xfffffff5, 0x00000003,
	          0x55555551, FALSE)

	/* Divide a number that would be positive under 2's complement by a
	   number that would be negative under 2's complement and check
	   rounding. Should set no flags. */
	TEST_DIV (l.divu, 0x0000000c, 0xfffffffd,
	          0x00000000, FALSE)

	TEST_DIV (l.divu, 0x0000000b, 0xfffffffd,
	          0x00000000, FALSE)

	/* Divide by zero. Should set the overflow flag. */
	TEST_DIV (l.divu, 0x0000000c, 0x00000000,
	          0x00000000, TRUE)
	
	TEST_DIV (l.divu, 0xfffffff4, 0x00000000,
	          0x00000000, TRUE)

        /* check if AECSRP is set */
        l.mfspr r3,r0,SPR_CPUCFGR
        LOAD_CONST(r2, SPR_CPUCFGR_AECSRP)
        l.and r3,r3,r2
        l.sfeq r3,r2
        OR1K_DELAYED_NOP(l.bf _divu_aecsr)

_divu_no_aecsr:
	LOAD_STR (r3, "  ** OVE flag set **\n")
	OR1K_DELAYED_NOP(l.jal	_puts)

	/* Check that range exceptions are triggered */
	l.mfspr	r3,r0,SPR_SR
	LOAD_CONST (r2, SPR_SR_OVE)	/* Set OVE */
	l.or	r3,r3,r2
	l.mtspr	r0,r3,SPR_SR
	
	/* Divide by zero. Should set the overflow flag and trigger an
	   exception. */
	TEST_DIV (l.divu, 0x0000000c, 0x00000000,
	          0x00000000, TRUE)
	
	TEST_DIV (l.divu, 0xfffffff4, 0x00000000,
	          0x00000000, TRUE)

        OR1K_DELAYED_NOP(l.j _divu_except_done)

_divu_aecsr:
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

	/* Divide by zero. Should set the overflow flag and not trigger an
	   exception. */
	TEST_DIV (l.divu, 0x0000000c, 0x00000000,
	          0x00000000, TRUE)
	
	TEST_DIV (l.divu, 0xfffffff4, 0x00000000,
	          0x00000000, TRUE)

        /* Clear OVE so following printout won't raise an exception */
        l.mfspr r3,r0,SPR_SR
        LOAD_CONST (r2, ~SPR_SR_OVE)
        l.and r3,r2,r3
        l.mtspr r0,r3,SPR_SR
        
	LOAD_STR (r3, "  ** AECR set to DBZE **\n")
	OR1K_DELAYED_NOP(l.jal	_puts)
        
	/* Check that range exceptions are triggered */
	l.mfspr	r3,r0,SPR_SR
	LOAD_CONST (r2, SPR_SR_OVE)	/* Set OVE */
	l.or	r3,r3,r2
	l.mtspr	r0,r3,SPR_SR
	
        /* set AECR to DBZE */
        LOAD_CONST (r2, SPR_AECR_DBZE)
        l.mtspr r0,r2,SPR_AECR

	/* Divide by zero. Should set the overflow flag and trigger an
	   exception. */
	TEST_DIV (l.divu, 0x0000000c, 0x00000000,
	          0x00000000, TRUE)
	
	TEST_DIV (l.divu, 0xfffffff4, 0x00000000,
	          0x00000000, TRUE)

_divu_except_done:
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
