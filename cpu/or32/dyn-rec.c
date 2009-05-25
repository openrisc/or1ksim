/* dyn-rec.c -- Dynamic recompiler implementation for or32
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <signal.h>
#include <errno.h>
#include <execinfo.h>

#include "config.h"

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#include "port.h"
#include "arch.h"
#include "immu.h"
#include "abstract.h"
#include "opcode/or32.h"
#include "spr-defs.h"
#include "execute.h"
#include "except.h"
#include "spr-defs.h"
#include "sim-config.h"
#include "sched.h"
#include "i386-regs.h"
#include "def-op-t.h"
#include "dyn-rec.h"
#include "gen-ops.h"
#include "op-support.h"
#include "toplevel-support.h"


/* NOTE: All openrisc (or) addresses in this file are *PHYSICAL* addresses */

/* FIXME: Optimise sorted list adding */

typedef void (*generic_gen_op)(struct op_queue *opq, int end);
typedef void (*imm_gen_op)(struct op_queue *opq, int end, uorreg_t imm);

void gen_l_invalid(struct op_queue *opq, int param_t[3], int delay_slot);

/* ttg->temporary to gpr */
DEF_GPR_OP(generic_gen_op, gen_op_move_gpr_t, gen_op_ttg_gpr);
/* gtt->gpr to temporary */
DEF_GPR_OP(generic_gen_op, gen_op_move_t_gpr, gen_op_gtt_gpr);

DEF_1T_OP(imm_gen_op, calc_insn_ea_table, gen_op_calc_insn_ea);

/* Linker stubs.  This will allow the linker to link in op.o.  The relocations
 * that the linker does for these will be irrelevent anyway, since we patch the
 * relocations during recompilation. */
uorreg_t __op_param1;
uorreg_t __op_param2;
uorreg_t __op_param3;

/* The number of bytes that a dynamicly recompiled page should be enlarged by */
#define RECED_PAGE_ENLARGE_BY 51200

/* The number of entries that the micro operations array in op_queue should be
 * enlarged by */
#define OPS_ENLARGE_BY 5

#define T_NONE (-1)

/* Temporary is used as a source operand */
#define TFLAG_SRC 1
/* Temporary is used as a destination operand */
#define TFLAG_DST 2
/* Temporary has been saved to permanent storage */
#define TFLAG_SAVED 4
/* Temporary contains the value of the register before the instruction execution
 * occurs (either by an explicit reg->t move or implicitly being left over from
 * a previous instruction) */
#define TFLAG_SOURCED 8

/* FIXME: Put this into some header */
extern int do_stats;

static int sigsegv_state = 0;
static void *sigsegv_addr = NULL;

void dyn_ret_stack_prot(void);

void dyn_sigsegv_debug(int u, siginfo_t *siginf, void *dat)
{
  struct dyn_page *dp;
  FILE *f;
  char filen[18]; /* 18 == strlen("or_page.%08x") + 1 */
  int i;
  struct sigcontext *sigc = dat;

  if(!sigsegv_state) {
    sigsegv_addr = siginf->si_addr;
  } else {
    fprintf(stderr, "Nested SIGSEGV occured, dumping next chuck of info\n");
    sigsegv_state++;
  }

  /* First dump all the data that does not need dereferenceing to get */
  switch(sigsegv_state) {
  case 0:
    fflush(stderr);
    fprintf(stderr, "Segmentation fault on acces to %p at 0x%08lx, (or address: 0x%"PRIxADDR")\n\n",
            sigsegv_addr, sigc->eip, cpu_state.pc);
    sigsegv_state++;
  case 1:
    /* Run through the recompiled pages, dumping them to disk as we go */
    for(i = 0; i < (2 << (32 - immu_state->pagesize_log2)); i++) {
      dp = cpu_state.dyn_pages[i];
      if(!dp)
        continue;
      fprintf(stderr, "Dumping%s page 0x%"PRIxADDR" recompiled to %p (len: %u) to disk\n",
             dp->dirty ? " dirty" : "", dp->or_page, dp->host_page,
             dp->host_len);
      fflush(stdout);

      sprintf(filen, "or_page.%"PRIxADDR, dp->or_page);
      if(!(f = fopen(filen, "w"))) {
        fprintf(stderr, "Unable to open %s to dump the recompiled page to: %s\n",
                filen, strerror(errno));
        continue;
      }
      if(fwrite(dp->host_page, dp->host_len, 1, f) < 1)
        fprintf(stderr, "Unable to write recompiled data to file: %s\n",
                strerror(errno));

      fclose(f);
    }
    sigsegv_state++;
  case 2:
    sim_done();
  }
}

struct dyn_page *new_dp(oraddr_t page)
{
  struct dyn_page *dp = malloc(sizeof(struct dyn_page));
  dp->or_page = IADDR_PAGE(page);

  dp->locs = malloc(sizeof(void *) * (immu_state->pagesize / 4));

  dp->host_len = 0;
  dp->host_page = NULL;
  dp->dirty = 1;

  if(do_stats) {
    dp->insns = malloc(immu_state->pagesize);
    dp->insn_indexs = malloc(sizeof(unsigned int) * (immu_state->pagesize / 4));
  }

  cpu_state.dyn_pages[dp->or_page >> immu_state->pagesize_log2] = dp;
  return dp;
}

void dyn_main(void)
{
  struct dyn_page *target_dp;
  oraddr_t phys_page;

  setjmp(cpu_state.excpt_loc);
  for(;;) {
    phys_page = immu_translate(cpu_state.pc);

/*
    printf("Recompiled code jumping out to %"PRIxADDR" from %"PRIxADDR"\n",
           phys_page, cpu_state.sprs[SPR_PPC] - 4);
*/

    /* immu_translate() adds the hit delay to runtime.sim.mem_cycles but we add
     * it to the cycles when the instruction is executed so if we don't reset it
     * now it will produce wrong results */
    runtime.sim.mem_cycles = 0;

    target_dp = cpu_state.dyn_pages[phys_page >> immu_state->pagesize_log2];

    if(!target_dp)
      target_dp = new_dp(phys_page);

    /* Since writes to the 0x0-0xff range do not dirtyfy a page recompile the
     *  0x0 page if the jump is to that location */
    if(phys_page < 0x100)
      target_dp->dirty = 1;

    if(target_dp->dirty)
      recompile_page(target_dp);

    cpu_state.curr_page = target_dp;

    /* FIXME: If the page is backed by more than one type of memory, this will
     * produce wrong results */
    cpu_state.cycles_dec = target_dp->delayr;
    if(cpu_state.sprs[SPR_SR] & SPR_SR_IME)
      /* Add the mmu hit delay to the cycle counter */
      cpu_state.cycles_dec -= immu_state->hitdelay;

    /* FIXME: ebp, ebx, esi and edi are expected to be preserved across function
     * calls but the recompiled code trashes them... */
    enter_dyn_code(phys_page, target_dp);
  }
}

static void immu_retranslate(void *dat)
{
  int got_en_dis = (int)dat;
  immu_translate(cpu_state.pc);
  runtime.sim.mem_cycles = 0;

  /* Only update the cycle decrementer if the mmu got enabled or disabled */
  if(got_en_dis == IMMU_GOT_ENABLED)
    /* Add the mmu hit delay to the cycle counter */
    cpu_state.cycles_dec = cpu_state.curr_page->delayr - immu_state->hitdelay;
  else if(got_en_dis == IMMU_GOT_DISABLED)
    cpu_state.cycles_dec = cpu_state.curr_page->delayr;
}

/* This is called whenever the immu is either enabled/disabled or reconfigured
 * while enabled.  This checks if an itlb miss would occour and updates the immu
 * hit delay counter */
void recheck_immu(int got_en_dis)
{
  oraddr_t addr;

  if(cpu_state.delay_insn)
    addr = cpu_state.pc_delay;
  else
    addr = cpu_state.pc + 4;

  if(IADDR_PAGE(cpu_state.pc) == IADDR_PAGE(addr))
    /* Schedule a job to do immu_translate() */
    SCHED_ADD(immu_retranslate, (void *)got_en_dis, 0);
}

/* Runs the scheduler.  Called from except_handler (and dirtyfy_page below) */
void run_sched_out_of_line(void)
{
  oraddr_t off = (cpu_state.pc & immu_state->page_offset_mask) >> 2;

  if(do_stats) {
    cpu_state.iqueue.insn_addr = cpu_state.pc;
    cpu_state.iqueue.insn = cpu_state.curr_page->insns[off];
    cpu_state.iqueue.insn_index = cpu_state.curr_page->insn_indexs[off];
    runtime.cpu.instructions++;
    analysis(&cpu_state.iqueue);
  }

  /* Run the scheduler */
  scheduler.job_queue->time += cpu_state.cycles_dec;
  runtime.sim.cycles -= cpu_state.cycles_dec;

  op_join_mem_cycles();
  if(scheduler.job_queue->time <= 0)
    do_scheduler();
}

/* Signals a page as dirty */
static void dirtyfy_page(struct dyn_page *dp)
{
  oraddr_t check;

  printf("Dirtyfying page 0x%"PRIxADDR"\n", dp->or_page);

  dp->dirty = 1;

  /* If the execution is currently in the page that was touched then recompile
   * it now and jump back to the point of execution */
  check = cpu_state.delay_insn ? cpu_state.pc_delay : cpu_state.pc + 4;
  if(IADDR_PAGE(check) == dp->or_page) {
    run_sched_out_of_line();
    recompile_page(dp);

    cpu_state.delay_insn = 0;

    /* Jump out to the next instruction */
    do_jump(check);
  }
}

/* Checks to see if a write happened to a recompiled page.  If so marks it as
 * dirty */
void dyn_checkwrite(oraddr_t addr)
{
  /* FIXME: Do this with mprotect() */
  struct dyn_page *dp = cpu_state.dyn_pages[addr >> immu_state->pagesize_log2];

  /* Since the locations 0x0-0xff are nearly always written to in an exception
   * handler, ignore any writes to these locations.  If code ends up jumping
   * out there, we'll recompile when the jump actually happens. */
  if((addr > 0x100) && dp && !dp->dirty)
    dirtyfy_page(dp);
}

/* Moves the temprary t to its permanent storage if it has been used as a
 * destination register */
