/* debug-unit.h -- Simulation of Or1k debug unit

   Copyright (C) 2001 Chris Ziomkowski, chris@asics.ws
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


#ifndef DEBUG_UNIT__H
#define DEBUG_UNIT__H


/*! The possible debug unit actions */
enum debug_unit_action
{
  DebugInstructionFetch = 1,
  DebugLoadAddress      = 2,
  DebugStoreAddress     = 3,
  DebugLoadData         = 4,
  DebugStoreData        = 5
};

/*! Enumeration of the various JTAG scan chains. Only those actually
    implemented are specified. */
enum debug_scan_chain_ids
{
  JTAG_CHAIN_GLOBAL      = 0,
  JTAG_CHAIN_DEBUG_UNIT  = 1,
  JTAG_CHAIN_TRACE       = 3,
  JTAG_CHAIN_DEVELOPMENT = 4,
  JTAG_CHAIN_WISHBONE    = 5,
};

/* Function prototypes for external use */
extern void  du_reset ();
extern void  set_stall_state (int state);
extern int   check_debug_unit (enum debug_unit_action  action,
			       unsigned long           udata);
extern int   debug_get_register (oraddr_t  address,
				 uorreg_t *data);
extern int   debug_set_register (oraddr_t  address,
				 uorreg_t  data);
extern int   debug_set_chain (enum debug_scan_chain_ids  chain);
extern int   debug_ignore_exception (unsigned long except);
extern void  reg_debug_sec ();

#endif	/*  DEBUG_UNIT__H */
