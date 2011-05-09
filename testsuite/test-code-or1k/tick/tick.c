/* tick.c. Test of Or1ksim tick timer

   Copyright (C) 2005 György `nog' Jeney <nog@sdf.lonestar.org>
   Copyright (C) 2010 Embecosm Limited

   Contributor György `nog' Jeney <nog@sdf.lonestar.org>
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

#include "spr-defs.h"
#include "support.h"

/*! Whether to perform spurious interrupt test */
#define DO_SPURIOUS_INT_TEST 0

/*! Number of spurious interrupts we'll allow before properly disabling them */
#define MAX_SPURIOUS  5

/*! Convenience macro to check that a test has passed */
#define ASSERT(x, msg) ((x) ? printf ("Test succeeded %s\n", msg) : fail (msg))

/*! Convenience macro to check that a test has passed, but only print a
    message on failure. */
#define SILENT_ASSERT(x, msg) ((x) ?  : fail (msg))

/*! Convenience macros for accessing SPRs */
#define GET_TTMR() (mfspr (SPR_TTMR))
#define SET_TTMR(x) (mtspr (SPR_TTMR, x))
#define GET_TTCR() (mfspr (SPR_TTCR))
#define SET_TTCR(x) (mtspr (SPR_TTCR, x))

/*! Global count of number of times interrupt handler has been called  */
static volatile int tick_cnt = 0;

#if DO_SPURIOUS_INT_TEST==1
/*! Global flag to indicate if the TTMR_IP flag should be cleared */
static int clear_ip = 1;
#endif


/* --------------------------------------------------------------------------*/
/*!Report failure

  Terminate execution.

  @param[in] msg  Description of test which failed.                          */
/* --------------------------------------------------------------------------*/
static void
fail (char *msg)
{
  printf ("Test failed: %s\n", msg);
  report (0xeeeeeeee);
  exit (1);

}	/* fail () */


/* --------------------------------------------------------------------------*/
/*!Set a tick timer mode.

   All other bits and the period are masked

   @param[in] ttmr  The entire TTMR value.
   @param[in] mode  The new mode

   @return  The new TTMR with the mode set                                   */
/* --------------------------------------------------------------------------*/
static unsigned long int
set_mode (unsigned long int  ttmr,
	  unsigned long int  mode)
{
  ttmr &= ~SPR_TTMR_M;
  ttmr |= mode & SPR_TTMR_M;

  return  ttmr;

}	/* set_mode () */


/* --------------------------------------------------------------------------*/
/*!Set a tick timer period.

   All other bits and the period are masked

   @param[in] ttmr    The entire TTMR value.
   @param[in] period  The new period

   @return  The new TTMR with the period set                                 */
/* --------------------------------------------------------------------------*/
static unsigned long int
set_period (unsigned long int  ttmr,
	    unsigned long int  period)
{
  ttmr &= ~SPR_TTMR_TP;
  ttmr |= period & SPR_TTMR_TP;

  return  ttmr;

}	/* set_period () */


/* --------------------------------------------------------------------------*/
/*!Clear the mode register

   Period is zeroed, interrupt pending and enabled flags are cleared, disabled
   mode is set.

   @note  This function exists to allow for the disabled mode to be explicit,
          rather than assumed to be zero.

   @return  The new TTMR		                                     */
/* --------------------------------------------------------------------------*/
static unsigned long int
clear_ttmr ()
{
  unsigned long int  ttmr = SPR_TTMR_DI;

  SET_TTMR (ttmr);

  return  ttmr;

}	/* clear_ttmr () */


/* --------------------------------------------------------------------------*/
/*!Clear the count register

   Count is zeroed

   @return  The new TTCR		                                     */
/* --------------------------------------------------------------------------*/
static unsigned long int
clear_ttcr ()
{
  unsigned long int  ttcr = 0;

  SET_TTCR (ttcr);

  return  ttcr;

}	/* clear_ttcr () */


/* --------------------------------------------------------------------------*/
/*!Set a new timer.

   Convenience function for a common sequence

   @param[in] period  The period of the timer
   @param[in] mode    The timer mode
   @param[in] flags   Any falgs to set (usually IE)

   @return  The new value of TTMR	                                     */
/* --------------------------------------------------------------------------*/
static unsigned long int
new_timer (unsigned long int  period,
	   unsigned long int  mode,
	   unsigned long int  flags)
{
  unsigned long int  ttmr;

  ttmr  = 0;
  ttmr  = set_period (ttmr, period);
  ttmr  = set_mode (ttmr, mode);
  ttmr |= flags;

  SET_TTMR (ttmr);

  return  ttmr;

}	/* new_timer () */


