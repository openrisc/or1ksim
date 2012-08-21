/* execute.c -- OR1K architecture dependent simulation

   Copyright (C) 1999 Damjan Lampret, lampret@opencores.org
   Copyright (C) 2005 György `nog' Jeney, nog@sdf.lonestar.org
   Copyright (C) 2008 Embecosm Limited
   Copyright (C) 2010 ORSoC AB
  
   Contributor Jeremy Bennett <jeremy.bennett@embecosm.com>
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

/* This program is commented throughout in a fashion suitable for processing
   with Doxygen. */


/* Most of the OR1K simulation is done in here.

   When SIMPLE_EXECUTION is defined below a file insnset.c is included!
*/

/* Autoconf and/or portability configuration */
#include "config.h"
#include "port.h"

/* System includes */
#include <stdlib.h>

/* Package includes */
#include "execute.h"
#include "toplevel-support.h"
#include "except.h"
#include "labels.h"
#include "sched.h"
#include "stats.h"
#include "opcode/or32.h"
#include "dmmu.h"
#include "immu.h"
#include "sim-cmd.h"
#include "vapi.h"
#include "debug-unit.h"
#include "branch-predict.h"
#include "sprs.h"
#include "rsp-server.h"
#include "softfloat.h"

/* Includes and macros for simple execution */
#if SIMPLE_EXECUTION

#define SET_PARAM0(val) set_operand(0, val, current->insn_index, current->insn)

#define PARAM0 eval_operand(0, current->insn_index, current->insn)
#define PARAM1 eval_operand(1, current->insn_index, current->insn)
#define PARAM2 eval_operand(2, current->insn_index, current->insn)

#define INSTRUCTION(name) void name (struct iqueue_entry *current)

#endif	/* SIMPLE_EXECUTION */


/*! Current cpu state. Globally available. */
struct cpu_state  cpu_state;

/*! Temporary program counter. Globally available */
oraddr_t  pcnext;

/*! Num cycles waiting for stores to complete. Globally available */
int  sbuf_wait_cyc = 0;

/*! Number of total store cycles. Globally available */
int  sbuf_total_cyc = 0;

/*! Whether we are doing statistical analysis. Globally available */
int  do_stats = 0;

/*! History of execution. Globally available */
struct hist_exec *hist_exec_tail = NULL;

/* Benchmark multi issue execution. This file only */
static int  multissue[20];
static int  issued_per_cycle = 4;

/* Store buffer analysis - stores are accumulated and commited when IO is
   idle. This file only */
static int  sbuf_head              = 0;
static int  sbuf_tail              = 0;
static int  sbuf_count             = 0;
static int  sbuf_buf[MAX_SBUF_LEN] = { 0 };

static int sbuf_prev_cycles = 0;

/* Variables used throughout this file to share information */
static int  breakpoint;
static int  next_delay_insn;

/* Forward declaration of static functions */
static void decode_execute (struct iqueue_entry *current);

/*---------------------------------------------------------------------------*/
/*!Get an actual value of a specific register

   Implementation specific. Abort if we are given a duff register. Only used
   externally to support simprintf(), which is now obsolete.

   @param[in] regno  The register of interest

   @return  The value of the register                                        */
/*---------------------------------------------------------------------------*/
uorreg_t
evalsim_reg (unsigned int  regno)
{
  if (regno < MAX_GPRS)
    {
#if RAW_RANGE_STATS
      int delta = (runtime.sim.cycles - raw_stats.reg[regno]);

      if ((unsigned long) delta < (unsigned long) RAW_RANGE)
	{
	  raw_stats.range[delta]++;
	}
#endif /* RAW_RANGE */

      return cpu_state.reg[regno];
    }
  else
    {
      PRINTF ("\nABORT: read out of registers\n");
      sim_done ();
      return 0;
    }
}	/* evalsim_reg() */


/*---------------------------------------------------------------------------*/
/*!Set a specific register with value

   Implementation specific. Abort if we are given a duff register.

   @param[in] regno  The register of interest
   @param[in] value  The value to be set                                     */
/*---------------------------------------------------------------------------*/
void
setsim_reg (unsigned int  regno,
	    uorreg_t      value)
{
  if (regno == 0)		/* gpr0 is always zero */
    {
      value = 0;
    }

  if (regno < MAX_GPRS)
    {
      cpu_state.reg[regno] = value;
    }
  else
    {
      PRINTF ("\nABORT: write out of registers\n");
      sim_done ();
    }

#if RAW_RANGE_STATS
  raw_stats.reg[regno] = runtime.sim.cycles;
#endif /* RAW_RANGE */

}	/* setsim_reg() */


/*---------------------------------------------------------------------------*/
/*!Evaluates source operand operand

   Implementation specific. Declared global, although this is only actually
   required for DYNAMIC_EXECUTION,

   @param[in] insn  The instruction
   @param[in] opd   The operand

   @return  The value of the source operand                                  */
