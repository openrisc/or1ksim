
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
#include <stdlib.h>
#include <signal.h>
#include <string.h>
								 */
#include "support.h" // OR1k support C library
#include "milieu.h"
#include "fail.h"
#include "softfloat.h"
#include "slowfloat.h"
#include "testCases.h"
#include "testLoops.h"

int8 clearFlags( void )
{
    int8 flags;

    flags = float_exception_flags;
    float_exception_flags = 0;
    return flags;

}

enum {
    INT32_TO_FLOAT32 = 1,
    FLOAT32_TO_INT32,
    FLOAT32_TO_INT32_ROUND_TO_ZERO,
    FLOAT32_ROUND_TO_INT,
    FLOAT32_ADD,
    FLOAT32_SUB,
    FLOAT32_MUL,
    FLOAT32_DIV,
    FLOAT32_REM,
    //    FLOAT32_SQRT,
    FLOAT32_EQ,
    FLOAT32_LE,
    FLOAT32_LT,
    FLOAT32_EQ_SIGNALING,
    FLOAT32_LE_QUIET,
    FLOAT32_LT_QUIET,
    NUM_FUNCTIONS
};
static struct {
    char *name;
    int8 numInputs;
    flag roundingPrecision, roundingMode;
    flag tininessMode, tininessModeAtReducedPrecision;
} functions[ NUM_FUNCTIONS ] = {
    { 0, 0, 0, 0, 0, 0 },
    { "int32_to_float32",                1, FALSE, TRUE,  FALSE, FALSE },
    { "float32_to_int32",                1, FALSE, TRUE,  FALSE, FALSE },
    { "float32_to_int32_round_to_zero",  1, FALSE, FALSE, FALSE, FALSE },
    { "float32_round_to_int",            1, FALSE, TRUE,  FALSE, FALSE },
    { "float32_add",                     2, FALSE, TRUE,  FALSE, FALSE },
    { "float32_sub",                     2, FALSE, TRUE,  FALSE, FALSE },
    { "float32_mul",                     2, FALSE, TRUE,  TRUE,  FALSE },
    { "float32_div",                     2, FALSE, TRUE,  FALSE, FALSE },
    { "float32_rem",                     2, FALSE, FALSE, FALSE, FALSE },
    //{ "float32_sqrt",                    1, FALSE, TRUE,  FALSE, FALSE },
    { "float32_eq",                      2, FALSE, FALSE, FALSE, FALSE },
    { "float32_le",                      2, FALSE, FALSE, FALSE, FALSE },
    { "float32_lt",                      2, FALSE, FALSE, FALSE, FALSE },
    { "float32_eq_signaling",            2, FALSE, FALSE, FALSE, FALSE },
    { "float32_le_quiet",                2, FALSE, FALSE, FALSE, FALSE },
    { "float32_lt_quiet",                2, FALSE, FALSE, FALSE, FALSE }
};

enum {
    ROUND_NEAREST_EVEN = 1,
    ROUND_TO_ZERO,
    ROUND_DOWN,
    ROUND_UP,
    NUM_ROUNDINGMODES
};
enum {
    TININESS_BEFORE_ROUNDING = 1,
    TININESS_AFTER_ROUNDING,
    NUM_TININESSMODES
};