static void ship_t_out(struct op_queue *opq, unsigned int t)
{
  unsigned int gpr = opq->reg_t[t];

  for(; opq; opq = opq->prev) {
    if(opq->reg_t[t] != gpr)
      return;
    if((opq->tflags[t] & TFLAG_DST) && !(opq->tflags[t] & TFLAG_SAVED)) {
      opq->tflags[t] |= TFLAG_SAVED;

      /* FIXME: Check if this is still neccesary */
      /* Before takeing the temporaries out, temporarily remove the op_do_sched
       * operation such that dyn_page->ts_bound shall be correct before the
       * scheduler runs */
      if(opq->num_ops && (opq->ops[opq->num_ops - 1] == op_do_sched_indx)) {
        opq->num_ops--;
        gen_op_move_gpr_t[t][gpr](opq, 1);
        gen_op_do_sched(opq, 1);
        return;
      }

      gen_op_move_gpr_t[t][gpr](opq, 1);

      return;
    }
  }
}

static void ship_gprs_out_t(struct op_queue *opq)
{
  int i;

  if(!opq)
    return;

  for(i = 0; i < NUM_T_REGS; i++) {
    if(opq->reg_t[i] < 32)
      /* Ship temporaries out in the last opq that actually touched it */
      ship_t_out(opq, i);
  }
}

/* FIXME: Look at the following instructions to make a better guess at which
 * temporary to return */
static int find_t(struct op_queue *opq, unsigned int reg)
{
  int i, j, t = -1;

  for(i = 0; i < NUM_T_REGS; i++) {
    if(opq->reg_t[i] == reg)
      return i;

    /* Ok, we have found an as-yet unused temporary, check if it is needed
     * later in this instruction */
    for(j = 0; j < opq->param_num; j++) {
      if((opq->param_type[j] & OPTYPE_REG) && (opq->param[j] == opq->reg_t[i]))
        break;
    }

    if(j != opq->param_num)
      continue;

    /* We have found the temporary (temporarily:) fit for use */
    if((t == -1) || (opq->reg_t[i] == 32))
      t = i;
  }

  return t;
}

/* Checks if there is enough space in dp->host_page, if not grow it */
void *enough_host_page(struct dyn_page *dp, void *cur, unsigned int *len,
                       unsigned int amount)
{
  unsigned int used = cur - dp->host_page;

  /* The array is long enough */
  if((used + amount) <= *len)
    return cur;

  /* Reallocate */
  *len += RECED_PAGE_ENLARGE_BY;

  if(!(dp->host_page = realloc(dp->host_page, *len))) {
    fprintf(stderr, "OOM\n");
    exit(1);
  }

  return dp->host_page + used;
}

/* Adds an operation to the opq */
void add_to_opq(struct op_queue *opq, int end, int op)
{
  if(opq->num_ops == opq->ops_len) {
    opq->ops_len += OPS_ENLARGE_BY;
    if(!(opq->ops = realloc(opq->ops, opq->ops_len * sizeof(int)))) {
      fprintf(stderr, "OOM\n");
      exit(1);
    }
  }

  if(end)
    opq->ops[opq->num_ops] = op;
  else {
    /* Shift everything over by one */
    memmove(opq->ops + 1, opq->ops, opq->num_ops* sizeof(int));
    opq->ops[0] = op;
  }

  opq->num_ops++;
}

static void gen_op_mark_loc(struct op_queue *opq, int end)
{
  add_to_opq(opq, end, op_mark_loc_indx);
}

/* Adds a parameter to the opq */
void add_to_op_params(struct op_queue *opq, int end, unsigned long param)
{
  if(opq->num_ops_param == opq->ops_param_len) {
    opq->ops_param_len += OPS_ENLARGE_BY;
    if(!(opq->ops_param = realloc(opq->ops_param, opq->ops_param_len * sizeof(int)))) {
      fprintf(stderr, "OOM\n");
      exit(1);
    }
  }

  if(end)
    opq->ops_param[opq->num_ops_param] = param;
  else {
    /* Shift everything over by one */
    memmove(opq->ops_param + 1, opq->ops_param, opq->num_ops_param);
    opq->ops_param[0] = param;
  }

  opq->num_ops_param++;
}

/* Initialises the recompiler */
void init_dyn_recomp(void)
{
  struct sigaction sigact;
  struct op_queue *opq = NULL;
  unsigned int i;

  cpu_state.opqs = NULL;

  /* Allocate the operation queue list (+1 for the page chaining) */
  for(i = 0; i < (immu_state->pagesize / 4) + 1; i++) {
    if(!(opq = malloc(sizeof(struct op_queue)))) {
      fprintf(stderr, "OOM\n");
      exit(1);
    }

    /* initialise some fields */
    opq->ops_len = 0;
    opq->ops = NULL;
    opq->ops_param_len = 0;
    opq->ops_param = NULL;
    opq->xref = 0;

    if(cpu_state.opqs)
      cpu_state.opqs->prev = opq;

    opq->next = cpu_state.opqs;
    cpu_state.opqs = opq;
  }

  opq->prev = NULL;

  cpu_state.curr_page = NULL;
  if(!(cpu_state.dyn_pages = malloc(sizeof(void *) * (2 << (32 -
                                                immu_state->pagesize_log2))))) {
    fprintf(stderr, "OOM\n");
    exit(1);
  }
  memset(cpu_state.dyn_pages, 0,
         sizeof(void *) * (2 << (32 - immu_state->pagesize_log2)));

  /* Register our segmentation fault handler */
  sigact.sa_sigaction = dyn_sigsegv_debug;
  memset(&sigact.sa_mask, 0, sizeof(sigact.sa_mask));
  sigact.sa_flags = SA_SIGINFO | SA_NOMASK;
  if(sigaction(SIGSEGV, &sigact, NULL))
    printf("WARN: Unable to install SIGSEGV handler! Don't expect to be able to debug the recompiler.\n");

  /* FIXME: Find a better place for this */
    { /* Needed by execution */
      extern int do_stats;
      do_stats = config.cpu.dependstats || config.cpu.superscalar || config.cpu.dependstats
              || config.sim.history || config.sim.exe_log;
    }

  printf("Recompile engine up and running\n");
}

/* Parses instructions and their operands and populates opq with them */
static void eval_insn_ops(struct op_queue *opq, oraddr_t addr)
{
  int breakp;
  struct insn_op_struct *opd;

  for(; opq->next; opq = opq->next, addr += 4) {
    opq->param_num = 0;
    breakp = 0;
    opq->insn = eval_insn(addr, &breakp);

    /* FIXME: If a breakpoint is set at this location, insert exception code */
    if(breakp) {
      fprintf(stderr, "FIXME: Insert breakpoint code\n");
    }

    opq->insn_index = insn_decode(opq->insn);

    if(opq->insn_index == -1)
      continue;

    opd = op_start[opq->insn_index];
    
    do {
      opq->param[opq->param_num] = eval_operand_val(opq->insn, opd);
      opq->param_type[opq->param_num] = opd->type;

      opq->param_num++;
      while(!(opd->type & OPTYPE_OP)) opd++;
    } while(!(opd++->type & OPTYPE_LAST));
  }
}

/* Adds code to the opq for the instruction pointed to by addr */
static void recompile_insn(struct op_queue *opq, int delay_insn)
{
  int j, k;
  int param_t[5]; /* Which temporary the parameters reside in */

  /* Check if we have an illegal instruction */
  if(opq->insn_index == -1) {
    gen_l_invalid(opq, NULL, delay_insn);
    return;
  }

  /* If we are recompileing an instruction that has a delay slot and is in the
   * delay slot, ignore it.  This is undefined behavour. */
  if(delay_insn && (or32_opcodes[opq->insn_index].flags & OR32_IF_DELAY))
    return;

  param_t[0] = T_NONE;
  param_t[1] = T_NONE;
  param_t[2] = T_NONE;
  param_t[3] = T_NONE;
  param_t[4] = T_NONE;

  /* Jump instructions are special since they have a delay slot and thus they
   * need to control the exact operation sequence.  Special case these here to
   * avoid haveing loads of if(!(.& OR32_IF_DELAY)) below */
  if(or32_opcodes[opq->insn_index].flags & OR32_IF_DELAY) {
    /* Jump instructions don't have a disposition */
    or32_opcodes[opq->insn_index].exec(opq, param_t, delay_insn);

    /* Analysis is done by the individual jump instructions */
    /* Jump instructions don't touch runtime.sim.mem_cycles */
    /* Jump instructions run their own scheduler */
    return;
  }

  /* Before an exception takes place, all registers must be stored. */
  if((or32_opcodes[opq->insn_index].func_unit == it_exception)) {
    ship_gprs_out_t(opq);

    or32_opcodes[opq->insn_index].exec(opq, param_t, delay_insn);
    return;
  }

  for(j = 0; j < opq->param_num; j++) {
    if(!(opq->param_type[j] & OPTYPE_REG))
      continue;

    /* Never, ever, move r0 into a temporary */
    if(!opq->param[j])
      continue;

    k = find_t(opq, opq->param[j]);

    param_t[j] = k;

    if(opq->reg_t[k] == opq->param[j]) {
      if(!(opq->param_type[j] & OPTYPE_DST) && 
         !(opq->tflags[k] & TFLAG_SOURCED)) {
        gen_op_move_t_gpr[k][opq->reg_t[k]](opq, 0);
        opq->tflags[k] |= TFLAG_SOURCED;
      }

      if(opq->param_type[j] & OPTYPE_DST)
        opq->tflags[k] |= TFLAG_DST;
      else
        opq->tflags[k] |= TFLAG_SRC;

      continue;
    }

    if(opq->reg_t[k] < 32) {
      /* Only ship the temporary out if it has been used as a destination
       * register */
      ship_t_out(opq, k);
    }

    if(opq->param_type[j] & OPTYPE_DST)
      opq->tflags[k] = TFLAG_DST;
    else
      opq->tflags[k] = TFLAG_SRC;

    opq->reg_t[k] = opq->param[j];

    /* Only generate code to move the register into a temporary if it is used as
     * a source operand */
    if(!(opq->param_type[j] & OPTYPE_DST)) {
      gen_op_move_t_gpr[k][opq->reg_t[k]](opq, 0);
      opq->tflags[k] |= TFLAG_SOURCED;
    }
  }

  /* To get the execution log correct for instructions like l.lwz r4,0(r4) the
   * effective address needs to be calculated before the instruction is
   * simulated */
  if(do_stats) {
    for(j = 0; j < opq->param_num; j++) {
      if(!(opq->param_type[j] & OPTYPE_DIS))
        continue;

      if(!opq->param[j + 1])
        gen_op_store_insn_ea(opq, 1, opq->param[j]);
      else
        calc_insn_ea_table[param_t[j + 1]](opq, 1, opq->param[j]);
    }
  }

  or32_opcodes[opq->insn_index].exec(opq, param_t, delay_insn);

  if(do_stats) {
    ship_gprs_out_t(opq);
    gen_op_analysis(opq, 1);
  }

  /* The call to join_mem_cycles() could be put into the individual operations
   * that emulate the load/store instructions, but then it would be added to
   * the cycle counter before analysis() is called, which is not how the complex
   * execution model does it. */
  if((or32_opcodes[opq->insn_index].func_unit == it_load) ||
     (or32_opcodes[opq->insn_index].func_unit == it_store))
    gen_op_join_mem_cycles(opq, 1);

  /* Delay slot instructions get a special scheduler, thus don't generate it
   * here */
  if(!delay_insn)
    gen_op_do_sched(opq, 1);
}

