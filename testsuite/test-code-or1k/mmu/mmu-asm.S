/* mmu-asm.S. Machine code to support Or1ksim MMU test

   Copyright (C) 1999-2006 OpenCores
   Copyright (C) 2010 Embecosm Limited

   Contributors various OpenCores participants
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
#include "or1k-asm.h"
#include "spr-defs.h"

#define PAGE_SIZE 8192
#define DTLB_PR_NOLIMIT  (SPR_DTLBTR_URE  | \
                          SPR_DTLBTR_UWE  | \
                          SPR_DTLBTR_SRE  | \
                          SPR_DTLBTR_SWE  )
#define ITLB_PR_NOLIMIT  (SPR_ITLBTR_SXE  | \
                          SPR_ITLBTR_UXE  )
        .global lo_dmmu_en
        .global lo_immu_en
        .global lo_dtlb_ci_test
        .global lo_itlb_ci_test
        .global testjump
        .global ic_enable
        .global ic_disable
        .global dc_enable
        .global dc_disable

lo_dmmu_en:
	l.mfspr r11,r0,SPR_SR
        l.ori   r11,r11,SPR_SR_DME
        l.mtspr r0,r11,SPR_ESR_BASE
        l.mtspr r0,r9,SPR_EPCR_BASE
        l.rfe
  
lo_dmmu_dis:
        l.addi  r13,r0,-1
        l.xori  r13,r13,SPR_SR_DME
        l.mfspr r11,r0,SPR_SR
        l.and   r11,r11,r13
        l.mtspr r0,r11,SPR_SR
        OR1K_DELAYED_NOP(OR1K_INST(l.jr    r9))

lo_immu_en:
	l.mfspr r3,r0,SPR_SR
        l.ori   r3,r3,SPR_SR_IME
        l.mtspr r0,r3,SPR_ESR_BASE
        l.mtspr r0,r9,SPR_EPCR_BASE
        l.rfe
 
lo_immu_dis:
        l.addi  r13,r0,-1
        l.xori  r13,r13,SPR_SR_IME
        l.mfspr r11,r0,SPR_SR
        l.and   r11,r11,r13
        l.mtspr r0,r11,SPR_SR
        OR1K_DELAYED_NOP(OR1K_INST(l.jr    r9))

testjump:
        l.movhi r5,0x4800
        l.ori   r5,r5,0x4800
        l.sw    0x0(r3),r5
        l.movhi r5,0x1500
        l.ori   r5,r5,0x0000
        l.sw    0x4(r3),r5
        l.or    r5,r0,r9
        OR1K_DELAYED_NOP(OR1K_INST(l.jalr  r4))
        l.or    r9,r0,r5
        OR1K_DELAYED_NOP(OR1K_INST(l.jr    r9))

ic_enable:
        /* Disable IC */
        l.mfspr r13,r0,SPR_SR
        l.addi  r11,r0,-1
        l.xori  r11,r11,SPR_SR_ICE
        l.and   r11,r13,r11
        l.mtspr r0,r11,SPR_SR
 
        /* Invalidate IC */
        l.addi  r13,r0,0
        l.addi  r11,r0,8192
1:
        l.mtspr r0,r13,SPR_ICBIR
        l.sfne  r13,r11
        OR1K_DELAYED(
        OR1K_INST(l.addi  r13,r13,16),
        OR1K_INST(l.bf    1b)
        )
 
        /* Enable IC */
        l.mfspr r13,r0,SPR_SR
        l.ori   r13,r13,SPR_SR_ICE
        l.mtspr r0,r13,SPR_SR
        l.nop
        l.nop
        l.nop
        l.nop
        l.nop
 
        OR1K_DELAYED_NOP(OR1K_INST(l.jr    r9))
 
ic_disable:
        /* Disable IC */
        l.mfspr r13,r0,SPR_SR
        l.addi  r11,r0,-1
        l.xori  r11,r11,SPR_SR_ICE
        l.and   r11,r13,r11
        l.mtspr r0,r11,SPR_SR
 
        OR1K_DELAYED_NOP(OR1K_INST(l.jr    r9))
 
