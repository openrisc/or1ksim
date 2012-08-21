/* lib-jtag-full.c. Comprehensive test of Or1ksim library JTAG interface.

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
   with this program.  If not, see <http://www.gnu.org/licenses/>.  */

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
/*!Compute a IEEE 802.3 CRC-32.

   Print an error message if we get a duff argument, but we really should
   not.

   @param[in] value     The value to shift into the CRC
   @param[in] num_bits  The number of bits in the value.
   @param[in] crc_in    The existing CRC

   @return  The computed CRC.                                                */
/* --------------------------------------------------------------------------*/
static unsigned long int
crc32 (unsigned long long int  value,
       int                     num_bits,
       unsigned long int       crc_in)
{
  if ((1 > num_bits) || (num_bits > 64))
    {
      printf ("ERROR: Max 64 bits of CRC can be computed. Ignored\n");
      return  crc_in;
    }

  static const unsigned long int  CRC32_POLY = 0x04c11db7;
  int                             i;

  /* Compute the CRC, MS bit first */
  for (i = num_bits - 1; i >= 0; i--)
    {
      unsigned long int  d;
      unsigned long int  t;

      d = (1 == ((value >> i) & 1))   ? 0xfffffff : 0x0000000;
      t = (1 == ((crc_in >> 31) & 1)) ? 0xfffffff : 0x0000000;

      crc_in <<= 1;
      crc_in  ^= (d ^ t) & CRC32_POLY;
    }

  return  crc_in;

}	/* crc32 () */


/* --------------------------------------------------------------------------*/
/*!Reverse a value's bits

   @param[in] val  The value to reverse (up to 64 bits).
   @param[in] len  The number of bits to reverse.

   @return  The reversed value                                               */
/* --------------------------------------------------------------------------*/
static unsigned long long
reverse_bits (unsigned long long  val,
	      int                 len)
{
  if ((1 > len) || (len > 64))
    {
      printf ("ERROR: Cannot reverse %d bits. Returning zero\n", len);
      return  0;
    }

  /* Reverse the string */
  val = (((val & 0xaaaaaaaaaaaaaaaaULL) >>  1) |
	 ((val & 0x5555555555555555ULL) <<  1));
  val = (((val & 0xccccccccccccccccULL) >>  2) |
	 ((val & 0x3333333333333333ULL) <<  2));
  val = (((val & 0xf0f0f0f0f0f0f0f0ULL) >>  4) |
	 ((val & 0x0f0f0f0f0f0f0f0fULL) <<  4));
  val = (((val & 0xff00ff00ff00ff00ULL) >>  8) |
	 ((val & 0x00ff00ff00ff00ffULL) <<  8));
  val = (((val & 0xffff0000ffff0000ULL) >> 16) |
	 ((val & 0x0000ffff0000ffffULL) << 16));

  return  ((val >> 32) | (val << 32)) >> (64 - len);

}	/* reverse_bits () */


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

  printf ("%s:  0x", prefix);

  /* Dump each byte in turn */
  for (i = num_bytes - 1; i >=0; i--)
    {
      printf ("%02x", jreg[i]);
    }

  printf ("\n");

}	/* dump_jreg () */


/* --------------------------------------------------------------------------*/
/*!Process a JTAG instruction register

   Usage:

     INSTRUCTION <value>

   The single argument is a single hex digit, specifying the instruction
   value.

   Like all the JTAG instructions, it must be reversed, so it is shifted MS
   bit first.

   @param[in] next_jreg  Offset into argv of the next JTAG register hex
                         string.
   @param[in] argc       argc from the main program (for checking next_jreg).
   @param[in] argv       argv from the main program.

   @return  1 (TRUE) on success, 0 (FALSE) on failure.                       */
/* --------------------------------------------------------------------------*/
static int
process_instruction (int   next_jreg,
		     int   argc,
		     char *argv[])
{
  printf ("Shifting instruction.\n");

  /* Do we have the arg? */
  if (next_jreg >= argc)
    {
      printf ("ERROR: no instruction register value found.\n");
      return  0;
    }

  /* Is the argument in range? */
  unsigned long int  ival = strtoul (argv[next_jreg], NULL, 16);

  if (ival > 0xf)
    {
      printf ("ERROR: instruction value 0x%lx too large\n", ival);
      return  0;
    }

  /* Reverse the bits of the value */
  ival = reverse_bits (ival, 4);

  /* Allocate space and populate the register */
  unsigned char *jreg = malloc (1);

  if (NULL == jreg)
    {
      printf ("ERROR: malloc for instruction register failed.\n");
      return  0;
    }

  jreg[0] = ival;

  dump_jreg ("  shifting in", jreg, 1);

  double  t = or1ksim_jtag_shift_ir (jreg, 4);

  dump_jreg ("  shifted out", jreg, 1);
  printf ("  time taken:   %.12fs\n", t);

  free (jreg);
  return  1;			/* Completed successfully */

}	/* process_instruction () */

  
/* --------------------------------------------------------------------------*/
/*!Process a JTAG SELECT_MODULE debug data register

   Usage:

     SELECT_MODULE <value>

   The one argument is a single hex digit, specifying the module value.

   Like all the JTAG fields, it must be reversed, so it is shifted MS
   bit first. It also requires a 32-bit CRC.

   On return we get a status register and CRC.

   @param[in] next_jreg  Offset into argv of the next JTAG register hex
                         string.
   @param[in] argc       argc from the main program (for checking next_jreg).
   @param[in] argv       argv from the main program.

   @return  1 (TRUE) on success, 0 (FALSE) on failure.                       */
