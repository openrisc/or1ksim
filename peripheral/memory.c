/* memory.c -- Generic memory model

   Copyright (C) 1999 Damjan Lampret, lampret@opencores.org
   Copyright (C) 2005 György `nog' Jeney, nog@sdf.lonestar.org
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
#include <stdio.h>
#include <time.h>

/* Package includes */
#include "arch.h"
#include "sim-config.h"
#include "abstract.h"
#include "mc.h"
#include "toplevel-support.h"


struct mem_config
{
  int ce;			/* Which ce this memory is associated with */
  int mc;			/* Which mc this memory is connected to */
  oraddr_t baseaddr;		/* Start address of the memory */
  unsigned int size;		/* Memory size */
  char *name;			/* Memory type string */
  char *log;			/* Memory log filename */
  int delayr;			/* Read cycles */
  int delayw;			/* Write cycles */

  void *mem;			/* malloced memory for this memory */

  int pattern;			/* A user specified memory initialization
				 * pattern */
  int random_seed;		/* Initialize the memory with random values,
				 * starting with seed */
  enum
  {
    MT_UNKNOWN,
    MT_PATTERN,
    MT_RANDOM,
    MT_EXITNOPS
  } type;
};

static uint32_t
simmem_read32 (oraddr_t addr, void *dat)
{
  return *(uint32_t *) (dat + addr);
}

static uint16_t
simmem_read16 (oraddr_t addr, void *dat)
{
#ifdef WORDS_BIGENDIAN
  return *(uint16_t *) (dat + addr);
#else
  return *(uint16_t *) (dat + (addr ^ 2));
#endif
}

static uint8_t
simmem_read8 (oraddr_t addr, void *dat)
{
#ifdef WORDS_BIGENDIAN
  return *(uint8_t *) (dat + addr);
#else
  return *(uint8_t *) (dat + ((addr & ~ADDR_C (3)) | (3 - (addr & 3))));
#endif
}

static void
simmem_write32 (oraddr_t addr, uint32_t value, void *dat)
{
  *(uint32_t *) (dat + addr) = value;
}

static void
simmem_write16 (oraddr_t addr, uint16_t value, void *dat)
{
#ifdef WORDS_BIGENDIAN
  *(uint16_t *) (dat + addr) = value;
#else
  *(uint16_t *) (dat + (addr ^ 2)) = value;
#endif
}

static void
simmem_write8 (oraddr_t addr, uint8_t value, void *dat)
{
#ifdef WORDS_BIGENDIAN
  *(uint8_t *) (dat + addr) = value;
#else
  *(uint8_t *) (dat + ((addr & ~ADDR_C (3)) | (3 - (addr & 3)))) = value;
#endif
}

static uint32_t
simmem_read_zero32 (oraddr_t addr, void *dat)
{
  if (config.sim.verbose)
    fprintf (stderr,
	     "WARNING: 32-bit memory read from non-read memory area 0x%"
	     PRIxADDR ".\n", addr);
  return 0;
}

static uint16_t
simmem_read_zero16 (oraddr_t addr, void *dat)
{
  if (config.sim.verbose)
    fprintf (stderr,
	     "WARNING: 16-bit memory read from non-read memory area 0x%"
	     PRIxADDR ".\n", addr);
  return 0;
}

static uint8_t
simmem_read_zero8 (oraddr_t addr, void *dat)
{
  if (config.sim.verbose)
    fprintf (stderr,
	     "WARNING: 8-bit memory read from non-read memory area 0x%"
	     PRIxADDR ".\n", addr);
  return 0;
}

static void
simmem_write_null32 (oraddr_t addr, uint32_t value, void *dat)
{
  if (config.sim.verbose)
    fprintf (stderr,
	     "WARNING: 32-bit memory write to 0x%" PRIxADDR ", non-write "
	     "memory area (value 0x%08" PRIx32 ").\n", addr, value);
}

static void
simmem_write_null16 (oraddr_t addr, uint16_t value, void *dat)
{
  if (config.sim.verbose)
    fprintf (stderr,
	     "WARNING: 16-bit memory write to 0x%" PRIxADDR ", non-write "
	     "memory area (value 0x%08" PRIx16 ").\n", addr, value);
}

static void
simmem_write_null8 (oraddr_t addr, uint8_t value, void *dat)
{
  if (config.sim.verbose)
    fprintf (stderr,
	     "WARNING: 8-bit memory write to 0x%" PRIxADDR ", non-write "
	     "memory area (value 0x%08" PRIx8 ").\n", addr, value);
}