dc_enable:
        /* Disable DC */
        l.mfspr r13,r0,SPR_SR
        l.addi  r11,r0,-1
        l.xori  r11,r11,SPR_SR_DCE
        l.and   r11,r13,r11
        l.mtspr r0,r11,SPR_SR
 
        /* Flush DC */
        l.addi  r13,r0,0
        l.addi  r11,r0,8192
1:
        l.mtspr r0,r13,SPR_DCBIR
        l.sfne  r13,r11
        OR1K_DELAYED(
        OR1K_INST(l.addi  r13,r13,16),
        OR1K_INST(l.bf    1b)
        )
 
        /* Enable DC */
        l.mfspr r13,r0,SPR_SR
        l.ori   r13,r13,SPR_SR_DCE
        l.mtspr r0,r13,SPR_SR
 
        OR1K_DELAYED_NOP(OR1K_INST(l.jr    r9))

dc_disable:
        /* Disable DC */
        l.mfspr r13,r0,SPR_SR
        l.addi  r11,r0,-1
        l.xori  r11,r11,SPR_SR_DCE
        l.and   r11,r13,r11
        l.mtspr r0,r11,SPR_SR
 
        OR1K_DELAYED_NOP(OR1K_INST(l.jr    r9))

        /* dtlb_ic_test(unsigned long add, unsigned long set) */
lo_dtlb_ci_test:
        l.addi  r1,r1,-4
        l.sw    0(r1),r9

        l.addi  r8,r0,0

        l.movhi r5,hi(0x01234567)
        l.ori   r5,r5,lo(0x01234567)
        l.sw    0(r3),r5
        l.movhi r5,hi(0x89abcdef)
        l.ori   r5,r5,lo(0x89abcdef)
        l.sw    (PAGE_SIZE - 4)(r3),r5

        l.ori   r5,r3,SPR_DTLBMR_V
        l.mtspr r4,r5,SPR_DTLBMR_BASE(0)

        l.ori   r5,r3,(DTLB_PR_NOLIMIT  | SPR_DTLBTR_CI)
        l.mtspr r4,r5,SPR_DTLBTR_BASE(0)

        l.addi  r5,r3,PAGE_SIZE
        l.ori   r5,r5,SPR_DTLBMR_V
        l.addi  r6,r4,1
        l.mtspr r6,r5,SPR_DTLBMR_BASE(0)

        l.addi  r5,r3,PAGE_SIZE
        l.ori   r5,r5,(DTLB_PR_NOLIMIT  | SPR_DTLBTR_CI)
        l.addi  r6,r4,1
        l.mtspr r6,r5,SPR_DTLBTR_BASE(0)

        OR1K_DELAYED_NOP(OR1K_INST(l.jal   lo_dmmu_en))
        OR1K_DELAYED_NOP(OR1K_INST(l.jal   dc_enable))

        l.movhi r6,hi(0x01234567)
        l.ori   r6,r6,lo(0x01234567)
        l.lwz   r5,0(r3)
        l.sfeq  r6,r5
        OR1K_DELAYED_NOP(OR1K_INST(l.bnf   11f))
        l.movhi r6,hi(0x89abcdef)
        l.ori   r6,r6,lo(0x89abcdef)
        l.lwz   r5,(PAGE_SIZE - 4)(r3)
        l.sfeq  r6,r5
        OR1K_DELAYED_NOP(OR1K_INST(l.bnf   12f))

        l.movhi r5,hi(0x76543210)
        l.ori   r5,r5,lo(0x76543210)
        l.sw    0(r3),r5
        l.movhi r5,hi(0xfedcba9)
        l.ori   r5,r5,lo(0xfedcba9)
        l.sw    (PAGE_SIZE - 4)(r3),r5

        OR1K_DELAYED_NOP(OR1K_INST(l.jal   lo_dmmu_dis))
        l.ori   r5,r3,(DTLB_PR_NOLIMIT)
        l.mtspr r4,r5,SPR_DTLBTR_BASE(0)
        OR1K_DELAYED_NOP(OR1K_INST(l.jal   lo_dmmu_en))

        l.movhi r6,hi(0x76543210)
        l.ori   r6,r6,lo(0x76543210)
        l.lwz   r5,0(r3)
        l.sfeq  r6,r5
        OR1K_DELAYED_NOP(OR1K_INST(l.bnf   13f))
        l.movhi r6,hi(0xfedcba9)
        l.ori   r6,r6,lo(0xfedcba9)
        l.lwz   r5,(PAGE_SIZE - 4)(r3)
        l.sfeq  r6,r5
        OR1K_DELAYED_NOP(OR1K_INST(l.bnf   14f))

        OR1K_DELAYED_NOP(OR1K_INST(l.jal   lo_dmmu_dis))
        l.ori   r5,r3,(DTLB_PR_NOLIMIT | SPR_DTLBTR_CI)
        l.mtspr r4,r5,SPR_DTLBTR_BASE(0)
        OR1K_DELAYED_NOP(OR1K_INST(l.jal   lo_dmmu_en))

        l.movhi r5,hi(0x00112233)
        l.ori   r5,r5,lo(0x00112233)
        l.sw    0(r3),r5
