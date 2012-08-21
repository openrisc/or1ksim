/* icache-model.c -- instruction cache simulation

   Copyright (C) 1999 Damjan Lampret, lampret@opencores.org
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

/* Cache functions. 
   At the moment this functions only simulate functionality of instruction
   caches and do not influence on fetche/decode/execute stages and timings.
   They are here only to verify performance of various cache configurations.
 */


/* Autoconf and/or portability configuration */
#include "config.h"
#include "port.h"

/* System includes */
#include <stdlib.h>

/* Package includes */
#include "icache-model.h"
#include "execute.h"
#include "spr-defs.h"
#include "abstract.h"
#include "misc.h"
#include "stats.h"
#include "sim-cmd.h"
#include "pcu.h"

#define MAX_IC_SETS        1024
#define MAX_IC_WAYS          32
#define MIN_IC_BLOCK_SIZE    16
#define MAX_IC_BLOCK_SIZE    32


struct ic *ic_state = NULL;

static void
ic_info (void *dat)
{
  struct ic *ic = dat;
  if (!(cpu_state.sprs[SPR_UPR] & SPR_UPR_ICP))
    {
      PRINTF ("ICache not implemented. Set UPR[ICP].\n");
      return;
    }

  PRINTF ("Instruction cache %dKB: ",
	  ic->nsets * ic->blocksize * ic->nways / 1024);
  PRINTF ("%d ways, %d sets, block size %d bytes\n", ic->nways, ic->nsets,
	  ic->blocksize);
}

/* First check if instruction is already in the cache and if it is:
    - increment IC read hit stats,
    - set 'lru' at this way to ic->ustates - 1 and
      decrement 'lru' of other ways unless they have reached 0,
    - read insn from the cache line
   and if not:
    - increment IC read miss stats
    - find lru way and entry and replace old tag with tag of the 'fetchaddr'
    - set 'lru' with ic->ustates - 1 and decrement 'lru' of other
      ways unless they have reached 0
    - refill cache line
*/

uint32_t
ic_simulate_fetch (oraddr_t fetchaddr, oraddr_t virt_addr)
{
  oraddr_t set;
  oraddr_t way;
  oraddr_t lru_way;
  oraddr_t tagaddr;
  uint32_t tmp;
  oraddr_t reload_addr;
  oraddr_t reload_end;
  unsigned int minlru;
  struct ic *ic = ic_state;

  /* ICache simulation enabled/disabled. */
  if (!(cpu_state.sprs[SPR_UPR] & SPR_UPR_ICP) ||
      !(cpu_state.sprs[SPR_SR] & SPR_SR_ICE) || insn_ci)
    {
      tmp = evalsim_mem32 (fetchaddr, virt_addr);
      if (cur_area && cur_area->log)
	fprintf (cur_area->log, "[%" PRIxADDR "] -> read %08" PRIx32 "\n",
		 fetchaddr, tmp);
      return tmp;
    }

  /* Which set to check out? */
  set = (fetchaddr & ic->set_mask) >> ic->blocksize_log2;
  tagaddr = fetchaddr & ic->tagaddr_mask;

  /* Scan all ways and try to find a matching way. */
  for (way = set; way < ic->last_way; way += ic->nsets)
    {
      if (ic->tags[way] == tagaddr)
	{
	  ic_stats.readhit++;

	  for (lru_way = set; lru_way < ic->last_way; lru_way += ic->nsets)
	    if (ic->lrus[lru_way] > ic->lrus[way])
	      ic->lrus[lru_way]--;
	  ic->lrus[way] = ic->ustates_reload;
	  runtime.sim.mem_cycles += ic->hitdelay;
	  way <<= ic->blocksize_log2;
	  return *(uint32_t *) & ic->
	    mem[way | (fetchaddr & ic->block_offset_mask)];
	}
    }

  minlru = ic->ustates_reload;
  way = set;

  ic_stats.readmiss++;

  for (lru_way = set; lru_way < ic->last_way; lru_way += ic->nsets)
    {
      if (ic->lrus[lru_way] < minlru)
	{
	  way = lru_way;
	  minlru = ic->lrus[lru_way];
	}
    }

  ic->tags[way] = tagaddr;
  for (lru_way = set; lru_way < ic->last_way; lru_way += ic->nsets)
    if (ic->lrus[lru_way])
      ic->lrus[lru_way]--;
  ic->lrus[way] = ic->ustates_reload;

  reload_addr = fetchaddr & ic->block_offset_mask;
  reload_end = reload_addr + ic->blocksize;

  fetchaddr &= ic->block_mask;

  way <<= ic->blocksize_log2;
  for (; reload_addr < reload_end; reload_addr += 4)
    {
      tmp =
	*(uint32_t *) & ic->mem[way | (reload_addr & ic->block_offset_mask)] =
	/* FIXME: What is the virtual address meant to be? (ie. What happens if
	 * we read out of memory while refilling a cache line?) */
	evalsim_mem32 (fetchaddr | (reload_addr & ic->block_offset_mask), 0);
      if (!cur_area)
	{
	  ic->tags[way >> ic->blocksize_log2] = -1;
	  ic->lrus[way >> ic->blocksize_log2] = 0;
	  return 0;
	}
      else if (cur_area->log)
	fprintf (cur_area->log, "[%" PRIxADDR "] -> read %08" PRIx32 "\n",
		 fetchaddr, tmp);
    }

  runtime.sim.mem_cycles += ic->missdelay;

  if (config.pcu.enabled)
    pcu_count_event(SPR_PCMR_ICM);


  return *(uint32_t *) & ic->mem[way | (reload_addr & ic->block_offset_mask)];
}

