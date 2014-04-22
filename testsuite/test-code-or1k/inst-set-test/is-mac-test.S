/* is-mac-test.S. l.mac, l.maci, l.macrc and l.msb instruction test of Or1ksim
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
 * The l.mac, l.maci, l.macrc and l.msb instructions perform operations related
 * to combined signed multiply and addition/subtraction.
 *
 * The precise definition of these instructions is in flux. In addition there
 * are known problems with the assembler/disassembler (will not correctly
 * handle l.maci) and with the Verilog RTL implementation (not functional).
 *
 * Problems in this area were reported in Bugs 1773 and 1777. Having fixed the
 * problem, this is (in good software engineering style), a  regression test
 * to go with the fix.
 *
 * This is not a comprehensive test of any instruction (yet).
 *
 * Of course what is really needed is a comprehensive instruction test...
 * ------------------------------------------------------------------------- */


#include "inst-set-test.h"

/* ----------------------------------------------------------------------------
 * A macro to carry out a test of multiply accumulate read and clear
 *
 * Arguments
 *   machi:     Inital value of MACHI
 *   maclo:     Inital value of MACLO
 *   op1:       First operand value
 *   op2:       Second operand value
 *   res:       Expected result
 * ------------------------------------------------------------------------- */
#define TEST_MACRC(machi, maclo, op1, op2, res)				 \
	LOAD_CONST (r2,maclo)						;\
	l.mtspr	r0,r2,SPR_MACLO						;\
	LOAD_CONST (r2,machi)						;\
	l.mtspr	r0,r2,SPR_MACHI						;\
									;\
	LOAD_CONST (r5,op1)		/* Load numbers to add */	;\
	LOAD_CONST (r6,op2)						;\
	l.mac	r5,r6							;\
	l.macrc	r4							;\
	PUSH (r4)			/* Save for later */		;\
	PUTS ("  0x")							;\
	PUTH (machi)							;\
	PUTS (" ")							;\
	PUTH (maclo)							;\
	PUTS (" + 0x")							;\
	PUTH (op1)							;\
	PUTS (" * 0x")							;\
	PUTH (op2)							;\
	PUTS (" = 0x")							;\
	PUTH (res)							;\
	PUTS (": ")							;\
	POP (r4)							;\
	CHECK_RES ("", r4, res)						;\
									;\
	l.mfspr	r5,r0,SPR_MACHI						;\
	l.sfne	r5,r0							;\
	l.bf	50f							;\
									;\
	PUTS ("  - MACHI cleared\n")					;\
									;\
50:	l.mfspr	r6,r0,SPR_MACLO						;\
	l.sfne	r6,r0							;\
	l.bf	51f							;\
									;\
	PUTS ("  - MACLO cleared\n")					;\
51:	
	
/* ----------------------------------------------------------------------------
 * A macro to carry out a test of multiply accumulate in registers
 *
 * Arguments
 *   machi:     Inital value of MACHI
 *   maclo:     Inital value of MACLO
 *   op1:       First operand value
 *   op2:       Second operand value
 *   reshi:     Expected result
 *   reslo:     Expected result
 *   ov:        Expected overflow flag
 * ------------------------------------------------------------------------- */
#define TEST_MAC(machi, maclo, op1, op2, reshi, reslo, ov)		 \
	l.mfspr	r3,r0,SPR_SR						;\
	LOAD_CONST (r2, ~SPR_SR_OV)					;\
	l.and	r3,r3,r2		/* Clear flags */		;\
	l.mtspr	r0,r3,SPR_SR						;\
									;\
	LOAD_CONST (r2,maclo)						;\
	l.mtspr	r0,r2,SPR_MACLO						;\
	LOAD_CONST (r2,machi)						;\
	l.mtspr	r0,r2,SPR_MACHI						;\
									;\
	LOAD_CONST (r5,op1)		/* Load numbers to mac */	;\
	LOAD_CONST (r6,op2)						;\
	l.mtspr	r0,r0,SPR_EPCR_BASE	/* Clear record */		;\
        l.mtspr	r0,r0,SPR_AESR						;\
50:     l.mac	r5,r6							;\
	l.mfspr	r5,r0,SPR_MACHI						;\
	l.mfspr	r6,r0,SPR_MACLO						;\
	l.mfspr	r2,r0,SPR_SR		/* So we can examine flags */	;\
	l.mfspr	r3,r0,SPR_EPCR_BASE	/* What triggered exception */	;\
	l.mfspr	r4,r0,SPR_AESR						;\
									;\
        /* clear OVE so intervening l.add instructions don't trigger */	;\
        /* an exception */						;\
        LOAD_CONST(r7, ~SPR_SR_OVE)					;\
        l.and	r7,r2,r7						;\
        l.mtspr	r0,r7,SPR_SR						;\
									;\
	PUSH (r2)			/* Save SR for later */		;\
	PUSH (r4)			/* Save AESR for later */	;\
	PUSH (r3)			/* Save EPCR for later */	;\
	PUSH (r2)							;\
	PUSH (r5)			/* Save for later */		;\
	PUSH (r6)							;\
									;\
	PUTS ("  0x")							;\
	PUTH (machi)							;\
	PUTS (" ")							;\
	PUTH (maclo)							;\
	PUTS (" + 0x")							;\
	PUTH (op1)							;\
	PUTS (" * 0x")							;\
	PUTH (op2)							;\
	PUTS (" = 0x")							;\
	PUTH (reshi)							;\
	PUTS (" ")							;\
	PUTH (reslo)							;\
	PUTS (": ")							;\
	POP (r6)							;\
	POP (r5)							;\
	CHECK_RES2 (r5, r6, reshi, reslo)				;\
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
	/* if l.mul ever sets an exception in, OVMACADDE will be the */	;\
	/* only bit set */						;\
        PUTS ("  - AESR ")						;\
        PUTH (SPR_AESR_OVMACADDE)					;\
        PUTS (":       ")						;\
	POP (r2)			/* Retrieve AESR */		;\
        CHECK_RES1 (r2, SPR_AESR_OVMACADDE)				;\
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
 * A macro to carry out a test of multiply accumulate with immediate arg
 *
 * There is currently a bug in the assembler, so we must hand construct
 * l.maci r5,op1.
 *
 * Arguments
 *   machi:     Inital value of MACHI
 *   maclo:     Inital value of MACLO
 *   op1:       First operand value
 *   op2:       Second operand value
 *   reshi:     Expected result
 *   reslo:     Expected result
 * ------------------------------------------------------------------------- */