static void
mem_reset (void *dat)
{
  struct mem_config *mem = dat;
  int seed;
  int i;
  uint8_t *mem_area = mem->mem;

  /* Initialize memory */
  switch (mem->type)
    {
    case MT_RANDOM:
      if (mem->random_seed == -1)
	{
	  /* seed = time (NULL); */
	  /* srandom (seed); */
	  /* /\* Print out the seed just in case we ever need to debug *\/ */
	  /* PRINTF ("Seeding random generator with value %d\n", seed); */
	}
      else
	{
	  seed = mem->random_seed;
	  srandom (seed);
	}

      for (i = 0; i < mem->size; i++, mem_area++)
	*mem_area = random () & 0xFF;
      break;
    case MT_PATTERN:
      for (i = 0; i < mem->size; i++, mem_area++)
	*mem_area = mem->pattern;
      break;
    case MT_UNKNOWN:
      break;
    case MT_EXITNOPS:
      /* Fill memory with OR1K exit NOP */
      for (i = 0; i < mem->size; i++, mem_area++)
	switch(i & 0x3) {
	case 3:
	  *mem_area = 0x15;
	  break;
	case 0:
	  *mem_area = 0x01;
	  break;
	default:
	  *mem_area = 0x00;
	  break;
	}
      break;
    default:
      fprintf (stderr, "Invalid memory configuration type.\n");
      exit (1);
    }
}

/*-------------------------------------------------[ Memory configuration ]---*/
static void
memory_random_seed (union param_val val, void *dat)
{
  struct mem_config *mem = dat;
  mem->random_seed = val.int_val;
}

/*---------------------------------------------------------------------------*/
/*!Set the memory pattern

   Value must be up to 8 bits. Larger values are truncated with a warning.

   @param[in] val  The value to use
   @param[in] dat  The config data structure                                 */
/*---------------------------------------------------------------------------*/
static void
memory_pattern (union param_val val, void *dat)
{
  struct mem_config *mem = dat;

  if (val.int_val > 0xff)
    {
      fprintf (stderr, "Warning: memory pattern exceeds 8-bits: truncated\n");
    }

  mem->pattern = val.int_val & 0xff;

}	/* memory_pattern() */


/*---------------------------------------------------------------------------*/
/*!Set the memory type

   Value must be one of unknown, random, pattern or zero (case
   insensitive). Unrecognized values are ignored with a warning.

   @param[in] val  The value to use
   @param[in] dat  The config data structure                                 */
/*---------------------------------------------------------------------------*/
static void
memory_type (union param_val val, void *dat)
{
  struct mem_config *mem = dat;

  if (0 == strcasecmp (val.str_val, "unknown"))
    {
      mem->type = MT_UNKNOWN;
    }
  else if (0 == strcasecmp (val.str_val, "random"))
    {
      mem->type = MT_RANDOM;
    }
  else if (0 == strcasecmp (val.str_val, "pattern"))
    {
      mem->type = MT_PATTERN;
    }
  else if (0 == strcasecmp (val.str_val, "zero"))
    {
      mem->type = MT_PATTERN;
      mem->pattern = 0;
    }
  else if (0 == strcasecmp (val.str_val, "exitnops"))
    {
      mem->type = MT_EXITNOPS;
      mem->pattern = 0;
    }
  else
    {
      fprintf (stderr, "Warning: memory type invalid. Ignored");
    }
}				/* memory_type() */


static void
memory_ce (union param_val val, void *dat)
{
  struct mem_config *mem = dat;
  mem->ce = val.int_val;
}

static void
memory_mc (union param_val val, void *dat)
{
  struct mem_config *mem = dat;
  mem->mc = val.int_val;
}

static void
memory_baseaddr (union param_val val, void *dat)
{
  struct mem_config *mem = dat;
  mem->baseaddr = val.addr_val;
}

static void
memory_size (union param_val val, void *dat)
{
  struct mem_config *mem = dat;
  mem->size = val.int_val;
}

/* FIXME: Check use */
static void
memory_name (union param_val val, void *dat)
{
  struct mem_config *mem = dat;

  if (NULL != mem->name)
    {
      free (mem->name);
    }

  mem->name = strdup (val.str_val);
}

static void
memory_log (union param_val val, void *dat)
{
  struct mem_config *mem = dat;
  mem->log = strdup (val.str_val);
}