/*---------------------------------------------------------------------------*/
uorreg_t
eval_operand_val (uint32_t               insn,
		  struct insn_op_struct *opd)
{
  unsigned long  operand = 0;
  unsigned long  sbit;
  unsigned int   nbits = 0;

  while (1)
    {
      operand |=
	((insn >> (opd->type & OPTYPE_SHR)) & ((1 << opd->data) - 1)) <<
	nbits;
      nbits += opd->data;

      if (opd->type & OPTYPE_OP)
	{
	  break;
	}

      opd++;
    }

  if (opd->type & OPTYPE_SIG)
    {
      sbit = (opd->type & OPTYPE_SBIT) >> OPTYPE_SBIT_SHR;

      if (operand & (1 << sbit))
	{
	  operand |= ~REG_C (0) << sbit;
	}
    }

  return operand;

}	/* eval_operand_val() */


/*---------------------------------------------------------------------------*/
/*!Does source operand depend on computation of dest operand?

      Cycle t                 Cycle t+1
  dst: irrelevant         src: immediate                  always 0
  dst: reg1 direct        src: reg2 direct                0 if reg1 != reg2
  dst: reg1 disp          src: reg2 direct                always 0
  dst: reg1 direct        src: reg2 disp                  0 if reg1 != reg2
  dst: reg1 disp          src: reg2 disp                  always 1 (store must
                                                          finish before load)
  dst: flag               src: flag                       always 1

  @param[in] prev  Previous instruction
  @param[in] next  Next instruction

  @return  Non-zero if yes.                                                  */
/*---------------------------------------------------------------------------*/
static int
check_depend (struct iqueue_entry *prev,
	      struct iqueue_entry *next)
{
  /* Find destination type. */
  unsigned long          type = 0;
  int                    prev_dis;
  int                    next_dis;
  orreg_t                prev_reg_val = 0;
  struct insn_op_struct *opd;

  if (or1ksim_or32_opcodes[prev->insn_index].flags & OR32_W_FLAG
      && or1ksim_or32_opcodes[next->insn_index].flags & OR32_R_FLAG)
    {
      return  1;
    }

  opd      = or1ksim_op_start[prev->insn_index];
  prev_dis = 0;

  while (1)
    {
      if (opd->type & OPTYPE_DIS)
	{
	  prev_dis = 1;
	}

      if (opd->type & OPTYPE_DST)
	{
	  type = opd->type;

	  if (prev_dis)
	    {
	      type |= OPTYPE_DIS;
	    }

	  /* Destination is always a register */
	  prev_reg_val = eval_operand_val (prev->insn, opd);
	  break;
	}

      if (opd->type & OPTYPE_LAST)
	{
	  return 0;		/* Doesn't have a destination operand */
	}

      if (opd->type & OPTYPE_OP)
	{
	  prev_dis = 0;
	}

      opd++;
    }

  /* We search all source operands - if we find confict => return 1 */
  opd      = or1ksim_op_start[next->insn_index];
  next_dis = 0;

  while (1)
    {
      if (opd->type & OPTYPE_DIS)
	{
	  next_dis = 1;
	}

      /* This instruction sequence also depends on order of execution:
           l.lw r1, k(r1)
           l.sw k(r1), r4
         Here r1 is a destination in l.sw */
   
      /* FIXME: This situation is not handeld here when r1 == r2:
           l.sw k(r1), r4
           l.lw r3, k(r2) */
      if (!(opd->type & OPTYPE_DST) || (next_dis && (opd->type & OPTYPE_DST)))
	{
	  if (opd->type & OPTYPE_REG)
	    {
	      if (eval_operand_val (next->insn, opd) == prev_reg_val)
		{
		  return 1;
		}
	    }
	}

      if (opd->type & OPTYPE_LAST)
	{
	  break;
	}

      opd++;
    }

  return  0;

}	/* check_depend() */


/*---------------------------------------------------------------------------*/
/*!Should instruction NOT be executed?

   Modified by CZ 26/05/01 for new mode execution.

   @return  Nonzero if instruction should NOT be executed                    */
/*---------------------------------------------------------------------------*/
static int
fetch ()
{
  static int break_just_hit = 0;

  if (NULL != breakpoints)
    {
      /* MM: Check for breakpoint.  This has to be done in fetch cycle,
         because of peripheria.  
         MM1709: if we cannot access the memory entry, we could not set the
         breakpoint earlier, so just check the breakpoint list.  */
      if (has_breakpoint (peek_into_itlb (cpu_state.pc)) && !break_just_hit)
	{
	  break_just_hit = 1;
	  return 1;		/* Breakpoint set. */
	}
      break_just_hit = 0;
    }

  breakpoint                 = 0;
  cpu_state.iqueue.insn_addr = cpu_state.pc;
  cpu_state.iqueue.insn      = eval_insn (cpu_state.pc, &breakpoint);

  /* Fetch instruction. */
  if (!except_pending)
    {
      runtime.cpu.instructions++;
    }

  /* update_pc will be called after execution */
  return 0;

}	/* fetch() */