/* --------------------------------------------------------------------------*/
/*!Standard tick interrupt handler

  Up the count and clear the interrupt appropriately                        */
/* --------------------------------------------------------------------------*/
static void
tick_int (void)
{
  unsigned long int  ttmr = mfspr (SPR_TTMR);

  /* Make sure that the tick timer interrupt pending bit is set */
  SILENT_ASSERT (0 != (ttmr & SPR_TTMR_IP),
		 "IP bit not set in normal interrupt handler");

  /* One more interrupt handled */
  tick_cnt++;

  /* Clear interrupt (Write a 0 to SPR_TTMR_IP)

     If we programmed a one-shot timer, make sure to disable the interrupts,
     else we'd get a spurious interrupt */
  if (SPR_TTMR_SR == (ttmr & SPR_TTMR_M))
    {
      ttmr &= ~(SPR_TTMR_IP | SPR_TTMR_IE);
    }
  else
    {
      ttmr &= ~SPR_TTMR_IP;
    }

  mtspr (SPR_TTMR, ttmr);

}	/* tick_count () */


#if DO_SPURIOUS_INT_TEST==1
/* --------------------------------------------------------------------------*/
/*!Tick interrupt handler generting spurious interrupts

  If we have a one-shot timer set, then when we clear the interrupt (if a
  global flag allows), but leave the interrupt enabled we should get a
  spurious interrupt.

  Allow this to happen MAX_SPURIOUS times before disabling the interrupt.

  Up the count and clear the interrupt appropriately                        */
/* --------------------------------------------------------------------------*/
static void
tick_int_spurious (void)
{
  unsigned long int  ttmr = mfspr(SPR_TTMR);

  /* Make sure that the tick timer interrupt pending bit is set */
  SILENT_ASSERT (0 != (ttmr & SPR_TTMR_IP),
		 "IP bit not set in spurious interrupt handler");

  /* Clear interrupt if global flag allows it (Write a 0 to SPR_TTMR_IP) */
  if (clear_ip)
    {
      ttmr &= ~SPR_TTMR_IP;
      mtspr (SPR_TTMR, ttmr);
    }

  /* Allow MAX_SPURIOUS spurious spurious interrupt */
  if (++tick_cnt == MAX_SPURIOUS)
    {
      /* Clear mode register completely */
      ttmr = 0;
      mtspr (SPR_TTMR, ttmr);
      
      
    }
}	/* tick_int_spurious () */
#endif

/* --------------------------------------------------------------------------*/
/*!Waste a little time

  We are waiting for TTCR to increment

  @note This is an entirely arbitrary period. In particular excessive use of
  printf in an interrupt handler can tie up cycles and cause TTCR not
  to increment in time.                                               */
/* --------------------------------------------------------------------------*/
static void
waste_time (void)
{
  int          i;
  volatile int x;

  for (i = 0; i < 50; i++)
    {
      x = i;
    }
}	/* waste_time () */

/* --------------------------------------------------------------------------*/
/*!Wait for a tick timer exception

  This occurs when the tick count goes up. Reset the count once the
  exception is received.                                                    */
/* --------------------------------------------------------------------------*/
/* Waits for a tick timer exception */
static void
wait_match (void)
{
  while (!tick_cnt)
    {
    }

  tick_cnt = 0;

}	/* wait_match () */


/* --------------------------------------------------------------------------*/
/*!Main program testing tick timer

  Tests all aspecst of tick timer behavior.

  @return  Return code from the program (always zero).                      */