/* --------------------------------------------------------------------------*/
static int
process_select_module (int   next_jreg,
		       int   argc,
		       char *argv[])
{
  printf ("Selecting module.\n");

  /* Do we have the arg? */
  if (next_jreg >= argc)
    {
      printf ("ERROR: no module specified.\n");
      return  0;
    }

  /* Is the argument in range? */
  unsigned long int  module_id = strtoul (argv[next_jreg], NULL, 16);

  if (module_id > 0xf)
    {
      printf ("ERROR: module value 0x%lx too large\n", module_id);
      return  0;
    }

  /* Compute the CRC */
  unsigned long int  crc_in;

  crc_in = crc32 (1,         1, 0xffffffff);
  crc_in = crc32 (module_id, 4, crc_in);

  /* Reverse the fields */
  module_id = reverse_bits (module_id, 4);
  crc_in    = reverse_bits (crc_in, 32);

  /* Allocate space and initialize the register
     - 1 indicator bit
     - 4 module bits in
     - 32 bit CRC in
     - 4 bits status out
     - 32 bits CRC out

     Total 73 bits = 10 bytes */
  int            num_bytes = 10;
  unsigned char *jreg      = malloc (num_bytes);

  if (NULL == jreg)
    {
      printf ("ERROR: malloc for SELECT_MODULE register failed.\n");
      return  0;
    }

  memset (jreg, 0, num_bytes);

  jreg[0]  = 0x01;
  jreg[0] |= module_id << 1;
  jreg[0] |= crc_in << 5;
  jreg[1]  = crc_in >> 3;
  jreg[2]  = crc_in >> 11;
  jreg[3]  = crc_in >> 19;
  jreg[4]  = crc_in >> 27;

  /* Note what we are shifting in and shift it. */
  dump_jreg ("  shifting in", jreg, num_bytes);
  double  t = or1ksim_jtag_shift_dr (jreg, 32 + 4 + 32 + 4 + 1);

  /* Diagnose what we are shifting out. */
  dump_jreg ("  shifted out", jreg, num_bytes);

  /* Break out fields */
  unsigned char      status;
  unsigned long int  crc_out;

  status = ((jreg[4] >> 5) | (jreg[5] << 3)) & 0xf ;

  crc_out = ((unsigned long int) jreg[5] >>  1) |
            ((unsigned long int) jreg[6] <<  7) |
            ((unsigned long int) jreg[7] << 15) |
            ((unsigned long int) jreg[8] << 23) |
            ((unsigned long int) jreg[9] << 31);

  /* Reverse the fields */
  status  = reverse_bits (status,  4);
  crc_out = reverse_bits (crc_out, 32);

  /* Compute our own CRC */
  unsigned long int  crc_computed = crc32 (status, 4, 0xffffffff);

  /* Log the results */
  printf ("  status:       0x%01x\n", status);

  if (crc_out != crc_computed)
    {
      printf ("  CRC mismatch\n");
      printf ("    CRC out:      0x%08lx\n", crc_out);
      printf ("    CRC computed: 0x%08lx\n", crc_computed);
    }

  printf ("  time taken:   %.12fs\n", t);

  free (jreg);
  return  1;			/* Completed successfully */

}	/* process_select_module () */

  
/* --------------------------------------------------------------------------*/
/*!Process a JTAG WRITE_COMMAND debug data register

   Usage:

     WRITE_COMMAND <access_type> <address> <length>

   The argumens are all hex values:
   - access_type  Access type - 4 bits
   - address      32-bit address
   - length       number of bytes to transer up to 2^16.

   Like all the JTAG fields these must be reversed, so they are shifted MS bit
   first. They also require a 32-bit CRC.

   On return we get a status register and CRC.

   @param[in] next_jreg  Offset into argv of the next JTAG register hex
                         string.
   @param[in] argc       argc from the main program (for checking next_jreg).
   @param[in] argv       argv from the main program.

   @return  1 (TRUE) on success, 0 (FALSE) on failure.                       */
