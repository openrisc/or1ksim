/* branch-predict.c -- branch prediction simulation

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

/* Branch prediction functions.  At the moment this functions only simulate
   functionality of branch prediction and do not influence on
   fetche/decode/execute stages.  They are here only to verify performance of
   various branch prediction configurations. */


/* Autoconf and/or portability configuration */
#include "config.h"
#include "port.h"

/* System includes */
#include <stdio.h>
#include <errno.h>
#include <stdarg.h>

/* Package includes */
#include "sim-config.h"
#include "arch.h"
#include "stats.h"

/* Branch prediction buffer */

/* Length of BPB */
#define BPB_LEN 64

/* Number of BPB ways (1, 2, 3 etc.). */
#define BPB_WAYS 1

/* Number of prediction states (2, 4, 8 etc.). */
#define BPB_PSTATES 2

/* Number of usage states (2, 3, 4 etc.). */
#define BPB_USTATES 2

/* branch prediction buffer entry */
struct bpb_entry
{
  struct
  {
    oraddr_t addr;		/* address of a branch insn */
    int taken;			/* taken == 1, not taken == 0  OR */
    /* strongly taken == 3, taken == 2, 
       not taken == 1, strongly not taken == 0 */
    int lru;			/* least recently == 0 */
  } way[BPB_WAYS];
} bpb[BPB_LEN];

void
bpb_info ()
{
  if (!config.bpb.enabled)
    {
      PRINTF ("BPB not simulated. Check -bpb option.\n");
      return;
    }

  PRINTF ("BPB %d bytes: ",
	  BPB_LEN * BPB_WAYS * (BPB_PSTATES + BPB_USTATES) / 8);
  PRINTF ("%d ways, %d sets, %d bits/prediction\n", BPB_WAYS, BPB_LEN,
	  BPB_PSTATES + BPB_USTATES);
}

/* First check if branch is already in the cache and if it is:
    - increment BPB hit stats,
    - set 'lru' at this way to BPB_USTATES - 1 and
      decrement 'lru' of other ways unless they have reached 0,
    - increment correct/incorrect stats according to BPB 'taken' field
      and 'taken' variable,
    - increment or decrement BPB taken field according to 'taken' variable
   and if not:
    - increment BPB miss stats
    - find lru way and entry and replace old address with 'addr' and
      'taken' field with (BPB_PSTATES/2 - 1) + 'taken'
    - set 'lru' with BPB_USTATES - 1 and decrement 'lru' of other
      ways unless they have reached 0
*/

void
bpb_update (oraddr_t addr, int taken)
{
  int entry, way = -1;
  int i;

  /* BPB simulation enabled/disabled. */
  if (!config.bpb.enabled)
    return;

  /* Calc entry. */
  entry = addr % BPB_LEN;

  /* Scan all ways and try to find our addr. */
  for (i = 0; i < BPB_WAYS; i++)
    if (bpb[entry].way[i].addr == addr)
      way = i;

  /* Did we find our cached branch? */
  if (way >= 0)
    {				/* Yes, we did. */
      or1k_mstats.bpb.hit++;

      for (i = 0; i < BPB_WAYS; i++)
	if (bpb[entry].way[i].lru)
	  bpb[entry].way[i].lru--;
      bpb[entry].way[way].lru = BPB_USTATES - 1;

      if (bpb[entry].way[way].taken / (BPB_PSTATES / 2) == taken)
	or1k_mstats.bpb.correct++;
      else
	or1k_mstats.bpb.incorrect++;

      if (taken && (bpb[entry].way[way].taken < BPB_PSTATES - 1))
	bpb[entry].way[way].taken++;
      else if (!taken && (bpb[entry].way[way].taken))
	bpb[entry].way[way].taken--;
    }
  else
    {				/* No, we didn't. */
      int minlru = BPB_USTATES - 1;
      int minway = 0;

      or1k_mstats.bpb.miss++;

      for (i = 0; i < BPB_WAYS; i++)
	if (bpb[entry].way[i].lru < minlru)
	  minway = i;

      bpb[entry].way[minway].addr = addr;
      bpb[entry].way[minway].taken = (BPB_PSTATES / 2 - 1) + taken;
      for (i = 0; i < BPB_WAYS; i++)
	if (bpb[entry].way[i].lru)
	  bpb[entry].way[i].lru--;
      bpb[entry].way[minway].lru = BPB_USTATES - 1;
    }
}

/* Branch target instruction cache */

/* Length of BTIC */
#define BTIC_LEN 128

