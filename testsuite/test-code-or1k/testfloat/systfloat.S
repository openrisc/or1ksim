
/*
===============================================================================

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

Modified for use with or1ksim's testsuite.

Contributor Julius Baxter <julius.baxter@orsoc.se>

===============================================================================
*/

	.text

/*
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
*/
	.align 4
	.global _syst_int32_to_float32
_syst_int32_to_float32:
	lf.itof.s r11, r3
	l.jr r9
	l.nop




/*
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
*/
	.align 4
	.global _syst_float32_to_int32_round_to_zero
_syst_float32_to_int32_round_to_zero:
	lf.ftoi.s r11, r3
	l.jr r9
	l.nop

/*
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
*/
	.align 4
	.global _syst_float32_to_int32
_syst_float32_to_int32:
	lf.ftoi.s r11, r3
	l.jr r9
	l.nop

/*
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
*/
	.align 4
	.global _syst_float32_round_to_int
_syst_float32_round_to_int:
	lf.ftoi.s r11, r3
	l.jr r9
	l.nop
	
/*
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
*/
	.align 4
	.global _syst_float32_add
_syst_float32_add:
	lf.add.s r11, r3, r4
	l.jr r9
	l.nop

/*
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
*/
	.align 4
	.global _syst_float32_sub
_syst_float32_sub:
	lf.sub.s r11, r3, r4
	l.jr r9
	l.nop

/*
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
*/
	.align 4
	.global _syst_float32_mul
_syst_float32_mul:
	lf.mul.s r11, r3, r4
	l.jr r9
	l.nop

/*
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
*/
	.align 4
	.global _syst_float32_div
_syst_float32_div:
	lf.div.s r11, r3, r4
	l.jr r9
	l.nop

/*
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
*/
	.align 4
	.global _syst_float32_eq
_syst_float32_eq:
	lf.sfeq.s r3, r4
	l.bnf 1f
	l.addi r11, r0, 0
	l.addi r11, r0, 1
1:	l.jr r9
	l.nop

/*
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
*/
	.align 4
	.global _syst_float32_le
_syst_float32_le:
	lf.sfle.s r3, r4
	l.bnf 1f
	l.addi r11, r0, 0
	l.addi r11, r0, 1
1:	l.jr r9
	l.nop

	/*
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
*/
	.align 4
	.global _syst_float32_lt
_syst_float32_lt:
	lf.sflt.s r3, r4
	l.bnf 1f
	l.addi r11, r0, 0
	l.addi r11, r0, 1
1:	l.jr r9
	l.nop
