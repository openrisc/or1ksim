/* mc.c -- Simulation of Memory Controller

   Copyright (C) 2001 by Marko Mlinar, markom@opencores.org
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


/* Enable memory controller, via:
  section mc
    enable = 1
    POC = 0x13243545
  end

   Limitations:
    - memory refresh is not simulated
*/


/* Autoconf and/or portability configuration */
#include "config.h"
#include "port.h"

/* System includes */
#include <stdlib.h>

/* Package includes */
#include "arch.h"
#include "abstract.h"
#include "sim-config.h"
#include "toplevel-support.h"
#include "sim-cmd.h"
#include "mc.h"


struct mc_area
{
  struct dev_memarea *mem;
  unsigned int cs;
  int mc;
  struct mc_area *next;
};

struct mc
{
  uint32_t csr;
  uint32_t poc;
  uint32_t ba_mask;
  uint32_t csc[N_CE];
  uint32_t tms[N_CE];
  oraddr_t baseaddr;
  int enabled;

  /* Index of this memory controler amongst all the memory controlers */
  int index;
  /* List of memory devices under this mc's control */
  struct mc_area *mc_areas;

  struct mc *next;
};

static struct mc *mcs = NULL;

/* List used to temporarily hold memory areas registered with the mc, while the
 * mc configureation has not been loaded */
static struct mc_area *mc_areas = NULL;

void
set_csc_tms (int cs, uint32_t csc, uint32_t tms, struct mc *mc)
{
  struct mc_area *cur = mc->mc_areas;

  while (cur)
    {
      if (cur->cs == cs)
	{
	  /* FIXME: No peripheral should _ever_ acess a dev_memarea structure
	   * directly */
	  cur->mem->addr_mask = mc->ba_mask << 22;
	  cur->mem->addr_compare =
	    ((csc >> MC_CSC_SEL_OFFSET) /* & 0xff */ ) << 22;
	  set_mem_valid (cur->mem, (csc >> MC_CSC_EN_OFFSET) & 0x01);

	  if ((csc >> MC_CSC_MEMTYPE_OFFSET) && 0x07 == MC_CSC_MEMTYPE_ASYNC)
	    {
	      adjust_rw_delay (cur->mem, (tms & 0xff) + ((tms >> 8) & 0x0f),
			       ((tms >> 12) & 0x0f) + ((tms >> 16) & 0x0f) +
			       ((tms >> 20) & 0x3f));
	    }
	  else if ((csc >> MC_CSC_MEMTYPE_OFFSET)
		   && 0x07 == MC_CSC_MEMTYPE_SDRAM)
	    {
	      adjust_rw_delay (cur->mem, 3 + ((tms >> 4) & 0x03),
			       3 + ((tms >> 4) & 0x03));
	    }
	  else if ((csc >> MC_CSC_MEMTYPE_OFFSET)
		   && 0x07 == MC_CSC_MEMTYPE_SSRAM)
	    {
	      adjust_rw_delay (cur->mem, 2, 2);
	    }
	  else if ((csc >> MC_CSC_MEMTYPE_OFFSET)
		   && 0x07 == MC_CSC_MEMTYPE_SYNC)
	    {
	      adjust_rw_delay (cur->mem, 2, 2);
	    }
	  return;
	}
      cur = cur->next;
    }
}

/* Set a specific MC register with value. */
void
mc_write_word (oraddr_t addr, uint32_t value, void *dat)
{
  struct mc *mc = dat;
  int chipsel;

  switch (addr)
    {
    case MC_CSR:
      mc->csr = value;
      break;
    case MC_POC:
      fprintf (stderr, "Warning: write to MC's POC register!");
      break;
    case MC_BA_MASK:
      mc->ba_mask = value & MC_BA_MASK_VALID;
      for (chipsel = 0; chipsel < N_CE; chipsel++)
	set_csc_tms (chipsel, mc->csc[chipsel], mc->tms[chipsel], mc);
      break;
    default:
      if (addr >= MC_CSC (0) && addr <= MC_TMS (N_CE - 1))
	{
	  addr -= MC_CSC (0);
	  if ((addr >> 2) & 1)
	    mc->tms[addr >> 3] = value;
	  else
	    mc->csc[addr >> 3] = value;

	  set_csc_tms (addr >> 3, mc->csc[addr >> 3], mc->tms[addr >> 3], mc);
	  break;
	}
    }
}

