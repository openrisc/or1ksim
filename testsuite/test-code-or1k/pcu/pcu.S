/* pcu.S -- Tests the performance counters unit

   Contributor Julius Baxter <julius@opencores.org>

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

/* Currently only checks instruction fetch, TLB miss and cache miss counters. */

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

	/* DTLB miss - just disable DMMU and return */
	/* clobber r13 */
	.org 0x900
	l.mfspr	r13,r0,SPR_ESR_BASE
	l.xori	r13,r13,SPR_SR_DME
	l.mtspr	r0,r13,SPR_ESR_BASE
	l.rfe

	/* ITLB miss - just disable IMMU and return */
	/* clobber r13 */
	.org 0xa00
	l.mfspr	r13,r0,SPR_ESR_BASE
	l.xori	r13,r13,SPR_SR_IME
	l.mtspr	r0,r13,SPR_ESR_BASE
	l.rfe
	
	.org 0xb00
	l.jal	unhandled_except
	l.nop

	/* System call - switches user/supervisor mode */
	/* Clobbers r13 */
	.org 0xc00
	l.mfspr	r13,r0,SPR_ESR_BASE
	l.xori	r13,r13,SPR_SR_SM	/* Toggle SM bit */
	l.mtspr	r0,r13,SPR_ESR_BASE
	l.rfe
	
	.org 0xd00
	l.jal	unhandled_except
	l.nop
	.org 0xe00
	l.jal	unhandled_except
	l.nop
	.org 0xf00
	l.jal	unhandled_except
	l.nop


	.section .text
start_test:
	/* Check PCU is present */
	l.movhi	r0,0
	l.mfspr	r2,r0,SPR_UPR
	l.andi	r2,r2,SPR_UPR_PCUP
	l.sfnei	r2,SPR_UPR_PCUP
	/* Fail if not present */
	l.bf	fail
	l.nop

	/* Assumption that we are already in SM */

	/* Set SUMRA bit in SR */
	l.movhi	r2,hi(SPR_SR_SUMRA)
	l.mfspr	r3,r0,SPR_SR
	l.or	r3,r3,r2
	l.mtspr	r0,r3,SPR_SR
	
	/* Check instruction fetch counter - supervisor mode */
	/* r1 - counter of which counter (0-7) we're using */
	l.movhi	r1,0
	/* use r5 to hold pcmr value */
	l.ori	r5,r0,(SPR_PCMR_CISM | SPR_PCMR_IF | SPR_PCMR_UMRA)
	/* report pcmr value */
	l.or	r3,r5,r5
	l.nop	NOP_REPORT
pcr_if_sm:
	/* report which counter we're using */
	l.or	r3,r1,r1
	l.nop	NOP_REPORT
	/* clear (0) pccr */
	l.mtspr	r1,r0,SPR_PCCR(0)
	/* Load pcmr with appropriate mode */
	l.mtspr	r1,r5,SPR_PCMR(0)
	/* This next instruction should be counted */
	l.nop
	/* Pull PCCR out and check it's != 0 */
	l.mfspr	r3,r1,SPR_PCCR(0)
	//l.nop	NOP_REPORT
	l.sfnei	r3,0
	l.bnf	fail

	/* Now change to usermode, ensure the counter doesn't increment */
	l.sys	0
	/* SM = 0 */
	l.mfspr	r6,r1,SPR_PCCR(0)
	l.nop
	l.nop
	l.nop
	l.mfspr	r7,r1,SPR_PCCR(0)
	/* r6 and r7 should be equal */
	l.sfne	r6,r7
	l.bf	fail
	l.sys	0
	/* SM = 1 */

	/* End of loop */
	l.sfeqi	r1,7		/* Finished checking all 8 regs? */
	l.bnf	pcr_if_sm	/* check next set */
	l.addi	r1,r1,1		/* increment set counter */
	
	
	/* Check instruction fetch counter - user mode */
	/* r1 - counter of which counter (0-7) we're using */
	l.movhi	r1,0
	/* use r5 to hold pcmr value */
	l.ori	r5,r0,(SPR_PCMR_CIUM | SPR_PCMR_IF | SPR_PCMR_UMRA)
	/* report pcmr value */
	l.or	r3,r5,r5
	l.nop	NOP_REPORT
