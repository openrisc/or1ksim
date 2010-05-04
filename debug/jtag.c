/* jtag.c -- JTAG modeling

   Copyright (C) 2008 Embecosm Limited

   Contributor Jeremy Bennett <jeremy.bennett@embecosm.com>

   This file is part of Or1ksim, the OpenRISC 1000 Architectural Simulator.

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

/* This program is commented throughout in a fashion suitable for processing
   with Doxygen. */


/* Autoconf and/or portability configuration */
#include "config.h"
#include "port.h"

/* System includes */
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* Package includes */
#include "sim-config.h"
#include "sprs.h"
#include "debug-unit.h"
#include "spr-defs.h"
#include "jtag.h"
#include "abstract.h"
#include "toplevel-support.h"


/*---------------------------------------------------------------------------*/
/*!Calculate an ongoing 32-bit CRC for a value

   Utility function. This is for a CRC, where the value is presented in normal
   order (i.e its most significant bit is at the most significant positions,
   it has not been reversed for incorporating ina JTAG register).

   The function can be used for values with more than 64 bits, but will treat
   the supplied value as the most-significant 64 bits, with all other bits
   zero. In practice this is only of use when shifting in a large number of
   zeros.

   The CRC used is the IEEE 802.3 32-bit CRC using the polynomial:

   x^32 + x^26 + x^23 +x^22 +x^16 + x^12 + x^11 + x^10 + x^8 + x^7 + x^5 +
   x^4 + x^2 + x^1 + 1

   In this implementation, the CRC is initialized to all ones (outside this
   function), to catch leading zeros.

   The function is designed to be used in a continuing calculation if
   desired. It takes a partially computed CRC and shifts in the appropriate
   new bits for the value supplied. For a new calculation, this value should
   be 0xffffffff.

  @param[in] value     The number whose CRC is wanted (MSB first)
  @param[in] num_bits  The number of bits to use from value
  @param[in] crc_in    The value of the CRC computed so far
  
  @return  The updated CRC value                                             */
/*---------------------------------------------------------------------------*/
static uint32_t
crc32 (uint64_t  value,
       int       num_bits,
       uint32_t  crc_in)
{
  static const uint32_t  CRC32_POLY = 0x04c11db7;

  /* Incorporate the bits, MS bit first. */
  int  i;

  for (i = num_bits - 1; i >= 0; i--)
    {
      uint32_t  d = (1 == ((value >> i) & 1))   ? 0xfffffff : 0x0000000;
      uint32_t  t = (1 == ((crc_in >> 31) & 1)) ? 0xfffffff : 0x0000000;

      crc_in <<= 1;
      crc_in  ^= (d ^ t) & CRC32_POLY;
    }

  return  crc_in;

}	/* crc32 () */


/*---------------------------------------------------------------------------*/
/*!Reverse up to 64 bits in a word

   Utility function. Uses the "reverse" algorithm from the University of
   Kentucky Aggregate Magic Algorithms.

   @param[in] val      The long word to reverse
   @param[in] numBits  Number of bits to reverse

   @return  The reversed word.                                               */
/*---------------------------------------------------------------------------*/
static uint64_t
reverse_bits (uint64_t  val,
	      int       numBits)
{
  val = (((val & 0xaaaaaaaaaaaaaaaaULL) >>  1) |
	 ((val & 0x5555555555555555ULL) <<  1));
  val = (((val & 0xccccccccccccccccULL) >>  2) |
	 ((val & 0x3333333333333333ULL) <<  2));
  val = (((val & 0xf0f0f0f0f0f0f0f0ULL) >>  4) |
	 ((val & 0x0f0f0f0f0f0f0f0fULL) <<  4));
  val = (((val & 0xff00ff00ff00ff00ULL) >>  8) |
	 ((val & 0x00ff00ff00ff00ffULL) <<  8));
  val = (((val & 0xffff0000ffff0000ULL) >> 16) |
	 ((val & 0x00000fff0000ffffULL) << 16));
  val = ((val >> 32) | (val << 32));

  return  val >> (64 - numBits);	/* Only the bits we want */

}	/* reverse_bits () */


/*---------------------------------------------------------------------------*/
/*!Reverse a byte

   Utility function. Uses the "reverse" algorithm from the University of
   Kentucky Aggregate Magic Algorithms.

   @param[in] byte  The byte to reverse

   @return  The reversed byte.                                               */
/*---------------------------------------------------------------------------*/
static uint8_t
reverse_byte (uint8_t  byte)
{
  byte = (((byte & 0xaa) >> 1) | ((byte & 0x55) << 1));
  byte = (((byte & 0xcc) >> 2) | ((byte & 0x33) << 2));

  return ((byte >> 4) | (byte << 4));

}	/* reverse_byte () */


/*---------------------------------------------------------------------------*/
/*!Construct a response register to shift out.

   The generic format is:

          +-------+--------+---------+---------+
          |       |        |         |         |   
   TDI -> |  CRC  | Status |  XX..XX |  00..00 | -> TDO
          |       |        |         |         |
          +-------+--------+---------+---------+
              32       4    "ignored"   "zero"
             bits    bits      bits      bits


   Fields are always shifted in MS bit first, so must be reversed. The CRC is
   on the 4 status bits. Only the "zero" bits are zeroed. This allows for
   other fields to be specified in the "ignored" bits region, which are
   guaranteed to be left untouched.

   @param[out] jreg       The buffer for the constructed register
   @param[in]  status     The status bits
   @param[in]  crc_in     CRC computed so far (set to 0xffffffff if none)
   @param[in]  ign_bits   The number of bits to ignore
   @param[in]  zero_bits  The number of bits to zero

   @return  The register length in bits                                      */