/* Recompiles the page associated with *dyn */
void recompile_page(struct dyn_page *dyn)
{
  unsigned int j;
  struct op_queue *opq = cpu_state.opqs;
  oraddr_t rec_addr = dyn->or_page;
  oraddr_t rec_page = dyn->or_page;
  void **loc;

  /* The start of the next page */
  rec_page += immu_state->pagesize;

  printf("Recompileing page %"PRIxADDR"\n", rec_addr);
  fflush(stdout);

  /* Mark all temporaries as not containing a register */
  for(j = 0; j < NUM_T_REGS; j++) {
    opq->reg_t[j] = 32; /* Out-of-range registers */
    opq->tflags[j] = 0;
  }

  dyn->delayr = -verify_memoryarea(rec_addr)->ops.delayr;

  opq->num_ops = 0;
  opq->num_ops_param = 0;

  eval_insn_ops(opq, rec_addr);

  /* Insert code to check if the first instruction is exeucted in a delay slot*/
  gen_op_check_delay_slot(opq, 1, 0);
  recompile_insn(opq, 1);
  ship_gprs_out_t(opq);
  gen_op_do_sched_delay(opq, 1);
  gen_op_clear_delay_insn(opq, 1);
  gen_op_do_jump_delay(opq, 1);
  gen_op_do_jump(opq, 1);
  gen_op_mark_loc(opq, 1);

  for(j = 0; j < NUM_T_REGS; j++)
    opq->reg_t[j] = 32; /* Out-of-range registers */

  for(; rec_addr < rec_page; rec_addr += 4, opq = opq->next) {
    if(opq->prev) {
      opq->num_ops = 0;
      opq->num_ops_param = 0;
    }
    opq->jump_local = -1;
    opq->not_jump_loc = -1;

    opq->insn_addr = rec_addr;
 
    for(j = 0; j < NUM_T_REGS; j++)
      opq->tflags[j] = TFLAG_SOURCED;

    /* Check if this location is cross referenced */
    if(opq->xref) {
      /* If the current address is cross-referenced, the temporaries shall be
       * in an undefined state, so we must assume that no registers reside in
       * them */
      /* Ship out the current set of registers from the temporaries */
      if(opq->prev) {
        ship_gprs_out_t(opq->prev);
        for(j = 0; j < NUM_T_REGS; j++) {
          opq->reg_t[j] = 32;
          opq->prev->reg_t[j] = 32;
        }
      }
    }

    recompile_insn(opq, 0);

    /* Store the state of the temporaries */
    memcpy(opq->next->reg_t, opq->reg_t, sizeof(opq->reg_t));
  }

  dyn->dirty = 0;

  /* Ship temporaries out to the corrisponding registers */
  ship_gprs_out_t(opq->prev);

  opq->num_ops = 0;
  opq->num_ops_param = 0;
  opq->not_jump_loc = -1;
  opq->jump_local = -1;

  /* Insert code to jump to the next page */
  gen_op_do_jump(opq, 1);

  /* Generate the code */
  gen_code(cpu_state.opqs, dyn);

  /* Fix up the locations */
  for(loc = dyn->locs; loc < &dyn->locs[immu_state->pagesize / 4]; loc++)
    *loc += (unsigned int)dyn->host_page;

  cpu_state.opqs->ops_param[0] += (unsigned int)dyn->host_page;

  /* Search for page-local jumps */
  opq = cpu_state.opqs;
  for(j = 0; j < (immu_state->pagesize / 4); opq = opq->next, j++) {
    if(opq->jump_local != -1)
      opq->ops_param[opq->jump_local] =
                              (unsigned int)dyn->locs[opq->jump_local_loc >> 2];

    if(opq->not_jump_loc != -1)
      opq->ops_param[opq->not_jump_loc] = (unsigned int)dyn->locs[j + 1];

    /* Store the state of the temporaries into dyn->ts_bound */
    dyn->ts_bound[j] = 0;
    if(opq->reg_t[0] < 32)
      dyn->ts_bound[j] = opq->reg_t[0];
    if(opq->reg_t[1] < 32)
      dyn->ts_bound[j] |= opq->reg_t[1] << 5;
    if(opq->reg_t[2] < 32)
      dyn->ts_bound[j] |= opq->reg_t[2] << 10;

    /* Reset for the next page to be recompiled */
    opq->xref = 0;
  }

  /* Patch the relocations */
  patch_relocs(cpu_state.opqs, dyn->host_page);

  if(do_stats) {
    opq = cpu_state.opqs;
    for(j = 0; j < (immu_state->pagesize / 4); j++, opq = opq->next) {
      dyn->insns[j] = opq->insn;
      dyn->insn_indexs[j] = opq->insn_index;
    }
  }

  /* FIXME: Fix the issue below in a more elegent way */
  /* Since eval_insn is called to get the instruction, runtime.sim.mem_cycles is
   * updated but the recompiler expectes it to start a 0, so reset it */
  runtime.sim.mem_cycles = 0;
}

/* Recompiles a delay-slot instruction (opq is the opq of the instruction
 * haveing the delay-slot) */
static void recompile_delay_insn(struct op_queue *opq)
{
  struct op_queue delay_opq;
  int i;

  /* Setup a fake opq that looks very much like the delay slot instruction */
  memcpy(&delay_opq, opq, sizeof(struct op_queue));
  /* `Fix' a couple of bits */
  for(i = 0; i < NUM_T_REGS; i++)
    delay_opq.tflags[i] = TFLAG_SOURCED;
  delay_opq.insn_index = opq->next->insn_index;
  memcpy(delay_opq.param_type, opq->next->param_type, sizeof(delay_opq.param_type));
  memcpy(delay_opq.param, opq->next->param, sizeof(delay_opq.param));
  delay_opq.param_num = opq->next->param_num;
  delay_opq.insn = opq->next->insn;

  delay_opq.xref = 0;
  delay_opq.insn_addr = opq->insn_addr + 4;
  delay_opq.prev = opq->prev;
  delay_opq.next = NULL;

  /* Generate the delay slot instruction */
  recompile_insn(&delay_opq, 1);

  ship_gprs_out_t(&delay_opq);

  opq->num_ops = delay_opq.num_ops;
  opq->ops_len = delay_opq.ops_len;
  opq->ops = delay_opq.ops;
  opq->num_ops_param = delay_opq.num_ops_param;
  opq->ops_param_len = delay_opq.ops_param_len;
  opq->ops_param = delay_opq.ops_param;

  for(i = 0; i < NUM_T_REGS; i++)
    opq->reg_t[i] = 32;
}

/* Returns non-zero if the jump is into this page, 0 otherwise */
static int find_jump_loc(oraddr_t j_ea, struct op_queue *opq)
{
  int i;

  /* Mark the jump as non page local if the delay slot instruction is on the
   * next page to the jump instruction.  This should not be needed */
  if(IADDR_PAGE(j_ea) != IADDR_PAGE(opq->insn_addr))
    /* We can't do anything as the j_ea (as passed to find_jump_loc) is a
     * VIRTUAL offset and the next physical page may not be the next VIRTUAL
     * page */
    return 0;

  /* The jump is into the page currently undergoing dynamic recompilation */

  /* If we haven't got to the location of the jump, everything is ok */
  if(j_ea > opq->insn_addr) {
    /* Find the corissponding opq and mark it as cross referenced */
    for(i = (j_ea - opq->insn_addr) / 4; i; i--)
      opq = opq->next;
    opq->xref = 1;
    return 1;
  }

  /* Insert temporary -> register code before the jump ea and register ->
   * temporary at the x-ref address */
  for(i = (opq->insn_addr - j_ea) / 4; i; i--)
    opq = opq->prev;

  if(!opq->prev)
    /* We're at the begining of a page, no need to do anything */
    return 1;

  /* Found location, insert code */

  ship_gprs_out_t(opq->prev);

  for(i = 0; i < NUM_T_REGS; i++) {
    if(opq->prev->reg_t[i] < 32)
      /* FIXME: Ship temporaries in the begining of the opq that needs it */
      gen_op_move_t_gpr[i][opq->prev->reg_t[i]](opq, 0);
  }

  opq->xref = 1;

  return 1;
}

static void gen_j_imm(struct op_queue *opq, oraddr_t off)
{
  int jump_local;

  off <<= 2;

  if(IADDR_PAGE(opq->insn_addr) != IADDR_PAGE(opq->insn_addr + 4)) {
    gen_op_set_pc_delay_imm(opq, 1, off);
    gen_op_do_sched(opq, 1);
    return;
  }

  jump_local = find_jump_loc(opq->insn_addr + off, opq);

  gen_op_set_delay_insn(opq, 1);
  gen_op_do_sched(opq, 1);

  recompile_delay_insn(opq);

  gen_op_add_pc(opq, 1, (orreg_t)off - 8);
  gen_op_clear_delay_insn(opq, 1);
  gen_op_do_sched_delay(opq, 1);

  if(jump_local) {
    gen_op_jmp_imm(opq, 1, 0);
    opq->jump_local = opq->num_ops_param - 1;
    opq->jump_local_loc = (opq->insn_addr + (orreg_t)off) & immu_state->page_offset_mask;
  } else
    gen_op_do_jump(opq, 1);
}