pcr_if_um:
	/* report which counter we're using */
	l.or	r3,r1,r1
	l.nop	NOP_REPORT
	/* clear (0) pccr */
	l.mtspr	r1,r0,SPR_PCCR(0)
	/* Load pcmr with appropriate mode */
	l.mtspr	r1,r5,SPR_PCMR(0)
	/* Switch to user mode */
	l.sys 0
	/* SM = 0 */
	/* This next instruction should be counted */
	l.nop
	l.mfspr	r6,r1,SPR_PCCR(0)
	l.nop
	l.nop
	l.mfspr	r7,r1,SPR_PCCR(0)
	/* r6 should be smaller than r7 */
	l.sfgeu	r6,r7
	l.bf	fail

	/* Now change to supervisor, ensure the counter doesn't increment */
	l.sys	0
	/* SM = 1 */
	l.mfspr	r6,r1,SPR_PCCR(0)
	l.nop
	l.nop
	l.nop
	l.mfspr	r7,r1,SPR_PCCR(0)
	/* r6 and r7 should be equal */
	l.sfeq	r6,r7
	l.bnf	fail

	/* End of loop */
	l.sfeqi	r1,7		/* Finished checking all 8 regs? */
	l.bnf	pcr_if_um	/* check next set */
	l.addi	r1,r1,1		/* increment set counter */
	
	
	/* Check for MMU miss */
	/* immu */

	/* Clear MMU match registers, will cause a miss as soon as we turn on
	the MMU */
	l.movhi	r1,0	/* Set counter */
	l.movhi	r2,0	/* Way counter */
immu_clear_ways:
	l.slli	r3,r2,8	/* way * 0x100 */
immu_clear_sets:
	l.add	r4,r1,r3	/* set + way */
	/* clear immu entry */
	l.mtspr	r4,r0,SPR_ITLBMR_BASE(0)
	l.mtspr	r4,r0,SPR_ITLBTR_BASE(0)
	/* End of immu sets? */
	l.sfnei	r1,127
	l.bf	immu_clear_sets
	l.addi	r1,r1,1

	/* Reset set counter */
	l.movhi	r1,0
	
	/* end of immu ways? */
	l.sfnei	r2,3
	l.bf	immu_clear_ways
	l.addi	r2,r2,1

	/* Check itlb miss counter */
	/* r1 - counter of which counter (0-7) we're using */
	l.movhi	r1,0
	/* use r5 to hold pcmr value */
	l.ori	r5,r0,(SPR_PCMR_CISM | SPR_PCMR_ITLBM | SPR_PCMR_UMRA)
	/* report pcmr value */
	l.or	r3,r5,r5
	l.nop	NOP_REPORT
pcr_itlbm:
	/* report which counter we're using */
	l.or	r3,r1,r1
	l.nop	NOP_REPORT
	/* clear (0) pccr */
	l.mtspr	r1,r0,SPR_PCCR(0)
	/* Load pcmr with appropriate mode */
	l.mtspr	r1,r5,SPR_PCMR(0)
	
	/* Switch on IMMU - cause tlb miss*/
	l.mfspr	r4,r0,SPR_SR
	l.ori	r4,r4,SPR_SR_IME
	l.mtspr	r0,r4,SPR_SR
	/* Miss should occur here */

	/* Check PCCR incremented */
	l.mfspr	r6,r1,SPR_PCCR(0)
	l.sfeq	r6,r0
	l.bf	fail

	/* End of loop */
	l.sfeqi	r1,7		/* Finished checking all 8 regs? */
	l.bnf	pcr_itlbm	/* check next set */
	l.addi	r1,r1,1		/* increment set counter */

	/* Check for MMU miss */
	/* dmmu */

	/* Clear MMU match registers, will cause a miss as soon as we turn on
	the MMU */
	l.movhi	r1,0	/* Set counter */
	l.movhi	r2,0	/* Way counter */
dmmu_clear_ways:
	l.slli	r3,r2,8	/* way * 0x100 */
