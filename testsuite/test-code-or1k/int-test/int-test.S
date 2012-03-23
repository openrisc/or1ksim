/* int-test.S. Test of Or1ksim interrupt handling

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

/* NOTE. This is not a test of the Programmable Interrupt Controller.

   Within the test we'll use following global variables:
   
   r16 interrupt counter
   r17 current tick timer comparison counter
   r18 sanity counter
   r19 loop counter
   r20 temp value of SR reg
   r21 temp value of TTMR reg.
   r23 RAM_START
   
   r25-r31 used by int handler
   
   The test do the following:
   We set up the tick timer to trigger once and then we trigger interrupts
   incrementally on every cycle in the specified test program; on interrupt
   handler we check if data computed so far exactly matches precalculated
   values. If interrupt has returned incorreclty, we can detect this using
   assertion routine at the end.
*/

#include "spr-defs.h"
#include "board.h"

#define  RAM_START 0x00000000


.section  .except,"ax"

.org 0x100

_reset_vector:
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

  l.movhi r3,hi(_start)
  l.ori   r3,r3,lo(_start)
  l.jr    r3
  l.nop

	.org 0x500
_tick_handler:	
#
# Tick timer exception handler
#

  l.addi  r31,r3,0
# get interrupted program pc
  l.mfspr r25,r0,SPR_EPCR_BASE

# calculate instruction address
  l.movhi r26,hi(_ie_start)
  l.ori   r26,r26,lo(_ie_start)
  l.addi  r3,r25,0    #print insn index
  l.nop   2
  l.sub   r25,r25,r26
  l.addi  r3,r25,0    #print insn index
  l.nop   2
  
  l.addi  r3,r31,0    # restore r3
  l.sfeqi r25, 0x00
  l.bf    _i00
  l.sfeqi r25, 0x04
  l.bf    _i04
  l.sfeqi r25, 0x08
  l.bf    _i08
  l.sfeqi r25, 0x0c
  l.bf    _i0c
  l.sfeqi r25, 0x10
  l.bf    _i10
  l.sfeqi r25, 0x14
  l.bf    _i14
  l.sfeqi r25, 0x18
  l.bf    _i18
  l.sfeqi r25, 0x1c
  l.bf    _i1c
  l.sfeqi r25, 0x20
  l.bf    _i20
  l.sfeqi r25, 0x24
  l.bf    _i24
  l.sfeqi r25, 0x28
  l.bf    _i28
  l.sfeqi r25, 0x2c
  l.bf    _i2c
  l.sfeqi r25, 0x30
  l.bf    _i30
  l.sfeqi r25, 0x34
  l.bf    _i34
  l.sfeqi r25, 0x38
  l.bf    _i38
  l.nop

# value not defined
_die:
  l.nop   2             #print r3
  
  l.addi  r3,r0,0xeeee
  l.nop   2
  l.addi  r3,r0,1
  l.nop   1
1:
  l.j     1b
  l.nop


.section .text
	
_start:
	
  l.movhi r3,hi(_main)
  l.ori   r3,r3,lo(_main)
  l.jr    r3
  l.nop

	
_main:
	l.nop
  l.addi  r3,r0,SPR_SR_SM
  l.mtspr r0,r3,SPR_SR
	l.nop

#
# set tick counter to initial 3 cycles
#
  l.addi r16,r0,0
  l.addi r17,r0,1
  l.addi r18,r0,0
  l.addi r19,r0,0
  l.addi r22,r0,0
  
  l.movhi r23,hi(RAM_START)
  l.ori   r23,r23,lo(RAM_START)

# Set r20 to hold enable tick exception
	l.mfspr	r20,r0,SPR_SR
	l.ori r20,r20,SPR_SR_SM|SPR_SR_TEE|SPR_SR_F

# Set r21 to hold value of TTMR
	l.movhi r5,hi(SPR_TTMR_SR | SPR_TTMR_IE)
	l.add  r21,r5,r17

#
# MAIN LOOP
#
_main_loop:
# reinitialize memory and registers
  l.addi  r3,r0,0xaaaa
  l.addi  r9,r0,0xbbbb
  l.sw    0(r23),r3
  l.sw    4(r23),r9
  l.sw    8(r23),r3
  
# Reinitializes tick timer  
  l.addi  r17,r17,1
  l.mtspr r0,r0,SPR_TTCR 		# set TTCR
  l.mtspr r0,r21,SPR_TTMR		# set TTMR
  l.mtspr r0,r0,SPR_TTCR 		# set TTCR
	l.addi  r21,r21,1

# Enable exceptions and interrupts
	l.mtspr r0,r20,SPR_SR	# set SR

