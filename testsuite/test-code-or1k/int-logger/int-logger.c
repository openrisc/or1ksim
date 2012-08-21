/* int-logger.c. Test of Or1ksim handling of interrupts

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
/*!Count the number of ones in a register

   SIMD Within A Register (SWAR) algorithm from Aggregate Magic Algorithms
   (http://aggregate.org/MAGIC/) from the University of Kentucky.

   32-bit recursive reduction using SWAR. First step is mapping 2-bit
   values into sum of 2 1-bit values in sneaky way.

   @param[in] x  The 32-bit register whose bits are to be counted.

   @return  The number of bits that are set to 1.                             */
/* --------------------------------------------------------------------------*/
static int
ones32 (unsigned long int  x)
{
  x -=  ((x >>  1)      & 0x55555555);
  x  = (((x >>  2)      & 0x33333333) + (x & 0x33333333));
  x  = (((x >>  4) + x) & 0x0f0f0f0f);
  x +=   (x >>  8);
  x +=   (x >> 16);

  return (x & 0x0000003f);

}	/* ones32 () */

/* --------------------------------------------------------------------------*/
/*!Count the number of ones in a register

   SIMD Within A Register (SWAR) algorithm from Aggregate Magic Algorithms
   (http://aggregate.org/MAGIC/) from the University of Kentucky.

   Compute the log to base 2 of a supplied numger. In this case we know it will
   be an exact power of 2. We return -1 if asked for log (0).

   @param[in] x  The 32-bit register whose log to base 2 we want.

   @return  The log to base 2 of the argument, or -1 if the argument was
            zero.                                                            */
/* --------------------------------------------------------------------------*/
static int
int_log2 (unsigned int  x)
{
  x |= (x >> 1);
  x |= (x >> 2);
  x |= (x >> 4);
  x |= (x >> 8);
  x |= (x >> 16);

  return  ones32 (x) - 1;

}	/* int_log2 () */


/* --------------------------------------------------------------------------*/
/*!Generic interrupt handler

   This should receive the interrupt exception. Report the value in PICSR.

   Potentially PICSR has multiple bits set, so we report the least significant
   bit. This is consistent with an approach which gives highest priority to
   any NMI lines (0 or 1).

   It is up to the external agent to clear the interrupt. We prompt it by
   writing the number of the interrupt we have just received.                */
/* --------------------------------------------------------------------------*/
static void
interrupt_handler ()
{
  unsigned long int  picsr = mfspr (SPR_PICSR);

  printf ("PICSR = 0x%08lx\n", picsr);
  setreg (GENERIC_BASE, int_log2 (picsr & -picsr));

}	/* interrupt_handler () */


/* --------------------------------------------------------------------------*/
/*!Main program to set up interrupt handler

   We make a series of read upcalls, after 500 us and then every 1000us, which
   prompt some interrupts being set and cleared. By doing this, our upcalls
   should always be well clear of any calling function interrupt generation,
   which is on millisecond boundaries.

   A read upcall is a request to trigger an interrupt. We will subsequently
   use a write upcall in the interrupt handler to request clearing of the
   interrupt.

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
      static long int  end_time  = 500;

      while ((read_timer () - start) < end_time)
	{
	}

      /* Read to request an interrupt */
      (void)getreg (GENERIC_BASE);

      /* Wait 1000us before next upcall. */
      end_time += 1000;
    }

  /* We don't actually ever return */
  return 0;

}	/* main () */