/* --------------------------------------------------------------------------*/
static int
process_write_command (int   next_jreg,
		       int   argc,
		       char *argv[])
{
  printf ("Processing WRITE_COMMAND.\n");

  /* Do we have the args */
  if (next_jreg + 3 > argc)
    {
      printf ("WRITE_COMMAND usage: WRITE_COMMAND <access_type> <address> "
	      "<length>\n");
      return  0;
    }

  /* Are the arguments in range? Remember the length we actually put in has 1
     subtracted. */
  unsigned long int  cmd         = 2;	/* WRITE_COMMAND */

  unsigned long int  access_type = strtoul (argv[next_jreg    ], NULL, 16);
  unsigned long int  addr        = strtoul (argv[next_jreg + 1], NULL, 16);
  unsigned long int  len         = strtoul (argv[next_jreg + 2], NULL, 16) - 1;

  if (access_type > 0xf)
    {
      printf ("ERROR: WRITE_COMMAND access type 0x%lx too large\n",
	      access_type);
      return  0;
    }

  if (addr > 0xffffffff)
    {
      printf ("ERROR: WRITE_COMMAND address 0x%lx too large\n", addr);
      return  0;
    }

  if ((len + 1) < 0x1)
    {
      printf ("ERROR: WRITE_COMMAND length 0x%lx too small\n", len + 1);
      return  0;
    }
  else if ((len + 1) > 0x10000)
    {
      printf ("ERROR: WRITE_COMMAND length 0x%lx too large\n", len + 1);
      return  0;
    }

  /* Compute the CRC */
  unsigned long int  crc_in;

  crc_in = crc32 (0,            1, 0xffffffff);
  crc_in = crc32 (cmd,          4, crc_in);
  crc_in = crc32 (access_type,  4, crc_in);
  crc_in = crc32 (addr,        32, crc_in);
  crc_in = crc32 (len,         16, crc_in);

  /* Reverse the fields */
  cmd         = reverse_bits (cmd,          4);
  access_type = reverse_bits (access_type,  4);
  addr        = reverse_bits (addr,        32);
  len         = reverse_bits (len,         16);
  crc_in      = reverse_bits (crc_in,      32);

  /* Allocate space and initialize the register
     -  1 indicator bit
     -  4 bits command in
     -  4 bits access type in
     - 32 bits address in
     - 16 bits length in
     - 32 bits CRC in
     -  4 bits status out
     - 32 bits CRC out

     Total 125 bits = 16 bytes */
  int            num_bytes = 16;
  unsigned char *jreg      = malloc (num_bytes);

  if (NULL == jreg)
    {
      printf ("ERROR: malloc for WRITE_COMMAND register failed.\n");
      return  0;
    }

  memset (jreg, 0, num_bytes);

  jreg[ 0]  = 0x0;

  jreg[ 0] |= cmd         << 1;

  jreg[ 0] |= access_type << 5;
  jreg[ 1]  = access_type >> 3;

  jreg[ 1] |= addr        <<  1;
  jreg[ 2]  = addr        >>  7;
  jreg[ 3]  = addr        >> 15;
  jreg[ 4]  = addr        >> 23;
  jreg[ 5]  = addr        >> 31;

  jreg[ 5] |= len         <<  1;
  jreg[ 6]  = len         >>  7;
  jreg[ 7]  = len         >> 15;

  jreg[ 7] |= crc_in      <<  1;
  jreg[ 8]  = crc_in      >>  7;
  jreg[ 9]  = crc_in      >> 15;
  jreg[10]  = crc_in      >> 23;
  jreg[11]  = crc_in      >> 31;

  /* Note what we are shifting in and shift it. */
  dump_jreg ("  shifting in", jreg, num_bytes);
  double  t = or1ksim_jtag_shift_dr (jreg, 32 + 4 + 32 + 16 + 32 + 4 + 4 + 1);

  /* Diagnose what we are shifting out. */
  dump_jreg ("  shifted out", jreg, num_bytes);

  /* Break out fields */
  unsigned char      status;
  unsigned long int  crc_out;

  status = (jreg[11] >> 1) & 0xf ;

  crc_out = ((unsigned long int) jreg[11] >>  5) |
            ((unsigned long int) jreg[12] <<  3) |
            ((unsigned long int) jreg[13] << 11) |
            ((unsigned long int) jreg[14] << 19) |
            ((unsigned long int) jreg[15] << 27);

  /* Reverse the fields */
  status  = reverse_bits (status,  4);
  crc_out = reverse_bits (crc_out, 32);

  /* Compute our own CRC */
  unsigned long int  crc_computed = crc32 (status, 4, 0xffffffff);

  /* Log the results */
  printf ("  status:       0x%01x\n", status);

  if (crc_out != crc_computed)
    {
      printf ("  CRC mismatch\n");
      printf ("    CRC out:      0x%08lx\n", crc_out);
      printf ("    CRC computed: 0x%08lx\n", crc_computed);
    }

  printf ("  time taken:   %.12fs\n", t);

  free (jreg);
  return  1;			/* Completed successfully */

}	/* process_write_command () */

  
/* --------------------------------------------------------------------------*/
/*!Process a JTAG READ_COMMAND debug data register

   Usage:

     READ_COMMAND

   There are no arguments. It is used to read back the values used in a prior
   WRITE_COMMAND.

   On return we get the access type, address, length, status register and CRC.

   @param[in] next_jreg  Offset into argv of the next JTAG register hex
                         string.
   @param[in] argc       argc from the main program (for checking next_jreg).
   @param[in] argv       argv from the main program.

   @return  1 (TRUE) on success, 0 (FALSE) on failure.                       */