/* Read a specific MC register. */
uint32_t
mc_read_word (oraddr_t addr, void *dat)
{
  struct mc *mc = dat;
  uint32_t value = 0;

  switch (addr)
    {
    case MC_CSR:
      value = mc->csr;
      break;
    case MC_POC:
      value = mc->poc;
      break;
    case MC_BA_MASK:
      value = mc->ba_mask;
      break;
    default:
      if (addr >= MC_CSC (0) && addr <= MC_TMS (N_CE - 1))
	{
	  addr -= MC_CSC (0);
	  if ((addr >> 2) & 1)
	    value = mc->tms[addr >> 3];
	  else
	    value = mc->csc[addr >> 3];
	}

      break;
    }
  return value;
}

/* Read POC register and init memory controler regs. */
void
mc_reset (void *dat)
{
  struct mc *mc = dat;
  struct mc_area *cur, *prev, *tmp;

  PRINTF ("Resetting memory controller.\n");

  memset (mc->csc, 0, sizeof (mc->csc));
  memset (mc->tms, 0, sizeof (mc->tms));

  mc->csr = 0;
  mc->ba_mask = 0;

  /* Set CS0 */
  mc->csc[0] =
    (((mc->poc & 0x0c) >> 2) << MC_CSC_MEMTYPE_OFFSET) | ((mc->
							   poc & 0x03) <<
							  MC_CSC_BW_OFFSET) |
    1;

  if ((mc->csc[0] >> MC_CSC_MEMTYPE_OFFSET) && 0x07 == MC_CSC_MEMTYPE_ASYNC)
    {
      mc->tms[0] = MC_TMS_ASYNC_VALID;
    }
  else if ((mc->csc[0] >> MC_CSC_MEMTYPE_OFFSET)
	   && 0x07 == MC_CSC_MEMTYPE_SDRAM)
    {
      mc->tms[0] = MC_TMS_SDRAM_VALID;
    }
  else if ((mc->csc[0] >> MC_CSC_MEMTYPE_OFFSET)
	   && 0x07 == MC_CSC_MEMTYPE_SSRAM)
    {
      mc->tms[0] = MC_TMS_SSRAM_VALID;
    }
  else if ((mc->csc[0] >> MC_CSC_MEMTYPE_OFFSET)
	   && 0x07 == MC_CSC_MEMTYPE_SYNC)
    {
      mc->tms[0] = MC_TMS_SYNC_VALID;
    }

  /* Grab control over all the devices we are destined to control */
  cur = mc_areas;
  prev = NULL;
  while (cur)
    {
      if (cur->mc == mc->index)
	{
	  if (prev)
	    prev->next = cur->next;
	  else
	    mc_areas = cur->next;
	  prev = cur;
	  tmp = cur->next;
	  cur->next = mc->mc_areas;
	  mc->mc_areas = cur;
	  cur = tmp;
	}
      else
	{
	  prev = cur;
	  cur = cur->next;
	}
    }

  for (cur = mc->mc_areas; cur; cur = cur->next)
    set_mem_valid (cur->mem, 0);

  set_csc_tms (0, mc->csc[0], mc->tms[0], mc);
}

/*---------------------------------------------------------------------------*/
/*!Free all allocated memory                                                 */
/*---------------------------------------------------------------------------*/
void
mc_done ()
{
  while (NULL != mc_areas)
    {
      struct mc_area *next = mc_areas->next;

      free (mc_areas);
      mc_areas = next;
    }
}	/* mc_done () */


void
mc_status (void *dat)
{
  struct mc *mc = dat;
  int i;

  PRINTF ("\nMemory Controller at 0x%" PRIxADDR ":\n", mc->baseaddr);
  PRINTF ("POC: 0x%" PRIx32 "\n", mc->poc);
  PRINTF ("BAS: 0x%" PRIx32 "\n", mc->ba_mask);
  PRINTF ("CSR: 0x%" PRIx32 "\n", mc->csr);

  for (i = 0; i < N_CE; i++)
    {
      PRINTF ("CE %02d -  CSC: 0x%" PRIx32 "  TMS: 0x%" PRIx32 "\n", i,
	      mc->csc[i], mc->tms[i]);
    }
}

