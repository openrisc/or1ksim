/* toplevel.c -- Top level simulator support source file

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

/* System includes */
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/* Package includes */
#include "toplevel-support.h"
#include "sim-config.h"
#include "debug-unit.h"
#include "sim-cmd.h"
#include "sched.h"
#include "tick.h"
#include "pm.h"
#include "pic.h"
#include "execute.h"
#include "labels.h"
#include "stats.h"
#include "opcode/or32.h"
#include "parse.h"
#include "rsp-server.h"
#include "vapi.h"
#include "abstract.h"
#include "mc.h"
#include "except.h"


/*! Struct for list of reset hooks */
struct sim_reset_hook
{
  void *dat;
  void (*reset_hook) (void *);
  struct sim_reset_hook *next;
};

/*! The list of reset hooks. Local to this source file */
static struct sim_reset_hook *sim_reset_hooks = NULL;


/*---------------------------------------------------------------------------*/
/*!Random number initialization

   This has become more important, since we rely on randomness to generate
   different MACs in Linux running on Or1ksim.

   We take our seed from /dev/urandom.

   If /dev/urandom is not available, we use srandom with the PID instead.    */
/*---------------------------------------------------------------------------*/
void
init_randomness ()
{
  unsigned int   seed;
  int            fd;

  fd = open ("/dev/urandom", O_RDONLY);

  if (fd >= 0)
    {
      if (sizeof (seed) != read (fd, (void *) &seed, sizeof (seed)))
	{
	  fprintf (stderr, "Warning: Unable to read /dev/random, using PID.\n");
	  seed = getpid ();
	}
    }
  else
    {
      fprintf (stderr, "Warning: Unable to open /dev/random, using PID.\n");
      seed = getpid ();
    }

  srandom (seed);

  /* Print out the seed just in case we ever need to debug. Note that we
     cannot use PRINTF here, since the file handle will not yet have been set
     up. */
  printf ("Seeding random generator with value 0x%08x\n", seed);

}	/* init_randomness () */


/*---------------------------------------------------------------------------*/
/*!Signal handler for ctrl-C

   Sets the iprompt flag, so the simulator will stop next time round the
   loop. If the iprompt flag is set when we enter here, that means the
   simulator has not reacted since the last ctrl-C, so we kill the simulation.

   @param[in] signum  The signal which triggered this handler                */
/*---------------------------------------------------------------------------*/
void
ctrl_c (signum)
     int signum;
{
  /* Incase the user pressed ctrl+c twice without the sim reacting kill it.
   * This is incase the sim locks up in a high level routine, without executeing
   * any (or) code */
  if (runtime.sim.iprompt && !runtime.sim.iprompt_run)
    {
      sim_done ();
      exit (-1);
    }

  /* Mark the simulator to stop next time and reinstall the handler */
  runtime.sim.iprompt = 1;
  signal (SIGINT, ctrl_c);

}	/* ctrl_c() */


/*---------------------------------------------------------------------------*/
/*!Signal handler for SIGUSR1

  Toggles state of trace generating while program is running.

   @param[in] signum  The signal which triggered this handler                */
/*---------------------------------------------------------------------------*/
void
toggle_trace (signum)
     int signum;
{

  runtime.sim.hush = !runtime.sim.hush;

  signal (SIGUSR1, toggle_trace);

}	/* toggle_trace() */


/*---------------------------------------------------------------------------*/
/*!Routine poll to see if interaction is needed

   This is most likely to happen due to a ctrl-C. However when the -i flag is
   specified, the simulator starts up ready for interaction.

   The main simulator loop will stop for interaction if it hits a breakpoint.

   @param[in] dat  Data passed in by the Or1ksim scheduler. Not needed by this
   function.                                                                 */
/*---------------------------------------------------------------------------*/
void
check_int (void *dat)
{
  if (runtime.sim.iprompt)
    {
      set_stall_state (0);
      handle_sim_command ();
    }

  SCHED_ADD (check_int, NULL, CHECK_INT_TIME);

}	/* check_int() */


