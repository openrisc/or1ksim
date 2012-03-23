/* cfg.s. CPU configuration test of Or1ksim

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
 
	.section .except,"ax"
	.org 0x100
_reset:
        // Clear R0 on start-up. There is no guarantee that R0 is hardwired to zero,
        // and indeed it is not when simulating the or1200 Verilog core.
        l.andi  r0,r0,0x0
        
        l.addi  r1,r0,0x7f00
        l.movhi r2,hi(_main)
        l.ori   r2,r2,lo(_main)
        l.jr    r2
        l.nop

	.section .text
_main:
	l.addi	r2,r0,0

	l.mfspr	r3,r0,SPR_VR		/* Version */
  l.nop NOP_REPORT
	l.add	r2,r2,r3

	l.mfspr r3,r0,SPR_UPR		/* Unit Present */
  l.nop NOP_REPORT
        l.add   r2,r2,r3
		
	l.mfspr	r4,r0,SPR_PMR		/* Power Management */
	l.addi	r3,r0,0
	l.mtspr	r0,r3,SPR_PMR
	l.mfspr	r3,r0,SPR_PMR
	l.andi	r3,r3,0xff
	l.nop NOP_REPORT
        l.add   r2,r2,r3 

	l.addi	r3,r0,5
	l.mtspr r0,r3,SPR_PMR
        l.mfspr r3,r0,SPR_PMR
        l.andi  r3,r3,0xff
        l.nop NOP_REPORT
        l.add   r2,r2,r3
	
	l.mtspr	r0,r4,SPR_PMR

	l.mfspr	r3,r0,SPR_CPUCFGR
	l.nop NOP_REPORT
        l.add   r2,r2,r3

	l.mfspr	r3,r0,SPR_DMMUCFGR
	l.nop NOP_REPORT
        l.add   r2,r2,r3

	l.mfspr	r3,r0,SPR_IMMUCFGR
	l.nop NOP_REPORT
        l.add   r2,r2,r3

	l.mfspr	r3,r0,SPR_DCCFGR
	l.nop NOP_REPORT
        l.add   r2,r2,r3

	l.mfspr	r3,r0,SPR_ICCFGR
	l.nop NOP_REPORT
        l.add   r2,r2,r3
	
	l.mfspr	r3,r0,SPR_DCFGR
	l.nop NOP_REPORT
        l.add   r2,r2,r3
	
	l.mfspr	r3,r0,SPR_PCCFGR
	l.nop NOP_REPORT
        l.add   r2,r2,r3

	/* Configurations may differ, so we will insert another report*/
	l.movhi	r3,hi(0xdeacf5cc)
	l.ori	r3,r3,lo(0xdeacf5cc)
	l.add	r3,r2,r3
  l.nop NOP_REPORT

	l.movhi	r3,hi(0xdeaddead)
	l.ori	r3,r3,lo(0xdeaddead)
  l.nop NOP_REPORT
	l.addi	r3,r0,0
	l.nop NOP_EXIT
