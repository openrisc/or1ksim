/* basic.S. Basic instruction set test of Or1ksim

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

#define MEM_RAM 0x00000000

	.section .text
	.global main
main:	
	l.nop

_regs:
	l.addi	r1,r0,0x1
	l.addi	r2,r1,0x2
	l.addi	r3,r2,0x4
	l.addi	r4,r3,0x8
	l.addi	r5,r4,0x10
	l.addi	r6,r5,0x20
	l.addi	r7,r6,0x40
	l.addi	r8,r7,0x80
	l.addi	r9,r8,0x100
	l.addi	r10,r9,0x200
	l.addi	r11,r10,0x400
	l.addi	r12,r11,0x800
	l.addi	r13,r12,0x1000
	l.addi	r14,r13,0x2000
	l.addi	r15,r14,0x4000
	l.addi	r16,r15,0x8000

	l.sub	r31,r0,r1
	l.sub	r30,r31,r2
	l.sub	r29,r30,r3
	l.sub	r28,r29,r4
	l.sub	r27,r28,r5
	l.sub	r26,r27,r6
	l.sub	r25,r26,r7
	l.sub	r24,r25,r8
	l.sub	r23,r24,r9
	l.sub	r22,r23,r10
	l.sub	r21,r22,r11
	l.sub	r20,r21,r12
	l.sub	r19,r20,r13
	l.sub	r18,r19,r14
	l.sub	r17,r18,r15
	l.sub	r16,r17,r16

	l.or  r3,r0,r16
	l.nop NOP_REPORT  	/* Should be 0xffff0012 */

	l.movhi r31, hi(MEM_RAM)
	l.ori  r31,r31, lo(MEM_RAM)
	l.sw	0(r31),r16

_mem:	l.movhi	r3,0x1234
	l.ori	r3,r3,0x5678

	l.sw	4(r31),r3

	l.lbz	r4,4(r31)
	l.add	r8,r8,r4
	l.sb	11(r31),r4
	l.lbz	r4,5(r31)
	l.add	r8,r8,r4
	l.sb	10(r31),r4
	l.lbz	r4,6(r31)
	l.add	r8,r8,r4
	l.sb	9(r31),r4
	l.lbz	r4,7(r31)
	l.add	r8,r8,r4
	l.sb	8(r31),r4

	l.lbs	r4,8(r31)
	l.add	r8,r8,r4
	l.sb	7(r31),r4
	l.lbs	r4,9(r31)
	l.add	r8,r8,r4
	l.sb	6(r31),r4
	l.lbs	r4,10(r31)
	l.add	r8,r8,r4
	l.sb	5(r31),r4
	l.lbs	r4,11(r31)
	l.add	r8,r8,r4
	l.sb	4(r31),r4
	
	l.lhz	r4,4(r31)
	l.add	r8,r8,r4
	l.sh	10(r31),r4
	l.lhz	r4,6(r31)
	l.add	r8,r8,r4
	l.sh	8(r31),r4

	l.lhs	r4,8(r31)
	l.add	r8,r8,r4
	l.sh	6(r31),r4
	l.lhs	r4,10(r31)
	l.add	r8,r8,r4
	l.sh	4(r31),r4

	l.lwz	r4,4(r31)
	l.add	r8,r8,r4

	l.or  r3,r0,r8
	l.nop NOP_REPORT   	/* Should be 0x12352af7 */

	l.lwz	r9,0(r31)
	l.add	r8,r9,r8
	l.sw	0(r31),r8
	
_arith:
	l.addi	r3,r0,1
	l.addi	r4,r0,2
	l.addi	r5,r0,-1
	l.addi	r6,r0,-1
	l.addi	r8,r0,0
	
	l.sub	r7,r5,r3
	l.sub	r8,r3,r5
	l.add	r8,r8,r7
	
	l.div	r7,r7,r4
	l.add	r9,r3,r4
	l.mul	r7,r9,r7
	l.divu	r7,r7,r4
	l.add	r8,r8,r7

	l.or  r3,r0,r8
	l.nop NOP_REPORT   	/* Should be 0x7ffffffe */

	l.lwz	r9,0(r31)
	l.add	r8,r9,r8
	l.sw	0(r31),r8
	
