
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
#include "systflags.h"
#include "spr-defs.h"
//#include <stdio.h>

// Float flag values, from softfloat.h
enum {
    float_flag_invalid   =  1,
    float_flag_divbyzero =  4,
    float_flag_overflow  =  8,
    float_flag_underflow = 16,
    float_flag_inexact   = 32
};

/*
-------------------------------------------------------------------------------
Clears the system's IEC/IEEE floating-point exception flags.  Returns the
previous value of the flags.
-------------------------------------------------------------------------------
*/
int8 syst_float_flags_clear( void )
{

  // Read the FPCSR
  unsigned int spr = SPR_FPCSR;
  unsigned int value;
  int8 flags = 0;
  // Read the SPR
  asm("l.mfspr\t\t%0,%1,0" : "=r" (value) : "r" (spr));
  
  // Extract the flags from OR1K's FPCSR, put into testfloat's flags format  
  if (value & SPR_FPCSR_IXF)
    flags |= float_flag_inexact;

  if (value & SPR_FPCSR_UNF)
    flags |= float_flag_underflow;

  if (value & SPR_FPCSR_OVF)
    flags |= float_flag_overflow;

  if (value & SPR_FPCSR_DZF)
    flags |= float_flag_divbyzero;

  if (value & SPR_FPCSR_IVF)
    flags |= float_flag_invalid;
  
  //  printf("testfloat: getting flags, FPCSR: 0x%x, softfloatflags: 0x%x\n", 
  //         value & SPR_FPCSR_ALLF, flags);

  // Clear flags in FPCSR
  value &= ~(SPR_FPCSR_ALLF);
  
  // Write value back to FPCSR
  asm("l.mtspr\t\t%0,%1,0": : "r" (spr), "r" (value));

  return flags;

}