/*---------------------------------------------------------------------------*/
static int
construct_response (unsigned char *jreg,
		    uint8_t        status,
		    uint32_t       crc_in,
		    unsigned int   ign_bits,
		    unsigned int   zero_bits)
{
  /* Construct the outgoing CRC */
  uint32_t  crc_out = crc32 (status, 4, crc_in);

  /* Reversed versions of fields ready for insertion */
  uint8_t   status_r  = reverse_bits (status, 4);
  uint32_t  crc_out_r = reverse_bits (crc_out, 32);

  /* Construct the response register */
  unsigned int  zero_bytes = zero_bits / 8;
  unsigned int  zero_off   = zero_bits % 8;

  /* Clear the zero bits */
  if (zero_bytes > 0)
    {
      memset (jreg, 0, zero_bytes);
    }

  jreg[zero_bytes] >>= zero_off;
  jreg[zero_bytes] <<= zero_off;

  /* Determine how much to skip in total */
  unsigned int  skip_bytes = ign_bits + zero_bits / 8;
  unsigned int  bit_off    = ign_bits + zero_bits % 8;

  /* Simplify by dealing separately with two cases:
     - the bit offset is less than or equal to 4, so the status goes in the
       first free byte, with some CRC.
     - the bit offset is greater than 4 but less than 8, so the status goes in
       the first and second free bytes.

     For completeness we deal with what should be the impossible case of
     bit_off > 7. */
  if (bit_off <= 4)
    {
      /* Note that this works even if bit_off == 4, since there will be no CRC
	 remaining to OR into the first byte. */
      jreg[skip_bytes]     |=  ((status_r & 0xf) << bit_off) |
	                      ((crc_out_r << (4 + bit_off)) & 0xff);
      jreg[skip_bytes + 1]  =  (crc_out_r >> ( 4 - bit_off)) & 0xff;
      jreg[skip_bytes + 2]  =  (crc_out_r >> (12 - bit_off)) & 0xff;
      jreg[skip_bytes + 3]  =  (crc_out_r >> (20 - bit_off)) & 0xff;
      jreg[skip_bytes + 4]  =  (crc_out_r >> (28 - bit_off)) & 0xff;
    }
  else if (bit_off < 8)
    {
      jreg[skip_bytes]     |=   status_r << bit_off;
      jreg[skip_bytes + 1]  =  (status_r >> (8 - bit_off)) |
	                      ((crc_out_r << (bit_off - 4)) & 0xff);
      jreg[skip_bytes + 2]  =  (crc_out_r >> (12 - bit_off)) & 0xff;
      jreg[skip_bytes + 3]  =  (crc_out_r >> (20 - bit_off)) & 0xff;
      jreg[skip_bytes + 4]  =  (crc_out_r >> (28 - bit_off)) & 0xff;
      jreg[skip_bytes + 5]  =  (crc_out_r >> (36 - bit_off)) & 0xff;
    }
  else
    {
      fprintf (stderr, "*** ABORT ***: construct_response: impossible bit "
	       "offset: %u\n", bit_off);
      abort ();
    }

  /* Result is the register length in bits */
  return  32 + 4 + skip_bytes;

}	/* construct_response () */


/*---------------------------------------------------------------------------*/
/*!Select a module for debug

   Process a module selection register. The format is:

          +---------+-------+--------+---+
          |         |       |        |   |   
   TDI -> | Ignored |  CRC  | Module | 1 | -> TDO
          |         |       |   ID   |   |
          +---------+-------+--------+---+
              36        32       4
             bits      bits    bits

   The returned register has the format:

          +-------+--------+---------+
          |       |        |         |   
   TDI -> |  CRC  | Status |  00..00 | -> TDO
          |       |        |         |
          +-------+--------+---------+
              32       4        37
             bits    bits      bits


   Fields are always shifted in MS bit first, so must be reversed. The CRC in
   is computed on the first 5 bits, the CRC out on the 4 status bits.

   @param[in,out] jreg  The register to shift in, and the register shifted
                        back out.

   @return  The number of cycles the shift took, which in turn is the number
            of bits in the register                                          */
/*---------------------------------------------------------------------------*/
static int
module_select (unsigned char *jreg)
{
  /* Break out the fields */
  uint8_t   mod_id = reverse_bits ((jreg[0] >> 1) & 0xf, 4);
  uint32_t  crc_in = reverse_bits (((uint32_t )  jreg[0] & 0xe0  >>  5) |
	  		           ((uint32_t )  jreg[1]         <<  3) |
			           ((uint32_t )  jreg[2]         << 11) |
			           ((uint32_t )  jreg[3]         << 19) |
			           ((uint32_t ) (jreg[4] & 0x1f) << 27), 32);

  /* Compute the expected CRC */
  uint32_t  crc_computed;

  crc_computed = crc32 (1,      1, 0xffffffff);
  crc_computed = crc32 (mod_id, 4, crc_computed);

  /* Status flags */
  enum jtag_status  status = JS_OK;

  /* Validate the CRC */
  if (crc_computed != crc_in)
    {
      /* Mismatch: record the error */
      status |= JS_CRC_IN_ERROR;
    }
  else
    {
      /* Is it a valid module? */
      switch (mod_id)
	{
	case JM_WISHBONE:
	case JM_CPU0:
	case JM_CPU1:
	  /* All valid. Record the module */
	  runtime.debug.mod_id = mod_id;
	  
	  break;
	  
	default:
	  /* Bad module: record the error */
	  status |= JS_MODULE_MISSING;
	  break;
	}
    }

  /* Construct the outgoing register and return the JTAG cycles taken (the
     register length) */
  return  construct_response (jreg, status, 0xffffffff, 0, 37);

}	/* module_select */


/*---------------------------------------------------------------------------*/
/*!Validate WRITE_COMMAND fields for WishBone

   Check that a WRITE_COMMAND's fields are valid for WishBone access.

   - 16 and 32-bit access must be correctly aligned. If not a warning is
     printed and validation fails.

   - size must be a multiple of 2 for 16-bit access, and a multiple of 4 for
     32-bit access

   Validation is carried out when executing a GO_COMMAND, rather than when
   setting the WRITE_COMMAND, because only the GO_COMMAND can set the status
   error if there is a problem.

   Warning messages are printed to explain any validation problems.

   @todo Should multiple SPR accesses be allowed in a single access?

   @note The size of the data is one greater than the length specified in the
         original WRITE_COMMAND.

   @return  1 (TRUE) if validation is OK, 0 (FALSE) if validation fails.     */
/*---------------------------------------------------------------------------*/
static int
validate_wb_fields ()
{
  /* Determine the size of the access */
  uint32_t  access_bits;

  switch (runtime.debug.acc_type)
    {
    case JAT_WRITE8:
    case JAT_READ8:
      access_bits =  8;
      break;

    case JAT_WRITE16:
    case JAT_READ16:
      access_bits = 16;
      break;

    case JAT_WRITE32:
    case JAT_READ32:
      access_bits = 32;
      break;

    default:
      fprintf (stderr, "*** ABORT ***: validate_wb_fields: unknown access "
	       "type %u\n", runtime.debug.acc_type);
      abort ();
    }

  /* Check for validity. This works for 8-bit, although the tests will always
     pass. */
  uint32_t  access_bytes = access_bits / 8;

  if (0 != (runtime.debug.addr % access_bytes))
    {
      fprintf (stderr, "Warning: JTAG WishBone %d-bit access must be %d-byte "
	       "aligned\n", access_bits, access_bytes);
      return  0;
    }
  else if (0 != (runtime.debug.size % access_bytes))
    {
      fprintf (stderr, "Warning: JTAG %d-bit WishBone access must be multiple "
	       "of %d bytes in length\n", access_bits, access_bytes);
      return  0;
    }
  else
    {
      return  1;			/* No problems */
    }
}	/* validate_wb_fields () */


