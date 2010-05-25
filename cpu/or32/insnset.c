/* insnset.c -- Instruction specific functions.

   Copyright (C) 1999 Damjan Lampret, lampret@opencores.org
                 2000-2002 Marko Mlinar, markom@opencores.org
   Copyright (C) 2008 Embecosm Limited
   Copyright (C) 2009 Jungsook yang, jungsook.yang@uci.edu
  
   Contributor Jeremy Bennett <jeremy.bennett@embecosm.com>
   Contributor Julius Baxter julius@orsoc.se  

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


INSTRUCTION (l_add) {
  orreg_t temp1, temp2, temp3;
  int8_t temp4;
  
  temp2 = (orreg_t)PARAM2;
  temp3 = (orreg_t)PARAM1;
  temp1 = temp2 + temp3;
  SET_PARAM0(temp1);
  SET_OV_FLAG_FN (temp1);
  if (ARITH_SET_FLAG) {
    if(!temp1)
      cpu_state.sprs[SPR_SR] |= SPR_SR_F;
    else
      cpu_state.sprs[SPR_SR] &= ~SPR_SR_F;
  }
  if ((uorreg_t) temp1 < (uorreg_t) temp2)
    cpu_state.sprs[SPR_SR] |= SPR_SR_CY;
  else
    cpu_state.sprs[SPR_SR] &= ~SPR_SR_CY;

  temp4 = temp1;
  if (temp4 == temp1)
    or1k_mstats.byteadd++;
}
INSTRUCTION (l_addc) {
  orreg_t temp1, temp2, temp3;
  int8_t temp4;
  
  temp2 = (orreg_t)PARAM2;
  temp3 = (orreg_t)PARAM1;
  temp1 = temp2 + temp3;
  if(cpu_state.sprs[SPR_SR] & SPR_SR_CY)
    temp1++;
  SET_PARAM0(temp1);
  SET_OV_FLAG_FN (temp1);
  if (ARITH_SET_FLAG) {
    if(!temp1)
      cpu_state.sprs[SPR_SR] |= SPR_SR_F;
    else
      cpu_state.sprs[SPR_SR] &= ~SPR_SR_F;
  }
  if ((uorreg_t) temp1 < (uorreg_t) temp2)
    cpu_state.sprs[SPR_SR] |= SPR_SR_CY;
  else
    cpu_state.sprs[SPR_SR] &= ~SPR_SR_CY;

  temp4 = temp1;
  if (temp4 == temp1)
    or1k_mstats.byteadd++;
}
INSTRUCTION (l_sw) {
  int old_cyc = 0;
  if (config.cpu.sbuf_len) old_cyc = runtime.sim.mem_cycles;
  set_mem32(PARAM0, PARAM1, &breakpoint);
  if (config.cpu.sbuf_len) {
    int t = runtime.sim.mem_cycles;
    runtime.sim.mem_cycles = old_cyc;
    sbuf_store (t - old_cyc);
  }
}
INSTRUCTION (l_sb) {
  int old_cyc = 0;
  if (config.cpu.sbuf_len) old_cyc = runtime.sim.mem_cycles;
  set_mem8(PARAM0, PARAM1, &breakpoint);
  if (config.cpu.sbuf_len) {
    int t = runtime.sim.mem_cycles;
    runtime.sim.mem_cycles = old_cyc;
    sbuf_store (t- old_cyc);
  }
}
INSTRUCTION (l_sh) {
  int old_cyc = 0;
  if (config.cpu.sbuf_len) old_cyc = runtime.sim.mem_cycles;
  set_mem16(PARAM0, PARAM1, &breakpoint);
  if (config.cpu.sbuf_len) {
    int t = runtime.sim.mem_cycles;
    runtime.sim.mem_cycles = old_cyc;
    sbuf_store (t - old_cyc);
  }
}
INSTRUCTION (l_lwz) {
  uint32_t val;
  if (config.cpu.sbuf_len) sbuf_load ();
  val = eval_mem32(PARAM1, &breakpoint);
  /* If eval operand produced exception don't set anything. JPB changed to
     trigger on breakpoint, as well as except_pending (seemed to be a bug). */
  if (!(except_pending || breakpoint))
    SET_PARAM0(val);
}
INSTRUCTION (l_lbs) {
  int8_t val;
  if (config.cpu.sbuf_len) sbuf_load ();
  val = eval_mem8(PARAM1, &breakpoint);
  /* If eval operand produced exception don't set anything. JPB changed to
     trigger on breakpoint, as well as except_pending (seemed to be a bug). */
  if (!(except_pending || breakpoint))
    SET_PARAM0(val);
}
INSTRUCTION (l_lbz) {  
  uint8_t val;
  if (config.cpu.sbuf_len) sbuf_load ();
  val = eval_mem8(PARAM1, &breakpoint);
  /* If eval operand produced exception don't set anything. JPB changed to
     trigger on breakpoint, as well as except_pending (seemed to be a bug). */
  if (!(except_pending || breakpoint))
    SET_PARAM0(val);
}
INSTRUCTION (l_lhs) {  
  int16_t val;
  if (config.cpu.sbuf_len) sbuf_load ();
  val = eval_mem16(PARAM1, &breakpoint);
  /* If eval operand produced exception don't set anything. JPB changed to
     trigger on breakpoint, as well as except_pending (seemed to be a bug). */
  if (!(except_pending || breakpoint))
    SET_PARAM0(val);
}
INSTRUCTION (l_lhz) {  
  uint16_t val;
  if (config.cpu.sbuf_len) sbuf_load ();
  val = eval_mem16(PARAM1, &breakpoint);
  /* If eval operand produced exception don't set anything. JPB changed to
     trigger on breakpoint, as well as except_pending (seemed to be a bug). */
  if (!(except_pending || breakpoint))
    SET_PARAM0(val);
}
INSTRUCTION (l_movhi) {
  SET_PARAM0(PARAM1 << 16);
}
INSTRUCTION (l_and) {
  uorreg_t temp1;
  temp1 = PARAM1 & PARAM2;
  SET_OV_FLAG_FN (temp1);
  SET_PARAM0(temp1);
  if (ARITH_SET_FLAG) {
    if(!temp1)
      cpu_state.sprs[SPR_SR] |= SPR_SR_F;
    else
      cpu_state.sprs[SPR_SR] &= ~SPR_SR_F;
  }
}
INSTRUCTION (l_or) {
  uorreg_t temp1;
  temp1 = PARAM1 | PARAM2;
  SET_OV_FLAG_FN (temp1);
  SET_PARAM0(temp1);
}
INSTRUCTION (l_xor) {
  uorreg_t temp1;
  temp1 = PARAM1 ^ PARAM2;
  SET_OV_FLAG_FN (temp1);
  SET_PARAM0(temp1);
}
INSTRUCTION (l_sub) {
  orreg_t temp1;
  temp1 = (orreg_t)PARAM1 - (orreg_t)PARAM2;
  SET_OV_FLAG_FN (temp1);
  SET_PARAM0(temp1);
}
/*int mcount = 0;*/
INSTRUCTION (l_mul) {
  orreg_t temp1;
  
  temp1 = (orreg_t)PARAM1 * (orreg_t)PARAM2;
  SET_OV_FLAG_FN (temp1);
  SET_PARAM0(temp1);
  /*if (!(mcount++ & 1023)) {
    PRINTF ("[%i]\n",mcount);
    }*/
}
INSTRUCTION (l_div) {
  orreg_t temp3, temp2, temp1;
  
  temp3 = PARAM2;
  temp2 = PARAM1;
  if (temp3)
    temp1 = temp2 / temp3;
  else {
    except_handle(EXCEPT_ILLEGAL, cpu_state.pc);
    return;
  }
  SET_OV_FLAG_FN (temp1);
  SET_PARAM0(temp1);
}
INSTRUCTION (l_divu) {
  uorreg_t temp3, temp2, temp1;
  
  temp3 = PARAM2;
  temp2 = PARAM1;
  if (temp3)
    temp1 = temp2 / temp3;
  else {
    except_handle(EXCEPT_ILLEGAL, cpu_state.pc);
    return;
  }
  SET_OV_FLAG_FN (temp1);
  SET_PARAM0(temp1);
  /* runtime.sim.cycles += 16; */
}
INSTRUCTION (l_sll) {
  uorreg_t temp1;

  temp1 = PARAM1 << PARAM2;
  SET_OV_FLAG_FN (temp1);
  SET_PARAM0(temp1);
  /* runtime.sim.cycles += 2; */
}
INSTRUCTION (l_sra) {
  orreg_t temp1;
  
  temp1 = (orreg_t)PARAM1 >> PARAM2;
  SET_OV_FLAG_FN (temp1);
  SET_PARAM0(temp1);
  /* runtime.sim.cycles += 2; */
}
INSTRUCTION (l_srl) {
  uorreg_t temp1;
  temp1 = PARAM1 >> PARAM2;
  SET_OV_FLAG_FN (temp1);
  SET_PARAM0(temp1);
  /* runtime.sim.cycles += 2; */
}
INSTRUCTION (l_bf) {
  if (config.bpb.enabled) {
    int fwd = (PARAM0 >= cpu_state.pc) ? 1 : 0;
    or1k_mstats.bf[cpu_state.sprs[SPR_SR] & SPR_SR_F ? 1 : 0][fwd]++;
    bpb_update(current->insn_addr, cpu_state.sprs[SPR_SR] & SPR_SR_F ? 1 : 0);
  }
  if(cpu_state.sprs[SPR_SR] & SPR_SR_F) {
    cpu_state.pc_delay = cpu_state.pc + (orreg_t)PARAM0 * 4;
    btic_update(pcnext);
    next_delay_insn = 1;
  } else {
    btic_update(cpu_state.pc);
  }
}
INSTRUCTION (l_bnf) {
  if (config.bpb.enabled) {
    int fwd = (PARAM0 >= cpu_state.pc) ? 1 : 0;
    or1k_mstats.bnf[cpu_state.sprs[SPR_SR] & SPR_SR_F ? 0 : 1][fwd]++;
    bpb_update(current->insn_addr, cpu_state.sprs[SPR_SR] & SPR_SR_F ? 0 : 1);
  }
  if (!(cpu_state.sprs[SPR_SR] & SPR_SR_F)) {
    cpu_state.pc_delay = cpu_state.pc + (orreg_t)PARAM0 * 4;
    btic_update(pcnext);
    next_delay_insn = 1;
  } else {
    btic_update(cpu_state.pc);
  }
}
INSTRUCTION (l_j) {
  cpu_state.pc_delay = cpu_state.pc + (orreg_t)PARAM0 * 4;
  next_delay_insn = 1;
}
INSTRUCTION (l_jal) {
  cpu_state.pc_delay = cpu_state.pc + (orreg_t)PARAM0 * 4;
  
  setsim_reg(LINK_REGNO, cpu_state.pc + 8);
  next_delay_insn = 1;
  if (config.sim.profile) {
    struct label_entry *tmp;
    if (verify_memoryarea(cpu_state.pc_delay) && (tmp = get_label (cpu_state.pc_delay)))
      fprintf (runtime.sim.fprof, "+%08llX %"PRIxADDR" %"PRIxADDR" %s\n",
               runtime.sim.cycles, cpu_state.pc + 8, cpu_state.pc_delay,
               tmp->name);
    else
      fprintf (runtime.sim.fprof, "+%08llX %"PRIxADDR" %"PRIxADDR" @%"PRIxADDR"\n",
               runtime.sim.cycles, cpu_state.pc + 8, cpu_state.pc_delay,
               cpu_state.pc_delay);
  }
}
INSTRUCTION (l_jalr) {
  cpu_state.pc_delay = PARAM0;
  setsim_reg(LINK_REGNO, cpu_state.pc + 8);
  next_delay_insn = 1;
}
INSTRUCTION (l_jr) {
  cpu_state.pc_delay = PARAM0;
  next_delay_insn = 1;
  if (config.sim.profile)
    fprintf (runtime.sim.fprof, "-%08llX %"PRIxADDR"\n", runtime.sim.cycles,
             cpu_state.pc_delay);
}
INSTRUCTION (l_rfe) {
  pcnext = cpu_state.sprs[SPR_EPCR_BASE];
  mtspr(SPR_SR, cpu_state.sprs[SPR_ESR_BASE]);
}
INSTRUCTION (l_nop) {
  uint32_t k = PARAM0;
  switch (k) {
    case NOP_NOP:
      break;
    case NOP_EXIT:
      PRINTF("exit(%"PRIdREG")\n", evalsim_reg (3));
      fprintf(stderr, "@reset : cycles %lld, insn #%lld\n",
              runtime.sim.reset_cycles, runtime.cpu.reset_instructions);
      fprintf(stderr, "@exit  : cycles %lld, insn #%lld\n", runtime.sim.cycles,
              runtime.cpu.instructions);
      fprintf(stderr, " diff  : cycles %lld, insn #%lld\n",
              runtime.sim.cycles - runtime.sim.reset_cycles,
              runtime.cpu.instructions - runtime.cpu.reset_instructions);
      if (config.debug.gdb_enabled)
        set_stall_state (1);
      else
        sim_done();
      break;
    case NOP_CNT_RESET:
      PRINTF("****************** counters reset ******************\n");
      PRINTF("cycles %lld, insn #%lld\n", runtime.sim.cycles, runtime.cpu.instructions); 
      PRINTF("****************** counters reset ******************\n");
      runtime.sim.reset_cycles = runtime.sim.cycles;
      runtime.cpu.reset_instructions = runtime.cpu.instructions;
      break;    
    case NOP_PUTC:		/*JPB */
      printf( "%c", (char)(evalsim_reg( 3 ) & 0xff));
      fflush( stdout );
      break;
    case NOP_GET_TICKS:
      cpu_state.reg[11] = runtime.sim.cycles & 0xffffffff;
      cpu_state.reg[12] = runtime.sim.cycles >> 32;
      break;
    case NOP_GET_PS:
      cpu_state.reg[11] = config.sim.clkcycle_ps;
      break;
    case NOP_REPORT:
      PRINTF("report(0x%"PRIxREG");\n", evalsim_reg(3));
    default:
      if (k >= NOP_REPORT_FIRST && k <= NOP_REPORT_LAST)
      PRINTF("report %" PRIdREG " (0x%"PRIxREG");\n", k - NOP_REPORT_FIRST,
             evalsim_reg(3));
      break;
  }
}
INSTRUCTION (l_sfeq) {
  if(PARAM0 == PARAM1)
    cpu_state.sprs[SPR_SR] |= SPR_SR_F;
  else
    cpu_state.sprs[SPR_SR] &= ~SPR_SR_F;
}
INSTRUCTION (l_sfne) {
  if(PARAM0 != PARAM1)
    cpu_state.sprs[SPR_SR] |= SPR_SR_F;
  else
    cpu_state.sprs[SPR_SR] &= ~SPR_SR_F;
}
INSTRUCTION (l_sfgts) {
  if((orreg_t)PARAM0 > (orreg_t)PARAM1)
    cpu_state.sprs[SPR_SR] |= SPR_SR_F;
  else
    cpu_state.sprs[SPR_SR] &= ~SPR_SR_F;
}
INSTRUCTION (l_sfges) {
  if((orreg_t)PARAM0 >= (orreg_t)PARAM1)
    cpu_state.sprs[SPR_SR] |= SPR_SR_F;
  else
    cpu_state.sprs[SPR_SR] &= ~SPR_SR_F;
}
INSTRUCTION (l_sflts) {
  if((orreg_t)PARAM0 < (orreg_t)PARAM1)
    cpu_state.sprs[SPR_SR] |= SPR_SR_F;
  else
    cpu_state.sprs[SPR_SR] &= ~SPR_SR_F;
}
INSTRUCTION (l_sfles) {
  if((orreg_t)PARAM0 <= (orreg_t)PARAM1)
    cpu_state.sprs[SPR_SR] |= SPR_SR_F;
  else
    cpu_state.sprs[SPR_SR] &= ~SPR_SR_F;
}
INSTRUCTION (l_sfgtu) {
  if(PARAM0 > PARAM1)
    cpu_state.sprs[SPR_SR] |= SPR_SR_F;
  else
    cpu_state.sprs[SPR_SR] &= ~SPR_SR_F;
}
INSTRUCTION (l_sfgeu) {
  if(PARAM0 >= PARAM1)
    cpu_state.sprs[SPR_SR] |= SPR_SR_F;
  else
    cpu_state.sprs[SPR_SR] &= ~SPR_SR_F;
}
INSTRUCTION (l_sfltu) {
  if(PARAM0 < PARAM1)
    cpu_state.sprs[SPR_SR] |= SPR_SR_F;
  else
    cpu_state.sprs[SPR_SR] &= ~SPR_SR_F;
}
INSTRUCTION (l_sfleu) {
  if(PARAM0 <= PARAM1)
    cpu_state.sprs[SPR_SR] |= SPR_SR_F;
  else
    cpu_state.sprs[SPR_SR] &= ~SPR_SR_F;
}
INSTRUCTION (l_extbs) {
  int8_t x;
  x = PARAM1;
  SET_PARAM0((orreg_t)x);
}
INSTRUCTION (l_extbz) {
  uint8_t x;
  x = PARAM1;
  SET_PARAM0((uorreg_t)x);
}
INSTRUCTION (l_exths) {
  int16_t x;
  x = PARAM1;
  SET_PARAM0((orreg_t)x);
}
INSTRUCTION (l_exthz) {
  uint16_t x;
  x = PARAM1;
  SET_PARAM0((uorreg_t)x);
}
INSTRUCTION (l_extws) {
  int32_t x;
  x = PARAM1;
  SET_PARAM0((orreg_t)x);
}
INSTRUCTION (l_extwz) {
  uint32_t x;
  x = PARAM1;
  SET_PARAM0((uorreg_t)x);
}
INSTRUCTION (l_mtspr) {
  uint16_t regno = PARAM0 + PARAM2;
  uorreg_t value = PARAM1;

  if (cpu_state.sprs[SPR_SR] & SPR_SR_SM)
    mtspr(regno, value);
  else {
    PRINTF("WARNING: trying to write SPR while SR[SUPV] is cleared.\n");
    sim_done();
  }
}
INSTRUCTION (l_mfspr) {
  uint16_t regno = PARAM1 + PARAM2;
  uorreg_t value = mfspr(regno);

  if (cpu_state.sprs[SPR_SR] & SPR_SR_SM)
    SET_PARAM0(value);
  else {
    SET_PARAM0(0);
    PRINTF("WARNING: trying to read SPR while SR[SUPV] is cleared.\n");
    sim_done();
  }
}
INSTRUCTION (l_sys) {
  except_handle(EXCEPT_SYSCALL, cpu_state.sprs[SPR_EEAR_BASE]);
}
INSTRUCTION (l_trap) {
  /* TODO: some SR related code here! */
  except_handle(EXCEPT_TRAP, cpu_state.sprs[SPR_EEAR_BASE]);
}
INSTRUCTION (l_mac) {
  uorreg_t lo, hi;
  LONGEST l;
  orreg_t x, y;

  lo = cpu_state.sprs[SPR_MACLO];
  hi = cpu_state.sprs[SPR_MACHI];
  x = PARAM0;
  y = PARAM1;
/*   PRINTF ("[%"PRIxREG",%"PRIxREG"]\t", x, y); */
  l = (ULONGEST)lo | ((LONGEST)hi << 32);
  l += (LONGEST) x * (LONGEST) y;

  /* This implementation is very fast - it needs only one cycle for mac.  */
  lo = ((ULONGEST)l) & 0xFFFFFFFF;
  hi = ((LONGEST)l) >> 32;
  cpu_state.sprs[SPR_MACLO] = lo;
  cpu_state.sprs[SPR_MACHI] = hi;
/*   PRINTF ("(%"PRIxREG",%"PRIxREG"\n", hi, lo); */
}
INSTRUCTION (l_msb) {
  uorreg_t lo, hi;  
  LONGEST l;
  orreg_t x, y;

  lo = cpu_state.sprs[SPR_MACLO];
  hi = cpu_state.sprs[SPR_MACHI];
  x = PARAM0;
  y = PARAM1;

/*   PRINTF ("[%"PRIxREG",%"PRIxREG"]\t", x, y); */

  l = (ULONGEST)lo | ((LONGEST)hi << 32);
  l -= x * y;

  /* This implementation is very fast - it needs only one cycle for msb.  */
  lo = ((ULONGEST)l) & 0xFFFFFFFF;
  hi = ((LONGEST)l) >> 32;
  cpu_state.sprs[SPR_MACLO] = lo;
  cpu_state.sprs[SPR_MACHI] = hi;
/*   PRINTF ("(%"PRIxREG",%"PRIxREG")\n", hi, lo); */
}
INSTRUCTION (l_macrc) {
  uorreg_t lo, hi;
  LONGEST l;
  /* No need for synchronization here -- all MAC instructions are 1 cycle long.  */
  lo =  cpu_state.sprs[SPR_MACLO];
  hi =  cpu_state.sprs[SPR_MACHI];
  l = (ULONGEST) lo | ((LONGEST)hi << 32);
  l >>= 28;
  //PRINTF ("<%08x>\n", (unsigned long)l);
  SET_PARAM0((orreg_t)l);
  cpu_state.sprs[SPR_MACLO] = 0;
  cpu_state.sprs[SPR_MACHI] = 0;
}
INSTRUCTION (l_cmov) {
  SET_PARAM0(cpu_state.sprs[SPR_SR] & SPR_SR_F ? PARAM1 : PARAM2);
}
INSTRUCTION (l_ff1) {
  SET_PARAM0(ffs(PARAM1));
}
/******* Floating point instructions *******/
/* Single precision */
INSTRUCTION (lf_add_s) {
  if (config.cpu.hardfloat) {
  FLOAT param0, param1, param2;
  param1.hval = (uorreg_t)PARAM1;
  param2.hval = (uorreg_t)PARAM2;
  param0.fval = param1.fval + param2.fval;
  SET_PARAM0(param0.hval);
  } else l_invalid();
}
INSTRUCTION (lf_div_s) {
  if (config.cpu.hardfloat) {
  FLOAT param0, param1, param2;
  param1.hval = (uorreg_t)PARAM1;
  param2.hval = (uorreg_t)PARAM2;
  param0.fval = param1.fval / param2.fval;
  SET_PARAM0(param0.hval);
  } else l_invalid();
}
INSTRUCTION (lf_ftoi_s) {
  if (config.cpu.hardfloat) {
    // no other way appeared to work --jb
    float tmp_f; memcpy((void*)&tmp_f, (void*)&PARAM1, sizeof(float));
    SET_PARAM0((int)tmp_f);
  } else l_invalid();
}
INSTRUCTION (lf_itof_s) {
  if (config.cpu.hardfloat) {
  FLOAT param0;
  param0.fval = (float)((int)PARAM1);
  SET_PARAM0(param0.hval);
  } else l_invalid();
}
INSTRUCTION (lf_madd_s) {
  if (config.cpu.hardfloat) {
  FLOAT param0,param1, param2;
  param0.hval = PARAM0;
  param1.hval = PARAM1;
  param2.hval = PARAM2;
  param0.fval += param1.fval * param2.fval; 
  SET_PARAM0(param0.hval);
  } else l_invalid();
}
INSTRUCTION (lf_mul_s) {
  if (config.cpu.hardfloat) {
  FLOAT param0, param1, param2;
  param1.hval = (uorreg_t)PARAM1;
  param2.hval = (uorreg_t)PARAM2;
  param0.fval = param1.fval * param2.fval;
  SET_PARAM0(param0.hval); 
  } else l_invalid();
}
INSTRUCTION (lf_rem_s) {
  if (config.cpu.hardfloat) {
  FLOAT param0, param1, param2;
  param1.hval = PARAM1;
  param2.hval = PARAM2;
  param0.fval = param1.fval / param2.fval;
  SET_PARAM0(param0.hval);
  } else l_invalid();
}
INSTRUCTION (lf_sfeq_s) {
  if (config.cpu.hardfloat) {
  FLOAT param0, param1;
  param0.hval = PARAM0;
  param1.hval = PARAM1;
  if(param0.fval == param1.fval)
    cpu_state.sprs[SPR_SR] |= SPR_SR_F;
  else
    cpu_state.sprs[SPR_SR] &= ~SPR_SR_F;
  } else l_invalid();
}
INSTRUCTION (lf_sfge_s) {
  if (config.cpu.hardfloat) {
  FLOAT param0, param1;
  param0.hval = PARAM0;
  param1.hval = PARAM1;
  if(param0.fval >= param1.fval)
    cpu_state.sprs[SPR_SR] |= SPR_SR_F;
  else
    cpu_state.sprs[SPR_SR] &= ~SPR_SR_F;
  } else l_invalid();
}
INSTRUCTION (lf_sfgt_s) {
  if (config.cpu.hardfloat) {
  FLOAT param0, param1;
  param0.hval = PARAM0;
  param1.hval = PARAM1;
  if(param0.fval > param1.fval)
    cpu_state.sprs[SPR_SR] |= SPR_SR_F;
  else
    cpu_state.sprs[SPR_SR] &= ~SPR_SR_F;
  } else l_invalid();
}
INSTRUCTION (lf_sfle_s) {
  if (config.cpu.hardfloat) {
  FLOAT param0, param1;
  param0.hval = PARAM0;
  param1.hval = PARAM1;
  if(param0.fval <= param1.fval)
    cpu_state.sprs[SPR_SR] |= SPR_SR_F;
  else
    cpu_state.sprs[SPR_SR] &= ~SPR_SR_F;
  } else l_invalid();
}
INSTRUCTION (lf_sflt_s) {
  if (config.cpu.hardfloat) {
  FLOAT param0, param1;
  param0.hval = PARAM0;
  param1.hval = PARAM1;
  if(param0.fval < param1.fval)
    cpu_state.sprs[SPR_SR] |= SPR_SR_F;
  else
    cpu_state.sprs[SPR_SR] &= ~SPR_SR_F;
  } else l_invalid();
}
INSTRUCTION (lf_sfne_s) {
  if (config.cpu.hardfloat) {
  FLOAT param0, param1;
  param0.hval = PARAM0;
  param1.hval = PARAM1;
  if(param0.fval != param1.fval)
    cpu_state.sprs[SPR_SR] |= SPR_SR_F;
  else
    cpu_state.sprs[SPR_SR] &= ~SPR_SR_F;
  } else l_invalid();
}
INSTRUCTION (lf_sub_s) {
  if (config.cpu.hardfloat) {
  FLOAT param0, param1, param2;
  param1.hval = PARAM1;
  param2.hval = PARAM2;
  param0.fval = param1.fval - param2.fval;
  SET_PARAM0(param0.hval); 
  } else l_invalid();
}

/******* Custom instructions *******/
INSTRUCTION (l_cust1) {
  /*int destr = current->insn >> 21;
    int src1r = current->insn >> 15;
    int src2r = current->insn >> 9;*/
}
INSTRUCTION (l_cust2) {
}
INSTRUCTION (l_cust3) {
}
INSTRUCTION (l_cust4) {
}
INSTRUCTION (lf_cust1) {
}
INSTRUCTION (lf_cust2) {
}
INSTRUCTION (lf_cust3) {
}
INSTRUCTION (lf_cust4) {
}
