/* op.c -- Micro operations for the recompiler
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
#include <stdint.h>
#include <stdlib.h>

#include "config.h"

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#include "port.h"
#include "arch.h"
#include "spr-defs.h"
#include "opcode/or32.h"
#include "sim-config.h"
#include "except.h"
#include "abstract.h"
#include "execute.h"
#include "sprs.h"
#include "sched.h"
#include "immu.h"

#include "op-support.h"

#include "i386-regs.h"

#include "dyn-rec.h"

register struct cpu_state *env asm(CPU_STATE_REG);

#include "op-i386.h"

/*
 * WARNING: Before going of and wildly editing everything in this file remember
 * the following about its contents:
 * 1) The `functions' don't EVER return.  In otherwords haveing return state-
 *    ments _anywere_ in this file is likely not to work.  This is because
 *    dyngen just strips away the ret from the end of the function and just uses
 *    the function `body'.  If a ret statement is executed _anyware_ inside the
 *    dynamicly generated code, then it is undefined were we shall jump to.
 * 2) Because of 1), try not to have overly complicated functions.  In too
 *    complicated functions, gcc may decide to generate premature `exits'.  This
 *    is what passing the -fno-reorder-blocks command line switch to gcc helps
 *    with.  This is ofcourse not desired and is rather flaky as we don't (and
 *    can't) control the kind of code that gcc generates: It may work for one
 *    and break for another.  The less branches there are the less likely it is
 *    that a premature return shall occur.
 * 3) If gcc decides that it is going to be a basterd then it will optimise a
 *    very simple condition (if/switch) with a premature exit.  But gcc can't
 *    fuck ME over!  Just stick a FORCE_RET; at the END of the offending
 *    function.
 * 4) All operations must start with `op_'.  dyngen ignores all other functions.
 * 5) Local variables are depriciated: They hinder performance.
 * 6) Function calls are expensive as the stack has to be shifted (twice).
 */

/*#define __or_dynop __attribute__((noreturn))*/
#define __or_dynop

/* Temporaries to hold the (simulated) registers in */
register uint32_t t0 asm(T0_REG);
register uint32_t t1 asm(T1_REG);
register uint32_t t2 asm(T2_REG);

#define OP_PARAM1 ((uorreg_t)(&__op_param1))
#define OP_PARAM2 ((uorreg_t)(&__op_param2))
#define OP_PARAM3 ((uorreg_t)(&__op_param3))

extern uorreg_t __op_param1;
extern uorreg_t __op_param2;
extern uorreg_t __op_param3;


static inline void save_t_bound(oraddr_t pc)
{
  int reg;

  pc = (pc & immu_state->page_offset_mask) / 4;
  reg = env->curr_page->ts_bound[pc];

  if(reg & 0x1f)
    env->reg[reg & 0x1f] = t0;

  if((reg >> 5) & 0x1f)
    env->reg[(reg >> 5) & 0x1f] = t1;

  if((reg >> 10) & 0x1f)
    env->reg[(reg >> 10) & 0x1f] = t2;
}

void do_sched_wrap(void)
{
  env->pc += 4;
  //runtime.cpu.instructions++;
  runtime.sim.cycles -= env->cycles_dec;
  scheduler.job_queue->time += env->cycles_dec;
  if(scheduler.job_queue->time <= 0) {
    save_t_bound(env->pc - 4);
    do_scheduler();
  }
}

/* do_scheduler wrapper for instructions that are in the delay slot */
void do_sched_wrap_delay(void)
{
  /* FIXME: Can't this be eliminated? */
  env->pc += 4;
  //runtime.cpu.instructions++;
  runtime.sim.cycles -= env->cycles_dec;
  scheduler.job_queue->time += env->cycles_dec;
  if(scheduler.job_queue->time <= 0)
    do_scheduler();
}