/*---------------------------------------------------------------------------*/
/*!Read WishBone data

   Read memory from WishBone. The WRITE_COMMAND address is updated to reflect
   the completed read if successful.

   The result is the CRC of the data read. The status flag is supplied as a
   pointer, since this can also be updated if there is a problem reading the
   data.

   @note The size of the data is one greater than the length specified in the
         original WRITE_COMMAND.

   @todo For now we always read a byte a time. In the future, we ought to use
         16 and 32-bit accesses for greater efficiency.

   @todo  The algorithm for ensuring we only set the bits of interest in the
          register is inefficient. We should instead clear the whole area
          before starting.

   @param[out] jreg        The JTAG register buffer where data is to be
                           stored.
   @param[in]  skip_bits   Bits to skip before storing data in jreg.
   @param[in]  status_ptr  Pointer to the status register.

   @return  The CRC of the data read                                         */
/*---------------------------------------------------------------------------*/
static uint32_t
wishbone_read (unsigned char    *jreg,
	       unsigned int      skip_bits,
	       enum jtag_status *status_ptr)
{
  /* Compute the CRC as we go */
  uint32_t  crc_out  = 0xffffffff;

  /* Validate the fields for the wishbone read. If this fails we stop here,
     setting an error in the status_ptr. */
  if (!validate_wb_fields())
    {
      *status_ptr |= JS_WISHBONE_ERROR;
      return  crc_out;
    }
      
  /* Transfer each byte in turn, computing the CRC as we go */
  unsigned  byte_off = skip_bits / 8;
  unsigned  bit_off  = skip_bits % 8;

  uint32_t  i;				/* Index into the data being read */

  for (i = 0; i < runtime.debug.size; i++)
    {
      /* Error if we can't access this byte */
      if (NULL == verify_memoryarea (runtime.debug.addr + i))
	{
	  *status_ptr |= JS_WISHBONE_ERROR;
	  return  crc_out;
	}

      /* Get the data with no cache or VM translation and update the CRC */
      unsigned char  byte = eval_direct8 (runtime.debug.addr + i, 0, 0);
      crc_out = crc32 (byte, 8, crc_out);

      /* Store the byte in the register, without trampling adjacent
	 bits. Simplified version when the bit offset is zero. */
      if (0 == bit_off)
	{
	  jreg[byte_off + i] = byte;
	}
      else
	{
	  /* Clear the bits (only) we are setting */
	  jreg[byte_off + i]     <<= bit_off;
	  jreg[byte_off + i]     >>= bit_off;
	  jreg[byte_off + i + 1] >>= 8 - bit_off;
	  jreg[byte_off + i + 1] <<= 8 - bit_off;

	  /* OR in the bits */
	  jreg[byte_off + i]     |= (byte <<      bit_off)  & 0xff;
	  jreg[byte_off + i + 1] |= (byte >> (8 - bit_off)) & 0xff;
	}
    }

    runtime.debug.addr += runtime.debug.size;

  return  crc_out;

}	/* wishbone_read () */


/*---------------------------------------------------------------------------*/
/*!Validate WRITE_COMMAND fields for SPR

   Check that a WRITE_COMMAND's fields are valid for SPR access. Only prints
   messages, since the protocol does not allow for any errors.

   - 8 and 16-bit access is only permitted for WishBone. If they are specified
     for SPR, they are treated as their 32 bit equivalent. A warning is
     printed.

   - size must be 1 word. If a larger value is specified that is treated as 1
     with a warning.

   - address must be less than MAX_SPRS. If a larger value is specified, it is
     reduced module MAX_SPRS (which is hopefully a power of 2).

   Where errors are found, the data is updated.

   Validation is carried out with the GO_COMMAND, rather than with the
   WRITE_COMMAND, for consistency with WishBone error checking, for which
   only the GO_COMMAND can set the status error if there is a problem.

   Warning messages are printed to explain any validation problems.

   @todo Should multiple SPR accesses be allowed in a single access?

   @note The size of the data is one greater than the length specified in the
         original WRITE_COMMAND.                                             */
/*---------------------------------------------------------------------------*/
static void
validate_spr_fields ()
{
  int  access_bits;
  int  is_read_p;

  switch (runtime.debug.acc_type)
    {
    case JAT_WRITE8:  access_bits =  8; is_read_p = 0;
    case JAT_READ8:   access_bits =  8; is_read_p = 1;
    case JAT_WRITE16: access_bits = 16; is_read_p = 0;
    case JAT_READ16:  access_bits = 16; is_read_p = 1;
    case JAT_WRITE32: access_bits = 32; is_read_p = 0;
    case JAT_READ32:  access_bits = 32; is_read_p = 1;

    default:
      fprintf (stderr, "*** ABORT ***: validate_spr_fields: unknown access "
	       "type %u\n", runtime.debug.acc_type);
      abort ();
    }

  /* Validate and correct if necessary access width */
  if (32 != access_bits)
    {
      fprintf (stderr, "Warning: JTAG %d-bit access not permitted for SPR: "
	       "corrected\n", access_bits);
      runtime.debug.acc_type = is_read_p ? JAT_READ32 : JAT_WRITE32;
    }

  /* Validate and correct if necessary access size */
  if (1 != runtime.debug.size)
    {
      fprintf (stderr, "warning: JTAG SPR access must be 1 word in length: "
	       "corrected\n");
      runtime.debug.size = 1;
    }
  /* Validate and correct if necessary access size */
  if (runtime.debug.addr >= MAX_SPRS)
    {
      fprintf (stderr, "warning: JTAG SPR address exceeds MAX_SPRS: "
	       "truncated\n");
      runtime.debug.addr %= MAX_SPRS;
    }
}	/* validate_spr_fields () */


/*---------------------------------------------------------------------------*/
/*!Read SPR data

   Read memory from WishBone. The WRITE_COMMAND address is updated to reflect
   the completed read if successful.

   The result is the CRC of the data read.

   Unlike with Wishbone, there is no concept of any errors possible when
   reading an SPR.

   @todo The algorithm for ensuring we only set the bits of interest in the
         register is inefficient. We should instead clear the whole area
         before starting.

   @note The address is treated as a word address of the SPR.

   @note The debug unit is documented as being explicitly Big Endian. However
         that seems to be a poor basis for modeling, and more to do with the
         debug unit only ever being used with big-endian architectures. We
         transfer the bytes in the endianness of the OR1K.

   @param[out] jreg       The JTAG register buffer where data is to be stored.
   @param[in]  skip_bits  Bits to skip before storing data in jreg.

   @return  The CRC of the data read                                         */