/* --------------------------------------------------------------------------*/
static int
process_read_command (int   next_jreg,
		       int   argc,
		       char *argv[])
{
  printf ("Processing READ_COMMAND.\n");

  /* The only value on input is the READ_COMMAND command */
  unsigned long int  cmd         = 1;	/* READ_COMMAND */

  /* Compute the CRC */
  unsigned long int  crc_in;

  crc_in = crc32 (0,            1, 0xffffffff);
  crc_in = crc32 (cmd,          4, crc_in);

  /* Reverse the fields */
  cmd    = reverse_bits (cmd,     4);
  crc_in = reverse_bits (crc_in, 32);

  /* Allocate space and initialize the register
     -  1 indicator bit
     -  4 bits command in
     - 32 bits CRC in
     -  4 bits access type out
     - 32 bits address out
     - 16 bits length out
     -  4 bits status out
     - 32 bits CRC out

     Total 125 bits = 16 bytes */
  int            num_bytes = 16;
  unsigned char *jreg      = malloc (num_bytes);

  if (NULL == jreg)
    {
      printf ("ERROR: malloc for READ_COMMAND register failed.\n");
      return  0;
    }

  memset (jreg, 0, num_bytes);

  jreg[ 0]  = 0x0;

  jreg[0] |= cmd    << 1;

  jreg[0] |= crc_in <<  5;
  jreg[1]  = crc_in >>  3;
  jreg[2]  = crc_in >> 11;
  jreg[3]  = crc_in >> 19;
  jreg[4]  = crc_in >> 27;

  /* Note what we are shifting in and shift it. */
  dump_jreg ("  shifting in", jreg, num_bytes);
  double  t = or1ksim_jtag_shift_dr (jreg, 32 + 4 + 16 + 32 + 4 + 32 + 4 + 1);

  /* Diagnose what we are shifting out. */
  dump_jreg ("  shifted out", jreg, num_bytes);

  /* Break out fields */
  unsigned char      access_type;
  unsigned long int  addr;
  unsigned long int  len;
  unsigned char      status;
  unsigned long int  crc_out;

  access_type = ((jreg[4] >> 5) | (jreg[5] << 3)) & 0xf ;
  addr        = ((unsigned long int) jreg[ 5] >>  1) |
                ((unsigned long int) jreg[ 6] <<  7) |
                ((unsigned long int) jreg[ 7] << 15) |
                ((unsigned long int) jreg[ 8] << 23) |
                ((unsigned long int) jreg[ 9] << 31);

  len         = ((unsigned long int)  jreg[ 9]        >>  1) |
                ((unsigned long int)  jreg[10]        <<  7) |
                ((unsigned long int) (jreg[11] & 0x1) << 15);

  status      = (jreg[11] >> 1) & 0xf ;

  crc_out     = ((unsigned long int) jreg[11] >>  5) |
                ((unsigned long int) jreg[12] <<  3) |
                ((unsigned long int) jreg[13] << 11) |
                ((unsigned long int) jreg[14] << 19) |
                ((unsigned long int) jreg[15] << 27);

  /* Reverse the fields */

  access_type = reverse_bits (access_type,  4);
  addr        = reverse_bits (addr,        32);
  len         = reverse_bits (len,         16);
  status      = reverse_bits (status,       4);
  crc_out     = reverse_bits (crc_out,     32);

  /* Compute our own CRC */
  unsigned long int  crc_computed;

  crc_computed = crc32 (access_type,  4, 0xffffffff);
  crc_computed = crc32 (addr,        32, crc_computed);
  crc_computed = crc32 (len,         16, crc_computed);
  crc_computed = crc32 (status,       4, crc_computed);

  /* Log the results. Remember the length is 1 greater than the value
     returned. */
  printf ("  access_type:  0x%x\n",    access_type);
  printf ("  address:      0x%lx\n",   addr);
  printf ("  length:       0x%lx\n",   len + 1);
  printf ("  status:       0x%x\n",    status);

  if (crc_out != crc_computed)
    {
      printf ("  CRC mismatch\n");
      printf ("    CRC out:      0x%08lx\n", crc_out);
      printf ("    CRC computed: 0x%08lx\n", crc_computed);
    }

  printf ("  time taken:   %.12fs\n", t);

  free (jreg);
  return  1;			/* Completed successfully */

}	/* process_read_command () */

  
/* --------------------------------------------------------------------------*/
/*!Process a JTAG GO_COMMAND_WRITE debug data register

   Usage:

     GO_COMMAND_WRITE <data>

   The one argument is a string of bytes to be written, LS byte first.

   Like all the JTAG fields, each data byte must be reversed, so it is shifted
   MS bit first. It also requires a 32-bit CRC.

   On return we get a status register and CRC.

   @param[in] next_jreg  Offset into argv of the next JTAG register hex
                         string.
   @param[in] argc       argc from the main program (for checking next_jreg).
   @param[in] argv       argv from the main program.

   @return  1 (TRUE) on success, 0 (FALSE) on failure.                       */