void enter_dyn_code(oraddr_t addr, struct dyn_page *dp)
{
  uint16_t reg = 0;
  uint32_t t0_reg = t0, t1_reg = t1, t2_reg = t2;
  struct cpu_state *cpu_reg = env;

  addr &= immu_state->page_offset_mask;
  addr >>= 2;

  if(addr)
    reg = dp->ts_bound[addr - 1];

  t0 = cpu_state.reg[reg & 0x1f];
  t1 = cpu_state.reg[(reg >> 5) & 0x1f];

  /* Don't we all love gcc?  For some heavenly reason gcc 3.2 _knows_ that if I
   * don't put a condition around the assignment of t2, _all_ the assignments to
   * t{0,1,2} are useless and not needed.  I'm pleasently happy that gcc is so
   * bright, but on the other hand, t{0,1,2} are globals (!) how can you assume
   * that the value of a global won't be used in a function further up or
   * further down the stack?? */
  if(addr)
    t2 = cpu_state.reg[(reg >> 10) & 0x1f];

  env = &cpu_state;

  ((gen_code_ent *)dp->locs)[addr]();
  t0 = t0_reg;
  t1 = t1_reg;
  t2 = t2_reg;
  env = (struct cpu_state *)cpu_reg;
}

__or_dynop void op_set_pc_pc_delay(void)
{
  env->sprs[SPR_PPC] = env->pc;
  /* pc_delay is pulled back 4 since imediatly after this is run, the scheduler
   * runs which also increments it by 4 */
  env->pc = env->pc_delay - 4;
}

__or_dynop void op_set_pc_delay_imm(void)
{
  env->pc_delay = env->pc + (orreg_t)OP_PARAM1;
  env->delay_insn = 1;
}

__or_dynop void op_set_pc_delay_pc(void)
{
  env->pc_delay = env->pc;
  env->delay_insn = 1;
}

__or_dynop void op_clear_pc_delay(void)
{
  env->pc_delay = 0;
  env->delay_insn = 1;
}

__or_dynop void op_do_jump_delay(void)
{
  env->pc = env->pc_delay;
}

__or_dynop void op_clear_delay_insn(void)
{
  env->delay_insn = 0;
}

__or_dynop void op_set_delay_insn(void)
{
  env->delay_insn = 1;
}

__or_dynop void op_check_delay_slot(void)
{
  if(!env->delay_insn)
    OP_JUMP(OP_PARAM1);
}

__or_dynop void op_jmp_imm(void)
{
  OP_JUMP(OP_PARAM1);
}

__or_dynop void op_set_flag(void)
{
  env->sprs[SPR_SR] |= SPR_SR_F;
}

__or_dynop void op_clear_flag(void)
{
  env->sprs[SPR_SR] &= ~SPR_SR_F;
}

/* Used for the l.bf instruction.  Therefore if the flag is not set, jump over
 * all the jumping stuff */
__or_dynop void op_check_flag(void)
{
  if(!(env->sprs[SPR_SR] & SPR_SR_F)) {
    SPEEDY_CALL(do_sched_wrap);
    OP_JUMP(OP_PARAM1);
  }
}

/* Used for l.bf if the delay slot instruction is on another page */
__or_dynop void op_check_flag_delay(void)
{
  if(env->sprs[SPR_SR] & SPR_SR_F) {
    env->delay_insn = 1;
    env->pc_delay = env->pc + (orreg_t)OP_PARAM1;
  }
}

/* Used for the l.bnf instruction.  Therefore if the flag is set, jump over all
 * the jumping stuff */
__or_dynop void op_check_not_flag(void)
{
  if(env->sprs[SPR_SR] & SPR_SR_F) {
    SPEEDY_CALL(do_sched_wrap);
    OP_JUMP(OP_PARAM1);
  }
}

/* Used for l.bnf if the delay slot instruction is on another page */
__or_dynop void op_check_not_flag_delay(void)
{
  if(!(env->sprs[SPR_SR] & SPR_SR_F)) {
    env->delay_insn = 1;
    env->pc_delay = env->pc + (orreg_t)OP_PARAM1;
  }
}

__or_dynop void op_add_pc(void)
{
  env->pc += OP_PARAM1;
}

__or_dynop void op_nop_exit(void)
{
  op_support_nop_exit();
  FORCE_RET;
}

__or_dynop void op_nop_reset(void)
{
  op_support_nop_reset();
  FORCE_RET;
}

__or_dynop void op_nop_printf(void)
{
  op_support_nop_printf();
  FORCE_RET;
}

__or_dynop void op_nop_report(void)
{
  op_support_nop_report();
  FORCE_RET;
}

__or_dynop void op_nop_report_imm(void)
{
  op_support_nop_report_imm(OP_PARAM1);
}

/* FIXME: Create another 2 sched functions that to the actual analysis call
 * instead of bloating the recompiled code with this */
__or_dynop void op_analysis(void)
{
  SPEEDY_CALL(op_support_analysis);
}

