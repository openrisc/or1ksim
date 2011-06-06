/* immu.c -- Instruction MMU simulation

   Copyright (C) 1999 Damjan Lampret, lampret@opencores.org
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

/* System includes */
#include <stdlib.h>

/* Package includes */
#include "immu.h"
#include "sim-config.h"
#include "execute.h"
#include "stats.h"
#include "except.h"
#include "spr-dump.h"
#include "misc.h"
#include "sim-cmd.h"
#include "pcu.h"


struct immu *immu_state;

/* Insn MMU */

static uorreg_t *
immu_find_tlbmr (oraddr_t virtaddr, uorreg_t ** itlbmr_lru, struct immu *immu)
{
  int set;
  int i;
  oraddr_t vpn;
  uorreg_t *itlbmr;

  /* Which set to check out? */
  set = IADDR_PAGE (virtaddr) >> immu->pagesize_log2;
  set &= immu->set_mask;
  vpn = virtaddr & immu->vpn_mask;

  itlbmr = &cpu_state.sprs[SPR_ITLBMR_BASE (0) + set];
  *itlbmr_lru = itlbmr;

  /* Scan all ways and try to find a matching way. */
  /* FIXME: Should this be reversed? */
  for (i = immu->nways; i; i--, itlbmr += (128 * 2))
    {
      if (((*itlbmr & immu->vpn_mask) == vpn) && (*itlbmr & SPR_ITLBMR_V))
	return itlbmr;
    }

  return NULL;
}

oraddr_t
immu_translate (oraddr_t virtaddr)
{
  int i;
  uorreg_t *itlbmr;
  uorreg_t *itlbtr;
  uorreg_t *itlbmr_lru;
  struct immu *immu = immu_state;

  if (!(cpu_state.sprs[SPR_SR] & SPR_SR_IME) ||
      !(cpu_state.sprs[SPR_UPR] & SPR_UPR_IMP))
    {
      insn_ci = (virtaddr >= 0x80000000);
      return virtaddr;
    }

  itlbmr = immu_find_tlbmr (virtaddr, &itlbmr_lru, immu);

  /* Did we find our tlb entry? */
  if (itlbmr)
    {				/* Yes, we did. */
      immu_stats.fetch_tlbhit++;
      itlbtr = itlbmr + 128;

      /* Set LRUs */
      for (i = 0; i < immu->nways; i++, itlbmr_lru += (128 * 2))
	{
	  if (*itlbmr_lru & SPR_ITLBMR_LRU)
	    *itlbmr_lru = (*itlbmr_lru & ~SPR_ITLBMR_LRU) |
	      ((*itlbmr_lru & SPR_ITLBMR_LRU) - 0x40);
	}

      /* This is not necessary `*itlbmr &= ~SPR_ITLBMR_LRU;' since SPR_DTLBMR_LRU
       * is always decremented and the number of sets is always a power of two and
       * as such lru_reload has all bits set that get touched during decrementing
       * SPR_DTLBMR_LRU */
      *itlbmr |= immu->lru_reload;

      /* Check if page is cache inhibited */
      insn_ci = *itlbtr & SPR_ITLBTR_CI;

      runtime.sim.mem_cycles += immu->hitdelay;

      /* Test for page fault */
      if (cpu_state.sprs[SPR_SR] & SPR_SR_SM)
	{
	  if (!(*itlbtr & SPR_ITLBTR_SXE))
	    except_handle (EXCEPT_IPF, virtaddr);
	}
      else
	{
	  if (!(*itlbtr & SPR_ITLBTR_UXE))
	    except_handle (EXCEPT_IPF, virtaddr);
	}

      return (*itlbtr & SPR_ITLBTR_PPN) | (virtaddr & immu->page_offset_mask);
    }

  /* No, we didn't. */
  immu_stats.fetch_tlbmiss++;

  /* if tlb refill implemented in HW */
  /* return ((cpu_state.sprs[SPR_ITLBTR_BASE(minway) + set] & SPR_ITLBTR_PPN) >> 12) * immu->pagesize + (virtaddr % immu->pagesize); */
  runtime.sim.mem_cycles += immu->missdelay;

  except_handle (EXCEPT_ITLBMISS, virtaddr);

  if (config.pcu.enabled)
    pcu_count_event(SPR_PCMR_ITLBM);

  return 0;
}

