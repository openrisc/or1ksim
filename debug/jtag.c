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
	 ((val & 0x0000ffff0000ffffULL) << 16));
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
   @param[in]  zero_bits  The number of bits to zero                         */
/*---------------------------------------------------------------------------*/
static void
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
  unsigned int  skip_bytes = (ign_bits + zero_bits) / 8;
  unsigned int  bit_off    = (ign_bits + zero_bits) % 8;

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
	       "offset: %u.\n", bit_off);
      abort ();
    }
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

   This is a register of a fixed size, which we verify initially.

   @param[in,out] jreg      The register to shift in, and the register shifted
                            back out.
   @param[in]     num_bits  The number of bits supplied.

   @return  The number of cycles the shift took, which in turn is the number
            of bits in the register                                          */
/*---------------------------------------------------------------------------*/
static void
select_module (unsigned char *jreg,
	       int            num_bits)
{
  /* Validate register size, which is fixed */
  const int  REG_BITS = 36 + 32 + 4 + 1;

  if (REG_BITS != num_bits)
    {
      fprintf (stderr,
	       "ERROR: JTAG module select %d bits, when %d bits expected.\n",
	      num_bits, REG_BITS);
      return;
    }

  /* Break out the fields */
  uint8_t   mod_id = reverse_bits ((jreg[0] >> 1) & 0xf, 4);
  uint32_t  crc_in = reverse_bits (((uint32_t ) (jreg[0] & 0xe0) >>  5) |
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
  if (crc_computed == crc_in)
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
	  /* Bad module: record the error. Set the module to JM_UNDEFINED,
	     which will trigger more errors in the future, rather than
	     leaving the module unchanged, which might allow such errors to
	     slip by undetected. */
	  status |= JS_MODULE_MISSING;
	  runtime.debug.mod_id = JM_UNDEFINED;
	  break;
	}
    }
  else
    {
      /* CRC Mismatch: record the error */
      status |= JS_CRC_IN_ERROR;
    }

  /* Construct the outgoing register and return the JTAG cycles taken (the
     register length) */
  return  construct_response (jreg, status, 0xffffffff, 0, 37);

}	/* select_module */


/*---------------------------------------------------------------------------*/
/*!Read WishBone data

   Read memory from WishBone. The WRITE_COMMAND address is updated to reflect
   the completed read if successful.

   The data follows an initial 37 bits corresponding to the incoming command
   and CRC. We use the smaller of the data size in the original WRITE_COMMAND
   and the size of the data in the GO_COMMAND_WRITE packet, so we can handle
   over/under-run.

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
   @param[in]  jreg_bytes   The number of bytes expected in the JTAG register
                           (may be different to that in the prior READ/WRITE
                           if we have over- or under-run).
   @param[in]  status_ptr  Pointer to the status register.

   @return  The CRC of the data read                                         */
/*---------------------------------------------------------------------------*/
static uint32_t
wishbone_read (unsigned char     *jreg,
	       unsigned long int  jreg_bytes,
	       enum jtag_status  *status_ptr)
{
  const unsigned int  BIT_OFF  = 37 % 8;	/* Skip initial fields */
  const unsigned int  BYTE_OFF = 37 / 8;

  /* Transfer each byte in turn, computing the CRC as we go. We must supply
     jreg_bytes. If there is overrun, we use zero for the remaining bytes. */
  uint32_t           crc_out  = 0xffffffff;
  unsigned long int  i;

  for (i = 0; i < jreg_bytes; i++)
    {
      unsigned char  byte = 0;

      /* Get a byte if available */
      if (i < runtime.debug.size)
	{
	  /* Error if we can't access this byte */
	  if (NULL == verify_memoryarea (runtime.debug.addr + i))
	    {
	      *status_ptr |= JS_WISHBONE_ERROR;
	    }
	  else
	    {
	      /* Get the data with no cache or VM translation */
	      byte = eval_direct8 (runtime.debug.addr + i, 0, 0);
	    }
	}

      /* Update the CRC, reverse, then store the byte in the register, without
	 trampling adjacent bits. Simplified version when the bit offset is
	 zero. */
      crc_out = crc32 (byte, 8, crc_out);
      byte    = reverse_byte (byte);

      /* Clear the bits (only) we are setting */
      jreg[BYTE_OFF + i]     <<= 8 - BIT_OFF;
      jreg[BYTE_OFF + i]     >>= 8 - BIT_OFF;
      jreg[BYTE_OFF + i + 1] >>= BIT_OFF;
      jreg[BYTE_OFF + i + 1] <<= BIT_OFF;

      /* OR in the bits */
      jreg[BYTE_OFF + i]     |= (byte <<      BIT_OFF)  & 0xff;
      jreg[BYTE_OFF + i + 1] |= (byte >> (8 - BIT_OFF)) & 0xff;
    }

  /* Only advance if there we no errors (including over/under-run. */
  if (JS_OK == *status_ptr)
    {
      runtime.debug.addr += runtime.debug.size;
    }