/* --------------------------------------------------------------------------*/
int
main()
{
  unsigned long int  ttmr;
  unsigned long int  ttcr;

  /* Use normal interrupt handler */
  excpt_tick = (unsigned long)tick_int;

  /* Enable tick interrupt */
  mtspr (SPR_SR, mfspr (SPR_SR) | SPR_SR_TEE);

  /* Clear interrupt pending and interrupt enabled, zero the period, set
     disabled mode and set count to zero. */
  ttmr = clear_ttmr ();
  ttcr = clear_ttcr ();

  /* Waste some time to check if the timer is really disabled */
  waste_time ();

  /* Timer is disabled and shouldn't count, TTCR should still be 0 */
  ttcr = GET_TTCR ();
  ASSERT (0 == ttcr, "Tick timer not counting while disabled");

  /* Start timer in continous running mode.  Enable timer interrupt and set
   * the period to 0x100 */
  ttmr = new_timer (0x100, SPR_TTMR_CR, SPR_TTMR_IE);

  /* Wait for the timer to count up to the match value, get the count
     register value. Then waste some time and check that couting has
     continued. */
  wait_match ();
  ttcr = GET_TTCR ();
  waste_time();

  /* The timer should have kept counting and our saved TTCR should not be the
   * same as the current ttcr */
  ASSERT (ttcr < GET_TTCR (),
	  "Tick timer kept counting during continuous mode");

  /* Clear the mode register flags, zero the period and set disabled
     mode. Then get the count register which should be unaffected by this. */
  ttmr = clear_ttmr ();
  ttcr = GET_TTCR ();

  /* Restart the timer in continous run mode and the counter will keep
     going. There should be no interrupts, since we haven't enabled
     them. Waste some time to allow the counter to advance. */
  ttmr = set_mode (ttmr, SPR_TTMR_CR);
  SET_TTMR (ttmr);
  waste_time ();

  /* The timer should have carried on from what was SPR_TTCR when we started
     it. */
  ASSERT (ttcr < GET_TTCR (), "Tick timer continued counting after restart");

  /* Disable the timer and waste some time to check that the count does not
     advance */
  ttmr = clear_ttmr ();
  ttcr = GET_TTCR ();
  waste_time ();

  /* Timer should be disabled and should not have counted */
  ASSERT(ttcr == GET_TTCR (), "Tick timer counter stops when disabled");

  /* Start in single run mode with a count of 0x100. Run until the match is
     hit, then check that the counter does not advance further while wasting
     time. */
  ttcr = clear_ttcr ();
  ttmr = new_timer (0x100, SPR_TTMR_SR, SPR_TTMR_IE);

  wait_match();
  ttcr = GET_TTCR ();
  waste_time();

  /* The timer should have stoped and the counter advanced no further. */
  ASSERT (ttcr == GET_TTCR (), "Timer stopped after match");

  /* The counter should still indicate single run mode */
  ttmr = GET_TTMR ();
  ASSERT ((ttmr & SPR_TTMR_SR) == SPR_TTMR_SR,
	  "Timer still indicating one-shot mode after match");

  /* Disable the timer, then start auto-restarting timer every 0x1000 ticks. */
  ttmr = clear_ttmr ();
  ttcr = clear_ttcr ();
  ttmr = new_timer (0x1000, SPR_TTMR_RT, SPR_TTMR_IE);

  /* Wait for two matches, then disable the timer. If this doesn't work the
     test will time out, so no ASSERT here. */
  wait_match();
  ttcr = GET_TTCR ();
  wait_match();

  /* Start a one-shot counter but keep interrupts disabled */
  ttmr = clear_ttmr ();
  ttcr = clear_ttcr ();
  ttmr = new_timer (0x100, SPR_TTMR_SR, 0);

  /* Wait for the counter to stop */
  while (GET_TTCR () != 0x100)
    {
    }

  /* Make sure the counter has actually stopped and there have been no more
     interrupts (neither tick count nor pending flag. */
  waste_time();
  ttmr = GET_TTMR ();

  ASSERT (GET_TTCR () == 0x100, "One-shot timer stopped");
  ASSERT (tick_cnt == 0, "No more interrupts after one-shot timer");
  ASSERT (SPR_TTMR_IP != (ttmr & SPR_TTMR_IP),
	  "IP flag not set after one-shot timer");

  /* Start a perpetual counter but with no interrupts enabled while it is
     still counting. */
  ttmr = clear_ttmr ();
  ttcr = clear_ttcr ();
  ttmr = new_timer (0x100, SPR_TTMR_CR, 0);

  /* Wait until it reaches its count. */
  while(GET_TTCR () < 0x100)
    {
    }

  /* Waste some time and check the counter has carried on past its count and
     that there have bee no more interrupts nor interrupts pending. */
  waste_time();

  ttmr = GET_TTMR ();
  ttcr = GET_TTCR ();

  ASSERT (ttcr > 0x100, "Perptual timer kept counting");
  ASSERT (tick_cnt == 0, "No more interrupts during perpetual timer count");
  ASSERT (SPR_TTMR_IP != (ttmr & SPR_TTMR_IP),
	  "IP flag not set during perpetual timer count");

  /* Disable the timer interrupt */
  mtspr (SPR_SR, mfspr (SPR_SR) & ~SPR_SR_TEE);

  /* Set a one-shot timer, with the counter started at zero. */
  ttmr = clear_ttmr ();
  ttcr = clear_ttcr ();
  ttmr = new_timer (0x100, SPR_TTMR_SR, SPR_TTMR_IE);

  /* Wait for the interrupt pending bit to be set. */
  do
    {
      ttmr = GET_TTMR ();
    }
  while (0 == (ttmr & SPR_TTMR_IP));

  /* Give some time for a potential interrupt to occur */
  waste_time();

  /* No interrupt should have occured */
  ASSERT (tick_cnt == 0, "No interrupt when tick timer disabled");

  /* Enable tick interrupt */
  mtspr (SPR_SR, mfspr (SPR_SR) | SPR_SR_TEE);

  /* Test Setting TTCR while counting. Set a period of 0x3000 but do not
     enable interrupts. */
  ttmr = clear_ttmr ();
  ttcr = clear_ttcr ();
  ttmr = new_timer (0x3000, SPR_TTMR_CR, 0);

  /* Wait for the count to reach 10 times our period. */
  while (GET_TTCR () < 0x30000)
    {
    }

  /* Waste some time and then reset the count to 0x50 */
  waste_time();
  SET_TTCR (0x50);

  /* Verify that after a short wait we have counted to more than 0x50, but
     less than 0x30000 */
  waste_time();
  ttcr = GET_TTCR ();

  ASSERT ((0x50 < ttcr) && (ttcr < 0x30000), "TTCR reset while counting");

  /* Disable the timer. Set the counter to a high value and start a single run
     timer with a low timer period. Demonstrate the counter wraps round and
     then triggers at the period. Need to reset the tick counter, since there
     may have been an interrupt during the previous period. */
  ttmr = clear_ttmr ();
  ttcr = 0x20000;
  SET_TTCR (ttcr);

  ttmr = new_timer (0x100, SPR_TTMR_SR, SPR_TTMR_IE);

  /* The counter should start counting from 0x20000 and wrap around to 0x100
   * causeing an interrupt. Check we keep on counting. */
  waste_time();
  ASSERT (GET_TTCR () > 0x20000, "Timer started counting from high value");

  /* If TTCR is greater than TTMR_TP then the interrupt gets delivered after
     TTCR wraps around to 0 and counts to SPR_TTMR_TP.

     Set up an auto-restart timer to wrap around. Reset the tick count,
     because it may have incremented since the last match. */
  ttmr = clear_ttmr ();
  ttcr = 0xffffc00;
  SET_TTCR (ttcr);

  tick_cnt = 0;
  ttmr = new_timer (0x10000, SPR_TTMR_RT, SPR_TTMR_IE);

  /* Wait for the trigger, then check that we have restarted. */
  wait_match();
  ttcr = GET_TTCR ();

  ASSERT (ttcr < 0x10000, "Auto-restart wrapped round");

  /* Test wrap around in continuous mode. Reset the tick count in case we
     have had another interrupt. */
  ttmr = clear_ttmr ();
  ttcr = 0xffffc00;
  SET_TTCR (ttcr);

  tick_cnt = 0;
  ttmr = new_timer (0x10000, SPR_TTMR_CR, SPR_TTMR_IE);

  /* Wait for trigger, then check that we have carried on counting. */
  wait_match();
  ttcr = GET_TTCR () & SPR_TTCR_CNT;

  ASSERT ((0x00010000 < ttcr) && (ttcr < 0xfffffc00),
	  "Continuous mode wrapped round");

#if DO_SPURIOUS_INT_TEST==1

  /* Disable the timer and set up the special spurious interrupt handler, to
     check spurious interrupts occur as expected. */
  ttmr = clear_ttmr ();
  excpt_tick = (unsigned long)tick_int_spurious;

  /* Set up a disabled timer with a period of 0x100. */
  clear_ttcr ();
  ttmr = new_timer (0x100, SPR_TTMR_DI, SPR_TTMR_IE);

  /* Set up the count to match the period, and check that spurious interrupts
     are generated, even though the timer is disabled. */
  ttcr = 0x100;
  SET_TTCR (ttcr);

  while(tick_cnt != MAX_SPURIOUS)
    {
    }

  /* Check the count has not changed */
  ttcr = GET_TTCR ();
  ASSERT (0x100 == ttcr, "Spurious interrupts handled with matching period");

  /* Reset the tick count, then test setting TTCR first then TTMR */
  tick_cnt = 0;
  ttcr = 0x101;
  SET_TTCR (ttcr);
  ttmr = new_timer (0x101, SPR_TTMR_DI, SPR_TTMR_IE);

  while(tick_cnt != MAX_SPURIOUS)
    {
    }

  ttcr = GET_TTCR ();
  ASSERT (0x101 == ttcr, "Spurious interrupts handled after TTCR and TTMR");

  /* Set countinous counter, but make sure we never clear the TTMR_IP bit */
  tick_cnt = 0;
  clear_ip = 0;

  clear_ttcr ();
  ttmr = new_timer (0x100, SPR_TTMR_CR, SPR_TTMR_IE);

  while(tick_cnt != MAX_SPURIOUS)
    {
    }

#endif

  /* If we get here everything worked. */
  report(0xdeaddead);
  return 0;

}	/* main () */