/*---------------------------------------------------------------------------*/
/*!Register a new reset hook

   The registered functions will be called in turn, whenever the simulation is
   reset by calling sim_reset().

   @param[in] reset_hook  The function to be called on reset
   @param[in] dat         The data structure to be passed as argument when the
                          reset_hook function is called.                     */
/*---------------------------------------------------------------------------*/
void
reg_sim_reset (void (*reset_hook) (void *), void *dat)
{
  struct sim_reset_hook *new = malloc (sizeof (struct sim_reset_hook));

  if (!new)
    {
      fprintf (stderr, "reg_sim_reset: Out-of-memory\n");
      exit (1);
    }

  new->dat = dat;
  new->reset_hook = reset_hook;
  new->next = sim_reset_hooks;
  sim_reset_hooks = new;

}	/* reg_sim_reset() */


/*---------------------------------------------------------------------------*/
/*!Reset the simulator

   The scheduler is reset, then all reset functions on the reset hook list
   (i.e. peripherals) are reset. Then standard core functions (which do not
   use reset hooks) are reset: tick timer, power management, programmable
   interrupt controller and debug unit.

   The scheduler queue is reinitialized with an immediate check for ctrl-C on
   its queue.

   Finally the count of simulated cycles is set to zero. and the CPU itself is
   reset.                                                                    */
/*---------------------------------------------------------------------------*/
void
sim_reset ()
{
  struct sim_reset_hook *cur_reset = sim_reset_hooks;

  /* We absolutely MUST reset the scheduler first */
  sched_reset ();

  while (cur_reset)
    {
      cur_reset->reset_hook (cur_reset->dat);
      cur_reset = cur_reset->next;
    }

  tick_reset ();
  pm_reset ();
  pic_reset ();
  du_reset ();

  /* Make sure that runtime.sim.iprompt is the first thing to get checked */
  SCHED_ADD (check_int, NULL, 1);

  /* FIXME: Lame-ass way to get runtime.sim.mem_cycles not going into overly
   * negative numbers.  This happens because parse.c uses setsim_mem32 to load
   * the program but set_mem32 calls dc_simulate_write, which inturn calls
   * setsim_mem32.  This mess of memory statistics needs to be sorted out for
   * good one day */
  runtime.sim.mem_cycles = 0;
  cpu_reset ();

}	/* sim_reset() */


/*---------------------------------------------------------------------------*/
/*!Initalize the simulator

  Reset internal data: symbol table (aka labels), breakpoints and
  stats. Rebuild the FSA's used for disassembly.

  Initialize the dynamic execution system if required.

  Initialize the scheduler.

  Open the various logs and statistics files requested by the configuration
  and/or command arguments.

  Initialize GDB and VAPI connections.

  Reset the simulator.

  Wait for VAPI to connect if configured.                                    */
