/* int-logger-edge.c. Test of Or1ksim handling of edge triggered interrupts

   Copyright (C) 2010 Embecosm Limited

   Contributors various OpenCores participants
   Contributor Jeremy Bennett <jeremy.bennett@embecosm.com>

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

/* ----------------------------------------------------------------------------
   This code is commented throughout for use with Doxygen.
   --------------------------------------------------------------------------*/

#include "support.h"
#include "spr-defs.h"
#include "board.h"


/* --------------------------------------------------------------------------*/
/*!Write a memory mapped register

   @param[in] addr   Memory mapped address
   @param[in] value  Value to set                                            */
/* --------------------------------------------------------------------------*/
static void
setreg (unsigned long addr,
	unsigned char value)
{
  *((volatile unsigned char *) addr) = value;

}	/* setreg () */


/* --------------------------------------------------------------------------*/
/*!Read a memory mapped register

   @param[in] addr   Memory mapped address

   @return  Value read                                                       */
/* --------------------------------------------------------------------------*/
unsigned long
getreg (unsigned long addr)
{
  return *((volatile unsigned char *) addr);

}	/* getreg () */


/* --------------------------------------------------------------------------*/
/*!Generic interrupt handler

   This should receive the interrupt exception. Report the value in PICSR,
   then clear the interrupt and reporting it again                           */
/* --------------------------------------------------------------------------*/
static void
interrupt_handler ()
{
  unsigned long int  old_picsr = mfspr (SPR_PICSR);
  mtspr (SPR_PICSR, 0);
  unsigned long int  new_picsr = mfspr (SPR_PICSR);

  printf ("PICSR 0x%08lx -> 0x%08lx\n", old_picsr, new_picsr);

}	/* interrupt_handler () */


/* --------------------------------------------------------------------------*/
/*!Main program to set up interrupt handler

   We make a series of upcalls, after 500 us and then every 1000us, which
   prompt some interrupts. By doing this, our upcalls should always be well
   clear of any calling function interrupt generation, which is on millisecond
   boundaries.

   @return  The return code from the program (always zero).                  */
/* --------------------------------------------------------------------------*/
int
main ()
{
  printf ("Starting interrupt handler\n");
  excpt_int = (unsigned long)interrupt_handler;
  
  /* Enable interrupts */
  printf ("Enabling interrupts.\n");
  mtspr (SPR_SR, mfspr(SPR_SR) | SPR_SR_IEE);
  mtspr (SPR_PICMR, 0xffffffff);

  /* Loop forever, upcalling at the desired intervals. */
  unsigned long int  start = read_timer ();

  while (1)
    {
      static int       do_read_p = 1;
      static long int  end_time  = 500;

      while ((read_timer () - start) < end_time)
	{
	}

      /* Do our memory mapped upcall, alternating reads and writes. */
      if (do_read_p)
	{
	  (void)getreg (GENERIC_BASE);
	  do_read_p = 0;
	}
      else
	{
	  setreg (GENERIC_BASE, 0);
	  do_read_p = 1;
	}

      /* Wait 1000us before next upcall. */
      end_time += 1000;
    }

  /* We don't actually ever return */
  return 0;

}	/* main () */