/* First check if data is already in the cache and if it is:
    - invalidate block if way isn't locked
   otherwise don't do anything.
*/

void
ic_inv (oraddr_t dataaddr)
{
  oraddr_t set;
  oraddr_t way;
  oraddr_t tagaddr;
  struct ic *ic = ic_state;

  if (!(cpu_state.sprs[SPR_UPR] & SPR_UPR_ICP))
    return;

  /* Which set to check out? */
  set = (dataaddr & ic->set_mask) >> ic->blocksize_log2;

  if (!(cpu_state.sprs[SPR_SR] & SPR_SR_ICE))
    {
      for (way = set; way < ic->last_way; way += ic->nsets)
	{
	  ic->tags[way] = -1;
	  ic->lrus[way] = 0;
	}
      return;
    }

  tagaddr = dataaddr & ic->tagaddr_mask;

  /* Scan all ways and try to find a matching way. */
  for (way = set; way < ic->last_way; way += ic->nsets)
    {
      if (ic->tags[way] == tagaddr)
	{
	  ic->tags[way] = -1;
	  ic->lrus[way] = 0;
	}
    }
}

/*-----------------------------------------------------[ IC configuration ]---*/


/*---------------------------------------------------------------------------*/
/*!Enable or disable the instruction cache

   Set the corresponding fields in the UPR

   @param[in] val  The value to use
   @param[in] dat  The config data structure                                 */
/*---------------------------------------------------------------------------*/
static void
ic_enabled (union param_val  val,
	    void            *dat)
{
  struct ic *ic = dat;

  ic->enabled = val.int_val;

  if (val.int_val)
    {
      cpu_state.sprs[SPR_UPR] |= SPR_UPR_ICP;
    }
  else
    {
      cpu_state.sprs[SPR_UPR] &= ~SPR_UPR_ICP;
    }
}	/* ic_enabled() */


/*---------------------------------------------------------------------------*/
/*!Set the number of instruction cache sets

   Set the corresponding field in the UPR

   @param[in] val  The value to use
   @param[in] dat  The config data structure                                 */
/*---------------------------------------------------------------------------*/
static void
ic_nsets (union param_val  val,
	  void            *dat)
{
  struct ic *ic = dat;

  if (is_power2 (val.int_val) && (val.int_val <= MAX_IC_SETS))
    {
      int  set_bits = log2_int (val.int_val);

      ic->nsets = val.int_val;

      cpu_state.sprs[SPR_ICCFGR] &= ~SPR_ICCFGR_NCS;
      cpu_state.sprs[SPR_ICCFGR] |= set_bits << SPR_ICCFGR_NCS_OFF;
    }
  else
    {
      fprintf (stderr, "Warning: instruction cache nsets not a power of "
	       "2 <= %d: ignored\n", MAX_IC_SETS);
    }
}	/* ic_nsets() */


