/* execute-fp.h -- OR1K floating point functions for simulation

   Copyright (C) 2010 ORSoC AB
  
   Contributor Julius Baxter <julius.baxter@orsoc.se>
  
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
   with this program.  If not, see <http://www.gnu.org/licenses/>. */

/* Functions to configure the host machine's FPU before doing simulated OR1k
   FP ops, handle flags and restore things when it's done. One for before and
   one for afterwards.
*/

/* A place to store the host's FPU rounding mode while OR1k's is used */
extern int host_fp_rm;

void fp_set_or1k_rm(void);
void fp_set_flags_restore_host_rm(void);
