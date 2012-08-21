/* except-mc.s. Exception handling support for Or1k memory controller tests

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

/* Support file for c based tests */
#include "spr-defs.h"

#define reset reset

        .section .stack
        .space 0x1000
stack:

        .extern reset_support
        .extern c_reset
        .extern excpt_buserr
        .extern excpt_dpfault
        .extern excpt_ipfault
        .extern excpt_tick
        .extern excpt_align
        .extern excpt_illinsn
        .extern excpt_int
        .extern excpt_dtlbmiss
        .extern excpt_itlbmiss
        .extern excpt_range
        .extern excpt_syscall
        .extern excpt_break
        .extern excpt_trap

	.section .except,"ax"
	.org	0x100
reset_vector:
        l.nop
        l.nop
        
        // Clear R0 on start-up. There is no guarantee that R0 is hardwired to zero,
        // and indeed it is not when simulating the or1200 Verilog core.
        l.andi  r0,r0,0x0
        
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

        l.j     init_mc
        l.nop

start:  l.movhi r1,hi(stack)
        l.ori   r1,r1,lo(stack)

        /* Check if this is RTL version */
        l.lbz   r3,0(r0)
        l.sfeqi r3,0xff
        l.bf    2f
        l.nop
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
        l.movhi r2,hi(reset)
        l.ori   r2,r2,lo(reset)
        l.jr    r2
        l.nop

        .org    0x200
buserr_vector:
        l.addi  r1,r1,-116
        l.sw    0x18(r1),r9
        l.jal   store_regs
        l.nop
        l.movhi r9,hi(end_except)
        l.ori   r9,r9,lo(end_except)
        l.movhi r10,hi(excpt_buserr)
        l.ori   r10,r10,lo(excpt_buserr)
        l.lwz   r10,0x0(r10)
        l.jr    r10
        l.nop

        .org    0x300
dpfault_vector:
        l.addi  r1,r1,-116
        l.sw    0x18(r1),r9
        l.jal   store_regs
        l.nop
 
        l.mfspr r3,r0,SPR_EPCR_BASE
        l.addi  r3,r3,-4
        l.mtspr r0,r3,SPR_EPCR_BASE

        l.movhi r9,hi(end_except)
        l.ori   r9,r9,lo(end_except)
        l.movhi r10,hi(excpt_dpfault)
        l.ori   r10,r10,lo(excpt_dpfault)
        l.lwz   r10,0(r10)
        l.jr    r10
        l.nop

        .org    0x400
ipfault_vector:
        l.addi  r1,r1,-116
        l.sw    0x18(r1),r9
        l.jal   store_regs
        l.nop
        l.movhi r9,hi(end_except)
        l.ori   r9,r9,lo(end_except)
        l.movhi r10,hi(excpt_ipfault)
        l.ori   r10,r10,lo(excpt_ipfault)
        l.lwz   r10,0(r10)
        l.jr    r10
        l.nop

        .org    0x500
lpint_vector:
        l.addi  r1,r1,-116
        l.sw    0x18(r1),r9
        l.jal   store_regs
        l.nop
        l.movhi r9,hi(end_except)
        l.ori   r9,r9,lo(end_except)
        l.movhi r10,hi(excpt_tick)
        l.ori   r10,r10,lo(excpt_tick)
        l.lwz   r10,0(r10)
        l.jr    r10
        l.nop

        .org    0x600
align_vector:
        l.addi  r1,r1,-116
        l.sw    0x18(r1),r9
        l.jal   store_regs
        l.nop
        l.movhi r9,hi(end_except)
        l.ori   r9,r9,lo(end_except)
        l.movhi r10,hi(excpt_align)
        l.ori   r10,r10,lo(excpt_align)
        l.lwz   r10,0(r10)
        l.jr    r10
        l.nop

        .org    0x700