/*---------------------------------------------------------------------------*/
static uint32_t
spr_read (unsigned char *jreg,
	  unsigned int   skip_bits)
{
  /* Compute the CRC as we go */
  uint32_t  crc_out  = 0xffffffff;

  /* Validate the fields for the SPR read. This doesn't stop us - just prints
     out warnings and corrects the problems. */
  validate_spr_fields();
      
  /* Transfer the SPR */
  uint32_t  spr = mfspr (runtime.debug.addr);

  runtime.debug.addr++;

  /* Store the SPR in the register, without trampling adjacent
     bits. Simplified version when the bit offset is zero. Compute the CRC as
     we go. */
  unsigned  byte_off = skip_bits / 8;
  unsigned  bit_off  = skip_bits % 8;

  if (0 == bit_off)
    {
      /* Each byte in turn */
      int  i;

      for (i = 0; i < 4; i++)
	{
#ifdef OR32_BIG_ENDIAN
	  uint8_t byte = (spr >> 8 * i) & 0xff;
#else /* !OR32_BIG_ENDIAN */
	  uint8_t byte = (spr >> (24 - (8 * i))) & 0xff;
#endif /* OR32_BIG_ENDIAN */

	  jreg[byte_off + i] =  byte;
	  crc_out = crc32 (byte, 8, crc_out);
	}
    }
  else
    {
      /* Each byte in turn */
      int  i;

      for (i = 0; i < 4; i++)
	{
#ifdef OR32_BIG_ENDIAN
	  uint8_t byte = (spr >> 8 * i) & 0xff;
#else /* !OR32_BIG_ENDIAN */
	  uint8_t byte = (spr >> (24 - (8 * i))) & 0xff;
#endif /* OR32_BIG_ENDIAN */
	  
	  /* Clear the bits (only) we are setting */
	  jreg[byte_off + i]     <<= bit_off;
	  jreg[byte_off + i]     >>= bit_off;
	  jreg[byte_off + i + 1] >>= 8 - bit_off;
	  jreg[byte_off + i + 1] <<= 8 - bit_off;
      
	  /* OR in the bits */
	  jreg[byte_off + i]     |= (byte <<      bit_off)  & 0xff;
	  jreg[byte_off + i + 1] |= (byte >> (8 - bit_off)) & 0xff;

	  crc_out = crc32 (byte, 8, crc_out);
	}
    }

  return  crc_out;

}	/* spr_read () */


/*---------------------------------------------------------------------------*/
/*!Carry out a read from WishBone or SPR

   Process a GO_COMMAND register for read. The format is:

          +-------------+-------+---------+---+
          |             |       |    GO   |   |
   TDI -> |   Ignored   |  CRC  | COMMAND | 0 | -> TDO
          |             |       |  (0x0)  |   |
          +-------------+-------+---------+---+
           36 + 8 * size    32       4
               bits        bits    bits

   The returned register has the format:

          +-------+--------+------------+---------+
          |       |        |            |         |   
   TDI -> |  CRC  | Status |    Data    |  00..00 | -> TDO
          |       |        |            |         |
          +-------+--------+------------+---------+
              32       4      8 * size       37
             bits    bits       bits        bits

   Fields are always shifted in MS bit first, so must be reversed. The CRC in
   is computed on the first 5 bits, the CRC out on the 4 + 8 * size status and
   data bits.

   @note The size of the data is one greater than the length specified in the
         original WRITE_COMMAND.

   @param[in,out] jreg  The register to shift in, and the register shifted
                        back out.

   @return  The number of cycles the shift took, which in turn is the number
            of bits in the register                                          */
/*---------------------------------------------------------------------------*/
static int
go_command_read (unsigned char *jreg)
{
  /* Break out the fields */
  uint32_t  crc_in   = reverse_bits (((uint32_t) (jreg[0] & 0xe0) >>  5) |
                                     ((uint32_t)  jreg[1]         <<  3) |
		                     ((uint32_t)  jreg[2]         << 11) |
			             ((uint32_t)  jreg[3]         << 19) |
			             ((uint32_t) (jreg[4] & 0x1f) << 27), 32);

  /* Compute the expected CRC */
  uint32_t  crc_computed;

  crc_computed = crc32 (0,                1, 0xffffffff);
  crc_computed = crc32 (JCMD_GO_COMMAND,  4, crc_computed);

  /* Status flags */
  enum jtag_status  status = JS_OK;

  /* CRC to go out */
  uint32_t  crc_out = 0;

  /* Validate the CRC */
  if (crc_computed == crc_in)
    {
      /* Read the data. */
      switch (runtime.debug.mod_id)
	{
	case JM_WISHBONE:
	  crc_out = wishbone_read (jreg, 37, &status);
	  break;
	      
	case JM_CPU0:
	  crc_out = spr_read (jreg, 37);
	  break;
	      
	case JM_CPU1:
	  fprintf (stderr, "Warning: JTAG attempt to read from CPU1: Not "
		   "supported.\n");
	  break;
	  
	default:
	  fprintf (stderr, "*** ABORT ***: go_command_read: invalid "
		   "module\n");
	  abort ();
	}
    }
  else
    {
      /* Mismatch: record the error */
      status |= JS_CRC_IN_ERROR;
    }

  /* Construct the outgoing register, skipping the data read and returning the
     number of JTAG cycles taken (the register length). */
  return  construct_response (jreg, status, crc_out, 8 * runtime.debug.size,
			      37);

}	/* go_command_read () */


/*---------------------------------------------------------------------------*/
/*!Write WishBone data

   Write memory to WishBone. The WRITE_COMMAND address is updated to reflect
   the completed write if successful.

   @note The size of the data is one greater than the length specified in the
         original WRITE_COMMAND.

   @todo For now we always write a byte a time. In the future, we ought to use
         16 and 32-bit accesses for greater efficiency.

   @todo  The algorithm for ensuring we only set the bits of interest in the
          register is inefficient. We should instead clear the whole area
          before starting.

   @param[out] jreg        The JTAG register buffer where data is to be
                           stored.
   @param[in]  skip_bits   Bits to skip before reading data from jreg.
   @param[in]  status_ptr  Pointer to the status register.                   */
/*---------------------------------------------------------------------------*/
static void
wishbone_write (unsigned char   *jreg,
	       unsigned int      skip_bits,
	       enum jtag_status *status_ptr)
{
  /* Validate the fields for the wishbone write. If this fails we stop here,
     setting an error in the status_ptr. */
  if (!validate_wb_fields())
    {
      *status_ptr |= JS_WISHBONE_ERROR;
      return;
    }
      
  /* Transfer each byte in turn, computing the CRC as we go */
  unsigned  byte_off = skip_bits / 8;
  unsigned  bit_off  = skip_bits % 8;

  uint32_t  i;				/* Index into the data being write */

  for (i = 0; i < runtime.debug.size; i++)
    {
      /* Error if we can't access this byte */
      if (NULL == verify_memoryarea (runtime.debug.addr + i))
	{
	  *status_ptr |= JS_WISHBONE_ERROR;
	  return;
	}

      /* Extract the byte from the register. Simplified version when the bit
	 offset is zero. */
      unsigned char  byte_r;

      if (0 == bit_off)
	{
	  byte_r = jreg[byte_off + i];
	}
      else
	{
	  byte_r = jreg[byte_off + i]     >>      bit_off  |
	           jreg[byte_off + i + 1] >> (8 - bit_off);
	}

      /* Circumvent the read-only check usually done for mem accesses. */
      set_program8 (runtime.debug.addr + i, reverse_byte (byte_r));
    }