  return  crc_out;

}	/* wishbone_read () */


/*---------------------------------------------------------------------------*/
/*!Read SPR data

   Read memory from a SPR. The WRITE_COMMAND address is updated to reflect the
   completed read if successful.

   The data follows an initial 37 bits corresponding to the incoming command
   and CRC. We use the smaller of the data size in the original WRITE_COMMAND
   and the size of the data in the GO_COMMAND_WRITE packet, so we can handle
   over/under-run.

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
   @param[in]  jreg_bytes   The number of bytes expected in the JTAG register
                           (may be different to that in the prior READ/WRITE
                           if we have over- or under-run).
   @param[in]  status_ptr  Pointer to the status register.

   @return  The CRC of the data read                                         */
/*---------------------------------------------------------------------------*/
static uint32_t
spr_read (unsigned char     *jreg,
	  unsigned long int  jreg_bytes,
	  enum jtag_status  *status_ptr)
{
  const unsigned int  BIT_OFF  = 37 % 8;	/* Skip initial fields */
  const unsigned int  BYTE_OFF = 37 / 8;

  /* Store the SPR in the register, without trampling adjacent bits, computing
     the CRC as we go. We have to fill all the JTAG register bytes. If there
     is overrun, we pack in zeros. */
  uint32_t           spr      = mfspr (runtime.debug.addr);
  uint32_t           crc_out  = 0xffffffff;
  unsigned long int  i;

  for (i = 0; i < jreg_bytes; i++)
    {
      unsigned char  byte = 0;

      /* Get the byte from the SPR if available, otherwise use zero */
      if (i < 4)
	{
#ifdef OR32_BIG_ENDIAN
	  byte = (spr >> 8 * i) & 0xff;
#else /* !OR32_BIG_ENDIAN */
	  byte = (spr >> (24 - (8 * i))) & 0xff;
#endif /* OR32_BIG_ENDIAN */
	}

      /* Update the CRC */
      crc_out = crc32 (byte, 8, crc_out);

      /* Reverse, then store the byte in the register, without trampling
	 adjacent bits. */
      byte = reverse_byte (byte);

      /* Clear the bits (only) we are setting */
      jreg[BYTE_OFF + i]     <<= 8 - BIT_OFF;
      jreg[BYTE_OFF + i]     >>= 8 - BIT_OFF;
      jreg[BYTE_OFF + i + 1] >>= BIT_OFF;
      jreg[BYTE_OFF + i + 1] <<= BIT_OFF;
      
      /* OR in the bits */
      jreg[BYTE_OFF + i]     |= (byte <<      BIT_OFF)  & 0xff;
      jreg[BYTE_OFF + i + 1] |= (byte >> (8 - BIT_OFF)) & 0xff;
    }

  /* Only advance if there we no errors (including over/under-run). */
  if (JS_OK == *status_ptr)
    {
      runtime.debug.addr++;
    }

  return  crc_out;

}	/* spr_read () */


/*---------------------------------------------------------------------------*/
/*!Set up null data.

   When there is an error in GO_COMMAND_READ, the data fields must be
   populated and the CRC set up, to ensure a correct return.

   The data follows an initial 37 bits corresponding to the incoming command
   and CRC. We use the smaller of the data size in the original WRITE_COMMAND
   and the size of the data in the GO_COMMAND_WRITE packet, so we can handle
   over/under-run.

   @param[out] jreg       The JTAG register buffer where data is to be stored.
   @param[in]  jreg_bytes   The number of bytes expected in the JTAG register
                           (may be different to that in the prior READ/WRITE
                           if we have over- or under-run).

   @return  The CRC of the data read                                         */
/*---------------------------------------------------------------------------*/
static uint32_t
null_read (unsigned char     *jreg,
	   unsigned long int  jreg_bytes)
{
  const unsigned int  BIT_OFF  = 37 % 8;	/* Skip initial fields */
  const unsigned int  BYTE_OFF = 37 / 8;

  /* Store each null byte in turn, computing the CRC as we go */
  uint32_t           crc_out  = 0xffffffff;
  unsigned long int  i;

  for (i = 0; i < jreg_bytes; i++)
    {
      crc_out = crc32 (0, 8, crc_out);	/* Compute the CRC for null byte */

      /* Store null byte in the register, without trampling adjacent
	 bits. */
      jreg[BYTE_OFF + i]     <<= 8 - BIT_OFF;
      jreg[BYTE_OFF + i]     >>= 8 - BIT_OFF;
      jreg[BYTE_OFF + i + 1] >>= BIT_OFF;
      jreg[BYTE_OFF + i + 1] <<= BIT_OFF;
    }

  return  crc_out;

}	/* null_read () */


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

   @param[in,out] jreg      The register to shift in, and the register shifted
                            back out.
   @param[in]     num_bits  The number of bits supplied.                     */