/* --------------------------------------------------------------------------*/
static int
process_go_command_write (int   next_jreg,
			  int   argc,
			  char *argv[])
{
  printf ("Processing GO_COMMAND_WRITE.\n");

  /* Do we have the arg */
  if (next_jreg >= argc)
    {
      printf ("GO_COMMAND_WRITE usage: GO_COMMAND_WRITE <data>.\n");
      return  0;
    }

  /* Break out the fields, including the data string into a vector of bytes. */
  unsigned long int  cmd        = 0;	/* GO_COMMAND */

  char              *data_str   = argv[next_jreg];
  int                data_len   = strlen (data_str);
  int                data_bytes = data_len / 2;
  unsigned char     *data       = malloc (data_bytes);

  if (NULL == data)
    {
      printf ("ERROR: data malloc for GO_COMMAND_WRITE register failed.\n");
      return  0;
    }

  if (1 == (data_len % 2))
    {
      printf ("Warning: GO_COMMAND_WRITE odd char ignored\n");
    }

  int  i;

  for (i = 0; i < data_bytes; i++)
    {
      int            ch_off_ms = i * 2;
      int            ch_off_ls = i * 2 + 1;

      /* Get each nybble in turn, remembering that we may not have a MS nybble
	 if the data string has an odd number of chars. */
      data[i] = 0;

      int  j;

      for (j = ch_off_ms; j <= ch_off_ls; j++)
	{
	  char  c       = data_str[j];
	  int   dig_val = (('0' <= c) && (c <= '9')) ? c - '0' :
	                  (('a' <= c) && (c <= 'f')) ? c - 'a' + 10 :
	                  (('A' <= c) && (c <= 'F')) ? c - 'A' + 10 : -1;

	  if (dig_val < 0)
	    {
	      printf ("ERROR: Non-hex digit in data: %c\n", c);
	      free (data);
	      return  0;
	    }

	  data[i] = (data[i] << 4) | dig_val;
	}
    }

  /* Are the arguments in range? Remember the length we actually put in has 1
     subtracted. */

  /* Compute the CRC */
  unsigned long int  crc_in;

  crc_in = crc32 (0,   1, 0xffffffff);
  crc_in = crc32 (cmd, 4, crc_in);

  for (i = 0; i < data_bytes; i++)
    {
      crc_in = crc32 (data[i], 8, crc_in);
    }

  /* Reverse the fields */
  cmd = reverse_bits (cmd, 4);

  for (i = 0; i < data_bytes; i++)
    {
      data[i] = reverse_bits (data[i], 8);
    }

  crc_in = reverse_bits (crc_in,  32);

  /* Allocate space and initialize the register
     -              1 indicator bit
     -              4 bits command in
     - data_bytes * 8 bits access type in
     -             32 bits CRC in
     -              4 bits status out
     -             32 bits CRC out

     Total 73 + data_bytes * 8 bits = 10 + data_bytes bytes */
  int            num_bytes = 10 + data_bytes;
  unsigned char *jreg      = malloc (num_bytes);

  if (NULL == jreg)
    {
      printf ("ERROR: jreg malloc for GO_COMMAND_WRITE register failed.\n");
      free (data);
      return  0;
    }

  memset (jreg, 0, num_bytes);

  jreg[ 0]  = 0x0;
  jreg[ 0] |= cmd << 1;

  for (i = 0; i < data_bytes; i++)
    {
      jreg[i]     |= data[i] << 5;
      jreg[i + 1]  = data[i] >> 3;
    }

  jreg[data_bytes    ] |= crc_in <<  5;
  jreg[data_bytes + 1]  = crc_in >>  3;
  jreg[data_bytes + 2]  = crc_in >> 11;
  jreg[data_bytes + 3]  = crc_in >> 19;
  jreg[data_bytes + 4]  = crc_in >> 27;

  /* Note what we are shifting in and shift it. */
  dump_jreg ("  shifting in", jreg, num_bytes);
  double  t = or1ksim_jtag_shift_dr (jreg,
				     32 + 4 + 32 + data_bytes * 8 + 4 + 1);

  /* Diagnose what we are shifting out. */
  dump_jreg ("  shifted out", jreg, num_bytes);

  /* Break out fields */
  unsigned char      status;
  unsigned long int  crc_out;

  status = ((jreg[data_bytes + 4] >> 5) | (jreg[data_bytes + 5] << 3)) & 0xf ;

  crc_out = ((unsigned long int) jreg[data_bytes + 5] >>  1) |
            ((unsigned long int) jreg[data_bytes + 6] <<  7) |
            ((unsigned long int) jreg[data_bytes + 7] << 15) |
            ((unsigned long int) jreg[data_bytes + 8] << 23) |
            ((unsigned long int) jreg[data_bytes + 9] << 31);

  /* Reverse the fields */
  status  = reverse_bits (status,   4);
  crc_out = reverse_bits (crc_out, 32);

  /* Compute our own CRC */
  unsigned long int  crc_computed = crc32 (status, 4, 0xffffffff);

  /* Log the results */
  printf ("  status:       0x%01x\n", status);

  if (crc_out != crc_computed)
    {
      printf ("  CRC mismatch\n");
      printf ("    CRC out:      0x%08lx\n", crc_out);
      printf ("    CRC computed: 0x%08lx\n", crc_computed);
    }

  printf ("  time taken:   %.12fs\n", t);

  free (data);
  free (jreg);
  return  1;			/* Completed successfully */

}	/* process_go_command_write () */

  
/* --------------------------------------------------------------------------*/
/*!Process a JTAG GO_COMMAND_READ debug data register

   Usage:

     GO_COMMAND_READ <length>

   The one argument is a length in hex, specifying the number of bytes to be
   read.

   On return we get a status register and CRC.

   Like all JTAG fields, the CRC shifted in, the data read back, the status
   and CRC shifted out, must be reversed, since they are shifted in MS bit
   first and out LS bit first.

   @param[in] next_jreg  Offset into argv of the next JTAG register hex
                         string.
   @param[in] argc       argc from the main program (for checking next_jreg).
   @param[in] argv       argv from the main program.

   @return  1 (TRUE) on success, 0 (FALSE) on failure.                       */