#define TEST_MACI(machi, maclo, op1, op2, reshi, reslo, ov)		 \
	l.mfspr	r3,r0,SPR_SR						;\
	LOAD_CONST (r2, ~SPR_SR_OV)					;\
	l.and	r3,r3,r2		/* Clear flags */		;\
	l.mtspr	r0,r3,SPR_SR						;\
									;\
	LOAD_CONST (r2,maclo)						;\
	l.mtspr	r0,r2,SPR_MACLO						;\
	LOAD_CONST (r2,machi)						;\
	l.mtspr	r0,r2,SPR_MACHI						;\
									;\
	LOAD_CONST (r5,op1)		/* Load number to add */	;\
	l.mtspr	r0,r0,SPR_EPCR_BASE	/* Clear record */		;\
        l.mtspr	r0,r0,SPR_AESR						;\
	l.maci	r5,op2							;\
	l.mfspr	r5,r0,SPR_MACHI						;\
	l.mfspr	r6,r0,SPR_MACLO						;\
	l.mfspr	r2,r0,SPR_SR		/* So we can examine flags */	;\
	l.mfspr	r3,r0,SPR_EPCR_BASE	/* What triggered exception */	;\
	l.mfspr	r4,r0,SPR_AESR						;\
									;\
        /* clear OVE so intervening l.add instructions don't trigger */	;\
        /* an exception */						;\
        LOAD_CONST(r7, ~SPR_SR_OVE)					;\
        l.and	r7,r2,r7						;\
        l.mtspr	r0,r7,SPR_SR						;\
									;\
	PUSH (r2)			/* Save SR for later */		;\
	PUSH (r4)			/* Save AESR for later */	;\
	PUSH (r3)			/* Save EPCR for later */	;\
	PUSH (r2)							;\
	PUSH (r5)			/* Save for later */		;\
	PUSH (r6)							;\
									;\
	PUTS ("  0x")							;\
	PUTH (machi)							;\
	PUTS (" ")							;\
	PUTH (maclo)							;\
	PUTS (" + 0x")							;\
	PUTH (op1)							;\
	PUTS (" * 0x")							;\
	PUTH (op2)							;\
	PUTS (" = 0x")							;\
	PUTH (reshi)							;\
	PUTS (" ")							;\
	PUTH (reslo)							;\
	PUTS (": ")							;\
	POP (r6)							;\
	POP (r5)							;\
	CHECK_RES2 (r5, r6, reshi, reslo)				;\
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
	/* if l.mul ever sets an exception in, OVMACADDE will be the */	;\
	/* only bit set */						;\
        PUTS ("  - AESR ")						;\
        PUTH (SPR_AESR_OVMACADDE)					;\
        PUTS (":       ")						;\
	POP (r2)			/* Retrieve AESR */		;\
        CHECK_RES1 (r2, SPR_AESR_OVMACADDE)				;\
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
 * A macro to carry out a test of multiply and subract
 *
 * Arguments
 *   machi:     Inital value of MACHI
 *   maclo:     Inital value of MACLO
 *   op1:       First operand value
 *   op2:       Second operand value
 *   reshi:     Expected result
 *   reslo:     Expected result
 * ------------------------------------------------------------------------- */
#define TEST_MSB(machi, maclo, op1, op2, reshi, reslo, ov)		 \
	l.mfspr	r3,r0,SPR_SR						;\
	LOAD_CONST (r2, ~SPR_SR_OV)					;\
	l.and	r3,r3,r2		/* Clear flags */		;\
	l.mtspr	r0,r3,SPR_SR						;\
									;\
	LOAD_CONST (r2,maclo)						;\
	l.mtspr	r0,r2,SPR_MACLO						;\
	LOAD_CONST (r2,machi)						;\
	l.mtspr	r0,r2,SPR_MACHI						;\
									;\
	LOAD_CONST (r5,op1)		/* Load numbers to add */	;\
	LOAD_CONST (r6,op2)						;\
	l.mtspr	r0,r0,SPR_EPCR_BASE	/* Clear record */		;\
        l.mtspr	r0,r0,SPR_AESR						;\
50:     l.msb	r5,r6							;\
	l.mfspr	r5,r0,SPR_MACHI						;\
	l.mfspr	r6,r0,SPR_MACLO						;\
	l.mfspr	r2,r0,SPR_SR		/* So we can examine flags */	;\
	l.mfspr	r3,r0,SPR_EPCR_BASE	/* What triggered exception */	;\
	l.mfspr	r4,r0,SPR_AESR						;\
									;\
        /* clear OVE so intervening l.add instructions don't trigger */	;\
        /* an exception */						;\
        LOAD_CONST(r7, ~SPR_SR_OVE)					;\
        l.and	r7,r2,r7						;\
        l.mtspr	r0,r7,SPR_SR						;\
									;\
	PUSH (r2)			/* Save SR for later */		;\
	PUSH (r4)			/* Save AESR for later */	;\
	PUSH (r3)			/* Save EPCR for later */	;\
	PUSH (r2)							;\
	PUSH (r5)			/* Save for later */		;\
	PUSH (r6)							;\
									;\
	PUTS ("  0x")							;\
	PUTH (machi)							;\
	PUTS (" ")							;\
	PUTH (maclo)							;\
	PUTS (" - 0x")							;\
	PUTH (op1)							;\
	PUTS (" * 0x")							;\
	PUTH (op2)							;\
	PUTS (" = 0x")							;\
	PUTH (reshi)							;\
	PUTS (" ")							;\
	PUTH (reslo)							;\
	PUTS (": ")							;\
	POP (r6)							;\
	POP (r5)							;\
	CHECK_RES2 (r5, r6, reshi, reslo)				;\
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
	/* if l.mul ever sets an exception in, OVMACADDE will be the */	;\
	/* only bit set */						;\
        PUTS ("  - AESR ")						;\
        PUTH (SPR_AESR_OVMACADDE)					;\
        PUTS (":       ")						;\
	POP (r2)			/* Retrieve AESR */		;\
        CHECK_RES1 (r2, SPR_AESR_OVMACADDE)				;\
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

/* ----------------------------------------------------------------------------
 * Test of multiply signed and accumulate, l.mac
 * ------------------------------------------------------------------------- */
