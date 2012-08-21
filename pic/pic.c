/* pic.c -- Simulation of OpenRISC 1000 programmable interrupt controller

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
#include "port.h"

/* System includes */
#include <stdlib.h>
#include <stdio.h>

/* Package includes */
#include "arch.h"
#include "abstract.h"
#include "pic.h"
#include "opcode/or32.h"
#include "spr-defs.h"
#include "execute.h"
#include "except.h"
#include "sprs.h"
#include "sim-config.h"
#include "sched.h"


/* -------------------------------------------------------------------------- */
/* Reset the PIC.

   Sets registers to consistent values.                                       */
/* -------------------------------------------------------------------------- */
void
pic_reset (void)
{
  PRINTFQ ("Resetting PIC.\n");
  cpu_state.sprs[SPR_PICMR] = config.pic.use_nmi ? 0x00000003 : 0x00000000;
  cpu_state.sprs[SPR_PICSR] = 0x00000000;

}	/* pic_reset () */


/* -------------------------------------------------------------------------- */
/* Handle the reporting of an interrupt

   PIC interrupts are scheduled to take place after the current instruction
   has completed execution.

   @param[in] dat  Data associated with the exception (not used)              */
/* -------------------------------------------------------------------------- */
static void
pic_rep_int (void *dat)
{
  /* printf ("Handling interrupt PICSR: 0x%08x\n", cpu_state.sprs[SPR_PICSR]);
   */

  if (cpu_state.sprs[SPR_PICSR])
    {
      except_handle (EXCEPT_INT, cpu_state.sprs[SPR_EEAR_BASE]);
    }
}	/* pic_rep_int () */


/* -------------------------------------------------------------------------- */
/* Enable interrupts.

   Called whenever interrupts get enabled, or when PICMR is written.

   @todo Not sure if calling whenever PICMR is written is a good
         idea. However, so long as interrupts are properly cleared, it should
         not be a problem.                                                    */
/* -------------------------------------------------------------------------- */
void
pic_ints_en (void)
{
  if ((cpu_state.sprs[SPR_PICMR] & cpu_state.sprs[SPR_PICSR]))
    {
      SCHED_ADD (pic_rep_int, NULL, 0);
    }
}	/* pic_ints_en () */


/* -------------------------------------------------------------------------- */
/*!Assert an interrupt to the PIC

   OpenRISC supports both edge and level triggered interrupt. The only
   difference is how the interrupt is cleared. For edge triggered, it is by
   clearing the corresponding bit in PICSR. For level triggered it is by
   deasserting the interrupt line.

   For integrated peripherals, these amount to the same thing (using
   clear_interrupt ()). For external peripherals, the library provides two
   distinct interfaces.

   An interrupt disables any power management activity.

   We warn if an interrupt is received on a line that already has an interrupt
   pending.

   @note If this is called during a simulated instruction (ie. from a read/
         write mem callback), the interrupt will be delivered after the
         instruction has finished executing.

   @param[in] line  The interrupt being asserted.                             */
/* -------------------------------------------------------------------------- */
void
report_interrupt (int line)
{
  uint32_t lmask = 1 << line;

  /* printf ("Interrupt reported on line %d\n", line); */

  /* Disable doze and sleep mode */
  cpu_state.sprs[SPR_PMR] &= ~(SPR_PMR_DME | SPR_PMR_SME);

  /* If PIC is disabled, don't set any register, just raise EXCEPT_INT */
  if (!config.pic.enabled)
    {
      if (cpu_state.sprs[SPR_SR] & SPR_SR_IEE)
	except_handle (EXCEPT_INT, cpu_state.sprs[SPR_EEAR_BASE]);
      return;
    }

  /* We can't take another interrupt if the previous one has not been
     cleared. */
  if (cpu_state.sprs[SPR_PICSR] & lmask)
    {
      /* Interrupt already signaled and pending */
      PRINTF ("Warning: Int on line %d pending: ignored\n", line);
      return;
    }

  cpu_state.sprs[SPR_PICSR] |= lmask;

  /* If we are enabled in the mask, and interrupts are globally enabled in the
     SR, schedule the interrupt to take place after the next instruction. */
  if ((cpu_state.sprs[SPR_PICMR] & lmask) &&
      (cpu_state.sprs[SPR_SR] & SPR_SR_IEE))
    {
      /* printf ("Scheduling interrupt on line %d\n", line); */
      SCHED_ADD (pic_rep_int, NULL, 0);
    }
}	/* report_interrupt () */


/* -------------------------------------------------------------------------- */
/* Clear an interrupt on a PIC line.

   Logically this is different for a level sensitive interrupt (it lowers the
   input line) and an edge sensitive interrupt (it clears the PICSR bit).

   However within Or1ksim, these are implemented through the same operation -
   clearing the bit in PICSR.

   @param[in] line  The interrupt being cleared.                              */
/* -------------------------------------------------------------------------- */
void
clear_interrupt (int line)
{
  cpu_state.sprs[SPR_PICSR] &= ~(1 << line);

}	/* clear_interrupt */


/*----------------------------------------------------[ PIC configuration ]---*/


/*---------------------------------------------------------------------------*/
/*!Enable or disable the programmable interrupt controller

   Set the corresponding field in the UPR

   @param[in] val  The value to use
   @param[in] dat  The config data structure (not used here)                 */
/*---------------------------------------------------------------------------*/
static void
pic_enabled (union param_val  val,
	     void            *dat)
{
  if (val.int_val)
    {
      cpu_state.sprs[SPR_UPR] |= SPR_UPR_PICP;
    }
  else
    {
      cpu_state.sprs[SPR_UPR] &= ~SPR_UPR_PICP;
    }

  config.pic.enabled = val.int_val;

}	/* pic_enabled() */


/*---------------------------------------------------------------------------*/
/*!Enable or disable edge triggering of interrupts

   @param[in] val  The value to use
   @param[in] dat  The config data structure (not used here)                 */
/*---------------------------------------------------------------------------*/
static void
pic_edge_trigger (union param_val  val,
		  void            *dat)
{
  config.pic.edge_trigger = val.int_val;

}	/* pic_edge_trigger() */


/*---------------------------------------------------------------------------*/
/*!Enable or disable non-maskable interrupts

   @param[in] val  The value to use
   @param[in] dat  The config data structure (not used here)                 */
/*---------------------------------------------------------------------------*/
static void
pic_use_nmi (union param_val  val,
		  void            *dat)
{
  config.pic.use_nmi = val.int_val;

}	/* pic_use_nmi() */


/*---------------------------------------------------------------------------*/
/*!Initialize a new interrupt controller configuration

   ALL parameters are set explicitly to default values in init_defconfig()   */
/*---------------------------------------------------------------------------*/
void
reg_pic_sec ()
{
  struct config_section *sec = reg_config_sec ("pic", NULL, NULL);

  reg_config_param (sec, "enabled",      PARAMT_INT, pic_enabled);
  reg_config_param (sec, "edge_trigger", PARAMT_INT, pic_edge_trigger);
  reg_config_param (sec, "use_nmi",      PARAMT_INT, pic_use_nmi);

}	/* reg_pic_sec() */
