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

#define MC_CSR          (0x00)
#define MC_POC          (0x04)
#define MC_BA_MASK      (0x08)
#define MC_CSC(i)       (0x10 + (i) * 8)
#define MC_TMS(i)       (0x14 + (i) * 8)


        .extern _main

        .global _ic_enable
        .global _ic_disable
        .global _dc_enable
        .global _dc_disable
        .global _dc_inv
        .global _ic_inv_test
        .global _dc_inv_test

        .section .stack
        .space 0x1000
_stack:

	.section .reset, "ax"

        .org    0x100
_reset_vector:
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
        l.jal   _init_mc
        l.nop

        l.movhi r1,hi(_stack)
        l.ori   r1,r1,lo(_stack)

        /* Copy data section */
        l.movhi r3,hi(_src_beg)
        l.ori   r3,r3,lo(_src_beg)
        l.movhi r4,hi(_dst_beg)
        l.ori   r4,r4,lo(_dst_beg)
        l.movhi r5,hi(_dst_end)
        l.ori   r5,r5,lo(_dst_end)
        l.sub   r5,r5,r4
        l.sfeqi r5,0
        l.bf    2f
        l.nop
1:      l.lwz   r6,0(r3)
        l.sw    0(r4),r6
        l.addi  r3,r3,4
        l.addi  r4,r4,4
        l.addi  r5,r5,-4
        l.sfgtsi r5,0
        l.bf   	1b
        l.nop
2:
        l.movhi r2,hi(_main)
        l.ori   r2,r2,lo(_main)
        l.jr    r2
        l.nop

_init_mc:
 
        l.movhi r3,hi(MC_BASE_ADDR)
        l.ori   r3,r3,lo(MC_BASE_ADDR)
 
        l.addi  r4,r3,MC_CSC(0)
        l.movhi r5,hi(FLASH_BASE_ADDR)
        l.srai  r5,r5,6
        l.ori   r5,r5,0x0025
        l.sw    0(r4),r5
 
        l.addi  r4,r3,MC_TMS(0)
        l.movhi r5,hi(FLASH_TMS_VAL)
        l.ori   r5,r5,lo(FLASH_TMS_VAL)
        l.sw    0(r4),r5
 
        l.addi  r4,r3,MC_BA_MASK
        l.addi  r5,r0,MC_MASK_VAL
        l.sw    0(r4),r5
 
        l.addi  r4,r3,MC_CSR
        l.movhi r5,hi(MC_CSR_VAL)
        l.ori   r5,r5,lo(MC_CSR_VAL)
        l.sw    0(r4),r5
 
        l.addi  r4,r3,MC_TMS(1)
        l.movhi r5,hi(SDRAM_TMS_VAL)
        l.ori   r5,r5,lo(SDRAM_TMS_VAL)
        l.sw    0(r4),r5
 
        l.addi  r4,r3,MC_CSC(1)
        l.movhi r5,hi(SDRAM_BASE_ADDR)
        l.srai  r5,r5,6
        l.ori   r5,r5,0x0411
        l.sw    0(r4),r5
 
        l.jr    r9
        l.nop


        .section .text

_ic_enable:
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

_ic_disable:
        /* Disable IC */
        l.mfspr r13,r0,SPR_SR
        l.addi  r11,r0,-1
        l.xori  r11,r11,SPR_SR_ICE
        l.and   r11,r13,r11
        l.mtspr r0,r11,SPR_SR

        l.jr    r9
        l.nop

_dc_enable:
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

_dc_disable:
        /* Disable DC */
        l.mfspr r13,r0,SPR_SR
        l.addi  r11,r0,-1
        l.xori  r11,r11,SPR_SR_DCE
        l.and   r11,r13,r11
        l.mtspr r0,r11,SPR_SR

        l.jr    r9
        l.nop

_dc_inv:
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
_ic_inv_test:
        l.movhi r7,hi(_ic_test_1)
        l.ori   r7,r7,lo(_ic_test_1)
        l.addi  r3,r0,0
        l.addi  r4,r0,0
        l.addi  r5,r0,0
        l.nop
        l.nop
        l.nop

_ic_test_1:
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

_dc_inv_test:
        l.movhi r4,hi(0x08040201)
        l.ori   r4,r4,lo(0x08040201)
        l.sw    0x00(r3),r4
        l.slli  r4,r4,1
        l.sw    0x14(r3),r4
        l.slli  r4,r4,1
        l.sw    0x28(r3),r4

        l.addi  r8,r9,0
        l.jal   _dc_enable
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