/* DESC: try to find EA -> PA transaltion without changing 
 *       any of precessor states. if this is not passible gives up 
 *       (without triggering exceptions).
 *
 * PRMS: virtaddr  - EA for which to find translation 
 *
 * RTRN: 0         - no IMMU, IMMU disabled or ITLB miss
 *       else      - appropriate PA (note it IMMU is not present 
 *                   PA === EA)
 */
oraddr_t
peek_into_itlb (oraddr_t virtaddr)
{
  uorreg_t *itlbmr;
  uorreg_t *itlbtr;
  uorreg_t *itlbmr_lru;
  struct immu *immu = immu_state;

  if (!(cpu_state.sprs[SPR_SR] & SPR_SR_IME) ||
      !(cpu_state.sprs[SPR_UPR] & SPR_UPR_IMP))
    {
      return (virtaddr);
    }

  itlbmr = immu_find_tlbmr (virtaddr, &itlbmr_lru, immu);

  /* Did we find our tlb entry? */
  if (itlbmr)
    {				/* Yes, we did. */
      itlbtr = itlbmr + 128;

      /* Test for page fault */
      if (cpu_state.sprs[SPR_SR] & SPR_SR_SM)
	{
	  if (!(*itlbtr & SPR_ITLBTR_SXE))
	    {
	      /* no luck, giving up */
	      return (0);
	    }
	}
      else
	{
	  if (!(*itlbtr & SPR_ITLBTR_UXE))
	    {
	      /* no luck, giving up */
	      return (0);
	    }
	}

      return (*itlbtr & SPR_ITLBTR_PPN) | (virtaddr & immu->page_offset_mask);
    }

  return (0);
}


/* FIXME: Check validity */
/* First check if virtual address is covered by ITLB and if it is:
    - increment ITLB read hit stats,
    - set 'lru' at this way to immu->ustates - 1 and
      decrement 'lru' of other ways unless they have reached 0,
    - check page access attributes and invoke IMMU page fault exception
      handler if necessary
   and if not:
    - increment ITLB read miss stats
    - find lru way and entry and invoke ITLB miss exception handler
    - set 'lru' with immu->ustates - 1 and decrement 'lru' of other
      ways unless they have reached 0
*/

static void
itlb_status (void *dat)
{
  struct immu *immu = dat;
  int set;
  int way;
  int end_set = immu->nsets;

  if (!(cpu_state.sprs[SPR_UPR] & SPR_UPR_IMP))
    {
      PRINTF ("IMMU not implemented. Set UPR[IMP].\n");
      return;
    }

  if (0 < end_set)
    PRINTF ("\nIMMU: ");
  /* Scan set(s) and way(s). */
  for (set = 0; set < end_set; set++)
    {
      for (way = 0; way < immu->nways; way++)
	{
	  PRINTF ("%s\n", dump_spr (SPR_ITLBMR_BASE (way) + set,
				    cpu_state.sprs[SPR_ITLBMR_BASE (way) +
						   set]));
	  PRINTF ("%s\n",
		  dump_spr (SPR_ITLBTR_BASE (way) + set,
			    cpu_state.sprs[SPR_ITLBTR_BASE (way) + set]));
	}
    }
  if (0 < end_set)
    PRINTF ("\n");
}

/*---------------------------------------------------[ IMMU configuration ]---*/

/*---------------------------------------------------------------------------*/
/*!Enable or disable the IMMU

   Set the corresponding field in the UPR

   @param[in] val  The value to use
   @param[in] dat  The config data structure                                 */
