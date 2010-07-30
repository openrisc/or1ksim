/* pm.c -- Simulation of OpenRISC 1000 power management

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


/* Autoconf and/or portability configuration */
#include "config.h"

/* Package includes */
#include "spr-defs.h"
#include "execute.h"
#include "sim-config.h"


/*---------------------------------------------------------------------------*/
/*!Reset power management

   Initializes PMR register by clearing it.                                  */
/*---------------------------------------------------------------------------*/
void
pm_reset ()
{
  if (config.sim.verbose)
    {
      PRINTF ("Resetting Power Management.\n");
    }

  cpu_state.sprs[SPR_PMR] = 0;

}				/* pm_reset() */


/*---------------------------------------------------------------------------*/
/*!Enable or disable power management

   Set the corresponding field in the UPR

   @param[in] val  The value to use
   @param[in] dat  The config data structure (not used here)                 */
/*---------------------------------------------------------------------------*/
static void
pm_enabled (union param_val val, void *dat)
{
  if (val.int_val)
    {
      cpu_state.sprs[SPR_UPR] |= SPR_UPR_PMP;
    }
  else
    {
      cpu_state.sprs[SPR_UPR] &= ~SPR_UPR_PMP;
    }

  config.pm.enabled = val.int_val;

}				/* pm_enabled() */


/*---------------------------------------------------------------------------*/
/*!Set up a new power management configuration section                       */
/*---------------------------------------------------------------------------*/
void
reg_pm_sec ()
{
  struct config_section *sec = reg_config_sec ("pm", NULL, NULL);

  reg_config_param (sec, "enabled", PARAMT_INT, pm_enabled);

}				/* reg_pm_sec() */
