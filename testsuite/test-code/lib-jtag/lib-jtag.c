/* lib-jtag.c. Basic test of Or1ksim library JTAG interface.

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

#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "or1ksim.h"


/* --------------------------------------------------------------------------*/
/*!Dump a JTAG register

   Prefix with the supplied string and add a newline afterwards.

   @param[in] prefix     Prefix string to print out
   @param[in] jreg       The JTAG register
   @param[in] num_bytes  The number of bytes in the register                 */
/* --------------------------------------------------------------------------*/
static void
dump_jreg (const char    *prefix,
	   unsigned char *jreg,
	   int            num_bytes)
{
  int  i;

  printf ("%s: ", prefix);

  /* Dump each byte in turn */
  for (i = num_bytes - 1; i >=0; i--)
    {
      printf ("%02x", jreg[i]);
    }

  printf ("\n");

}	/* dump_jreg () */


/* --------------------------------------------------------------------------*/
/*!Convert a hex char into its value.

   @param[in] c  The char to convert

   @return  The value represented by the char, or -1 if it's not a valid
            char.                                                            */
/* --------------------------------------------------------------------------*/
static int
hexch2val (char  c)
{
  switch (c)
    {
    case '0': return  0;
    case '1': return  1;
    case '2': return  2;
    case '3': return  3;
    case '4': return  4;
    case '5': return  5;
    case '6': return  6;
    case '7': return  7;
    case '8': return  8;
    case '9': return  9;

    case 'a': case 'A': return 10;
    case 'b': case 'B': return 11;
    case 'c': case 'C': return 12;
    case 'd': case 'D': return 13;
    case 'e': case 'E': return 14;
    case 'f': case 'F': return 15;

    default:
      return  -1;
    }
}	/* hexch2val () */


/* --------------------------------------------------------------------------*/
/*!Shift a JTAG register.

   Almost all this code is common between the instruction and data
   registers. All that varies is the library function called and the error
   message if anything goes wrong. So we common things up here.

   @param[in] type       'D' if this is a data register, 'I' if an instruction
                         register.
   @param[in] next_jreg  Offset into argv of the next JTAG register length
                         field.
   @param[in] argc       argc from the main program (for checking next_jref).
   @param[in] argv       argv from the main program.

   @return  1 (TRUE) on success, 0 (FALSE) on failure.                       */
/* --------------------------------------------------------------------------*/
static int
process_jreg (const char  type,
	      int         next_jreg,
	      int         argc,
	      char       *argv[])
{
  const char *long_name = ('D' == type) ? "data" : "instruction";

  /* Do we have the arg (length and value)? */
  if ((next_jreg + 1) > argc)
    {
      printf ("ERROR: no %s register found.\n", long_name);
      return  0;
    }

  /* Get the length field */
  int  bit_len = strtol (argv[next_jreg++], NULL, 0);

  if (0 == bit_len)
    {
      printf ("ERROR: invalid register length\n");
      return  0;
    }

  /* Is the reg an exact number of bytes? */
  char *hex_str   = argv[next_jreg];
  int   num_chars = strlen (hex_str);
  int   num_bytes = (bit_len + 7) / 8;

  if (num_chars > (2 * num_bytes))
    {
      printf ("Warning: Too many digits for register: truncated.\n");
    }

  /* Allocate and clear space */
  unsigned char *jreg = malloc (num_bytes);

  if (NULL == jreg)
    {
      printf ("ERROR: malloc for %s register failed.\n", long_name);
      return  0;
    }

  memset (jreg, 0, num_bytes);

  /* Initialize the register. The hex presentation is MS byte of the string on
     the left (i.e. at offset 0), but the internal representation is LS byte
     at the lowest address. */
  int  i;
  
  for (i = num_chars - 1; i >= 0; i--)
    {
      int  dig_num  = num_chars - 1 - i;	/* Which digit */
      int  dig_val  = hexch2val (hex_str[i]);

      if (dig_val < 0)
	{
	  printf ("ERROR: %c not valid hex digit.\n", hex_str[i]);
	  free (jreg);
	  return  0;
	}

      /* MS digits are the odd numbered ones */
      jreg[dig_num / 2] |= (0 == (dig_num % 2)) ? dig_val : dig_val << 4;
    }

  /* Note what we are doing */
  dump_jreg ("  shifting in", jreg, num_bytes);

  double  t;

  if ('D' == type)
    {
      t = or1ksim_jtag_shift_dr (jreg, bit_len);
    }
  else
    {
      t = or1ksim_jtag_shift_ir (jreg, bit_len);
    }

  dump_jreg ("  shifted out", jreg, num_bytes);
  printf ("  time taken %.12fs\n", t);

  free (jreg);
  return  1;			/* Completed successfully */

}	/* process_jreg () */

  
/* --------------------------------------------------------------------------*/
/*!Main program

   Build an or1ksim program using the library which loads a program and config
   from the command line which will drive JTAG.

   lib-jtag <config-file> <image> <jtype> [<bitlen> <reg>]
            [<jtype> [<bitlen> <reg>]] ...

   - config-file  An Or1ksim configuration file.
   - image        A OpenRISC binary image to load into Or1ksim
   - jtype        One of 'R' (JTAG reset), 'I' (JTAG instruction register) or
                  'D' (JTAG data register).
   - bitlen       If jtype is 'D' or 'I', the number of bits in the JTAG
                  register.
   - reg          If jtype is 'D' or 'I', a JTAG register specified in
                  hex. Specified LS digit on the right, and leading zeros may
                  be omitted.

   The target program is run in bursts of 1ms execution, and the type of
   return (OK, hit breakpoint) noted. Between each burst of execution, the
   next register is submitted to the corresponding Or1ksim JTAG interface
   function, and the resulting register (for 'I' and 'D') noted.

   @param[in] argc  Number of elements in argv
   @param[in] argv  Vector of program name and arguments

   @return  Return code for the program.                                     */