/*---------------------------------------------------------------------------*/
/*!This code actually updates the PC value                                   */
/*---------------------------------------------------------------------------*/
static void
update_pc ()
{
  cpu_state.delay_insn    = next_delay_insn;
  cpu_state.sprs[SPR_PPC] = cpu_state.pc;	/* Store value for later */
  cpu_state.pc            = pcnext;
  pcnext                  = cpu_state.delay_insn ? cpu_state.pc_delay :
                                                   pcnext + 4;
}	/* update_pc() */


/*---------------------------------------------------------------------------*/
/*!Perform analysis of the instruction being executed

   This could be static for SIMPLE_EXECUTION, but made global for general use.

   @param[in] current  The instruction being executed                        */
/*---------------------------------------------------------------------------*/
void
analysis (struct iqueue_entry *current)
{
  if (config.cpu.dependstats)
    {
      /* Dynamic, dependency stats. */
      adddstats (cpu_state.icomplet.insn_index, current->insn_index, 1,
		 check_depend (&cpu_state.icomplet, current));

      /* Dynamic, functional units stats. */
      addfstats (or1ksim_or32_opcodes[cpu_state.icomplet.insn_index].func_unit,
		 or1ksim_or32_opcodes[current->insn_index].func_unit, 1,
		 check_depend (&cpu_state.icomplet, current));

      /* Dynamic, single stats. */
      addsstats (current->insn_index, 1);
    }

  if (config.cpu.superscalar)
    {
      if ((or1ksim_or32_opcodes[current->insn_index].func_unit == it_branch) ||
	  (or1ksim_or32_opcodes[current->insn_index].func_unit == it_jump))
	runtime.sim.storecycles += 0;

      if (or1ksim_or32_opcodes[current->insn_index].func_unit == it_store)
	runtime.sim.storecycles += 1;

      if (or1ksim_or32_opcodes[current->insn_index].func_unit == it_load)
	runtime.sim.loadcycles += 1;

      /* Pseudo multiple issue benchmark */
      if ((multissue[or1ksim_or32_opcodes[current->insn_index].func_unit] < 1) ||
	  (check_depend (&cpu_state.icomplet, current))
	  || (issued_per_cycle < 1))
	{
	  int i;
	  for (i = 0; i < 20; i++)
	    multissue[i] = 2;
	  issued_per_cycle = 2;
	  runtime.cpu.supercycles++;
	  if (check_depend (&cpu_state.icomplet, current))
	    runtime.cpu.hazardwait++;
	  multissue[it_unknown] = 2;
	  multissue[it_shift] = 2;
	  multissue[it_compare] = 1;
	  multissue[it_branch] = 1;
	  multissue[it_jump] = 1;
	  multissue[it_extend] = 2;
	  multissue[it_nop] = 2;
	  multissue[it_move] = 2;
	  multissue[it_movimm] = 2;
	  multissue[it_arith] = 2;
	  multissue[it_store] = 2;
	  multissue[it_load] = 2;
	}
      multissue[or1ksim_or32_opcodes[current->insn_index].func_unit]--;
      issued_per_cycle--;
    }

  if (config.cpu.dependstats)
    /* Instruction waits in completition buffer until retired. */
    memcpy (&cpu_state.icomplet, current, sizeof (struct iqueue_entry));

  if (config.sim.history)
    {
      /* History of execution */
      hist_exec_tail = hist_exec_tail->next;
      hist_exec_tail->addr = cpu_state.icomplet.insn_addr;
    }

  if (config.sim.exe_log)
    dump_exe_log ();

  if (config.sim.exe_bin_insn_log)
    dump_exe_bin_insn_log (current);

}	/* analysis() */


/*---------------------------------------------------------------------------*/
/*!Store buffer analysis for store instructions

   Stores are accumulated and commited when IO is idle

   @param[in] cyc  Number of cycles being analysed                           */