  runtime.debug.addr += runtime.debug.size;

}	/* wishbone_write () */


/*---------------------------------------------------------------------------*/
/*!Write SPR data

   Write memory to WishBone. The WRITE_COMMAND address is updated to reflect
   the completed write if successful.

   Unlike with Wishbone, there is no concept of any errors possible when
   writeing an SPR.

   @todo The algorithm for ensuring we only set the bits of interest in the
         register is inefficient. We should instead clear the whole area
         before starting.

   @note The address is treated as a word address of the SPR.

   @note The debug unit is documented as being explicitly Big Endian. However
         that seems to be a poor basis for modeling, and more to do with the
         debug unit only ever being used with big-endian architectures. We
         transfer the bytes in the endianness of the OR1K.

   @param[out] jreg       The JTAG register buffer where data is to be stored.
   @param[in]  skip_bits  Bits to skip before reading data from jreg.        */
/*---------------------------------------------------------------------------*/
static void
spr_write (unsigned char *jreg,
	  unsigned int   skip_bits)
{
  /* Validate the fields for the SPR write. This doesn't stop us - just prints
     out warnings and corrects the problems. */
  validate_spr_fields ();
      
  /* Construct the SPR value one byte at a time. */
  uint32_t  spr = 0;

  unsigned  byte_off = skip_bits / 8;
  unsigned  bit_off  = skip_bits % 8;

  /* Each byte in turn */
  int  i;

  for (i = 0; i < 4; i++)
    {
      uint8_t byte;

      /* Simplified version when the bit offset is zero */
      if (0 == bit_off)
	{
	  byte = reverse_byte (jreg[byte_off + i]);
	}
      else
	{
	  byte = reverse_byte ((jreg[byte_off + i]     >>      bit_off) |
			       (jreg[byte_off + i + 1] << (8 - bit_off)));
	}

#ifdef OR32_BIG_ENDIAN
      spr |= ((uint32_t) (byte)) << (8 * i);
#else /* !OR32_BIG_ENDIAN */
      spr |= ((uint32_t) (byte)) << (24 - (8 * i));
#endif /* OR32_BIG_ENDIAN */
    }

  /* Transfer the SPR */
  mtspr (runtime.debug.addr, spr);
  runtime.debug.addr++;

}	/* spr_write () */


/*---------------------------------------------------------------------------*/
/*!Carry out a write to WishBone or SPR

   Process a GO_COMMAND register for write. The format is:

          +-------------+-------+------------+---------+---+
          |             |       |            |    GO   |   |
   TDI -> |   Ignored   |  CRC  |    Data    | COMMAND | 0 | -> TDO
          |             |       |            |  (0x0)  |   |
          +-------------+-------+------------+---------+---+
                36          32     8 * size       4
               bits        bits      bits       bits

   The returned register has the format:

          +-------+--------+-------------+
          |       |        |             |   
   TDI -> |  CRC  | Status |  00......00 | -> TDO
          |       |        |             |
          +-------+--------+-------------+
              32       4    8 * size + 37
             bits    bits        bits

   Fields are always shifted in MS bit first, so must be reversed. The CRC in
   is computed on the first 5 + 8 * size bits, the CRC out on the 4 status
   bits.

   @note The size of the data is one greater than the length specified in the
         original WRITE_COMMAND.

   @todo The rules say we look for errors in the WRITE_COMMAND spec
         here. However it would be better to do that at WRITE_COMMAND time and
         save the result for here, to avoid using duff data.

   @param[in,out] jreg  The register to shift in, and the register shifted
                        back out.

   @return  The number of cycles the shift took, which in turn is the number
            of bits in the register                                          */
/*---------------------------------------------------------------------------*/
static int
go_command_write (unsigned char *jreg)
{
  /* Break out the fields */
  uint32_t  crc_in =
    reverse_bits (((uint32_t) (jreg[runtime.debug.size + 0] & 0xe0) >>  5) |
		  ((uint32_t)  jreg[runtime.debug.size + 1]         <<  3) |
		  ((uint32_t)  jreg[runtime.debug.size + 2]         << 11) |
		  ((uint32_t)  jreg[runtime.debug.size + 3]         << 19) |
		  ((uint32_t) (jreg[runtime.debug.size + 4] & 0x1f) << 27),
		  32);

  /* Compute the expected CRC */
  uint32_t  crc_computed;

  crc_computed = crc32 (0,                1, 0xffffffff);
  crc_computed = crc32 (JCMD_GO_COMMAND,  4, crc_computed);

  int  i;

  for (i = 0; i < runtime.debug.size; i++)
    {
      uint8_t  byte = ((jreg[i] & 0xe0) >> 5) | ((jreg[i + 1] & 0x1f) << 3);
      crc_computed = crc32 (byte, 8, crc_computed);
    }

  /* Status flags */
  enum jtag_status  status = JS_OK;

  /* Validate the CRC */
  if (crc_computed == crc_in)
    {
      /* Read the data. */
      switch (runtime.debug.mod_id)
	{
	case JM_WISHBONE:
	  wishbone_write (jreg, 5, &status);
	  break;
	      
	case JM_CPU0:
	  spr_write (jreg, 5);
	  break;
	      
	case JM_CPU1:
	  fprintf (stderr, "Warning: JTAG attempt to write to CPU1: Not "
		   "supported.\n");
	  break;
	  
	default:
	  fprintf (stderr, "*** ABORT ***: go_command_write: invalid "
		   "module\n");
	  abort ();
	}
    }
  else
    {
      /* Mismatch: record the error */
      status |= JS_CRC_IN_ERROR;
    }

  /* Construct the outgoing register, skipping the data read and returning the
     number of JTAG cycles taken (the register length). */
  return  construct_response (jreg, status, 0xffffffff, 0,
			      37 + 8 * runtime.debug.size);

}	/* go_command_write () */


/*---------------------------------------------------------------------------*/
/*!Invoke the action specified by a prior WRITE_COMMAND register

   Process a GO_COMMAND register.

   How this is handled depends on whether a previous WRITE_COMMAND has
   selected a read access type or a write access type has been selected.

   This function breaks this out.

   @param[in,out] jreg  The register to shift in, and the register shifted
                        back out.

   @return  The number of cycles the shift took, which in turn is the number
            of bits in the register                                          */