static void
 testFunctionVariety(
     uint8 functionCode,
     int8 roundingPrecision,
     int8 roundingMode,
     int8 tininessMode
 )
{
    uint8 roundingCode;
    int8 tininessCode;

    functionName = functions[ functionCode ].name;
    if ( roundingPrecision == 32 ) {
        roundingPrecisionName = "32";
    }
    else {
        roundingPrecisionName = 0;
    }
    switch ( roundingMode ) {
     case 0:
        roundingModeName = 0;
        roundingCode = float_round_nearest_even;
        break;
     case ROUND_NEAREST_EVEN:
        roundingModeName = "nearest_even";
        roundingCode = float_round_nearest_even;
        break;
     case ROUND_TO_ZERO:
        roundingModeName = "to_zero";
        roundingCode = float_round_to_zero;
        break;
     case ROUND_DOWN:
        roundingModeName = "down";
        roundingCode = float_round_down;
        break;
     case ROUND_UP:
        roundingModeName = "up";
        roundingCode = float_round_up;
        break;
    default: // To keep GCC happy
        roundingModeName = "nearest_even";
        roundingCode = float_round_nearest_even;
        break;      
    }
    float_rounding_mode = roundingCode;
    slow_float_rounding_mode = roundingCode;
    switch ( tininessMode ) {
     case 0:
        tininessModeName = 0;
        tininessCode = float_tininess_after_rounding;
        break;
     case TININESS_BEFORE_ROUNDING:
        tininessModeName = "before";
        tininessCode = float_tininess_before_rounding;
        break;
     case TININESS_AFTER_ROUNDING:
        tininessModeName = "after";
        tininessCode = float_tininess_after_rounding;
        break;
    default:
      tininessCode = -1; // to keep GCC happy
    }
    float_detect_tininess = tininessCode;
    slow_float_detect_tininess = tininessCode;
    printf( "Testing "/*, stderr*/ );
    writeFunctionName( /*stderr*/ );
    printf( ".\n"/*, stderr*/ );
    switch ( functionCode ) {
     case INT32_TO_FLOAT32:
        test_a_int32_z_float32( slow_int32_to_float32, int32_to_float32 );
        break;
     case FLOAT32_TO_INT32:
        test_a_float32_z_int32( slow_float32_to_int32, float32_to_int32 );
        break;
     case FLOAT32_TO_INT32_ROUND_TO_ZERO:
        test_a_float32_z_int32(
            slow_float32_to_int32_round_to_zero,
            float32_to_int32_round_to_zero
        );
        break;
     case FLOAT32_ROUND_TO_INT:
        test_az_float32( slow_float32_round_to_int, float32_round_to_int );
        break;
     case FLOAT32_ADD:
        test_abz_float32( slow_float32_add, float32_add );
        break;
     case FLOAT32_SUB:
        test_abz_float32( slow_float32_sub, float32_sub );
        break;
     case FLOAT32_MUL:
        test_abz_float32( slow_float32_mul, float32_mul );
        break;
     case FLOAT32_DIV:
        test_abz_float32( slow_float32_div, float32_div );
        break;
     case FLOAT32_REM:
        test_abz_float32( slow_float32_rem, float32_rem );
        break;
	//     case FLOAT32_SQRT:
	//        test_az_float32( slow_float32_sqrt, float32_sqrt );
	//        break;
     case FLOAT32_EQ:
        test_ab_float32_z_flag( slow_float32_eq, float32_eq );
        break;
     case FLOAT32_LE:
        test_ab_float32_z_flag( slow_float32_le, float32_le );
        break;
     case FLOAT32_LT:
        test_ab_float32_z_flag( slow_float32_lt, float32_lt );
        break;
     case FLOAT32_EQ_SIGNALING:
        test_ab_float32_z_flag(
            slow_float32_eq_signaling, float32_eq_signaling );
        break;
     case FLOAT32_LE_QUIET:
        test_ab_float32_z_flag( slow_float32_le_quiet, float32_le_quiet );
        break;
     case FLOAT32_LT_QUIET:
        test_ab_float32_z_flag( slow_float32_lt_quiet, float32_lt_quiet );
        break;
    }
    if ( ( errorStop && anyErrors ) || stop ) exitWithStatus();

}

static void
 testFunction(
     uint8 functionCode,
     int8 roundingPrecisionIn,
     int8 roundingModeIn,
     int8 tininessModeIn
 )
{
    int8 roundingPrecision, roundingMode, tininessMode;

    roundingPrecision = 32;
    for (;;) {
        if ( ! functions[ functionCode ].roundingPrecision ) {
            roundingPrecision = 0;
        }
        else if ( roundingPrecisionIn ) {
            roundingPrecision = roundingPrecisionIn;
        }
        for ( roundingMode = 1;
              roundingMode < NUM_ROUNDINGMODES;
              ++roundingMode
            ) {
            if ( ! functions[ functionCode ].roundingMode ) {
                roundingMode = 0;
            }
            else if ( roundingModeIn ) {
                roundingMode = roundingModeIn;
            }
            for ( tininessMode = 1;
                  tininessMode < NUM_TININESSMODES;
                  ++tininessMode
                ) {
                if (    ( roundingPrecision == 32 )
                     || ( roundingPrecision == 64 ) ) {
                    if ( ! functions[ functionCode ]
                               .tininessModeAtReducedPrecision
                       ) {
                        tininessMode = 0;
                    }
                    else if ( tininessModeIn ) {
                        tininessMode = tininessModeIn;
                    }
                }
                else {
                    if ( ! functions[ functionCode ].tininessMode ) {
                        tininessMode = 0;
                    }
                    else if ( tininessModeIn ) {
                        tininessMode = tininessModeIn;
                    }
                }
                testFunctionVariety(
                    functionCode, roundingPrecision, roundingMode, tininessMode
                );
                if ( tininessModeIn || ! tininessMode ) break;
            }
            if ( roundingModeIn || ! roundingMode ) break;
        }
        if ( roundingPrecisionIn || ! roundingPrecision ) break;
        if ( roundingPrecision == 80 ) {
            break;
        }
        else if ( roundingPrecision == 64 ) {
            roundingPrecision = 80;
        }
        else if ( roundingPrecision == 32 ) {
            roundingPrecision = 64;
        }
    }

}