dmmu_clear_sets:
	l.add	r4,r1,r3	/* set + way */
	/* clear dmmu entry */
	l.mtspr	r4,r0,SPR_DTLBMR_BASE(0)
	l.mtspr	r4,r0,SPR_DTLBTR_BASE(0)
	/* End of dmmu sets? */
	l.sfnei	r1,127
	l.bf	dmmu_clear_sets
	l.addi	r1,r1,1

	/* Reset set counter */
	l.movhi	r1,0
	
	/* end of dmmu ways? */
	l.sfnei	r2,3
	l.bf	dmmu_clear_ways
	l.addi	r2,r2,1

	/* Check dtlb miss counter */
	/* r1 - counter of which counter (0-7) we're using */
	l.movhi	r1,0
	/* use r5 to hold pcmr value */
	l.ori	r5,r0,(SPR_PCMR_CISM | SPR_PCMR_DTLBM | SPR_PCMR_UMRA)
	/* report pcmr value */
	l.or	r3,r5,r5
	l.nop	NOP_REPORT
pcr_dtlbm:
	/* report which counter we're using */
	l.or	r3,r1,r1
	l.nop	NOP_REPORT
	/* clear (0) pccr */
	l.mtspr	r1,r0,SPR_PCCR(0)
	/* Load pcmr with appropriate mode */
	l.mtspr	r1,r5,SPR_PCMR(0)
	
	/* Switch on DMMU - cause tlb miss*/
	l.mfspr	r4,r0,SPR_SR
	l.ori	r4,r4,SPR_SR_DME
	l.mtspr	r0,r4,SPR_SR

	l.sw	0(r0),r6
	/* Miss should occur here */

	/* Check PCCR incremented */
	l.mfspr	r6,r1,SPR_PCCR(0)
	l.sfeq	r6,r0
	l.bf	fail

	/* End of loop */
	l.sfeqi	r1,7		/* Finished checking all 8 regs? */
	l.bnf	pcr_dtlbm	/* check next set */
	l.addi	r1,r1,1		/* increment set counter */


	/* Check instruction cache miss counter */
icache_miss_test:	
	/* Check if IC present and skip enabling otherwise */
	l.mfspr r3,r0,SPR_UPR
	l.andi  r4,r3,SPR_UPR_ICP
	l.sfeq  r4,r0
	l.bf    dcache_miss_test
	l.nop
		
	/* r1 - counter of which counter (0-7) we're using */
	l.movhi	r1,0


	/* use r5 to hold pcmr value */
	l.ori	r5,r0,(SPR_PCMR_CISM | SPR_PCMR_ICM | SPR_PCMR_UMRA)
	/* report pcmr value */
	l.or	r3,r5,r5
	l.nop	NOP_REPORT

pcr_icmiss:
	/* report which counter we're using */
	l.or	r3,r1,r1
	l.nop	NOP_REPORT
	/* clear (0) pccr */
	l.mtspr	r1,r0,SPR_PCCR(0)
	/* Load pcmr with appropriate mode */
	/* use r5 to hold pcmr value */
	l.ori	r5,r0,(SPR_PCMR_CISM | SPR_PCMR_ICM | SPR_PCMR_UMRA)
	l.mtspr	r1,r5,SPR_PCMR(0)

	/* Disable IC */
	l.mfspr r6,r0,SPR_SR
	l.addi  r5,r0,-1
	l.xori  r5,r5,SPR_SR_ICE
	l.and   r5,r6,r5
	l.mtspr r0,r5,SPR_SR
	
	/* Establish cache block size
	If BS=0, 16;
	If BS=1, 32;
	r14 contain block size
	*/
	l.mfspr r3,r0,SPR_ICCFGR
	l.andi  r4,r3,SPR_ICCFGR_CBS
	l.srli  r5,r4,7
	l.ori   r6,r0,16
	l.sll   r14,r6,r5
	
	/* Establish number of cache sets
	r7 contains number of cache sets
	r5 contains log(# of cache sets)
	*/
	l.andi  r4,r3,SPR_ICCFGR_NCS
	l.srli  r5,r4,3
	l.ori   r6,r0,1
	l.sll   r7,r6,r5
	
	/* Invalidate IC */
	l.addi  r6,r0,0
	l.sll   r5,r14,r5
	