static const generic_gen_op set_pc_delay_gpr[32] = {
 NULL,
 gen_op_move_gpr1_pc_delay,
 gen_op_move_gpr2_pc_delay,
 gen_op_move_gpr3_pc_delay,
 gen_op_move_gpr4_pc_delay,
 gen_op_move_gpr5_pc_delay,
 gen_op_move_gpr6_pc_delay,
 gen_op_move_gpr7_pc_delay,
 gen_op_move_gpr8_pc_delay,
 gen_op_move_gpr9_pc_delay,
 gen_op_move_gpr10_pc_delay,
 gen_op_move_gpr11_pc_delay,
 gen_op_move_gpr12_pc_delay,
 gen_op_move_gpr13_pc_delay,
 gen_op_move_gpr14_pc_delay,
 gen_op_move_gpr15_pc_delay,
 gen_op_move_gpr16_pc_delay,
 gen_op_move_gpr17_pc_delay,
 gen_op_move_gpr18_pc_delay,
 gen_op_move_gpr19_pc_delay,
 gen_op_move_gpr20_pc_delay,
 gen_op_move_gpr21_pc_delay,
 gen_op_move_gpr22_pc_delay,
 gen_op_move_gpr23_pc_delay,
 gen_op_move_gpr24_pc_delay,
 gen_op_move_gpr25_pc_delay,
 gen_op_move_gpr26_pc_delay,
 gen_op_move_gpr27_pc_delay,
 gen_op_move_gpr28_pc_delay,
 gen_op_move_gpr29_pc_delay,
 gen_op_move_gpr30_pc_delay,
 gen_op_move_gpr31_pc_delay };

static void gen_j_reg(struct op_queue *opq, unsigned int gpr)
{
  int i;

  /* Ship the jump-to register out (if it exists).  It requires special
   * handleing */
  for(i = 0; i < NUM_T_REGS; i++) {
    if(opq->reg_t[i] == opq->param[0])
      /* Ship temporary out in the last opq that used it */
      ship_t_out(opq, i);
  }

  if(do_stats)
    gen_op_analysis(opq, 1);

  if(!gpr)
    gen_op_clear_pc_delay(opq, 1);
  else
    set_pc_delay_gpr[gpr](opq, 1);

  gen_op_do_sched(opq, 1);

  if(IADDR_PAGE(opq->insn_addr) != IADDR_PAGE(opq->insn_addr + 4))
    return;

  recompile_delay_insn(opq);

  gen_op_set_pc_pc_delay(opq, 1);
  gen_op_clear_delay_insn(opq, 1);
  gen_op_do_sched_delay(opq, 1);

  gen_op_do_jump_delay(opq, 1);
  gen_op_do_jump(opq, 1);
}

/*------------------------------[ Operation generation for an instruction ]---*/
/* FIXME: Flag setting is not done in any instruction */
/* FIXME: Since r0 is not moved into a temporary, check all arguments below! */

DEF_1T_OP(generic_gen_op, clear_t, gen_op_clear);
DEF_2T_OP_NEQ(generic_gen_op, move_t_t, gen_op_move);
DEF_1T_OP(imm_gen_op, mov_t_imm, gen_op_imm);

DEF_2T_OP(imm_gen_op, l_add_imm_t_table, gen_op_add_imm);
DEF_3T_OP(generic_gen_op, l_add_t_table, gen_op_add);

void gen_l_add(struct op_queue *opq, int param_t[3], int delay_slot)
{
  if(!opq->param[0])
    /* Screw this, the operation shall do nothing */
    return;

  if(!opq->param[1] && !opq->param[2]) {
    /* Just clear param_t[0] */
    clear_t[param_t[0]](opq, 1);
    return;
  }

  if(!opq->param[2]) {
    if(opq->param[0] != opq->param[1])
      /* This just moves a register */
      move_t_t[param_t[0]][param_t[1]](opq, 1);
    return;
  }

  if(!opq->param[1]) {
    /* Check if we are moveing an immediate */
    if(param_t[2] == T_NONE) {
      /* Yep, an immediate */
      mov_t_imm[param_t[0]](opq, 1, opq->param[2]);
      return;
    }
    /* Just another move */
    if(opq->param[0] != opq->param[2])
      move_t_t[param_t[0]][param_t[2]](opq, 1);
    return;
  }

  /* Ok, This _IS_ an add... */
  if(param_t[2] == T_NONE)
    /* immediate */
    l_add_imm_t_table[param_t[0]][param_t[1]](opq, 1, opq->param[2]);
  else
    l_add_t_table[param_t[0]][param_t[1]][param_t[2]](opq, 1);
}

DEF_3T_OP(generic_gen_op, l_addc_t_table, gen_op_addc);

void gen_l_addc(struct op_queue *opq, int param_t[3], int delay_slot)
{
  if(!opq->param[0])
    /* Screw this, the operation shall do nothing */
    return;

  /* FIXME: More optimisations !! (...and immediate...) */
  l_addc_t_table[param_t[0]][param_t[1]][param_t[2]](opq, 1);
}

DEF_2T_OP(imm_gen_op, l_and_imm_t_table, gen_op_and_imm);
DEF_3T_OP_NEQ(generic_gen_op, l_and_t_table, gen_op_and);

void gen_l_and(struct op_queue *opq, int param_t[3], int delay_slot)
{
  if(!opq->param[0])
    /* Screw this, the operation shall do nothing */
    return;

  if(!opq->param[1] || !opq->param[2]) {
    /* Just clear param_t[0] */
    clear_t[param_t[0]](opq, 1);
    return;
  }

  if((opq->param[0] == opq->param[1]) &&
     (opq->param[0] == opq->param[2]) &&
     (param_t[2] != T_NONE))
    return;

  if(param_t[2] == T_NONE)
    l_and_imm_t_table[param_t[0]][param_t[1]](opq, 1, opq->param[2]);
  else
    l_and_t_table[param_t[0]][param_t[1]][param_t[2]](opq, 1);
}

void gen_l_bf(struct op_queue *opq, int param_t[3], int delay_slot)
{
  if(do_stats)
    /* All gprs are current since this insn doesn't touch any reg */
    gen_op_analysis(opq, 1);

  /* The temporaries are expected to be shiped out after the execution of the
   * branch instruction wether it branched or not */
  ship_gprs_out_t(opq->prev);

  if(IADDR_PAGE(opq->insn_addr) != IADDR_PAGE(opq->insn_addr + 4)) {
    gen_op_check_flag_delay(opq, 1, opq->param[0] << 2);
    gen_op_do_sched(opq, 1);
    opq->not_jump_loc = -1;
    return;
  }
 
  gen_op_check_flag(opq, 1, 0);
  opq->not_jump_loc = opq->num_ops_param - 1;

  gen_j_imm(opq, opq->param[0]);
}

void gen_l_bnf(struct op_queue *opq, int param_t[3], int delay_slot)
{
  if(do_stats)
    /* All gprs are current since this insn doesn't touch any reg */
    gen_op_analysis(opq, 1);

  /* The temporaries are expected to be shiped out after the execution of the
   * branch instruction wether it branched or not */
  ship_gprs_out_t(opq->prev);

  if(IADDR_PAGE(opq->insn_addr) != IADDR_PAGE(opq->insn_addr + 4)) {
    gen_op_check_not_flag_delay(opq, 1, opq->param[0] << 2);
    gen_op_do_sched(opq, 1);
    opq->not_jump_loc = -1;
    return;
  }

  gen_op_check_not_flag(opq, 1, 0);
  opq->not_jump_loc = opq->num_ops_param - 1;

  gen_j_imm(opq, opq->param[0]);
}

DEF_3T_OP_NEQ(generic_gen_op, l_cmov_t_table, gen_op_cmov);

/* FIXME: Check if either opperand 1 or 2 is r0 */
void gen_l_cmov(struct op_queue *opq, int param_t[3], int delay_slot)
{
  if(!opq->param[0])
    return;

  if(!opq->param[1] && !opq->param[2]) {
    clear_t[param_t[0]](opq, 1);
    return;
  }

  if((opq->param[1] == opq->param[2]) && (opq->param[0] == opq->param[1]))
    return;

  if(opq->param[1] == opq->param[2]) {
    move_t_t[param_t[0]][param_t[1]](opq, 1);
    return;
  }

  l_cmov_t_table[param_t[0]][param_t[1]][param_t[2]](opq, 1);
}

void gen_l_cust1(struct op_queue *opq, int param_t[3], int delay_slot)
{
}

void gen_l_cust2(struct op_queue *opq, int param_t[3], int delay_slot)
{
}

void gen_l_cust3(struct op_queue *opq, int param_t[3], int delay_slot)
{
}

void gen_l_cust4(struct op_queue *opq, int param_t[3], int delay_slot)
{
}

void gen_l_cust5(struct op_queue *opq, int param_t[3], int delay_slot)
{
}

void gen_l_cust6(struct op_queue *opq, int param_t[3], int delay_slot)
{
}

void gen_l_cust7(struct op_queue *opq, int param_t[3], int delay_slot)
{
}

void gen_l_cust8(struct op_queue *opq, int param_t[3], int delay_slot)
{
}

/* FIXME: All registers need to be stored before the div instructions as they
 * have the potenticial to cause an exception */

DEF_1T_OP(generic_gen_op, check_null_excpt, gen_op_check_null_except);
DEF_1T_OP(generic_gen_op, check_null_excpt_delay, gen_op_check_null_except_delay);
DEF_3T_OP(generic_gen_op, l_div_t_table, gen_op_div);

void gen_l_div(struct op_queue *opq, int param_t[3], int delay_slot)
{
  if(!opq->param[2]) {
    /* There is no option.  This _will_ cause an illeagal exception */
    if(!delay_slot) {
      gen_op_illegal(opq, 1);
      gen_op_do_jump(opq, 1);
    } else {
      gen_op_illegal(opq, 1);
      gen_op_do_jump(opq, 1);
    }
    return;
  }

  if(!delay_slot)
    check_null_excpt[param_t[2]](opq, 1);
  else
    check_null_excpt_delay[param_t[2]](opq, 1);

  if(!opq->param[0])
    return;

  if(!opq->param[1]) {
    /* Clear param_t[0] */
    clear_t[param_t[0]](opq, 1);
    return;
  }
    
  l_div_t_table[param_t[0]][param_t[1]][param_t[2]](opq, 1);
}

