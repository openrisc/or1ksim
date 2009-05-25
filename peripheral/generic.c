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


/* Convert a 32-bit value from host to model endianess */
static unsigned long int
htoml (unsigned long int  host_val)
{
  unsigned char  model_array[4];

#ifdef OR32_BIG_ENDIAN
  model_array[0] = (host_val >> 24) & 0xff;
  model_array[1] = (host_val >> 16) & 0xff;
  model_array[2] = (host_val >>  8) & 0xff;
  model_array[3] = (host_val      ) & 0xff;
#else
  model_array[0] = (host_val      ) & 0xff;
  model_array[1] = (host_val >>  8) & 0xff;
  model_array[2] = (host_val >> 16) & 0xff;
  model_array[3] = (host_val >> 24) & 0xff;
#endif

  return *((unsigned long int *)model_array);

}	/* htoml () */


/* Convert a 16-bit value from host to model endianess */
static unsigned short int
htoms (unsigned short int  host_val)
{
  unsigned char  model_array[2];

#ifdef OR32_BIG_ENDIAN
  model_array[0] = (host_val >>  8) & 0xff;
  model_array[1] = (host_val      ) & 0xff;
#else
  model_array[0] = (host_val      ) & 0xff;
  model_array[1] = (host_val >>  8) & 0xff;
#endif

  return *((unsigned short int *)model_array);

}	/* htoms () */


/* Convert a 32-bit value from model to host endianess */
static unsigned long int
mtohl (unsigned long int  model_val)
{
  unsigned char     *model_array = (unsigned char *)(&model_val);
  unsigned long int  host_val;

#ifdef OR32_BIG_ENDIAN
  host_val =                   model_array[0];
  host_val = (host_val << 8) | model_array[1];
  host_val = (host_val << 8) | model_array[2];
  host_val = (host_val << 8) | model_array[3];
#else
  host_val =                   model_array[3];
  host_val = (host_val << 8) | model_array[2];
  host_val = (host_val << 8) | model_array[1];
  host_val = (host_val << 8) | model_array[0];
#endif

  return  host_val;

}	/* mtohl () */


/* Convert a 32-bit value from model to host endianess */
static unsigned short int
mtohs (unsigned short int  model_val)
{
  unsigned char      *model_array = (unsigned char *)(&model_val);
  unsigned short int  host_val;

#ifdef OR32_BIG_ENDIAN
  host_val =                   model_array[0];
  host_val = (host_val << 8) | model_array[1];
#else
  host_val =                   model_array[1];
  host_val = (host_val << 8) | model_array[0];
#endif

  return  host_val;

}	/* mtohs () */


/* Generic read and write upcall routines. Note the address here is absolute,
   not relative to the device. The mask uses host endianess, not Or1ksim
   endianess. */

static unsigned long int
ext_read (unsigned long int  addr,
	  unsigned long int  mask)
{
  return config.ext.read_up (config.ext.class_ptr, addr, mask);

}				/* ext_callback() */


/* Generic read and write upcall routines. Note the address here is absolute,
   not relative to the device. The mask and value use host endianess, not
   Or1ksim endianess. */

static void
ext_write (unsigned long int  addr,
	   unsigned long int  mask,
	   unsigned long int  value)
{
  config.ext.write_up (config.ext.class_ptr, addr, mask, value);

}				/* ext_callback() */


/* I/O routines. Note that address is relative to start of address space. */

static uint8_t
generic_read_byte (oraddr_t addr, void *dat)
{
  struct dev_generic *dev = (struct dev_generic *) dat;

  if (!config.ext.class_ptr)
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
      unsigned long  fulladdr = (unsigned long int) (addr + dev->baseaddr);
      unsigned long  wordaddr = fulladdr & 0xfffffffc;
      unsigned long  bytenum  = fulladdr & 0x00000003;

      uint8_t        mask_array[4];
      unsigned long  res;
      uint8_t       *res_array;

      /* This works whatever the host endianess */
      memset (mask_array, 0, 4);
      mask_array[bytenum] = 0xff;

      res       = ext_read (wordaddr, *((unsigned int *)mask_array));
      res_array = (uint8_t *)(&res);

      return  res_array[bytenum];
    }
}				/* generic_read_byte() */


