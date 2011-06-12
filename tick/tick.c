/* tick.c -- Simulation of OpenRISC 1000 tick timer

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

/* This is functional simulation of OpenRISC 1000 architectural tick timer */

/* Autoconf and/or portability configuration */
#include "config.h"
#include "port.h"

/* System includes */
#include <stdlib.h>
#include <stdio.h>

/* Package includes */
#include "arch.h"
#include "abstract.h"
#include "except.h"
#include "tick.h"
#include "opcode/or32.h"
#include "spr-defs.h"
#include "execute.h"
#include "pic.h"
#include "sprs.h"
#include "sim-config.h"
#include "sched.h"


/*! When did the timer start to count */
static long long cycle_count_at_tick_start = 0;

/*! Indicates if the timer is actually counting.  Needed to simulate one-shot
    mode correctly */
int tick_counting;

/*! Reset. It initializes TTCR register. */
void
tick_reset (void)
{
  if (config.sim.verbose)
    {
      PRINTF ("Resetting Tick Timer.\n");
    }

  cpu_state.sprs[SPR_TTCR] = 0;
  cpu_state.sprs[SPR_TTMR] = 0;
  tick_counting = 0;
}

/*! Raises a timer exception */
static void
tick_raise_except (void *dat)
{
  cpu_state.sprs[SPR_TTMR] |= SPR_TTMR_IP;

  /* Reschedule unconditionally, since we have to raise the exception until
   * TTMR_IP has been cleared */
  sched_next_insn (tick_raise_except, NULL);

  /* be sure not to issue a timer exception if an exception occured before it */
  if (cpu_state.sprs[SPR_SR] & SPR_SR_TEE)
    {
      except_handle (EXCEPT_TICK, cpu_state.sprs[SPR_EEAR_BASE]);
    }
}

/*! Restarts the tick timer */
static void
tick_restart (void *dat)
{
  cpu_state.sprs[SPR_TTCR] = 0;
  cycle_count_at_tick_start = runtime.sim.cycles;
  SCHED_ADD (tick_restart, NULL, cpu_state.sprs[SPR_TTMR] & SPR_TTMR_TP);
}

/*! Stops the timer */
static void
tick_one_shot (void *dat)
{
  cpu_state.sprs[SPR_TTCR] = cpu_state.sprs[SPR_TTMR] & SPR_TTMR_TP;
  tick_counting = 0;
}

/*! Schedules the timer jobs */
static void
sched_timer_job (uorreg_t prev_ttmr)
{
  uorreg_t ttmr = cpu_state.sprs[SPR_TTMR];
  uint32_t match_ttmr = ttmr & SPR_TTMR_TP;
  /* TTCR register, only concerned with part of TTCR which will trigger int */
  uint32_t match_ttcr = spr_read_ttcr () & SPR_TTMR_TP;
  uint32_t cycles_until_except;

  /* On clearing TTMR interrupt signal bit remove previous jobs if they 
     exist */
  if ((prev_ttmr & SPR_TTMR_IE) && !(ttmr & SPR_TTMR_IP))
    {
      SCHED_FIND_REMOVE (tick_raise_except, NULL);
    }

  switch (prev_ttmr & SPR_TTMR_M)
    {
    case SPR_TTMR_RT:
      SCHED_FIND_REMOVE (tick_restart, NULL);
      break;

    case SPR_TTMR_SR:
      SCHED_FIND_REMOVE (tick_one_shot, NULL);
      break;
    }

  /* Calculate cycles until next tick exception, based on current TTCR value */
  if (match_ttmr >= match_ttcr)
    {
      cycles_until_except = match_ttmr - match_ttcr;
    }
  else
    {
      /* Cycles after "wrap" of section of TTCR which will cause a match and, 
	 potentially, an exception */
      cycles_until_except = match_ttmr + (0x0fffffffu - match_ttcr) + 1;
    }

  switch (ttmr & SPR_TTMR_M)
    {
    case 0:			/* Disabled timer */
      if (!cycles_until_except && (ttmr & SPR_TTMR_IE) && !(ttmr & SPR_TTMR_IP))
	SCHED_ADD (tick_raise_except, NULL, 0);
      break;

    case SPR_TTMR_RT:		/* Auto-restart timer */
      SCHED_ADD (tick_restart, NULL, cycles_until_except);
      if ((ttmr & SPR_TTMR_IE) && !(ttmr & SPR_TTMR_IP))
	SCHED_ADD (tick_raise_except, NULL, cycles_until_except);
      break;

    case SPR_TTMR_SR:		/* One-shot timer */
      if (tick_counting)
	{
	  SCHED_ADD (tick_one_shot, NULL, cycles_until_except);
	  if ((ttmr & SPR_TTMR_IE) && !(ttmr & SPR_TTMR_IP))
	    SCHED_ADD (tick_raise_except, NULL, cycles_until_except);
	}
      break;

    case SPR_TTMR_CR:		/* Continuos timer */
      if ((ttmr & SPR_TTMR_IE) && !(ttmr & SPR_TTMR_IP))
	SCHED_ADD (tick_raise_except, NULL, cycles_until_except);
    }
}


/*! Handles a write to the ttcr spr */
void
spr_write_ttcr (uorreg_t value)
{
  cycle_count_at_tick_start = runtime.sim.cycles - value;

  sched_timer_job (cpu_state.sprs[SPR_TTMR]);
}

/*! prev_val is the *previous* value of SPR_TTMR.  The new one can be found in
    cpu_state.sprs[SPR_TTMR] */
void
spr_write_ttmr (uorreg_t prev_val)
{
  uorreg_t value = cpu_state.sprs[SPR_TTMR];

  /* Code running on or1k can't set SPR_TTMR_IP so make sure it isn't */
  cpu_state.sprs[SPR_TTMR] &= ~SPR_TTMR_IP;

  /* If the timer was already disabled, ttcr should not be updated */
  if (tick_counting)
    {
      cpu_state.sprs[SPR_TTCR] = runtime.sim.cycles - cycle_count_at_tick_start;
    }

  cycle_count_at_tick_start = runtime.sim.cycles - cpu_state.sprs[SPR_TTCR];

  tick_counting = value & SPR_TTMR_M;

  /* If TTCR==TTMR_TP when setting MR, we disable counting?? 
     I think this should be looked at - Julius */
  if ((tick_counting == SPR_TTMR_CR) &&
      (cpu_state.sprs[SPR_TTCR] == (value & SPR_TTMR_TP)))
    {
      tick_counting = 0;
    }

  sched_timer_job (prev_val);
}

uorreg_t
spr_read_ttcr ()
{
  uorreg_t ret;

  if (!tick_counting)
    {
      /* Report the time when the counter stopped (and don't carry on
	 counting) */
      ret = cpu_state.sprs[SPR_TTCR];
    }
  else
    {
      ret = runtime.sim.cycles - cycle_count_at_tick_start;
    }

  return  ret;
}
