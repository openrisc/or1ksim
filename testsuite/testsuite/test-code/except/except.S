/* except.s. Exception handling support for Or1k tests

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

#define reset _reset

#define MC_CSR          (0x00)
#define MC_POC          (0x04)
#define MC_BA_MASK      (0x08)
#define MC_CSC(i)       (0x10 + (i) * 8)
#define MC_TMS(i)       (0x14 + (i) * 8)

        .section .stack
        .space 0x1000
_stack:

        .extern _reset_support
        .extern _c_reset
        .extern _excpt_buserr
        .extern _excpt_dpfault
        .extern _excpt_ipfault
        .extern _excpt_tick
        .extern _excpt_align
        .extern _excpt_illinsn
        .extern _excpt_int
        .extern _excpt_dtlbmiss
        .extern _excpt_itlbmiss
        .extern _excpt_range
        .extern _excpt_syscall
        .extern _excpt_break
        .extern _excpt_trap


	.section .except, "ax"
_buserr_vector:
        l.addi  r1,r1,-120
        l.sw    0x1c(r1),r9
        l.sw    0x20(r1),r10
        l.movhi r9,hi(store_regs)
        l.ori   r9,r9,lo(store_regs)
        l.movhi r10,hi(_excpt_buserr)
        l.ori   r10,r10,lo(_excpt_buserr)
        l.jr    r9
        l.nop
_buserr_vector_end:

_dpfault_vector:
        l.addi  r1,r1,-120
        l.sw    0x1c(r1),r9
        l.sw    0x20(r1),r10
        l.movhi r9,hi(store_regs)
        l.ori   r9,r9,lo(store_regs)
        l.movhi r10,hi(_excpt_dpfault)
        l.ori   r10,r10,lo(_excpt_dpfault)
        l.jr    r9
        l.nop
_dpfault_vector_end:

_ipfault_vector:
        l.addi  r1,r1,-120
        l.sw    0x1c(r1),r9
        l.sw    0x20(r1),r10
        l.movhi r9,hi(store_regs)
        l.ori   r9,r9,lo(store_regs)
        l.movhi r10,hi(_excpt_ipfault)
        l.ori   r10,r10,lo(_excpt_ipfault)
        l.jr    r9
        l.nop
_ipfault_vector_end:

_lpint_vector:
        l.addi  r1,r1,-120
        l.sw    0x1c(r1),r9
        l.sw    0x20(r1),r10
        l.movhi r9,hi(store_regs)
        l.ori   r9,r9,lo(store_regs)
        l.movhi r10,hi(_excpt_tick)
        l.ori   r10,r10,lo(_excpt_tick)
        l.jr    r9
        l.nop
_lpint_vector_end:

_align_vector:
        l.addi  r1,r1,-120
        l.sw    0x1c(r1),r9
        l.sw    0x20(r1),r10
        l.movhi r9,hi(store_regs)
        l.ori   r9,r9,lo(store_regs)
        l.movhi r10,hi(_excpt_align)
        l.ori   r10,r10,lo(_excpt_align)
        l.jr    r9
        l.nop
_align_vector_end:

_illinsn_vector:
        l.addi  r1,r1,-120
        l.sw    0x1c(r1),r9
        l.sw    0x20(r1),r10
        l.movhi r9,hi(store_regs)
        l.ori   r9,r9,lo(store_regs)
        l.movhi r10,hi(_excpt_illinsn)
        l.ori   r10,r10,lo(_excpt_illinsn)
        l.jr    r9
        l.nop
_illinsn_vector_end:

_hpint_vector:
        l.addi  r1,r1,-120
        l.sw    0x1c(r1),r9
        l.sw    0x20(r1),r10
        l.movhi r9,hi(store_regs)
        l.ori   r9,r9,lo(store_regs)
        l.movhi r10,hi(_excpt_int)
        l.ori   r10,r10,lo(_excpt_int)
        l.jr    r9
        l.nop
_hpint_vector_end:

_dtlbmiss_vector:
        l.addi  r1,r1,-120
        l.sw    0x1c(r1),r9
        l.sw    0x20(r1),r10
        l.movhi r9,hi(store_regs)
        l.ori   r9,r9,lo(store_regs)
        l.movhi r10,hi(_excpt_dtlbmiss)
        l.ori   r10,r10,lo(_excpt_dtlbmiss)
        l.jr    r9
        l.nop
_dtlbmiss_vector_end:

_itlbmiss_vector:
        l.addi  r1,r1,-120
        l.sw    0x1c(r1),r9
        l.sw    0x20(r1),r10
        l.movhi r9,hi(store_regs)
        l.ori   r9,r9,lo(store_regs)
        l.movhi r10,hi(_excpt_itlbmiss)
        l.ori   r10,r10,lo(_excpt_itlbmiss)
        l.jr    r9
        l.nop
_itlbmiss_vector_end:

_range_vector:
        l.addi  r1,r1,-120
        l.sw    0x1c(r1),r9
        l.sw    0x20(r1),r10
        l.movhi r9,hi(store_regs)
        l.ori   r9,r9,lo(store_regs)
        l.movhi r10,hi(_excpt_range)
        l.ori   r10,r10,lo(_excpt_range)
        l.jr    r9
        l.nop
_range_vector_end:

_syscall_vector:
        l.addi  r1,r1,-120
        l.sw    0x1c(r1),r9
        l.sw    0x20(r1),r10
        l.movhi r9,hi(store_regs)
        l.ori   r9,r9,lo(store_regs)
        l.movhi r10,hi(_excpt_syscall)
        l.ori   r10,r10,lo(_excpt_syscall)
        l.jr    r9
        l.nop
_syscall_vector_end:

_break_vector:
        l.addi  r1,r1,-120
        l.sw    0x1c(r1),r9
        l.sw    0x20(r1),r10
        l.movhi r9,hi(store_regs)
        l.ori   r9,r9,lo(store_regs)
        l.movhi r10,hi(_excpt_break)
        l.ori   r10,r10,lo(_excpt_break)
        l.jr    r9
        l.nop
_break_vector_end:

_trap_vector:
        l.addi  r1,r1,-120
        l.sw    0x1c(r1),r9
        l.sw    0x20(r1),r10
        l.movhi r9,hi(store_regs)
        l.ori   r9,r9,lo(store_regs)
        l.movhi r10,hi(_excpt_trap)
        l.ori   r10,r10,lo(_excpt_trap)
        l.jr    r9
        l.nop
_trap_vector_end:

	/* Our special text section is used to guarantee this code goes first
	   when linking. */
	.section .except-text

        .org    0x100
	.align  4
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
	.global	start
