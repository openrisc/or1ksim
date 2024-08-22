/* or1ksim_trace.c -- Simulator trace for exporting to fusesoc

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

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <argp.h>
#include "or1ksim_trace.h"
#include "or32.h"

#define OPT_TRACE 515

static struct argp_option options[] = {
  { 0, 0, 0, 0, "OpenRISC instruction tracing:", 1 },
  { "trace_enable", OPT_TRACE, "VAL", OPTION_ARG_OPTIONAL, "Enable OpenRIRSC instruction tracing" },
  { 0 },
};

static bool trace_enable = false;

static int or1ksim_trace_parse_opts(int key, char *arg, struct argp_state *state)
{

  switch (key) {
  case OPT_TRACE:
    trace_enable = true;
    break;

  default:
    return ARGP_ERR_UNKNOWN;
  }

  return 0;
}

void or1ksim_trace (char *buf, size_t size, uint32_t insn)
{
  int insn_idx;

  or1ksim_build_automata (1);
  insn_idx = or1ksim_insn_decode (insn);
  or1ksim_disassemble_trace_index (insn, insn_idx);
  or1ksim_destruct_automata ();

  strncat (buf, or1ksim_disassembled, size);
}

int or1ksim_trace_dest_reg_get ()
{
  return trace_dest_reg;
}

int or1ksim_trace_src_reg_get ()
{
  return trace_src_reg;
}

int or1ksim_trace_dest_spr_get ()
{
  return trace_dest_spr;
}

unsigned int or1ksim_trace_store_addr_reg_get ()
{
  return trace_store_addr_reg;
}

int or1ksim_trace_store_imm_get ()
{
  return trace_store_imm;
}

int or1ksim_trace_store_val_reg_get ()
{
  return trace_store_val_reg;
}

int or1ksim_trace_store_width_get ()
{
  return trace_store_width;
}

bool or1ksim_trace_enable ()
{
  return trace_enable;
}

struct argp or1ksim_trace_argp = {options, or1ksim_trace_parse_opts, 0, 0};