_mac:
	LOAD_STR (r3, "l.mac\n")
	l.jal	_puts
	l.nop

	/* MAC two small positive numbers on a zero total */
	TEST_MAC (0x00000000, 0x00000000,
	          0x00000002, 0x00000003,
	          0x00000000, 0x00000006,
                  FALSE)

	/* MAC two small positive numbers on a small positive total */
	TEST_MAC (0x00000000, 0x00000006,
	          0x00000002, 0x00000003,
	          0x00000000, 0x0000000c,
                  FALSE)

	/* MAC two small positive numbers on a moderate positive total */
	TEST_MAC (0x00000000, 0xfffffffa,
	          0x00000002, 0x00000003,
	          0x00000001, 0x00000000,
                  FALSE)

	/* MAC two small positive numbers on a large positive total */
	TEST_MAC (0x3fffffff, 0xfffffffa,
	          0x00000002, 0x00000003,
	          0x40000000, 0x00000000,
                  FALSE)

	/* MAC two small positive numbers on a small negative total */
	TEST_MAC (0xffffffff, 0xfffffffa,
	          0x00000002, 0x00000003,
	          0x00000000, 0x00000000,
                  FALSE)

	/* MAC two small positive numbers on a moderate negative total */
	TEST_MAC (0xffffffff, 0x00000000,
	          0x00000002, 0x00000003,
	          0xffffffff, 0x00000006,
                  FALSE)

	/* MAC two small positive numbers on a large negative total */
	TEST_MAC (0x80000000, 0x00000000,
	          0x00000002, 0x00000003,
	          0x80000000, 0x00000006,
                  FALSE)

	PUTC ('\n')

	/* MAC two moderate positive numbers on a zero total */
	TEST_MAC (0x00000000, 0x00000000,
	          0x00008001, 0x0000fffe,
	          0x00000000, 0x7ffffffe,
                  FALSE)

	/* MAC two moderate positive numbers on a small positive total */
	TEST_MAC (0x00000000, 0x00000002,
	          0x00008001, 0x0000fffe,
	          0x00000000, 0x80000000,
                  FALSE)

	/* MAC two moderate positive numbers on a moderate positive total */
	TEST_MAC (0x00000000, 0x80000002,
	          0x00008001, 0x0000fffe,
	          0x00000001, 0x00000000,
                  FALSE)

	/* MAC two moderate positive numbers on a large positive total */
	TEST_MAC (0x7fffffff, 0x80000001,
	          0x00008001, 0x0000fffe,
	          0x7fffffff, 0xffffffff,
                  FALSE)

	/* MAC two moderate positive numbers on a small negative total */
	TEST_MAC (0xffffffff, 0xffffffff,
	          0x00008001, 0x0000fffe,
	          0x00000000, 0x7ffffffd,
                  FALSE)

	/* MAC two moderate positive numbers on a moderate negative total */
	TEST_MAC (0xffffffff, 0x80000002,
	          0x00008001, 0x0000fffe,
	          0x00000000, 0x00000000,
                  FALSE)

	/* MAC two moderate positive numbers on a large negative total */
	TEST_MAC (0xfffffffe, 0x80000002,
	          0x00008001, 0x0000fffe,
	          0xffffffff, 0x00000000,
                  FALSE)

	PUTC ('\n')

	/* MAC two small negative numbers on a zero total */
	TEST_MAC (0x00000000, 0x00000000,
	          0xfffffffe, 0xfffffffd,
	          0x00000000, 0x00000006,
                  FALSE)

	/* MAC two small negative numbers on a small positive total */
	TEST_MAC (0x00000000, 0x00000006,
	          0xfffffffe, 0xfffffffd,
	          0x00000000, 0x0000000c,
                  FALSE)

	/* MAC two small negative numbers on a small negative total */
	TEST_MAC (0xffffffff, 0xffffffff,
	          0xfffffffe, 0xfffffffd,
	          0x00000000, 0x00000005,
                  FALSE)

	PUTC ('\n')

	/* MAC one small positive and one small negative number on a zero
	   total */
	TEST_MAC (0x00000000, 0x00000000,
	          0x00000002, 0xfffffffd,
	          0xffffffff, 0xfffffffa,
                  FALSE)

	/* MAC one small positive and one small negative number on a small
	   positive total */
	TEST_MAC (0x00000000, 0x0000000c,
	          0x00000002, 0xfffffffd,
	          0x00000000, 0x00000006,
                  FALSE)

	/* MAC one small positive and one small negative number on a moderate
	   positive total */
	TEST_MAC (0x00000001, 0x00000005,
	          0x00000002, 0xfffffffd,
	          0x00000000, 0xffffffff,
                  FALSE)

	/* MAC one small positive and one small negative number on a large
	   positive total */
	TEST_MAC (0x7fffffff, 0xffffffff,
	          0x00000002, 0xfffffffd,
	          0x7fffffff, 0xfffffff9,
                  FALSE)

	/* MAC one small positive and one small negative number on a small
	   negative total */
	TEST_MAC (0xffffffff, 0xffffffff,
	          0x00000002, 0xfffffffd,
	          0xffffffff, 0xfffffff9,
                  FALSE)

	/* MAC one small positive and one small negative number on a moderate
	   negative total */
	TEST_MAC (0xffffffff, 0x00000005,
	          0x00000002, 0xfffffffd,
	          0xfffffffe, 0xffffffff,
                  FALSE)

	/* MAC one small positive and one small negative number on a large
	   negative total */
	TEST_MAC (0x80000000, 0x00000006,
	          0x00000002, 0xfffffffd,
	          0x80000000, 0x00000000,
                  FALSE)

	PUTC ('\n')

	/* MAC one moderate positive and one moderate negative number on a
	   zero total */
	TEST_MAC (0x00000000, 0x00000000,
	          0x00008000, 0xffff0000,
	          0xffffffff, 0x80000000,
                  FALSE)

	/* MAC one moderate positive and one moderate negative number on a
	   small positive total */
	TEST_MAC (0x00000000, 0x00000006,
	          0x00008000, 0xffff0000,
	          0xffffffff, 0x80000006,
                  FALSE)

	/* MAC one moderate positive and one moderate negative number on a
	   moderate positive total */
	TEST_MAC (0x00000000, 0x80000000,
	          0x00008000, 0xffff0000,
	          0x00000000, 0x00000000,
                  FALSE)

	/* MAC one moderate positive and one moderate negative number on a
	   large positive total */
	TEST_MAC (0x7fffffff, 0xffffffff,
	          0x00008000, 0xffff0000,
	          0x7fffffff, 0x7fffffff,
                  FALSE)

	/* MAC one moderate positive and one moderate negative number on a
	   small negative total */
	TEST_MAC (0xffffffff, 0xffffffff,
	          0x00008000, 0xffff0000,
	          0xffffffff, 0x7fffffff,
                  FALSE)

	/* MAC one moderate positive and one moderate negative number on a
	   moderate negative total */
	TEST_MAC (0xffffffff, 0x7fffffff,
	          0x00008000, 0xffff0000,
	          0xfffffffe, 0xffffffff,
                  FALSE)

	/* MAC one moderate positive and one moderate negative number on a
	   large negative total */
	TEST_MAC (0x80000000, 0x80000000,
	          0x00008000, 0xffff0000,
	          0x80000000, 0x00000000,
                  FALSE)

	PUTC ('\n')

        /* MAC 1 and -1 with largest negative total, should set overflow */
	TEST_MAC (0x80000000, 0x00000000,
	          0xffffffff, 0x00000001,
	          0x7fffffff, 0xffffffff,
                  TRUE)

        /* MAC 1 and -1 with -1, should not set overflow */
	TEST_MAC (0xffffffff, 0xffffffff,
	          0xffffffff, 0x00000001,
	          0xffffffff, 0xfffffffe,
                  FALSE)

        /* MAC large positive and large negative with large negative,
   	   should set overflow */
	TEST_MAC (0x7fffffff, 0xffffffff,
	          0xbfffffff, 0xffff8000,
	          0x80002000, 0x00007fff,
                  TRUE)

	PUTC ('\n')
        
        /* check if AECSRP is set */
        l.mfspr r3,r0,SPR_CPUCFGR
        LOAD_CONST(r2, SPR_CPUCFGR_AECSRP)
        l.and r3,r3,r2
        l.sfeq r3,r2
        OR1K_DELAYED_NOP(l.bf _mac_aecsr)

_mac_no_aecsr:
	LOAD_STR (r3, "  ** OVE flag set **\n")
	OR1K_DELAYED_NOP(l.jal	_puts)

	/* Check that range exceptions are triggered */
	l.mfspr	r3,r0,SPR_SR
	LOAD_CONST (r2, SPR_SR_OVE)	/* Set OVE */
	l.or	r3,r3,r2
	l.mtspr	r0,r3,SPR_SR

        /* MAC 1 and -1 with largest negative total, should raise exception */
	TEST_MAC (0x80000000, 0x00000000,
	          0xffffffff, 0x00000001,
	          0x7fffffff, 0xffffffff,
                  TRUE)

        /* MAC 1 and -1 with -1, should not raise exception */
	TEST_MAC (0xffffffff, 0xffffffff,
	          0xffffffff, 0x00000001,
	          0xffffffff, 0xfffffffe,
                  FALSE)

        /* MAC large positive and large negative with large negative,
   	   should raise exception */
	TEST_MAC (0x7fffffff, 0xffffffff,
	          0xbfffffff, 0xffff8000,
	          0x80002000, 0x00007fff,
                  TRUE)

        OR1K_DELAYED_NOP(l.j _mac_except_done)