start:
        l.jal   _init_mc
        l.nop

        l.movhi r1,hi(_stack)
        l.ori   r1,r1,lo(_stack)

        /* Setup exception wrappers */
        l.movhi r3,hi(_src_beg)
        l.ori   r3,r3,lo(_src_beg)
        l.addi  r7,r0,0x100

1:      l.addi  r7,r7,0x100
        l.sfeqi r7,0xf00
        l.bf    1f
        l.nop
        l.addi  r4,r7,0
        l.addi  r5,r0,0
2:
        l.lwz   r6,0(r3)
        l.sw    0(r4),r6
        l.addi  r3,r3,4
        l.addi  r4,r4,4
        l.addi  r5,r5,1
        l.sfeqi r5,9
        l.bf    1b
        l.nop
        l.j     2b  
        l.nop
1:
        /* Copy data section */
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

store_regs:
        l.sw    0x00(r1),r2
        l.sw    0x04(r1),r3
        l.sw    0x08(r1),r4
        l.sw    0x0c(r1),r5
        l.sw    0x10(r1),r6
        l.sw    0x14(r1),r7
        l.sw    0x18(r1),r8
        l.sw    0x24(r1),r11
        l.sw    0x28(r1),r12
        l.sw    0x2c(r1),r13
        l.sw    0x30(r1),r14
        l.sw    0x34(r1),r15
        l.sw    0x38(r1),r16
        l.sw    0x3c(r1),r17
        l.sw    0x40(r1),r18
        l.sw    0x44(r1),r19
        l.sw    0x48(r1),r20
        l.sw    0x4c(r1),r21
        l.sw    0x50(r1),r22
        l.sw    0x54(r1),r23
        l.sw    0x58(r1),r24
        l.sw    0x5c(r1),r25
        l.sw    0x60(r1),r26
        l.sw    0x64(r1),r27
        l.sw    0x68(r1),r28
        l.sw    0x6c(r1),r29
        l.sw    0x70(r1),r30
        l.sw    0x74(r1),r31
        l.movhi r9,hi(end_except)
        l.ori   r9,r9,lo(end_except)
        l.lwz   r10,0(r10)
        l.jr    r10
        l.nop

end_except:
        l.lwz   r2,0x00(r1)
        l.lwz   r3,0x04(r1)
        l.lwz   r4,0x08(r1)
        l.lwz   r5,0x0c(r1)
        l.lwz   r6,0x10(r1)
        l.lwz   r7,0x14(r1)
        l.lwz   r8,0x18(r1)
        l.lwz   r9,0x1c(r1)
        l.lwz   r10,0x20(r1)
        l.lwz   r11,0x24(r1)
        l.lwz   r12,0x28(r1)
        l.lwz   r13,0x2c(r1)
        l.lwz   r14,0x30(r1)
        l.lwz   r15,0x34(r1)
        l.lwz   r16,0x38(r1)
        l.lwz   r17,0x3c(r1)
        l.lwz   r18,0x40(r1)
        l.lwz   r19,0x44(r1)
        l.lwz   r20,0x48(r1)
        l.lwz   r21,0x4c(r1)
        l.lwz   r22,0x50(r1)
        l.lwz   r23,0x54(r1)
        l.lwz   r24,0x58(r1)
        l.lwz   r25,0x5c(r1)
        l.lwz   r26,0x60(r1)
        l.lwz   r27,0x64(r1)
        l.lwz   r28,0x68(r1)
        l.lwz   r29,0x6c(r1)
        l.lwz   r30,0x70(r1)
        l.lwz   r31,0x74(r1)
        l.addi  r1,r1,120
        l.rfe
        l.nop 