int
main( int argc, char **argv )
{
  //char *argPtr;
    flag functionArgument;
    uint8 functionCode;
    int8 operands, roundingPrecision, roundingMode, tininessMode;
    /*
    fail_programName = "testsoftfloat";
    if ( argc <= 1 ) goto writeHelpMessage;
    */
    testCases_setLevel( 1 );
    trueName = "true";
    testName = "soft";
    errorStop = FALSE;
    forever = FALSE;
    maxErrorCount = 5000;
    trueFlagsPtr = &slow_float_exception_flags;
    testFlagsFunctionPtr = clearFlags;
    functionArgument = TRUE;
    functionCode = 0;
    operands = 0;
    roundingPrecision = 0;
    roundingMode = 0;
    tininessMode = 0;
    /*
    --argc;
    ++argv;
    while ( argc && ( argPtr = argv[ 0 ] ) ) {
        if ( argPtr[ 0 ] == '-' ) ++argPtr;
        if ( strcmp( argPtr, "help" ) == 0 ) {
 writeHelpMessage:
            fputs(
"testsoftfloat [<option>...] <function>\n"
"  <option>:  (* is default)\n"
"    -help            --Write this message and exit.\n"
"    -level <num>     --Testing level <num> (1 or 2).\n"
" *  -level 1\n"
"    -errors <num>    --Stop each function test after <num> errors.\n"
" *  -errors 20\n"
"    -errorstop       --Exit after first function with any error.\n"
"    -forever         --Test one function repeatedly (implies `-level 2').\n"
#ifdef FLOATX80
"    -precision32     --Only test rounding precision equivalent to float32.\n"
"    -precision64     --Only test rounding precision equivalent to float64.\n"
"    -precision80     --Only test maximum rounding precision.\n"
#endif
"    -nearesteven     --Only test rounding to nearest/even.\n"
"    -tozero          --Only test rounding to zero.\n"
"    -down            --Only test rounding down.\n"
"    -up              --Only test rounding up.\n"
"    -tininessbefore  --Only test underflow tininess before rounding.\n"
"    -tininessafter   --Only test underflow tininess after rounding.\n"
"  <function>:\n"
"    int32_to_<float>                 <float>_add   <float>_eq\n"
"    <float>_to_int32                 <float>_sub   <float>_le\n"
"    <float>_to_int32_round_to_zero   <float>_mul   <float>_lt\n"
#ifdef BITS64
"    int64_to_<float>                 <float>_div   <float>_eq_signaling\n"
"    <float>_to_int64                 <float>_rem   <float>_le_quiet\n"
"    <float>_to_int64_round_to_zero                 <float>_lt_quiet\n"
"    <float>_to_<float>\n"
"    <float>_round_to_int\n"
"    <float>_sqrt\n"
#else
"    <float>_to_<float>               <float>_div   <float>_eq_signaling\n"
"    <float>_round_to_int             <float>_rem   <float>_le_quiet\n"
"    <float>_sqrt                                   <float>_lt_quiet\n"
#endif
"    -all1            --All 1-operand functions.\n"
"    -all2            --All 2-operand functions.\n"
"    -all             --All functions.\n"
"  <float>:\n"
"    float32          --Single precision.\n"
"    float64          --Double precision.\n"
#ifdef FLOATX80
"    floatx80         --Extended double precision.\n"
#endif
#ifdef FLOAT128
"    float128         --Quadruple precision.\n"
#endif
                ,
                stdout
            );
            return EXIT_SUCCESS;
        }
        else if ( strcmp( argPtr, "level" ) == 0 ) {
            if ( argc < 2 ) goto optionError;
            testCases_setLevel( atoi( argv[ 1 ] ) );
            --argc;
            ++argv;
        }
        else if ( strcmp( argPtr, "level1" ) == 0 ) {
            testCases_setLevel( 1 );
        }
        else if ( strcmp( argPtr, "level2" ) == 0 ) {
            testCases_setLevel( 2 );
        }
        else if ( strcmp( argPtr, "errors" ) == 0 ) {
            if ( argc < 2 ) {
     optionError:
                fail( "`%s' option requires numeric argument", argv[ 0 ] );
            }
            maxErrorCount = atoi( argv[ 1 ] );
            --argc;
            ++argv;
        }
        else if ( strcmp( argPtr, "errorstop" ) == 0 ) {
            errorStop = TRUE;
        }
        else if ( strcmp( argPtr, "forever" ) == 0 ) {
            testCases_setLevel( 2 );
            forever = TRUE;
        }
#ifdef FLOATX80
        else if ( strcmp( argPtr, "precision32" ) == 0 ) {
            roundingPrecision = 32;
        }
        else if ( strcmp( argPtr, "precision64" ) == 0 ) {
            roundingPrecision = 64;
        }
        else if ( strcmp( argPtr, "precision80" ) == 0 ) {
            roundingPrecision = 80;
        }
#endif
        else if (    ( strcmp( argPtr, "nearesteven" ) == 0 )
                  || ( strcmp( argPtr, "nearest_even" ) == 0 ) ) {
            roundingMode = ROUND_NEAREST_EVEN;
        }
        else if (    ( strcmp( argPtr, "tozero" ) == 0 )
                  || ( strcmp( argPtr, "to_zero" ) == 0 ) ) {
            roundingMode = ROUND_TO_ZERO;
        }
        else if ( strcmp( argPtr, "down" ) == 0 ) {
            roundingMode = ROUND_DOWN;
        }
        else if ( strcmp( argPtr, "up" ) == 0 ) {
            roundingMode = ROUND_UP;
        }
        else if ( strcmp( argPtr, "tininessbefore" ) == 0 ) {
            tininessMode = TININESS_BEFORE_ROUNDING;
        }
        else if ( strcmp( argPtr, "tininessafter" ) == 0 ) {
            tininessMode = TININESS_AFTER_ROUNDING;
        }
        else if ( strcmp( argPtr, "all1" ) == 0 ) {
            functionArgument = TRUE;
            functionCode = 0;
            operands = 1;
        }
        else if ( strcmp( argPtr, "all2" ) == 0 ) {
            functionArgument = TRUE;
            functionCode = 0;
            operands = 2;
        }
        else if ( strcmp( argPtr, "all" ) == 0 ) {
            functionArgument = TRUE;
            functionCode = 0;
            operands = 0;
        }
        else {
            for ( functionCode = 1;
                  functionCode < NUM_FUNCTIONS;
                  ++functionCode
                ) {
                if ( strcmp( argPtr, functions[ functionCode ].name ) == 0 ) {
                    break;
                }
            }
            if ( functionCode == NUM_FUNCTIONS ) {
                fail( "Invalid option or function `%s'", argv[ 0 ] );
            }
            functionArgument = TRUE;
        }
        --argc;
        ++argv;
    }

    */

    // Test just a single function
    if ( functionCode ) {
        testFunction(
            functionCode, roundingPrecision, roundingMode, tininessMode );
    }
    else {
        if ( operands == 1 ) {
            for ( functionCode = 1;
                  functionCode < NUM_FUNCTIONS;
                  ++functionCode
                ) {
                if ( functions[ functionCode ].numInputs == 1 ) {
                    testFunction(
                        functionCode,
                        roundingPrecision,
                        roundingMode,
                        tininessMode
                    );
                }
            }
        }
        else if ( operands == 2 ) {
            for ( functionCode = 1;
                  functionCode < NUM_FUNCTIONS;
                  ++functionCode
                ) {
                if ( functions[ functionCode ].numInputs == 2 ) {
                    testFunction(
                        functionCode,
                        roundingPrecision,
                        roundingMode,
                        tininessMode
                    );
                }
            }
        }
        else {
            for ( functionCode = 1;
                  functionCode < NUM_FUNCTIONS;
                  ++functionCode
                ) {
                testFunction(
                    functionCode, roundingPrecision, roundingMode, tininessMode
                );
            }
        }
    }
    exitWithStatus();
    
    // Shouldn't reach here.
    return 1;
}