_log:
	l.addi	r3,r0,1
	l.addi	r4,r0,2
	l.addi	r5,r0,-1
	l.addi	r6,r0,-1
	l.addi	r8,r0,0 

	l.andi	r8,r8,1
	l.and	r8,r8,r3

	l.xori	r8,r5,0xa5a5
	l.xor	r8,r8,r5

	l.ori	r8,r8,2
	l.or	r8,r8,r4

	l.or  r3,r0,r8
	l.nop NOP_REPORT   	/* Should be 0xffffa5a7 */

	l.lwz	r9,0(r31)
	l.add	r8,r9,r8
	l.sw	0(r31),r8

_shift:
	l.addi	r3,r0,1
	l.addi	r4,r0,2
	l.addi	r5,r0,-1
	l.addi	r6,r0,-1
	l.addi	r8,r0,0

	l.slli	r8,r5,6
	l.sll	r8,r8,r4
	
	l.srli	r8,r8,6
	l.srl	r8,r8,r4
	
	l.srai	r8,r8,2
	l.sra	r8,r8,r4	

	l.or  r3,r0,r8
	l.nop NOP_REPORT  	/* Should be 0x000fffff */

	l.lwz	r9,0(r31)
	l.add	r8,r9,r8
	l.sw	0(r31),r8
	
_flag:
	l.addi	r3,r0,1
	l.addi	r4,r0,-2
	l.addi	r8,r0,0

	l.sfeq	r3,r3
	l.mfspr	r5,r0,17
	l.andi	r4,r5,0x200
	l.add	r8,r8,r4

	l.sfeq	r3,r4
	l.mfspr r5,r0,17
	l.andi	r4,r5,0x200
	l.add	r8,r8,r4

	l.sfeqi r3,1
	l.mfspr r5,r0,17
	l.andi	r4,r5,0x200
	l.add	r8,r8,r4
 
	l.sfeqi r3,-2
	l.mfspr r5,r0,17
	l.andi	r4,r5,0x200
	l.add	r8,r8,r4 
	
	l.sfne	r3,r3
	l.mfspr r5,r0,17
	l.andi	r4,r5,0x200
	l.add	r8,r8,r4
 
	l.sfne	r3,r4
	l.mfspr r5,r0,17
	l.andi	r4,r5,0x200
	l.add	r8,r8,r4 

	l.sfnei r3,1
	l.mfspr r5,r0,17
	l.andi	r4,r5,0x200
	l.add	r8,r8,r4
 
	l.sfnei r3,-2
	l.mfspr r5,r0,17
	l.andi	r4,r5,0x200
	l.add	r8,r8,r4

	l.sfgtu r3,r3
	l.mfspr r5,r0,17
	l.andi	r4,r5,0x200
	l.add	r8,r8,r4
 
	l.sfgtu r3,r4
	l.mfspr r5,r0,17
	l.andi	r4,r5,0x200
	l.add	r8,r8,r4

	l.sfgtui	r3,1
	l.mfspr r5,r0,17
	l.andi	r4,r5,0x200
	l.add	r8,r8,r4
 
	l.sfgtui	r3,-2
	l.mfspr r5,r0,17
	l.andi	r4,r5,0x200
	l.add	r8,r8,r4

	l.sfgeu r3,r3
	l.mfspr r5,r0,17
	l.andi	r4,r5,0x200
	l.add	r8,r8,r4
 
	l.sfgeu r3,r4
	l.mfspr r5,r0,17
	l.andi	r4,r5,0x200
	l.add	r8,r8,r4

	l.sfgeui	r3,1
	l.mfspr r5,r0,17
	l.andi	r4,r5,0x200
	l.add	r8,r8,r4
 
	l.sfgeui	r3,-2
	l.mfspr r5,r0,17
	l.andi	r4,r5,0x200
	l.add	r8,r8,r4

	l.sfltu r3,r3
	l.mfspr r5,r0,17
	l.andi	r4,r5,0x200
	l.add	r8,r8,r4
 
	l.sfltu r3,r4
	l.mfspr r5,r0,17
	l.andi	r4,r5,0x200
	l.add	r8,r8,r4

	l.sfltui	r3,1
	l.mfspr r5,r0,17
	l.andi	r4,r5,0x200
	l.add	r8,r8,r4
 
	l.sfltui	r3,-2
	l.mfspr r5,r0,17
	l.andi	r4,r5,0x200
	l.add	r8,r8,r4

	l.sfleu r3,r3
	l.mfspr r5,r0,17
	l.andi	r4,r5,0x200
	l.add	r8,r8,r4
 
	l.sfleu r3,r4
	l.mfspr r5,r0,17
	l.andi	r4,r5,0x200
	l.add	r8,r8,r4
 
	l.sfleui	r3,1
	l.mfspr r5,r0,17
	l.andi	r4,r5,0x200
	l.add	r8,r8,r4
 
	l.sfleui	r3,-2
	l.mfspr r5,r0,17
	l.andi	r4,r5,0x200
	l.add	r8,r8,r4

	l.sfgts r3,r3
	l.mfspr r5,r0,17
	l.andi	r4,r5,0x200
	l.add	r8,r8,r4
 
	l.sfgts r3,r4
	l.mfspr r5,r0,17
	l.andi	r4,r5,0x200
	l.add	r8,r8,r4
 
	l.sfgtsi	r3,1
	l.mfspr r5,r0,17
	l.andi	r4,r5,0x200
	l.add	r8,r8,r4
 
	l.sfgtsi	r3,-2
	l.mfspr r5,r0,17
	l.andi	r4,r5,0x200
	l.add	r8,r8,r4

	l.sfges r3,r3
	l.mfspr r5,r0,17
	l.andi	r4,r5,0x200
	l.add	r8,r8,r4
 
	l.sfges r3,r4
	l.mfspr r5,r0,17
	l.andi	r4,r5,0x200
	l.add	r8,r8,r4
 
	l.sfgesi	r3,1
	l.mfspr r5,r0,17
	l.andi	r4,r5,0x200
	l.add	r8,r8,r4
 
	l.sfgesi	r3,-2
	l.mfspr r5,r0,17
	l.andi	r4,r5,0x200
	l.add	r8,r8,r4

	l.sflts r3,r3
	l.mfspr r5,r0,17
	l.andi	r4,r5,0x200
	l.add	r8,r8,r4
 
	l.sflts r3,r4
	l.mfspr r5,r0,17
	l.andi	r4,r5,0x200
	l.add	r8,r8,r4
 
	l.sfltsi	r3,1
	l.mfspr r5,r0,17
	l.andi	r4,r5,0x200
	l.add	r8,r8,r4
 
	l.sfltsi	r3,-2
	l.mfspr r5,r0,17
	l.andi	r4,r5,0x200
	l.add	r8,r8,r4

	l.sfles r3,r3
	l.mfspr r5,r0,17
	l.andi	r4,r5,0x200
	l.add	r8,r8,r4
 
	l.sfles r3,r4
	l.mfspr r5,r0,17
	l.andi	r4,r5,0x200
	l.add	r8,r8,r4
 
	l.sflesi	r3,1
	l.mfspr r5,r0,17
	l.andi	r4,r5,0x200
	l.add	r8,r8,r4
 
	l.sflesi	r3,-2
	l.mfspr r5,r0,17
	l.andi	r4,r5,0x200
	l.add	r8,r8,r4

	l.or  r3,r0,r8
	l.nop NOP_REPORT   	/* Should be 0x00002800 */

	l.lwz	r9,0(r31)
	l.add	r8,r9,r8
	l.sw	0(r31),r8

