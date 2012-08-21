/* toplevel_support.h -- Top level simulator support header

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

/* Broken out from toplevel.c. Simulator commands. Help and version
   output. SIGINT processing.  Stdout redirection is specific to linux (I need
   to fix this). */


#ifndef TOPLEVEL_SUPPORT__H
#define TOPLEVEL_SUPPORT__H

/* Prototypes for external use */
extern void  init_randomness ();
extern void  ctrl_c (int  signum);
extern void  toggle_trace (int signum);
extern void  reg_sim_reset (void (*reset_hook) (void *), void *dat);
extern void  sim_done ();
extern void  check_int (void *dat);
extern void  sim_reset ();
extern void  sim_init ();

#endif	/* TOPLEVEL_SUPPORT__H */
