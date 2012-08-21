/* stats.c -- Various statistics about instruction scheduling etc.

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
#include "port.h"

/* Package includes */
#include "stats.h"
#include "sim-config.h"
#include "icache-model.h"
#include "spr-defs.h"
#include "execute.h"


#define DSTATS_LEN	3000
#define SSTATS_LEN	300
#define FSTATS_LEN	200

/* Used by safe division - increment divisor by one if it is zero */
#define SD(X) (X != 0 ? X : 1)

struct branchstat
{
  int taken;
  int nottaken;
  int forward;
  int backward;
};

/*! @see also enum insn_type in abstract.h */
static const char func_unit_str[30][30] = {
  "unknown",
  "exception",
  "arith",
  "shift",
  "compare",
  "branch",
  "jump",
  "load",
  "store",
  "movimm",
  "move",
  "extend",
  "nop",
  "mac"
};

struct dstats_entry
{
  int insn1;
  int insn2;
  int cnt_dynamic;
  int depend;
};				/*!< double stats */

struct sstats_entry
{
  int insn;
  int cnt_dynamic;
};				/*!< single stats */

struct fstats_entry
{
  enum insn_type insn1;
  enum insn_type insn2;
  int cnt_dynamic;
  int depend;
};				/*!< functional units stats */

/* Globally visible statistics data. Renamed mstats to or1k_mstats because Mac
   OS X has a lib function called mstats */
struct mstats_entry      or1k_mstats = { 0 };	/*!< misc units stats */
struct cachestats_entry  ic_stats    = { 0 };	/*!< instruction cache stats */
struct cachestats_entry  dc_stats    = { 0 };	/*!< data cache stats */
struct immustats_entry   immu_stats  = { 0 };	/*!< insn MMU stats */
struct dmmustats_entry   dmmu_stats  = { 0 };	/*!< data MMU stats */
struct raw_stats         raw_stats;		/*!< RAW hazard stats */

/* Statistics data strutures used just here */
static struct dstats_entry  dstats[DSTATS_LEN];	/*!< dependency stats */
static struct sstats_entry  sstats[SSTATS_LEN];	/*!< single stats */
static struct fstats_entry  fstats[FSTATS_LEN];	/*!< func units stats */


void
addsstats (int item, int cnt_dynamic)
{
  int i = 0;

  while (sstats[i].insn != item && sstats[i].insn >= 0 && i < SSTATS_LEN)
    i++;

  if (i >= SSTATS_LEN - 1)
    return;

  if (sstats[i].insn >= 0)
    {
      sstats[i].cnt_dynamic += cnt_dynamic;
    }
  else
    {
      sstats[i].insn = item;
      sstats[i].cnt_dynamic = cnt_dynamic;
    }
}

void
adddstats (int item1, int item2, int cnt_dynamic, int depend)
{
  int i = 0;

  while ((dstats[i].insn1 != item1 || dstats[i].insn2 != item2)
	 && (i < DSTATS_LEN) && dstats[i].insn1 >= 0)
    i++;

  if (i >= DSTATS_LEN - 1)
    return;

  if (dstats[i].insn1 >= 0)
    {
      dstats[i].cnt_dynamic += cnt_dynamic;
      dstats[i].depend += depend;
    }
  else
    {
      dstats[i].insn1 = item1;
      dstats[i].insn2 = item2;
      dstats[i].cnt_dynamic = cnt_dynamic;
      dstats[i].depend = depend;
    }
}

void
addfstats (enum insn_type item1, enum insn_type item2, int cnt_dynamic,
	   int depend)
{
  int i = 0;

  while (((fstats[i].insn1 != item1) || (fstats[i].insn2 != item2)) &&
	 (fstats[i].insn1 != it_unknown) && (i < FSTATS_LEN))
    i++;

  if (i >= FSTATS_LEN - 1)
    return;

  if ((fstats[i].insn1 == item1) && (fstats[i].insn2 == item2))
    {
      fstats[i].cnt_dynamic += cnt_dynamic;
      fstats[i].depend += depend;
    }
  else
    {
      fstats[i].insn1 = item1;
      fstats[i].insn2 = item2;
      fstats[i].cnt_dynamic = cnt_dynamic;
      fstats[i].depend = depend;
    }
}