illinsn_vector:
        l.addi  r1,r1,-116
        l.sw    0x18(r1),r9
        l.jal   store_regs
        l.nop
        l.movhi r9,hi(end_except)
        l.ori   r9,r9,lo(end_except)
        l.movhi r10,hi(excpt_illinsn)
        l.ori   r10,r10,lo(excpt_illinsn)
        l.lwz   r10,0(r10)
        l.jr    r10
        l.nop

        .org    0x800
hpint_vector:
        l.addi  r1,r1,-116
        l.sw    0x18(r1),r9
        l.jal   store_regs
        l.nop
        l.movhi r9,hi(end_except)
        l.ori   r9,r9,lo(end_except)
        l.movhi r10,hi(excpt_int)
        l.ori   r10,r10,lo(excpt_int)
        l.lwz   r10,0(r10)
        l.jr    r10
        l.nop

        .org    0x900
dtlbmiss_vector:
        l.addi  r1,r1,-116
        l.sw    0x18(r1),r9
        l.jal   store_regs
        l.nop

        l.mfspr r3,r0,SPR_EPCR_BASE
        l.addi  r3,r3,-4
        l.mtspr r0,r3,SPR_EPCR_BASE

        l.movhi r9,hi(end_except)
        l.ori   r9,r9,lo(end_except)
        l.movhi r10,hi(excpt_dtlbmiss)
        l.ori   r10,r10,lo(excpt_dtlbmiss)
        l.lwz   r10,0(r10)
        l.jr    r10
        l.nop

        .org    0xa00
itlbmiss_vector:
        l.addi  r1,r1,-116
        l.sw    0x18(r1),r9
        l.jal   store_regs
        l.nop
        l.movhi r9,hi(end_except)
        l.ori   r9,r9,lo(end_except)
        l.movhi r10,hi(excpt_itlbmiss)
        l.ori   r10,r10,lo(excpt_itlbmiss)
        l.lwz   r10,0(r10)
        l.jr    r10
        l.nop

        .org    0xb00
range_vector:
        l.addi  r1,r1,-116
        l.sw    0x18(r1),r9
        l.jal   store_regs
        l.nop
        l.movhi r9,hi(end_except)
        l.ori   r9,r9,lo(end_except)
        l.movhi r10,hi(excpt_range)
        l.ori   r10,r10,lo(excpt_range)
        l.lwz   r10,0(r10)
        l.jr    r10
        l.nop

        .org    0xc00
syscall_vector:
        l.addi  r1,r1,-116
        l.sw    0x18(r1),r9
        l.jal   store_regs
        l.nop
        l.movhi r9,hi(end_except)
        l.ori   r9,r9,lo(end_except)
        l.movhi r10,hi(excpt_syscall)
        l.ori   r10,r10,lo(excpt_syscall)
        l.lwz   r10,0(r10)
        l.jr    r10
        l.nop

        .org    0xd00
break_vector:
        l.addi  r1,r1,-116
        l.sw    0x18(r1),r9
        l.jal   store_regs
        l.nop
        l.movhi r9,hi(end_except)
        l.ori   r9,r9,lo(end_except)
        l.movhi r10,hi(excpt_break)
        l.ori   r10,r10,lo(excpt_break)
        l.lwz   r10,0(r10)
        l.jr    r10
        l.nop

        .org    0xe00
trap_vector:
        l.addi  r1,r1,-116
        l.sw    0x18(r1),r9
        l.jal   store_regs
        l.nop
        l.movhi r9,hi(end_except)
        l.ori   r9,r9,lo(end_except)
        l.movhi r10,hi(excpt_trap)
        l.ori   r10,r10,lo(excpt_trap)
        l.lwz   r10,0(r10)
        l.jr    r10
        l.nop

