/* branch-predict.h -- branch prediction header file

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

/* more or less useless at the moment */


#ifndef BRANCH_PREDICT__H
#define BRANCH_PREDICT__H

/* Package includes */
#include "arch.h"

/* Prototypes for external use */
extern void  bpb_info ();
extern void  bpb_update (oraddr_t addr, int taken);
extern void  btic_info ();
extern void  btic_update (oraddr_t targetaddr);
extern void  reg_bpb_sec ();

#endif	/* BRANCH_PREDICT__H */
