/* generic.c -- Generic external peripheral

   Copyright (C) 2008 Embecosm Limited
  
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
   with this program.  If not, see <http://www.gnu.org/licenses/>. */

/* This program is commented throughout in a fashion suitable for processing
   with Doxygen. */


/* This is functional simulation of any external peripheral. It's job is to
 * trap accesses in a specific range, so that the simulator can drive an
 * external device.
 *
 * A note on endianess. All external communication is done using HOST
 * endianess. A set of functions are provided to convert between host and
 * model endianess (htoml, htoms, mtohl, mtohs).
 */

/* Autoconf and/or portability configuration */
#include "config.h"

/* System includes */
#include <stdlib.h>
#include <stdio.h>

/* Package includes */
#include "arch.h"
#include "sim-config.h"
#include "abstract.h"
#include "toplevel-support.h"
#include "sim-cmd.h"


/*! State associated with the generic device. */
struct dev_generic
{

  /* Info about a particular transaction */

  enum
  {				/* Direction of the access */
    GENERIC_READ,
    GENERIC_WRITE
  } trans_direction;

  enum
  {				/* Size of the access */
    GENERIC_BYTE,
    GENERIC_HW,
    GENERIC_WORD
  } trans_size;

  uint32_t value;		/* The value to read/write */

  /* Configuration */

  int enabled;			/* Device enabled */
  int byte_enabled;		/* Byte R/W allowed */
  int hw_enabled;		/* Half word R/W allowed */
  int word_enabled;		/* Full word R/W allowed */
  char *name;			/* Name of the device */
  oraddr_t baseaddr;		/* Base address of device */
  uint32_t size;		/* Address space size (bytes) */

};


/* --------------------------------------------------------------------------*/
/*!Read a byte from an external device

   To model Wishbone accurately, we always do this as a 4-byte access, with a
   mask for the bytes we don't want.

   Since this is only a byte, the endianess of the result is irrelevant.

   @note We are passed the device address, but we must convert it to a full
         address for external use, to allow the single upcall handler to
         decode multiple generic devices.

   @param[in] addr  The device address to read from (host endian).
   @param[in] dat   The device data structure

   @return  The byte read.                                                   */
/* --------------------------------------------------------------------------*/
static uint8_t
generic_read_byte (oraddr_t  addr,
		   void     *dat)
{
  struct dev_generic *dev = (struct dev_generic *) dat;

  if (!config.ext.read_up)
    {
      fprintf (stderr, "Byte read from disabled generic device\n");
      return 0;
    }
  else if (addr >= dev->size)
    {
      fprintf (stderr, "Byte read  out of range for generic device %s "
	       "(addr %" PRIxADDR ")\n", dev->name, addr);
      return 0;
    }
  else
    {
      unsigned long int  fulladdr = (unsigned long int) (addr + dev->baseaddr);
      unsigned long int  wordaddr = fulladdr & 0xfffffffc;
      int                bytenum  = fulladdr & 0x00000003;

      unsigned char      mask[4];
      unsigned char      res[4];

      /* Set the mask, read and get the result */
      memset (mask, 0, sizeof (mask));
      mask[bytenum] = 0xff;

      if (0 != config.ext.read_up (config.ext.class_ptr, wordaddr, mask, res,
				   4))
	{
	  fprintf (stderr, "Warning: external byte read failed.\n");
	  return  0;
	}

      return  res[bytenum];
    }
}	/* generic_read_byte() */


/* --------------------------------------------------------------------------*/
/*!Write a byte to an external device

   To model Wishbone accurately, we always do this as a 4-byte access, with a
   mask for the bytes we don't want.

   Since this is only a byte, the endianess of the value is irrelevant.

   @note We are passed the device address, but we must convert it to a full
         address for external use, to allow the single upcall handler to
         decode multiple generic devices.

   @param[in] addr  The device address to write to (host endian)
   @param[in] value The byte value to write
   @param[in] dat   The device data structure                                */