_mac_aecsr:
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

        /* MAC 1 and -1 with largest negative total, should not raise exception */
	TEST_MAC (0x80000000, 0x00000000,
	          0xffffffff, 0x00000001,
	          0x7fffffff, 0xffffffff,
                  TRUE)

        /* MAC 1 and -1 with -1, should not raise exception */
	TEST_MAC (0xffffffff, 0xffffffff,
	          0xffffffff, 0x00000001,
	          0xffffffff, 0xfffffffe,
                  FALSE)

        /* MAC large positive and large negative with large negative,
   	   should not raise exception */
	TEST_MAC (0x7fffffff, 0xffffffff,
	          0xbfffffff, 0xffff8000,
	          0x80002000, 0x00007fff,
                  TRUE)
        
        /* Clear OVE so following printout won't raise an exception */
        l.mfspr r3,r0,SPR_SR
        LOAD_CONST (r2, ~SPR_SR_OVE)
        l.and r3,r2,r3
        l.mtspr r0,r3,SPR_SR
        
	PUTC ('\n')
        
	LOAD_STR (r3, "  ** AECR set to OVMACADDE **\n")
	OR1K_DELAYED_NOP(l.jal	_puts)
	
        /* set AECR to OVMACADDE */
        LOAD_CONST(r2, SPR_AECR_OVMACADDE)
        l.mtspr r0,r2,SPR_AECR
        
        /* Set OVE again */
	l.mfspr	r3,r0,SPR_SR
	LOAD_CONST (r2, SPR_SR_OVE)	/* Set OVE */
	l.or	r3,r3,r2
	l.mtspr	r0,r3,SPR_SR
	
        /* set AECR to OVMACADDE */
        LOAD_CONST(r2, SPR_AECR_OVMACADDE)
        l.mtspr r0,r2,SPR_AECR


        /* MAC 1 and -1 with largest negative total, should raise exception */
	TEST_MAC (0x80000000, 0x00000000,
	          0xffffffff, 0x00000001,
	          0x7fffffff, 0xffffffff,
                  TRUE)

        /* MAC 1 and -1 with -1, should not raise exception */
	TEST_MAC (0xffffffff, 0xffffffff,
	          0xffffffff, 0x00000001,
	          0xffffffff, 0xfffffffe,
                  FALSE)

        /* MAC large positive and large negative with large negative,
   	   should raise exception */
	TEST_MAC (0x7fffffff, 0xffffffff,
	          0xbfffffff, 0xffff8000,
	          0x80002000, 0x00007fff,
                  TRUE)

        /* Clear OVE so following printout won't raise an exception */
        l.mfspr r3,r0,SPR_SR
        LOAD_CONST (r2, ~SPR_SR_OVE)
        l.and r3,r2,r3
        l.mtspr r0,r3,SPR_SR
        
	PUTC ('\n')
        
	LOAD_STR (r3, "  ** AECR set to CYMACADDE **\n")
	OR1K_DELAYED_NOP(l.jal	_puts)
        
        /* set AECR to CYMACADDE */
        LOAD_CONST(r2, SPR_AECR_CYMACADDE)
        l.mtspr r0,r2,SPR_AECR
        
        /* Set OVE again */
	l.mfspr	r3,r0,SPR_SR
	LOAD_CONST (r2, SPR_SR_OVE)	/* Set OVE */
	l.or	r3,r3,r2
	l.mtspr	r0,r3,SPR_SR
	
        /* set AECR to CYMACADDE */
        LOAD_CONST(r2, SPR_AECR_CYMACADDE)
        l.mtspr r0,r2,SPR_AECR

        /* MAC 1 and -1 with largest negative total, should not raise exception */
	TEST_MAC (0x80000000, 0x00000000,
	          0xffffffff, 0x00000001,
	          0x7fffffff, 0xffffffff,
                  TRUE)

        /* MAC 1 and -1 with -1, should not raise exception */
	TEST_MAC (0xffffffff, 0xffffffff,
	          0xffffffff, 0x00000001,
	          0xffffffff, 0xfffffffe,
                  FALSE)

        /* MAC large positive and large negative with large negative,
   	   should not raise exception */
	TEST_MAC (0x7fffffff, 0xffffffff,
	          0xbfffffff, 0xffff8000,
	          0x80002000, 0x00007fff,
                  TRUE)

_mac_except_done:
	/* Finished checking range exceptions */
	l.mfspr	r3,r0,SPR_SR
	LOAD_CONST (r2, ~SPR_SR_OVE)	/* Clear OVE */
	l.and	r3,r3,r2
	l.mtspr	r0,r3,SPR_SR
	
	PUTC ('\n')
        
	LOAD_STR (r3, "  ** OVE flag cleared **\n")
	l.jal	_puts
	l.nop
        
	PUTC ('\n')

/* ----------------------------------------------------------------------------
 * Test of multiply signed and accumulate, l.maci
 * ------------------------------------------------------------------------- */
_maci:
	LOAD_STR (r3, "l.maci\n")
	l.jal	_puts
	l.nop

	/* MAC two small positive numbers on a zero total */
	TEST_MACI (0x00000000, 0x00000000,
	           0x00000002, 0x0003,
	           0x00000000, 0x00000006,
                   FALSE)

	/* MAC two small positive numbers on a small positive total */
	TEST_MACI (0x00000000, 0x00000006,
	           0x00000002, 0x0003,
	           0x00000000, 0x0000000c,
                   FALSE)

	/* MAC two small positive numbers on a moderate positive total */
	TEST_MACI (0x00000000, 0xfffffffa,
	           0x00000002, 0x0003,
	           0x00000001, 0x00000000,
                   FALSE)

	/* MAC two small positive numbers on a large positive total */
	TEST_MACI (0x3fffffff, 0xfffffffa,
	           0x00000002, 0x0003,
	           0x40000000, 0x00000000,
                   FALSE)

	/* MAC two small positive numbers on a small negative total */
	TEST_MACI (0xffffffff, 0xfffffffa,
	           0x00000002, 0x0003,
	           0x00000000, 0x00000000,
                   FALSE)

	/* MAC two small positive numbers on a moderate negative total */
	TEST_MACI (0xffffffff, 0x00000000,
	           0x00000002, 0x0003,
	           0xffffffff, 0x00000006,
                   FALSE)

	/* MAC two small positive numbers on a large negative total */
	TEST_MACI (0x80000000, 0x00000000,
	           0x00000002, 0x0003,
	           0x80000000, 0x00000006,
                   FALSE)

	PUTC ('\n')
