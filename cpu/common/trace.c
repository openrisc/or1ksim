/* trace.c -- Simulator breakpoints

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

/* Package includes */
#include "trace.h"
#include "sim-config.h"
#include "abstract.h"
#include "labels.h"


/*---------------------------------------------------------------------------*/
/*!Set instruction execution breakpoint

   @param[in] addr  The address for the breakpoint                           */
/*---------------------------------------------------------------------------*/
void
set_insnbrkpoint (oraddr_t addr)
{
  addr &= ~ADDR_C (3);		/* 32-bit aligned */

  if (!verify_memoryarea (addr))
    {
      PRINTF
	("WARNING: This breakpoint is out of the simulated memory range.\n");
    }

  if (has_breakpoint (addr))
    {
      remove_breakpoint (addr);
      PRINTF ("\nBreakpoint at 0x%" PRIxADDR " cleared.\n", addr);
    }
  else
    {
      add_breakpoint (addr);
      PRINTF ("\nBreakpoint at 0x%" PRIxADDR " set.\n", addr);
    }

  return;

}	/* set_insnbrkpoint () */
