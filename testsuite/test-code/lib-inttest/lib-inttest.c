/* lib-inttest.c. Test of Or1ksim library interrupt funcs.

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
#include <string.h>

#include "or1ksim.h"


/*! Number of interrupts in PIC */
#define  NUM_INTS  32

/*! Number of the PICSR SPR. */
#define  SPR_PICSR  0x4802

/*! Flag indicating if we are using level sensitive interrupts */
static int  level_sensitive_p;

/*! Vector of interrupt numbers. */
static int *intv;

/*! Count of interrupts. */
static int  intc = 0;

/*! Next interrupt */
static int  next_int = 0;


/* --------------------------------------------------------------------------*/
/*!Read upcall

   Upcall from Or1ksim to read a word from an external peripheral. If called
   this will trigger an interrupt.

   We may get an end case where we are asked for an interrupt, but have no
   more to give. We silently ignore this - it is just due to allowing the
   target program enough time to finish.

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
  if (next_int < intc)
    {
      int  inum = intv[next_int++];

      printf ("Triggering interrupt %d\n", inum);

      if (level_sensitive_p)
	{
	  or1ksim_interrupt_set (inum);
	}
      else
	{
	  or1ksim_interrupt (inum);
	}
    }

  return  0;			/* Success */

}	/* read_upcall () */


/* --------------------------------------------------------------------------*/
/*!Write upcall

   Upcall from Or1ksim to write a word to an external peripheral. If called
   this will clear an interrupt. The data is the number of the interrupt to
   clear.

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
  /* Extract the inum */
  int inum = wdata[0];

  printf ("Clearing interrupt %d\n", inum);

  if (level_sensitive_p)
    {
      or1ksim_interrupt_clear (inum);
    }
  else
    {
      unsigned long int  picsr;
      
      if (!or1ksim_read_spr (SPR_PICSR, &picsr))
	{
	  fprintf (stderr, "ERROR failed to read PICSR: Exiting.\n");
	  exit (1);
	}
      
      picsr &= ~(1UL << inum);
      printf ("Cleared PICSR is 0x%08lx\n", picsr);
      
      if (!or1ksim_write_spr (SPR_PICSR, picsr))
	{
	  fprintf (stderr, "ERROR failed to write PICSR: Exiting.\n");
	  exit (1);
	}
    }

  return  0;			/* Success */

}	/* write_upcall () */


/* --------------------------------------------------------------------------*/
/*!Main program

   Build an or1ksim program using the library which loads a program and config
   from the command line, and then drives interrupts. Usage:

   lib-inttest <config-file> <image> -l|-e <int #> <int #> ...

   <image> is run continuously. It requests that the next interrupt event be
   triggered by a read upcall. Clearing an interrupt occurs when a write
   upcall is received.

   The third argument specifies whether the level senstive (-l) or edge
   triggered (-e) mechanism should be used for setting and clearing
   interrupts.

   @param[in] argc  Number of elements in argv
   @param[in] argv  Vector of program name and arguments

   @return  Return code for the program.                                     */
/* --------------------------------------------------------------------------*/
int
main (int   argc,
      char *argv[])
{
  const int  INT_OFF = 4;		/* Offset in argv of interrupts */
  int        i;				/* General counter */

  /* Parse args */
  if (argc < (INT_OFF + 1))
    {
      fprintf (stderr,
	       "usage: lib-inttest-level <config-file> <image> -l|-e "
	       "<int #> <int #> ...\n");
      return  1;
    }

  /* What type of interrupt? */
  if (0 == strcmp ("-l", argv[3]))
    {
      level_sensitive_p = 1;
    }
  else if (0 == strcmp ("-e", argv[3]))
    {
      level_sensitive_p = 0;
    }
  else
    {
      fprintf (stderr, "ERROR: Must specify -l or -e: exiting\n");
      exit (1);
    }

  /* Get all the interrupts sorted out into the vector. We hold them with one
     greater than absolute value, so we can distinguish between +0 and -0. */
  intv = malloc ((argc - INT_OFF) * sizeof (int));

  for (i = INT_OFF; i < argc; i++)
    {
      int  inum = atoi (argv[i]);

      if ((inum <= 0) || (NUM_INTS <= inum))
      {
	printf ("Warning: Invalid interrupt # %d to raise: Exiting.\n", inum);
	exit (1);
      }

      intv[i - INT_OFF] = inum;
    }

  /* Global counters */
  intc     = argc - INT_OFF;
  next_int = 0;

  /* Dummy argv array to pass arguments to or1ksim_init. Varies depending on
     whether an image file is specified. */
  int   dummy_argc;
  char *dummy_argv[5];

  dummy_argv[0] = "libsim";
  dummy_argv[1] = "-q";
  dummy_argv[2] = "-f";
  dummy_argv[3] = argv[1];
  dummy_argv[4] = argv[2];

  dummy_argc = 5;

  /* Initialize the program. Put the initialization message afterwards, or it
     will get swamped by the Or1ksim header. */
  if (0 == or1ksim_init (dummy_argc, dummy_argv, NULL, &read_upcall,
			 &write_upcall))
    {
      printf ("Initalization succeeded.\n");
    }
  else
    {
      printf ("ERROR: Initalization failed.\n");
      free (intv);
      exit (1);
    }

  /* Run in bursts of 1ms until all interrupts have been set and cleared. */
  do
    {
      if (OR1KSIM_RC_OK != or1ksim_run (1.0e-3))
	{
	  printf ("ERROR: run failed\n");
	  free (intv);
	  exit (1);
	}
    }
  while (next_int < intc);

  /* One more burst to allow the device driver time to complete. */
  if (OR1KSIM_RC_OK != or1ksim_run (1.0e-3))
    {
      printf ("ERROR: run failed\n");
      free (intv);
      exit (1);
    }

  /* All interrupts should have been handled. */
  free (intv);

  printf ("Test completed successfully.\n");
  return  0;

}	/* main () */