.L7:
	l.mtspr r0,r6,SPR_ICBIR
	l.sfne  r6,r5
	l.bf    .L7
	l.add   r6,r6,r14
	
	/* Enable IC */
	l.mfspr r6,r0,SPR_SR
	l.ori   r6,r6,SPR_SR_ICE
	l.mtspr r0,r6,SPR_SR
	l.nop
	l.nop
	l.nop
	l.nop

	/* Should have had at least 1 miss, check this */
	l.mfspr	r6,r1,SPR_PCCR(0)
	l.sfeq	r6,r0
	l.bf	fail
	l.nop

	/* End of loop */
	l.sfeqi	r1,7		/* Finished checking all 8 regs? */
	l.bnf	pcr_icmiss	/* check next set */
	l.addi	r1,r1,1		/* increment set counter */



dcache_miss_test:
	
	/* Check if DC present and skip enabling otherwise */
        l.mfspr r3,r0,SPR_UPR
        l.andi  r4,r3,SPR_UPR_DCP
        l.sfeq  r4,r0
        l.bf    dcache_miss_test_done
        l.nop

	/* r1 - counter of which counter (0-7) we're using */
	l.movhi	r1,0

	/* use r5 to hold pcmr value */
	l.ori	r5,r0,(SPR_PCMR_CISM | SPR_PCMR_DCM | SPR_PCMR_UMRA)
	/* report pcmr value */
	l.or	r3,r5,r5
	l.nop	NOP_REPORT

pcr_dcmiss:
	/* report which counter we're using */
	l.or	r3,r1,r1
	l.nop	NOP_REPORT
	/* clear (0) pccr */
	l.mtspr	r1,r0,SPR_PCCR(0)
	/* Load pcmr with appropriate mode */
	/* use r5 to hold pcmr value */
	l.ori	r5,r0,(SPR_PCMR_CISM | SPR_PCMR_DCM | SPR_PCMR_UMRA)
	l.mtspr	r1,r5,SPR_PCMR(0)	

	        /* Disable DC */
        l.mfspr r6,r0,SPR_SR
        l.addi  r5,r0,-1
        l.xori  r5,r5,SPR_SR_DCE
	l.and   r5,r6,r5
        l.mtspr r0,r5,SPR_SR
        /* Establish cache block size
           If BS=0, 16;
           If BS=1, 32;
           r14 contain block size
        */
        l.mfspr r3,r0,SPR_DCCFGR
        l.andi  r4,r3,SPR_DCCFGR_CBS
        l.srli  r5,r4,7
        l.ori   r6,r0,16
        l.sll   r14,r6,r5
        /* Establish number of cache sets
           r7 contains number of cache sets
           r5 contains log(# of cache sets)
        */
	l.andi  r4,r3,SPR_DCCFGR_NCS
	l.srli  r5,r4,3
        l.ori   r6,r0,1
        l.sll   r7,r6,r5
        /* Invalidate DC */
        l.addi  r6,r0,0
        l.sll   r5,r14,r5
.L9:
        l.mtspr r0,r6,SPR_DCBIR
        l.sfne  r6,r5
        l.bf    .L9
	l.add   r6,r6,r14
        /* Enable DC */
        l.mfspr r6,r0,SPR_SR
        l.ori   r6,r6,SPR_SR_DCE
        l.mtspr r0,r6,SPR_SR

	/* Should trigger a dcache miss */
	l.sw	0(r0),r6
	/* Should trigger another dcache miss */
	l.lwz	r6,0x1000(r0)

	/* Should have had 2 misses */
	l.mfspr	r6,r1,SPR_PCCR(0)
	l.sfnei	r6,2
	l.bf	fail
	l.nop

	/* End of loop */
	l.sfeqi	r1,7		/* Finished checking all 8 regs? */
	l.bnf	pcr_dcmiss	/* check next set */
	l.addi	r1,r1,1		/* increment set counter */

dcache_miss_test_done:	
	
finish_ok:	
	l.movhi	r3,0xdead
	l.ori	r3,r3,0xdead
	l.nop	NOP_REPORT
	l.ori	r3,r0,0
	l.nop	NOP_EXIT

fail:
	l.ori	r3,r0,0x1234
	l.nop	NOP_EXIT

unhandled_except:
	l.addi	r3,r9,-8
	l.nop	NOP_REPORT
	l.nop	NOP_EXIT

