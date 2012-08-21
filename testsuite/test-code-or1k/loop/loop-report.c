/* loop-report.c. Or1ksim simple C loop program which reports changes.

   Copyright (C) 1999-2006 OpenCores
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

/* A program which initializes memory and SPR, then just loops. Used in
   testing libor1ksim JTAG read functionality. */

#include "support.h"
#include "spr-defs.h"

/*---------------------------------------------------------------------------*/
/*!main program

   We have two blocks of memory of interest (0x00000000 - 0x00200000,
   0xffe0000 - 0xffffffff) and two SPRs of interest (MACLO, 0x2801 and MACHI,
   0x2802). We monitor for any changes in the first 16 and last 16 bytes of
   each memory block, the middle 16 of the first memory block and both SPRs,
   having first initialized them to zero.

   We have to be careful about strobing. We sit in a loop looking for changes,
   but only print out the changes after we have had one loop with no changes.

   @return  The return code from the program (of no interest to us).         */
/*---------------------------------------------------------------------------*/
int
main ()
{
  /* Useful constants */
  unsigned char *b0_start_addr = (unsigned char *) 0x00000000;
  unsigned char *b0_mid_addr   = (unsigned char *) 0x00100000;
  unsigned char *b0_end_addr   = (unsigned char *) 0x001ffff0;
  unsigned char *b1_start_addr = (unsigned char *) 0xffe00000;
  unsigned char *b1_end_addr   = (unsigned char *) 0xfffffff0;
   
  /* General purpose */
  unsigned long int  r;
  int                i;

  /* Values remembered */
  unsigned long int  maclo;
  unsigned long int  machi;

  unsigned char      b0_start[16];
  unsigned char      b0_mid[16];
  unsigned char      b0_end[16];
  unsigned char      b1_start[16];
  unsigned char      b1_end[16];

  /* Flags indicating change */
  int  maclo_f;
  int  machi_f;

  int  b0_start_f[16];
  int  b0_mid_f[16];
  int  b0_end_f[16];
  int  b1_start_f[16];
  int  b1_end_f[16];

  int  changed_since_print;

  /* Set the SR to have SUMRA bit set, so that we can access certain regs in
     user mode. */
  r = mfspr (SPR_SR);
  mtspr (SPR_SR, r | SPR_SR_SUMRA);

  /* Initialize remembered values and flags */
  maclo = 0;
  machi = 0;

  maclo_f = 0;
  machi_f = 0;

  for (i = 0; i < 16; i++)
    {
      b0_start[i] = 0;
      b0_mid[i]   = 0;
      b0_end[i]   = 0;
      b1_start[i] = 0;
      b1_end[i]   = 0;

      b0_start_f[i] = 0;
      b0_mid_f[i]   = 0;
      b0_end_f[i]   = 0;
      b1_start_f[i] = 0;
      b1_end_f[i]   = 0;
    }

  /* Set the values in SPR and memory */
  mtspr (SPR_MACLO, maclo);
  mtspr (SPR_MACHI, machi);

  for (i = 0; i < 16; i++)
    {
      b0_start_addr[i] = b0_start[i];
      b0_mid_addr[i]   = b0_mid[i];
      b0_end_addr[i]   = b0_end[i];
      b1_start_addr[i] = b1_start[i];
      b1_end_addr[i]   = b1_end[i];
    }

  /* Loop for ever checking if any values have changed. */
  changed_since_print = 0;

  while (1)
    {
      int  changed_this_loop = 0;

      /* Check SPRs */
      if (mfspr (SPR_MACLO) != maclo)
	{
	  maclo               = mfspr (SPR_MACLO);
	  maclo_f             = 1;
	  changed_since_print = 1;
	  changed_this_loop   = 1;
	}

      if (mfspr (SPR_MACHI) != machi)
	{
	  machi               = mfspr (SPR_MACHI);
	  machi_f             = 1;
	  changed_since_print = 1;
	  changed_this_loop   = 1;
	}

      /* Check memory blocks */
      for (i = 0; i < 16; i++)
	{
	  if (b0_start_addr[i] != b0_start[i])
	    {
	      b0_start[i]         = b0_start_addr[i];
	      b0_start_f[i]       = 1;
	      changed_since_print = 1;
	      changed_this_loop   = 1;
	    }

	  if (b0_mid_addr[i] != b0_mid[i])
	    {
	      b0_mid[i]           = b0_mid_addr[i];
	      b0_mid_f[i]         = 1;
	      changed_since_print = 1;
	      changed_this_loop   = 1;
	    }

	  if (b0_end_addr[i] != b0_end[i])
	    {
	      b0_end[i]           = b0_end_addr[i];
	      b0_end_f[i]         = 1;
	      changed_since_print = 1;
	      changed_this_loop   = 1;
	    }

	  if (b1_start_addr[i] != b1_start[i])
	    {
	      b1_start[i]         = b1_start_addr[i];
	      b1_start_f[i]       = 1;
	      changed_since_print = 1;
	      changed_this_loop   = 1;
	    }

	  if (b1_end_addr[i] != b1_end[i])
	    {
	      b1_end[i]           = b1_end_addr[i];
	      b1_end_f[i]         = 1;
	      changed_since_print = 1;
	      changed_this_loop   = 1;
	    }
	}

      /* Only print out if there have been changes since the last print, but
	 not during this loop. This makes sure we don't strobe with writing
	 from JTAG. */
      if (changed_since_print && !changed_this_loop)
	{
	  /* Print any changed SPRs */
	  if (maclo_f)
	    {
	      printf ("New MACLO 0x%08lx\n", maclo);
	      maclo_f = 0;
	    }

	  if (machi_f)
	    {
	      printf ("New MACHI 0x%08lx\n", machi);
	      machi_f = 0;
	    }

	  /* Print any changed memory. Each in its own loop to give ascending
	     order. */
	  for (i = 0; i < 16; i++)
	    {
	      if (b0_start_f[i])
		{
		  printf ("New byte at 0x%08lx = 0x%02x\n",
			  (unsigned long int) &b0_start_addr[i], b0_start[i]);
		  b0_start_f[i] = 0;
		}
	    }

	  for (i = 0; i < 16; i++)
	    {
	      if (b0_mid_f[i])
		{
		  printf ("New byte at 0x%08lx = 0x%02x\n",
			  (unsigned long int) &b0_mid_addr[i], b0_mid[i]);
		  b0_mid_f[i] = 0;
		}
	    }

	  for (i = 0; i < 16; i++)
	    {
	      if (b0_end_f[i])
		{
		  printf ("New byte at 0x%08lx = 0x%02x\n",
			  (unsigned long int) &b0_end_addr[i], b0_end[i]);
		  b0_end_f[i] = 0;
		}
	    }

	  for (i = 0; i < 16; i++)
	    {
	      if (b1_start_f[i])
		{
		  printf ("New byte at 0x%08lx = 0x%02x\n",
			  (unsigned long int) &b1_start_addr[i], b1_start[i]);
		  b1_start_f[i] = 0;
		}
	    }

	  for (i = 0; i < 16; i++)
	    {
	      if (b1_end_f[i])
		{
		  printf ("New byte at 0x%08lx = 0x%02x\n",
			  (unsigned long int) &b1_end_addr[i], b1_end[i]);
		  b1_end_f[i] = 0;
		}
	    }

	  changed_since_print = 0;	/* Start it all again */
	}
    }
}	/* main () */