static void
memory_delayr (union param_val val, void *dat)
{
  struct mem_config *mem = dat;
  mem->delayr = val.int_val;
}

static void
memory_delayw (union param_val val, void *dat)
{
  struct mem_config *mem = dat;
  mem->delayw = val.int_val;
}

/*---------------------------------------------------------------------------*/
/*!Initialize a new block of memory configuration

   ALL parameters are set explicitly to default values.

   @return  The new memory configuration data structure                      */
/*---------------------------------------------------------------------------*/
static void *
memory_sec_start ()
{
  struct mem_config *mem = malloc (sizeof (struct mem_config));

  if (!mem)
    {
      fprintf (stderr, "Memory Peripheral: Run out of memory\n");
      exit (-1);
    }

  mem->type = MT_UNKNOWN;
  mem->random_seed = -1;
  mem->pattern = 0;
  mem->baseaddr = 0;
  mem->size = 1024;
  mem->name = strdup ("anonymous memory block");
  mem->ce = -1;
  mem->mc = 0;
  mem->delayr = 1;
  mem->delayw = 1;
  mem->log = NULL;

  return mem;

}				/* memory_sec_start() */


static void
memory_sec_end (void *dat)
{
  struct mem_config *mem = dat;
  struct dev_memarea *mema;

  struct mem_ops ops;

  if (!mem->size)
    {
      free (dat);
      return;
    }

  /* Round up to the next 32-bit boundry */
  if (mem->size & 3)
    {
      mem->size &= ~3;
      mem->size += 4;
    }

  if (!(mem->mem = malloc (mem->size)))
    {
      fprintf (stderr,
	       "Unable to allocate memory at %" PRIxADDR ", length %i\n",
	       mem->baseaddr, mem->size);
      exit (-1);
    }

  if (mem->delayr > 0)
    {
      ops.readfunc32 = simmem_read32;
      ops.readfunc16 = simmem_read16;
      ops.readfunc8 = simmem_read8;
    }
  else
    {
      ops.readfunc32 = simmem_read_zero32;
      ops.readfunc16 = simmem_read_zero16;
      ops.readfunc8 = simmem_read_zero8;
    }

  if (mem->delayw > 0)
    {
      ops.writefunc32 = simmem_write32;
      ops.writefunc16 = simmem_write16;
      ops.writefunc8 = simmem_write8;
    }
  else
    {
      ops.writefunc32 = simmem_write_null32;
      ops.writefunc16 = simmem_write_null16;
      ops.writefunc8 = simmem_write_null8;
    }

  ops.writeprog8 = simmem_write8;
  ops.writeprog32 = simmem_write32;
  ops.writeprog8_dat = mem->mem;
  ops.writeprog32_dat = mem->mem;

  ops.read_dat32 = mem->mem;
  ops.read_dat16 = mem->mem;
  ops.read_dat8 = mem->mem;

  ops.write_dat32 = mem->mem;
  ops.write_dat16 = mem->mem;
  ops.write_dat8 = mem->mem;

  ops.delayr = mem->delayr;
  ops.delayw = mem->delayw;

  ops.log = mem->log;

  mema = reg_mem_area (mem->baseaddr, mem->size, 0, &ops);

  /* Set valid */
  /* FIXME: Should this be done during reset? */
  set_mem_valid (mema, 1);

  if (mem->ce >= 0)
    mc_reg_mem_area (mema, mem->ce, mem->mc);

  reg_sim_reset (mem_reset, dat);
}

void
reg_memory_sec (void)
{
  struct config_section *sec = reg_config_sec ("memory", memory_sec_start,
					       memory_sec_end);

  reg_config_param (sec, "type",        PARAMT_WORD, memory_type);
  reg_config_param (sec, "random_seed", PARAMT_INT, memory_random_seed);
  reg_config_param (sec, "pattern",     PARAMT_INT, memory_pattern);
  reg_config_param (sec, "baseaddr",    PARAMT_ADDR, memory_baseaddr);
  reg_config_param (sec, "size",        PARAMT_INT, memory_size);
  reg_config_param (sec, "name",        PARAMT_STR, memory_name);
  reg_config_param (sec, "ce",          PARAMT_INT, memory_ce);
  reg_config_param (sec, "mc",          PARAMT_INT, memory_mc);
  reg_config_param (sec, "delayr",      PARAMT_INT, memory_delayr);
  reg_config_param (sec, "delayw",      PARAMT_INT, memory_delayw);
  reg_config_param (sec, "log",         PARAMT_STR, memory_log);
}