__or_dynop void op_move_gpr1_pc_delay(void)
{
  env->pc_delay = env->reg[1];
  env->delay_insn = 1;
}

__or_dynop void op_move_gpr2_pc_delay(void)
{
  env->pc_delay = env->reg[2];
  env->delay_insn = 1;
}

__or_dynop void op_move_gpr3_pc_delay(void)
{
  env->pc_delay = env->reg[3];
  env->delay_insn = 1;
}

__or_dynop void op_move_gpr4_pc_delay(void)
{
  env->pc_delay = env->reg[4];
  env->delay_insn = 1;
}

__or_dynop void op_move_gpr5_pc_delay(void)
{
  env->pc_delay = env->reg[5];
  env->delay_insn = 1;
}

__or_dynop void op_move_gpr6_pc_delay(void)
{
  env->pc_delay = env->reg[6];
  env->delay_insn = 1;
}

__or_dynop void op_move_gpr7_pc_delay(void)
{
  env->pc_delay = env->reg[7];
  env->delay_insn = 1;
}

__or_dynop void op_move_gpr8_pc_delay(void)
{
  env->pc_delay = env->reg[8];
  env->delay_insn = 1;
}

__or_dynop void op_move_gpr9_pc_delay(void)
{
  env->pc_delay = env->reg[9];
  env->delay_insn = 1;
}

__or_dynop void op_move_gpr10_pc_delay(void)
{
  env->pc_delay = env->reg[10];
  env->delay_insn = 1;
}

__or_dynop void op_move_gpr11_pc_delay(void)
{
  env->pc_delay = env->reg[11];
  env->delay_insn = 1;
}

__or_dynop void op_move_gpr12_pc_delay(void)
{
  env->pc_delay = env->reg[12];
  env->delay_insn = 1;
}

__or_dynop void op_move_gpr13_pc_delay(void)
{
  env->pc_delay = env->reg[13];
  env->delay_insn = 1;
}

__or_dynop void op_move_gpr14_pc_delay(void)
{
  env->pc_delay = env->reg[14];
  env->delay_insn = 1;
}

__or_dynop void op_move_gpr15_pc_delay(void)
{
  env->pc_delay = env->reg[15];
  env->delay_insn = 1;
}

__or_dynop void op_move_gpr16_pc_delay(void)
{
  env->pc_delay = env->reg[16];
  env->delay_insn = 1;
}

__or_dynop void op_move_gpr17_pc_delay(void)
{
  env->pc_delay = env->reg[17];
  env->delay_insn = 1;
}

__or_dynop void op_move_gpr18_pc_delay(void)
{
  env->pc_delay = env->reg[18];
  env->delay_insn = 1;
}

__or_dynop void op_move_gpr19_pc_delay(void)
{
  env->pc_delay = env->reg[19];
  env->delay_insn = 1;
}

__or_dynop void op_move_gpr20_pc_delay(void)
{
  env->pc_delay = env->reg[20];
  env->delay_insn = 1;
}

__or_dynop void op_move_gpr21_pc_delay(void)
{
  env->pc_delay = env->reg[21];
  env->delay_insn = 1;
}

__or_dynop void op_move_gpr22_pc_delay(void)
{
  env->pc_delay = env->reg[22];
  env->delay_insn = 1;
}

__or_dynop void op_move_gpr23_pc_delay(void)
{
  env->pc_delay = env->reg[23];
  env->delay_insn = 1;
}

__or_dynop void op_move_gpr24_pc_delay(void)
{
  env->pc_delay = env->reg[24];
  env->delay_insn = 1;
}

__or_dynop void op_move_gpr25_pc_delay(void)
{
  env->pc_delay = env->reg[25];
  env->delay_insn = 1;
}

__or_dynop void op_move_gpr26_pc_delay(void)
{
  env->pc_delay = env->reg[26];
  env->delay_insn = 1;
}

__or_dynop void op_move_gpr27_pc_delay(void)
{
  env->pc_delay = env->reg[27];
  env->delay_insn = 1;
}

__or_dynop void op_move_gpr28_pc_delay(void)
{
  env->pc_delay = env->reg[28];
  env->delay_insn = 1;
}

__or_dynop void op_move_gpr29_pc_delay(void)
{
  env->pc_delay = env->reg[29];
  env->delay_insn = 1;
}

