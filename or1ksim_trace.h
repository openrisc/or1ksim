/* or1ksim_trace.h -- Simulator trace API for exporting to fusesoc

   Copyright (C) 2024 Stafford Horne <shorne@gmail.com>

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

#ifndef OR1KSIM_TRACE_H
#define OR1KSIM_TRACE_H

#include <argp.h>

/* Trace and instruction placing it's details into BUF.  */
void or1ksim_trace (char *buf, size_t size, uint32_t insn);

/* Details on last traced instruction.  */
int or1ksim_trace_dest_reg_get ();
/* Source register used for mtspr.  */
int or1ksim_trace_src_reg_get ();
int or1ksim_trace_dest_spr_get ();
unsigned int or1ksim_trace_store_addr_reg_get ();
int or1ksim_trace_store_imm_get ();
int or1ksim_trace_store_val_reg_get ();
int or1ksim_trace_store_width_get ();

/* Command line parsing for integrating into a verlator test bench.  */
bool or1ksim_trace_enable ();

extern struct argp or1ksim_trace_argp;

#endif /* OR1KSIM_TRACE_H */