/*---------------------------------------------------------------------------*/
void
sim_init ()
{
  PRINTFQ ("Or1ksim " PACKAGE_VERSION "\n" );
  init_labels ();
  init_breakpoints ();
  initstats ();
  or1ksim_build_automata (config.sim.quiet);

#if DYNAMIC_EXECUTION
  /* Note: This must be called before the scheduler is used */
  init_dyn_recomp ();
#endif

  sched_init ();

  sim_reset ();			/* Must do this first - torches memory! */

  if (config.sim.profile)
    {
      runtime.sim.fprof = fopen (config.sim.prof_fn, "wt+");
      if (!runtime.sim.fprof)
	{
	  fprintf (stderr, "ERROR: sim_init: cannot open profile file %s: ",
		   config.sim.prof_fn);
	  perror (NULL);
	  exit (1);
	}
      else
	fprintf (runtime.sim.fprof,
		 "+00000000 FFFFFFFF FFFFFFFF [outside_functions]\n");
    }

  if (config.sim.mprofile)
    {
      runtime.sim.fmprof = fopen (config.sim.mprof_fn, "wb+");
      if (!runtime.sim.fmprof)
	{
	  fprintf (stderr, "ERROR: sim_init: cannot open memory profile "
		   "file %s: ", config.sim.mprof_fn);
	  perror (NULL);
	  exit (1);
	}
    }

  if (config.sim.exe_log)
    {
      runtime.sim.fexe_log = fopen (config.sim.exe_log_fn, "wt+");
      if (!runtime.sim.fexe_log)
	{
	  fprintf (stderr, "sim_init: cannot open execution log file %s: ",
		   config.sim.exe_log_fn);
	  perror (NULL);
	  exit (1);
	}
    }

  if (config.sim.exe_bin_insn_log)
    {
      runtime.sim.fexe_bin_insn_log = fopen (config.sim.exe_bin_insn_log_fn, "wb+");
      if (!runtime.sim.fexe_bin_insn_log)
	{
	  fprintf (stderr, "sim_init: cannot open binary instruction execution log file %s: ",
		   config.sim.exe_bin_insn_log_fn);
	  perror (NULL);
	  exit (1);
	}
    }

  /* MM170901 always load at address zero */
  if (runtime.sim.filename)
    {
      unsigned long endaddr = loadcode (runtime.sim.filename, 0, 0);

      if (endaddr == -1)
	{
	  fprintf (stderr, "ERROR: sim_init: problem loading code from %s\n",
		   runtime.sim.filename);
	  exit (1);
	}
    }

  /* Disable RSP debugging, if debug unit is not available.  */
  if (config.debug.rsp_enabled && !config.debug.enabled)
    {
      config.debug.rsp_enabled = 0;

      if (config.sim.verbose)
	{
	  fprintf (stderr, "WARNING: sim_init: Debug module not enabled, "
		   "cannot start remote service to GDB\n");
	}
    }

  /* Start either RSP or legacy GDB debugging service */
  if (config.debug.rsp_enabled)
    {
      rsp_init ();

      /* RSP always starts stalled as though we have just reset the
	 processor. */
      rsp_exception (EXCEPT_TRAP);
      set_stall_state (1);
    }

  /* Enable dependency stats, if we want to do history analisis */
  if (config.sim.history && !config.cpu.dependstats)
    {
      config.cpu.dependstats = 1;
      if (config.sim.verbose)
	{
	  fprintf (stderr, "WARNING: sim_init: dependstats stats must be "
		   "enabled to do history analysis\n");
	}
    }

  /* Debug forces verbose */
  if (config.sim.debug && !config.sim.verbose)
    {
      config.sim.verbose = 1;
      fprintf (stderr, "WARNING: sim_init: verbose forced on by debug\n");
    }

  /* Start VAPI before device initialization.  */
  if (config.vapi.enabled)
    {
      runtime.vapi.enabled = 1;
      vapi_init ();
      if (config.sim.verbose)
	{
	  PRINTF ("VAPI started, waiting for clients\n");
	}
    }

  /* Wait till all test are connected.  */
  if (runtime.vapi.enabled)
    {
      int numu = vapi_num_unconnected (0);
      if (numu)
	{
	  PRINTF ("\nWaiting for VAPI tests with ids:\n");
	  vapi_num_unconnected (1);
	  PRINTF ("\n");
	  while ((numu = vapi_num_unconnected (0)))
	    {
	      vapi_check ();
	      PRINTF
		("\rStill waiting for %i VAPI test(s) to connect",
		 numu);
	      usleep (100);
	    }
	  PRINTF ("\n");
	}
      PRINTF ("All devices connected\n");
    }
}	/* sim_init() */


/*---------------------------------------------------------------------------*/
/*!Clean up

   Close an profile or log files, disconnect VAPI. Call any memory mapped
   peripheral close down function. Exit with rc 0.                           */
/*---------------------------------------------------------------------------*/
void
sim_done ()
{
  if (config.sim.profile)
    {
      fprintf (runtime.sim.fprof, "-%08llX FFFFFFFF\n", runtime.sim.cycles);
      fclose (runtime.sim.fprof);
    }

  if (config.sim.mprofile)
    {
      fclose (runtime.sim.fmprof);
    }

  if (config.sim.exe_log)
    {
      fclose (runtime.sim.fexe_log);
    }

  if (config.sim.exe_bin_insn_log)
    {
      fclose (runtime.sim.fexe_bin_insn_log);
    }
  
  if (runtime.vapi.enabled)
    {
      vapi_done ();
    }

  done_memory_table ();
  mc_done ();

  exit (0);

}	/* sim_done() */
