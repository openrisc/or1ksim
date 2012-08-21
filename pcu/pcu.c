/* pcu.c -- Simulation of OR1k performance counters

   Copyright (C) 2011 Julius Baxter, julius@opencores.org

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

#include "config.h"
#include "sim-config.h"
#include "execute.h"

#include "pcu.h"

/*---------------------------------------------------------------------------*/
/*!Increment a PCCR depending on the event          

   Check the event, see if any PCMRs are set to increment, and permissions and
   increment the corresponding PCCR.

   The performance counters unit relies on a call to pcu_count_event() 
   being placed a the appropriate place elsewhere in the simulator code.
   At present only instruction fetch, LSU accesses, MMU and cache misses are 
   logged. Counts for other things such as stalls for LSU or fetch can 
   probably be done but haven't been, yet.
   The function expects to receive the bit of the PCMR to check against for
   incrementing. See the existing use of the function in the 
   cpu/common/abstract.c file which is where LSU accesses and instruction
   fetches are logged.

   @param[in] event  The event that has occurred (PCMR bit)                  */
/*---------------------------------------------------------------------------*/
void
pcu_count_event(unsigned event)
{

  int i;

  // Look through each PCMR, check if this event should be logged
  for(i=0;i<8;i++)
    if ((event & cpu_state.sprs[SPR_PCMR(i)]) && 
	// Count in SM, and SR[SM] set
	(((cpu_state.sprs[SPR_PCMR(i)] & SPR_PCMR_CISM) && 
	  (cpu_state.sprs[SPR_SR] & SPR_SR_SM)) ||
	 // Count in UM and SR[SM] cleared
	 ((cpu_state.sprs[SPR_PCMR(i)] & SPR_PCMR_CIUM) && 
	  !(cpu_state.sprs[SPR_SR] & SPR_SR_SM))))
      {
	cpu_state.sprs[SPR_PCCR(i)]++;
      }
  
}


/*---------------------------------------------------------------------------*/
/*!Enable or disable the performance counters unit

   Set the corresponding field in the UPR, set present bit in all PCMRs

   @param[in] val  The value to use
   @param[in] dat  The config data structure (not used here)                 */
/*---------------------------------------------------------------------------*/
static void
pcu_enabled (union param_val val, void *dat)
{
  if (val.int_val)
    {
      cpu_state.sprs[SPR_UPR] |= SPR_UPR_PCUP;
      cpu_state.sprs[SPR_PCMR(0)] = SPR_PCMR_CP;
      cpu_state.sprs[SPR_PCMR(1)] = SPR_PCMR_CP;
      cpu_state.sprs[SPR_PCMR(2)] = SPR_PCMR_CP;
      cpu_state.sprs[SPR_PCMR(3)] = SPR_PCMR_CP;
      cpu_state.sprs[SPR_PCMR(4)] = SPR_PCMR_CP;
      cpu_state.sprs[SPR_PCMR(5)] = SPR_PCMR_CP;
      cpu_state.sprs[SPR_PCMR(6)] = SPR_PCMR_CP;
      cpu_state.sprs[SPR_PCMR(7)] = SPR_PCMR_CP;
    }
  else
    {
      cpu_state.sprs[SPR_UPR] &= ~SPR_UPR_PCUP;
    }



  config.pcu.enabled = val.int_val;

}	/* pcu_enabled() */


/*---------------------------------------------------------------------------*/
/*!Register the configuration functions for the performance counters unit    */
/*---------------------------------------------------------------------------*/
void
reg_pcu_sec ()
{
  struct config_section *sec = reg_config_sec ("pcu", NULL, NULL);

  reg_config_param (sec, "enabled",     PARAMT_INT, pcu_enabled);

}	/* reg_pcu_sec () */