store_regs:
        l.sw    0x00(r1),r3
        l.sw    0x04(r1),r4
        l.sw    0x08(r1),r5
        l.sw    0x0c(r1),r6
        l.sw    0x10(r1),r7
        l.sw    0x14(r1),r8
        l.sw    0x1c(r1),r10
        l.sw    0x20(r1),r11
        l.sw    0x24(r1),r12
        l.sw    0x28(r1),r13
        l.sw    0x2c(r1),r14
        l.sw    0x30(r1),r15
        l.sw    0x34(r1),r16
        l.sw    0x38(r1),r17
        l.sw    0x3c(r1),r18
        l.sw    0x40(r1),r19
        l.sw    0x44(r1),r20
        l.sw    0x48(r1),r21
        l.sw    0x4c(r1),r22
        l.sw    0x50(r1),r23
        l.sw    0x54(r1),r24
        l.sw    0x58(r1),r25
        l.sw    0x5c(r1),r26
        l.sw    0x60(r1),r27
        l.sw    0x64(r1),r28
        l.sw    0x68(r1),r29
        l.sw    0x6c(r1),r30
        l.sw    0x70(r1),r31
        l.jr    r9
        l.nop

end_except:
        l.lwz   r3,0x00(r1)
        l.lwz   r4,0x04(r1)
        l.lwz   r5,0x08(r1)
        l.lwz   r6,0x0c(r1)
        l.lwz   r7,0x10(r1)
        l.lwz   r8,0x14(r1)
        l.lwz   r9,0x18(r1)
        l.lwz   r10,0x1c(r1)
        l.lwz   r11,0x20(r1)
        l.lwz   r12,0x24(r1)
        l.lwz   r13,0x28(r1)
        l.lwz   r14,0x2c(r1)
        l.lwz   r15,0x30(r1)
        l.lwz   r16,0x34(r1)
        l.lwz   r17,0x38(r1)
        l.lwz   r18,0x3c(r1)
        l.lwz   r19,0x40(r1)
        l.lwz   r20,0x44(r1)
        l.lwz   r21,0x48(r1)
        l.lwz   r22,0x4c(r1)
        l.lwz   r23,0x50(r1)
        l.lwz   r24,0x54(r1)
        l.lwz   r25,0x58(r1)
        l.lwz   r26,0x5c(r1)
        l.lwz   r27,0x60(r1)
        l.lwz   r28,0x64(r1)
        l.lwz   r29,0x68(r1)
        l.lwz   r30,0x6c(r1)
        l.lwz   r31,0x70(r1)
        l.addi  r1,r1,116
        l.rfe
        l.nop 

init_mc:
        l.movhi r0, 0x0
        l.slli  r0,r0,16

/* Set speed of FLASH access */
/* TMS[0] = 0x00000210       */
/* TMS address = 0x60000014  */
        l.ori   r2,r0,0x020a
        l.movhi r3, 0x6000
        l.ori   r1,r3,0x0014
        l.sw    0(r1),r2

/* Set SDRAM parameters for CS1    */
/* old TMS[1] (*6000001c) = 0xfffff020 */
/* old CSC[1] (*60000018) = 0x00200611 */
/* CSR (*60000000) = 0x60300300    */
/* BA_MASK (*60000008) = 0x000000f0 */

/* TMS[1] (*6000001c) = 0xfffff023 */
/* CSC[1] (*60000018) = 0x00200491 */
        l.movhi r2,0x6030
        l.ori   r2,r2,0x0300
        l.ori   r1,r3,0x0000
        l.sw    0(r1),r2
        l.movhi r2,0x0000
        l.ori   r2,r2,0x00f0
        l.ori   r1,r3,0x0008
        l.sw    0(r1),r2
        l.movhi r2,0x0724
        l.ori   r2,r2,0x8230
        l.ori   r1,r3,0x001c
        l.sw    0(r1),r2
        l.movhi r2,0x0020
        l.ori   r2,r2,0x0411
        l.ori   r1,r3,0x0018
        l.sw    0(r1),r2
        l.xor   r0,r0,r0
        l.xor   r1,r1,r1
        l.xor   r2,r2,r2
        l.xor   r3,r3,r3

        l.j     start
        l.nop