/* --------------------------------------------------------------------------*/
static void
generic_write_byte (oraddr_t  addr,
		    uint8_t   value,
		    void     *dat)
{
  struct dev_generic *dev = (struct dev_generic *) dat;

  if (!config.ext.write_up)
    {
      fprintf (stderr, "Byte write to disabled generic device\n");
    }
  else if (addr >= dev->size)
    {
      fprintf (stderr, "Byte written out of range for generic device %s "
	       "(addr %" PRIxADDR ")\n", dev->name, addr);
    }
  else
    {
      unsigned long int  fulladdr = (unsigned long int) (addr + dev->baseaddr);
      unsigned long int  wordaddr = fulladdr & 0xfffffffc;
      int                bytenum  = fulladdr & 0x00000003;

      unsigned char      mask[4];
      unsigned char      val[4];

      /* Set the mask and write data do the write. */
      memset (mask, 0, sizeof (mask));
      mask[bytenum] = 0xff;
      val[bytenum]  = value;

      if (0 != config.ext.write_up (config.ext.class_ptr, wordaddr, mask, val,
				    4))
	{
	  fprintf (stderr, "Warning: external byte write failed.\n");
	}
    }
}	/* generic_write_byte() */


/* --------------------------------------------------------------------------*/
/*!Read a half word from an external device

   To model Wishbone accurately, we always do this as a 4-byte access, with a
   mask for the bytes we don't want.

   Since this is a half word, the result must be converted to host endianess.

   @note We are passed the device address, but we must convert it to a full
         address for external use, to allow the single upcall handler to
         decode multiple generic devices.

   @param[in] addr  The device address to read from (host endian).
   @param[in] dat   The device data structure.

   @return  The half word read (host endian).                                */
/* --------------------------------------------------------------------------*/
static uint16_t
generic_read_hw (oraddr_t  addr,
		 void     *dat)
{
  struct dev_generic *dev = (struct dev_generic *) dat;

  if (!config.ext.read_up)
    {
      fprintf (stderr, "Half word read from disabled generic device\n");
      return 0;
    }
  else if (addr >= dev->size)
    {
      fprintf (stderr, "Half-word read  out of range for generic device %s "
	       "(addr %" PRIxADDR ")\n", dev->name, addr);
      return 0;
    }
  else if (addr & 0x1)
    {
      /* This should be trapped elsewhere - here for safety. */
      fprintf (stderr,
	       "Unaligned half word read from 0x%" PRIxADDR " ignored\n",
	       addr);
      return 0;
    }
  else
    {
      unsigned long int  fulladdr = (unsigned long int) (addr + dev->baseaddr);
      unsigned long int  wordaddr = fulladdr & 0xfffffffc;
      int                hwnum    = fulladdr & 0x00000002;

      unsigned char      mask[4];
      unsigned char      res[4];

      /* Set the mask, read and get the result */
      memset (mask, 0, sizeof (mask));
      mask[hwnum    ] = 0xff;
      mask[hwnum + 1] = 0xff;

      if (0 != config.ext.read_up (config.ext.class_ptr, wordaddr, mask, res,
				   4))
	{
	  fprintf (stderr, "Warning: external half word read failed.\n");
	  return  0;
	}

      /* Result converted according to endianess */
#ifdef OR32_BIG_ENDIAN
      return  (unsigned short int) res[hwnum    ] << 8 |
	      (unsigned short int) res[hwnum + 1];
#else
      return  (unsigned short int) res[hwnum + 1] << 8 |
	      (unsigned short int) res[hwnum    ];
#endif
    }
}	/* generic_read_hw() */


/* --------------------------------------------------------------------------*/
/*!Write a half word to an external device

   To model Wishbone accurately, we always do this as a 4-byte access, with a
   mask for the bytes we don't want.

   Since this is a half word, the value must be converted from host endianess.

   @note We are passed the device address, but we must convert it to a full
         address for external use, to allow the single upcall handler to
         decode multiple generic devices.

   @param[in] addr  The device address to write to (host endian).
   @param[in] value The half word value to write (model endian).
   @param[in] dat   The device data structure.                               */
