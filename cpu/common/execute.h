/* execute.h -- Header file for architecture dependent execute.c

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


#ifndef EXECUTE__H
#define EXECUTE__H

#include "spr-defs.h"
#include "opcode/or32.h"
#include "abstract.h"

#define CURINSN(INSN) (strcmp(cur->insn, (INSN)) == 0)

/*!The main structure holding the current execution state of the CPU

   Not to be confused with @c runtime, which holds the state of the
   simulation.

   @c insn_ea field is only used to get dump_exe_log() correct.

   @c iqueue and @c icomplet fields are only used in analysis().

   The micro-operation queue, @c opqs, is only used to speed up
   recompile_page().                                                         */
struct cpu_state {
  uorreg_t             reg[MAX_GPRS];	/*!< General purpose registers */
  uorreg_t             sprs[MAX_SPRS];	/*!< Special purpose registers */
  oraddr_t             insn_ea;		/*!< EA of instrs that have an EA */
  int                  delay_insn;	/*!< Is current instr in delay slot */
  int                  npc_not_valid;	/*!< NPC updated while stalled */
  oraddr_t             pc;		/*!< PC (and translated PC) */
  oraddr_t             pc_delay;	/*!< Delay instr EA register */
  struct iqueue_entry  iqueue;		/*!< Decode of just executed instr */
  struct iqueue_entry  icomplet;        /*!< Decode of instr before this */

};

/*! History of execution */
struct hist_exec
{
  oraddr_t addr;
  struct hist_exec *prev;
  struct hist_exec *next;
};

/* Globally visible data structures */
extern struct cpu_state  cpu_state;
extern oraddr_t          pcnext;
extern int               sbuf_wait_cyc;
extern int               sbuf_total_cyc;
extern int               do_stats;
extern struct hist_exec *hist_exec_tail;


/* Prototypes for external use */
extern void      dumpreg ();
extern void      trace_instr ();
extern void      dump_exe_log ();
extern void      dump_exe_bin_insn_log (struct iqueue_entry *current);

extern int       cpu_clock ();
extern void      cpu_reset ();
extern uorreg_t  evalsim_reg (unsigned int  regno);
extern void      setsim_reg (unsigned int  regno,
			     uorreg_t      value);
extern uorreg_t  eval_operand_val (uint32_t               insn,
				   struct insn_op_struct *opd);
extern void      analysis (struct iqueue_entry *current);
extern void      cpu_reset ();
extern int       cpu_clock ();
extern void      exec_main ();
extern int       depend_operands (struct iqueue_entry *prev,
				  struct iqueue_entry *next);
#endif  /* EXECUTE__H */