tmp:	
	/* MAC two moderate positive numbers on a zero total */
	TEST_MACI (0x00000000, 0x00000000,
	           0x00010002, 0x7fff,
	           0x00000000, 0x7ffffffe,
                   FALSE)

	/* MAC two moderate positive numbers on a small positive total */
	TEST_MACI (0x00000000, 0x00000002,
	           0x00010002, 0x7fff,
	           0x00000000, 0x80000000,
                   FALSE)

	/* MAC two moderate positive numbers on a moderate positive total */
	TEST_MACI (0x00000000, 0x80000002,
	           0x00010002, 0x7fff,
	           0x00000001, 0x00000000,
                   FALSE)

	/* MAC two moderate positive numbers on a large positive total */
	TEST_MACI (0x7fffffff, 0x80000001,
	           0x00010002, 0x7fff,
	           0x7fffffff, 0xffffffff,
                   FALSE)

	/* MAC two moderate positive numbers on a small negative total */
	TEST_MACI (0xffffffff, 0xffffffff,
	           0x00010002, 0x7fff,
	           0x00000000, 0x7ffffffd,
                   FALSE)

	/* MAC two moderate positive numbers on a moderate negative total */
	TEST_MACI (0xffffffff, 0x80000002,
	           0x00010002, 0x7fff,
	           0x00000000, 0x00000000,
                   FALSE)

	/* MAC two moderate positive numbers on a large negative total */
	TEST_MACI (0xfffffffe, 0x80000002,
	           0x00010002, 0x7fff,
	           0xffffffff, 0x00000000,
                   FALSE)

	PUTC ('\n')

	/* MAC two small negative numbers on a zero total */
	TEST_MACI (0x00000000, 0x00000000,
	           0xfffffffe, 0xfffd,
	           0x00000000, 0x00000006,
                   FALSE)

	/* MAC two small negative numbers on a small positive total */
	TEST_MACI (0x00000000, 0x00000006,
	           0xfffffffe, 0xfffd,
	           0x00000000, 0x0000000c,
                   FALSE)

	/* MAC two small negative numbers on a small negative total */
	TEST_MACI (0xffffffff, 0xffffffff,
	           0xfffffffe, 0xfffd,
	           0x00000000, 0x00000005,
                   FALSE)

	PUTC ('\n')

	/* MAC one small positive and one small negative number on a zero
	   total */
	TEST_MACI (0x00000000, 0x00000000,
	           0x00000002, 0xfffd,
	           0xffffffff, 0xfffffffa,
                   FALSE)

	/* MAC one small positive and one small negative number on a small
	   positive total */
	TEST_MACI (0x00000000, 0x0000000c,
	           0x00000002, 0xfffd,
	           0x00000000, 0x00000006,
                   FALSE)

	/* MAC one small positive and one small negative number on a moderate
	   positive total */
	TEST_MACI (0x00000001, 0x00000005,
	           0x00000002, 0xfffd,
	           0x00000000, 0xffffffff,
                   FALSE)

	/* MAC one small positive and one small negative number on a large
	   positive total */
	TEST_MACI (0x7fffffff, 0xffffffff,
	           0x00000002, 0xfffd,
	           0x7fffffff, 0xfffffff9,
                   FALSE)

	/* MAC one small positive and one small negative number on a small
	   negative total */
	TEST_MACI (0xffffffff, 0xffffffff,
	           0x00000002, 0xfffd,
	           0xffffffff, 0xfffffff9,
                   FALSE)

	/* MAC one small positive and one small negative number on a moderate
	   negative total */
	TEST_MACI (0xffffffff, 0x00000005,
	           0x00000002, 0xfffd,
	           0xfffffffe, 0xffffffff,
                   FALSE)

	/* MAC one small positive and one small negative number on a large
	   negative total */
	TEST_MACI (0x80000000, 0x00000006,
	           0x00000002, 0xfffd,
	           0x80000000, 0x00000000,
                   FALSE)

	PUTC ('\n')

	/* MAC one moderate positive and one moderate negative number on a
	   zero total */
	TEST_MACI (0x00000000, 0x00000000,
	           0x00010000, 0x8000,
	           0xffffffff, 0x80000000,
                   FALSE)

	/* MAC one moderate positive and one moderate negative number on a
	   small positive total */
	TEST_MACI (0x00000000, 0x00000006,
	           0x00010000, 0x8000,
	           0xffffffff, 0x80000006,
                   FALSE)

	/* MAC one moderate positive and one moderate negative number on a
	   moderate positive total */
	TEST_MACI (0x00000000, 0x80000000,
	           0x00010000, 0x8000,
	           0x00000000, 0x00000000,
                   FALSE)

	/* MAC one moderate positive and one moderate negative number on a
	   large positive total */
	TEST_MACI (0x7fffffff, 0xffffffff,
	           0x00010000, 0x8000,
	           0x7fffffff, 0x7fffffff,
                   FALSE)

	/* MAC one moderate positive and one moderate negative number on a
	   small negative total */
	TEST_MACI (0xffffffff, 0xffffffff,
	           0x00010000, 0x8000,
	           0xffffffff, 0x7fffffff,
                   FALSE)

	/* MAC one moderate positive and one moderate negative number on a
	   moderate negative total */
	TEST_MACI (0xffffffff, 0x7fffffff,
	           0x00010000, 0x8000,
	           0xfffffffe, 0xffffffff,
                   FALSE)

	/* MAC one moderate positive and one moderate negative number on a
	   large negative total */
	TEST_MACI (0x80000000, 0x80000000,
	           0x00010000, 0x8000,
	           0x80000000, 0x00000000,
                   FALSE)

	PUTC ('\n')
        
        /* MACI 1 and -1 with largest negative total, should not raise exception */
	TEST_MACI (0x80000000, 0x00000000,
	          0xffffffff, 0x00000001,
	          0x7fffffff, 0xffffffff,
                  TRUE)

        /* MACI 1 and -1 with -1, should not raise exception */
	TEST_MACI (0xffffffff, 0xffffffff,
	          0xffffffff, 0x00000001,
	          0xffffffff, 0xfffffffe,
                  FALSE)

        /* MACI large positive and large negative with large negative,
   	   should raise exception */
	TEST_MACI (0x7fffffff, 0xffffffff,
	          0xbfffffff, 0xffff8000,
	          0x80002000, 0x00007fff,
                  TRUE)

	PUTC ('\n')
        
        /* check if AECSRP is set */
        l.mfspr r3,r0,SPR_CPUCFGR
        LOAD_CONST(r2, SPR_CPUCFGR_AECSRP)
        l.and r3,r3,r2
        l.sfeq r3,r2
        OR1K_DELAYED_NOP(l.bf _maci_aecsr)

_maci_no_aecsr:
	LOAD_STR (r3, "  ** OVE flag set **\n")
	OR1K_DELAYED_NOP(l.jal	_puts)

	/* Check that range exceptions are triggered */
	l.mfspr	r3,r0,SPR_SR
	LOAD_CONST (r2, SPR_SR_OVE)	/* Set OVE */
	l.or	r3,r3,r2
	l.mtspr	r0,r3,SPR_SR

        /* MACI 1 and -1 with largest negative total, should raise exception */
	TEST_MACI (0x80000000, 0x00000000,
	          0xffffffff, 0x00000001,
	          0x7fffffff, 0xffffffff,
                  TRUE)

        /* MACI 1 and -1 with -1, should not raise exception */
	TEST_MACI (0xffffffff, 0xffffffff,
	          0xffffffff, 0x00000001,
	          0xffffffff, 0xfffffffe,
                  FALSE)

        /* MACI large positive and large negative with large negative,
   	   should raise exception */
	TEST_MACI (0x7fffffff, 0xffffffff,
	          0xbfffffff, 0xffff8000,
	          0x80002000, 0x00007fff,
                  TRUE)

        OR1K_DELAYED_NOP(l.j _maci_except_done)

_maci_aecsr:
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

        /* MACI 1 and -1 with largest negative total, should not raise exception */
	TEST_MACI (0x80000000, 0x00000000,
	          0xffffffff, 0x00000001,
	          0x7fffffff, 0xffffffff,
                  TRUE)

        /* MACI 1 and -1 with -1, should not raise exception */
	TEST_MACI (0xffffffff, 0xffffffff,
	          0xffffffff, 0x00000001,
	          0xffffffff, 0xfffffffe,
                  FALSE)

        /* MACI large positive and large negative with large negative,
   	   should not raise exception */
	TEST_MACI (0x7fffffff, 0xffffffff,
	          0xbfffffff, 0xffff8000,
	          0x80002000, 0x00007fff,
                  TRUE)
        
        /* Clear OVE so following printout won't raise an exception */
        l.mfspr r3,r0,SPR_SR
        LOAD_CONST (r2, ~SPR_SR_OVE)
        l.and r3,r2,r3
        l.mtspr r0,r3,SPR_SR
        
	PUTC ('\n')
        
	LOAD_STR (r3, "  ** AECR set to OVMACADDE **\n")
	OR1K_DELAYED_NOP(l.jal	_puts)
	
        /* set AECR to OVMACADDE */
        LOAD_CONST(r2, SPR_AECR_OVMACADDE)
        l.mtspr r0,r2,SPR_AECR
        
        /* Set OVE again */
	l.mfspr	r3,r0,SPR_SR
	LOAD_CONST (r2, SPR_SR_OVE)	/* Set OVE */
	l.or	r3,r3,r2
	l.mtspr	r0,r3,SPR_SR
	
        /* set AECR to OVMACADDE */
        LOAD_CONST(r2, SPR_AECR_OVMACADDE)
        l.mtspr r0,r2,SPR_AECR


        /* MACI 1 and -1 with largest negative total, should raise exception */
	TEST_MACI (0x80000000, 0x00000000,
	          0xffffffff, 0x00000001,
	          0x7fffffff, 0xffffffff,
                  TRUE)

        /* MACI 1 and -1 with -1, should not raise exception */
	TEST_MACI (0xffffffff, 0xffffffff,
	          0xffffffff, 0x00000001,
	          0xffffffff, 0xfffffffe,
                  FALSE)

        /* MACI large positive and large negative with large negative,
   	   should raise exception */
	TEST_MACI (0x7fffffff, 0xffffffff,
	          0xbfffffff, 0xffff8000,
	          0x80002000, 0x00007fff,
                  TRUE)

        /* Clear OVE so following printout won't raise an exception */
        l.mfspr r3,r0,SPR_SR
        LOAD_CONST (r2, ~SPR_SR_OVE)
        l.and r3,r2,r3
        l.mtspr r0,r3,SPR_SR
        
	PUTC ('\n')
        
	LOAD_STR (r3, "  ** AECR set to CYMACADDE **\n")
	OR1K_DELAYED_NOP(l.jal	_puts)
        
        /* set AECR to CYMACADDE */
        LOAD_CONST(r2, SPR_AECR_CYMACADDE)
        l.mtspr r0,r2,SPR_AECR
        
        /* Set OVE again */
	l.mfspr	r3,r0,SPR_SR
	LOAD_CONST (r2, SPR_SR_OVE)	/* Set OVE */
	l.or	r3,r3,r2
	l.mtspr	r0,r3,SPR_SR
	
        /* set AECR to CYMACADDE */
        LOAD_CONST(r2, SPR_AECR_CYMACADDE)
        l.mtspr r0,r2,SPR_AECR

        /* MACI 1 and -1 with largest negative total, should not raise exception */
	TEST_MACI (0x80000000, 0x00000000,
	          0xffffffff, 0x00000001,
	          0x7fffffff, 0xffffffff,
                  TRUE)

        /* MACI 1 and -1 with -1, should not raise exception */
	TEST_MACI (0xffffffff, 0xffffffff,
	          0xffffffff, 0x00000001,
	          0xffffffff, 0xfffffffe,
                  FALSE)

        /* MACI large positive and large negative with large negative,
   	   should not raise exception */
	TEST_MACI (0x7fffffff, 0xffffffff,
	          0xbfffffff, 0xffff8000,
	          0x80002000, 0x00007fff,
                  TRUE)