/* --------------------------------------------------------------------------*/
static void
generic_write_hw (oraddr_t  addr,
		  uint16_t  value,
		  void     *dat)
{
  struct dev_generic *dev = (struct dev_generic *) dat;

  if (!config.ext.write_up)
    {
      fprintf (stderr, "Half word write to disabled generic device\n");
    }
  else if (addr >= dev->size)
    {
      fprintf (stderr, "Half-word written  out of range for generic device %s "
	       "(addr %" PRIxADDR ")\n", dev->name, addr);
    }
  else if (addr & 0x1)
    {
      fprintf (stderr,
	       "Unaligned half word write to 0x%" PRIxADDR " ignored\n", addr);
    }
  else
    {
      unsigned long int  fulladdr = (unsigned long int) (addr + dev->baseaddr);
      unsigned long int  wordaddr = fulladdr & 0xfffffffc;
      int                hwnum    = fulladdr & 0x00000002;

      unsigned char      mask[4];
      unsigned char      val[4];

      /* Set the mask and write data do the write. */
      memset (mask, 0, sizeof (mask));
      mask[hwnum    ] = 0xff;
      mask[hwnum + 1] = 0xff;

      /* Value converted according to endianess */
#ifdef OR32_BIG_ENDIAN
      val[hwnum    ] = (unsigned char) (value >> 8);
      val[hwnum + 1] = (unsigned char) (value     );
#else
      val[hwnum + 1] = (unsigned char) (value >> 8);
      val[hwnum    ] = (unsigned char) (value     );
#endif

      if (0 != config.ext.write_up (config.ext.class_ptr, wordaddr, mask, val,
				    4))
	{
	  fprintf (stderr, "Warning: external half word write failed.\n");
	}
    }
}	/* generic_write_hw() */


/* --------------------------------------------------------------------------*/
/*!Read a full word from an external device

   Since this is a full word, the result must be converted to host endianess.

   @note We are passed the device address, but we must convert it to a full
         address for external use, to allow the single upcall handler to
         decode multiple generic devices.

   @param[in] addr  The device address to read from (host endian).
   @param[in] dat   The device data structure.

   @return  The full word read (host endian).                                */
/* --------------------------------------------------------------------------*/
static uint32_t
generic_read_word (oraddr_t  addr,
		   void     *dat)
{
  struct dev_generic *dev = (struct dev_generic *) dat;

  if (!config.ext.read_up)
    {
      fprintf (stderr, "Full word read from disabled generic device\n");
      return 0;
    }
  else if (addr >= dev->size)
    {
      fprintf (stderr, "Full word read  out of range for generic device %s "
	       "(addr %" PRIxADDR ")\n", dev->name, addr);
      return 0;
    }
  else if (0 != (addr & 0x3))
    {
      fprintf (stderr,
	       "Unaligned full word read from 0x%" PRIxADDR " ignored\n",
	       addr);
      return 0;
    }
  else
    {
      unsigned long int  wordaddr = (unsigned long int) (addr + dev->baseaddr);

      unsigned char      mask[4];
      unsigned char      res[4];

      /* Set the mask, read and get the result */
      memset (mask, 0xff, sizeof (mask));

      if (0 != config.ext.read_up (config.ext.class_ptr, wordaddr, mask, res,
				   4))
	{
	  fprintf (stderr, "Warning: external full word read failed.\n");
	  return  0;
	}

      /* Result converted according to endianess */
#ifdef OR32_BIG_ENDIAN
      return  (unsigned long int) res[0] << 24 |
	      (unsigned long int) res[1] << 16 |
	      (unsigned long int) res[2] <<  8 |
	      (unsigned long int) res[3];
#else
      return  (unsigned long int) res[3] << 24 |
	      (unsigned long int) res[2] << 16 |
	      (unsigned long int) res[1] <<  8 |
	      (unsigned long int) res[0];
#endif
    }
}	/* generic_read_word() */


