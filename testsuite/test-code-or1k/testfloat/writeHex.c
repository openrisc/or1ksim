
/*
===============================================================================

This C source file is part of TestFloat, Release 2a, a package of programs
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
#include <stdio.h>
								 */
#include "support.h" // OR1k support C library
#include "milieu.h"
#include "softfloat.h"
#include "writeHex.h"

void writeHex_flag( flag a/*, FILE *stream */ )
{

    putchar( a ? '1' : '0' );

}

static void writeHex_bits8( bits8 a/*, FILE *stream */ )
{
    int digit;

    digit = ( a>>4 ) & 0xF;
    if ( 9 < digit ) digit += 'A' - ( '0' + 10 );
    putchar( '0' + digit );
    digit = a & 0xF;
    if ( 9 < digit ) digit += 'A' - ( '0' + 10 );
    putchar( '0' + digit );

}

static void writeHex_bits12( int16 a/*, FILE *stream */ )
{
    int digit;

    digit = ( a>>8 ) & 0xF;
    if ( 9 < digit ) digit += 'A' - ( '0' + 10 );
    putchar( '0' + digit );
    digit = ( a>>4 ) & 0xF;
    if ( 9 < digit ) digit += 'A' - ( '0' + 10 );
    putchar( '0' + digit );
    digit = a & 0xF;
    if ( 9 < digit ) digit += 'A' - ( '0' + 10 );
    putchar( '0' + digit );

}

static void writeHex_bits16( bits16 a/*, FILE *stream */ )
{
    int digit;

    digit = ( a>>12 ) & 0xF;
    if ( 9 < digit ) digit += 'A' - ( '0' + 10 );
    putchar( '0' + digit );
    digit = ( a>>8 ) & 0xF;
    if ( 9 < digit ) digit += 'A' - ( '0' + 10 );
    putchar( '0' + digit );
    digit = ( a>>4 ) & 0xF;
    if ( 9 < digit ) digit += 'A' - ( '0' + 10 );
    putchar( '0' + digit );
    digit = a & 0xF;
    if ( 9 < digit ) digit += 'A' - ( '0' + 10 );
    putchar( '0' + digit );

}

void writeHex_bits32( bits32 a/*, FILE *stream */ )
{

    writeHex_bits16( a>>16 );
    writeHex_bits16( a );

}

#ifdef BITS64

void writeHex_bits64( bits64 a/*, FILE *stream */ )
{

    writeHex_bits32( a>>32 );
    writeHex_bits32( a );

}

#endif

void writeHex_float32( float32 a/*, FILE *stream */ )
{

    putchar( ( ( (sbits32) a ) < 0 ) ? '8' : '0' );
    writeHex_bits8( a>>23 );
    putchar( '.' );
    writeHex_bits8( ( a>>16 ) & 0x7F );
    writeHex_bits16( a );

}

#ifdef BITS64

void writeHex_float64( float64 a/*, FILE *stream */ )
{

    writeHex_bits12( a>>52 );
    putchar( '.' );
    writeHex_bits12( a>>40 );
    writeHex_bits8( a>>32 );
    writeHex_bits32( a );

}

#else

void writeHex_float64( float64 a/*, FILE *stream */ )
{

    writeHex_bits12( a.high>>20 );
    putchar( '.' );
    writeHex_bits12( a.high>>8 );
    writeHex_bits8( a.high );
    writeHex_bits32( a.low );

}

#endif

#ifdef FLOATX80

void writeHex_floatx80( floatx80 a/*, FILE *stream */ )
{

    writeHex_bits16( a.high );
    putchar( '.' );
    writeHex_bits64( a.low );

}

#endif

#ifdef FLOAT128

void writeHex_float128( float128 a/*, FILE *stream */ )
{

    writeHex_bits16( a.high>>48 );
    putchar( '.' );
    writeHex_bits16( a.high>>32 );
    writeHex_bits32( a.high );
    writeHex_bits64( a.low );

}

#endif

void writeHex_float_flags( uint8 flags/*, FILE *stream */ )
{

    putchar( flags & float_flag_invalid   ? 'v' : '.' );
    putchar( flags & float_flag_divbyzero ? 'z' : '.' );
    putchar( flags & float_flag_overflow  ? 'o' : '.' );
    putchar( flags & float_flag_underflow ? 'u' : '.' );
    putchar( flags & float_flag_inexact   ? 'x' : '.' );

}

