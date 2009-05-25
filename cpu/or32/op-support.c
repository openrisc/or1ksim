/* op-support.c -- Support routines for micro operations
   Copyright (C) 2005 György `nog' Jeney, nog@sdf.lonestar.org

This file is part of OpenRISC 1000 Architectural Simulator.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA. */


#include <stdlib.h>

#include "config.h"

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#include "port.h"
#include "arch.h"
#include "opcode/or32.h"
#include "sim-config.h"
#include "spr-defs.h"
#include "except.h"
#include "immu.h"
#include "abstract.h"
#include "execute.h"
#include "sched.h"
#include "i386-regs.h"
#include "dyn-rec.h"
#include "op-support.h"
#include "simprintf.h"


/* Stuff that is really a `micro' operation but is rather big (or for some other
 * reason like calling exit()) */

void op_support_nop_exit(void)
{
  PRINTF("exit(%"PRIdREG")\n", cpu_state.reg[3]);
  fprintf(stderr, "@reset : cycles %lld, insn #%lld\n",
          runtime.sim.reset_cycles, runtime.cpu.reset_instructions);
  fprintf(stderr, "@exit  : cycles %lld, insn #%lld\n", runtime.sim.cycles,
          runtime.cpu.instructions);
  fprintf(stderr, " diff  : cycles %lld, insn #%lld\n",
          runtime.sim.cycles - runtime.sim.reset_cycles,
          runtime.cpu.instructions - runtime.cpu.reset_instructions);
  /* FIXME: Implement emulation of a stalled cpu
  if (config.debug.gdb_enabled)
    set_stall_state (1);
  else {
    handle_sim_command();
    sim_done();
  }
  */
  exit(0);
}

void op_support_nop_reset(void)
{
  PRINTF("****************** counters reset ******************\n");
  PRINTF("cycles %lld, insn #%lld\n", runtime.sim.cycles, runtime.cpu.instructions);
  PRINTF("****************** counters reset ******************\n");
  runtime.sim.reset_cycles = runtime.sim.cycles;
  runtime.cpu.reset_instructions = runtime.cpu.instructions;
}

void op_support_nop_printf(void)
{
  simprintf(cpu_state.reg[4], cpu_state.reg[3]);
}

void op_support_nop_report(void)
{
  PRINTF("report(0x%"PRIxREG");\n", cpu_state.reg[3]);
}

void op_support_nop_report_imm(int imm)
{
  PRINTF("report %i (0x%"PRIxREG");\n", imm, cpu_state.reg[3]);
}

/* Handles a jump */
/* addr is a VIRTUAL address */
/* NOTE: We can't use env since this code is compiled like the rest of the
 * simulator (most likely without -fomit-frame-pointer) and thus env will point
 * to some bogus value. */
void do_jump(oraddr_t addr)
{
  cpu_state.pc = addr;
  longjmp(cpu_state.excpt_loc, 0);
}

/* Wrapper around analysis() that contains all the recompiler specific stuff */
void op_support_analysis(void)
{
  oraddr_t off = (cpu_state.pc & immu_state->page_offset_mask) >> 2;
  runtime.cpu.instructions++;
  cpu_state.iqueue.insn_index = cpu_state.curr_page->insn_indexs[off];
  cpu_state.iqueue.insn = cpu_state.curr_page->insns[off];
  cpu_state.iqueue.insn_addr = cpu_state.pc;
  analysis(&cpu_state.iqueue);
}