void
initstats ()
{
  int i;
  memset (sstats, 0, sizeof (sstats));
  for (i = 0; i < SSTATS_LEN; i++)
    sstats[i].insn = -1;
  memset (dstats, 0, sizeof (dstats));
  for (i = 0; i < DSTATS_LEN; i++)
    dstats[i].insn1 = dstats[i].insn2 = -1;
  memset (fstats, 0, sizeof (fstats));
  memset (&or1k_mstats, 0, sizeof (or1k_mstats));
  memset (&ic_stats, 0, sizeof (ic_stats));
  memset (&dc_stats, 0, sizeof (dc_stats));
  memset (&raw_stats, 0, sizeof (raw_stats));
}

static void
printotherstats (int which)
{
  PRINTF ("\n");
  if (config.bpb.enabled)
    {
      struct branchstat bf;
      struct branchstat bnf;
      long bf_all, bnf_all;
      bf.taken = or1k_mstats.bf[1][0] + or1k_mstats.bf[1][1];
      bf.nottaken = or1k_mstats.bf[0][0] + or1k_mstats.bf[0][1];
      bf.forward = or1k_mstats.bf[0][1] + or1k_mstats.bf[1][1];
      bf.backward = or1k_mstats.bf[0][0] + or1k_mstats.bf[1][0];
      bf_all = bf.forward + bf.backward;

      bnf.taken = or1k_mstats.bnf[1][0] + or1k_mstats.bf[1][1];
      bnf.nottaken = or1k_mstats.bnf[0][0] + or1k_mstats.bf[0][1];
      bnf.forward = or1k_mstats.bnf[0][1] + or1k_mstats.bf[1][1];
      bnf.backward = or1k_mstats.bnf[0][0] + or1k_mstats.bf[1][0];
      bnf_all = bnf.forward + bnf.backward;

      PRINTF ("bnf: %d (%ld%%) taken,", bf.taken,
	      (bf.taken * 100) / SD (bf_all));
      PRINTF (" %d (%ld%%) not taken,", bf.nottaken,
	      (bf.nottaken * 100) / SD (bf_all));
      PRINTF (" %d (%ld%%) forward,", bf.forward,
	      (bf.forward * 100) / SD (bf_all));
      PRINTF (" %d (%ld%%) backward\n", bf.backward,
	      (bf.backward * 100) / SD (bf_all));
      PRINTF ("bf: %d (%ld%%) taken,", bnf.taken,
	      (bnf.taken * 100) / SD (bnf_all));
      PRINTF (" %d (%ld%%) not taken,", bnf.nottaken,
	      (bnf.nottaken * 100) / SD (bnf_all));
      PRINTF (" %d (%ld%%) forward,", bnf.forward,
	      (bnf.forward * 100) / SD (bnf_all));
      PRINTF (" %d (%ld%%) backward\n", bnf.backward,
	      (bnf.backward * 100) / SD (bnf_all));

      PRINTF ("StaticBP bnf(%s): correct %ld%%\n",
	      config.bpb.sbp_bnf_fwd ? "forward" : "backward",
	      (or1k_mstats.bnf[0][config.bpb.sbp_bnf_fwd] * 100) /
	      SD (bnf_all));
      PRINTF ("StaticBP bf(%s): correct %ld%%\n",
	      config.bpb.sbp_bf_fwd ? "forward" : "backward",
	      (or1k_mstats.bnf[1][config.bpb.sbp_bf_fwd] * 100) /
	      SD (bf_all));
      PRINTF ("BPB: hit %d (correct %d%%), miss %d\n", or1k_mstats.bpb.hit,
	      (or1k_mstats.bpb.correct * 100) / SD (or1k_mstats.bpb.hit),
	      or1k_mstats.bpb.miss);
    }
  else
    PRINTF ("BPB simulation disabled. Enable it to see BPB analysis\n");

  if (config.bpb.btic)
    {
      PRINTF ("BTIC: hit %d(%d%%), miss %d\n", or1k_mstats.btic.hit,
	      (or1k_mstats.btic.hit * 100) / SD (or1k_mstats.btic.hit +
						 or1k_mstats.btic.miss),
	      or1k_mstats.btic.miss);
    }
  else
    PRINTF ("BTIC simulation disabled. Enabled it to see BTIC analysis\n");

  if ((NULL != ic_state) && ic_state->enabled)
    {
      PRINTF ("IC read:  hit %d(%d%%), miss %d\n", ic_stats.readhit,
	      (ic_stats.readhit * 100) / SD (ic_stats.readhit +
					     ic_stats.readmiss),
	      ic_stats.readmiss);
    }
  else
    PRINTF ("No ICache. Enable it to see IC results.\n");

  if (config.dc.enabled)
    {
      PRINTF ("DC read:  hit %d(%d%%), miss %d\n", dc_stats.readhit,
	      (dc_stats.readhit * 100) / SD (dc_stats.readhit +
					     dc_stats.readmiss),
	      dc_stats.readmiss);
      PRINTF ("DC write: hit %d(%d%%), miss %d\n", dc_stats.writehit,
	      (dc_stats.writehit * 100) / SD (dc_stats.writehit +
					      dc_stats.writemiss),
	      dc_stats.writemiss);
    }
  else
    PRINTF ("No DCache. Enable it to see DC results.\n");

  if (cpu_state.sprs[SPR_UPR] & SPR_UPR_IMP)
    {
      PRINTF ("IMMU read:  hit %d(%d%%), miss %d\n", immu_stats.fetch_tlbhit,
	      (immu_stats.fetch_tlbhit * 100) / SD (immu_stats.fetch_tlbhit +
						    immu_stats.fetch_tlbmiss),
	      immu_stats.fetch_tlbmiss);
    }
  else
    PRINTF ("No IMMU. Set UPR[IMP]\n");

  if (cpu_state.sprs[SPR_UPR] & SPR_UPR_DMP)
    {
      PRINTF ("DMMU read:  hit %d(%d%%), miss %d\n", dmmu_stats.loads_tlbhit,
	      (dmmu_stats.loads_tlbhit * 100) / SD (dmmu_stats.loads_tlbhit +
						    dmmu_stats.loads_tlbmiss),
	      dmmu_stats.loads_tlbmiss);
    }
  else
    PRINTF ("No DMMU. Set UPR[DMP]\n");

  PRINTF ("Additional LOAD CYCLES: %u  STORE CYCLES: %u\n",
	  runtime.sim.loadcycles, runtime.sim.storecycles);
}