/* --------------------------------------------------------------------------*/
static int
process_go_command_read (int   next_jreg,
			 int   argc,
			 char *argv[])
{
  printf ("Processing GO_COMMAND_READ.\n");

  /* Do we have the args */
  if (next_jreg >= argc)
    {
      printf ("GO_COMMAND_READ usage: GO_COMMAND_READ <length>\n");
      return  0;
    }

  /* Is the argument in range? Remember the length we actually put in has 1
     subtracted, so although it is a 16-bit field, it can be up to 2^16. */
  unsigned long int  cmd        = 0;	/* GO_COMMAND */
  unsigned long int  data_bytes = strtoul (argv[next_jreg], NULL, 16);

  if (data_bytes < 0)
    {
      printf ("ERROR: GO_COMMAND_READ length 0x%lx too small\n", data_bytes);
      return  0;
    }
  else if (data_bytes > 0x10000)
    {
      printf ("ERROR: GO_COMMAND_READ length 0x%lx too large\n", data_bytes);
      return  0;
    }

  /* Compute the CRC */
  unsigned long int  crc_in;

  crc_in = crc32 (0,   1, 0xffffffff);
  crc_in = crc32 (cmd, 4, crc_in);

  /* Reverse the fields */
  cmd    = reverse_bits (cmd, 4);
  crc_in = reverse_bits (crc_in,  32);

  /* Allocate space and initialize the register
     -              1 indicator bit
     -              4 bits command in
     -             32 bits CRC in
     - data_bytes * 8 bits access type out
     -              4 bits status out
     -             32 bits CRC out

     Total 73 + data_bytes * 8 bits = 10 + data_bytes bytes */
  int            num_bytes = 10 + data_bytes;
  unsigned char *jreg      = malloc (num_bytes);

  if (NULL == jreg)
    {
      printf ("ERROR: malloc forGO_COMMAND_READ register failed.\n");
      return  0;
    }

  memset (jreg, 0, num_bytes);

  jreg[0]  = 0x0;
  jreg[0] |= cmd << 1;

  jreg[0] |= crc_in <<  5;
  jreg[1]  = crc_in >>  3;
  jreg[2]  = crc_in >> 11;
  jreg[3]  = crc_in >> 19;
  jreg[4]  = crc_in >> 27;

  /* Note what we are shifting in and shift it. */
  dump_jreg ("  shifting in", jreg, num_bytes);
  double  t = or1ksim_jtag_shift_dr (jreg,
				     32 + 4 + data_bytes * 8 + 32 + 4 + 1);

  /* Diagnose what we are shifting out. */
  dump_jreg ("  shifted out", jreg, num_bytes);

  /* Break out fields */
  unsigned char     *data = malloc (data_bytes);
  unsigned char      status;
  unsigned long int  crc_out;

  if (NULL == data)
    {
      printf ("ERROR: data malloc for GO_COMMAND_READ register failed.\n");
      free (jreg);
      return  0;
    }

  int  i;

  for (i = 0; i < data_bytes; i++)
    {
      data[i] = ((jreg[i + 4] >> 5) | (jreg[i + 5] << 3)) & 0xff;
    }

  status = ((jreg[data_bytes + 4] >> 5) | (jreg[data_bytes + 5] << 3)) & 0xf ;

  crc_out = ((unsigned long int) jreg[data_bytes + 5] >>  1) |
            ((unsigned long int) jreg[data_bytes + 6] <<  7) |
            ((unsigned long int) jreg[data_bytes + 7] << 15) |
            ((unsigned long int) jreg[data_bytes + 8] << 23) |
            ((unsigned long int) jreg[data_bytes + 9] << 31);

  /* Reverse the fields */
  for (i = 0; i < data_bytes; i++)
    {
      data[i] = reverse_bits (data[i], 8);
    }

  status  = reverse_bits (status,   4);
  crc_out = reverse_bits (crc_out, 32);

  /* Compute our own CRC */
  unsigned long int  crc_computed = 0xffffffff;

  for (i = 0; i < data_bytes; i++)
    {
      crc_computed = crc32 (data[i], 8, crc_computed);
    }
  
  crc_computed = crc32 (status, 4, crc_computed);

  /* Log the results, remembering these are bytes, so endianness is not a
     factor here. Since the OR1K is big endian, the lowest numbered byte will
     be the least significant, and the first printed */
  printf ("  data:         ");

  for (i = 0; i < data_bytes; i++)
    {
      printf ("%02x", data[i]);
    }

  printf ("\n");
  printf ("  status:       0x%01x\n", status);

  if (crc_out != crc_computed)
    {
      printf ("  CRC mismatch\n");
      printf ("    CRC out:      0x%08lx\n", crc_out);
      printf ("    CRC computed: 0x%08lx\n", crc_computed);
    }

  printf ("  time taken:   %.12fs\n", t);

  free (data);
  free (jreg);
  return  1;			/* Completed successfully */

}	/* process_go_command_read () */

  
/* --------------------------------------------------------------------------*/
/*!Process a JTAG WRITE_CONTROL debug data register

   Usage:

     WRITE_CONTROL <reset> <stall>

   The arguments should be either zero or one.

   The arguments are used to construct the 52-bit CPU control register. Like
   all JTAG fields, it must be reversed, so it is shifted MS bit first. It
   also requires a 32-bit CRC.

   On return we get a status register and CRC.

   @param[in] next_jreg  Offset into argv of the next JTAG register hex
                         string.
   @param[in] argc       argc from the main program (for checking next_jreg).
   @param[in] argv       argv from the main program.

   @return  1 (TRUE) on success, 0 (FALSE) on failure.                       */