/*---------------------------------------------------------------------------*/
static void
sbuf_store (int cyc)
{
  int delta = runtime.sim.cycles - sbuf_prev_cycles;

  sbuf_total_cyc   += cyc;
  sbuf_prev_cycles  = runtime.sim.cycles;

  /* Take stores from buffer, that occured meanwhile */
  while (sbuf_count && delta >= sbuf_buf[sbuf_tail])
    {
      delta     -= sbuf_buf[sbuf_tail];
      sbuf_tail  = (sbuf_tail + 1) % MAX_SBUF_LEN;
      sbuf_count--;
    }

  if (sbuf_count)
    {
      sbuf_buf[sbuf_tail] -= delta;
    }

  /* Store buffer is full, take one out */
  if (sbuf_count >= config.cpu.sbuf_len)
    {
      sbuf_wait_cyc          += sbuf_buf[sbuf_tail];
      runtime.sim.mem_cycles += sbuf_buf[sbuf_tail];
      sbuf_prev_cycles       += sbuf_buf[sbuf_tail];
      sbuf_tail               = (sbuf_tail + 1) % MAX_SBUF_LEN;
      sbuf_count--;
    }

  /* Put newest store in the buffer */
  sbuf_buf[sbuf_head] = cyc;
  sbuf_head           = (sbuf_head + 1) % MAX_SBUF_LEN;
  sbuf_count++;

}	/* sbuf_store() */


/*---------------------------------------------------------------------------*/
/*!Store buffer analysis for load instructions

   Previous stores should commit, before any load                            */
/*---------------------------------------------------------------------------*/
static void
sbuf_load ()
{
  int delta = runtime.sim.cycles - sbuf_prev_cycles;
  sbuf_prev_cycles = runtime.sim.cycles;

  /* Take stores from buffer, that occured meanwhile */
  while (sbuf_count && delta >= sbuf_buf[sbuf_tail])
    {
      delta     -= sbuf_buf[sbuf_tail];
      sbuf_tail  = (sbuf_tail + 1) % MAX_SBUF_LEN;
      sbuf_count--;
    }

  if (sbuf_count)
    {
      sbuf_buf[sbuf_tail] -= delta;
    }

  /* Wait for all stores to complete */
  while (sbuf_count > 0)
    {
      sbuf_wait_cyc          += sbuf_buf[sbuf_tail];
      runtime.sim.mem_cycles += sbuf_buf[sbuf_tail];
      sbuf_prev_cycles       += sbuf_buf[sbuf_tail];
      sbuf_tail               = (sbuf_tail + 1) % MAX_SBUF_LEN;
      sbuf_count--;
    }
}	/* sbuf_load() */

/*---------------------------------------------------------------------------*/
/*!Outputs dissasembled instruction                                          */
/*---------------------------------------------------------------------------*/
void
dump_exe_log ()
{
  oraddr_t      insn_addr = cpu_state.iqueue.insn_addr;
  unsigned int  i;
  unsigned int  j;
  uorreg_t      operand;

  if (insn_addr == 0xffffffff)
    {
      return;
    }

  if ((config.sim.exe_log_start <= runtime.cpu.instructions) &&
      ((config.sim.exe_log_end <= 0) ||
       (runtime.cpu.instructions <= config.sim.exe_log_end)))
    {
      struct label_entry *entry;

      if (config.sim.exe_log_marker &&
	  !(runtime.cpu.instructions % config.sim.exe_log_marker))
	{
	  fprintf (runtime.sim.fexe_log,
		   "--------------------- %8lli instruction "
		   "---------------------\n",
		   runtime.cpu.instructions);
	}

      switch (config.sim.exe_log_type)
	{
	case EXE_LOG_HARDWARE:
	  fprintf (runtime.sim.fexe_log,
		   "\nEXECUTED(%11llu): %" PRIxADDR ":  ",
		   runtime.cpu.instructions, insn_addr);
	  fprintf (runtime.sim.fexe_log, "%.2x%.2x",
		   eval_direct8 (insn_addr, 0, 0),
		   eval_direct8 (insn_addr + 1, 0, 0));
	  fprintf (runtime.sim.fexe_log, "%.2x%.2x",
		   eval_direct8 (insn_addr + 2, 0, 0),
		   eval_direct8 (insn_addr + 3, 0, 0));

	  for (i = 0; i < MAX_GPRS; i++)
	    {
	      if (i % 4 == 0)
		{
		  fprintf (runtime.sim.fexe_log, "\n");
		}

	      fprintf (runtime.sim.fexe_log, "GPR%2u: %" PRIxREG "  ", i,
		       cpu_state.reg[i]);
	    }

	  fprintf (runtime.sim.fexe_log, "\n");
	  fprintf (runtime.sim.fexe_log, "SR   : %.8" PRIx32 "  ",
		   cpu_state.sprs[SPR_SR]);
	  fprintf (runtime.sim.fexe_log, "EPCR0: %" PRIxADDR "  ",
		   cpu_state.sprs[SPR_EPCR_BASE]);
	  fprintf (runtime.sim.fexe_log, "EEAR0: %" PRIxADDR "  ",
		   cpu_state.sprs[SPR_EEAR_BASE]);
	  fprintf (runtime.sim.fexe_log, "ESR0 : %.8" PRIx32 "\n",
		   cpu_state.sprs[SPR_ESR_BASE]);
	  break;

	case EXE_LOG_SIMPLE:
	case EXE_LOG_SOFTWARE:
	  or1ksim_disassemble_index (cpu_state.iqueue.insn,
			     cpu_state.iqueue.insn_index);

	  entry = get_label (insn_addr);
	  if (entry)
	    {
	      fprintf (runtime.sim.fexe_log, "%s:\n", entry->name);
	    }
	  
	  if (config.sim.exe_log_type == EXE_LOG_SOFTWARE)
	    {
	      struct insn_op_struct *opd =
		or1ksim_op_start[cpu_state.iqueue.insn_index];

	      j = 0;
	      while (1)
		{
		  operand = eval_operand_val (cpu_state.iqueue.insn, opd);
		  while (!(opd->type & OPTYPE_OP))
		    {
		      opd++;
		    }
		  if (opd->type & OPTYPE_DIS)
		    {
		      fprintf (runtime.sim.fexe_log,
			       "EA =%" PRIxADDR " PA =%" PRIxADDR " ",
			       cpu_state.insn_ea,
			       peek_into_dtlb (cpu_state.insn_ea, 0, 0));
		      opd++;	/* Skip of register operand */
		      j++;
		    }
		  else if ((opd->type & OPTYPE_REG) && operand)
		    {
		      fprintf (runtime.sim.fexe_log, "r%-2i=%" PRIxREG " ",
			       (int) operand, evalsim_reg (operand));
		    }
		  else
		    {
		      fprintf (runtime.sim.fexe_log, "             ");
		    }
		  j++;
		  if (opd->type & OPTYPE_LAST)
		    {
		      break;
		    }
		  opd++;
		}
	      if (or1ksim_or32_opcodes[cpu_state.iqueue.insn_index].flags & OR32_R_FLAG)
		{
		  fprintf (runtime.sim.fexe_log, "SR =%" PRIxREG " ",
			   cpu_state.sprs[SPR_SR]);
		  j++;
		}
	      while (j < 3)
		{
		  fprintf (runtime.sim.fexe_log, "             ");
		  j++;
		}
	    }
	  fprintf (runtime.sim.fexe_log, "%" PRIxADDR " ", insn_addr);
	  fprintf (runtime.sim.fexe_log, "%s\n", or1ksim_disassembled);
	}
    }
}	/* dump_exe_log() */



