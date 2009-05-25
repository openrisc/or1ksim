/* tick.c -- Tests all aspects of the tick timer
   Copyright (C) 2005 György `nog' Jeney, nog@sdf.lonestar.org

This file is part of OpenRISC 1000 Architectural Simulator.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA. */

#include "spr_defs.h"
#include "support.h"

#define ASSERT(x) ((x)? printf(" Test succeeded %s:%i\n", __FILE__, __LINE__) : fail (__FILE__, __LINE__))

void fail (char *func, int line)
{
#ifndef __FUNCTION__
#define __FUNCTION__ "?"
#endif
  printf ("Test failed in %s:%i\n", func, line);
  report(0xeeeeeeee);
  exit (1);
}

static volatile int tick_cnt = 0;
static int clear_ip = 1;

void tick_int(void)
{
  int ttcr = mfspr(SPR_TTCR);
  int ttmr = mfspr(SPR_TTMR);

  /* Make sure that the tick timer interrupt pending bit is set */
  ASSERT(mfspr(SPR_TTMR) & SPR_TTMR_IP);

  tick_cnt++;

  /* Clear interrupt (Write a 0 to SPR_TTMR_IP) */
  /* If we programmed a one-shot timer, make sure to disable the interrupts,
   * else we'd get a spurious interrupt */
  if((ttmr & SPR_TTMR_M) != 0x80000000)
    mtspr(SPR_TTMR, mfspr(SPR_TTMR) & ~SPR_TTMR_IP);
  else
    mtspr(SPR_TTMR, mfspr(SPR_TTMR) & ~(SPR_TTMR_IP | SPR_TTMR_IE));
}

void tick_int_spurious(void)
{
  /* Make sure that the tick timer interrupt pending bit is set */
  ASSERT(mfspr(SPR_TTMR) & SPR_TTMR_IP);

  /* Clear interrupt (Write a 0 to SPR_TTMR_IP) */
  if(clear_ip)
    mtspr(SPR_TTMR, mfspr(SPR_TTMR) & ~SPR_TTMR_IP);

  /* Make sure we actually get a spurious interrupt */
  if(++tick_cnt == 5)
    /* We should get spurious tick interrupts so disable it */
    mtspr(SPR_TTMR, 0);
}

/* Does nothing for a small amount of time (waiting for TTCR to increment) */
void waste_time(void)
{
  int i;
  volatile int x;

  for(i = 0; i < 50; i++)
    x = i;
}

/* Waits for a tick timer exception */
void wait_match(void)
{
  while(!tick_cnt);
  tick_cnt = 0;
}

