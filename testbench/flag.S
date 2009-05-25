/* Basic SR flag test */
#include "spr_defs.h"

#define SET_ARITH_FLAG   0  /* If this is not set this test has no meaning */

	.section .except, "ax"
  l.movhi r10,0x8000

  .section .text
	.org 0x100
_reset:
	l.nop
  
  l.movhi r10,0x8000
  l.addi  r11,r0,-1
  l.addi  r12,r0,2
  l.addi  r13,r0,0x5678
  l.movhi r14,0xdead
  l.ori   r14,r14,0xdead
  l.addi  r15,r0,0xdead

  /* Test start */
  
#if SET_ARITH_FLAG
  /* Simple zero test */
  l.addi r1,r0,1        /* f = 0 */
  l.addi  r1, r0, 0
  l.bnf     _err
  l.bf     _err
  l.addi r1,r0,1        /* f = 0 */
  l.add  r1, r0, r0
  l.bnf     _err
  l.addi r1,r0,1        /* f = 0 */
  l.andi  r1, r0, 0
  l.bnf     _err
  l.addi r1,r0,1        /* f = 0 */
  l.and  r1, r0, r0
  l.bnf     _err
  
  l.addi r1,r0,1        /* f = 0 */
  l.sub  r1, r0, r0
  l.bf     _err
  l.or   r1, r0, r0
  l.bf     _err
  l.ori  r1, r0, 0
  l.bf     _err
  l.xor  r1, r0, r0
  l.bf     _err
  l.xori r1, r0, 0
  l.bf     _err
  
  l.addi r1,r0,0        /* f = 1 */
  l.sub  r1, r0, r0
  l.bnf     _err
  l.or   r1, r0, r0
  l.bnf     _err
  l.ori  r1, r0, 0
  l.bnf     _err
  l.xor  r1, r0, r0
  l.bnf     _err
  l.xori r1, r0, 0
  l.bnf     _err

  l.addi r1,r0,0        /* f = 1 */
  l.addi  r1, r0, 0xdead
  l.bf     _err
  l.addi r1,r0,0        /* f = 1 */
  l.add  r1, r0, r15
  l.bf     _err
  l.addi r1,r0,0        /* f = 1 */
  l.andi  r1, r11, 0xdead
  l.bf     _err
  l.addi r1,r0,0        /* f = 1 */
  l.and  r1, r11, r15
  l.bf     _err
  
  l.addi r1,r0,0        /* f = 1 */
  l.addi  r1, r11, 0
  l.bf     _err
  l.addi r1,r0,0        /* f = 1 */
  l.add  r1, r11, r0
  l.bf     _err
  l.addi r1,r0,0        /* f = 1 */
  l.andi  r1, r11, 0x1234
  l.bf     _err
  l.addi r1,r0,0        /* f = 1 */
  l.and  r1, r11, r10
  l.bf     _err
#endif

  l.movhi r3,0xdead
  l.ori   r3,r3,0xdead
  l.nop   NOP_REPORT
  l.ori   r3,r0,0
  l.nop   NOP_EXIT

_err:
  l.ori   r3,r1,0
  l.nop   NOP_REPORT
  l.mfspr r3,r0,SPR_SR
  l.nop   NOP_REPORT
  l.nop   NOP_EXIT
