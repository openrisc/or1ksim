/* lib-inttest-edge.c. Test of Or1ksim library edge triggered interrupt funcs.

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

#include <stdlib.h>
#include <stdio.h>

#include "or1ksim.h"


/*! Number of interrupts in PIC */
#define  NUM_INTS  32

/*! Globally shared pointer to the interrupts */
char **intv;

/*! Number of interrupts to process */
int  intc;


/* --------------------------------------------------------------------------*/
/*!Trigger the next interrupt

   Generate the next edge triggered interrupt. Put out a message if doing so,
   and warn if the interrupt would be invalid.

   @return  1 (true) if an interrupt was triggered, 0 (false) otherwise.     */
/* --------------------------------------------------------------------------*/
static int
do_next_int ()
{
  static int  next_int = 0;		/* Next interrupt to process */

  while (next_int < intc)
    {
      int  inum = atoi (intv[next_int++]);

      if ((0 <= inum) && (inum < NUM_INTS))
	{
	  printf ("Triggering interrupt %d\n", inum);
	  or1ksim_interrupt (inum);
	  return  1;
	}
      else
	{
	  printf ("Warning: Invalid interrupt # %d - ignored\n", inum);
	}
    }

  return  0;				/* No more interrupts to trigger */

}	/* do_next_int () */


/* --------------------------------------------------------------------------*/
/*!Read upcall

   Upcall from Or1ksim to read a word from an external peripheral. If called
   this will trigger an interrupt.

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
  do_next_int ();

  return  0;			/* Success */

}	/* read_upcall () */


/* --------------------------------------------------------------------------*/
/*!Write upcall

   Upcall from Or1ksim to write a word to an external peripheral. If called
   this will trigger an interrupt.

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
  do_next_int ();

  return  0;			/* Success */

}	/* write_upcall () */


/* --------------------------------------------------------------------------*/
/*!Main program

   Build an or1ksim program using the library which loads a program and config
   from the command line, and then drives interrupts. Usage:

   lib-inttest-edge <config-file> <image> <duration_ms> <int #> <int #> ...

   <image> is run for repeated perios of <duration_ms>, during which it
   will make upcalls. Between each period and during each upcall an edge based
   interrupt will be triggered, until all interrupts listed have been
   triggered.

   @param[in] argc  Number of elements in argv
   @param[in] argv  Vector of program name and arguments

   @return  Return code for the program.                                     */
/* --------------------------------------------------------------------------*/
int
main (int   argc,
      char *argv[])
{
  /* Parse args */
  if (argc < 5)
    {
      fprintf (stderr,
	       "usage: lib-inttest-edge <config-file> <image> <duration_ms> "
	       "<int #> <int #> ...\n");
      return  1;
    }

  int     duration_ms = atoi (argv[3]);
  double  duration    = (double) duration_ms / 1.0e3;

  if (duration_ms <= 0)
    {
      fprintf (stderr, "ERROR. Duration must be positive number of ms\n");
      return  1;
    }

  /* Initialize the program. Put the initialization message afterwards, or it
     will get swamped by the Or1ksim header. */
  if (0 == or1ksim_init (argv[1], argv[2], NULL, &read_upcall, &write_upcall))
    {
      printf ("Initalization succeeded.\n");
    }
  else
    {
      printf ("Initalization failed.\n");
      return  1;
    }

  /* Do each interrupt in turn. Set up the shared pointer to the interrupts
     and counter. */
  intv = &(argv[4]);
  intc = argc - 4;

  do
    {
      /* Run the code */
      if (OR1KSIM_RC_OK != or1ksim_run (duration))
	{
	  printf ("ERROR: run failed\n");
	  return  1;
	}
    }
  while (do_next_int ());

  /* One more run, to allow interrupt handler enough time to finish
     processing. */
  if (OR1KSIM_RC_OK != or1ksim_run (duration))
    {
      printf ("ERROR: run failed\n");
      return  1;
    }

  /* All interrupts should have been handled. */
  printf ("Test completed successfully.\n");
  return  0;

}	/* main () */