/* Number of BTIC ways (1, 2, 3 etc.). */
#define BTIC_WAYS 2

/* Number of usage states (2, 3, 4 etc.). */
#define BTIC_USTATES 2

/* Target block size in bytes. */
#define BTIC_BLOCKSIZE 4

struct btic_entry
{
  struct
  {
    oraddr_t addr;		/* cached target address of a branch */
    int lru;			/* least recently used */
    char *insn;			/* cached insn at target address (not used currently) */
  } way[BTIC_WAYS];
} btic[BTIC_LEN];

void
btic_info ()
{
  if (!config.bpb.btic)
    {
      PRINTF ("BTIC not simulated. Check --btic option.\n");
      return;
    }

  PRINTF ("BTIC %d bytes: ",
	  BTIC_LEN * BTIC_WAYS * (BTIC_USTATES + BTIC_BLOCKSIZE * 8) / 8);
  PRINTF ("%d ways, %d sets, %d bits/target\n", BTIC_WAYS, BTIC_LEN,
	  BTIC_USTATES + BTIC_BLOCKSIZE * 8);
}

/* First check if target addr is already in the cache and if it is:
    - increment BTIC hit stats,
    - set 'lru' at this way to BTIC_USTATES - 1 and
      decrement 'lru' of other ways unless they have reached 0,
   and if not:
    - increment BTIC miss stats
    - find lru way and entry and replace old address with 'addr' and
      'insn' with NULL
    - set 'lru' with BTIC_USTATES - 1 and decrement 'lru' of other
      ways unless they have reached 0
*/

void
btic_update (oraddr_t targetaddr)
{
  int entry, way = -1;
  int i;

  /* BTIC simulation enabled/disabled. */
  if (!config.bpb.btic)
    return;

  /* Calc entry. */
  entry = targetaddr % BTIC_LEN;

  /* Scan all ways and try to find our addr. */
  for (i = 0; i < BTIC_WAYS; i++)
    if (btic[entry].way[i].addr == targetaddr)
      way = i;

  /* Did we find our cached branch? */
  if (way >= 0)
    {				/* Yes, we did. */
      or1k_mstats.btic.hit++;

      for (i = 0; i < BTIC_WAYS; i++)
	if (btic[entry].way[i].lru)
	  btic[entry].way[i].lru--;
      btic[entry].way[way].lru = BTIC_USTATES - 1;
    }
  else
    {				/* No, we didn't. */
      int minlru = BTIC_USTATES - 1;
      int minway = 0;

      or1k_mstats.btic.miss++;

      for (i = 0; i < BTIC_WAYS; i++)
	if (btic[entry].way[i].lru < minlru)
	  minway = i;

      btic[entry].way[minway].addr = targetaddr;
      btic[entry].way[minway].insn = NULL;
      for (i = 0; i < BTIC_WAYS; i++)
	if (btic[entry].way[i].lru)
	  btic[entry].way[i].lru--;
      btic[entry].way[minway].lru = BTIC_USTATES - 1;
    }
}

/*----------------------------------------------------[ BPB configuration ]---*/
static void
bpb_enabled (union param_val val, void *dat)
{
  config.bpb.enabled = val.int_val;
}

static void
bpb_btic (union param_val val, void *dat)
{
  config.bpb.btic = val.int_val;
}

static void
bpb_sbp_bnf_fwd (union param_val val, void *dat)
{
  config.bpb.sbp_bnf_fwd = val.int_val;
}

static void
bpb_sbp_bf_fwd (union param_val val, void *dat)
{
  config.bpb.sbp_bf_fwd = val.int_val;
}

static void
bpb_missdelay (union param_val val, void *dat)
{
  config.bpb.missdelay = val.int_val;
}

static void
bpb_hitdelay (union param_val val, void *dat)
{
  config.bpb.hitdelay = val.int_val;
}

void
reg_bpb_sec ()
{
  struct config_section *sec = reg_config_sec ("bpb", NULL, NULL);

  reg_config_param (sec, "enabled",     PARAMT_INT, bpb_enabled);
  reg_config_param (sec, "btic",        PARAMT_INT, bpb_btic);
  reg_config_param (sec, "sbp_bnf_fwd", PARAMT_INT, bpb_sbp_bnf_fwd);
  reg_config_param (sec, "sbp_bf_fwd",  PARAMT_INT, bpb_sbp_bf_fwd);
  reg_config_param (sec, "missdelay",   PARAMT_INT, bpb_missdelay);
  reg_config_param (sec, "hitdelay",    PARAMT_INT, bpb_hitdelay);
}
