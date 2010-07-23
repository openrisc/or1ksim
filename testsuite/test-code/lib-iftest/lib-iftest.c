/* lib-iftest.c. Test of Or1ksim library interface functions.

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

#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#include "or1ksim.h"


/* --------------------------------------------------------------------------*/
/*!Main program

   Build an or1ksim program using the library which loads a program and config
   from the command line. Usage:

   lib-iftest <config-file> <image> <duration_ms>

   Run that test for a <duration_ms> milliseconds of simulated time. The test
   the various interface functions.

   @param[in] argc  Number of elements in argv
   @param[in] argv  Vector of program name and arguments

   @return  Return code for the program.                                     */
/* --------------------------------------------------------------------------*/
int
main (int   argc,
      char *argv[])
{
  /* Parse args */
  if (4 != argc)
    {
      fprintf (stderr,
	       "usage: lib-iftest <config-file> <image> <duration_ms>\n");
      return  1;
    }

  int     duration_ms = atoi (argv[3]);
  double  duration    = (double) duration_ms / 1.0e3;

  if (duration_ms <= 0)
    {
      fprintf (stderr, "ERROR. Duration must be positive number of ms\n");
      return  1;
    }

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

  /* Put the initialization message afterwards, or it will get swamped by the
     Or1ksim header. */
  if (0 == or1ksim_init (dummy_argc, dummy_argv, NULL, NULL, NULL))
    {
      printf ("Initalization succeeded.\n");
    }
  else
    {
      printf ("Initalization failed.\n");
      return  1;
    }

  /* Test of running */
  printf ("Running code...");
  switch (or1ksim_run (duration))
    {
    case OR1KSIM_RC_OK:    printf ("done.\n");           break;
    case OR1KSIM_RC_BRKPT: printf ("hit breakpoint.\n"); break;
    default:               printf ("failed.\n");         return  1;
    }


  /* Test of timing. Set a time point, run for the duration, then check the
     timing matches. */
  or1ksim_set_time_point ();
  printf ("Set time point.\n");
  printf ("Running code...");
  switch (or1ksim_run (duration))
    {
    case OR1KSIM_RC_OK:    printf ("done.\n");           break;
    case OR1KSIM_RC_BRKPT: printf ("hit breakpoint.\n"); break;
    default:               printf ("failed.\n");         return  1;
    }
  /* All done OK (within 1ps) */
  double measured_duration = or1ksim_get_time_period ();

  if (fabs (duration - measured_duration) < 1e-12)
    {
      printf ("Measured time period correctly.\n");
    }
  else
    {
      printf ("Failed. Requested period %.12f, but measured %.12f\n", duration,
	      measured_duration);
      return  1;
    }

  /* Test endianness */
  if (or1ksim_is_le ())
    {
      printf ("Little endian architecture.\n");
    }
  else
    {
      printf ("Big endian architecture.\n");
    }

  /* Check for clock rate */
  unsigned long int  clock_rate = or1ksim_clock_rate ();

  if (clock_rate > 0)
    {
      printf ("Clock rate %ld Hz.\n", clock_rate);
    }
  else
    {
      printf ("Invalid clock rate %ld Hz.\n", clock_rate);
      return  1;
    }

  printf ("Test completed successfully.\n");
  return  0;

}	/* main () */