__or_dynop void op_move_gpr30_pc_delay(void)
{
  env->pc_delay = env->reg[30];
  env->delay_insn = 1;
}

__or_dynop void op_move_gpr31_pc_delay(void)
{
  env->pc_delay = env->reg[31];
  env->delay_insn = 1;
}

#define OP_FILE "op-1t-op.h"
#include "op-1t.h"
#undef OP_FILE

#define OP_FILE "op-2t-op.h"
#include "op-2t.h"
#undef OP_FILE

#define OP_FILE "op-3t-op.h"
#include "op-3t.h"
#undef OP_FILE

#define OP_FILE "op-arith-op.h"
#define OP_EXTRA

#define OP /
#define OP_CAST(x) (orreg_t)(x)
#define OP_NAME div
#include "op-3t.h"
#undef OP_NAME
#undef OP_CAST
#undef OP

#define OP /
#define OP_CAST(x) (x)
#define OP_NAME divu
#include "op-3t.h"
#undef OP_NAME
#undef OP_CAST
#undef OP

#define OP *
#define OP_CAST(x) (x)
#define OP_NAME mulu
#include "op-3t.h"
#undef OP_NAME
#undef OP_CAST
#undef OP

#define OP -
#define OP_CAST(x) (orreg_t)(x)
#define OP_NAME sub
#include "op-3t.h"
#undef OP_NAME
#undef OP_CAST
#undef OP

#undef OP_EXTRA

#define OP_EXTRA + ((env->sprs[SPR_SR] & SPR_SR_CY) >> 10)
#define OP +
#define OP_CAST(x) (orreg_t)(x)
#define OP_NAME addc
#include "op-3t.h"
#include "op-2t.h"
#undef OP_NAME
#undef OP_CAST
#undef OP

#undef OP_EXTRA
#define OP_EXTRA

#define OP +
#define OP_CAST(x) (orreg_t)(x)
#define OP_NAME add
#include "op-3t.h"
#include "op-2t.h"
#undef OP_NAME
#undef OP_CAST
#undef OP

#define OP &
#define OP_CAST(x) (x)
#define OP_NAME and
#include "op-3t.h"
#include "op-2t.h"
#undef OP_NAME
#undef OP_CAST
#undef OP

#define OP *
#define OP_CAST(x) (orreg_t)(x)
#define OP_NAME mul
#include "op-3t.h"
#include "op-2t.h"
#undef OP_NAME
#undef OP_CAST
#undef OP

#define OP |
#define OP_CAST(x) (x)
#define OP_NAME or
#include "op-3t.h"
#include "op-2t.h"
#undef OP_NAME
#undef OP_CAST
#undef OP

#define OP <<
#define OP_CAST(x) (x)
#define OP_NAME sll
#include "op-3t.h"
#include "op-2t.h"
#undef OP_NAME
#undef OP_CAST
#undef OP

#define OP >>
#define OP_CAST(x) (orreg_t)(x)
#define OP_NAME sra
#include "op-3t.h"
#include "op-2t.h"
#undef OP_NAME
#undef OP_CAST
#undef OP

#define OP >>
#define OP_CAST(x) (x)
#define OP_NAME srl
#include "op-3t.h"
#include "op-2t.h"
#undef OP_NAME
#undef OP_CAST
#undef OP

#define OP ^
#define OP_CAST(x) (x)
#define OP_NAME xor
#include "op-3t.h"
#include "op-2t.h"
#undef OP_NAME
#undef OP_CAST
#undef OP

#undef OP_EXTRA
#undef OP_FILE

#define OP_FILE "op-extend-op.h"

#define EXT_NAME extbs
#define EXT_TYPE int8_t
#define EXT_CAST (orreg_t)
#include "op-2t.h"
#undef EXT_CAST
#undef EXT_TYPE
#undef EXT_NAME

#define EXT_NAME extbz
#define EXT_TYPE uint8_t
#define EXT_CAST (uorreg_t)
#include "op-2t.h"
#undef EXT_CAST
#undef EXT_TYPE
#undef EXT_NAME

#define EXT_NAME exths
#define EXT_TYPE int16_t
#define EXT_CAST (orreg_t)
#include "op-2t.h"
#undef EXT_CAST
#undef EXT_TYPE
#undef EXT_NAME

#define EXT_NAME exthz
#define EXT_TYPE uint16_t
#define EXT_CAST (uorreg_t)
#include "op-2t.h"
#undef EXT_CAST
#undef EXT_TYPE
#undef EXT_NAME