/*---------------------------------------------------------------------------*/
static void
go_command_read (unsigned char *jreg,
		 int            num_bits)
{
  /* Check the length is consistent with the prior WRITE_COMMAND. If not we
     have overrun or underrun and flag accordingly. */
  const int          REG_BITS = 36 + 8 * runtime.debug.size + 32 + 4 + 1;
  unsigned long int  jreg_bytes;
  enum jtag_status   status;

  if (REG_BITS == num_bits)
    {
      jreg_bytes = runtime.debug.size;
      status    = JS_OK;
    }
  else
    {
      /* Size will round down for safety */
      jreg_bytes = (runtime.debug.size * 8 + num_bits - REG_BITS) / 8;
      status    = JS_OVER_UNDERRUN;
    }

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

  /* CRC to go out */
  uint32_t  crc_out = 0;

  /* Validate the CRC */
  if (crc_computed == crc_in)
    {
      /* Read the data. Module errors will have been detected earlier, so we
	 do nothing more here. */
      switch (runtime.debug.mod_id)
	{
	case JM_WISHBONE:
	  crc_out = wishbone_read (jreg, jreg_bytes, &status);
	  break;
	      
	case JM_CPU0:
	  crc_out = spr_read (jreg, jreg_bytes, &status);
	  break;
	      
	default:
	  crc_out = null_read (jreg, jreg_bytes);
	  break;
	}
    }
  else
    {
      /* Mismatch: record the error */
      status |= JS_CRC_IN_ERROR;
    }

  /* Construct the outgoing register, skipping the data read and returning the
     number of JTAG cycles taken (the register length). */
  construct_response (jreg, status, crc_out, 8UL * jreg_bytes, 37);

}	/* go_command_read () */


/*---------------------------------------------------------------------------*/
/*!Write WishBone data

   Write memory to WishBone. The WRITE_COMMAND address is updated to reflect
   the completed write if successful.

   The data follows an initial 5 bits specifying the command. We use the
   smaller of the data size in the original WRITE_COMMAND and the size of the
   data in the GO_COMMAND_WRITE packet, so we can handle over/under-run.

   @note The size of the data is one greater than the length specified in the
         original WRITE_COMMAND.

   @todo For now we always write a byte a time. In the future, we ought to use
         16 and 32-bit accesses for greater efficiency and to correctly model
         the use of different access types.

   @param[in]  jreg        The JTAG register buffer where data is found.
   @param[in]  jreg_bytes   The number of bytes expected in the JTAG register
                           (may be different to that in the prior READ/WRITE
                           if we have over- or under-run).
   @param[out] status_ptr  Pointer to the status register.                   */
/*---------------------------------------------------------------------------*/
static void
wishbone_write (unsigned char     *jreg,
		unsigned long int  jreg_bytes,
		enum jtag_status  *status_ptr)
{
  const unsigned int  BIT_OFF  = 5;	/* Skip initial command */

  /* Transfer each byte in turn, computing the CRC as we go. We must supply
     jreg_bytes. If there is overrun, we ignore the remaining bytes. */
  unsigned long int  i;

  for (i = 0; i < jreg_bytes; i++)
    {
      /* Set a byte if available */
      if (i < runtime.debug.size)
	{
	  /* Error if we can't access this byte */
	  if (NULL == verify_memoryarea (runtime.debug.addr + i))
	    {
	      *status_ptr |= JS_WISHBONE_ERROR;
	    }
	  else
	    {
	      /* Extract the byte from the register, reverse it and write it,
		 circumventing the usual checks by pretending this is program
		 memory. */
	      unsigned char  byte;

	      byte = (jreg[i] >> BIT_OFF) | (jreg[i + 1] << (8 - BIT_OFF));
	      byte = reverse_byte (byte);
	      
	      set_program8 (runtime.debug.addr + i, byte);
	    }
	}
    }

  if (JS_OK == *status_ptr)
    {
      runtime.debug.addr += runtime.debug.size;
    }
}	/* wishbone_write () */


/*---------------------------------------------------------------------------*/
/*!Write SPR data

   Write memory to WishBone. The WRITE_COMMAND address is updated to reflect
   the completed write if successful.

   The data follows an initial 5 bits specifying the command. We use the
   smaller of the data size in the original WRITE_COMMAND and the size of the
   data in the GO_COMMAND_WRITE packet, so we can handle over/under-run.

   Unlike with Wishbone, there is no concept of any errors possible when
   writing an SPR.

   @todo The algorithm for ensuring we only set the bits of interest in the
         register is inefficient. We should instead clear the whole area
         before starting.

   @note The address is treated as a word address of the SPR.

   @note The debug unit is documented as being explicitly Big Endian. However
         that seems to be a poor basis for modeling, and more to do with the
         debug unit only ever being used with big-endian architectures. We
         transfer the bytes in the endianness of the OR1K.

   @param[out] jreg       The JTAG register buffer where data is to be stored.
   @param[in]  jreg_bytes   The number of bytes expected in the JTAG register
                           (may be different to that in the prior READ/WRITE
                           if we have over- or under-run).
   @param[out] status_ptr  Pointer to the status register.                   */
