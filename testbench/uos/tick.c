/* This file is part of test microkernel for OpenRISC 1000. */
/* (C) 2001 Simon Srot, srot@opencores.org */

#include "spr_defs.h"
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
	mtspr(SPR_TTMR, SPR_TTMR_IE | SPR_TTMR_RT | (tick_period & SPR_TTMR_PERIOD));
} 

/* Initialize routine */
int tick_init(unsigned long period, void (* inf)())
{
  /* Save tick timer period and inform routine */
  tick_period = period;
  tick_inf = inf;

  /* Set counter period, enable timer and interrupt */
  mtspr(SPR_TTMR, SPR_TTMR_IE | SPR_TTMR_RT | (period & SPR_TTMR_PERIOD));
	
  return 0;
}