/*---------------------------------------------------------------------------*/
/*!Outputs binary copy of instruction to a file                              */
/*---------------------------------------------------------------------------*/
void
dump_exe_bin_insn_log (struct iqueue_entry *current)
{
  // Do endian swap before spitting out (will be kept in LE on a LE machine)
  // but more useful to see it in big endian format.
  // Should probably host htonl().
  uint32_t insn = (((current->insn & 0xff)<<24) | 
		   ((current->insn & 0xff00)<<8) | 
		   ((current->insn & 0xff0000)>>8) | 
		   ((current->insn & 0xff000000)>>24));
  
  //for(i=0;i<4;i++) tmp_insn[i] = eval_direct8 (insn_addr+i, 0, 0);
  
  // Dump it into binary log file
  fwrite((void*)&insn, 4, 1, runtime.sim.fexe_bin_insn_log);
    

}	/* dump_exe_bin_insn_log() */


/*---------------------------------------------------------------------------*/
/*!Dump registers

   Supports the CLI 'r' and 't' commands                                     */
/*---------------------------------------------------------------------------*/
void
dumpreg ()
{
  int       i;
  oraddr_t  physical_pc;

  if ((physical_pc = peek_into_itlb (cpu_state.iqueue.insn_addr)))
    {
      disassemble_memory (physical_pc, physical_pc + 4, 0);
    }
  else
    {
      PRINTF ("INTERNAL SIMULATOR ERROR:\n");
      PRINTF ("no translation for currently executed instruction\n");
    }

  // generate_time_pretty (temp, runtime.sim.cycles * config.sim.clkcycle_ps);
  PRINTF (" (executed) [cycle %lld, #%lld]\n", runtime.sim.cycles,
	  runtime.cpu.instructions);
  if (config.cpu.superscalar)
    {
      PRINTF ("Superscalar CYCLES: %u", runtime.cpu.supercycles);
    }
  if (config.cpu.hazards)
    {
      PRINTF ("  HAZARDWAIT: %u\n", runtime.cpu.hazardwait);
    }
  else if (config.cpu.superscalar)
    {
      PRINTF ("\n");
    }

  if ((physical_pc = peek_into_itlb (cpu_state.pc)))
    {
      disassemble_memory (physical_pc, physical_pc + 4, 0);
    }
  else
    {
      PRINTF ("%" PRIxADDR ": : xxxxxxxx  ITLB miss follows", cpu_state.pc);
    }

  PRINTF (" (next insn) %s", (cpu_state.delay_insn ? "(delay insn)" : ""));

  for (i = 0; i < MAX_GPRS; i++)
    {
      if (i % 4 == 0)
	{
	  PRINTF ("\n");
	}

      PRINTF ("GPR%.2u: %" PRIxREG "  ", i, evalsim_reg (i));
    }

  PRINTF ("flag: %u\n", cpu_state.sprs[SPR_SR] & SPR_SR_F ? 1 : 0);

}	/* dumpreg() */