void
printstats (int which)
{
  int i, all = 0, dependall = 0;

  if (which > 1 && which <= 5 && !config.cpu.dependstats)
    {
      PRINTF
	("Hazard analysis disabled. Enable it to see analysis results.\n");
      return;
    }

  switch (which)
    {
    case 1:
      PRINTF ("stats 1: Misc stats\n");
      printotherstats (which);
      break;
    case 2:
      PRINTF ("stats 2: Instruction usage\n");
      for (i = 0; i < SSTATS_LEN; i++)
	all += sstats[i].cnt_dynamic;

      for (i = 0; i < SSTATS_LEN; i++)
	if (sstats[i].cnt_dynamic)
	  PRINTF ("  %-15s used %6dx (%5.1f%%)\n", or1ksim_insn_name (sstats[i].insn),
		  sstats[i].cnt_dynamic,
		  (sstats[i].cnt_dynamic * 100.) / SD (all));

      PRINTF ("%d instructions (dynamic, single stats)\n", all);
      break;

    case 3:
      PRINTF ("stats 3: Instruction dependencies\n");
      for (i = 0; i < DSTATS_LEN; i++)
	{
	  all += dstats[i].cnt_dynamic;
	  dependall += dstats[i].depend;
	}

      for (i = 0; i < DSTATS_LEN; i++)
	if (dstats[i].cnt_dynamic)
	  {
	    char temp[100];
	    sprintf (temp, "%s, %s ", or1ksim_insn_name (dstats[i].insn1),
		     or1ksim_insn_name (dstats[i].insn2));
	    PRINTF ("  %-30s %6dx (%5.1f%%)", temp, dstats[i].cnt_dynamic,
		    (dstats[i].cnt_dynamic * 100.) / SD (all));
	    PRINTF ("   depend: %5.1f%%\n",
		    (dstats[i].depend * 100.) / dstats[i].cnt_dynamic);
	  }

      PRINTF ("%d instructions (dynamic, dependency stats)  depend: %d%%\n",
	      all, (dependall * 100) / SD (all));
      break;

    case 4:
      PRINTF ("stats 4: Functional units dependencies\n");
      for (i = 0; i < FSTATS_LEN; i++)
	{
	  all += fstats[i].cnt_dynamic;
	  dependall += fstats[i].depend;
	}

      for (i = 0; i < FSTATS_LEN; i++)
	if (fstats[i].cnt_dynamic)
	  {
	    char temp[100];
	    sprintf (temp, "%s, %s", func_unit_str[fstats[i].insn1],
		     func_unit_str[fstats[i].insn2]);
	    PRINTF ("  %-30s %6dx (%5.1f%%)", temp, fstats[i].cnt_dynamic,
		    (fstats[i].cnt_dynamic * 100.) / SD (all));
	    PRINTF ("   depend: %5.1f%%\n",
		    (fstats[i].depend * 100.) / fstats[i].cnt_dynamic);
	  }
      PRINTF
	("%d instructions (dynamic, functional units stats)  depend: %d%%\n\n",
	 all, (dependall * 100) / SD (all));
      break;

    case 5:
      PRINTF ("stats 5: Raw register usage over time\n");
#if RAW_RANGE_STATS
      for (i = 0; (i < RAW_RANGE); i++)
	PRINTF ("  Register set and reused in %d. cycle: %d cases\n", i,
		raw_stats.range[i]);
#endif
      break;
    case 6:
      if (config.cpu.sbuf_len)
	{
	  extern int sbuf_total_cyc, sbuf_wait_cyc;
	  PRINTF ("stats 6: Store buffer analysis\n");
	  PRINTF ("Using store buffer of length %i.\n", config.cpu.sbuf_len);
	  PRINTF ("Number of total memory store cycles: %i/%lli\n",
		  sbuf_total_cyc,
		  runtime.sim.cycles + sbuf_total_cyc - sbuf_wait_cyc);
	  PRINTF ("Number of cycles waiting for memory stores: %i\n",
		  sbuf_wait_cyc);
	  PRINTF ("Number of memory cycles spared: %i\n",
		  sbuf_total_cyc - sbuf_wait_cyc);
	  PRINTF ("Store speedup %3.2f%%, total speedup %3.2f%%\n",
		  100. * (sbuf_total_cyc - sbuf_wait_cyc) / sbuf_total_cyc,
		  100. * (sbuf_total_cyc -
			  sbuf_wait_cyc) / (runtime.sim.cycles +
					    sbuf_total_cyc - sbuf_wait_cyc));
	}
      else
	PRINTF
	  ("Store buffer analysis disabled. Enable it to see analysis results.\n");
      break;
    default:
      PRINTF ("Please specify a stats group (1-6).\n");
      break;
    }
}