#if 1
        l.movhi r5,hi(0x44556677)
        l.ori   r5,r5,lo(0x44556677)
        l.sw    4(r3),r5
        l.movhi r5,hi(0x8899aabb)
        l.ori   r5,r5,lo(0x8899aabb)
        l.sw    8(r3),r5
        l.movhi r5,hi(0xccddeeff)
        l.ori   r5,r5,lo(0xccddeeff)
        l.sw    12(r3),r5
#endif
        l.movhi r5,hi(0x44556677)
        l.ori   r5,r5,lo(0x44556677)
        l.sw    (PAGE_SIZE - 4)(r3),r5

        l.movhi r6,hi(0x00112233)
        l.ori   r6,r6,lo(0x00112233)
        l.lwz   r5,0(r3)
        l.sfeq  r6,r5
        OR1K_DELAYED_NOP(OR1K_INST(l.bnf   15f))
        l.movhi r6,hi(0x44556677)
        l.ori   r6,r6,lo(0x44556677)
        l.lwz   r5,(PAGE_SIZE - 4)(r3)
        l.sfeq  r6,r5
        OR1K_DELAYED_NOP(OR1K_INST(l.bnf   16f))

        OR1K_DELAYED_NOP(OR1K_INST(l.jal   lo_dmmu_dis))
        l.ori   r5,r3,(DTLB_PR_NOLIMIT)
        l.mtspr r4,r5,SPR_DTLBTR_BASE(0)
        OR1K_DELAYED_NOP(OR1K_INST(l.jal   lo_dmmu_en))

        l.movhi r6,hi(0x76543210)
        l.ori   r6,r6,lo(0x76543210)
        l.lwz   r5,0(r3)
        l.sfeq  r6,r5
        OR1K_DELAYED_NOP(OR1K_INST(l.bnf   17f))

        l.movhi r6,hi(0xfedcba9)
        l.ori   r6,r6,lo(0xfedcba9)
        l.lwz   r5,(PAGE_SIZE - 4)(r3)
        l.sfeq  r6,r5
        OR1K_DELAYED_NOP(OR1K_INST(l.bnf   18f))

        /* Invalidate cache */
        OR1K_DELAYED_NOP(OR1K_INST(l.jal   dc_disable))

        l.movhi r5,hi(0x00112233)
        l.ori   r5,r5,lo(0x00112233)
        l.sw    12(r3),r5
        l.movhi r5,hi(0x44556677)
        l.ori   r5,r5,lo(0x44556677)
        l.sw    8(r3),r5
        l.movhi r5,hi(0x8899aabb)
        l.ori   r5,r5,lo(0x8899aabb)
        l.sw    4(r3),r5
        l.movhi r5,hi(0xccddeeff)
        l.ori   r5,r5,lo(0xccddeeff)
        l.sw    0(r3),r5
        l.movhi r5,hi(0x44556677)
        l.ori   r5,r5,lo(0x44556677)
        l.sw    (PAGE_SIZE - 4)(r3),r5

        OR1K_DELAYED_NOP(OR1K_INST(l.jal   dc_enable))

        /* I want this part to execute as fast as possible */
        OR1K_DELAYED_NOP(OR1K_INST(l.jal   ic_enable))

        l.addi  r5,r3,PAGE_SIZE

        /* This jump is just to be shure that the following 
           instructions will get into IC */
        OR1K_DELAYED_NOP(OR1K_INST(l.j     1f))
        /* This shuld trigger cahe line refill */