/*---------------------------------------------------------------------------*/
static void
spr_write (unsigned char     *jreg,
	   unsigned long int  jreg_bytes,
	   enum jtag_status  *status_ptr)
{
  const unsigned int  BIT_OFF  = 5;	/* Skip initial command */

  /* Construct the SPR value one byte at a time. If there is overrun, ignore
     the excess bytes. */
  uint32_t           spr = 0;
  unsigned long int  i;

  for (i = 0; i < jreg_bytes; i++)
    {
      if (i < 4)
	{
	  uint8_t byte;

	  byte = (jreg[i] >> BIT_OFF) | (jreg[i + 1] << (8 - BIT_OFF));
	  byte = reverse_byte (byte);

#ifdef OR32_BIG_ENDIAN
	  spr |= ((uint32_t) (byte)) << (8 * i);
#else /* !OR32_BIG_ENDIAN */
	  spr |= ((uint32_t) (byte)) << (24 - (8 * i));
#endif /* OR32_BIG_ENDIAN */
	}
    }

  /* Transfer the SPR */
  mtspr (runtime.debug.addr, spr);

  /* Only advance if there we no errors (including over/under-run). */
  if (JS_OK == *status_ptr)
    {
      runtime.debug.addr++;
    }
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

   @param[in,out] jreg      The register to shift in, and the register shifted
                            back out.
   @param[in]     num_bits  The number of bits supplied.                     */
/*---------------------------------------------------------------------------*/
static void
go_command_write (unsigned char *jreg,
		  int            num_bits)
{
  /* Check the length is consistent with the prior WRITE_COMMAND. If not we
     have overrun or underrun and flag accordingly. */
  const int          REG_BITS = 36 + 32 + 8 * runtime.debug.size + 4 + 1;
  unsigned long int  real_size;
  enum jtag_status   status;

  if (REG_BITS == num_bits)
    {
      real_size = runtime.debug.size;
      status    = JS_OK;
    }
  else
    {
      /* Size will round down for safety */
      real_size = (runtime.debug.size * 8UL + num_bits - REG_BITS) / 8UL;
      status    = JS_OVER_UNDERRUN;
    }

  /* Break out the fields */
  uint32_t  crc_in =
    reverse_bits (((uint32_t) (jreg[real_size + 0] & 0xe0) >>  5) |
		  ((uint32_t)  jreg[real_size + 1]         <<  3) |
		  ((uint32_t)  jreg[real_size + 2]         << 11) |
		  ((uint32_t)  jreg[real_size + 3]         << 19) |
		  ((uint32_t) (jreg[real_size + 4] & 0x1f) << 27),
		  32);

  /* Compute the expected CRC */
  uint32_t  crc_computed;

  crc_computed = crc32 (0,                1, 0xffffffff);
  crc_computed = crc32 (JCMD_GO_COMMAND,  4, crc_computed);

  unsigned long int  i;

  for (i = 0; i < real_size; i++)
    {
      uint8_t  byte = reverse_bits (((jreg[i] & 0xe0) >> 5) |
		      ((jreg[i + 1] & 0x1f) << 3), 8);
      crc_computed = crc32 (byte, 8, crc_computed);
    }

  /* Validate the CRC */
  if (crc_computed == crc_in)
    {
      /* Write the data. Module errors will have been detected earlier, so we
	 do nothing more here. */
      switch (runtime.debug.mod_id)
	{
	case JM_WISHBONE:
	  wishbone_write (jreg, real_size, &status);
	  break;
	      
	case JM_CPU0:
	  spr_write (jreg, real_size, &status);
	  break;

	default:
	  break;
	}
    }
  else
    {
      /* Mismatch: record the error */
      status |= JS_CRC_IN_ERROR;
    }

  /* Construct the outgoing register, skipping the data read and returning the
     number of JTAG cycles taken (the register length). */
  construct_response (jreg, status, 0xffffffff, 0, 37UL + 8UL * real_size);

}	/* go_command_write () */


/*---------------------------------------------------------------------------*/
/*!Invoke the action specified by a prior WRITE_COMMAND register

   Process a GO_COMMAND register.

   How this is handled depends on whether a previous WRITE_COMMAND has
   selected a read access type or a write access type has been selected.

   This function breaks this out.

   @param[in,out] jreg      The register to shift in, and the register shifted
                            back out.
   @param[in]     num_bits  The number of bits supplied.                     */
/*---------------------------------------------------------------------------*/
static void
go_command (unsigned char *jreg,
	    int            num_bits)
{
  /* Have we even had a WRITE_COMMAND? */
  if (!runtime.debug.write_defined_p)
    {
      memset (jreg, 0, (num_bits + 7) / 8);
      fprintf (stderr, "ERROR: JTAG GO_COMMAND with no prior WRITE_COMMAND.\n");
      return;
    }

  /* Whether to read or write depends on the access type. We don't put an
     error message if it's invalid here - we rely on the prior WRITE_COMMAND
     to do that. We just silently ignore. */
  switch (runtime.debug.acc_type)
    {
    case JAT_WRITE8:
    case JAT_WRITE16:
    case JAT_WRITE32:
      go_command_write (jreg, num_bits);
      break;

    case JAT_READ8:
    case JAT_READ16:
    case JAT_READ32:
      go_command_read (jreg, num_bits);
      break;

    default:
      break;
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

   This is a register of a fixed size, which we verify initially.

   @param[in,out] jreg      The register to shift in, and the register shifted
                            back out.
   @param[in]     num_bits  The number of bits supplied.                     */
/*---------------------------------------------------------------------------*/
static void
read_command (unsigned char *jreg,
	      int            num_bits)
{
  /* Validate register size, which is fixed */
  const int  REG_BITS = 88 + 32 + 4 + 1;

  if (REG_BITS != num_bits)
    {
      fprintf (stderr,
	       "ERROR: JTAG READ_COMMAND %d bits, when %d bits expected.\n",
	       num_bits, REG_BITS);
      return;
    }

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

  /* Only do anything with this if the CRC's match */
  if (crc_computed == crc_in)
    {
      /* If we haven't had a previous WRITE_COMMAND, then we return empty
	 data. There is no valid status flag we can use, but we print a rude
	 message. */
      uint8_t   acc_type;
      uint32_t  addr;
      uint16_t  len;

      if (runtime.debug.write_defined_p)
	{
	  acc_type = runtime.debug.acc_type;
	  addr     = runtime.debug.addr;
	  len      = runtime.debug.size - 1;
	}
      else
	{
	  fprintf (stderr, "ERROR: JTAG READ_COMMAND finds no data.\n");
	  
	  acc_type = 0;
	  addr     = 0;
	  len      = 0;
	}
      
      /* Compute the CRC */
      crc_out = crc32 (acc_type,  4, crc_out);
      crc_out = crc32 (addr,     32, crc_out);
      crc_out = crc32 (len,      16, crc_out);
      
      /* Reverse the bit fields */
      acc_type = reverse_bits (acc_type,  4);
      addr     = reverse_bits (addr,     32);
      len      = reverse_bits (len,      16);
      
      /* Construct the outgoing register. */
      jreg[ 4] |= (acc_type <<  5) & 0xe0;
      jreg[ 5] |= (acc_type >>  3) & 0x01;
      jreg[ 5] |= (addr     <<  1) & 0xfe;
      jreg[ 6] |= (addr     >>  7) & 0xff;
      jreg[ 7] |= (addr     >> 15) & 0xff;
      jreg[ 8] |= (addr     >> 23) & 0xff;
      jreg[ 9] |= (addr     >> 31) & 0x01;
      jreg[ 9] |= (len      <<  1) & 0xfe;
      jreg[10] |= (len      >>  7) & 0xff;
      jreg[11] |= (len      >> 15) & 0x01;
    }
  else
    {
      /* CRC Mismatch: record the error */
      status |= JS_CRC_IN_ERROR;
    }

  /* Construct the final response with the status, skipping the fields we've
     just written. */
  return  construct_response (jreg, status, crc_out, 52, 37);

}	/* read_command () */


/*---------------------------------------------------------------------------*/
/*!Validate WRITE_COMMAND fields for WishBone

   Check that a WRITE_COMMAND's fields are valid for WishBone access.

   - 16 and 32-bit access must be correctly aligned.

   - size must be a multiple of 2 for 16-bit access, and a multiple of 4 for
     32-bit access.

   Error messages are printed to explain any validation problems.

   @todo Should multiple SPR accesses be allowed in a single access?

   @note The size of the data is one greater than the length specified in the
         original WRITE_COMMAND.

   @param[in] acc_type  The access type field
   @param[in] addr      The address field
   @param[in] size      The number of bytes to transfer (field is 1 less than
                        this).

   @return  1 (TRUE) if validation is OK, 0 (FALSE) if validation fails.     */
/*---------------------------------------------------------------------------*/
static int
validate_wb_fields (unsigned char      acc_type,
		    unsigned long int  addr,
		    unsigned long int  size)
{
  int  res_p = 1;			/* Result */

  /* Determine the size of the access */
  uint32_t  access_bytes = 1;

  switch (acc_type)
    {
    case JAT_WRITE8:
    case JAT_READ8:
      access_bytes = 1;
      break;

    case JAT_WRITE16:
    case JAT_READ16:
      access_bytes = 2;
      break;

    case JAT_WRITE32:
    case JAT_READ32:
      access_bytes = 4;
      break;

    default:
      fprintf (stderr, "ERROR: JTAG WRITE_COMMAND unknown access type %u.\n",
	       acc_type);
      res_p = 0;
      break;
    }

  /* Check for alignment. This works for 8-bit and undefined access type,
     although the tests will always pass. */

  if (0 != (addr % access_bytes))
    {
      fprintf (stderr, "ERROR: JTAG WishBone %d-bit access must be %d-byte "
	       "aligned.\n", access_bytes * 8, access_bytes);
      res_p = 0;
    }

  /* Check byte length is multiple of access width */
  if (0 != (size % access_bytes))
    {
      fprintf (stderr, "ERROR: JTAG %d-bit WishBone access must be multiple "
	       "of %d bytes in length.\n", access_bytes * 8, access_bytes);
      res_p = 0;
    }

  return  res_p;

}	/* validate_wb_fields () */


/*---------------------------------------------------------------------------*/
/*!Validate WRITE_COMMAND fields for SPR

   Check that a WRITE_COMMAND's fields are valid for SPR access. Only prints
   messages, since the protocol does not allow for any errors.

   - 8 and 16-bit access is not permitted.

   - size must be 4 bytes (1 word). Any other value is an error.

   - address must be less than MAX_SPRS. If a larger value is specified, it
     will be reduced module MAX_SPRS (which is hopefully a power of 2). This
     is only a warning, not an error.

   Error/warning messages are printed to explain any validation problems.

   @todo Should multiple SPR accesses be allowed in a single access?

   @note The size of the data is one greater than the length specified in the
         original WRITE_COMMAND.                                             

   @param[in] acc_type  The access type field
   @param[in] addr      The address field
   @param[in] size      The number of bytes to transfer (field is 1 less than
                        this).

   @return  1 (TRUE) if we could validate (even if addr needs truncating),
            0 (FALSE) otherwise.                                             */
/*---------------------------------------------------------------------------*/
static int
validate_spr_fields (unsigned char      acc_type,
		     unsigned long int  addr,
		     unsigned long int  size)
{
  int  res_p = 1;			/* Result */

  /* Determine the size and direction of the access */
  switch (acc_type)
    {
    case JAT_WRITE8:
    case JAT_READ8:
      fprintf (stderr, "ERROR: JTAG 8-bit access for SPR not supported.\n");
      res_p = 0;
      break;

    case JAT_WRITE16:
    case JAT_READ16:
      fprintf (stderr, "ERROR: JTAG 16-bit access for SPR not supported.\n");
      res_p = 0;
      break;

    case JAT_WRITE32:
    case JAT_READ32:
      break;

    default:
      fprintf (stderr, "ERROR: unknown JTAG SPR access type %u.\n",
	       acc_type);
      res_p = 0;
      break;
    }

  /* Validate access size */
  if (4 != size)
    {
      fprintf (stderr, "ERROR: JTAG SPR access 0x%lx bytes not supported.\n",
	       size);
      res_p = 0;
    }

  /* Validate address. This will be truncated if wrong, so not an error. */
  if (addr >= MAX_SPRS)
    {
      fprintf (stderr, "Warning: truncated JTAG SPR address 0x%08lx.\n",
	       addr);
    }

  return  res_p;			/* Success */

}	/* validate_spr_fields () */


/*---------------------------------------------------------------------------*/
/*!Specify details for a subsequent GO_COMMAND

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

   There are no bits for reporting bit specification errors other than CRC
   mismatch. The subsequent GO_COMMAND will report an error in reading from
   Wishbone or over/under-run. We report any inconsistencies here with warning
   messages, correcting them if possible.

   All errors invalidate any prior data. This will ensure any subsequent usage
   continues to trigger faults, rather than a failed WRITE_COMMAND being
   missed.

   This is a register of a fixed size, which we verify initially.

   @param[in,out] jreg      The register to shift in, and the register shifted
                            back out.
   @param[in]     num_bits  The number of bits supplied.                     */
/*---------------------------------------------------------------------------*/
static void
write_command (unsigned char *jreg,
	       int            num_bits)
{
  /* Validate register size, which is fixed */
  const int  REG_BITS = 36 + 32 +16 + 32 + 4 + 4 + 1;

  if (REG_BITS != num_bits)
    {
      runtime.debug.write_defined_p = 0;
      fprintf (stderr,
	       "ERROR: JTAG WRITE_COMMAND %d bits, when %d bits expected.\n",
	       num_bits, REG_BITS);
      return;
    }

  /* Break out the fields */
  uint8_t   acc_type = reverse_bits (((jreg[0] & 0xe0) >> 5) |
				     ((jreg[1] & 0x01) << 3) , 4);
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

  /* We only do anything with this packet if the CRC's match */
  if (crc_computed == crc_in)
    {
      unsigned long int  data_size = (unsigned long int) len + 1UL;

      switch (runtime.debug.mod_id)
	{
	case JM_WISHBONE:

	  if (validate_wb_fields (acc_type, addr, data_size))
	    {
	      runtime.debug.write_defined_p = 1;
	      runtime.debug.acc_type        = acc_type;
	      runtime.debug.addr            = addr;
	      runtime.debug.size            = data_size;
	    }
	  else
	    {
	      runtime.debug.write_defined_p = 0;
	    }
	  
	  break;
	  
	case JM_CPU0:
	  
	  /* Oversize addresses are permitted, but cause a validation warning
	     and are truncated here. */
	  if (validate_spr_fields (acc_type, addr, data_size))
	    {
	      runtime.debug.write_defined_p = 1;
	      runtime.debug.acc_type        = acc_type;
	      runtime.debug.addr            = addr % MAX_SPRS;
	      runtime.debug.size            = data_size;
	    }
	  else
	    {
	      runtime.debug.write_defined_p = 0;
	    }
	  
	  break;
	  
	case JM_CPU1:
	  
	  runtime.debug.write_defined_p = 0;
	  fprintf (stderr,
		   "ERROR: JTAG WRITE_COMMAND for CPU1 not supported.\n");
	  break;
	  
	case JM_UNDEFINED:
	  
	  runtime.debug.write_defined_p = 0;
	  fprintf (stderr,
		   "ERROR: JTAG WRITE_COMMAND with no module selected.\n");
	  break;
	  
	default:

	  /* All other modules will have triggered an error on selection. */
	  runtime.debug.write_defined_p = 0;
	  break;
	}
    }
  else
    {
      /* CRC Mismatch: record the error */
      runtime.debug.write_defined_p  = 0;
      status                        |= JS_CRC_IN_ERROR;
    }

  
  /* Construct the outgoing register */
  construct_response (jreg, status, 0xffffffff, 0, 89);

}	/* write_command () */


/*---------------------------------------------------------------------------*/
/*!Read the control bits from a CPU.

   Process a READ_CONTROL register. The format is:

          +---------+-------+---------+---+
          |         |       |  READ   |   |
   TDI -> | Ignored |  CRC  | CONTROL | 0 | -> TDO
          |         |       |  (0x3)  |   |
          +---------+-------+---------+---+
              88        32       4
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

   This is a register of a fixed size, which we verify initially.

   @param[in,out] jreg      The register to shift in, and the register shifted
                            back out.
   @param[in]     num_bits  The number of bits supplied.                     */
/*---------------------------------------------------------------------------*/
static void
read_control (unsigned char *jreg,
	      int            num_bits)
{
  /* Validate register size, which is fixed */
  const int  REG_BITS = 88 + 32 + 4 + 1;

  if (REG_BITS != num_bits)
    {
      fprintf (stderr,
	       "ERROR: JTAG READ_CONTROL %d bits, when %d bits expected.\n",
	       num_bits, REG_BITS);
      return;
    }

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

  /* Only do anything if the CRC's match */
  if (crc_computed == crc_in)
    {
      uint64_t  data = 0;

      switch (runtime.debug.mod_id)
	{
	case JM_CPU0:
	  
	  /* Valid module. Only bit we can sensibly read is the stall
	     bit. Compute the CRC, reverse the data and construct the
	     outgoing register. */
	  data    = (uint64_t) runtime.cpu.stalled << JCB_STALL;
	  break;
	  
	case JM_UNDEFINED:
	  fprintf (stderr,
		   "ERROR: JTAG READ_CONTROL with no module selected.\n");
	  break;
      
	case JM_WISHBONE:
	  fprintf (stderr,
		   "ERROR: JTAG READ_CONTROL of WishBone not supported.\n");
	  break;
      
	case JM_CPU1:
	  fprintf (stderr,
		   "ERROR: JTAG READ_CONTROL of CPU1 not supported.\n");
	  break;
      
	default:
	  /* All other modules will have triggered an error on selection. */
	  break;
	}
  
      /* Compute the CRC, reverse and store the data, and construct the
	 response with the status. */
      crc_out = crc32 (data,  52, crc_out);
      data    = reverse_bits (data, 52);
  
      jreg[ 4] |= (data <<  5) & 0xf8;
      jreg[ 5] |= (data >>  3) & 0x07;
      jreg[ 6] |= (data >> 11) & 0xff;
      jreg[ 7] |= (data >> 19) & 0xff;
      jreg[ 8] |= (data >> 27) & 0xff;
      jreg[ 9] |= (data >> 35) & 0xff;
      jreg[10] |= (data >> 43) & 0xff;
      jreg[11] |= (data >> 51) & 0x01;
    }
  else
    {
      /* CRC Mismatch: record the error */
      status |= JS_CRC_IN_ERROR;
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

   This is a register of a fixed size, which we verify initially.

   @param[in,out] jreg  The register to shift in, and the register shifted
                        back out.
   @param[in]     num_bits  The number of bits supplied.                     */
/*---------------------------------------------------------------------------*/
static void
write_control (unsigned char *jreg,
	       int            num_bits)
{
  /* Validate register size, which is fixed */
  const int  REG_BITS = 36 + 32 + 52 + 4 + 1;

  if (REG_BITS != num_bits)
    {
      fprintf (stderr,
	       "ERROR: JTAG WRITE_CONTROL %d bits, when %d bits expected.\n",
	       num_bits, REG_BITS);
      return;
    }

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

  /* Only use the data if CRC's match */
  if (crc_computed == crc_in)
    {
      int  reset_bit;
      int  stall_bit;

      switch (runtime.debug.mod_id)
	{
	case JM_CPU0:

	  /* Good data and valid module. Reset, stall or unstall the register
	     as required. If reset is requested, there is no point considering
	     stalling! */
	  reset_bit = (0x1 == ((data >> JCB_RESET) & 0x1));
	  stall_bit = (0x1 == ((data >> JCB_STALL) & 0x1));

	  if (reset_bit)
	    {
	      sim_reset ();
	    }
	  else
	    {
	      set_stall_state (stall_bit);
	    }

	  break;

	case JM_UNDEFINED:
	  fprintf (stderr,
		   "ERROR: JTAG WRITE_CONTROL with no module selected.\n");
	  break;
      
	case JM_WISHBONE:
	  fprintf (stderr,
		   "ERROR: JTAG WRITE_CONTROL of WishBone not supported.\n");
	  break;
      
	case JM_CPU1:
	  fprintf (stderr,
		   "ERROR: JTAG WRITE_CONTROL of CPU1 not supported.\n");
	  break;
      
	default:
	  /* All other modules will have triggered an error on selection. */
	  break;
	}
    }
  else
    {
      /* CRC Mismatch: record the error */
      status |= JS_CRC_IN_ERROR;
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

   Mark the current JTAG instruction as undefined.                           */
/*---------------------------------------------------------------------------*/
void
jtag_reset ()
{
  runtime.debug.instr = JI_UNDEFINED;

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

   We first verify that we have received the correct number of bits. If not we
   put out a warning, and for consistency return the number of bits supplied
   as the number of cycles the shift took.

   With this debug interface, registers are shifted MS bit first, so we must
   reverse the bits to get the actual value.

   We record the selected instruction. For completeness the register is parsed
   and a warning given if any register other than DEBUG is shifted.

   @param[in,out] jreg      The register to shift in, and the register shifted
                            back out.
   @param[in]     num_bits  The number of bits supplied.                     */
/*---------------------------------------------------------------------------*/
void
jtag_shift_ir (unsigned char *jreg,
	       int            num_bits)
{
  if (4 != num_bits)
    {
      fprintf (stderr, "ERROR: Invalid JTAG instruction length %d bits.\n",
	       num_bits);
      return;
    }

  runtime.debug.instr = reverse_bits (jreg[0] & 0xf, 4);

  switch (runtime.debug.instr)
    {
    case JI_EXTEST:
      fprintf (stderr, "Warning: JTAG EXTEST shifted.\n");
      break;

    case JI_SAMPLE_PRELOAD:
      fprintf (stderr, "Warning: JTAG SAMPLE/PRELOAD shifted.\n");
      break;

    case JI_IDCODE:
      fprintf (stderr, "Warning: JTAG IDCODE shifted.\n");
      break;

    case JI_DEBUG:
      /* Do nothing for this one */
      break;

    case JI_MBIST:
      fprintf (stderr, "Warning: JTAG MBIST shifted.\n");
      break;

    case JI_BYPASS:
      fprintf (stderr, "Warning: JTAG BYPASS shifted.\n");
      break;

    default:
      fprintf (stderr, "ERROR: Unknown JTAG instruction 0x%1x shifted.\n",
	       runtime.debug.instr);
      break;
    }
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
   - SELECT_MODULE
   - WRITE_COMMNAND
   - READ_COMMAND
   - GO_COMMAND
   - WRITE_CONTROL
   - READ_CONTROL

   @note In practice READ_COMMAND is not used. However the functionality is
         provided for future compatibility.

   The parsing is hierarchical. The first bit determines if we have
   SELECT_MODULE, if not, the next 4 bits determine the command.

   @param[in,out] jreg      The register to shift in, and the register shifted
                            back out.
   @param[in]     num_bits  The number of bits supplied.                     */
/*---------------------------------------------------------------------------*/
void
jtag_shift_dr (unsigned char *jreg,
	       int            num_bits)
{
  if (JI_DEBUG != runtime.debug.instr)
    {
      fprintf (stderr, "ERROR: Attempt to shift JTAG data register when "
	       "DEBUG not instruction.\n");
      return;
    }

  int  select_module_p = (1 == (jreg[0] & 0x1));

  if (select_module_p)
    {
      select_module (jreg, num_bits);
    }
  else
    {
      switch (reverse_bits ((jreg[0] >> 1) & 0xf, 4))
	{
	case JCMD_GO_COMMAND:    go_command (jreg, num_bits); break;
	case JCMD_READ_COMMAND:  read_command (jreg, num_bits); break;
	case JCMD_WRITE_COMMAND: write_command (jreg, num_bits); break;
	case JCMD_READ_CONTROL:  read_control (jreg, num_bits); break;
	case JCMD_WRITE_CONTROL: write_control (jreg, num_bits); break;

	default:
	  /* Not a command we recognize. */
	  fprintf (stderr, "ERROR: DEBUG command not recognized.\n");
	  break;
	}
    }
}	/* jtag_shift_dr () */