DEF_3T_OP(generic_gen_op, l_divu_t_table, gen_op_divu);

void gen_l_divu(struct op_queue *opq, int param_t[3], int delay_slot)
{
  if(!opq->param[2]) {
    /* There is no option.  This _will_ cause an illeagal exception */
    if(!delay_slot) {
      gen_op_illegal(opq, 1);
      gen_op_do_jump(opq, 1);
    } else {
      gen_op_illegal(opq, 1);
      gen_op_do_jump(opq, 1);
    }
    return;
  }

  if(!delay_slot)
    check_null_excpt[param_t[2]](opq, 1);
  else
    check_null_excpt_delay[param_t[2]](opq, 1);

  if(!opq->param[0])
    return;

  if(!opq->param[1]) {
    /* Clear param_t[0] */
    clear_t[param_t[0]](opq, 1);
    return;
  }
 
  l_divu_t_table[param_t[0]][param_t[1]][param_t[2]](opq, 1);
}

DEF_2T_OP(generic_gen_op, l_extbs_t_table, gen_op_extbs);

void gen_l_extbs(struct op_queue *opq, int param_t[3], int delay_slot)
{
  if(!opq->param[0])
    return;

  if(!opq->param[1]) {
    clear_t[param_t[0]](opq, 1);
    return;
  }

  l_extbs_t_table[param_t[0]][param_t[1]](opq, 1);
}

DEF_2T_OP(generic_gen_op, l_extbz_t_table, gen_op_extbz);

void gen_l_extbz(struct op_queue *opq, int param_t[3], int delay_slot)
{
  if(!opq->param[0])
    return;

  if(!opq->param[1]) {
    clear_t[param_t[0]](opq, 1);
    return;
  }

  l_extbz_t_table[param_t[0]][param_t[1]](opq, 1);
}

DEF_2T_OP(generic_gen_op, l_exths_t_table, gen_op_exths);

void gen_l_exths(struct op_queue *opq, int param_t[3], int delay_slot)
{
  if(!opq->param[0])
    return;

  if(!opq->param[1]) {
    clear_t[param_t[0]](opq, 1);
    return;
  }

  l_exths_t_table[param_t[0]][param_t[1]](opq, 1);
}

DEF_2T_OP(generic_gen_op, l_exthz_t_table, gen_op_exthz);

void gen_l_exthz(struct op_queue *opq, int param_t[3], int delay_slot)
{
  if(!opq->param[0])
    return;

  if(!opq->param[1]) {
    clear_t[param_t[0]](opq, 1);
    return;
  }

  l_exthz_t_table[param_t[0]][param_t[1]](opq, 1);
}

void gen_l_extws(struct op_queue *opq, int param_t[3], int delay_slot)
{
  if(!opq->param[0])
    return;

  if(!opq->param[1]) {
    clear_t[param_t[0]](opq, 1);
    return;
  }

  if(opq->param[0] == opq->param[1])
    return;

  /* In the 32-bit architechture this instruction reduces to a move */
  move_t_t[param_t[0]][param_t[1]](opq, 1);
}

void gen_l_extwz(struct op_queue *opq, int param_t[3], int delay_slot)
{
  if(!opq->param[0])
    return;

  if(!opq->param[1]) {
    clear_t[param_t[0]](opq, 1);
    return;
  }

  if(opq->param[0] == opq->param[1])
    return;

  /* In the 32-bit architechture this instruction reduces to a move */
  move_t_t[param_t[0]][param_t[1]](opq, 1);
}

DEF_2T_OP(generic_gen_op, l_ff1_t_table, gen_op_ff1);

void gen_l_ff1(struct op_queue *opq, int param_t[3], int delay_slot)
{
  if(!opq->param[0])
    return;

  if(!opq->param[1]) {
    clear_t[param_t[0]](opq, 1);
    return;
  }

  l_ff1_t_table[param_t[0]][param_t[1]](opq, 1);
}

void gen_l_j(struct op_queue *opq, int param_t[3], int delay_slot)
{
  if(do_stats)
    /* All gprs are current since this insn doesn't touch any reg */
    gen_op_analysis(opq, 1);

  gen_j_imm(opq, opq->param[0]);
}

void gen_l_jal(struct op_queue *opq, int param_t[3], int delay_slot)
{
  int i;

  /* In the case of a l.jal instruction, make sure that LINK_REGNO is not in
   * a temporary.  The problem is that the l.jal(r) instruction stores the
   * `return address' in LINK_REGNO.  The temporaries are shiped out only
   * after the delay slot instruction has executed and so it overwrittes the
   * `return address'. */
  for(i = 0; i < NUM_T_REGS; i++) {
    if(opq->reg_t[i] == LINK_REGNO) {
      /* Don't bother storeing the register, it is going to get clobered in this
       * instruction anyway */
      opq->reg_t[i] = 32;
      break;
    }
  }

  /* Store the return address */
  gen_op_store_link_addr_gpr(opq, 1);

  if(do_stats)
    /* All gprs are current since this insn doesn't touch any reg */
    gen_op_analysis(opq, 1);

  gen_j_imm(opq, opq->param[0]);
}

void gen_l_jr(struct op_queue *opq, int param_t[3], int delay_slot)
{
  gen_j_reg(opq, opq->param[0]);
}

void gen_l_jalr(struct op_queue *opq, int param_t[3], int delay_slot)
{
  int i;

  /* In the case of a l.jal instruction, make sure that LINK_REGNO is not in
   * a temporary.  The problem is that the l.jal(r) instruction stores the
   * `return address' in LINK_REGNO.  The temporaries are shiped out only
   * after the delay slot instruction has executed and so it overwrittes the
   * `return address'. */
  for(i = 0; i < NUM_T_REGS; i++) {
    if(opq->reg_t[i] == LINK_REGNO) {
      /* Don't bother storeing the register, it is going to get clobered in this
       * instruction anyway */
      opq->reg_t[i] = 32;
      break;
    }
  }

  /* Store the return address */
  gen_op_store_link_addr_gpr(opq, 1);

  gen_j_reg(opq, opq->param[0]);
}

/* FIXME: Optimise all load instruction when the disposition == 0 */

DEF_1T_OP(imm_gen_op, l_lbs_imm_t_table, gen_op_lbs_imm);
DEF_2T_OP(imm_gen_op, l_lbs_t_table, gen_op_lbs);

void gen_l_lbs(struct op_queue *opq, int param_t[3], int delay_slot)
{
  if(!opq->param[0]) {
    /* FIXME: This will work, but the statistics need to be updated... */
    return;
  }

  /* Just in case an exception happens */
  ship_gprs_out_t(opq->prev);

  if(!opq->param[2]) {
    /* Load the data from the immediate */
    l_lbs_imm_t_table[param_t[0]](opq, 1, opq->param[1]);
    return;
  }

  l_lbs_t_table[param_t[0]][param_t[2]](opq, 1, opq->param[1]);
}

DEF_1T_OP(imm_gen_op, l_lbz_imm_t_table, gen_op_lbz_imm);
DEF_2T_OP(imm_gen_op, l_lbz_t_table, gen_op_lbz);

void gen_l_lbz(struct op_queue *opq, int param_t[3], int delay_slot)
{
  if(!opq->param[0]) {
    /* FIXME: This will work, but the statistics need to be updated... */
    return;
  }

  /* Just in case an exception happens */
  ship_gprs_out_t(opq->prev);

  if(!opq->param[2]) {
    /* Load the data from the immediate */
    l_lbz_imm_t_table[param_t[0]](opq, 1, opq->param[1]);
    return;
  }

  l_lbz_t_table[param_t[0]][param_t[2]](opq, 1, opq->param[1]);
}

DEF_1T_OP(imm_gen_op, l_lhs_imm_t_table, gen_op_lhs_imm);
DEF_2T_OP(imm_gen_op, l_lhs_t_table, gen_op_lhs);

void gen_l_lhs(struct op_queue *opq, int param_t[3], int delay_slot)
{
  if(!opq->param[0]) {
    /* FIXME: This will work, but the statistics need to be updated... */
    return;
  }

  /* Just in case an exception happens */
  ship_gprs_out_t(opq->prev);

  if(!opq->param[2]) {
    /* Load the data from the immediate */
    l_lhs_imm_t_table[param_t[0]](opq, 1, opq->param[1]);
    return;
  }

  l_lhs_t_table[param_t[0]][param_t[2]](opq, 1, opq->param[1]);
}

DEF_1T_OP(imm_gen_op, l_lhz_imm_t_table, gen_op_lhz_imm);
DEF_2T_OP(imm_gen_op, l_lhz_t_table, gen_op_lhz);

void gen_l_lhz(struct op_queue *opq, int param_t[3], int delay_slot)
{
  if(!opq->param[0]) {
    /* FIXME: This will work, but the statistics need to be updated... */
    return;
  }

  /* Just in case an exception happens */
  ship_gprs_out_t(opq->prev);

  if(!opq->param[2]) {
    /* Load the data from the immediate */
    l_lhz_imm_t_table[param_t[0]](opq, 1, opq->param[1]);
    return;
  }

  l_lhz_t_table[param_t[0]][param_t[2]](opq, 1, opq->param[1]);
}

DEF_1T_OP(imm_gen_op, l_lws_imm_t_table, gen_op_lws_imm);
DEF_2T_OP(imm_gen_op, l_lws_t_table, gen_op_lws);

void gen_l_lws(struct op_queue *opq, int param_t[3], int delay_slot)
{
  if(!opq->param[0]) {
    /* FIXME: This will work, but the statistics need to be updated... */
    return;
  }

  /* Just in case an exception happens */
  ship_gprs_out_t(opq->prev);

  if(!opq->param[2]) {
    /* Load the data from the immediate */
    l_lws_imm_t_table[param_t[0]](opq, 1, opq->param[1]);
    return;
  }

  l_lws_t_table[param_t[0]][param_t[2]](opq, 1, opq->param[1]);
}

DEF_1T_OP(imm_gen_op, l_lwz_imm_t_table, gen_op_lwz_imm);
DEF_2T_OP(imm_gen_op, l_lwz_t_table, gen_op_lwz);