/*---------------------------------------------------------------------------*/
static int
go_command (unsigned char *jreg)
{
  /* Have we even had a WRITE_COMMAND? */
  if (!runtime.debug.write_defined_p)
    {
      fprintf (stderr, "Warning: JTAG GO_COMMAND with no prior WRITE_COMMAND: "
	       "ignored\n");
      return 4 + 1;			/* Only the first 5 bits meaningful */
    }

  /* Whether to read or write depends on the access type */
  switch (runtime.debug.acc_type)
    {
    case JAT_WRITE8:
    case JAT_WRITE16:
    case JAT_WRITE32:
      return  go_command_write (jreg);

    case JAT_READ8:
    case JAT_READ16:
    case JAT_READ32:
      return  go_command_read (jreg);

    default:
      fprintf (stderr, "Warning: JTAG GO_COMMAND: invalid access type: "
	       "ignored\n");
      return 4 + 1;			/* Only the first 5 bits meaningful */
    }
}	/* go_command () */
      
      
/*---------------------------------------------------------------------------*/
/*!Read a previouse WRITE_COMMAND register

   Process a READ_COMMAND register. The format is:

          +---------+-------+---------+---+
          |         |       |   READ  |   |
   TDI -> | Ignored |  CRC  | COMMAND | 0 | -> TDO
          |         |       |  (0x1)  |   |
          +---------+-------+---------+---+
              88        32       4
             bits      bits    bits

   The returned register has the format:

          +-------+--------+--------+---------+--------+---------+
          |       |        |        |         |        |         |   
   TDI -> |  CRC  | Status | Length | Address | Access |  00..00 | -> TDO
          |       |        |        |         |  Type  |         |
          +-------+--------+--------+---------+--------+---------+
              32       4       16        32        4        37
             bits    bits     bits      bits     bits      bits

   Fields are always shifted in MS bit first, so must be reversed. The CRC in
   is computed on the first 5 bits, the CRC out on the 56 status, length,
   address and access type bits.

   @param[in,out] jreg  The register to shift in, and the register shifted
                        back out.

   @return  The number of cycles the shift took, which in turn is the number
            of bits in the register                                          */
/*---------------------------------------------------------------------------*/
static int
read_command (unsigned char *jreg)
{
  /* Break out the fields */
  uint32_t  crc_in   = reverse_bits (((uint32_t) (jreg[0] & 0xe0) >>  5) |
                                     ((uint32_t)  jreg[1]         <<  3) |
		                     ((uint32_t)  jreg[2]         << 11) |
			             ((uint32_t)  jreg[3]         << 19) |
			             ((uint32_t) (jreg[4] & 0x1f) << 27), 32);

  /* Compute the expected CRC */
  uint32_t  crc_computed;

  crc_computed = crc32 (0,                   1, 0xffffffff);
  crc_computed = crc32 (JCMD_READ_COMMAND,  4, crc_computed);

  /* CRC to go out */
  uint32_t  crc_out = 0xffffffff;

  /* Status flags */
  enum jtag_status  status = JS_OK;

  /* Validate the CRC */
  if (crc_computed != crc_in)
    {
      /* Mismatch: record the error */
      status |= JS_CRC_IN_ERROR;
    }
  else if (runtime.debug.write_defined_p)
    {
      /* Compute the CRC */
      crc_out = crc32 (runtime.debug.acc_type,  4, crc_out);
      crc_out = crc32 (runtime.debug.addr,     32, crc_out);
      crc_out = crc32 (runtime.debug.size - 1, 16, crc_out);

      /* Construct the outgoing register. Fields can only be written if they
	 are available */
      uint8_t   acc_type_r = reverse_bits (runtime.debug.acc_type,  4);
      uint32_t  addr_r     = reverse_bits (runtime.debug.addr,     32);
      uint16_t  len_r      = reverse_bits (runtime.debug.size - 1, 16);

      jreg[ 4] |= (acc_type_r <<  5) & 0xf8;
      jreg[ 5] |= (acc_type_r >>  3) & 0x07;
      jreg[ 5] |= (addr_r     <<  1) & 0xfe;
      jreg[ 6] |= (addr_r     >>  7) & 0xff;
      jreg[ 7] |= (addr_r     >> 15) & 0xff;
      jreg[ 8] |= (addr_r     >> 23) & 0xff;
      jreg[ 9] |= (addr_r     >> 31) & 0x01;
      jreg[ 9] |= (len_r      <<  1) & 0xfe;
      jreg[10] |= (len_r      >>  7) & 0xff;
      jreg[11] |= (len_r      >> 15) & 0x01;
    }
  else
    {
      fprintf (stderr, "Warning: JTAG attempt to READ_COMMAND without prior "
	       "WRITE_COMMAND: no data returned\n");
    }

  /* Construct the final response with the status, skipping the fields we've
     just (possibly) written. */
  return  construct_response (jreg, status, crc_out, 52, 37);

}	/* read_command () */


/*---------------------------------------------------------------------------*/
/*!Specify details for a subsequence GO_COMMAND

   Process a WRITE_COMMAND register. The format is:

          +---------+-------+--------+---------+--------+---------+---+
          |         |       |        |         |        |  WRITE  |   |
   TDI -> | Ignored |  CRC  | Length | Address | Access | COMMAND | 0 | -> TDO
          |         |       |        |         |  Type  |  (0x2)  |   |
          +---------+-------+--------+---------+--------+---------+---+
              36        32      16        32        4         4
             bits      bits    bits      bits     bits      bits

   The returned register has the format:

          +-------+--------+---------+
          |       |        |         |   
   TDI -> |  CRC  | Status |  00..00 | -> TDO
          |       |        |         |
          +-------+--------+---------+
              32       4        89
             bits    bits      bits


   Fields are always shifted in MS bit first, so must be reversed. The CRC in
   is computed on the first 57 bits, the CRC out on the 4 status bits.

   @param[in,out] jreg  The register to shift in, and the register shifted
                        back out.

   @return  The number of cycles the shift took, which in turn is the number
            of bits in the register                                          */