##### TEST CODE #####
_ie_start:
  l.movhi r3,0x1234         #00
  l.sw    0(r23),r3         #04
  l.movhi r3,hi(RAM_START)  #08
  l.lwz   r3,0(r3)          #0c
  l.movhi r3,hi(RAM_START)  #10
  l.addi  r3,r3,4           #14
  l.j     1f                #18
  l.lwz   r3,0(r3)          #1c
  l.addi  r3,r3,1           #20
1:
  l.sfeqi r3,0xdead         #24
  l.jal   2f                #28
  l.addi  r3,r0,0x5678      #2c

_return_addr:
2:
  l.bf    _die              #30
  l.sw    8(r23),r3         #34
_ie_end:
  l.nop                     #38
##### END OF TEST CODE #####

# do some testing

  l.j     _main_loop
  l.nop

_i00:
  l.sfeqi r3,0xaaaa
  l.bnf   _die
  l.nop
  l.j     _resume
  l.nop
_i04:
  l.movhi  r26,0x1234
  l.sfeq   r3,r26
  l.bnf   _die
  l.nop
  l.lwz   r26,0(r23)
  l.sfeqi r26,0xaaaa
  l.bnf   _die
  l.nop
  l.j     _resume
  l.nop
_i08:
  l.movhi r26,0x1234
  l.sfeq  r3,r26
  l.bnf   _die
  l.nop
  l.lwz   r27,0(r23)
  l.sfeq  r27,r26
  l.bnf   _die
  l.nop
  l.j     _resume
  l.nop
_i0c:
  l.sfeq  r3,r23
  l.bnf   _die
  l.nop
  l.j     _resume
  l.nop
_i10:
  l.movhi r26,0x1234
  l.sfeq  r26,r3
  l.bnf   _die
  l.nop
  l.j     _resume
  l.nop
_i14:
  l.sfeq  r3,r23
  l.bnf   _die
  l.nop
  l.j     _resume
  l.nop
_i18:
  l.addi  r26,r23,4
  l.sfeq  r3,r26
  l.bnf   _die
  l.nop
  l.j     _resume
  l.nop
_i1c:
  l.j     _die
  l.nop
_i20:
  l.j     _die
  l.nop
_i24:
  l.mfspr r26,r0,SPR_ESR_BASE
  l.addi  r30,r3,0
  l.addi  r3,r26,0
  l.nop   2
  l.addi  r3,r30,0
  l.andi  r26,r26,SPR_SR_F
  l.sfeq  r26,r0
/*  l.bnf   _die */
  l.nop
  l.sfeqi  r3,0xbbbb
  l.bnf   _die
  l.nop
  l.j     _resume
  l.nop
_i28:
  l.mfspr r26,r0,SPR_ESR_BASE
  l.addi  r30,r3,0
  l.addi  r3,r26,0
  l.nop   2
  l.addi  r3,r30,0
  l.andi  r26,r26,SPR_SR_F
  l.sfeq  r26,r0
  l.bnf    _die
  l.nop
  l.sfeqi  r22,1
  l.bf     _resume
  l.addi   r22,r0,1
  l.sfeqi  r9,0xbbbb
  l.bnf   _die
  l.nop
  l.j     _resume
  l.nop
_i2c:
  l.movhi  r26,hi(_return_addr)
  l.ori    r26,r26,lo(_return_addr)
  l.sfeq   r9,r26
  l.bnf   _die
  l.nop
  l.sfeqi  r3,0xbbbb
  l.bnf   _die
  l.nop
  l.j     _resume
  l.nop
_i30:
  l.sfeqi  r3,0x5678
  l.bnf   _die
  l.nop
  l.j     _resume
  l.nop  
_i34:
  l.sfeqi  r3,0x5678
  l.bnf   _die
  l.nop
  l.lwz    r26,8(r23)
  l.sfeqi  r26,0xaaaa
  l.bnf   _die
  l.nop
  l.j     _resume
  l.nop  
_i38:
  l.lwz    r26,8(r23)
  l.sfeqi  r26,0x5678
  l.bnf   _die
  l.nop
#
# mark finished ok
#
  l.movhi r3,hi(0xdeaddead)
  l.ori   r3,r3,lo(0xdeaddead)
  l.nop   2
  l.addi  r3,r0,0
  l.nop   1
_ok:
  l.j     _ok
  l.nop

_resume:
  l.mfspr  r27,r0,SPR_ESR_BASE
  l.addi   r26,r0,SPR_SR_TEE
  l.addi   r28,r0,-1
  l.xor    r26,r26,r28
  l.and    r26,r26,r27
  l.mtspr  r0,r26,SPR_ESR_BASE
  
  l.rfe
  l.addi    r3,r3,5         # should not be executed