_maci_except_done:
	/* Finished checking range exceptions */
	l.mfspr	r3,r0,SPR_SR
	LOAD_CONST (r2, ~SPR_SR_OVE)	/* Clear OVE */
	l.and	r3,r3,r2
	l.mtspr	r0,r3,SPR_SR
	
	PUTC ('\n')
        
	LOAD_STR (r3, "  ** OVE flag cleared **\n")
	l.jal	_puts
	l.nop
        
	PUTC ('\n')

/* ----------------------------------------------------------------------------
 * Test of multiply signed and accumulate, read and clear l.macrc
 * ------------------------------------------------------------------------- */
_macrc:
	LOAD_STR (r3, "l.macrc\n")
	l.jal	_puts
	l.nop

	/* MAC two small positive numbers on a zero total */
	TEST_MACRC (0x00000000, 0x00000000,
	            0x00000002, 0x00000003,
	            0x00000006)

	/* MAC two small positive numbers on a small positive total */
	TEST_MACRC (0x00000000, 0x00000006,
	            0x00000002, 0x00000003,
	            0x0000000c)

	/* MAC two small positive numbers on a moderate positive total */
	TEST_MACRC (0x00000000, 0xfffffffa,
	          0x00000002, 0x00000003,
	          0x00000000)

	/* MAC two small positive numbers on a large positive total */
	TEST_MACRC (0x3fffffff, 0xfffffffa,
	            0x00000002, 0x00000003,
	            0x00000000)

	/* MAC two small positive numbers on a small negative total */
	TEST_MACRC (0xffffffff, 0xfffffffa,
	            0x00000002, 0x00000003,
	            0x00000000)

	/* MAC two small positive numbers on a moderate negative total */
	TEST_MACRC (0xffffffff, 0x00000000,
	            0x00000002, 0x00000003,
	            0x00000006)

	/* MAC two small positive numbers on a large negative total */
	TEST_MACRC (0x80000000, 0x00000000,
	            0x00000002, 0x00000003,
	            0x00000006)

	PUTC ('\n')

	/* MAC two moderate positive numbers on a zero total */
	TEST_MACRC (0x00000000, 0x00000000,
	            0x00008001, 0x0000fffe,
	            0x7ffffffe)

	/* MAC two moderate positive numbers on a small positive total */
	TEST_MACRC (0x00000000, 0x00000002,
	            0x00008001, 0x0000fffe,
	            0x80000000)

	/* MAC two moderate positive numbers on a moderate positive total */
	TEST_MACRC (0x00000000, 0x80000002,
	            0x00008001, 0x0000fffe,
	            0x00000000)

	/* MAC two moderate positive numbers on a large positive total */
	TEST_MACRC (0x7fffffff, 0x80000001,
	            0x00008001, 0x0000fffe,
	            0xffffffff)

	/* MAC two moderate positive numbers on a small negative total */
	TEST_MACRC (0xffffffff, 0xffffffff,
	            0x00008001, 0x0000fffe,
	            0x7ffffffd)

	/* MAC two moderate positive numbers on a moderate negative total */
	TEST_MACRC (0xffffffff, 0x80000002,
	            0x00008001, 0x0000fffe,
	            0x00000000)

	/* MAC two moderate positive numbers on a large negative total */
	TEST_MACRC (0xfffffffe, 0x80000002,
	            0x00008001, 0x0000fffe,
	            0x00000000)

	PUTC ('\n')

	/* MAC two small negative numbers on a zero total */
	TEST_MACRC (0x00000000, 0x00000000,
	            0xfffffffe, 0xfffffffd,
	            0x00000006)

	/* MAC two small negative numbers on a small positive total */
	TEST_MACRC (0x00000000, 0x00000006,
	            0xfffffffe, 0xfffffffd,
	            0x0000000c)

	/* MAC two small negative numbers on a small negative total */
	TEST_MACRC (0xffffffff, 0xffffffff,
	            0xfffffffe, 0xfffffffd,
	            0x00000005)

	PUTC ('\n')

	/* MAC one small positive and one small negative number on a zero
	   total */
	TEST_MACRC (0x00000000, 0x00000000,
	            0x00000002, 0xfffffffd,
	            0xfffffffa)

	/* MAC one small positive and one small negative number on a small
	   positive total */
	TEST_MACRC (0x00000000, 0x0000000c,
	            0x00000002, 0xfffffffd,
	            0x00000006)

	/* MAC one small positive and one small negative number on a moderate
	   positive total */
	TEST_MACRC (0x00000001, 0x00000005,
	            0x00000002, 0xfffffffd,
	            0xffffffff)

	/* MAC one small positive and one small negative number on a large
	   positive total */
	TEST_MACRC (0x7fffffff, 0xffffffff,
	            0x00000002, 0xfffffffd,
	            0xfffffff9)

	/* MAC one small positive and one small negative number on a small
	   negative total */
	TEST_MACRC (0xffffffff, 0xffffffff,
	            0x00000002, 0xfffffffd,
	            0xfffffff9)

	/* MAC one small positive and one small negative number on a moderate
	   negative total */
	TEST_MACRC (0xffffffff, 0x00000005,
	            0x00000002, 0xfffffffd,
	            0xffffffff)

	/* MAC one small positive and one small negative number on a large
	   negative total */
	TEST_MACRC (0x80000000, 0x00000006,
	            0x00000002, 0xfffffffd,
	            0x00000000)

	PUTC ('\n')

	/* MAC one moderate positive and one moderate negative number on a
	   zero total */
	TEST_MACRC (0x00000000, 0x00000000,
	            0x00008000, 0xffff0000,
	            0x80000000)

	/* MAC one moderate positive and one moderate negative number on a
	   small positive total */
	TEST_MACRC (0x00000000, 0x00000006,
	            0x00008000, 0xffff0000,
	            0x80000006)

	/* MAC one moderate positive and one moderate negative number on a
	   moderate positive total */
	TEST_MACRC (0x00000000, 0x80000000,
	            0x00008000, 0xffff0000,
	            0x00000000)

	/* MAC one moderate positive and one moderate negative number on a
	   large positive total */
	TEST_MACRC (0x7fffffff, 0xffffffff,
	            0x00008000, 0xffff0000,
	            0x7fffffff)

	/* MAC one moderate positive and one moderate negative number on a
	   small negative total */
	TEST_MACRC (0xffffffff, 0xffffffff,
	            0x00008000, 0xffff0000,
	            0x7fffffff)

	/* MAC one moderate positive and one moderate negative number on a
	   moderate negative total */
	TEST_MACRC (0xffffffff, 0x7fffffff,
	            0x00008000, 0xffff0000,
	            0xffffffff)

	/* MAC one moderate positive and one moderate negative number on a
	   large negative total */
	TEST_MACRC (0x80000000, 0x80000000,
	            0x00008000, 0xffff0000,
	            0x00000000)

	PUTC ('\n')