#undef OP_FILE

#define OP_FILE "op-comp-op.h"

#define COMP ==
#define COMP_NAME sfeq
#define COMP_CAST(x) (x)
#include "op-2t.h"
#include "op-1t.h"
#undef COMP_CAST
#undef COMP_NAME
#undef COMP

#define COMP !=
#define COMP_NAME sfne
#define COMP_CAST(x) (x)
#include "op-2t.h"
#include "op-1t.h"
#undef COMP_CAST
#undef COMP_NAME
#undef COMP

#define COMP >
#define COMP_NAME sfgtu
#define COMP_CAST(x) (x)
#include "op-2t.h"
#include "op-1t.h"
#undef COMP_CAST
#undef COMP_NAME
#undef COMP

#define COMP >=
#define COMP_NAME sfgeu
#define COMP_CAST(x) (x)
#include "op-2t.h"
#include "op-1t.h"
#undef COMP_CAST
#undef COMP_NAME
#undef COMP

#define COMP <
#define COMP_NAME sfltu
#define COMP_CAST(x) (x)
#include "op-2t.h"
#include "op-1t.h"
#undef COMP_CAST
#undef COMP_NAME
#undef COMP

#define COMP <=
#define COMP_NAME sfleu
#define COMP_CAST(x) (x)
#include "op-2t.h"
#include "op-1t.h"
#undef COMP_CAST
#undef COMP_NAME
#undef COMP

#define COMP >
#define COMP_NAME sfgts
#define COMP_CAST(x) (orreg_t)(x)
#include "op-2t.h"
#include "op-1t.h"
#undef COMP_CAST
#undef COMP_NAME
#undef COMP

#define COMP >=
#define COMP_NAME sfges
#define COMP_CAST(x) (orreg_t)(x)
#include "op-2t.h"
#include "op-1t.h"
#undef COMP_CAST
#undef COMP_NAME
#undef COMP

#define COMP <
#define COMP_NAME sflts
#define COMP_CAST(x) (orreg_t)(x)
#include "op-2t.h"
#include "op-1t.h"
#undef COMP_CAST
#undef COMP_NAME
#undef COMP

#define COMP <=
#define COMP_NAME sfles
#define COMP_CAST(x) (orreg_t)(x)
#include "op-2t.h"
#include "op-1t.h"
#undef COMP_CAST
#undef COMP_NAME
#undef COMP

#undef OP_FILE

#define OP_FILE "op-t-reg-mov-op.h"
#include "op-1t.h"
#undef OP_FILE

#define OP_FILE "op-mftspr-op.h"
#include "op-1t.h"
#include "op-2t.h"
#undef OP_FILE
#include "op-mftspr-op.h"

#define OP_FILE "op-mac-op.h"

#define OP +=
#define OP_NAME mac
#include "op-2t.h"
#undef OP_NAME
#undef OP

#define OP -=
#define OP_NAME msb
#include "op-2t.h"
#undef OP_NAME
#undef OP

#undef OP_FILE

#define OP_FILE "op-lwhb-op.h"

#define LS_OP_NAME lbz
#define LS_OP_CAST
#define LS_OP_FUNC eval_mem8
#include "op-2t.h"
#include "op-1t.h"
#undef LS_OP_FUNC
#undef LS_OP_CAST
#undef LS_OP_NAME

#define LS_OP_NAME lbs
#define LS_OP_CAST (int8_t)
#define LS_OP_FUNC eval_mem8
#include "op-2t.h"
#include "op-1t.h"
#undef LS_OP_FUNC
#undef LS_OP_CAST
#undef LS_OP_NAME

#define LS_OP_NAME lhz
#define LS_OP_CAST
#define LS_OP_FUNC eval_mem16
#include "op-2t.h"
#include "op-1t.h"
#undef LS_OP_FUNC
#undef LS_OP_CAST
#undef LS_OP_NAME

#define LS_OP_NAME lhs
#define LS_OP_CAST (int16_t)
#define LS_OP_FUNC eval_mem16
#include "op-2t.h"
#include "op-1t.h"
#undef LS_OP_FUNC
#undef LS_OP_CAST
#undef LS_OP_NAME

#define LS_OP_NAME lwz
#define LS_OP_CAST
#define LS_OP_FUNC eval_mem32
#include "op-2t.h"
#include "op-1t.h"
#undef LS_OP_FUNC
#undef LS_OP_CAST
#undef LS_OP_NAME

