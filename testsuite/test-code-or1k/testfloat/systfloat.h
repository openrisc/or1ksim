
/*
===============================================================================

This C header file is part of TestFloat, Release 2a, a package of programs
for testing the correctness of floating-point arithmetic complying to the
IEC/IEEE Standard for Floating-Point.

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

/*
-------------------------------------------------------------------------------
The following macros are defined to indicate that the corresponding
functions exist.
-------------------------------------------------------------------------------
*/
#define SYST_INT32_TO_FLOAT32
#define SYST_FLOAT32_TO_INT32
#define SYST_FLOAT32_TO_INT32_ROUND_TO_ZERO
								 //#define SYST_FLOAT32_ROUND_TO_INT
#define SYST_FLOAT32_ADD
#define SYST_FLOAT32_SUB
#define SYST_FLOAT32_MUL
#define SYST_FLOAT32_DIV
#define SYST_FLOAT32_EQ
#define SYST_FLOAT32_LE
#define SYST_FLOAT32_LT




/*
-------------------------------------------------------------------------------
System function declarations.  (Some of these functions may not exist.)
-------------------------------------------------------------------------------
*/
float32 syst_int32_to_float32( int32 );
int32 syst_float32_to_int32( float32 );
int32 syst_float32_to_int32_round_to_zero( float32 );
float32 syst_float32_round_to_int( float32 );
float32 syst_float32_add( float32, float32 );
float32 syst_float32_sub( float32, float32 );
float32 syst_float32_mul( float32, float32 );
float32 syst_float32_div( float32, float32 );
float32 syst_float32_rem( float32, float32 );
flag syst_float32_eq( float32, float32 );
flag syst_float32_le( float32, float32 );
flag syst_float32_lt( float32, float32 );
flag syst_float32_eq_signaling( float32, float32 );
flag syst_float32_le_quiet( float32, float32 );
flag syst_float32_lt_quiet( float32, float32 );