/* --------------------------------------------------------------------------*/
static int
process_write_control (int   next_jreg,
		       int   argc,
		       char *argv[])
{
  printf ("Processing WRITE_CONTROL.\n");

  /* Do we have the args */
  if (next_jreg + 2 > argc)
    {
      printf ("WRITE_CONTROL usage: WRITE_CONTROL <reset> <status>\n");
      return  0;
    }

  /* Are the arguments in range? */
  unsigned long int  cmd   = 4;		/* WRITE_CONTROL */

  unsigned long int  reset = strtoul (argv[next_jreg    ], NULL, 16);
  unsigned long int  stall = strtoul (argv[next_jreg + 1], NULL, 16);

  if (reset > 0x1)
    {
      printf ("ERROR: invalid WRITE_CONTROL reset value 0x%lx.\n", reset);
      return  0;
    }

  if (stall > 0x1)
    {
      printf ("ERROR: invalid WRITE_CONTROL stall value 0x%lx.\n", stall);
      return  0;
    }

  /* Construct the control register */
  unsigned long long int  creg = ((unsigned long long int) reset << 51) |
                                 ((unsigned long long int) stall << 50);

  /* Compute the CRC */
  unsigned long int  crc_in;

  crc_in = crc32 (0,     1, 0xffffffff);
  crc_in = crc32 (cmd,   4, crc_in);
  crc_in = crc32 (creg, 52, crc_in);

  /* Reverse the fields */
  cmd    = reverse_bits (cmd,     4);
  creg   = reverse_bits (creg,   52);
  crc_in = reverse_bits (crc_in, 32);

  /* Allocate space and initialize the register
     -  1 indicator bit
     -  4 bits command in
     - 52 bits control register
     - 32 bits CRC in
     -  4 bits status out
     - 32 bits CRC out

     Total 125 bits = 16 bytes */
  int            num_bytes = 16;
  unsigned char *jreg      = malloc (num_bytes);

  if (NULL == jreg)
    {
      printf ("ERROR: malloc for WRITE_CONTROL register failed.\n");
      return  0;
    }

  memset (jreg, 0, num_bytes);

  jreg[ 0]  = 0x0;

  jreg[ 0] |= cmd    <<  1;

  jreg[ 0] |= creg   <<  5;
  jreg[ 1]  = creg   >>  3;
  jreg[ 2]  = creg   >> 11;
  jreg[ 3]  = creg   >> 19;
  jreg[ 4]  = creg   >> 27;
  jreg[ 5]  = creg   >> 35;
  jreg[ 6]  = creg   >> 43;
  jreg[ 7]  = creg   >> 51;

  jreg[ 7] |= crc_in      <<  1;
  jreg[ 8]  = crc_in      >>  7;
  jreg[ 9]  = crc_in      >> 15;
  jreg[10]  = crc_in      >> 23;
  jreg[11]  = crc_in      >> 31;

  /* Note what we are shifting in and shift it. */
  dump_jreg ("  shifting in", jreg, num_bytes);
  double  t = or1ksim_jtag_shift_dr (jreg, 32 + 4 + 32 + 52 + 4 + 1);

  /* Diagnose what we are shifting out. */
  dump_jreg ("  shifted out", jreg, num_bytes);

  /* Break out fields */
  unsigned char      status;
  unsigned long int  crc_out;

  status  = (jreg[11] >> 1) & 0xf ;

  crc_out = ((unsigned long int) jreg[11] >>  5) |
            ((unsigned long int) jreg[12] <<  3) |
            ((unsigned long int) jreg[13] << 11) |
            ((unsigned long int) jreg[14] << 19) |
            ((unsigned long int) jreg[15] << 27);

  /* Reverse the fields */
  status  = reverse_bits (status,  4);
  crc_out = reverse_bits (crc_out, 32);

  /* Compute our own CRC */
  unsigned long int  crc_computed = crc32 (status, 4, 0xffffffff);

  /* Log the results */
  printf ("  status:       0x%01x\n", status);

  if (crc_out != crc_computed)
    {
      printf ("  CRC mismatch\n");
      printf ("    CRC out:      0x%08lx\n", crc_out);
      printf ("    CRC computed: 0x%08lx\n", crc_computed);
    }

  printf ("  time taken:   %.12fs\n", t);

  free (jreg);
  return  1;			/* Completed successfully */

}	/* process_write_control () */

  
/* --------------------------------------------------------------------------*/
/*!Process a JTAG READ_CONTROL debug data register

   Usage:

     READ_CONTROL

   There are no arguments. It requires a 32-bit CRC.

   On return we get the control register, status and CRC.

   Like all the JTAG fields, they must be reversed, as resutl is shifted out
   LS bit first.

   @param[in] next_jreg  Offset into argv of the next JTAG register hex
                         string.
   @param[in] argc       argc from the main program (for checking next_jreg).
   @param[in] argv       argv from the main program.

   @return  1 (TRUE) on success, 0 (FALSE) on failure.                       */
/* --------------------------------------------------------------------------*/
static int
process_read_control (int   next_jreg,
		       int   argc,
		       char *argv[])
{
  printf ("Processing READ_CONTROL.\n");

  /* Only input field is cmd. */
  unsigned long int  cmd   = 3;		/* READ_CONTROL */

  /* Compute the CRC */
  unsigned long int  crc_in;

  crc_in = crc32 (0,     1, 0xffffffff);
  crc_in = crc32 (cmd,   4, crc_in);

  /* Reverse the fields */
  cmd    = reverse_bits (cmd,     4);
  crc_in = reverse_bits (crc_in, 32);

  /* Allocate space and initialize the register
     -  1 indicator bit
     -  4 bits command in
     - 32 bits CRC in
     - 52 bits control register out
     -  4 bits status out
     - 32 bits CRC out

     Total 125 bits = 16 bytes */
  int            num_bytes = 16;
  unsigned char *jreg      = malloc (num_bytes);

  if (NULL == jreg)
    {
      printf ("ERROR: malloc for READ_CONTROL register failed.\n");
      return  0;
    }

  memset (jreg, 0, num_bytes);

  jreg[0]  = 0x0;

  jreg[0] |= cmd    <<  1;

  jreg[0] |= crc_in <<  5;
  jreg[1]  = crc_in >>  3;
  jreg[2]  = crc_in >> 11;
  jreg[3]  = crc_in >> 19;
  jreg[4]  = crc_in >> 27;

  /* Note what we are shifting in and shift it. */
  dump_jreg ("  shifting in", jreg, num_bytes);
  double  t = or1ksim_jtag_shift_dr (jreg, 32 + 4 + 52 + 32 + 4 + 1);

  /* Diagnose what we are shifting out. */
  dump_jreg ("  shifted out", jreg, num_bytes);

  /* Break out fields */
  unsigned long long int  creg;
  unsigned char           status;
  unsigned long int       crc_out;

  creg    = ((unsigned long long int)  jreg[ 4]        >>  5) |
            ((unsigned long long int)  jreg[ 5]        <<  3) |
            ((unsigned long long int)  jreg[ 6]        << 11) |
            ((unsigned long long int)  jreg[ 7]        << 19) |
            ((unsigned long long int)  jreg[ 8]        << 27) |
            ((unsigned long long int)  jreg[ 9]        << 35) |
            ((unsigned long long int)  jreg[10]        << 43) |
            ((unsigned long long int) (jreg[11] & 0x1) << 51);

  status  = (jreg[11] >> 1) & 0xf ;

  crc_out = ((unsigned long int) jreg[11] >>  5) |
            ((unsigned long int) jreg[12] <<  3) |
            ((unsigned long int) jreg[13] << 11) |
            ((unsigned long int) jreg[14] << 19) |
            ((unsigned long int) jreg[15] << 27);

  /* Reverse the fields */
  creg    = reverse_bits (creg,    52);
  status  = reverse_bits (status,   4);
  crc_out = reverse_bits (crc_out, 32);

  /* Compute our own CRC */
  unsigned long int  crc_computed;

  crc_computed = crc32 (creg,   52, 0xffffffff);
  crc_computed = crc32 (status,  4, crc_computed);

  const char *reset = (1 == ((creg >> 51) & 1)) ? "enabled" : "disabled";
  const char *stall = (1 == ((creg >> 50) & 1)) ? "stalled" : "unstalled";

  /* Log the results */
  printf ("  reset:        %s\n", reset);
  printf ("  stall:        %s\n", stall);
  printf ("  status:       0x%01x\n", status);

  if (crc_out != crc_computed)
    {
      printf ("  CRC mismatch\n");
      printf ("    CRC out:      0x%08lx\n", crc_out);
      printf ("    CRC computed: 0x%08lx\n", crc_computed);
    }

  printf ("  time taken:   %.12fs\n", t);

  free (jreg);
  return  1;			/* Completed successfully */

}	/* process_read_control () */

  
/* --------------------------------------------------------------------------*/
/*!Main program

   Build an or1ksim program using the library which loads a program and config
   from the command line and then drives JTAG.

   lib-jtag-full <config-file> <image> <jregtype> [<args>]
                 [<jregtype> [<args>]] ...

   - config-file  An Or1ksim configuration file.
   - image        A OpenRISC binary image to load into Or1ksim
   - jregtype     One of RESET, INSTRUCTION, SELECT_MODULE, WRITE_COMMAND,
                  READ_COMMAND, GO_COMMAND_WRITE, GO_COMMAND_READ,
                  WRITE_CONTROL or READ_CONTROL.
   - args         Arguments required by the jregtype. RESET, READ_COMMAND and
                  READ_CONTROL require none.

   The target program is run in bursts of 1ms execution, and the type of
   return (OK, hit breakpoint) noted. Between each burst of execution, the
   JTAG interface is reset (for RESET) or the next register is submitted to
   the corresponding Or1ksim JTAG interface and the resulting register noted.

   @param[in] argc  Number of elements in argv
   @param[in] argv  Vector of program name and arguments

   @return  Return code for the program, zero on success.                    */