/* ----------------------------------------------------------------------------
 * Test of multiply signed and accumulate, l.msb
 * ------------------------------------------------------------------------- */
_msb:
	LOAD_STR (r3, "l.msb\n")
	l.jal	_puts
	l.nop

	/* MSB two small positive numbers on a zero total */
	TEST_MSB (0x00000000, 0x00000000,
	          0x00000002, 0x00000003,
	          0xffffffff, 0xfffffffa,
                  FALSE)

	/* MSB two small positive numbers on a small positive total */
	TEST_MSB (0x00000000, 0x0000000c,
	          0x00000002, 0x00000003,
	          0x00000000, 0x00000006,
                  FALSE)

	/* MSB two small positive numbers on a moderate positive total */
	TEST_MSB (0x00000001, 0x00000000,
	          0x00000002, 0x00000003,
	          0x00000000, 0xfffffffa,
                  FALSE)

	/* MSB two small positive numbers on a large positive total */
	TEST_MSB (0x40000000, 0x00000000,
	          0x00000002, 0x00000003,
	          0x3fffffff, 0xfffffffa,
                  FALSE)

	/* MSB two small positive numbers on a small negative total */
	TEST_MSB (0xffffffff, 0xfffffffa,
	          0x00000002, 0x00000003,
	          0xffffffff, 0xfffffff4,
                  FALSE)

	/* MSB two small positive numbers on a moderate negative total */
	TEST_MSB (0xffffffff, 0x00000005,
	          0x00000002, 0x00000003,
	          0xfffffffe, 0xffffffff,
                  FALSE)

	/* MSB two small positive numbers on a large negative total */
	TEST_MSB (0x80000000, 0x00000006,
	          0x00000002, 0x00000003,
	          0x80000000, 0x00000000,
                  FALSE)

	PUTC ('\n')

	/* MSB two moderate positive numbers on a zero total */
	TEST_MSB (0x00000000, 0x00000000,
	          0x00008001, 0x0000fffe,
	          0xffffffff, 0x80000002,
                  FALSE)

	/* MSB two moderate positive numbers on a small positive total */
	TEST_MSB (0x00000000, 0x00000002,
	          0x00008001, 0x0000fffe,
	          0xffffffff, 0x80000004,
                  FALSE)

	/* MSB two moderate positive numbers on a moderate positive total */
	TEST_MSB (0x00000000, 0x80000002,
	          0x00008001, 0x0000fffe,
	          0x00000000, 0x00000004,
                  FALSE)

	/* MSB two moderate positive numbers on a large positive total */
	TEST_MSB (0x7fffffff, 0x7ffffffd,
	          0x00008001, 0x0000fffe,
	          0x7ffffffe, 0xffffffff,
                  FALSE)

	/* MSB two moderate positive numbers on a small negative total */
	TEST_MSB (0xffffffff, 0xffffffff,
	          0x00008001, 0x0000fffe,
	          0xffffffff, 0x80000001,
                  FALSE)

	/* MSB two moderate positive numbers on a moderate negative total */
	TEST_MSB (0xffffffff, 0x80000002,
	          0x00008001, 0x0000fffe,
	          0xffffffff, 0x00000004,
                  FALSE)

	/* MSB two moderate positive numbers on a large negative total */
	TEST_MSB (0xfffffffe, 0x80000002,
	          0x00008001, 0x0000fffe,
	          0xfffffffe, 0x00000004,
                  FALSE)

	PUTC ('\n')

	/* MSB two small negative numbers on a zero total */
	TEST_MSB (0x00000000, 0x00000006,
	          0xfffffffe, 0xfffffffd,
	          0x00000000, 0x00000000,
                  FALSE)

	/* MSB two small negative numbers on a small positive total */
	TEST_MSB (0x00000000, 0x0000000c,
	          0xfffffffe, 0xfffffffd,
	          0x00000000, 0x00000006,
                  FALSE)

	/* MSB two small negative numbers on a small negative total */
	TEST_MSB (0xffffffff, 0xffffffff,
	          0xfffffffe, 0xfffffffd,
	          0xffffffff, 0xfffffff9,
                  FALSE)

	PUTC ('\n')

	/* MSB one small positive and one small negative number on a zero
	   total */
	TEST_MSB (0x00000000, 0x00000000,
	          0x00000002, 0xfffffffd,
	          0x00000000, 0x00000006,
                  FALSE)

	/* MSB one small positive and one small negative number on a small
	   positive total */
	TEST_MSB (0x00000000, 0x00000006,
	          0x00000002, 0xfffffffd,
	          0x00000000, 0x0000000c,
                  FALSE)

	/* MSB one small positive and one small negative number on a moderate
	   positive total */
	TEST_MSB (0x00000000, 0xffffffff,
	          0x00000002, 0xfffffffd,
	          0x00000001, 0x00000005,
                  FALSE)

	/* MSB one small positive and one small negative number on a large
	   positive total */
	TEST_MSB (0x7fffffff, 0xfffffff9,
	          0x00000002, 0xfffffffd,
	          0x7fffffff, 0xffffffff,
                  FALSE)

	/* MSB one small positive and one small negative number on a small
	   negative total */
	TEST_MSB (0xffffffff, 0xfffffff9,
	          0x00000002, 0xfffffffd,
	          0xffffffff, 0xffffffff,
                  FALSE)

	/* MSB one small positive and one small negative number on a moderate
	   negative total */
	TEST_MSB (0xfffffffe, 0xffffffff,
	          0x00000002, 0xfffffffd,
	          0xffffffff, 0x00000005,
                  FALSE)

	/* MSB one small positive and one small negative number on a large
	   negative total */
	TEST_MSB (0x80000000, 0x00000000,
	          0x00000002, 0xfffffffd,
	          0x80000000, 0x00000006,
                  FALSE)

	PUTC ('\n')

	/* MSB one moderate positive and one moderate negative number on a
	   zero total */
	TEST_MSB (0x00000000, 0x00000000,
	          0x00008000, 0xffff0000,
	          0x00000000, 0x80000000,
                  FALSE)

	/* MSB one moderate positive and one moderate negative number on a
	   small positive total */
	TEST_MSB (0x00000000, 0x00000006,
	          0x00008000, 0xffff0000,
	          0x00000000, 0x80000006,
                  FALSE)

	/* MSB one moderate positive and one moderate negative number on a
	   moderate positive total */
	TEST_MSB (0x00000000, 0x80000000,
	          0x00008000, 0xffff0000,
	          0x00000001, 0x00000000,
                  FALSE)

	/* MSB one moderate positive and one moderate negative number on a
	   large positive total */
	TEST_MSB (0x7fffffff, 0x7fffffff,
	          0x00008000, 0xffff0000,
	          0x7fffffff, 0xffffffff,
                  FALSE)

	/* MSB one moderate positive and one moderate negative number on a
	   small negative total */
	TEST_MSB (0xffffffff, 0xffffffff,
	          0x00008000, 0xffff0000,
	          0x00000000, 0x7fffffff,
                  FALSE)

	/* MSB one moderate positive and one moderate negative number on a
	   moderate negative total */
	TEST_MSB (0xfffffffe, 0xffffffff,
	          0x00008000, 0xffff0000,
	          0xffffffff, 0x7fffffff,
                  FALSE)

	/* MSB one moderate positive and one moderate negative number on a
	   large negative total */
	TEST_MSB (0x80000000, 0x00000000,
	          0x00008000, 0xffff0000,
	          0x80000000, 0x80000000,
                  FALSE)

	PUTC ('\n')

        /* MSB 1 and 1 with largest negative total, should set overflow */
	TEST_MSB (0x80000000, 0x00000000,
	          0x00000001, 0x00000001,
	          0x7fffffff, 0xffffffff,
                  TRUE)

        /* MSB 1 and -1 with -1, should not set overflow */
	TEST_MSB (0xffffffff, 0xffffffff,
	          0xffffffff, 0xffffffff,
	          0xffffffff, 0xfffffffe,
                  FALSE)

        /* MSB large positive and large negative with large positive,
   	   should not set overflow */
	TEST_MSB (0x00000000, 0x00000000,
	          0xffffffff, 0xffffffff,
	          0xffffffff, 0xffffffff,
                  FALSE)

	PUTC ('\n')
        
        /* check if AECSRP is set */
        l.mfspr r3,r0,SPR_CPUCFGR
        LOAD_CONST(r2, SPR_CPUCFGR_AECSRP)
        l.and r3,r3,r2
        l.sfeq r3,r2
        OR1K_DELAYED_NOP(l.bf _msb_aecsr)