void gen_l_lwz(struct op_queue *opq, int param_t[3], int delay_slot)
{
  if(!opq->param[0]) {
    /* FIXME: This will work, but the statistics need to be updated... */
    return;
  }

  /* Just in case an exception happens */
  ship_gprs_out_t(opq->prev);

  if(!opq->param[2]) {
    /* Load the data from the immediate */
    l_lwz_imm_t_table[param_t[0]](opq, 1, opq->param[1]);
    return;
  }

  l_lwz_t_table[param_t[0]][param_t[2]](opq, 1, opq->param[1]);
}

DEF_1T_OP(imm_gen_op, l_mac_imm_t_table, gen_op_mac_imm);
DEF_2T_OP(generic_gen_op, l_mac_t_table, gen_op_mac);

void gen_l_mac(struct op_queue *opq, int param_t[3], int delay_slot)
{
  if(!opq->param[0] || !opq->param[1])
    return;

  if(param_t[1] == T_NONE)
    l_mac_imm_t_table[param_t[0]](opq, 1, opq->param[1]);
  else
    l_mac_t_table[param_t[0]][param_t[1]](opq, 1);
}

DEF_1T_OP(generic_gen_op, l_macrc_t_table, gen_op_macrc);

void gen_l_macrc(struct op_queue *opq, int param_t[3], int delay_slot)
{
  if(!opq->param[0]) {
    gen_op_macc(opq, 1);
    return;
  }

  l_macrc_t_table[param_t[0]](opq, 1);
}

DEF_1T_OP(imm_gen_op, l_mfspr_imm_t_table, gen_op_mfspr_imm);
DEF_2T_OP(imm_gen_op, l_mfspr_t_table, gen_op_mfspr);

void gen_l_mfspr(struct op_queue *opq, int param_t[3], int delay_slot)
{
  if(!opq->param[0])
    return;

  if(!opq->param[1]) {
    l_mfspr_imm_t_table[param_t[0]](opq, 1, opq->param[2]);
    return;
  }

  l_mfspr_t_table[param_t[0]][param_t[1]](opq, 1, opq->param[2]);
}

void gen_l_movhi(struct op_queue *opq, int param_t[3], int delay_slot)
{
  if(!opq->param[0])
    return;

  if(!opq->param[1]) {
    clear_t[param_t[0]](opq, 1);
    return;
  }

  mov_t_imm[param_t[0]](opq, 1, opq->param[1] << 16);
}

DEF_2T_OP(generic_gen_op, l_msb_t_table, gen_op_msb);

void gen_l_msb(struct op_queue *opq, int param_t[3], int delay_slot)
{
  if(!opq->param[0] || !opq->param[1])
    return;

  l_msb_t_table[param_t[0]][param_t[1]](opq, 1);
}

DEF_1T_OP(imm_gen_op, l_mtspr_clear_t_table, gen_op_mtspr_clear);
DEF_1T_OP(imm_gen_op, l_mtspr_imm_t_table, gen_op_mtspr_imm);
DEF_2T_OP(imm_gen_op, l_mtspr_t_table, gen_op_mtspr);

void gen_l_mtspr(struct op_queue *opq, int param_t[3], int delay_slot)
{
  /* Just in case an exception happens */
  ship_gprs_out_t(opq->prev);

  if(!opq->param[0]) {
    if(!opq->param[1]) {
      /* Clear the immediate SPR */
      gen_op_mtspr_imm_clear(opq, 1, opq->param[2]);
      return;
    }
    l_mtspr_imm_t_table[param_t[1]](opq, 1, opq->param[2]);
    return;
  }

  if(!opq->param[1]) {
    l_mtspr_clear_t_table[param_t[0]](opq, 1, opq->param[2]);
    return;
  }

  l_mtspr_t_table[param_t[0]][param_t[1]](opq, 1, opq->param[2]);
}

DEF_2T_OP(imm_gen_op, l_mul_imm_t_table, gen_op_mul_imm);
DEF_3T_OP(generic_gen_op, l_mul_t_table, gen_op_mul);

void gen_l_mul(struct op_queue *opq, int param_t[3], int delay_slot)
{
  if(!opq->param[0])
    return;

  if(!opq->param[1] || !opq->param[2]) {
    clear_t[param_t[0]](opq, 1);
    return;
  }

  if(param_t[2] == T_NONE)
    l_mul_imm_t_table[param_t[0]][param_t[1]](opq, 1, opq->param[2]);
  else
    l_mul_t_table[param_t[0]][param_t[1]][param_t[2]](opq, 1);
}

DEF_3T_OP(generic_gen_op, l_mulu_t_table, gen_op_mulu);

void gen_l_mulu(struct op_queue *opq, int param_t[3], int delay_slot)
{
  if(!opq->param[0])
    return;

  if(!opq->param[1] || !opq->param[2]) {
    clear_t[param_t[0]](opq, 1);
    return;
  }

  l_mulu_t_table[param_t[0]][param_t[1]][param_t[2]](opq, 1);
}

void gen_l_nop(struct op_queue *opq, int param_t[3], int delay_slot)
{
  /* Do parameter switch now */
  switch(opq->param[0]) {
  case NOP_NOP:
    break;
  case NOP_EXIT:
    ship_gprs_out_t(opq->prev);
    gen_op_nop_exit(opq, 1);
    break;
  case NOP_CNT_RESET:
    gen_op_nop_reset(opq, 1);
    break;    
  case NOP_PRINTF:
    ship_gprs_out_t(opq->prev);
    gen_op_nop_printf(opq, 1);
    break;
  case NOP_REPORT:
    ship_gprs_out_t(opq->prev);
    gen_op_nop_report(opq, 1);
    break;
  default:
    if((opq->param[0] >= NOP_REPORT_FIRST) && (opq->param[0] <= NOP_REPORT_LAST)) {
      ship_gprs_out_t(opq->prev);
      gen_op_nop_report_imm(opq, 1, opq->param[0] - NOP_REPORT_FIRST);
    }
    break;
  }
}

DEF_2T_OP(imm_gen_op, l_or_imm_t_table, gen_op_or_imm);
DEF_3T_OP_NEQ(generic_gen_op, l_or_t_table, gen_op_or);

void gen_l_or(struct op_queue *opq, int param_t[3], int delay_slot)
{
  if(!opq->param[0])
    return;

  if((opq->param[0] == opq->param[1]) &&
     (opq->param[0] == opq->param[2]) &&
     (param_t[2] != T_NONE))
    return;

  if(!opq->param[1] && !opq->param[2]) {
    clear_t[param_t[0]](opq, 1);
    return;
  }

  if(!opq->param[2]) {
    if((param_t[2] == T_NONE) && (opq->param[0] == opq->param[1]))
      return;
    move_t_t[param_t[0]][param_t[1]](opq, 1);
    return;
  }

  if(!opq->param[1]) {
    /* Check if we are moveing an immediate */
    if(param_t[2] == T_NONE) {
      /* Yep, an immediate */
      mov_t_imm[param_t[0]](opq, 1, opq->param[2]);
      return;
    }
    /* Just another move */
    move_t_t[param_t[0]][param_t[2]](opq, 1);
    return;
  }

  if(param_t[2] == T_NONE)
    l_or_imm_t_table[param_t[0]][param_t[1]](opq, 1, opq->param[2]);
  else
    l_or_t_table[param_t[0]][param_t[1]][param_t[2]](opq, 1);
}

void gen_l_rfe(struct op_queue *opq, int param_t[3], int delay_slot)
{
  if(do_stats)
    /* All gprs are current since this insn doesn't touch any reg */
    gen_op_analysis(opq, 1);

  gen_op_prep_rfe(opq, 1);
  /* FIXME: rename op_do_sched_delay */
  gen_op_do_sched_delay(opq, 1);
  gen_op_do_jump(opq, 1);
}

/* FIXME: All store instructions should be optimised when the disposition = 0 */

DEF_1T_OP(imm_gen_op, l_sb_clear_table, gen_op_sb_clear);
DEF_1T_OP(imm_gen_op, l_sb_imm_t_table, gen_op_sb_imm);
DEF_2T_OP(imm_gen_op, l_sb_t_table, gen_op_sb);

void gen_l_sb(struct op_queue *opq, int param_t[3], int delay_slot)
{
  /* Just in case an exception happens */
  ship_gprs_out_t(opq->prev);

  if(!opq->param[2]) {
    if(!opq->param[1]) {
      gen_op_sb_clear_imm(opq, 1, opq->param[0]);
      return;
    }
    l_sb_clear_table[param_t[1]](opq, 1, opq->param[0]);
    return;
  }

  if(!opq->param[1]) {
    /* Store the data to the immediate */
    l_sb_imm_t_table[param_t[2]](opq, 1, opq->param[0]);
    return;
  }

  l_sb_t_table[param_t[1]][param_t[2]](opq, 1, opq->param[0]);
}

DEF_1T_OP(imm_gen_op, l_sh_clear_table, gen_op_sh_clear);
DEF_1T_OP(imm_gen_op, l_sh_imm_t_table, gen_op_sh_imm);
DEF_2T_OP(imm_gen_op, l_sh_t_table, gen_op_sh);

void gen_l_sh(struct op_queue *opq, int param_t[3], int delay_slot)
{
  /* Just in case an exception happens */
  ship_gprs_out_t(opq->prev);

  if(!opq->param[2]) {
    if(!opq->param[1]) {
      gen_op_sh_clear_imm(opq, 1, opq->param[0]);
      return;
    }
    l_sh_clear_table[param_t[1]](opq, 1, opq->param[0]);
    return;
  }

  if(!opq->param[1]) {
    /* Store the data to the immediate */
    l_sh_imm_t_table[param_t[2]](opq, 1, opq->param[0]);
    return;
  }

  l_sh_t_table[param_t[1]][param_t[2]](opq, 1, opq->param[0]);
}

DEF_1T_OP(imm_gen_op, l_sw_clear_table, gen_op_sw_clear);
DEF_1T_OP(imm_gen_op, l_sw_imm_t_table, gen_op_sw_imm);
DEF_2T_OP(imm_gen_op, l_sw_t_table, gen_op_sw);

void gen_l_sw(struct op_queue *opq, int param_t[3], int delay_slot)
{
  /* Just in case an exception happens */
  ship_gprs_out_t(opq->prev);

  if(!opq->param[2]) {
    if(!opq->param[1]) {
      gen_op_sw_clear_imm(opq, 1, opq->param[0]);
      return;
    }
    l_sw_clear_table[param_t[1]](opq, 1, opq->param[0]);
    return;
  }

  if(!opq->param[1]) {
    /* Store the data to the immediate */
    l_sw_imm_t_table[param_t[2]](opq, 1, opq->param[0]);
    return;
  }

  l_sw_t_table[param_t[1]][param_t[2]](opq, 1, opq->param[0]);
}