/*---------------------------------------------------------------------------*/
/*!Set the number of instruction cache ways

   Set the corresponding field in the UPR

   @param[in] val  The value to use
   @param[in] dat  The config data structure                                 */
/*---------------------------------------------------------------------------*/
static void
ic_nways (union param_val  val,
	    void            *dat)
{
  struct ic *ic = dat;

  if (is_power2 (val.int_val) && (val.int_val <= MAX_IC_WAYS))
    {
      int  way_bits = log2_int (val.int_val);

      ic->nways = val.int_val;

      cpu_state.sprs[SPR_ICCFGR] &= ~SPR_ICCFGR_NCW;
      cpu_state.sprs[SPR_ICCFGR] |= way_bits << SPR_ICCFGR_NCW_OFF;
    }
  else
    {
      fprintf (stderr, "Warning: instruction cache nways not a power of "
	       "2 <= %d: ignored\n", MAX_IC_WAYS);
    }
}	/* ic_nways() */


/*---------------------------------------------------------------------------*/
/*!Set the instruction cache block size

   Value must be either MIN_IC_BLOCK_SIZE or MAX_IC_BLOCK_SIZE. If not issue a
   warning and ignore. Set the relevant field in the data cache config register

   @param[in] val  The value to use
   @param[in] dat  The config data structure                                 */
/*---------------------------------------------------------------------------*/
static void
ic_blocksize (union param_val  val,
	      void            *dat)
{
  struct ic *ic = dat;

  switch (val.int_val)
    {
    case MIN_IC_BLOCK_SIZE:
      ic->blocksize               = val.int_val;
      cpu_state.sprs[SPR_ICCFGR] &= ~SPR_ICCFGR_CBS;
      break;

    case MAX_IC_BLOCK_SIZE:
      ic->blocksize               = val.int_val;
      cpu_state.sprs[SPR_ICCFGR] |= SPR_ICCFGR_CBS;
      break;

    default:
      fprintf (stderr, "Warning: instruction cache block size not %d or %d: "
	       "ignored\n", MIN_IC_BLOCK_SIZE, MAX_IC_BLOCK_SIZE);
      break;
    }
}	/* ic_blocksize() */


/*---------------------------------------------------------------------------*/
/*!Set the number of instruction cache usage states

   Value must be 2, 3 or 4. If not issue a warning and ignore.

   @param[in] val  The value to use
   @param[in] dat  The config data structure                                 */
/*---------------------------------------------------------------------------*/
static void
ic_ustates (union param_val  val,
	    void            *dat)
{
  struct ic *ic = dat;

  if ((val.int_val >= 2) && (val.int_val <= 4))
    {
      ic->ustates = val.int_val;
    }
  else
    {
      fprintf (stderr, "Warning number of instruction cache usage states "
	       "must be 2, 3 or 4: ignored\n");
    }
}	/* ic_ustates() */


static void
ic_hitdelay (union param_val  val,
	     void            *dat)
{
  struct ic *ic = dat;
  ic->hitdelay = val.int_val;
}


static void
ic_missdelay (union param_val  val,
	      void            *dat)
{
  struct ic *ic = dat;
  ic->missdelay = val.int_val;
}


/*---------------------------------------------------------------------------*/
/*!Initialize a new instruction cache configuration

   ALL parameters are set explicitly to default values. Corresponding SPR
   flags are set as appropriate.

   @return  The new memory configuration data structure                      */
