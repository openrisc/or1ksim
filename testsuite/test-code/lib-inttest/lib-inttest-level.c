/* lib-inttest-level.c. Test of Or1ksim library level triggered interrupt funcs.

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

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

#include "or1ksim.h"


/*! Number of interrupts in PIC */
#define  NUM_INTS  32

/*! Vector of interrupts to be raised */
static int *raisev;

/*! Count of interrupts to be raised */
static int  raisec = 0;

/*! Next interrupt to be raised */
static int  next_raise = 0;

/*! Vector of interrupts to be cleared */
static int *clearv;

/*! Count of interrupts to be cleared */
static int  clearc = 0;

/*! Next interrupt to be cleared */
static int  next_clear = 0;


/* --------------------------------------------------------------------------*/
/*!Read upcall

   Upcall from Or1ksim to read a word from an external peripheral. If called
   this will raise the next interrupt.

   @param[in]  class_ptr  A handle pass back from the initalization. Intended
                          for C++ callers, not used here.
   @param[in]  addr       Address to read from. Ignored here.
   @param[in]  mask       Byte mask for the read. Ignored here.
   @param[out] rdata      Buffer for the data read. Ignored here.
   @param[in]  data_len   Number of bytes in mask and rdata.

   @return  Zero on success, non-zero on failure. Always zero here.          */
/* --------------------------------------------------------------------------*/
static int
read_upcall (void              *class_ptr,
	     unsigned long int  addr,
	     unsigned char      mask[],
	     unsigned char      rdata[],
	     int                data_len)
{
  /* Raise an interrupt if there is one left to raise. */
  if ((NULL != raisev) && (next_raise < raisec))
    {
      printf ("Raising interrupt %d\n", raisev[next_raise]);
      or1ksim_interrupt_set (raisev[next_raise++]);
    }

  return  0;			/* Success */

}	/* read_upcall () */


/* --------------------------------------------------------------------------*/
/*!Write upcall

   Upcall from Or1ksim to write a word to an external peripheral. If called
   this will clear the next interrupt.

   @param[in] class_ptr  A handle pass back from the initalization. Intended
                         for C++ callers, not used here.
   @param[in] addr       Address to write to. Ignored here.
   @param[in] mask       Byte mask for the write. Ignored here.
   @param[in] wdata      The data to write. Ignored here.
   @param[in] data_len   Number of bytes in mask and rdata.

   @return  Zero on success, non-zero on failure. Always zero here.          */
/* --------------------------------------------------------------------------*/
static int
write_upcall (void              *class_ptr,
	      unsigned long int  addr,
	      unsigned char      mask[],
	      unsigned char      wdata[],
	      int                data_len)
{
  /* Clear an interrupt if there is one left to clear. */
  if ((NULL != clearv) && (next_clear < clearc))
    {
      printf ("Clearing interrupt %d\n", clearv[next_clear]);
      or1ksim_interrupt_clear (clearv[next_clear++]);
    }

  return  0;			/* Success */

}	/* write_upcall () */


/* --------------------------------------------------------------------------*/
/*!Main program

   Build an or1ksim program using the library which loads a program and config
   from the command line, and then drives interrupts. Usage:

   lib-inttest-level <config-file> <image> +|-<int #> +|-<int #> ...

   <image> is run continuously. It requests that an interrupt be raised by a
   read upcall and cleared by a write upcall. The sequence of interrupts is
   specified to be raised (if preceded by '+') or cleared (if preceded by
   '-'). This allows the program to test the effect of clearing the wrong
   interrupt.

   @param[in] argc  Number of elements in argv
   @param[in] argv  Vector of program name and arguments

   @return  Return code for the program.                                     */
/* --------------------------------------------------------------------------*/
int
main (int   argc,
      char *argv[])
{
  /* Parse args */
  if (argc < 4)
    {
      fprintf (stderr,
	       "usage: lib-inttest-level <config-file> <image> "
	       "+|-<int #> +|-<int #> ...\n");
      return  1;
    }

  /* Get all the interrupts sorted out. Count them, then place them in a
     vector. */
  int  i;

  for (i = 3; i < argc; i++)
    {
      int  inum;

      switch (argv[i][0])
	{
	case '+':
	  inum = atoi (&(argv[i][1]));

	  if ((0 <= inum) && (inum < NUM_INTS))
	    {
	      raisec++;
	    }
	  else
	    {
	      printf ("Warning: Invalid interrupt # %d to raise.\n", inum);
	    }

	  break;

	case '-':
	  inum = atoi (&(argv[i][1]));

	  if ((0 <= inum) && (inum < NUM_INTS))
	    {
	      clearc++;
	    }
	  else
	    {
	      printf ("Warning: Invalid interrupt # %d to clear.\n", inum);
	    }

	  break;


	default:
	  printf ("Warning: No raise/clear specified - ignored\n");
	  break;
	}
    }

  if ((0 == raisec) && (0 == clearc))
    {
      printf ("ERROR: No interrupts specified.\n");
      return  1;
    }

  /* Allocate space and populate the vectors. */
  raisev = (raisec > 0) ? calloc (raisec, sizeof (int)) : NULL;
  clearv = (clearc > 0) ? calloc (clearc, sizeof (int)) : NULL;

  raisec = 0;
  clearc = 0;

  for (i = 3; i < argc; i++)
    {
      int  inum;

      switch (argv[i][0])
	{
	case '+':
	  inum = atoi (&(argv[i][1]));

	  if ((0 <= inum) && (inum < NUM_INTS))
	    {
	      raisev[raisec++] = inum;
	    }

	  break;

	case '-':
	  inum = atoi (&(argv[i][1]));

	  if ((0 <= inum) && (inum < NUM_INTS))
	    {
	      clearv[clearc++] = inum;
	    }

	  break;


	default:
	  printf ("Warning: No raise/clear specified - ignored\n");
	  break;
	}
    }

  /* Start the counts */
  next_raise = 0;
  next_clear = 0;

  /* Initialize the program. Put the initialization message afterwards, or it
     will get swamped by the Or1ksim header. */
  if (0 == or1ksim_init (argv[1], argv[2], NULL, &read_upcall, &write_upcall))
    {
      printf ("Initalization succeeded.\n");
    }
  else
    {
      printf ("Initalization failed.\n");
      free (raisev);
      free (clearv);
      return  1;
    }

  /* Run in bursts of 1ms until all interrupts have been set and cleared. */
  do
    {
      if (OR1KSIM_RC_OK != or1ksim_run (1.0e-3))
	{
	  printf ("ERROR: run failed\n");
	  free (raisev);
	  free (clearv);
	  return  1;
	}
    }
  while ((next_raise < raisec) || (next_clear < clearc));

  /* One more burst to allow the device driver time to complete. */
  if (OR1KSIM_RC_OK != or1ksim_run (1.0e-3))
    {
      printf ("ERROR: run failed\n");
      free (raisev);
      free (clearv);
      return  1;
    }

  /* All interrupts should have been handled. */
  free (raisev);
  free (clearv);

  printf ("Test completed successfully.\n");
  return  0;

}	/* main () */
