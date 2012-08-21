/* lib-upcalls.c. Test of Or1ksim library upcall interface.

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


/*! Memory mapped register memory */
static unsigned char *regv;

/*! Number of bytes of register memory */
static int  regc;

/*! Number of times each upcall has been called */
static int  upcall_count;


/* --------------------------------------------------------------------------*/
/*!Read upcall

   Upcall from Or1ksim to read a word from an external peripheral. Read the
   specified bytes from the peripheral if available.

   We only ever expect to get 4 byte reads here (modeling Wishbone).

   @note We receive the full address, since there is a single upcall routine,
         and this allows us to decode between potentially multiple devices.

   @todo We assume that the register memory size is a power of 2, making
         address shortening a simple modulo exercise. We should use a more
         generic solution.

   @note The mask is a byte mask. Since this is testing, we warn if any byte
         is not either 0xff or 0x00.

   @param[in]  class_ptr  A handle pass back from the initalization. Intended
                          for C++ callers, not used here.
   @param[in]  addr       Address to read from.
   @param[in]  mask       Byte mask for the read.
   @param[out] rdata      Buffer for the data read.
   @param[in]  data_len   Number of bytes in mask and rdata.

   @return  Zero on success, non-zero on failure.                            */
/* --------------------------------------------------------------------------*/
static int
read_upcall (void              *class_ptr,
	     unsigned long int  addr,
	     unsigned char      mask[],
	     unsigned char      rdata[],
	     int                data_len)
{
  unsigned long int  devaddr = addr % regc;

  upcall_count--;			/* One less upcall to do */

  if (4 != data_len)
    {
      printf ("Warning: 4-byte reads only supported for this device.\n");
      return  -1;
    }

  if (devaddr > regc - data_len)
    {
      printf ("Warning: read from 0x%08lx out of range: zero result.\n",
	      devaddr);
      return  -1;
    }

  /* Read the data */
  int  i;

  for (i = 0; i < 4; i++)
    {
      switch (mask[i])
	{
	case 0x00: rdata[i] = 0;                 break;
	case 0xff: rdata[i] = regv[devaddr + i]; break;

	default:
	  printf ("Warning: invalid mask byte %d for read 0x%02x: "
		  "treated as 0xff.\n", i, mask[i]);
	  rdata[i] = regv[devaddr + i];
	  break;
	}
    }

  /* printf ("Read from 0x%08lx: vector [%02x %02x %02x %02x ], " */
  /* 	  "mask [%02x %02x %02x %02x ].\n", devaddr, rdata[0], rdata[1], */
  /* 	  rdata[2], rdata[3], mask[0], mask[1], mask[2], mask[3]); */

  return  0;			/* Success */

}	/* read_upcall () */


/* --------------------------------------------------------------------------*/
/*!Write upcall

   Upcall from Or1ksim to write a word to an external peripheral. Read the
   specified bytes from the peripheral if available.

   We only ever expect to get 4 byte reads here (modeling Wishbone).

   @note We receive the full address, since there is a single upcall routine,
         and this allows us to decode between potentially multiple devices.

   @todo We assume that the register memory size is a power of 2, making
         address shortening a simple modulo exercise. We should use a more
         generic solution.

   @note The mask is a byte mask. Since this is testing, we warn if any byte
         is not either 0xff or 0x00.

   @param[in] class_ptr  A handle pass back from the initalization. Intended
                         for C++ callers, not used here.
   @param[in] addr       Address to write to.
   @param[in] mask       Byte mask for the write.
   @param[in] wdata      The data to write.
   @param[in] data_len   Number of bytes in mask and rdata.

   @return  Zero on success, non-zero on failure.                            */
/* --------------------------------------------------------------------------*/
static int
write_upcall (void              *class_ptr,
	      unsigned long int  addr,
	      unsigned char      mask[],
	      unsigned char      wdata[],
	      int                data_len)
{
  unsigned long int  devaddr  = addr % regc;

  upcall_count--;			/* One less upcall to do */

  if (4 != data_len)
    {
      printf ("Warning: 4-byte writes only supported for this device.\n");
      return  -1;
    }

  if (devaddr > regc - data_len)
    {
      printf ("Warning: write to 0x%08lx out of range: ignored.\n",
	      devaddr);
      return  -1;
    }

  /* printf ("Write to 0x%08lx: vector [%02x %02x %02x %02x ], " */
  /* 	  "mask [%02x %02x %02x %02x ].\n", devaddr, wdata[0], wdata[1], */
  /* 	  wdata[2], wdata[3], mask[0], mask[1], mask[2], mask[3]); */

  /* Write the data */
  int  i;

  for (i = 0; i < 4; i++)
    {
      switch (mask[i])
	{
	case 0x00:                               break;
	case 0xff: regv[devaddr + i] = wdata[i]; break;

	default:
	  printf ("Warning: invalid mask byte %d for write 0x%02x: "
		  "treated as 0xff.\n", i, mask[i]);
	  regv[devaddr + i] = wdata[i];
	  break;
	}
    }

  return  0;			/* Success */

}	/* write_upcall () */


