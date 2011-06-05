/* except.c -- Simulation of OR1K exceptions

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
#include "except.h"
#include "sim-config.h"
#include "arch.h"
#include "debug.h"
#include "spr-defs.h"
#include "execute.h"
#include "debug-unit.h"

extern void op_join_mem_cycles(void);



int except_pending = 0;

/* Asserts OR1K exception. */
/* WARNING: Don't expect except_handle to return.  Sometimes it _may_ return at
 * other times it may not. */
void
except_handle (oraddr_t except, oraddr_t ea)
{
  oraddr_t except_vector;

  if (debug_ignore_exception (except))
    return;

  /* In the dynamic recompiler, this function never returns, so this is not
   * needed.  Ofcourse we could set it anyway, but then all code that checks
   * this variable would break, since it is never reset */
  except_pending = 1;

  except_vector =
    except + (cpu_state.sprs[SPR_SR] & SPR_SR_EPH ? 0xf0000000 : 0x00000000);

  pcnext = except_vector;

  cpu_state.sprs[SPR_EEAR_BASE] = ea;
  cpu_state.sprs[SPR_ESR_BASE] = cpu_state.sprs[SPR_SR];

  cpu_state.sprs[SPR_SR] &= ~SPR_SR_OVE;	/* Disable overflow flag exception. */

  cpu_state.sprs[SPR_SR] |= SPR_SR_SM;	/* SUPV mode */
  cpu_state.sprs[SPR_SR] &= ~(SPR_SR_IEE | SPR_SR_TEE);	/* Disable interrupts. */

  /* Address translation is always disabled when starting exception. */
  cpu_state.sprs[SPR_SR] &= ~SPR_SR_DME;

  switch (except)
    {
      /* EPCR is irrelevent */
    case EXCEPT_RESET:
      break;
      /* EPCR is loaded with address of instruction that caused the exception */
    case EXCEPT_ITLBMISS:
    case EXCEPT_IPF:
      cpu_state.sprs[SPR_EPCR_BASE] = ea - (cpu_state.delay_insn ? 4 : 0);
      break;
    case EXCEPT_BUSERR:
    case EXCEPT_DPF:
    case EXCEPT_ALIGN:
    case EXCEPT_ILLEGAL:
    case EXCEPT_DTLBMISS:
    case EXCEPT_RANGE:
    case EXCEPT_TRAP:
      /* All these exceptions happen during a simulated instruction */
      cpu_state.sprs[SPR_EPCR_BASE] =
	cpu_state.pc - (cpu_state.delay_insn ? 4 : 0);
      break;
      /* EPCR is loaded with address of next not-yet-executed instruction */
    case EXCEPT_SYSCALL:
      cpu_state.sprs[SPR_EPCR_BASE] =
	(cpu_state.pc + 4) - (cpu_state.delay_insn ? 4 : 0);
      break;
      /* These exceptions happen AFTER (or before) an instruction has been
       * simulated, therefore the pc already points to the *next* instruction */
    case EXCEPT_TICK:
    case EXCEPT_INT:
      cpu_state.sprs[SPR_EPCR_BASE] =
	cpu_state.pc - (cpu_state.delay_insn ? 4 : 0);
      /* If we don't update the pc now, then it will only happen *after* the next
       * instruction (There would be serious problems if the next instruction just
       * happens to be a branch), when it should happen NOW. */
      cpu_state.pc = pcnext;
      pcnext += 4;
      break;
    }

  /* Address trnaslation is here because run_sched_out_of_line calls
   * eval_insn_direct which checks out the immu for the address translation but
   * if it would be disabled above then there would be not much point... */
  cpu_state.sprs[SPR_SR] &= ~SPR_SR_IME;

  /* Complex/simple execution strictly don't need this because of the
   * next_delay_insn thingy but in the dynamic execution modell that doesn't
   * exist and thus cpu_state.delay_insn would stick in the exception handler
   * causeing grief if the first instruction of the exception handler is also in
   * the delay slot of the previous instruction */
  cpu_state.delay_insn = 0;

}
