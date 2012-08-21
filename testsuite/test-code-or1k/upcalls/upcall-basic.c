/* upcall-basic.c. Test of Or1ksim basic handling of upcalls

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


/* --------------------------------------------------------------------------*/
/*!Write a memory mapped byte register

   @param[in] addr   Memory mapped address
   @param[in] value  Value to set                                            */
/* --------------------------------------------------------------------------*/
static void
setreg8 (unsigned long int  addr,
	 unsigned char      value)
{
  *((volatile unsigned char *) addr) = value;
  printf ("Wrote byte at 0x%08lx = 0x%02x.\n", addr, value);

}	/* setreg8 () */


/* --------------------------------------------------------------------------*/
/*!Write a memory mapped half word register

   @param[in] addr   Memory mapped address
   @param[in] value  Value to set                                            */
/* --------------------------------------------------------------------------*/
static void
setreg16 (unsigned long int   addr,
	  unsigned short int  value)
{
  *((volatile unsigned short int *) addr) = value;
  printf ("Wrote half word at 0x%08lx = 0x%04x.\n", addr, value);

}	/* setreg16 () */


/* --------------------------------------------------------------------------*/
/*!Write a memory mapped full word register

   @param[in] addr   Memory mapped address
   @param[in] value  Value to set                                            */
/* --------------------------------------------------------------------------*/
static void
setreg32 (unsigned long int  addr,
	  unsigned long int  value)
{
  *((volatile unsigned long int *) addr) = value;
  printf ("Wrote full word at 0x%08lx = 0x%08lx.\n", addr, value);

}	/* setreg32 () */


/* --------------------------------------------------------------------------*/
/*!Read a memory mapped byte register

   @param[in] addr   Memory mapped address

   @return  Value read                                                       */
/* --------------------------------------------------------------------------*/
unsigned char
getreg8 (unsigned long int  addr)
{
  unsigned char  res = *((volatile unsigned char *) addr);
  printf ("Read byte at 0x%08lx = 0x%02x.\n", addr, res);
  return  res;

}	/* getreg8 () */


/* --------------------------------------------------------------------------*/
/*!Read a memory mapped half word register

   @param[in] addr   Memory mapped address

   @return  Value read                                                       */
/* --------------------------------------------------------------------------*/
unsigned short int
getreg16 (unsigned long int  addr)
{
  unsigned short int  res = *((volatile unsigned short int *) addr);
  printf ("Read half word at 0x%08lx = 0x%04x.\n", addr, res);
  return  res;

}	/* getreg16 () */


/* --------------------------------------------------------------------------*/
/*!Read a memory mapped full word register

   @param[in] addr   Memory mapped address

   @return  Value read                                                       */
/* --------------------------------------------------------------------------*/
unsigned long int
getreg32 (unsigned long int  addr)
{
  unsigned long int  res = *((volatile unsigned long int *) addr);
  printf ("Read full word at 0x%08lx = 0x%08lx.\n", addr, res);
  return  res;

}	/* getreg32 () */


/* --------------------------------------------------------------------------*/
/*! Exception handler for misalignment.

  Notes what has happened.                                                   */
/* --------------------------------------------------------------------------*/
void align_handler (void)
{
  printf ("Misaligned access.\n");

}	/* align_handler () */


/* --------------------------------------------------------------------------*/
/*!Main program to read and write memory mapped registers

   All these calls should be correctly aligned, but we set an handler, just in
   case.

   @return  The return code from the program (always zero).                  */
/* --------------------------------------------------------------------------*/
int
main ()
{
  /* Set the exception handler */
  printf ("Setting alignment exception handler.\n");
  excpt_align = (unsigned long)align_handler;

  /* Write some registers */
  printf ("Writing registers.\n");

  /* Byte aligned */
  setreg8 (GENERIC_BASE,     0xde);
  setreg8 (GENERIC_BASE + 1, 0xad);
  setreg8 (GENERIC_BASE + 2, 0xbe);
  setreg8 (GENERIC_BASE + 3, 0xef);

  /* Half word aligned */
  setreg16 (GENERIC_BASE + 4, 0xbaad);
  setreg16 (GENERIC_BASE + 6, 0xf00d);

  /* Full word aligned */
  setreg32 (GENERIC_BASE + 8, 0xcafebabe);

  /* Read some registers, but mix up which size from where. */
  printf ("Reading registers.\n");

  /* Full word aligned */
  (void)getreg32 (GENERIC_BASE + 0);

  /* Byte aligned */
  (void)getreg8 (GENERIC_BASE + 4);
  (void)getreg8 (GENERIC_BASE + 5);
  (void)getreg8 (GENERIC_BASE + 6);
  (void)getreg8 (GENERIC_BASE + 7);

  /* Half word aligned */
  (void)getreg16 (GENERIC_BASE +  8);
  (void)getreg16 (GENERIC_BASE + 10);

  /* All done */
  printf ("All done.\n");
  report (0xdeaddead);
  return 0;

}	/* main () */