/* --------------------------------------------------------------------------*/
/*!Write a full word to an external device

   Since this is a half word, the value must be converted from host endianess.

   @note We are passed the device address, but we must convert it to a full
         address for external use, to allow the single upcall handler to
         decode multiple generic devices.

   @param[in] addr  The device address to write to (host endian).
   @param[in] value The full word value to write (host endian).
   @param[in] dat   The device data structure.                               */
/* --------------------------------------------------------------------------*/
static void
generic_write_word (oraddr_t  addr,
		    uint32_t  value,
		    void     *dat)
{
  struct dev_generic *dev = (struct dev_generic *) dat;

  if (!config.ext.write_up)
    {
      fprintf (stderr, "Full word write to disabled generic device\n");
    }
  else if (addr >= dev->size)
    {
      fprintf (stderr, "Full word written  out of range for generic device %s "
	       "(addr %" PRIxADDR ")\n", dev->name, addr);
    }
  else if (0 != (addr & 0x3))
    {
      fprintf (stderr,
	       "Unaligned full word write to 0x%" PRIxADDR " ignored\n", addr);
    }
  else
    {
      unsigned long int  wordaddr = (unsigned long int) (addr + dev->baseaddr);

      unsigned char      mask[4];
      unsigned char      val[4];

      /* Set the mask and write data do the write. */
      memset (mask, 0xff, sizeof (mask));

      /* Value converted according to endianess */
#ifdef OR32_BIG_ENDIAN
      val[0] = (unsigned char) (value >> 24);
      val[1] = (unsigned char) (value >> 16);
      val[2] = (unsigned char) (value >>  8);
      val[3] = (unsigned char) (value      );
#else
      val[3] = (unsigned char) (value >> 24);
      val[2] = (unsigned char) (value >> 16);
      val[1] = (unsigned char) (value >>  8);
      val[0] = (unsigned char) (value      );
#endif

      if (0 != config.ext.write_up (config.ext.class_ptr, wordaddr, mask, val,
				    4))
	{
	  fprintf (stderr, "Warning: external full word write failed.\n");
	}
    }
}	/* generic_write_word() */


/* Reset is a null operation */

static void
generic_reset (void *dat)
{
  return;

}				/* generic_reset() */


/* Status report can only advise of configuration. */

static void
generic_status (void *dat)
{
  struct dev_generic *dev = (struct dev_generic *) dat;

  PRINTF ("\nGeneric device \"%s\" at 0x%" PRIxADDR ":\n", dev->name,
	  dev->baseaddr);
  PRINTF ("  Size 0x%" PRIx32 "\n", dev->size);

  if (dev->byte_enabled)
    {
      PRINTF ("  Byte R/W enabled\n");
    }

  if (dev->hw_enabled)
    {
      PRINTF ("  Half word R/W enabled\n");
    }

  if (dev->word_enabled)
    {
      PRINTF ("  Full word R/W enabled\n");
    }

  PRINTF ("\n");

}				/* generic_status() */


/* Functions to set configuration */

static void
generic_enabled (union param_val val, void *dat)
{
  ((struct dev_generic *) dat)->enabled = val.int_val;

}				/* generic_enabled() */


static void
generic_byte_enabled (union param_val val, void *dat)
{
  ((struct dev_generic *) dat)->byte_enabled = val.int_val;

}				/* generic_byte_enabled() */


static void
generic_hw_enabled (union param_val val, void *dat)
{
  ((struct dev_generic *) dat)->hw_enabled = val.int_val;

}				/* generic_hw_enabled() */


static void
generic_word_enabled (union param_val val, void *dat)
{
  ((struct dev_generic *) dat)->word_enabled = val.int_val;

}				/* generic_word_enabled() */


