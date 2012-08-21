/* =============================================================================

This GNU assembler source file is part of TestFloat, Release 2a, a package
of programs for testing the correctness of floating-point arithmetic
complying to the IEC/IEEE Standard for Floating-Point.

Written by John R. Hauser.  More information is available through the Web
page `http://HTTP.CS.Berkeley.EDU/~jhauser/arithmetic/TestFloat.html'.

THIS SOFTWARE IS DISTRIBUTED AS IS, FOR FREE.  Although reasonable effort
has been made to avoid it, THIS SOFTWARE MAY CONTAIN FAULTS THAT WILL AT
TIMES RESULT IN INCORRECT BEHAVIOR.  USE OF THIS SOFTWARE IS RESTRICTED TO
PERSONS AND ORGANIZATIONS WHO CAN AND WILL TAKE FULL RESPONSIBILITY FOR ANY
AND ALL LOSSES, COSTS, OR OTHER PROBLEMS ARISING FROM ITS USE.

Derivative works are acceptable, even for commercial purposes, so long as
(1) they include prominent notice that the work is derivative, and (2) they
include prominent notice akin to these four paragraphs for those parts of
this code that are retained.

Modified for use with or1ksim's testsuite. Updated for GCC 4.5.1.

Contributor Julius Baxter <julius.baxter@orsoc.se>
Contributor Jeremy Bennett <jeremy.bennett@embecosm.com>
	
============================================================================= */

	.text

/*
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
*/
	.align	4
	.global syst_int32_to_float32
syst_int32_to_float32:
	lf.itof.s r11, r3
	l.jr	r9
	l.nop




/*
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
*/
	.align	4
	.global syst_float32_to_int32_round_to_zero
syst_float32_to_int32_round_to_zero:
	lf.ftoi.s r11, r3
	l.jr	r9
	l.nop

/*
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
*/
	.align	4
	.global syst_float32_to_int32
syst_float32_to_int32:
	lf.ftoi.s r11, r3
	l.jr	r9
	l.nop

/*
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
*/
	.align	4
	.global syst_float32_round_to_int
syst_float32_round_to_int:
	lf.ftoi.s r11, r3
	l.jr	r9
	l.nop
	
/*
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
*/
	.align	4
	.global syst_float32_add
syst_float32_add:
	lf.add.s r11, r3, r4
	l.jr	r9
	l.nop

/*
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
*/
	.align	4
	.global syst_float32_sub
syst_float32_sub:
	lf.sub.s r11, r3, r4
	l.jr	r9
	l.nop

/*
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
*/
	.align	4
	.global syst_float32_mul
syst_float32_mul:
	lf.mul.s r11, r3, r4
	l.jr	r9
	l.nop

/*
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
*/
	.align	4
	.global syst_float32_div
syst_float32_div:
	lf.div.s r11, r3, r4
	l.jr	r9
	l.nop

/*
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
*/
	.align	4
	.global syst_float32_eq
syst_float32_eq:
	lf.sfeq.s r3, r4
	l.bnf	1f
	l.addi	r11, r0, 0
	l.addi	r11, r0, 1
1:	l.jr	r9
	l.nop

/*
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
*/
	.align	4
	.global syst_float32_le
syst_float32_le:
	lf.sfle.s r3, r4
	l.bnf	1f
	l.addi	r11, r0, 0
	l.addi	r11, r0, 1
1:	l.jr	r9
	l.nop

	/*
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
*/
	.align	4
	.global syst_float32_lt
syst_float32_lt:
	lf.sflt.s r3, r4
	l.bnf	1f
	l.addi	r11, r0, 0
	l.addi	r11, r0, 1
1:	l.jr	r9
	l.nop