/*---------------------------------------------------------------------------*/
/*!Trace an instruction

   Supports GDB tracing                                                      */
/*---------------------------------------------------------------------------*/
void
trace_instr ()
{
  oraddr_t  physical_pc;

  if ((physical_pc = peek_into_itlb (cpu_state.iqueue.insn_addr)))
    {
      disassemble_instr (physical_pc, cpu_state.iqueue.insn_addr,
			 cpu_state.iqueue.insn);
    }
  else
    {
      PRINTF ("Instruction address translation failed: no trace available\n");
    }
}	/* trace_instr () */


/*---------------------------------------------------------------------------*/
/*!Wrapper around real decode_execute function

   Some statistics here only

   @param[in] current  Instruction being executed                            */
/*---------------------------------------------------------------------------*/
static void
decode_execute_wrapper (struct iqueue_entry *current)
{
  breakpoint = 0;

#ifndef HAVE_EXECUTION
#error HAVE_EXECUTION has to be defined in order to execute programs.
#endif

  decode_execute (current);

  if (breakpoint)
    {
      except_handle (EXCEPT_TRAP, cpu_state.sprs[SPR_EEAR_BASE]);
    }
}	/* decode_execute_wrapper() */

/*---------------------------------------------------------------------------*/
/*!Reset the CPU                                                             */
/*---------------------------------------------------------------------------*/
void
cpu_reset ()
{
  int               i;
  struct hist_exec *hist_exec_head = NULL;
  struct hist_exec *hist_exec_new;

  runtime.sim.cycles       = 0;
  runtime.sim.loadcycles   = 0;
  runtime.sim.storecycles  = 0;
  runtime.cpu.instructions = 0;
  runtime.cpu.supercycles  = 0;
  runtime.cpu.hazardwait   = 0;

  for (i = 0; i < MAX_GPRS; i++)
    {
      setsim_reg (i, 0);
    }

  memset (&cpu_state.iqueue,   0, sizeof (cpu_state.iqueue));
  memset (&cpu_state.icomplet, 0, sizeof (cpu_state.icomplet));

  sbuf_head        = 0;
  sbuf_tail        = 0;
  sbuf_count       = 0;
  sbuf_prev_cycles = 0;

  /* Initialise execution history circular buffer */
  for (i = 0; i < HISTEXEC_LEN; i++)
    {
      hist_exec_new = malloc (sizeof (struct hist_exec));

      if (!hist_exec_new)
	{
	  fprintf (stderr, "Out-of-memory\n");
	  exit (1);
	}

      if (!hist_exec_head)
	{
	  hist_exec_head = hist_exec_new;
	}
      else
	{
	  hist_exec_tail->next = hist_exec_new;
	}

      hist_exec_new->prev = hist_exec_tail;
      hist_exec_tail = hist_exec_new;
    }

  /* Make hist_exec_tail->next point to hist_exec_head */
  hist_exec_tail->next = hist_exec_head;
  hist_exec_head->prev = hist_exec_tail;

  /* MM1409: All progs should start at reset vector entry! This sorted out by
     setting the cpu_state.pc field below. Not clear this is very good code! */

  /* Patches suggested by Shinji Wakatsuki, so that the vector address takes
     notice of the Exception Prefix High bit of the Supervision register */
  pcnext = (cpu_state.sprs[SPR_SR] & SPR_SR_EPH ? 0xf0000000 : 0x00000000);

  if (config.sim.verbose)
    {
      PRINTF ("Starting at 0x%" PRIxADDR "\n", pcnext);
    }

  cpu_state.pc  = pcnext;
  pcnext       += 4;

  /* MM1409: All programs should set their stack pointer!  */
  except_handle (EXCEPT_RESET, 0);
  update_pc ();

  except_pending = 0;
  cpu_state.pc   = cpu_state.sprs[SPR_SR] & SPR_SR_EPH ?
    0xf0000000 + EXCEPT_RESET : EXCEPT_RESET;

}	/* cpu_reset() */


/*---------------------------------------------------------------------------*/
/*!Simulates one CPU clock cycle
 
  @return  non-zero if a breakpoint is hit, zero otherwise.                  */