/* --------------------------------------------------------------------------*/
int
main (int   argc,
      char *argv[])
{
  /* Check we have minimum number of args. */
  if (argc < 4)
    {
      printf ("usage: lib-jtag <config-file> <image> <jtype> [<bitlen> <reg>] "
	      "[<jtype> [<bitlen> <reg>]] ...\n");
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

  /* Initialize the program. Put the initialization message afterwards, or it
     will get swamped by the Or1ksim header. */
  if (0 == or1ksim_init (dummy_argc, dummy_argv, NULL, NULL, NULL))
    {
      printf ("Initalization succeeded.\n");
    }
  else
    {
      printf ("Initalization failed.\n");
      return  1;
    }

  /* Run repeatedly for 1 millisecond until we have processed all JTAG
     registers */
  int  next_jreg = 3;			/* Offset to next JTAG register */

  do
    {
      switch (or1ksim_run (1.0e-3))
	{
	case OR1KSIM_RC_OK:
	  printf ("Execution step completed OK.\n");
	  break;

	case OR1KSIM_RC_BRKPT:
	  printf ("Execution step completed with breakpoint.\n");
	  break;

	default:
	  printf ("ERROR: run failed.\n");
	  return  1;
	}

      /* Process the next arg appropriately. */
      switch (argv[next_jreg++][0])
	{
	case 'R':
	  printf ("Resetting JTAG.\n");
	  or1ksim_jtag_reset ();
	  break;

	case 'I':
	  printf ("Shifting instruction register.\n");

	  if (process_jreg ('I', next_jreg, argc, argv))
	    {
	      next_jreg += 2;
	    }
	  else
	    {
	      return  1;		/* Something went wrong */
	    }
	  
	  break;

	case 'D':
	  printf ("Shifting data register.\n");

	  if (process_jreg ('D', next_jreg, argc, argv))
	    {
	      next_jreg += 2;
	    }
	  else
	    {
	      return  1;		/* Something went wrong */
	    }

	  break;

	default:
	  printf ("ERROR: unknown JTAG request type.\n");
	  return  1;
	}
    }
  while (next_jreg < argc);

  /* A little longer to allow response to last upcall to be handled. */
  switch (or1ksim_run (1.0e-3))
    {
    case OR1KSIM_RC_OK:
      printf ("Execution step completed OK.\n");
      break;

    case OR1KSIM_RC_BRKPT:
      printf ("Execution step completed with breakpoint.\n");
      break;

    default:
      printf ("ERROR: run failed.\n");
      return  1;
    }

  printf ("Test completed successfully.\n");
  return  0;

}	/* main () */