_jump:
	l.addi	r8,r0,0

	l.j	_T1
	l.addi	r8,r8,1

_T2:	l.jr	r9
	l.addi	r8,r8,1

_T1:	l.jal	_T2
	l.addi	r8,r8,1

	l.sfeqi	r0,0
	l.bf	_T3
	l.addi	r8,r8,1

_T3:	l.sfeqi	r0,1
	l.bf	_T4
	l.addi	r8,r8,1
	
	l.addi	r8,r8,1

_T4:	l.sfeqi r0,0
	l.bnf	 _T5
	l.addi	r8,r8,1

	l.addi	r8,r8,1

_T5:	l.sfeqi r0,1
	l.bnf	 _T6
	l.addi	r8,r8,1
 
	l.addi	r8,r8,1

_T6:	l.movhi r3,hi(_T7)
	l.ori  r3,r3,lo(_T7)
	l.mtspr r0,r3,32
	l.mfspr r5,r0,17
	l.mtspr r0,r5,64
	l.rfe
	l.addi	r8,r8,1 	/* l.rfe should not have a delay slot */

	l.addi	r8,r8,1

_T7:	l.or  r3,r0,r8
	l.nop NOP_REPORT   	/* Should be 0x000000009 */

	l.lwz	r9,0(r31)
	l.add	r8,r9,r8
	l.sw	0(r31),r8

	l.lwz	r9,0(r31)
	l.movhi	r3,0x4c69
	l.ori	r3,r3,0xe5f7
	l.add	r8,r8,r3

	l.or  r3,r0,r8
	l.nop NOP_REPORT   	/* Should be 0xdeaddead */

	l.addi	r3,r0,0
	l.nop NOP_EXIT