2:      l.lwz   r6,0(r3)
        OR1K_DELAYED(
        OR1K_INST(l.lwz   r6,0(r5)),
        OR1K_INST(l.j     2f)
        )
        /* This load is from non cached area and may cause some problems
           in previuos refill, which is probably still in progress */
1:      OR1K_DELAYED_NOP(OR1K_INST(l.j     2b))
2:
        /* Check the line that was previosly refilled */
        l.movhi r6,hi(0x00112233)
        l.ori   r6,r6,lo(0x00112233)
        l.lwz   r5,12(r3)
        l.sfeq  r6,r5
        OR1K_DELAYED_NOP(OR1K_INST(l.bnf   19f))
        l.movhi r6,hi(0x44556677)
        l.ori   r6,r6,lo(0x44556677)
        l.lwz   r5,8(r3)
        l.sfeq  r6,r5
        OR1K_DELAYED_NOP(OR1K_INST(l.bnf   19f))
        l.movhi r6,hi(0x8899aabb)
        l.ori   r6,r6,lo(0x8899aabb)
        l.lwz   r5,4(r3)
        l.sfeq  r6,r5
        OR1K_DELAYED_NOP(OR1K_INST(l.bnf   19f))
        l.movhi r6,hi(0xccddeeff)
        l.ori   r6,r6,lo(0xccddeeff)
        l.lwz   r5,0(r3)
        l.sfeq  r6,r5
        OR1K_DELAYED_NOP(OR1K_INST(l.bnf   19f))

        OR1K_DELAYED_NOP(OR1K_INST(l.jal   dc_disable))

        OR1K_DELAYED_NOP(OR1K_INST(l.jal   lo_dmmu_dis))

        OR1K_DELAYED_NOP(OR1K_INST(l.j     10f))

19:     l.addi  r8,r8,1
18:     l.addi  r8,r8,1
17:     l.addi  r8,r8,1
16:     l.addi  r8,r8,1
15:     l.addi  r8,r8,1
14:     l.addi  r8,r8,1
13:     l.addi  r8,r8,1
12:     l.addi  r8,r8,1
11:     l.addi  r8,r8,1

10:     OR1K_DELAYED_NOP(OR1K_INST(l.jal   dc_disable))

        OR1K_DELAYED_NOP(OR1K_INST(l.jal   ic_disable))

        OR1K_DELAYED_NOP(OR1K_INST(l.jal   lo_dmmu_dis))

        l.addi  r11,r8,0
        l.sw    0(r0),r8
        l.sw    4(r0),r5
        
        l.lwz   r9,0(r1)
        OR1K_DELAYED(
        OR1K_INST(l.addi  r1,r1,4),
        OR1K_INST(l.jr    r9)
        )

        /* itlb_ic_test(unsigned long add, unsigned long set) */
lo_itlb_ci_test:
        l.addi  r1,r1,-4
        l.sw    0(r1),r9

        l.addi  r8,r0,0

        /* Copy the code to the prepeared location */
        l.addi  r7,r0,88
        l.movhi r5,hi(ci_test)
        l.ori   r5,r5,lo(ci_test)
        l.addi  r6,r3,0
