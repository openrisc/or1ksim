/* fpee.c. Test of Or1ksim floating point exceptions

   Copyright (C) 2023 OpenRISC Developers

   Contributor Stafford Horne <shorne@gmail.com>

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

#include <float.h>
#include <stdint.h>

#include "spr-defs.h"
#include "support.h"

static void enter_user_mode()
{
  int32_t sr;

  sr = mfspr(SPR_SR);
  sr &= ~SPR_SR_SM;
  mtspr(SPR_SR, sr);
}

void enable_fpe()
{
  unsigned long fpcsr = SPR_FPCSR_FPEE;
  mtspr(SPR_FPCSR, fpcsr);
}

/* A divide routine using the or1k FPU, this ensures the result
   does not overwrite the inputs in case we have looping
   as we saw with a bug in QEMU.

   Return uint32_t to avoid having to link to floating point
   routines.
 */
static uint32_t fpu_div(float a, float b)
{
  uint32_t c = 0;
  asm volatile("lf.div.s %0, %1, %2"
               : "+r" (c)
               :  "r" (a), "r" (b));
  return c;
}

/* A multiply routine using the or1k FPU, this ensure the result
   does not overwrite the inputs in case we have looping as we
   with a bug in QEMU.

   Return uint32_t to avoid having to link to floating point
   routines,
 */
static uint32_t fpu_mul(float a, float b)
{
  uint32_t c = 0;
  asm volatile("lf.mul.s %0, %1, %2" : "+r" (c) : "r" (a), "r" (b));
  return c;
}

static int handle_count = 0;

static void fpe_handler()
{
 uint32_t epcr = mfspr(SPR_EPCR_BASE);

 printf("Got fpe: %lx\n", epcr);

 if (handle_count++ > 0)
   {
     printf("Oh no, multiple exceptions, abort QEMU/Kernel/normal?!\n");
     exit(1);
   }
}

int main()
{
  uint32_t result;

  /* Setup FPU exception handler.  */
  excpt_fpu = (unsigned long) fpe_handler;

  printf("Switching to User Mode\n");
  enter_user_mode();

  printf("Enabling FPU Exceptions\n");
  enable_fpe();

  printf("Exceptions enabled, now DIV 3.14 / 0!\n");
  result = fpu_div(3.14f, 0.0f);
  /* Verify we see infinity.  */
  report((unsigned long) result);
  /* Verify we see DZF set.  */
  report(mfspr(SPR_FPCSR));
  mtspr(SPR_FPCSR, mfspr(SPR_FPCSR) & ~SPR_FPCSR_ALLF);

  handle_count = 0;
  printf("One more, now MUL 3.14 * MAX!\n");
  result = fpu_mul(3.14f, FLT_MAX);
  /* Verify we see infinity.  */
  report((unsigned long) result);
  /* Verify we see Inexact and Overflow flags set.  */
  report(mfspr(SPR_FPCSR));
  mtspr(SPR_FPCSR, mfspr(SPR_FPCSR) & ~SPR_FPCSR_ALLF);
}