/*---------------------------------------------------------------------------*/
static void
immu_enabled (union param_val val, void *dat)
{
  struct immu *immu = dat;

  if (val.int_val)
    {
      cpu_state.sprs[SPR_UPR] |= SPR_UPR_IMP;
    }
  else
    {
      cpu_state.sprs[SPR_UPR] &= ~SPR_UPR_IMP;
    }

  immu->enabled = val.int_val;
}


/*---------------------------------------------------------------------------*/
/*!Set the number of DMMU sets

   Value must be a power of 2 <= 256. Ignore any other values with a
   warning. Set the corresponding IMMU configuration flags.

   @param[in] val  The value to use
   @param[in] dat  The config data structure                                 */
/*---------------------------------------------------------------------------*/
static void
immu_nsets (union param_val  val,
	    void            *dat)
{
  struct immu *immu = dat;

  if (is_power2 (val.int_val) && (val.int_val <= 128))
    {
      int  set_bits = log2_int (val.int_val);

      immu->nsets = val.int_val;

      cpu_state.sprs[SPR_IMMUCFGR] &= ~SPR_IMMUCFGR_NTS;
      cpu_state.sprs[SPR_IMMUCFGR] |= set_bits << SPR_IMMUCFGR_NTS_OFF;
    }
  else
    {
      fprintf (stderr, "Warning IMMU nsets not a power of 2 <= 128: ignored\n");
    }
}	/* immu_nsets() */


/*---------------------------------------------------------------------------*/
/*!Set the number of IMMU ways

   Value must be in the range 1-4. Ignore other values with a warning.  Set
   the corresponding IMMU configuration flags.

   @param[in] val  The value to use
   @param[in] dat  The config data structure                                 */
/*---------------------------------------------------------------------------*/
static void
immu_nways (union param_val  val,
	    void            *dat)
{
  struct immu *immu = dat;

  if (val.int_val >= 1 && val.int_val <= 4)
    {
      int  way_bits = val.int_val - 1;

      immu->nways = val.int_val;

      cpu_state.sprs[SPR_IMMUCFGR] &= ~SPR_IMMUCFGR_NTW;
      cpu_state.sprs[SPR_IMMUCFGR] |= way_bits << SPR_IMMUCFGR_NTW_OFF;
    }
  else
    {
      fprintf (stderr, "Warning IMMU nways not in range 1-4: ignored\n");
    }
}	/* immu_nways() */


/*---------------------------------------------------------------------------*/
/*!Set the IMMU page size

   Value must be a power of 2. Ignore other values with a warning

   @param[in] val  The value to use
   @param[in] dat  The config data structure                                 */
/*---------------------------------------------------------------------------*/
static void
immu_pagesize (union param_val  val,
	       void            *dat)
{
  struct immu *immu = dat;

  if (is_power2 (val.int_val))
    {
      immu->pagesize = val.int_val;
    }
  else
    {
      fprintf (stderr, "Warning IMMU page size must be power of 2: ignored\n");
    }
}	/* immu_pagesize() */


/*---------------------------------------------------------------------------*/
/*!Set the IMMU entry size

   Value must be a power of 2. Ignore other values with a warning

   @param[in] val  The value to use
   @param[in] dat  The config data structure                                 */
/*---------------------------------------------------------------------------*/
static void
immu_entrysize (union param_val  val,
		void            *dat)
{
  struct immu *immu = dat;

  if (is_power2 (val.int_val))
    {
      immu->entrysize = val.int_val;
    }
  else
    {
      fprintf (stderr, "Warning IMMU entry size must be power of 2: ignored\n");
    }
}	/* immu_entrysize() */


/*---------------------------------------------------------------------------*/
/*!Set the number of IMMU usage states

   Value must be 2, 3 or 4. Ignore other values with a warning

   @param[in] val  The value to use
   @param[in] dat  The config data structure                                 */
