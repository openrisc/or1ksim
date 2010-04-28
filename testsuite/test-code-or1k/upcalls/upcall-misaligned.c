/* upcall-misaligned.c. Test of Or1ksim misaligned handling of upcalls

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


/*!Alignment exception handler defined here. */
unsigned long excpt_align;

/*!Flag set if we get a misaligned access */
static int  misaligned_p;

/* --------------------------------------------------------------------------*/
/*!Write a memory mapped byte register

   Report the result, noting if we get a misalignment exception.

   @param[in] addr   Memory mapped address
   @param[in] value  Value to set                                            */
/* --------------------------------------------------------------------------*/
static void
setreg8 (unsigned long int  addr,
	 unsigned char      value)
{
  *((volatile unsigned char *) addr) = value;

  if (misaligned_p)
    {
      printf ("Writing byte at 0x%08lx: misalignment exception.\n", addr);
      misaligned_p = 0;
    }
  else
    {
      printf ("Wrote byte at 0x%08lx = 0x%02x.\n", addr, value);
    }
}	/* setreg8 () */


/* --------------------------------------------------------------------------*/
/*!Write a memory mapped half word register

   Report the result, noting if we get a misalignment exception.

   @param[in] addr   Memory mapped address
   @param[in] value  Value to set                                            */
/* --------------------------------------------------------------------------*/
static void
setreg16 (unsigned long int   addr,
	  unsigned short int  value)
{
  *((volatile unsigned short int *) addr) = value;

  if (misaligned_p)
    {
      printf ("Writing half word at 0x%08lx: misalignment exception.\n", addr);
      misaligned_p = 0;
    }
  else
    {
      printf ("Wrote half word at 0x%08lx = 0x%04x.\n", addr, value);
    }
}	/* setreg16 () */


/* --------------------------------------------------------------------------*/
/*!Write a memory mapped full word register

   Report the result, noting if we get a misalignment exception.

   @param[in] addr   Memory mapped address
   @param[in] value  Value to set                                            */
/* --------------------------------------------------------------------------*/
static void
setreg32 (unsigned long int  addr,
	  unsigned long int  value)
{
  *((volatile unsigned long int *) addr) = value;

  if (misaligned_p)
    {
      printf ("Writing full word at 0x%08lx: misalignment exception.\n", addr);
      misaligned_p = 0;
    }
  else
    {
      printf ("Wrote full word at 0x%08lx = 0x%08lx.\n", addr, value);
    }
}	/* setreg32 () */


/* --------------------------------------------------------------------------*/
/*!Read a memory mapped byte register

   Report the result, noting if we get a misalignment exception.

   @param[in] addr   Memory mapped address

   @return  Value read                                                       */
/* --------------------------------------------------------------------------*/
unsigned char
getreg8 (unsigned long int  addr)
{
  unsigned char  res = *((volatile unsigned char *) addr);

  if (misaligned_p)
    {
      printf ("Reading byte at 0x%08lx: misalignment exception.\n", addr);
      misaligned_p = 0;
    }
  else
    {
      printf ("Read byte at 0x%08lx = 0x%02x.\n", addr, res);
    }

  return  res;

}	/* getreg8 () */


/* --------------------------------------------------------------------------*/
/*!Read a memory mapped half word register

   Report the result, noting if we get a misalignment exception.

   @param[in] addr   Memory mapped address

   @return  Value read                                                       */
/* --------------------------------------------------------------------------*/
unsigned short int
getreg16 (unsigned long int  addr)
{
  unsigned short int  res = *((volatile unsigned short int *) addr);

  if (misaligned_p)
    {
      printf ("Reading half word at 0x%08lx: misalignment exception.\n", addr);
      misaligned_p = 0;
    }
  else
    {
      printf ("Read half word at 0x%08lx = 0x%04x.\n", addr, res);
    }

  return  res;

}	/* getreg16 () */