#define LS_OP_NAME lws
#define LS_OP_CAST (int32_t)
#define LS_OP_FUNC eval_mem32
#include "op-2t.h"
#include "op-1t.h"
#undef LS_OP_FUNC
#undef LS_OP_CAST
#undef LS_OP_NAME

#undef OP_FILE

#define OP_FILE "op-swhb-op.h"

#define S_OP_NAME sb
#define S_FUNC set_mem8
#include "op-swhb-op.h"
#include "op-2t.h"
#include "op-1t.h"
#undef S_FUNC
#undef S_OP_NAME

#define S_OP_NAME sh
#define S_FUNC set_mem16
#include "op-swhb-op.h"
#include "op-2t.h"
#include "op-1t.h"
#undef S_FUNC
#undef S_OP_NAME

#define S_OP_NAME sw
#define S_FUNC set_mem32
#include "op-swhb-op.h"
#include "op-2t.h"
#include "op-1t.h"
#undef S_FUNC
#undef S_OP_NAME

__or_dynop void op_join_mem_cycles(void)
{
  runtime.sim.cycles += runtime.sim.mem_cycles;
  scheduler.job_queue->time -= runtime.sim.mem_cycles;
  runtime.sim.mem_cycles = 0;
}

__or_dynop void op_store_link_addr_gpr(void)
{
  env->reg[LINK_REGNO] = env->pc + 8;
}

__or_dynop void op_prep_rfe(void)
{
  env->sprs[SPR_SR] = env->sprs[SPR_ESR_BASE] | SPR_SR_FO;
  env->sprs[SPR_PPC] = env->pc;
  env->pc = env->sprs[SPR_EPCR_BASE] - 4;
}

static inline void prep_except(oraddr_t epcr_base)
{
  env->sprs[SPR_EPCR_BASE] = epcr_base;

  env->sprs[SPR_ESR_BASE] = env->sprs[SPR_SR];

  /* Address translation is always disabled when starting exception. */
  env->sprs[SPR_SR] &= ~SPR_SR_DME;
  env->sprs[SPR_SR] &= ~SPR_SR_IME;

  env->sprs[SPR_SR] &= ~SPR_SR_OVE;   /* Disable overflow flag exception. */

  env->sprs[SPR_SR] |= SPR_SR_SM;     /* SUPV mode */
  env->sprs[SPR_SR] &= ~(SPR_SR_IEE | SPR_SR_TEE);    /* Disable interrupts. */
}

/* Before the code in op_{sys,trap}{,_delay} gets run, the scheduler runs.
 * Therefore the pc will point to the instruction after the l.sys or l.trap
 * instruction */
__or_dynop void op_prep_sys_delay(void)
{
  env->delay_insn = 0;
  prep_except(env->pc - 4);
  env->pc = EXCEPT_SYSCALL - 4;
}

__or_dynop void op_prep_sys(void)
{
  prep_except(env->pc + 4);
  env->pc = EXCEPT_SYSCALL - 4;
}

__or_dynop void op_prep_trap_delay(void)
{
  env->delay_insn = 0;
  prep_except(env->pc - 4);
  env->pc = EXCEPT_TRAP - 4;
}

__or_dynop void op_prep_trap(void)
{
  prep_except(env->pc);
  env->pc = EXCEPT_TRAP - 4;
}

/* FIXME: This `instruction' should be split up like the l.trap and l.sys
 * instructions are done */
__or_dynop void op_illegal_delay(void)
{
  env->delay_insn = 0;
  env->sprs[SPR_EEAR_BASE] = env->pc - 4;
  env->pc = EXCEPT_ILLEGAL - 4;
}

__or_dynop void op_illegal(void)
{
  env->sprs[SPR_EEAR_BASE] = env->pc;
  env->pc = EXCEPT_ILLEGAL;
}

__or_dynop void op_do_sched(void)
{
  SPEEDY_CALL(do_sched_wrap);
}

__or_dynop void op_do_sched_delay(void)
{
  SPEEDY_CALL(do_sched_wrap_delay);
}

__or_dynop void op_macc(void)
{
  env->sprs[SPR_MACLO] = 0;
  env->sprs[SPR_MACHI] = 0;
}

__or_dynop void op_store_insn_ea(void)
{
  env->insn_ea = OP_PARAM1;
}

