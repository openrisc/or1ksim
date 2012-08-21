
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

#include "milieu.h"
#include "systmodes.h"
#include "spr-defs.h"

// Rounding modes as used by softfloat, we use them here to
enum {
    float_round_nearest_even = 0,
    float_round_down         = 1,
    float_round_up           = 2,
    float_round_to_zero      = 3
};

/*
-------------------------------------------------------------------------------
Sets the system's IEC/IEEE floating-point rounding mode.  Also disables all
system exception traps.
-------------------------------------------------------------------------------
*/
void syst_float_set_rounding_mode( int8 roundingMode )
{

  // Read the FPCSR
  unsigned int spr = SPR_FPCSR;
  unsigned int value;
  // Read the SPR
  asm("l.mfspr\t\t%0,%1,0" : "=r" (value) : "r" (spr));

  // Clear the current rounding mode
  value &= ~SPR_FPCSR_RM;

  // Extract the flags from OR1K's FPCSR, put into testfloat's flags format  
  switch(roundingMode)
    {
    case float_round_nearest_even:
      value |= FPCSR_RM_RN;
      break;
    case float_round_down:
      value |= FPCSR_RM_RIN;
      break;
    case float_round_up:
      value |= FPCSR_RM_RIP;
      break;
    case float_round_to_zero:
      value |= FPCSR_RM_RZ;
      break;
    default:
      //printf("%s: Unknown rounding mode: 0x%x\n",__FUNCTION__,roundingMode);
      // error!
      break;
    }

  // Disable FPEE
  value &= ~SPR_FPCSR_FPEE;

  // Write value back to FPCSR
  asm("l.mtspr\t\t%0,%1,0": : "r" (spr), "r" (value));
}

/*
-------------------------------------------------------------------------------
Sets the rounding precision of subsequent extended double-precision
operations.  The `precision' argument should be one of 0, 32, 64, or 80.
If `precision' is 32, the rounding precision is set equivalent to single
precision; else if `precision' is 64, the rounding precision is set
equivalent to double precision; else the rounding precision is set to full
extended double precision.
-------------------------------------------------------------------------------
*/
void syst_float_set_rounding_precision( int8 precision )
{

  //!!!code (possibly empty)
  // Yes empty for OR1K 32-bit implementation - have no choice of rounding
  // precision.
  return;

}