DEF_1T_OP(generic_gen_op, l_sfeq_null_t_table, gen_op_sfeq_null);
DEF_1T_OP(imm_gen_op, l_sfeq_imm_t_table, gen_op_sfeq_imm);
DEF_2T_OP(generic_gen_op, l_sfeq_t_table, gen_op_sfeq);

void gen_l_sfeq(struct op_queue *opq, int param_t[3], int delay_slot)
{
  if(!opq->param[0] && !opq->param[1]) {
    gen_op_set_flag(opq, 1);
    return;
  }

  if(!opq->param[0]) {
    if(param_t[1] == T_NONE) {
      if(!opq->param[1])
        gen_op_set_flag(opq, 1);
      else
        gen_op_clear_flag(opq, 1);
    } else
      l_sfeq_null_t_table[param_t[1]](opq, 1);
    return;
  }

  if(!opq->param[1]) {
    l_sfeq_null_t_table[param_t[0]](opq, 1);
    return;
  }

  if(param_t[1] == T_NONE)
    l_sfeq_imm_t_table[param_t[0]](opq, 1, opq->param[1]);
  else
    l_sfeq_t_table[param_t[0]][param_t[1]](opq, 1);
}

DEF_1T_OP(generic_gen_op, l_sfges_null_t_table, gen_op_sfges_null);
DEF_1T_OP(generic_gen_op, l_sfles_null_t_table, gen_op_sfles_null);
DEF_1T_OP(imm_gen_op, l_sfges_imm_t_table, gen_op_sfges_imm);
DEF_2T_OP(generic_gen_op, l_sfges_t_table, gen_op_sfges);

void gen_l_sfges(struct op_queue *opq, int param_t[3], int delay_slot)
{
  if(!opq->param[0] && !opq->param[1]) {
    gen_op_set_flag(opq, 1);
    return;
  }

  if(!opq->param[0]) {
    /* sfles IS correct */
    if(param_t[1] == T_NONE) {
      if(0 >= (orreg_t)opq->param[1])
        gen_op_set_flag(opq, 1);
      else
        gen_op_clear_flag(opq, 1);
    } else
      l_sfles_null_t_table[param_t[1]](opq, 1);
    return;
  }

  if(!opq->param[1]) {
    l_sfges_null_t_table[param_t[0]](opq, 1);
    return;
  }

  if(param_t[1] == T_NONE)
    l_sfges_imm_t_table[param_t[0]](opq, 1, opq->param[1]);
  else
    l_sfges_t_table[param_t[0]][param_t[1]](opq, 1);
}

DEF_1T_OP(generic_gen_op, l_sfgeu_null_t_table, gen_op_sfgeu_null);
DEF_1T_OP(generic_gen_op, l_sfleu_null_t_table, gen_op_sfleu_null);
DEF_1T_OP(imm_gen_op, l_sfgeu_imm_t_table, gen_op_sfgeu_imm);
DEF_2T_OP(generic_gen_op, l_sfgeu_t_table, gen_op_sfgeu);

void gen_l_sfgeu(struct op_queue *opq, int param_t[3], int delay_slot)
{
  if(!opq->param[0] && !opq->param[1]) {
    gen_op_set_flag(opq, 1);
    return;
  }

  if(!opq->param[0]) {
    /* sfleu IS correct */
    if(param_t[1] == T_NONE) {
      if(0 >= opq->param[1])
        gen_op_set_flag(opq, 1);
      else
        gen_op_clear_flag(opq, 1);
    } else
      l_sfleu_null_t_table[param_t[1]](opq, 1);
    return;
  }

  if(!opq->param[1]) {
    l_sfgeu_null_t_table[param_t[0]](opq, 1);
    return;
  }
  if(param_t[1] == T_NONE)
    l_sfgeu_imm_t_table[param_t[0]](opq, 1, opq->param[1]);
  else
    l_sfgeu_t_table[param_t[0]][param_t[1]](opq, 1);
}

DEF_1T_OP(generic_gen_op, l_sfgts_null_t_table, gen_op_sfgts_null);
DEF_1T_OP(generic_gen_op, l_sflts_null_t_table, gen_op_sflts_null);
DEF_1T_OP(imm_gen_op, l_sfgts_imm_t_table, gen_op_sfgts_imm);
DEF_2T_OP(generic_gen_op, l_sfgts_t_table, gen_op_sfgts);

void gen_l_sfgts(struct op_queue *opq, int param_t[3], int delay_slot)
{
  if(!opq->param[0] && !opq->param[1]) {
    gen_op_clear_flag(opq, 1);
    return;
  }

  if(!opq->param[0]) {
    /* sflts IS correct */
    if(param_t[1] == T_NONE) {
      if(0 > (orreg_t)opq->param[1])
        gen_op_set_flag(opq, 1);
      else
        gen_op_clear_flag(opq, 1);
    } else
      l_sflts_null_t_table[param_t[1]](opq, 1);
    return;
  }

  if(!opq->param[1]) {
    l_sfgts_null_t_table[param_t[0]](opq, 1);
    return;
  }

  if(param_t[1] == T_NONE)
    l_sfgts_imm_t_table[param_t[0]](opq, 1, opq->param[1]);
  else
    l_sfgts_t_table[param_t[0]][param_t[1]](opq, 1);
}

DEF_1T_OP(generic_gen_op, l_sfgtu_null_t_table, gen_op_sfgtu_null);
DEF_1T_OP(generic_gen_op, l_sfltu_null_t_table, gen_op_sfltu_null);
DEF_1T_OP(imm_gen_op, l_sfgtu_imm_t_table, gen_op_sfgtu_imm);
DEF_2T_OP(generic_gen_op, l_sfgtu_t_table, gen_op_sfgtu);

void gen_l_sfgtu(struct op_queue *opq, int param_t[3], int delay_slot)
{
  if(!opq->param[0] && !opq->param[1]) {
    gen_op_clear_flag(opq, 1);
    return;
  }

  if(!opq->param[0]) {
    /* sfltu IS correct */
    if(param_t[1] == T_NONE) {
      if(0 > opq->param[1])
        gen_op_set_flag(opq, 1);
      else
        gen_op_clear_flag(opq, 1);
    } else
      l_sfltu_null_t_table[param_t[1]](opq, 1);
    return;
  }

  if(!opq->param[1]) {
    l_sfgtu_null_t_table[param_t[0]](opq, 1);
    return;
  }

  if(param_t[1] == T_NONE)
    l_sfgtu_imm_t_table[param_t[0]](opq, 1, opq->param[1]);
  else
    l_sfgtu_t_table[param_t[0]][param_t[1]](opq, 1);
}

DEF_1T_OP(imm_gen_op, l_sfles_imm_t_table, gen_op_sfles_imm);
DEF_2T_OP(generic_gen_op, l_sfles_t_table, gen_op_sfles);

void gen_l_sfles(struct op_queue *opq, int param_t[3], int delay_slot)
{
  if(!opq->param[0] && !opq->param[1]) {
    gen_op_set_flag(opq, 1);
    return;
  }

  if(!opq->param[0]) {
    /* sfges IS correct */
    if(param_t[1] == T_NONE) {
      if(0 <= (orreg_t)opq->param[1])
        gen_op_set_flag(opq, 1);
      else
        gen_op_clear_flag(opq, 1);
    } else
      l_sfges_null_t_table[param_t[1]](opq, 1);
    return;
  }

  if(!opq->param[1]) {
    l_sfles_null_t_table[param_t[0]](opq, 1);
    return;
  }

  if(param_t[1] == T_NONE)
    l_sfles_imm_t_table[param_t[0]](opq, 1, opq->param[1]);
  else
    l_sfles_t_table[param_t[0]][param_t[1]](opq, 1);
}

DEF_1T_OP(imm_gen_op, l_sfleu_imm_t_table, gen_op_sfleu_imm);
DEF_2T_OP(generic_gen_op, l_sfleu_t_table, gen_op_sfleu);

void gen_l_sfleu(struct op_queue *opq, int param_t[3], int delay_slot)
{
  if(!opq->param[0] && !opq->param[1]) {
    gen_op_set_flag(opq, 1);
    return;
  }

  if(!opq->param[0]) {
    /* sfleu IS correct */
    if(param_t[1] == T_NONE) {
      if(0 <= opq->param[1])
        gen_op_set_flag(opq, 1);
      else
        gen_op_clear_flag(opq, 1);
    } else
      l_sfgeu_null_t_table[param_t[1]](opq, 1);
    return;
  }

  if(!opq->param[1]) {
    l_sfleu_null_t_table[param_t[0]](opq, 1);
    return;
  }

  if(param_t[1] == T_NONE)
    l_sfleu_imm_t_table[param_t[0]](opq, 1, opq->param[1]);
  else
    l_sfleu_t_table[param_t[0]][param_t[1]](opq, 1);
}

DEF_1T_OP(imm_gen_op, l_sflts_imm_t_table, gen_op_sflts_imm);
DEF_2T_OP(generic_gen_op, l_sflts_t_table, gen_op_sflts);

void gen_l_sflts(struct op_queue *opq, int param_t[3], int delay_slot)
{
  if(!opq->param[0] && !opq->param[1]) {
    gen_op_clear_flag(opq, 1);
    return;
  }

  if(!opq->param[0]) {
    /* sfgts IS correct */
    if(param_t[1] == T_NONE) {
      if(0 < (orreg_t)opq->param[1])
        gen_op_set_flag(opq, 1);
      else
        gen_op_clear_flag(opq, 1);
    } else
      l_sfgts_null_t_table[param_t[1]](opq, 1);
    return;
  }

  if(!opq->param[1]) {
    l_sflts_null_t_table[param_t[0]](opq, 1);
    return;
  }

  if(param_t[1] == T_NONE)
    l_sflts_imm_t_table[param_t[0]](opq, 1, opq->param[1]);
  else
    l_sflts_t_table[param_t[0]][param_t[1]](opq, 1);
}

DEF_1T_OP(imm_gen_op, l_sfltu_imm_t_table, gen_op_sfltu_imm);
DEF_2T_OP(generic_gen_op, l_sfltu_t_table, gen_op_sfltu);