1:      l.lwz   r11,0(r5)
        l.sw    0(r6),r11
        l.addi  r5,r5,4
        l.addi  r6,r6,4
        l.addi  r7,r7,-4
        l.sfeqi r7,0
        OR1K_DELAYED_NOP(OR1K_INST(l.bnf   1b))

        l.ori   r5,r3,SPR_ITLBMR_V
        l.mtspr r4,r5,SPR_ITLBMR_BASE(0)

        l.ori   r5,r3,ITLB_PR_NOLIMIT 
        l.mtspr r4,r5,SPR_ITLBTR_BASE(0)

        OR1K_DELAYED_NOP(OR1K_INST(l.jal   lo_immu_en))
        OR1K_DELAYED_NOP(OR1K_INST(l.jal   ic_enable))

        l.addi  r5,r0,0
        l.addi  r6,r0,0
        OR1K_DELAYED_NOP(OR1K_INST(l.jalr  r3))

        l.sfeqi r5,5
        OR1K_DELAYED_NOP(OR1K_INST(l.bnf   11f))

        /* Copy the code to the prepeared location */
        l.addi  r7,r0,20
        l.movhi r5,hi(ic_refill_test)
        l.ori   r5,r5,lo(ic_refill_test)
        l.addi  r6,r3,12
1:      l.lwz   r11,0(r5)
        l.sw    0(r6),r11
        l.addi  r5,r5,4
        l.addi  r6,r6,4
        l.addi  r7,r7,-4
        l.sfeqi r7,0
        OR1K_DELAYED_NOP(OR1K_INST(l.bnf   1b))

        OR1K_DELAYED_NOP(OR1K_INST(l.jal   ic_disable))
        OR1K_DELAYED_NOP(OR1K_INST(l.jal   ic_enable))

        l.addi  r5,r0,0
        l.addi  r6,r3,12
        OR1K_DELAYED_NOP(OR1K_INST(l.jalr  r6))
        l.addi  r6,r3,16
        OR1K_DELAYED_NOP(OR1K_INST(l.jalr  r6))
        
        l.sfeqi r5,4
        OR1K_DELAYED_NOP(OR1K_INST(l.bnf   12f))

        OR1K_DELAYED_NOP(OR1K_INST(l.j     10f))

12:     l.addi  r8,r8,1
11:     l.addi  r8,r8,1

10:     OR1K_DELAYED_NOP(OR1K_INST(l.jal   ic_disable))

        OR1K_DELAYED_NOP(OR1K_INST(l.jal   lo_dmmu_dis))

        l.addi  r11,r8,0
        l.sw    0(r0),r11
        l.sw    4(r0),r5

        l.lwz   r9,0(r1)
        OR1K_DELAYED(
        OR1K_INST(l.jr    r9),
        OR1K_INST(l.addi  r1,r1,4)
        )
 
ci_test:
3:      l.addi  r5,r5,1

        l.sfeqi r6,0x01
        OR1K_DELAYED_NOP(OR1K_INST(l.bnf   1f))

        l.addi  r13,r0,-1
        l.xori  r13,r13,SPR_SR_IME
        l.mfspr r11,r0,SPR_SR
        l.and   r13,r11,r13
        l.mtspr r0,r13,SPR_SR

        l.ori   r7,r3,(ITLB_PR_NOLIMIT  | SPR_ITLBTR_CI)
        l.mtspr r4,r7,SPR_ITLBTR_BASE(0)

        l.mtspr r0,r11,SPR_SR

1:      l.lwz   r7,0(r3)
        l.addi  r7,r7,1
        l.sw    0(r3),r7

2:      l.addi  r6,r6,1
        l.sfeqi r6,3
        OR1K_DELAYED_NOP(OR1K_INST(l.bnf   3b))

        OR1K_DELAYED_NOP(OR1K_INST(l.jr    r9))

       
ic_refill_test:
        OR1K_DELAYED(
        OR1K_INST(l.addi  r5,r5,1),
        OR1K_INST(l.jr    r9)
        )
        l.addi  r5,r5,1
        OR1K_DELAYED(
        OR1K_INST(l.addi  r5,r5,1),
        OR1K_INST(l.jr    r9)
        )