/*---------------------------------------------------------------------------*/
static void *
ic_start_sec ()
{
  struct ic *ic;
  int        set_bits;
  int        way_bits;

  if (NULL == (ic = malloc (sizeof (struct ic))))
    {
      fprintf (stderr, "OOM\n");
      exit (1);
    }

  ic->enabled   = 0;
  ic->nsets     = 1;
  ic->nways     = 1;
  ic->blocksize = MIN_IC_BLOCK_SIZE;
  ic->ustates   = 2;
  ic->hitdelay  = 1;
  ic->missdelay = 1;

  ic->mem       = NULL;		/* Internal configuration */
  ic->lrus      = NULL;
  ic->tags      = NULL;

  /* Set SPRs as appropriate */

  if (ic->enabled)
    {
      cpu_state.sprs[SPR_UPR] |= SPR_UPR_ICP;
    }
  else
    {
      cpu_state.sprs[SPR_UPR] &= ~SPR_UPR_ICP;
    }

  set_bits = log2_int (ic->nsets);
  cpu_state.sprs[SPR_ICCFGR] &= ~SPR_ICCFGR_NCS;
  cpu_state.sprs[SPR_ICCFGR] |= set_bits << SPR_ICCFGR_NCS_OFF;

  way_bits = log2_int (ic->nways);
  cpu_state.sprs[SPR_ICCFGR] &= ~SPR_ICCFGR_NCW;
  cpu_state.sprs[SPR_ICCFGR] |= way_bits << SPR_ICCFGR_NCW_OFF;

  if (MIN_IC_BLOCK_SIZE == ic->blocksize)
    {
      cpu_state.sprs[SPR_ICCFGR] &= ~SPR_ICCFGR_CBS;
    }
  else
    {
      cpu_state.sprs[SPR_ICCFGR] |= SPR_ICCFGR_CBS;
    }

  ic_state = ic;
  return ic;

}	/* ic_start_sec() */


static void
ic_end_sec (void *dat)
{
  struct ic *ic = dat;
  unsigned int size = ic->nways * ic->nsets * ic->blocksize;

  if (size)
    {
      if (!(ic->mem = malloc (size)))
	{
	  fprintf (stderr, "OOM\n");
	  exit (1);
	}
      if (!
	  (ic->lrus = malloc (ic->nsets * ic->nways * sizeof (unsigned int))))
	{
	  fprintf (stderr, "OOM\n");
	  exit (1);
	}
      if (!(ic->tags = malloc (ic->nsets * ic->nways * sizeof (oraddr_t))))
	{
	  fprintf (stderr, "OOM\n");
	  exit (1);
	}

      /* Clear the cache data. John Alfredo's fix for using 0 (which is a
	 valid tag), so we now use -1 */
      memset (ic->mem,  0, size);
      memset (ic->lrus, 0, ic->nsets * ic->nways * sizeof (unsigned int));
      memset (ic->tags, -1, ic->nsets * ic->nways * sizeof (oraddr_t));
    }
  else
    {
      ic->enabled = 0;
    }

  ic->blocksize_log2    = log2_int (ic->blocksize);
  ic->set_mask          = (ic->nsets - 1) << ic->blocksize_log2;
  ic->tagaddr_mask      = ~((ic->nsets * ic->blocksize) - 1);
  ic->last_way          = ic->nsets * ic->nways;
  ic->block_offset_mask = ic->blocksize - 1;
  ic->block_mask        = ~ic->block_offset_mask;
  ic->ustates_reload    = ic->ustates - 1;

  if (ic->enabled)
    reg_sim_stat (ic_info, dat);
}

void
reg_ic_sec (void)
{
  struct config_section *sec =
    reg_config_sec ("ic", ic_start_sec, ic_end_sec);

  reg_config_param (sec, "enabled",   PARAMT_INT, ic_enabled);
  reg_config_param (sec, "nsets",     PARAMT_INT, ic_nsets);
  reg_config_param (sec, "nways",     PARAMT_INT, ic_nways);
  reg_config_param (sec, "blocksize", PARAMT_INT, ic_blocksize);
  reg_config_param (sec, "ustates",   PARAMT_INT, ic_ustates);
  reg_config_param (sec, "missdelay", PARAMT_INT, ic_missdelay);
  reg_config_param (sec, "hitdelay" , PARAMT_INT, ic_hitdelay);
}
