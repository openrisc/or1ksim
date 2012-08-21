/* toplevel.c -- Top level simulator source file

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
#include <signal.h>

/* Package includes */
#include "sim-config.h"
#include "toplevel-support.h"
#include "execute.h"


/*---------------------------------------------------------------------------*/
/*!Main function

   Set up the standalone simulation. Initialize the default configuration and
   register all the sections that may appear in a user configuration.

   Then attempt to parse the args, configure the system from any configuration
   file specified and print out the configuration used.

   Add a signal hander, so ctrl-C will drop the user into the CLI.

   The initialize the simulator, call the appropriate main simulator function
   and when it returns tidy up.

   @param[in] argc  The number of arguments to the command
   @param[in] argv  The vector of argument strings

   @return  The return code required from the simulator. This is actually
            achieved by calling exit() with the return code, rather than
            returning an explict value.                                      */
/*---------------------------------------------------------------------------*/
int
main (int   argc,
      char *argv[])
{
  init_randomness ();
  init_defconfig ();
  reg_config_secs ();

  if (parse_args (argc, argv))
    {
      exit (-1);		/* Parse args will have printed any messages */
    }

  print_config ();
  signal (SIGINT, ctrl_c);
  signal (SIGUSR1, toggle_trace);

  do_stats         = config.cpu.superscalar ||
                     config.cpu.dependstats ||
                     config.sim.history     ||
                     config.sim.exe_log     ||
                     config.sim.exe_bin_insn_log;

  sim_init ();

#if DYNAMIC_EXECUTION
  dyn_main ();
#else
  exec_main ();
#endif

  sim_done ();
  exit (0);
}
