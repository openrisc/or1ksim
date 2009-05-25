/* Configuration tester */
#include "spr_defs.h"
 
	.section .except, "ax"
	l.addi	r2,r0,0

	.section .text
	.org 0x100
_reset:
        l.addi  r1,r0,0x7f00
        l.movhi r2,hi(_main)
        l.ori   r2,r2,lo(_main)
        l.jr    r2
        l.nop


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
