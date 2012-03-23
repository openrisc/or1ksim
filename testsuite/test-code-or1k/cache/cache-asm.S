/* cache_asm.S. Machine code support for cache test of Or1ksim

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

#include "spr-defs.h"
#include "board.h"

#define IC_ENABLE 0
#define DC_ENABLE 0

        .extern main

        .global ic_enable
        .global ic_disable
        .global dc_enable
        .global dc_disable
        .global dc_inv
        .global ic_inv_test
        .global dc_inv_test

        .section .stack
        .space 0x1000
stack:

	.section .reset, "ax"

        .org    0x100
reset_vector:
        // Clear R0 on start-up. There is no guarantee that R0 is hardwired to zero,
        // and indeed it is not when simulating the or1200 Verilog core.
        l.andi  r0,r0,0x0
        
        l.addi  r2,r0,0x0
        l.addi  r3,r0,0x0
        l.addi  r4,r0,0x0
        l.addi  r5,r0,0x0
        l.addi  r6,r0,0x0
        l.addi  r7,r0,0x0
        l.addi  r8,r0,0x0
        l.addi  r9,r0,0x0
        l.addi  r10,r0,0x0
        l.addi  r11,r0,0x0
        l.addi  r12,r0,0x0
        l.addi  r13,r0,0x0
        l.addi  r14,r0,0x0
        l.addi  r15,r0,0x0
        l.addi  r16,r0,0x0
        l.addi  r17,r0,0x0
        l.addi  r18,r0,0x0
        l.addi  r19,r0,0x0
        l.addi  r20,r0,0x0
        l.addi  r21,r0,0x0
        l.addi  r22,r0,0x0
        l.addi  r23,r0,0x0
        l.addi  r24,r0,0x0
        l.addi  r25,r0,0x0
        l.addi  r26,r0,0x0
        l.addi  r27,r0,0x0
        l.addi  r28,r0,0x0
        l.addi  r29,r0,0x0
        l.addi  r30,r0,0x0
        l.addi  r31,r0,0x0

        l.movhi r3,hi(start)
        l.ori   r3,r3,lo(start)
        l.jr    r3
        l.nop
start:

        l.movhi r1,hi(stack)
        l.ori   r1,r1,lo(stack)
	l.ori	r2,r1, 0
	
        l.movhi r3,hi(main)
        l.ori   r3,r3,lo(main)
        l.jr    r3
        l.nop

        .section .text

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
        l.bf    1b
        l.addi  r13,r13,16

        /* Enable IC */
        l.mfspr r13,r0,SPR_SR
        l.ori   r13,r13,SPR_SR_ICE
        l.mtspr r0,r13,SPR_SR
        l.nop
        l.nop
        l.nop
        l.nop
        l.nop

        l.jr    r9
        l.nop

ic_disable:
        /* Disable IC */
        l.mfspr r13,r0,SPR_SR
        l.addi  r11,r0,-1
        l.xori  r11,r11,SPR_SR_ICE
        l.and   r11,r13,r11
        l.mtspr r0,r11,SPR_SR

        l.jr    r9
        l.nop

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
        l.bf    1b
        l.addi  r13,r13,16

        /* Enable DC */
        l.mfspr r13,r0,SPR_SR
        l.ori   r13,r13,SPR_SR_DCE
        l.mtspr r0,r13,SPR_SR

        l.jr    r9
        l.nop

dc_disable:
        /* Disable DC */
        l.mfspr r13,r0,SPR_SR
        l.addi  r11,r0,-1
        l.xori  r11,r11,SPR_SR_DCE
        l.and   r11,r13,r11
        l.mtspr r0,r11,SPR_SR

        l.jr    r9
        l.nop

dc_inv:
        l.mfspr r4,r0,SPR_SR
        l.addi  r5,r0,-1
        l.xori  r5,r5,SPR_SR_DCE
        l.and   r5,r4,r5
        l.mtspr r0,r5,SPR_SR
        l.mtspr r0,r3,SPR_DCBIR        
        l.mtspr r0,r4,SPR_SR
        l.jr    r9
        l.nop

        .align  0x10
ic_inv_test:
        l.movhi r7,hi(ic_test_1)
        l.ori   r7,r7,lo(ic_test_1)
        l.addi  r3,r0,0
        l.addi  r4,r0,0
        l.addi  r5,r0,0
        l.nop
        l.nop
        l.nop

ic_test_1:
3:      l.addi  r3,r3,1

        l.sfeqi r4,0x01
        l.bnf   1f
        l.nop

        l.mfspr r8,r0,SPR_SR
        l.addi  r11,r0,-1
        l.xori  r11,r11,SPR_SR_ICE
        l.and   r11,r8,r11
        l.mtspr r0,r11,SPR_SR
        l.mtspr r0,r7,SPR_ICBIR
        l.mtspr r0,r8,SPR_SR
        l.bf    2f
        l.nop

1:      l.lwz   r6,0(r7)
        l.addi  r6,r6,1
        l.sw    0(r7),r6

2:      l.addi  r5,r5,1
        l.sfeqi r5,10
        l.bnf   3b
        l.xori  r4,r4,0x01
        
        l.addi  r11,r3,0
        l.jr    r9
        l.nop

dc_inv_test:
        l.movhi r4,hi(0x08040201)
        l.ori   r4,r4,lo(0x08040201)
        l.sw    0x00(r3),r4
        l.slli  r4,r4,1
        l.sw    0x14(r3),r4
        l.slli  r4,r4,1
        l.sw    0x28(r3),r4

        l.addi  r8,r9,0
        l.jal   dc_enable
        l.nop
        l.addi  r9,r8,0

        l.lbz   r4,0x03(r3)
        l.lhz   r5,0x16(r3)
        l.add   r4,r4,r5
        l.lwz   r5,0x28(r3)
        l.add   r4,r4,r5

        l.mfspr r6,r0,SPR_SR
        l.addi  r5,r0,-1
        l.xori  r5,r5,SPR_SR_DCE
        l.and   r5,r6,r5
        l.mtspr r0,r5,SPR_SR
        l.addi  r7,r3,0x10
        l.mtspr r0,r7,SPR_DCBIR

        l.lwz   r5,0(r3)
        l.slli  r5,r5,3
        l.sw    0x00(r3),r5
        l.slli  r5,r5,1
        l.sw    0x14(r3),r5
        l.slli  r5,r5,1
        l.sw    0x28(r3),r5

        l.mtspr r0,r6,SPR_SR

        l.lbz   r5,0x03(r3)
        l.add   r4,r4,r5
        l.lhz   r5,0x16(r3)
        l.add   r4,r4,r5
        l.lwz   r5,0x28(r3)
        l.add   r4,r4,r5

        l.addi  r5,r0,-1
        l.xori  r5,r5,SPR_SR_DCE
        l.and   r5,r6,r5
        l.mtspr r0,r5,SPR_SR

        l.addi  r11,r4,0x0
1:      
        l.jr  r9
        l.nop