/*---------------------------------------------------------------------------*/
static void
immu_ustates (union param_val  val,
	      void            *dat)
{
  struct immu *immu = dat;

  if ((val.int_val >= 2) && (val.int_val <= 4))
    {
      immu->ustates = val.int_val;
    }
  else
    {
      fprintf (stderr, "Warning number of IMMU usage states must be 2, 3 or 4:"
	       "ignored\n");
    }
}	/* immu_ustates() */


static void
immu_missdelay (union param_val val, void *dat)
{
  struct immu *immu = dat;

  immu->missdelay = val.int_val;
}

static void
immu_hitdelay (union param_val val, void *dat)
{
  struct immu *immu = dat;

  immu->hitdelay = val.int_val;
}

/*---------------------------------------------------------------------------*/
/*!Initialize a new DMMU configuration

   ALL parameters are set explicitly to default values.                      */
/*---------------------------------------------------------------------------*/
static void *
immu_start_sec ()
{
  struct immu *immu;
  int          set_bits;
  int          way_bits;

  if (NULL == (immu = malloc (sizeof (struct immu))))
    {
      fprintf (stderr, "OOM\n");
      exit (1);
    }

  immu->enabled   = 0;
  immu->nsets     = 1;
  immu->nways     = 1;
  immu->pagesize  = 8192;
  immu->entrysize = 1;		/* Not currently used */
  immu->ustates   = 2;
  immu->hitdelay  = 1;
  immu->missdelay = 1;

  if (immu->enabled)
    {
      cpu_state.sprs[SPR_UPR] |= SPR_UPR_IMP;
    }
  else
    {
      cpu_state.sprs[SPR_UPR] &= ~SPR_UPR_IMP;
    }

  set_bits = log2_int (immu->nsets);
  cpu_state.sprs[SPR_IMMUCFGR] &= ~SPR_IMMUCFGR_NTS;
  cpu_state.sprs[SPR_IMMUCFGR] |= set_bits << SPR_IMMUCFGR_NTS_OFF;

  way_bits = immu->nways - 1;
  cpu_state.sprs[SPR_IMMUCFGR] &= ~SPR_IMMUCFGR_NTW;
  cpu_state.sprs[SPR_IMMUCFGR] |= way_bits << SPR_IMMUCFGR_NTW_OFF;

  immu_state = immu;
  return immu;

}	/* immu_start_sec() */


static void
immu_end_sec (void *dat)
{
  struct immu *immu = dat;

  /* Precalculate some values for use during address translation */
  immu->pagesize_log2 = log2_int (immu->pagesize);
  immu->page_offset_mask = immu->pagesize - 1;
  immu->page_mask = ~immu->page_offset_mask;
  immu->vpn_mask = ~((immu->pagesize * immu->nsets) - 1);
  immu->set_mask = immu->nsets - 1;
  immu->lru_reload = (immu->set_mask << 6) & SPR_ITLBMR_LRU;

  if (immu->enabled)
    {
      PRINTF ("Insn MMU %dKB: %d ways, %d sets, entry size %d bytes\n",
	      immu->nsets * immu->entrysize * immu->nways / 1024, immu->nways,
	      immu->nsets, immu->entrysize);
      reg_sim_stat (itlb_status, immu);
    }
}

void
reg_immu_sec (void)
{
  struct config_section *sec = reg_config_sec ("immu", immu_start_sec,
					       immu_end_sec);

  reg_config_param (sec, "enabled",   PARAMT_INT, immu_enabled);
  reg_config_param (sec, "nsets",     PARAMT_INT, immu_nsets);
  reg_config_param (sec, "nways",     PARAMT_INT, immu_nways);
  reg_config_param (sec, "pagesize",  PARAMT_INT, immu_pagesize);
  reg_config_param (sec, "entrysize", PARAMT_INT, immu_entrysize);
  reg_config_param (sec, "ustates",   PARAMT_INT, immu_ustates);
  reg_config_param (sec, "missdelay", PARAMT_INT, immu_missdelay);
  reg_config_param (sec, "hitdelay",  PARAMT_INT, immu_hitdelay);
}