/* --------------------------------------------------------------------------*/
/*!Read a memory mapped full word register

   Report the result, noting if we get a misalignment exception.

   @param[in] addr   Memory mapped address

   @return  Value read                                                       */
/* --------------------------------------------------------------------------*/
unsigned long int
getreg32 (unsigned long int  addr)
{
  unsigned long int  res = *((volatile unsigned long int *) addr);

  if (misaligned_p)
    {
      printf ("Reading full word at 0x%08lx: misalignment exception.\n", addr);
      misaligned_p = 0;
    }
  else
    {
      printf ("Read full word at 0x%08lx = 0x%08lx.\n", addr, res);
    }

  return  res;

}	/* getreg32 () */


/* --------------------------------------------------------------------------*/
/*!Exception handler for misalignment.

   Set the flag to indicated we have hit an exception.

   We can't just return, since that will give us the instruction that caused
   the problem in the first place. What we do instead is patch the EPCR SPR.

   This is fine, so long as we didn't trap in a delay slot. The crude response
   to that then we just exit.                                                */
/* --------------------------------------------------------------------------*/
void align_handler (void)
{
  misaligned_p = 1;

  unsigned long int  iaddr = mfspr (SPR_EPCR_BASE);
  unsigned long int  instr = *(unsigned long int *) iaddr;
  unsigned char      opc   = instr >> 26;

  switch (opc)
    {
    case 0x00:			/* l.j    */
    case 0x01:			/* l.jal  */
    case 0x03:			/* l.bnf  */
    case 0x04:			/* l.bf   */
    case 0x11:			/* l.jr   */
    case 0x12:			/* l.jalr */
  
      printf ("Misalignment exception unable to recover from delay slot.\n");
      exit (1);
      break;

    default:
      mtspr (SPR_EPCR_BASE, iaddr + 4);		/* Step past */
      break;
    }
}	/* align_handler () */


/* --------------------------------------------------------------------------*/
/*!Main program to read and write memory mapped registers

   Some of these are misaligned and should cause the appropriate exception.

   @return  The return code from the program (always zero).                  */
/* --------------------------------------------------------------------------*/
int
main ()
{
  /* Set the exception handler, and note that no exceptions have yet
     happened. */
  printf ("Setting alignment exception handler.\n");
  excpt_align  = (unsigned long)align_handler;
  misaligned_p = 0;

  /* Write some registers */
  printf ("Writing registers.\n");

  setreg8 (GENERIC_BASE, 0xde);

  setreg16 (GENERIC_BASE +  2, 0xdead);		/* Aligned */
  setreg16 (GENERIC_BASE +  5, 0xbeef);		/* Unaligned */

  setreg32 (GENERIC_BASE +  8, 0xdeadbeef);	/* Aligned */
  setreg32 (GENERIC_BASE + 13, 0xcafebabe);	/* Unaligned */
  setreg32 (GENERIC_BASE + 18, 0xbaadf00d);	/* Unaligned */
  setreg32 (GENERIC_BASE + 23, 0xfee1babe);	/* Unaligned */
  setreg32 (GENERIC_BASE + 28, 0xbaadbabe);	/* Aligned */

  /* Write some registers */
  printf ("Reading registers.\n");

  (void)getreg8 (GENERIC_BASE);

  (void)getreg16 (GENERIC_BASE +  2);		/* Aligned */
  (void)getreg16 (GENERIC_BASE +  5);		/* Unaligned */

  (void)getreg32 (GENERIC_BASE +  8);		/* Aligned */
  (void)getreg32 (GENERIC_BASE + 13);		/* Unaligned */
  (void)getreg32 (GENERIC_BASE + 18);		/* Unaligned */
  (void)getreg32 (GENERIC_BASE + 23);		/* Unaligned */
  (void)getreg32 (GENERIC_BASE + 28);		/* Aligned */

  /* We don't actually ever return */
  report (0xdeaddead);
  return 0;

}	/* main () */