/*---------------------------------------------------------------------------*/
static int
write_command (unsigned char *jreg)
{
  /* Break out the fields */
  uint8_t   acc_type = reverse_bits (((jreg[0] & 0x0e) >> 5) |
                                      (jreg[1] & 0x01), 4);
  uint32_t  addr     = reverse_bits (((uint32_t) (jreg[ 1] & 0xfe) >>  1) |
                                     ((uint32_t)  jreg[ 2]         <<  7) |
                                     ((uint32_t)  jreg[ 3]         << 15) |
                                     ((uint32_t)  jreg[ 4]         << 23) |
                                     ((uint32_t) (jreg[ 5] & 0x01) << 31), 32);
  uint16_t  len      = reverse_bits (((uint32_t) (jreg[ 5] & 0xfe) >>  1) |
                                     ((uint32_t)  jreg[ 6]         <<  7) |
                                     ((uint32_t) (jreg[ 7] & 0x01) << 15), 16);
  uint32_t  crc_in   = reverse_bits (((uint32_t) (jreg[ 7] & 0xfe) >>  1) |
                                     ((uint32_t)  jreg[ 8]         <<  7) |
		                     ((uint32_t)  jreg[ 9]         << 15) |
			             ((uint32_t)  jreg[10]         << 23) |
			             ((uint32_t) (jreg[11] & 0x01) << 31), 32);

  /* Compute the expected CRC */
  uint32_t  crc_computed;

  crc_computed = crc32 (0,                   1, 0xffffffff);
  crc_computed = crc32 (JCMD_WRITE_COMMAND,  4, crc_computed);
  crc_computed = crc32 (acc_type,            4, crc_computed);
  crc_computed = crc32 (addr,               32, crc_computed);
  crc_computed = crc32 (len,                16, crc_computed);

  /* Status flags */
  enum jtag_status  status = JS_OK;

  /* Validate the CRC */
  if (crc_computed != crc_in)
    {
      /* Mismatch: record the error */
      status |= JS_CRC_IN_ERROR;
    }
  else
    {
      /* All OK. All other errors can only occur when the GO_COMMAND tries to
	 execute the write. Record the information for next GO_COMMAND */
      runtime.debug.write_defined_p = 1;
      runtime.debug.acc_type        = acc_type;
      runtime.debug.addr            = addr;
      runtime.debug.size            = (unsigned long int) len + 1UL;
    }
      
  /* Construct the outgoing register and return the JTAG cycles taken (the
     register length) */
  return  construct_response (jreg, status, 0xffffffff, 0, 89);

}	/* write_command () */


/*---------------------------------------------------------------------------*/
/*!Read the control bits from a CPU.

   Process a READ_CONTROL register. The format is:

          +---------+-------+---------+---+
          |         |       |  READ   |   |
   TDI -> | Ignored |  CRC  | CONTROL | 0 | -> TDO
          |         |       |  (0x3)  |   |
          +---------+-------+---------+---+
              36        32       4
             bits      bits    bits

   The returned register has the format:

          +-------+--------+--------+---------+
          |       |        |        |         |   
   TDI -> |  CRC  | Status |  Data  |  00..00 | -> TDO
          |       |        |        |         |
          +-------+--------+--------+---------+
              32       4       52        37
             bits    bits     bits      bits

   Fields are always shifted in MS bit first, so must be reversed. The CRC in
   is computed on the first 57 bits, the CRC out on the 4 status bits.

   @param[in,out] jreg  The register to shift in, and the register shifted
                        back out.

   @return  The number of cycles the shift took, which in turn is the number
            of bits in the register                                          */
/*---------------------------------------------------------------------------*/
static int
read_control (unsigned char *jreg)
{
  /* Break out the fields. */
  uint32_t  crc_in = reverse_bits (((uint32_t) (jreg[0] & 0xe0) >>  5) |
                                   ((uint32_t)  jreg[1]         <<  3) |
		                   ((uint32_t)  jreg[2]         << 11) |
				   ((uint32_t)  jreg[3]         << 19) |
				   ((uint32_t) (jreg[4] & 0x1f) << 27), 32);

  /* Compute the expected CRC */
  uint32_t  crc_computed;

  crc_computed = crc32 (0,                   1, 0xffffffff);
  crc_computed = crc32 (JCMD_READ_CONTROL,  4, crc_computed);

  /* CRC to go out */
  uint32_t  crc_out = 0xffffffff;

  /* Status flags */
  enum jtag_status  status = JS_OK;

  /* Validate the CRC */
  if (crc_computed != crc_in)
    {
      /* Mismatch: record the error */
      status |= JS_CRC_IN_ERROR;
    }
  else if (JM_CPU0 == runtime.debug.mod_id)
    {
      /* Valid module. Only bit we can sensibly read is the stall bit. */
      uint64_t  data = (uint64_t) runtime.cpu.stalled << JCB_STALL;

      /* Compute the CRC */
      crc_out = crc32 (data,  52, crc_out);

      /* Construct the outgoing register. */
      uint64_t  data_r = reverse_bits (data, 52);

      jreg[ 4] |= (data_r <<  5) & 0xf8;
      jreg[ 5] |= (data_r >>  3) & 0x07;
      jreg[ 5] |= (data_r >> 11) & 0xff;
      jreg[ 5] |= (data_r >> 19) & 0xff;
      jreg[ 5] |= (data_r >> 27) & 0xff;
      jreg[ 5] |= (data_r >> 35) & 0xff;
      jreg[ 5] |= (data_r >> 43) & 0xff;
      jreg[ 5] |= (data_r >> 51) & 0x01;
    }
  else
    {
      /* Not a valid module */
      fprintf (stderr, "ERROR: JTAG attempt to read control data for module "
	       "other than CPU0: ignored\n");
    }

  /* Construct the response with the status */
  return  construct_response (jreg, status, crc_out, 52, 37);

}	/* read_control () */


/*---------------------------------------------------------------------------*/
/*!Write the control bits to a CPU.

   Process a WRITE_CONTROL register. The format is:

          +---------+-------+--------+---------+---+
          |         |       |        |  WRITE  |   |
   TDI -> | Ignored |  CRC  |  Data  | CONTROL | 0 | -> TDO
          |         |       |        |  (0x4)  |   |
          +---------+-------+--------+---------+---+
              36        32      52        4
             bits      bits    bits     bits

   The returned register has the format:

          +-------+--------+---------+
          |       |        |         |   
   TDI -> |  CRC  | Status |  00..00 | -> TDO
          |       |        |         |
          +-------+--------+---------+
              32       4        89
             bits    bits      bits

   Fields are always shifted in MS bit first, so must be reversed. The CRC in
   is computed on the first 57 bits, the CRC out on the 4 status bits.

   @param[in,out] jreg  The register to shift in, and the register shifted
                        back out.

   @return  The number of cycles the shift took, which in turn is the number
            of bits in the register                                          */
