/* int-logger-level.c. Test of Or1ksim handling of level triggered interrupts

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

   This should receive the interrupt exception. Report the value in
   PICSR. Clearing will be done externally.                                  */
/* --------------------------------------------------------------------------*/
static void
interrupt_handler ()
{
  unsigned long int  picsr = mfspr (SPR_PICSR);

  /* Report the interrupt */
  printf ("PICSR = 0x%08lx\n", picsr);

  /* Request the interrupt be cleared with a write upcall. */
  setreg (GENERIC_BASE, 0);

}	/* interrupt_handler () */


/* --------------------------------------------------------------------------*/
/*!Main program to set up interrupt handler

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

  /* Loop forever, upcalling reads every 500us to generate interrupts. */
  unsigned long int  start = read_timer ();

  while (1)
    {
      static long int  end_time  = 500;

      while ((read_timer () - start) < end_time)
	{
	}

      /* Do our memory mapped upcall read to generate an interrupt. */
      (void)getreg (GENERIC_BASE);
    }

  /* We don't actually ever return */
  return 0;

}	/* main () */
