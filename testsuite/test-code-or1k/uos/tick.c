/* tick.c.  Microkernel tick handler for Or1ksim
	
   Copyright (C) 2000 Damjan Lampret
   Copyright (C) 2010 Embecosm Limited
   
   Contributor Damjan Lampret <lampret@opencores.org>
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
   with this program.  If not, see <http:  www.gnu.org/licenses/>.  */

/* ----------------------------------------------------------------------------
   This code is commented throughout for use with Doxygen.
   --------------------------------------------------------------------------*/

/* This file is part of test microkernel for OpenRISC 1000. */

#include "spr-defs.h"
#include "support.h"

/* Tick timer period */
unsigned long tick_period;

/* Inform of tick interrupt */
void (*tick_inf)();

/* Tick interrupt routine */
void tick_int()
{
  /* Call inf routine */
  (*tick_inf)();

  /* Set new counter period iand clear inet pending bit */
  mtspr(SPR_TTMR, SPR_TTMR_IE | SPR_TTMR_RT | (tick_period & SPR_TTMR_TP));
} 

/* Initialize routine */
int tick_init(unsigned long period, void (* inf)())
{
  /* Save tick timer period and inform routine */
  tick_period = period;
  tick_inf = inf;

  /* Set counter period, enable timer and interrupt */
  mtspr(SPR_TTMR, SPR_TTMR_IE | SPR_TTMR_RT | (period & SPR_TTMR_TP));
	
  return 0;
}