_msb_no_aecsr:
	LOAD_STR (r3, "  ** OVE flag set **\n")
	OR1K_DELAYED_NOP(l.jal	_puts)

	/* Check that range exceptions are triggered */
	l.mfspr	r3,r0,SPR_SR
	LOAD_CONST (r2, SPR_SR_OVE)	/* Set OVE */
	l.or	r3,r3,r2
	l.mtspr	r0,r3,SPR_SR

        /* MSB 1 and 1 with largest negative total, should raise exception */
	TEST_MSB (0x80000000, 0x00000000,
	          0x00000001, 0x00000001,
	          0x7fffffff, 0xffffffff,
                  TRUE)

        /* MSB 1 and -1 with -1, should not raise exception */
	TEST_MSB (0xffffffff, 0xffffffff,
	          0xffffffff, 0xffffffff,
	          0xffffffff, 0xfffffffe,
                  FALSE)

        /* MSB large positive and large negative with large positive,
   	   should not raise exception */
	TEST_MSB (0x00000000, 0x00000000,
	          0xffffffff, 0xffffffff,
	          0xffffffff, 0xffffffff,
                  FALSE)

        OR1K_DELAYED_NOP(l.j _msb_except_done)

_msb_aecsr:
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

        /* MSB 1 and 1 with largest negative total, should not raise exception */
	TEST_MSB (0x80000000, 0x00000000,
	          0x00000001, 0x00000001,
	          0x7fffffff, 0xffffffff,
                  TRUE)

        /* MSB 1 and -1 with -1, should not raise exception */
	TEST_MSB (0xffffffff, 0xffffffff,
	          0xffffffff, 0xffffffff,
	          0xffffffff, 0xfffffffe,
                  FALSE)

        /* MSB large positive and large negative with large positive,
   	   should not raise exception */
	TEST_MSB (0x00000000, 0x00000000,
	          0xffffffff, 0xffffffff,
	          0xffffffff, 0xffffffff,
                  FALSE)

        /* Clear OVE so following printout won't raise an exception */
        l.mfspr r3,r0,SPR_SR
        LOAD_CONST (r2, ~SPR_SR_OVE)
        l.and r3,r2,r3
        l.mtspr r0,r3,SPR_SR
        
	PUTC ('\n')
        
	LOAD_STR (r3, "  ** AECR set to OVMACADDE **\n")
	OR1K_DELAYED_NOP(l.jal	_puts)
	
        /* set AECR to OVMACADDE */
        LOAD_CONST(r2, SPR_AECR_OVMACADDE)
        l.mtspr r0,r2,SPR_AECR
        
        /* Set OVE again */
	l.mfspr	r3,r0,SPR_SR
	LOAD_CONST (r2, SPR_SR_OVE)	/* Set OVE */
	l.or	r3,r3,r2
	l.mtspr	r0,r3,SPR_SR
	
        /* set AECR to OVMACADDE */
        LOAD_CONST(r2, SPR_AECR_OVMACADDE)
        l.mtspr r0,r2,SPR_AECR


        /* MSB 1 and 1 with largest negative total, should raise exception */
	TEST_MSB (0x80000000, 0x00000000,
	          0x00000001, 0x00000001,
	          0x7fffffff, 0xffffffff,
                  TRUE)

        /* MSB 1 and -1 with -1, should not raise exception */
	TEST_MSB (0xffffffff, 0xffffffff,
	          0xffffffff, 0xffffffff,
	          0xffffffff, 0xfffffffe,
                  FALSE)

        /* MSB large positive and large negative with large positive,
   	   should not raise exception */
	TEST_MSB (0x00000000, 0x00000000,
	          0xffffffff, 0xffffffff,
	          0xffffffff, 0xffffffff,
                  FALSE)

        /* Clear OVE so following printout won't raise an exception */
        l.mfspr r3,r0,SPR_SR
        LOAD_CONST (r2, ~SPR_SR_OVE)
        l.and r3,r2,r3
        l.mtspr r0,r3,SPR_SR
        
	PUTC ('\n')
        
	LOAD_STR (r3, "  ** AECR set to CYMACADDE **\n")
	OR1K_DELAYED_NOP(l.jal	_puts)
        
        /* set AECR to CYMACADDE */
        LOAD_CONST(r2, SPR_AECR_CYMACADDE)
        l.mtspr r0,r2,SPR_AECR
        
        /* Set OVE again */
	l.mfspr	r3,r0,SPR_SR
	LOAD_CONST (r2, SPR_SR_OVE)	/* Set OVE */
	l.or	r3,r3,r2
	l.mtspr	r0,r3,SPR_SR
	
        /* set AECR to CYMACADDE */
        LOAD_CONST(r2, SPR_AECR_CYMACADDE)
        l.mtspr r0,r2,SPR_AECR

        /* MSB 1 and 1 with largest negative total, should not raise exception */
	TEST_MSB (0x80000000, 0x00000000,
	          0x00000001, 0x00000001,
	          0x7fffffff, 0xffffffff,
                  TRUE)

        /* MSB 1 and -1 with -1, should not raise exception */
	TEST_MSB (0xffffffff, 0xffffffff,
	          0xffffffff, 0xffffffff,
	          0xffffffff, 0xfffffffe,
                  FALSE)

        /* MSB large positive and large negative with large positive,
   	   should not raise exception */
	TEST_MSB (0x00000000, 0x00000000,
	          0xffffffff, 0xffffffff,
	          0xffffffff, 0xffffffff,
                  FALSE)

_msb_except_done:
	/* Finished checking range exceptions */
	l.mfspr	r3,r0,SPR_SR
	LOAD_CONST (r2, ~SPR_SR_OVE)	/* Clear OVE */
	l.and	r3,r3,r2
	l.mtspr	r0,r3,SPR_SR
	
	PUTC ('\n')
        
	LOAD_STR (r3, "  ** OVE flag cleared **\n")
	l.jal	_puts
	l.nop
        
	PUTC ('\n')

/* ----------------------------------------------------------------------------
 * All done
 * ------------------------------------------------------------------------- */
_exit:
	LOAD_STR (r3, "Test completed\n")
	l.jal	_puts
	l.nop

	TEST_EXIT