void gen_l_sfltu(struct op_queue *opq, int param_t[3], int delay_slot)
{
  if(!opq->param[0] && !opq->param[1]) {
    gen_op_clear_flag(opq, 1);
    return;
  }

  if(!opq->param[0]) {
    /* sfgtu IS correct */
    if(param_t[1] == T_NONE) {
      if(0 < opq->param[1])
        gen_op_set_flag(opq, 1);
      else
        gen_op_clear_flag(opq, 1);
    } else
      l_sfgtu_null_t_table[param_t[1]](opq, 1);
    return;
  }

  if(!opq->param[1]) {
    l_sfltu_null_t_table[param_t[0]](opq, 1);
    return;
  }

  if(param_t[1] == T_NONE)
    l_sfltu_imm_t_table[param_t[0]](opq, 1, opq->param[1]);
  else
    l_sfltu_t_table[param_t[0]][param_t[1]](opq, 1);
}

DEF_1T_OP(generic_gen_op, l_sfne_null_t_table, gen_op_sfne_null);
DEF_1T_OP(imm_gen_op, l_sfne_imm_t_table, gen_op_sfne_imm);
DEF_2T_OP(generic_gen_op, l_sfne_t_table, gen_op_sfne);

void gen_l_sfne(struct op_queue *opq, int param_t[3], int delay_slot)
{
  if(!opq->param[0] && !opq->param[1]) {
    gen_op_set_flag(opq, 1);
    return;
  }

  if(!opq->param[0]) {
    if(param_t[1] == T_NONE)
      if(opq->param[1])
        gen_op_set_flag(opq, 1);
      else
        gen_op_clear_flag(opq, 1);
    else
      l_sfne_null_t_table[param_t[1]](opq, 1);
    return;
  }

  if(!opq->param[1]) {
    l_sfne_null_t_table[param_t[0]](opq, 1);
    return;
  }

  if(param_t[1] == T_NONE)
    l_sfne_imm_t_table[param_t[0]](opq, 1, opq->param[1]);
  else
    l_sfne_t_table[param_t[0]][param_t[1]](opq, 1);
}

DEF_2T_OP(imm_gen_op, l_sll_imm_t_table, gen_op_sll_imm);
DEF_3T_OP(generic_gen_op, l_sll_t_table, gen_op_sll);

void gen_l_sll(struct op_queue *opq, int param_t[3], int delay_slot)
{
  if(!opq->param[0])
    return;

  if(!opq->param[1]) {
    clear_t[param_t[0]](opq, 1);
    return;
  }

  if(!opq->param[2]) {
    move_t_t[param_t[0]][param_t[1]](opq, 1);
    return;
  }

  if(param_t[2] == T_NONE)
    l_sll_imm_t_table[param_t[0]][param_t[1]](opq, 1, opq->param[2]);
  else
    l_sll_t_table[param_t[0]][param_t[1]][param_t[2]](opq, 1);
}

DEF_2T_OP(imm_gen_op, l_sra_imm_t_table, gen_op_sra_imm);
DEF_3T_OP(generic_gen_op, l_sra_t_table, gen_op_sra);

void gen_l_sra(struct op_queue *opq, int param_t[3], int delay_slot)
{
  if(!opq->param[0])
    return;

  if(!opq->param[1]) {
    clear_t[param_t[0]](opq, 1);
    return;
  }

  if(!opq->param[2]) {
    move_t_t[param_t[0]][param_t[1]](opq, 1);
    return;
  }

  if(param_t[2] == T_NONE)
    l_sra_imm_t_table[param_t[0]][param_t[1]](opq, 1, opq->param[2]);
  else
    l_sra_t_table[param_t[0]][param_t[1]][param_t[2]](opq, 1);
}

DEF_2T_OP(imm_gen_op, l_srl_imm_t_table, gen_op_srl_imm);
DEF_3T_OP(generic_gen_op, l_srl_t_table, gen_op_srl);

void gen_l_srl(struct op_queue *opq, int param_t[3], int delay_slot)
{
  if(!opq->param[0])
    return;

  if(!opq->param[1]) {
    clear_t[param_t[0]](opq, 1);
    return;
  }

  if(!opq->param[2]) {
    move_t_t[param_t[0]][param_t[1]](opq, 1);
    return;
  }

  if(param_t[2] == T_NONE)
    l_srl_imm_t_table[param_t[0]][param_t[1]](opq, 1, opq->param[2]);
  else
    l_srl_t_table[param_t[0]][param_t[1]][param_t[2]](opq, 1);
}

DEF_2T_OP(generic_gen_op, l_neg_t_table, gen_op_neg);
DEF_3T_OP(generic_gen_op, l_sub_t_table, gen_op_sub);

void gen_l_sub(struct op_queue *opq, int param_t[3], int delay_slot)
{
  if(!opq->param[0])
    return;

  if((param_t[2] != T_NONE) && (opq->param[1] == opq->param[2])) {
    clear_t[param_t[0]](opq, 1);
    return;
  }

  if(!opq->param[1] && !opq->param[2]) {
    clear_t[param_t[0]](opq, 1);
    return;
  }

  if(!opq->param[1]) {
    if(param_t[2] == T_NONE)
      mov_t_imm[param_t[0]](opq, 1, -opq->param[2]);
    else
      l_neg_t_table[param_t[0]][param_t[2]](opq, 1);
    return;
  }

  if(!opq->param[2]) {
    move_t_t[param_t[0]][param_t[1]](opq, 1);
    return;
  }

  l_sub_t_table[param_t[0]][param_t[1]][param_t[2]](opq, 1);
}

/* FIXME: This will not work if the l.sys is in a delay slot */
void gen_l_sys(struct op_queue *opq, int param_t[3], int delay_slot)
{
  if(do_stats)
    /* All gprs are current since this insn doesn't touch any reg */
    gen_op_analysis(opq, 1);

  if(!delay_slot)
    gen_op_prep_sys(opq, 1);
  else
    gen_op_prep_sys_delay(opq, 1);

  gen_op_do_sched(opq, 1);
  gen_op_do_jump(opq, 1);
}

/* FIXME: This will not work if the l.trap is in a delay slot */
void gen_l_trap(struct op_queue *opq, int param_t[3], int delay_slot)
{
  if(do_stats)
    /* All gprs are current since this insn doesn't touch any reg */
    gen_op_analysis(opq, 1);

  if(!delay_slot)
    gen_op_prep_trap(opq, 1);
  else
    gen_op_prep_trap_delay(opq, 1);
}

DEF_2T_OP(imm_gen_op, l_xor_imm_t_table, gen_op_xor_imm);
/* FIXME: Make unused elements NULL */
DEF_3T_OP_NEQ(generic_gen_op, l_xor_t_table, gen_op_xor);

void gen_l_xor(struct op_queue *opq, int param_t[3], int delay_slot)
{
  if(!opq->param[0])
    return;

  if((param_t[2] != T_NONE) && (opq->param[1] == opq->param[2])) {
    clear_t[param_t[0]](opq, 1);
    return;
  }
    
  if(!opq->param[2]) {
    if((param_t[2] == T_NONE) && (opq->param[0] == opq->param[1]))
      return;
    move_t_t[param_t[0]][param_t[1]](opq, 1);
    return;
  }

  if(!opq->param[1]) {
    if(param_t[2] == T_NONE) {
      mov_t_imm[param_t[0]](opq, 1, opq->param[2]);
      return;
    }
    move_t_t[param_t[0]][param_t[2]](opq, 1);
    return;
  }

  if(param_t[2] == T_NONE)
    l_xor_imm_t_table[param_t[0]][param_t[1]](opq, 1, opq->param[2]);
  else
    l_xor_t_table[param_t[0]][param_t[1]][param_t[2]](opq, 1);
}

void gen_l_invalid(struct op_queue *opq, int param_t[3], int delay_slot)
{
  if(!delay_slot) {
    gen_op_illegal(opq, 1);
    gen_op_do_jump(opq, 1);
  } else {
    gen_op_illegal_delay(opq, 1);
    gen_op_do_jump(opq, 1);
  }
}

/*----------------------------------[ Floating point instructions (stubs) ]---*/
void gen_lf_add_s(struct op_queue *opq, int param_t[3], int delay_slot)
{
  gen_l_invalid(opq, param_t, delay_slot);
}

void gen_lf_div_s(struct op_queue *opq, int param_t[3], int delay_slot)
{
  gen_l_invalid(opq, param_t, delay_slot);
}

void gen_lf_ftoi_s(struct op_queue *opq, int param_t[3], int delay_slot)
{
  gen_l_invalid(opq, param_t, delay_slot);
}

void gen_lf_itof_s(struct op_queue *opq, int param_t[3], int delay_slot)
{
  gen_l_invalid(opq, param_t, delay_slot);
}

void gen_lf_madd_s(struct op_queue *opq, int param_t[3], int delay_slot)
{
  gen_l_invalid(opq, param_t, delay_slot);
}

void gen_lf_mul_s(struct op_queue *opq, int param_t[3], int delay_slot)
{
  gen_l_invalid(opq, param_t, delay_slot);
}

void gen_lf_rem_s(struct op_queue *opq, int param_t[3], int delay_slot)
{
  gen_l_invalid(opq, param_t, delay_slot);
}

void gen_lf_sfeq_s(struct op_queue *opq, int param_t[3], int delay_slot)
{
  gen_l_invalid(opq, param_t, delay_slot);
}

void gen_lf_sfge_s(struct op_queue *opq, int param_t[3], int delay_slot)
{
  gen_l_invalid(opq, param_t, delay_slot);
}

void gen_lf_sfgt_s(struct op_queue *opq, int param_t[3], int delay_slot)
{
  gen_l_invalid(opq, param_t, delay_slot);
}

void gen_lf_sfle_s(struct op_queue *opq, int param_t[3], int delay_slot)
{
  gen_l_invalid(opq, param_t, delay_slot);
}

void gen_lf_sflt_s(struct op_queue *opq, int param_t[3], int delay_slot)
{
  gen_l_invalid(opq, param_t, delay_slot);
}

void gen_lf_sfne_s(struct op_queue *opq, int param_t[3], int delay_slot)
{
  gen_l_invalid(opq, param_t, delay_slot);
}

void gen_lf_sub_s(struct op_queue *opq, int param_t[3], int delay_slot)
{
  gen_l_invalid(opq, param_t, delay_slot);
}