static void
generic_write_byte (oraddr_t addr, uint8_t value, void *dat)
{
  struct dev_generic *dev = (struct dev_generic *) dat;

  if (!config.ext.class_ptr)
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
      unsigned long  fulladdr = (unsigned long int) (addr + dev->baseaddr);
      unsigned long  wordaddr = fulladdr & 0xfffffffc;

      unsigned long  bytenum  = fulladdr & 0x00000003;
      uint8_t        mask_array[4];
      uint8_t        value_array[4];

      /* This works whatever the host endianess */
      memset (mask_array, 0, 4);
      mask_array[bytenum] = 0xff;
      memset (value_array, 0, 4);
      value_array[bytenum] = value;

      ext_write (wordaddr, *((unsigned long int *)mask_array),
		 *((unsigned long int *)value_array));
    }
}				/* generic_write_byte() */


/* Result is in model endianess */
static uint16_t
generic_read_hw (oraddr_t addr, void *dat)
{
  struct dev_generic *dev = (struct dev_generic *) dat;

  if (!config.ext.class_ptr)
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
      fprintf (stderr,
	       "Unaligned half word read from 0x%" PRIxADDR " ignored\n",
	       addr);
      return 0;
    }
  else
    {
      unsigned long   fulladdr = (unsigned long int) (addr + dev->baseaddr);
      unsigned long   wordaddr = fulladdr & 0xfffffffc;
      unsigned long   bytenum  = fulladdr & 0x00000002;

      uint8_t         mask_array[4];
      unsigned long   res;
      uint8_t        *res_array;
      uint8_t         hwres_array[2];

      /* This works whatever the host endianess */
      memset (mask_array, 0, 4);
      mask_array[bytenum]     = 0xff;
      mask_array[bytenum + 1] = 0xff;

      res       = ext_read (wordaddr, *((unsigned int *)mask_array));
      res_array = (uint8_t *)(&res);

      hwres_array[0] = res_array[bytenum];
      hwres_array[1] = res_array[bytenum + 1];

      return htoms (*((uint16_t *)hwres_array));
    }
}				/* generic_read_hw() */


/* Value is in model endianness */
static void
generic_write_hw (oraddr_t addr, uint16_t value, void *dat)
{
  struct dev_generic *dev = (struct dev_generic *) dat;

  if (!config.ext.class_ptr)
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
      uint16_t       host_value = mtohs (value);

      unsigned long  fulladdr = (unsigned long int) (addr + dev->baseaddr);
      unsigned long  wordaddr = fulladdr & 0xfffffffc;
      unsigned long  bytenum  = fulladdr & 0x00000002;

      uint8_t        mask_array[4];
      uint8_t        value_array[4];
      uint8_t       *hw_value_array;

      /* This works whatever the host endianess */
      memset (mask_array, 0, 4);
      mask_array[bytenum]     = 0xff;
      mask_array[bytenum + 1] = 0xff;

      memset (value_array, 0, 4);
      hw_value_array           = (uint8_t *)(&host_value);
      value_array[bytenum]     = hw_value_array[0];
      value_array[bytenum + 1] = hw_value_array[1];

      ext_write (wordaddr, *((unsigned long int *)mask_array),
		 *((unsigned long int *)value_array));
    }
}				/* generic_write_hw() */


static uint32_t
generic_read_word (oraddr_t addr, void *dat)
{
  struct dev_generic *dev = (struct dev_generic *) dat;

  if (!config.ext.class_ptr)
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
      unsigned long wordaddr = (unsigned long int) (addr + dev->baseaddr);

      return (uint32_t) htoml (ext_read (wordaddr, 0xffffffff));
    }
}				/* generic_read_word() */


static void
generic_write_word (oraddr_t addr, uint32_t value, void *dat)
{
  struct dev_generic *dev = (struct dev_generic *) dat;

  if (!config.ext.class_ptr)
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
      unsigned long host_value = mtohl (value);
      unsigned long wordaddr   = (unsigned long int) (addr + dev->baseaddr);

      ext_write (wordaddr, 0xffffffff, host_value);
    }
}				/* generic_write_word() */


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

  reg_config_param (sec, "enabled", paramt_int, generic_enabled);
  reg_config_param (sec, "byte_enabled", paramt_int, generic_byte_enabled);
  reg_config_param (sec, "hw_enabled", paramt_int, generic_hw_enabled);
  reg_config_param (sec, "word_enabled", paramt_int, generic_word_enabled);
  reg_config_param (sec, "name", paramt_str, generic_name);
  reg_config_param (sec, "baseaddr", paramt_addr, generic_baseaddr);
  reg_config_param (sec, "size", paramt_int, generic_size);

}				/* reg_generic_sec */