/* --------------------------------------------------------------------------*/
int
main (int   argc,
      char *argv[])
{
  const double  QUANTUM = 5.0e-3;	/* Time in sec for each step. */

  /* Check we have minimum number of args. */
  if (argc < 4)
    {
      printf ("usage: lib-jtag <config-file> <image> <jregtype> [<args>] "
	      "[<jregtype> [<args>]] ...\n");
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

  /* Run repeatedly for 10 milliseconds until we have processed all JTAG
     registers */
  int  next_jreg = 3;			/* Offset to next JTAG register */

  do
    {
      switch (or1ksim_run (QUANTUM))
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

      /* Process the next register appropriately, skipping any args after
	 processing. */
      char *jregtype = argv[next_jreg++];

      if (0 == strcasecmp ("RESET", jregtype))
	{
	  printf ("Resetting JTAG.\n");
	  or1ksim_jtag_reset ();
	}
      else if (0 == strcasecmp ("INSTRUCTION", jregtype))
	{
	  if (process_instruction (next_jreg, argc, argv))
	    {
	      next_jreg++;		/* succeeded */
	    }
	  else
	    {
	      return  1;		/* failed */
	    }
	}
      else if (0 == strcasecmp ("SELECT_MODULE", jregtype))
	{
	  if (process_select_module (next_jreg, argc, argv))
	    {
	      next_jreg++;		/* succeeded */
	    }
	  else
	    {
	      return  1;		/* failed */
	    }
	}
      else if (0 == strcasecmp ("WRITE_COMMAND", jregtype))
	{
	  if (process_write_command (next_jreg, argc, argv))
	    {
	      next_jreg += 3;		/* succeeded */
	    }
	  else
	    {
	      return  1;		/* failed */
	    }
	}
      else if (0 == strcasecmp ("READ_COMMAND", jregtype))
	{
	  if (process_read_command (next_jreg, argc, argv))
	    {
					/* succeeded (no args) */
	    }
	  else
	    {
	      return  1;		/* failed */
	    }
	}
      else if (0 == strcasecmp ("GO_COMMAND_WRITE", jregtype))
	{
	  if (process_go_command_write (next_jreg, argc, argv))
	    {
	      next_jreg++;		/* succeeded */
	    }
	  else
	    {
	      return  1;		/* failed */
	    }
	}
      else if (0 == strcasecmp ("GO_COMMAND_READ", jregtype))
	{
	  if (process_go_command_read (next_jreg, argc, argv))
	    {
	      next_jreg++;		/* succeeded */
	    }
	  else
	    {
	      return  1;		/* failed */
	    }
	}
      else if (0 == strcasecmp ("WRITE_CONTROL", jregtype))
	{
	  if (process_write_control (next_jreg, argc, argv))
	    {
	      next_jreg += 2;		/* succeeded */
	    }
	  else
	    {
	      return  1;		/* failed */
	    }
	}
      else if (0 == strcasecmp ("READ_CONTROL", jregtype))
	{
	  if (process_read_control (next_jreg, argc, argv))
	    {
					/* succeeded (no args) */
	    }
	  else
	    {
	      return  1;		/* failed */
	    }
	}
      else
	{
	  printf ("ERROR: Unrecognized JTAG register '%s'.\n", jregtype);
	  return  1;
	}
    }
  while (next_jreg < argc);

  /* A little longer to allow response to last upcall to be handled. */
  switch (or1ksim_run (QUANTUM))
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