/*---------------------------------------------------------------------------*/
static int
write_control (unsigned char *jreg)
{
  /* Break out the fields. */
  uint64_t  data   = reverse_bits (((uint64_t) (jreg[ 0] & 0xe0) >>  5) |
				   ((uint64_t)  jreg[ 1]         <<  3) |
				   ((uint64_t)  jreg[ 2]         << 11) |
				   ((uint64_t)  jreg[ 3]         << 19) |
				   ((uint64_t)  jreg[ 4]         << 27) |
				   ((uint64_t)  jreg[ 5]         << 35) |
				   ((uint64_t)  jreg[ 6]         << 43) |
				   ((uint64_t) (jreg[ 7] & 0x01) << 51), 52);
  uint32_t  crc_in = reverse_bits (((uint32_t) (jreg[ 7] & 0xfe) >>  1) |
                                   ((uint32_t)  jreg[ 8]         <<  7) |
		                   ((uint32_t)  jreg[ 9]         << 15) |
				   ((uint32_t)  jreg[10]         << 23) |
				   ((uint32_t) (jreg[11] & 0x01) << 31), 32);

  /* Compute the expected CRC */
  uint32_t  crc_computed;

  crc_computed = crc32 (0,                   1, 0xffffffff);
  crc_computed = crc32 (JCMD_WRITE_CONTROL,  4, crc_computed);
  crc_computed = crc32 (data,               52, crc_computed);

  /* Status flags */
  enum jtag_status  status = JS_OK;

  /* Validate the CRC */
  if (crc_computed != crc_in)
    {
      /* Mismatch: record the error */
      status |= JS_CRC_IN_ERROR;
    }
  else if (JM_CPU0 == runtime.debug.mod_id)
    {
      /* Good data and valid module. Reset, stall or unstall the register as
	 required. If reset is requested, there is no point considering
	 stalling! */
      int  reset_bit = (0x1 == ((data >> JCB_RESET) & 0x1));
      int  stall_bit = (0x1 == ((data >> JCB_STALL) & 0x1));

      if (reset_bit)
	{
	  sim_reset ();
	}
      else
	{
	  set_stall_state (stall_bit);
	}
    }
  else
    {
      /* Not a valid module */
      fprintf (stderr, "ERROR: JTAG attempt to control module other than "
	       "CPU0: ignored\n");
    }

  /* Construct the response with the status */
  return  construct_response (jreg, status, 0xffffffff, 0, 89);

}	/* write_control () */


/*---------------------------------------------------------------------------*/
/*!Initialize the JTAG system

   For now just reset the JTAG interface                                     */
/*---------------------------------------------------------------------------*/
void
jtag_init ()
{
  (void) jtag_reset ();

}	/* jtag_init () */


/*---------------------------------------------------------------------------*/
/*!Reset the JTAG interface

   Mark the current JTAG instruction as undefined.

   @return  The number of cycles the reset took.                             */
/*---------------------------------------------------------------------------*/
int
jtag_reset ()
{
  runtime.debug.instr = JI_UNDEFINED;

  return  JTAG_RESET_CYCLES;

}	/* jtag_reset () */


/*---------------------------------------------------------------------------*/
/*!Shift a JTAG instruction register

   @note Like all the JTAG interface functions, this must not be called
         re-entrantly while a call to any other function (e.g. or1kim_run ())
         is in progress. It is the responsibility of the caller to ensure this
         constraint is met, for example by use of a SystemC mutex.

   The register is represented as a vector of bytes, with the byte at offset
   zero being shifted first, and the least significant bit in each byte being
   shifted first. Where the register will not fit in an exact number of bytes,
   the odd bits are in the highest numbered byte, shifted to the low end.

   The format is:

          +-------------+
          |             |   
   TDI -> | Instruction | -> TDO
          |             |
          +-------------+
                 4
               bits

   With this debug interface, registers are shifted MS bit first, so we must
   reverse the bits to get the actual value.

   We record the selected instruction. For completeness the register is parsed
   and a warning given if any register other than DEBUG is shifted.

   @param[in,out] jreg  The register to shift in, and the register shifted
                        back out.

   @return  The number of cycles the shift took, which in turn is the number
            of bits in the register                                          */
/*---------------------------------------------------------------------------*/
int
jtag_shift_ir (unsigned char *jreg)
{
  runtime.debug.instr = reverse_bits (jreg[0] & 0xf, 4);

  switch (runtime.debug.instr)
    {
    case JI_EXTEST:
      fprintf (stderr, "Warning: JTAG EXTEST shifted\n");
      break;

    case JI_SAMPLE_PRELOAD:
      fprintf (stderr, "Warning: JTAG SAMPLE/PRELOAD shifted\n");
      break;

    case JI_IDCODE:
      fprintf (stderr, "Warning: JTAG IDCODE shifted\n");
      break;

    case JI_DEBUG:
      /* Do nothing for this one */
      break;

    case JI_MBIST:
      fprintf (stderr, "Warning: JTAG MBIST shifted\n");
      break;

    case JI_BYPASS:
      fprintf (stderr, "Warning: JTAG BYPASS shifted\n");
      break;

    default:
      fprintf (stderr, "Warning: Unknown JTAG instruction %04x shifted\n",
	       runtime.debug.instr);
      break;
    }

  return  4;			/* Register length */
      
}	/* jtag_shift_ir () */


/*---------------------------------------------------------------------------*/
/*!Shift a JTAG data register

   @note Like all the JTAG interface functions, this must not be called
         re-entrantly while a call to any other function (e.g. or1kim_run ())
         is in progress. It is the responsibility of the caller to ensure this
         constraint is met, for example by use of a SystemC mutex.

   The register is represented as a vector of bytes, with the byte at offset
   zero being shifted first, and the least significant bit in each byte being
   shifted first. Where the register will not fit in an exact number of bytes,
   the odd bits are in the highest numbered byte, shifted to the low end.

   This is only meaningful if the DEBUG register instruction is already
   selected. If not, the data register is rejected.

   The register is parsed to determine which of the six possible register
   types it could be.
   - MODULE_SELECT
   - WRITE_COMMNAND
   - READ_COMMAND
   - GO_COMMAND
   - WRITE_CONTROL
   - READ_CONTROL

   @note In practice READ_COMMAND is not used. However the functionality is
         provided for future compatibility.

   The parsing is hierarchical. The first bit determines if we have
   MODULE_SELECT, if not, the next 4 bits determine the command.

   @param[in,out] jreg  The register to shift in, and the register shifted
                        back out.

   @return  The number of cycles the shift took, which in turn is the number
            of bits in the register                                          */
/*---------------------------------------------------------------------------*/
int
jtag_shift_dr (unsigned char *jreg)
{
  if (JI_DEBUG != runtime.debug.instr)
    {
      fprintf (stderr, "ERROR: Attempt to shift JTAG data register when "
	       "DEBUG not instruction: ignored\n");
      return  0;
    }

  int  module_select_p = (1 == (jreg[0] & 0x1));

  if (module_select_p)
    {
      return  module_select (jreg);
    }
  else
    {
      enum jtag_cmd  cmd = reverse_bits ((jreg[0] >> 1) & 0xf, 4);

      switch (cmd)
	{
	case JCMD_GO_COMMAND:    return  go_command (jreg);
	case JCMD_READ_COMMAND:  return  read_command (jreg);
	case JCMD_WRITE_COMMAND: return  write_command (jreg);
	case JCMD_READ_CONTROL:  return  read_control (jreg);
	case JCMD_WRITE_CONTROL: return  write_control (jreg);

	default:
	  /* Not a command we recognize. Decide this after we've read just
	     the module and command bits */
	  return  4 + 1;
	}
    }
}	/* jtag_shift_dr () */