int main()
{
  int ttcr;

  excpt_tick = (unsigned long)tick_int;

  /* Enable tick interrupt */
  mtspr(SPR_SR, mfspr(SPR_SR) | SPR_SR_TEE);

  /* Disable timer */
  mtspr(SPR_TTMR, 0);

  mtspr(SPR_TTCR, 0);

  /* Waste some time to check if the timer is really disabled */
  waste_time();

  /* Timer is disabled and shouldn't count, TTCR should still be 0 */
  ASSERT(mfspr(SPR_TTCR) == 0);

  /* Start timer in continous timeing mode.  Enable timer interrupt and set the
   * value to match */
  mtspr(SPR_TTMR, (3 << 30) | SPR_TTMR_IE | 0x100);

  /* Wait for the timer to count up to the match value */
  wait_match();

  ttcr = mfspr(SPR_TTCR);

  waste_time();

  /* The timer should have kept counting and our saved ttcr should not be the
   * same as SPR_TTCR */
  ASSERT(ttcr < mfspr(SPR_TTCR));

  mtspr(SPR_TTMR, 0);
  ttcr = mfspr(SPR_TTCR);
  /* Restart the timer */
  mtspr(SPR_TTMR, (3 << 30));
  waste_time();
  /* The timer should have carried on from what was SPR_TTCR when we started it.
   */
  ASSERT(ttcr < mfspr(SPR_TTCR));

  mtspr(SPR_TTMR, 0);
  ttcr = mfspr(SPR_TTCR);
  waste_time();
  /* Timer should be disabled and should not have counted */
  ASSERT(ttcr == mfspr(SPR_TTCR));

  /* Stop the timer when a match occured */
  mtspr(SPR_TTCR, 0);
  mtspr(SPR_TTMR, (2 << 30) | SPR_TTMR_IE | 0x100);

  wait_match();
  ttcr = mfspr(SPR_TTCR);
  waste_time();
  /* The timer should have stoped and thus SPR_TTCR sould = ttcr */
  ASSERT(ttcr == mfspr(SPR_TTCR));

  /* The counter should still indicate one-shot mode */
  ASSERT((mfspr(SPR_TTMR) >> 30) == 2);

  /* Set auto-restarting timer */
  mtspr(SPR_TTMR, 0);
  mtspr(SPR_TTCR, 0);
  mtspr(SPR_TTMR, (1 << 30) | SPR_TTMR_IE | 0x1000);
  wait_match();
  ttcr = mfspr(SPR_TTCR);
  wait_match();

  /* Disable timer */
  mtspr(SPR_TTMR, 0);

  /* Start a one-shot counter but keep interrupts disabled */
  mtspr(SPR_TTCR, 0);
  mtspr(SPR_TTMR, (2 << 30) | 0x100);

  /* Wait for the counter to stop */
  while(mfspr(SPR_TTCR) != 0x100);
  /* Make sure the counter has actually stopped */
  waste_time();
  ASSERT(mfspr(SPR_TTCR) == 0x100);
  ASSERT(tick_cnt == 0);

  /* SPR_TTMR_IP should not be set */
  ASSERT(!(mfspr(SPR_TTMR) & SPR_TTMR_IP));

  /* Start a perpetual counter (makeing sure that no interrupts occur while it's
   * counting) */
  mtspr(SPR_TTMR, 0);
  mtspr(SPR_TTCR, 0);
  mtspr(SPR_TTMR, (3 << 30) | 0x100);
  while(mfspr(SPR_TTCR) < 0x100);
  waste_time();
  ASSERT(mfspr(SPR_TTCR) > 0x100);
  ASSERT(tick_cnt == 0);
  ASSERT(!(mfspr(SPR_TTMR) & SPR_TTMR_IP));

  /* Disable the timer interrupt */
  mtspr(SPR_SR, mfspr(SPR_SR) & ~SPR_SR_TEE);

  /* Set one-shot timer */
  mtspr(SPR_TTCR, 0);
  mtspr(SPR_TTMR, (2 << 30) | SPR_TTMR_IE | 0x100);
  while(!(mfspr(SPR_TTMR) & SPR_TTMR_IP));
  /* Give some time for a potential interrupt to occur */
  waste_time();
  /* No interrupt should have occured */
  ASSERT(tick_cnt == 0);

  /* Enable tick interrupt */
  mtspr(SPR_SR, mfspr(SPR_SR) | SPR_SR_TEE);

  /* Test Setting TTCR while counting */
  mtspr(SPR_TTMR, 0);
  mtspr(SPR_TTCR, 0);
  mtspr(SPR_TTMR, (3 << 30) | 0x3000);
  while(mfspr(SPR_TTCR) < 0x30000);
  waste_time();
  mtspr(SPR_TTCR, 0x50);
  waste_time();
  ttcr = mfspr(SPR_TTCR);
  ASSERT(ttcr > 0x50 && ttcr < 0x30000);
  mtspr(SPR_TTMR, 0);

  /* Set ttcr to a high value */
  mtspr(SPR_TTCR, 0x20000);
  /* Now, start timer in one-shot mode */
  mtspr(SPR_TTMR, (2 << 30) | SPR_TTMR_IE | 0x100);

  /* The counter should start counting from 0x20000 and wrap around to 0x100
   * causeing an interrupt */
  waste_time();
  ASSERT(mfspr(SPR_TTCR) > 0x20000);
  wait_match();

  ASSERT(1);

  /* If TTCR is greater than TTMR_PERIOD then the interrupt gets delivered after
   * TTCR wraps around to 0 and counts to SPR_TTMR_PERIOD */
  mtspr(SPR_TTMR, 0);
  mtspr(SPR_TTCR, 0xffffc00);
  mtspr(SPR_TTMR, (1 << 30) | SPR_TTMR_IE | 0x10000);
  wait_match();

  ASSERT(1);

  /* test continuous mode */
  mtspr(SPR_TTMR, 0);
  mtspr(SPR_TTCR, 0xffffc00);
  mtspr(SPR_TTMR, (3 << 30) | SPR_TTMR_IE | 0x10000);
  wait_match();
  mtspr(SPR_TTMR, 0);

  ASSERT(1);

  excpt_tick = (unsigned long)tick_int_spurious;

  /* Make sure sure that TTMR_PERIOD is not 0!! */
  mtspr(SPR_TTMR, 1);
  /* Set SPR_TTMR_PERIOD to some value, while keeping the timer disabled */
  mtspr(SPR_TTCR, 0);
  mtspr(SPR_TTMR, SPR_TTMR_IE | 0x100);

  /* Set SPR_TTCR with the same value */
  mtspr(SPR_TTCR, 0x100);

  while(tick_cnt != 5);
  tick_cnt = 0;
  ASSERT(mfspr(SPR_TTCR) == 0x100);

  /* Test setting TTCR first then TTMR */
  mtspr(SPR_TTCR, 0x101);
  mtspr(SPR_TTMR, SPR_TTMR_IE | 0x101);

  while(tick_cnt != 5);
  tick_cnt = 0;
  ASSERT(mfspr(SPR_TTCR) == 0x101);

  /* Set countinous counter, but make sure we never clear the TTMR_IP bit */
  clear_ip = 0;
  mtspr(SPR_TTCR, 0);
  mtspr(SPR_TTMR, (3 << 30) | SPR_TTMR_IE | 0x100);

  while(tick_cnt != 5);

  report(0xdeaddead);
  return 0;
}