/*--------------------------------------------[ Peripheral<->MC interface ]---*/
/* Registers some memory to be under the memory controllers control */
void
mc_reg_mem_area (struct dev_memarea *mem, unsigned int cs, int mc)
{
  struct mc_area *new = malloc (sizeof (struct mc_area));

  if (NULL == new)
    {
      fprintf (stderr, "Out-of-memory\n");
      exit (-1);
    }

  new->cs   = cs;
  new->mem  = mem;
  new->mc   = mc;

  new->next = mc_areas;

  mc_areas  = new;
}

/*-----------------------------------------------------[ MC configuration ]---*/
static void
mc_enabled (union param_val val, void *dat)
{
  struct mc *mc = dat;
  mc->enabled = val.int_val;
}

static void
mc_baseaddr (union param_val val, void *dat)
{
  struct mc *mc = dat;
  mc->baseaddr = val.addr_val;
}


/*---------------------------------------------------------------------------*/
/*!Set the power on configuration state

   Only the bottom 4 bits are signficant. Other bits are truncated with a
   warning.

   @param[in] val  The value to use
   @param[in] dat  The config data structure                                 */
/*---------------------------------------------------------------------------*/
static void
mc_poc (union param_val  val,
	void            *dat)
{
  struct mc *mc = dat;

  if (val.int_val > MC_POC_VALID)
    {
      fprintf (stderr, "Warning: memory controller PoC > 0x%lx: truncated\n",
	       MC_POC_VALID);
    }

  mc->poc = val.int_val & MC_POC_VALID;

}	/* mc_poc() */


/*---------------------------------------------------------------------------*/
/*!Set the index of this memory controller.

   This identifies which chip enable value will select this memory
   controller. Truncate if the value is too large with a warning.

   @param[in] val  The value to use
   @param[in] dat  The config data structure                                 */
/*---------------------------------------------------------------------------*/
static void
mc_index (union param_val val, void *dat)
{
  struct mc *mc = dat;

  if (val.int_val > MC_CE_VALID)
    {
      fprintf (stderr, "Warning: memory controller index > 0x%x: truncated.\n",
	       MC_CE_VALID);
    }
      
  mc->index = val.int_val & MC_CE_VALID;
}


/*---------------------------------------------------------------------------*/
/*!Initialize a new memory controller configuration

   ALL parameters are set explicitly to default values.                      */
/*---------------------------------------------------------------------------*/
static void *
mc_sec_start ()
{
  struct mc *new = malloc (sizeof (struct mc));

  if (!new)
    {
      fprintf (stderr, "Peripheral MC: Run out of memory\n");
      exit (-1);
    }

  new->enabled  = 1;
  new->baseaddr = 0;
  new->poc      = 0;
  new->index    = 0;

  new->mc_areas = NULL;

  return new;
}

static void
mc_sec_end (void *dat)
{
  struct mc *mc = dat;
  struct mem_ops ops;

  if (!mc->enabled)
    {
      free (dat);
      return;
    }

  /* FIXME: Check to see that the index given to this mc is unique */

  mc->next = mcs;
  mcs = mc;

  memset (&ops, 0, sizeof (struct mem_ops));

  ops.readfunc32 = mc_read_word;
  ops.writefunc32 = mc_write_word;
  ops.write_dat32 = dat;
  ops.read_dat32 = dat;

  /* FIXME: Correct delays? */
  ops.delayr = 2;
  ops.delayw = 2;

  reg_mem_area (mc->baseaddr, MC_ADDR_SPACE, 1, &ops);
  reg_sim_reset (mc_reset, dat);
  reg_sim_stat (mc_status, dat);
}

void
reg_mc_sec (void)
{
  struct config_section *sec =
    reg_config_sec ("mc", mc_sec_start, mc_sec_end);

  reg_config_param (sec, "enabled",  PARAMT_INT, mc_enabled);
  reg_config_param (sec, "baseaddr", PARAMT_ADDR, mc_baseaddr);
  reg_config_param (sec, "POC",      PARAMT_INT, mc_poc);
  reg_config_param (sec, "index",    PARAMT_INT, mc_index);
}