/*---------------------------------------------------------------------------*/
int
cpu_clock ()
{
  except_pending  = 0;
  next_delay_insn = 0;

  if (fetch ())
    {
      PRINTF ("Breakpoint hit.\n");
      return  1;
    }

  if (except_pending)
    {
      update_pc ();
      except_pending = 0;
      return  0;
    }

  if (breakpoint)
    {
      except_handle (EXCEPT_TRAP, cpu_state.sprs[SPR_EEAR_BASE]);
      update_pc ();
      except_pending = 0;
      return  0;
    }

  decode_execute_wrapper (&cpu_state.iqueue);
  update_pc ();
  return  0;

}	/* cpu_clock() */


/*---------------------------------------------------------------------------*/
/*!If decoding cannot be found, call this function                           */
/*---------------------------------------------------------------------------*/
#if SIMPLE_EXECUTION
void
l_invalid (struct iqueue_entry *current)
{
#else
void
l_invalid ()
{
#endif
  except_handle (EXCEPT_ILLEGAL, cpu_state.iqueue.insn_addr);

}	/* l_invalid() */


/*---------------------------------------------------------------------------*/
/*!The main execution loop                                                   */
/*---------------------------------------------------------------------------*/
void
exec_main ()
{
  long long time_start;

  while (1)
    {
      time_start = runtime.sim.cycles;
      if (config.debug.enabled)
	{
	  while (runtime.cpu.stalled)
	    {
	      if (config.debug.rsp_enabled)
		{
		  handle_rsp ();
		}
	      else
		{
		  fprintf (stderr, "ERROR: CPU stalled and GDB connection not "
			   "enabled: Invoking CLI and terminating.\n");
		  /* Dump the user into interactive mode.  From there he or
		     she can decide what to do. */
		  handle_sim_command ();
		  sim_done ();
		}
	      if (runtime.sim.iprompt)
		handle_sim_command ();
	    }
	}

      /* Each cycle has counter of mem_cycles; this value is joined with cycles
         at the end of the cycle; no sim originated memory accesses should be
         performed inbetween. */
      runtime.sim.mem_cycles = 0;

      if (!config.pm.enabled ||
	  !(config.pm.enabled &
	    (cpu_state.sprs[SPR_PMR] & (SPR_PMR_DME | SPR_PMR_SME))))
	{
	  if (cpu_clock ())
	    {
	      /* A breakpoint has been hit, drop to interactive mode */
	      handle_sim_command ();
	    }
	}

      /* If we are tracing, dump after each instruction. */
      if (!runtime.sim.hush)
	{
	  trace_instr ();
	}

      if (config.vapi.enabled && runtime.vapi.enabled)
	{
	  vapi_check ();
	}

      if (config.debug.enabled)
	{
	  if (cpu_state.sprs[SPR_DMR1] & SPR_DMR1_ST)
	    {
	      set_stall_state (1);

	      if (config.debug.rsp_enabled)
		{
		  rsp_exception (EXCEPT_TRAP);
		}
	    }
	}

      runtime.sim.cycles        += runtime.sim.mem_cycles;
      scheduler.job_queue->time -= runtime.sim.cycles - time_start;

      if (scheduler.job_queue->time <= 0)
	{
	  do_scheduler ();
	}
    }
}	/* exec_main() */

/*---------------------------------------------------------------------------*/
/*!Update the rounding mode variable the softfloat library reads             */
/*---------------------------------------------------------------------------*/
static void
float_set_rm ()
{
  //
  // float_rounding_mode is used by the softfloat library, it is declared in
  // "softfloat.h"
  //
  switch(cpu_state.sprs[SPR_FPCSR] & SPR_FPCSR_RM)
    {
    case FPCSR_RM_RN:
      //printf("or1ksim <%s>: rounding mode RN\n",__FUNCTION__);
      float_rounding_mode = float_round_nearest_even;
      break;
    case FPCSR_RM_RZ:
      //printf("or1ksim <%s>: rounding mode RZ\n",__FUNCTION__);
      float_rounding_mode = float_round_to_zero;
      break;
    case FPCSR_RM_RIP:
      //printf("or1ksim <%s>: rounding mode R+\n",__FUNCTION__);
      float_rounding_mode = float_round_up;
      break;
    case FPCSR_RM_RIN:
      //printf("or1ksim <%s>: rounding mode R-\n",__FUNCTION__);
      float_rounding_mode = float_round_down;
      break;
    }
}

/*---------------------------------------------------------------------------*/
/*!Update the OR1K's FPCSR after each floating point instruction             */
/*---------------------------------------------------------------------------*/
static void
float_set_flags ()
{
  // Get the flags from softfloat's variable and set the OR1K's FPCR values
  // First clear all flags in OR1K FPCSR
  cpu_state.sprs[SPR_FPCSR] &= ~SPR_FPCSR_ALLF;
  
  if (float_exception_flags & float_flag_invalid)
    cpu_state.sprs[SPR_FPCSR] |= SPR_FPCSR_IVF;
  if (float_exception_flags & float_flag_divbyzero)
    cpu_state.sprs[SPR_FPCSR] |= SPR_FPCSR_DZF;
  if (float_exception_flags & float_flag_overflow)
    cpu_state.sprs[SPR_FPCSR] |= SPR_FPCSR_OVF;
  if (float_exception_flags & float_flag_underflow)
    cpu_state.sprs[SPR_FPCSR] |= SPR_FPCSR_UNF;
  if (float_exception_flags & float_flag_inexact)
    cpu_state.sprs[SPR_FPCSR] |= SPR_FPCSR_IXF;
  /*
  printf("or1ksim: post-fp-op flags from softfloat: %x%x%x%x%x\n",
	 !!(float_exception_flags & float_flag_invalid),
	 !!(float_exception_flags & float_flag_divbyzero),
	 !!(float_exception_flags & float_flag_overflow),
	 !!(float_exception_flags & float_flag_underflow),
	 !!(float_exception_flags & float_flag_inexact));
  */
  // TODO: Call FP exception is FPEE set and any of the flags were set
  /*
     if ((cpu_state.sprs[SPR_FPCSR] & SPR_FPCSR_FPEE) &
     (|(cpu_state.sprs[SPR_FPCSR] & SPR_FPCSR_ALLF)))
     except_handle (EXCEPT_FPE, cpu_state.iqueue.insn_addr);
  */
  // Now clear softfloat's flags:
  float_exception_flags = 0;
  
}

#if COMPLEX_EXECUTION

/* Include generated/built in decode_execute function */
#include "execgen.c"

#elif SIMPLE_EXECUTION


/*---------------------------------------------------------------------------*/
/*!Evaluates source operand

   Implementation specific.

   @param[in] op_no       The operand
   @param[in] insn_index  Address of the instruction
   @param[in] insn        The instruction

   @return  The value of the operand                                         */
/*---------------------------------------------------------------------------*/
static uorreg_t
eval_operand (int            op_no,
	      unsigned long  insn_index,
	      uint32_t       insn)
{
  struct insn_op_struct *opd = or1ksim_op_start[insn_index];
  uorreg_t               ret;

  while (op_no)
    {
      if (opd->type & OPTYPE_LAST)
	{
	  fprintf (stderr,
		   "Instruction requested more operands than it has\n");
	  exit (1);
	}

      if ((opd->type & OPTYPE_OP) && !(opd->type & OPTYPE_DIS))
	{
	  op_no--;
	}

      opd++;
    }

  if (opd->type & OPTYPE_DIS)
    {
      ret = eval_operand_val (insn, opd);

      while (!(opd->type & OPTYPE_OP))
	{
	  opd++;
	}

      opd++;
      ret               += evalsim_reg (eval_operand_val (insn, opd));
      cpu_state.insn_ea  = ret;

      return  ret;
    }

  if (opd->type & OPTYPE_REG)
    {
      return  evalsim_reg (eval_operand_val (insn, opd));
    }

  return  eval_operand_val (insn, opd);

}	/* eval_operand() */


/*---------------------------------------------------------------------------*/
/*!Set destination operand (register direct) with value.

   Implementation specific.

   @param[in] op_no       The operand
   @param[in] value       The value to set
   @param[in] insn_index  Address of the instruction
   @param[in] insn        The instruction                                    */
/*---------------------------------------------------------------------------*/
static void
set_operand (int            op_no,
	     orreg_t        value,
	     unsigned long  insn_index,
	     uint32_t       insn)
{
  struct insn_op_struct *opd = or1ksim_op_start[insn_index];

  while (op_no)
    {
      if (opd->type & OPTYPE_LAST)
	{
	  fprintf (stderr,
		   "Instruction requested more operands than it has\n");
	  exit (1);
	}

      if ((opd->type & OPTYPE_OP) && !(opd->type & OPTYPE_DIS))
	{
	  op_no--;
	}

      opd++;
    }

  if (!(opd->type & OPTYPE_REG))
    {
      fprintf (stderr, "Trying to set a non-register operand\n");
      exit (1);
    }

  setsim_reg (eval_operand_val (insn, opd), value);

}	/* set_operand() */


/*---------------------------------------------------------------------------*/
/*!Simple and rather slow decoding function

   Based on built automata.

   @param[in] current  The current instruction to execute                    */
/*---------------------------------------------------------------------------*/
static void
decode_execute (struct iqueue_entry *current)
{
  int insn_index;

  current->insn_index = insn_index = or1ksim_insn_decode (current->insn);

  if (insn_index < 0)
    {
      l_invalid (current);
    }
  else
    {
      or1ksim_or32_opcodes[insn_index].exec (current);
    }

  if (do_stats)
    analysis (&cpu_state.iqueue);
}

#include "insnset.c"

#else
# error "Must define SIMPLE_EXECUTION, COMPLEX_EXECUTION"
#endif