/* --------------------------------------------------------------------------*/
/*!Main program

   Build an or1ksim program using the library which loads a program and config
   from the command line which will drive upcalls.

   lib-upcalls <config-file> <image> <upcall_count> <reg_bytes> [<reg_file>]

   A register memory of <reg_bytes> bytes is initalized from <reg_file> if
   present, or zeroed if not. <image> is run continuously, making upcalls,
   which are satisfied using the register memory. The program exits
   successfully when <upcall_count> upcalls have been made.

   @param[in] argc  Number of elements in argv
   @param[in] argv  Vector of program name and arguments

   @return  Return code for the program.                                     */
/* --------------------------------------------------------------------------*/
int
main (int   argc,
      char *argv[])
{
  char *reg_file;

  /* Parse args */
  switch (argc)
    {
    case 5:
      reg_file = NULL;
      break;

    case 6:
      reg_file = argv[5];
      break;

    default:
      printf ("usage: lib-upcalls <config-file> <image> "
	      "<upcall_count> <reg_bytes> [<reg_file>]\n");
      return  1;
    }

  upcall_count = atoi (argv[3]);

  if (upcall_count <= 0)
    {
      printf ("ERROR: Upcall count must be positive\n");
      return  1;
    }

  regc = atoi (argv[4]);

  if (regc < 0)
    {
      printf ("ERROR: Register memory size must be positive\n");
      return  1;
    }

  /* Read the register file if provided. */
  regv = malloc (regc * sizeof (unsigned char));

  if (NULL == regv)
    {
      printf ("ERROR: Failed to allocate register memory\n");
      return  1;
    }

  int  next_free_byte = 0;

  if (NULL != reg_file)
    {
      FILE *fd = fopen (reg_file, "r");

      if (NULL == fd)
	{
	  printf ("ERROR: Failed to open register file: %s\n",
		  strerror (errno));
	  free (regv);
	  return  1;
	}

      for (; next_free_byte < regc; next_free_byte++)
	{
	  unsigned char  byte;

	  if (1 == fscanf (fd, "%2hhx ", &byte))
	    {
	      regv[next_free_byte] = byte;
	    }
	  else
	    {
	      break;
	    }
	}

      /* Should have read the whole file successfully. */
      if (ferror (fd))
	{
	  printf ("ERROR: Reading register file: %s\n", strerror (errno));
	  free (regv);
	  return  1;
	}
      
      if (!feof (fd))
	{
	  printf ("Warning: additional register file data ignored.\n");
	}

      if (0 != fclose (fd))
	{
	  printf ("ERROR: Failed to close register file: %s\n",
		  strerror (errno));
	  free (regv);
	  return  1;
	}
    }

  /* Fill in any remaining bytes with zero */
  if (next_free_byte < regc)
    {
      (void)memset (&(regv[next_free_byte]), 0, regc - next_free_byte);
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
  if (0 == or1ksim_init (dummy_argc, dummy_argv, NULL, &read_upcall,
			 &write_upcall))
    {
      printf ("Initalization succeeded.\n");
    }
  else
    {
      printf ("Initalization failed.\n");
      return  1;
    }

  /* Run repeatedly for 1 millisecond until we hit a breakpoint or all upcalls
     are done. */
  do
    {
      switch (or1ksim_run (1.0e-3))
	{
	case OR1KSIM_RC_OK:
	  break;

	case OR1KSIM_RC_HALTED:
	  printf ("Test completed successfully: hit exit l.nop.\n");
	  return  0;

	case OR1KSIM_RC_BRKPT:
	  printf ("Test completed successfully: hit breakpoint.\n");
	  return  0;
	
	default:
	  printf ("ERROR: run failed\n");
	  return  1;
	} 
    }
  while (upcall_count > 0);

  /* A little longer to allow response to last upcall to be handled. */
  switch (or1ksim_run (1.0e-3))
    {
    case OR1KSIM_RC_OK:
      printf ("Test completed successfully: All upcalls processed.\n");
      return  0;

    case OR1KSIM_RC_HALTED:
      printf ("Test completed successfully: hit exit l.nop.\n");
      return  0;

    case OR1KSIM_RC_BRKPT:
      printf ("Test completed successfully: hit breakpoint.\n");
      return  0;

    default:
      printf ("ERROR: run failed\n");
      return  1;
    }
}	/* main () */