static void
generic_name (union param_val val, void *dat)
{
  ((struct dev_generic *) dat)->name = strdup (val.str_val);

  if (!((struct dev_generic *) dat)->name)
    {
      fprintf (stderr, "Peripheral 16450: name \"%s\": Run out of memory\n",
	       val.str_val);
      exit (-1);
    }
}				/* generic_name() */


static void
generic_baseaddr (union param_val val, void *dat)
{
  ((struct dev_generic *) dat)->baseaddr = val.addr_val;

}				/* generic_baseaddr() */


static void
generic_size (union param_val val, void *dat)
{
  ((struct dev_generic *) dat)->size = val.int_val;

}				/* generic_size() */


/* Start of new generic section */

static void *
generic_sec_start ()
{
  struct dev_generic *new =
    (struct dev_generic *) malloc (sizeof (struct dev_generic));

  if (0 == new)
    {
      fprintf (stderr, "Generic peripheral: Run out of memory\n");
      exit (-1);
    }

  /* Default names */

  new->enabled = 1;
  new->byte_enabled = 1;
  new->hw_enabled = 1;
  new->word_enabled = 1;
  new->name = "anonymous external peripheral";
  new->baseaddr = 0;
  new->size = 0;

  return new;

}				/* generic_sec_start() */


/* End of new generic section */

static void
generic_sec_end (void *dat)
{
  struct dev_generic *generic = (struct dev_generic *) dat;
  struct mem_ops ops;

  /* Give up if not enabled, or if size is zero, or if no access size is
     enabled. */

  if (!generic->enabled)
    {
      free (dat);
      return;
    }

  if (0 == generic->size)
    {
      fprintf (stderr, "Generic peripheral \"%s\" has size 0: ignoring",
	       generic->name);
      free (dat);
      return;
    }

  if (!generic->byte_enabled &&
      !generic->hw_enabled && !generic->word_enabled)
    {
      fprintf (stderr, "Generic peripheral \"%s\" has no access: ignoring",
	       generic->name);
      free (dat);
      return;
    }

  /* Zero all the ops, then set the ones we care about. Read/write delays will
   * come from the peripheral if desired.
   */

  memset (&ops, 0, sizeof (struct mem_ops));

  if (generic->byte_enabled)
    {
      ops.readfunc8 = generic_read_byte;
      ops.writefunc8 = generic_write_byte;
      ops.read_dat8 = dat;
      ops.write_dat8 = dat;
    }

  if (generic->hw_enabled)
    {
      ops.readfunc16 = generic_read_hw;
      ops.writefunc16 = generic_write_hw;
      ops.read_dat16 = dat;
      ops.write_dat16 = dat;
    }

  if (generic->word_enabled)
    {
      ops.readfunc32 = generic_read_word;
      ops.writefunc32 = generic_write_word;
      ops.read_dat32 = dat;
      ops.write_dat32 = dat;
    }

  /* Register everything */

  reg_mem_area (generic->baseaddr, generic->size, 0, &ops);

  reg_sim_reset (generic_reset, dat);
  reg_sim_stat (generic_status, dat);

}				/* generic_sec_end() */


/* Register a generic section. */

void
reg_generic_sec (void)
{
  struct config_section *sec = reg_config_sec ("generic",
					       generic_sec_start,
					       generic_sec_end);

  reg_config_param (sec, "enabled",      PARAMT_INT, generic_enabled);
  reg_config_param (sec, "byte_enabled", PARAMT_INT, generic_byte_enabled);
  reg_config_param (sec, "hw_enabled",   PARAMT_INT, generic_hw_enabled);
  reg_config_param (sec, "word_enabled", PARAMT_INT, generic_word_enabled);
  reg_config_param (sec, "name",         PARAMT_STR, generic_name);
  reg_config_param (sec, "baseaddr",     PARAMT_ADDR, generic_baseaddr);
  reg_config_param (sec, "size",         PARAMT_INT, generic_size);

}				/* reg_generic_sec */
