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
  SET_PARAM0 (temp1);

  /* Set overflow if two negative values gave a positive sum, or if two
     positive values gave a negative sum. Otherwise clear it */
  if ((((long int) temp2 <  0) && 
       ((long int) temp3 <  0) &&
       ((long int) temp1 >= 0)) ||
      (((long int) temp2 >= 0) && 
       ((long int) temp3 >= 0) &&
       ((long int) temp1 <  0)))
    {
      cpu_state.sprs[SPR_SR] |= SPR_SR_OV;
    }
  else
    {
      cpu_state.sprs[SPR_SR] &= ~SPR_SR_OV;
    }

  /* Set the carry flag if (as unsigned values) the result is smaller than
     either operand (if it smaller than one, it will be smaller than both, so
     we need only test one). */
  if ((uorreg_t) temp1 < (uorreg_t) temp2)
    {
      cpu_state.sprs[SPR_SR] |= SPR_SR_CY;
    }
  else
    {
      cpu_state.sprs[SPR_SR] &= ~SPR_SR_CY;
    }

  /* Trigger a range exception if the overflow flag is set and the SR[OVE] bit
     is set. */
  if (((cpu_state.sprs[SPR_SR] & SPR_SR_OVE) == SPR_SR_OVE) &&
      ((cpu_state.sprs[SPR_SR] & SPR_SR_OV)  == SPR_SR_OV))
    {
      except_handle (EXCEPT_RANGE, cpu_state.pc);
    }

  temp4 = temp1;
  if (temp4 == temp1)
    or1k_mstats.byteadd++;
}
INSTRUCTION (l_addc) {
  orreg_t temp1, temp2, temp3;
  int8_t temp4;
  int    carry_in = (cpu_state.sprs[SPR_SR] & SPR_SR_CY) == SPR_SR_CY;

  temp2 = (orreg_t)PARAM2;
  temp3 = (orreg_t)PARAM1;
  temp1 = temp2 + temp3;

  if(carry_in)
    {
      temp1++;				/* Add in the carry bit */
    }

  SET_PARAM0(temp1);

  /* Set overflow if two negative values gave a positive sum, or if two
     positive values gave a negative sum. Otherwise clear it. There are no
     corner cases with the extra bit carried in (unlike the carry flag - see
     below). */
  if ((((long int) temp2 <  0) && 
       ((long int) temp3 <  0) &&
       ((long int) temp1 >= 0)) ||
      (((long int) temp2 >= 0) && 
       ((long int) temp3 >= 0) &&
       ((long int) temp1 <  0)))
    {
      cpu_state.sprs[SPR_SR] |= SPR_SR_OV;
    }
  else
    {
      cpu_state.sprs[SPR_SR] &= ~SPR_SR_OV;
    }

  /* Set the carry flag if (as unsigned values) the result is smaller than
     either operand (if it smaller than one, it will be smaller than both, so
     we need only test one). If there is a carry in, the test should be less
     than or equal, to deal with the 0 + 0xffffffff + c = 0 case (which
     generates a carry). */
  if ((carry_in && ((uorreg_t) temp1 <= (uorreg_t) temp2)) ||
      ((uorreg_t) temp1 < (uorreg_t) temp2))
    {
      cpu_state.sprs[SPR_SR] |= SPR_SR_CY;
    }
  else
    {
      cpu_state.sprs[SPR_SR] &= ~SPR_SR_CY;
    }

  /* Trigger a range exception if the overflow flag is set and the SR[OVE] bit
     is set. */
  if (((cpu_state.sprs[SPR_SR] & SPR_SR_OVE) == SPR_SR_OVE) &&
      ((cpu_state.sprs[SPR_SR] & SPR_SR_OV)  == SPR_SR_OV))
    {
      except_handle (EXCEPT_RANGE, cpu_state.pc);
    }

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
INSTRUCTION (l_lws) {
  uint32_t val;
  if (config.cpu.sbuf_len) sbuf_load ();
  val = eval_mem32(PARAM1, &breakpoint);
  /* If eval operand produced exception don't set anything. JPB changed to
     trigger on breakpoint, as well as except_pending (seemed to be a bug). */
  if (!(except_pending || breakpoint))
    SET_PARAM0(val);
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
  SET_PARAM0(temp1);
}
INSTRUCTION (l_or) {
  uorreg_t temp1;
  temp1 = PARAM1 | PARAM2;
  SET_PARAM0(temp1);
}
INSTRUCTION (l_xor) {
  /* The argument is now specified as unsigned, but historically OR1K has
     always treated the argument as signed (so l.xori rD,rA,-1 can be used in
     the absence of l.not). Use this as the default behavior. This is
     controlled from or32.c. */
  uorreg_t  temp1 = PARAM1 ^ PARAM2;
  SET_PARAM0(temp1);
}
INSTRUCTION (l_sub) {
  orreg_t temp1, temp2, temp3;

  temp3 = (orreg_t)PARAM2;
  temp2 = (orreg_t)PARAM1;
  temp1 = temp2 - temp3;
  SET_PARAM0 (temp1);

  /* Set overflow if a negative value minus a positive value gave a positive
     sum, or if a positive value minus a negative value gave a negative
     sum. Otherwise clear it */
  if ((((long int) temp2 <  0) && 
       ((long int) temp3 >= 0) &&
       ((long int) temp1 >= 0)) ||
      (((long int) temp2 >= 0) && 
       ((long int) temp3 <  0) &&
       ((long int) temp1 <  0)))
    {
      cpu_state.sprs[SPR_SR] |= SPR_SR_OV;
    }
  else
    {
      cpu_state.sprs[SPR_SR] &= ~SPR_SR_OV;
    }

  /* Set the carry flag if (as unsigned values) the second operand is greater
     than the first. */
  if ((uorreg_t) temp3 > (uorreg_t) temp2)
    {
      cpu_state.sprs[SPR_SR] |= SPR_SR_CY;
    }
  else
    {
      cpu_state.sprs[SPR_SR] &= ~SPR_SR_CY;
    }

  /* Trigger a range exception if the overflow flag is set and the SR[OVE] bit
     is set. */
  if (((cpu_state.sprs[SPR_SR] & SPR_SR_OVE) == SPR_SR_OVE) &&
      ((cpu_state.sprs[SPR_SR] & SPR_SR_OV)  == SPR_SR_OV))
    {
      except_handle (EXCEPT_RANGE, cpu_state.pc);
    }
}
/*int mcount = 0;*/
INSTRUCTION (l_mul) {
  orreg_t   temp0, temp1, temp2;
  LONGEST   ltemp0, ltemp1, ltemp2;
  ULONGEST  ultemp0, ultemp1, ultemp2;

  /* Args in 32-bit */
  temp2 = (orreg_t) PARAM2;
  temp1 = (orreg_t) PARAM1;

  /* Compute initially in 64-bit */
  ltemp1 = (LONGEST) temp1;
  ltemp2 = (LONGEST) temp2;
  ltemp0 = ltemp1 * ltemp2;

  temp0  = (orreg_t) (ltemp0  & 0xffffffffLL);
  SET_PARAM0 (temp0);

  /* We have 2's complement overflow, if the result is less than the smallest
     possible 32-bit negative number, or greater than the largest possible
     32-bit positive number. */
  if ((ltemp0 < (LONGEST) INT32_MIN) || (ltemp0 > (LONGEST) INT32_MAX))
    {
      cpu_state.sprs[SPR_SR] |= SPR_SR_OV;
    }
  else
    {
      cpu_state.sprs[SPR_SR] &= ~SPR_SR_OV;
    }

  /* We have 1's complement overflow, if, as an unsigned operation, the result
     is greater than the largest possible 32-bit unsigned number. This is
     probably quicker than unpicking the bits of the signed result. */
  ultemp1 = (ULONGEST) temp1 & 0xffffffffULL;
  ultemp2 = (ULONGEST) temp2 & 0xffffffffULL;
  ultemp0 = ultemp1 * ultemp2;

  if (ultemp0 > (ULONGEST) UINT32_MAX)
    {
      cpu_state.sprs[SPR_SR] |= SPR_SR_CY;
    }
  else
    {
      cpu_state.sprs[SPR_SR] &= ~SPR_SR_CY;
    }

  /* Trigger a range exception if the overflow flag is set and the SR[OVE] bit
     is set. */
  if (((cpu_state.sprs[SPR_SR] & SPR_SR_OVE) == SPR_SR_OVE) &&
      ((cpu_state.sprs[SPR_SR] & SPR_SR_OV)  == SPR_SR_OV))
    {
      except_handle (EXCEPT_RANGE, cpu_state.pc);
    }
}
INSTRUCTION (l_mulu) {
  uorreg_t   temp0, temp1, temp2;
  ULONGEST  ultemp0, ultemp1, ultemp2;

  /* Args in 32-bit */
  temp2 = (uorreg_t) PARAM2;
  temp1 = (uorreg_t) PARAM1;

  /* Compute initially in 64-bit */
  ultemp1 = (ULONGEST) temp1 & 0xffffffffULL;
  ultemp2 = (ULONGEST) temp2 & 0xffffffffULL;
  ultemp0 = ultemp1 * ultemp2;

  temp0  = (uorreg_t) (ultemp0  & 0xffffffffULL);
  SET_PARAM0 (temp0);

  /* We never have 2's complement overflow */
  cpu_state.sprs[SPR_SR] &= ~SPR_SR_OV;

  /* We have 1's complement overflow, if the result is greater than the
     largest possible 32-bit unsigned number. */
  if (ultemp0 > (ULONGEST) UINT32_MAX)
    {
      cpu_state.sprs[SPR_SR] |= SPR_SR_CY;
    }
  else
    {
      cpu_state.sprs[SPR_SR] &= ~SPR_SR_CY;
    }
}
INSTRUCTION (l_div) {
  orreg_t  temp3, temp2, temp1;
  
  temp3 = (orreg_t) PARAM2;
  temp2 = (orreg_t) PARAM1;
 
 /* Check for divide by zero (sets carry) */
  if (0 == temp3)
    {
      cpu_state.sprs[SPR_SR] |= SPR_SR_CY;
    }
  else
    {
      temp1 = temp2 / temp3;
      SET_PARAM0(temp1);
      cpu_state.sprs[SPR_SR] &= ~SPR_SR_CY;
    }

  cpu_state.sprs[SPR_SR] &= ~SPR_SR_OV;	/* Never set */

  /* Trigger a range exception if the overflow flag is set and the SR[OVE] bit
     is set. */
  if (((cpu_state.sprs[SPR_SR] & SPR_SR_OVE) == SPR_SR_OVE) &&
      ((cpu_state.sprs[SPR_SR] & SPR_SR_CY)  == SPR_SR_CY))
    {
      except_handle (EXCEPT_RANGE, cpu_state.pc);
    }
}
INSTRUCTION (l_divu) {
  uorreg_t temp3, temp2, temp1;
  
  temp3 = (uorreg_t) PARAM2;
  temp2 = (uorreg_t) PARAM1;
 
 /* Check for divide by zero (sets carry) */
  if (0 == temp3)
    {
      cpu_state.sprs[SPR_SR] |= SPR_SR_CY;
    }
  else
    {
      temp1 = temp2 / temp3;
      SET_PARAM0(temp1);
      cpu_state.sprs[SPR_SR] &= ~SPR_SR_CY;
    }

  cpu_state.sprs[SPR_SR] &= ~SPR_SR_OV;	/* Never set */

  /* Trigger a range exception if the overflow flag is set and the SR[OVE] bit
     is set. */
  if (((cpu_state.sprs[SPR_SR] & SPR_SR_OVE) == SPR_SR_OVE) &&
      ((cpu_state.sprs[SPR_SR] & SPR_SR_CY)  == SPR_SR_CY))
    {
      except_handle (EXCEPT_RANGE, cpu_state.pc);
    }
}
INSTRUCTION (l_sll) {
  uorreg_t temp1;

  temp1 = PARAM1 << PARAM2;
  SET_PARAM0(temp1);
  /* runtime.sim.cycles += 2; */
}
INSTRUCTION (l_sra) {
  orreg_t temp1;
  
  temp1 = (orreg_t)PARAM1 >> PARAM2;
  SET_PARAM0(temp1);
  /* runtime.sim.cycles += 2; */
}
INSTRUCTION (l_srl) {
  uorreg_t temp1;
  temp1 = PARAM1 >> PARAM2;
  SET_PARAM0(temp1);
  /* runtime.sim.cycles += 2; */
}
INSTRUCTION (l_ror) {
  uorreg_t temp1;
  temp1  = PARAM1 >> (PARAM2 & 0x1f);
  temp1 |= PARAM1 << (32 - (PARAM2 & 0x1f));
  SET_PARAM0(temp1);
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
  /* Badly aligned destination or use of link register triggers an exception */
  uorreg_t  temp1 = PARAM0;

  if (REG_PARAM0 == LINK_REGNO)
    {
      except_handle (EXCEPT_ILLEGAL, cpu_state.pc);
    }
  else if ((temp1 & 0x3) != 0)
    {
      except_handle (EXCEPT_ALIGN, cpu_state.pc);
    }
  else
    {
      cpu_state.pc_delay = temp1;
      setsim_reg(LINK_REGNO, cpu_state.pc + 8);
      next_delay_insn = 1;
    }
}
INSTRUCTION (l_jr) {
  /* Badly aligned destination triggers an exception */
  uorreg_t  temp1 = PARAM0;

  if ((temp1 & 0x3) != 0)
    {
      except_handle (EXCEPT_ALIGN, cpu_state.pc);
    }
  else
    {
      cpu_state.pc_delay = temp1;
      next_delay_insn = 1;

      if (config.sim.profile)
	{
	  fprintf (runtime.sim.fprof, "-%08llX %"PRIxADDR"\n",
		   runtime.sim.cycles, cpu_state.pc_delay);
	}
    }
}
INSTRUCTION (l_rfe) {
  pcnext = cpu_state.sprs[SPR_EPCR_BASE];
  mtspr(SPR_SR, cpu_state.sprs[SPR_ESR_BASE]);
}
INSTRUCTION (l_nop) {
  uint32_t k = PARAM0;
  switch (k)
    {
    case NOP_NOP:
      break;
    case NOP_EXIT:
      PRINTFQ("exit(%"PRIdREG")\n", evalsim_reg (3));
      PRINTFQ("@reset : cycles %lld, insn #%lld\n",
              runtime.sim.reset_cycles, runtime.cpu.reset_instructions);
      PRINTFQ("@exit  : cycles %lld, insn #%lld\n", runtime.sim.cycles,
              runtime.cpu.instructions);
      PRINTFQ(" diff  : cycles %lld, insn #%lld\n",
              runtime.sim.cycles - runtime.sim.reset_cycles,
              runtime.cpu.instructions - runtime.cpu.reset_instructions);
      if (config.sim.is_library)
	{
	  runtime.cpu.halted = 1;
	  set_stall_state (1);
	}
      else
	{
	  sim_done();
	}
      break;
    case NOP_REPORT:
      PRINTF("report(0x%"PRIxREG");\n", evalsim_reg(3));
      break;
    case NOP_PUTC:		/*JPB */
      printf( "%c", (char)(evalsim_reg( 3 ) & 0xff));
      fflush( stdout );
      break;
    case NOP_CNT_RESET:
      PRINTF("****************** counters reset ******************\n");
      PRINTF("cycles %lld, insn #%lld\n", runtime.sim.cycles, runtime.cpu.instructions); 
      PRINTF("****************** counters reset ******************\n");
      runtime.sim.reset_cycles = runtime.sim.cycles;
      runtime.cpu.reset_instructions = runtime.cpu.instructions;
      break;    
    case NOP_GET_TICKS:
      cpu_state.reg[11] = runtime.sim.cycles & 0xffffffff;
      cpu_state.reg[12] = runtime.sim.cycles >> 32;
      break;
    case NOP_GET_PS:
      cpu_state.reg[11] = config.sim.clkcycle_ps;
      break;
    case NOP_TRACE_ON:
      runtime.sim.hush = 0;
      break;
    case NOP_TRACE_OFF:
      runtime.sim.hush = 1;
      break;
    case NOP_RANDOM:
      cpu_state.reg[11] = (unsigned int) (random () & 0xffffffff);
      break;
    case NOP_OR1KSIM:
      cpu_state.reg[11] = 1;
      break;
    default:
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
  uint16_t regno = PARAM0 | PARAM2;
  uorreg_t value = PARAM1;

  if (cpu_state.sprs[SPR_SR] & SPR_SR_SM)
    mtspr(regno, value);
  else {
    PRINTF("WARNING: trying to write SPR while SR[SUPV] is cleared.\n");
    sim_done();
  }
}
INSTRUCTION (l_mfspr) {
  uint16_t regno = PARAM1 | PARAM2;
  uorreg_t value = mfspr(regno);

  if ((cpu_state.sprs[SPR_SR] & SPR_SR_SM) ||
      // TODO: Check if this SPR should actually be allowed to be read with
      // SR's SM==0 and SUMRA==1
      (!(cpu_state.sprs[SPR_SR] & SPR_SR_SM) && 
       (cpu_state.sprs[SPR_SR] & SPR_SR_SUMRA)))
    SET_PARAM0(value);
  else
    {
      SET_PARAM0(0);
      PRINTF("WARNING: trying to read SPR while SR[SUPV] and SR[SUMRA] is cleared.\n");
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
  orreg_t x, y, t;

  lo = cpu_state.sprs[SPR_MACLO];
  hi = cpu_state.sprs[SPR_MACHI];
  x = PARAM0;
  y = PARAM1;
/*   PRINTF ("[%"PRIxREG",%"PRIxREG"]\t", x, y); */

  /* Compute the temporary as (signed) 32-bits, then sign-extend to 64 when
     adding in. */
  l = (ULONGEST)lo | ((LONGEST)hi << 32);
  t = x * y;
  l += (LONGEST) t;

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
  orreg_t lo;
  /* No need for synchronization here -- all MAC instructions are 1 cycle long.  */
  lo =  cpu_state.sprs[SPR_MACLO];
  //PRINTF ("<%08x>\n", (unsigned long)l);
  SET_PARAM0(lo);
  cpu_state.sprs[SPR_MACLO] = 0;
  cpu_state.sprs[SPR_MACHI] = 0;
}
INSTRUCTION (l_cmov) {
  SET_PARAM0(cpu_state.sprs[SPR_SR] & SPR_SR_F ? PARAM1 : PARAM2);
}
INSTRUCTION (l_ff1) {
  SET_PARAM0(ffs(PARAM1));
}
INSTRUCTION (l_fl1) {
  orreg_t t = (orreg_t)PARAM1;

  /* Reverse the word and use ffs */
  t = (((t & 0xaaaaaaaa) >> 1) | ((t & 0x55555555) << 1));
  t = (((t & 0xcccccccc) >> 2) | ((t & 0x33333333) << 2));
  t = (((t & 0xf0f0f0f0) >> 4) | ((t & 0x0f0f0f0f) << 4));
  t = (((t & 0xff00ff00) >> 8) | ((t & 0x00ff00ff) << 8));
  t = ffs ((t >> 16) | (t << 16));
  
  SET_PARAM0 (0 == t ? t : 33 - t);
}
/******* Floating point instructions *******/
/* Do calculation, and update FPCSR as required */
/* Single precision */
INSTRUCTION (lf_add_s) {
  if (config.cpu.hardfloat) {
  float_set_rm();
  SET_PARAM0(float32_add((unsigned int)PARAM1,(unsigned int)PARAM2));
  float_set_flags();
  } else l_invalid();
}
INSTRUCTION (lf_div_s) {
  if (config.cpu.hardfloat) {
  float_set_rm();
  SET_PARAM0(float32_div((unsigned int)PARAM1,(unsigned int)PARAM2));
  float_set_flags();
  } else l_invalid();
}
INSTRUCTION (lf_ftoi_s) {
  if (config.cpu.hardfloat) {
  float_set_rm();
  SET_PARAM0(float32_to_int32((unsigned int)PARAM1));
  float_set_flags();
  } else l_invalid();
}
INSTRUCTION (lf_itof_s) {
  if (config.cpu.hardfloat) {
  float_set_rm();
  SET_PARAM0(int32_to_float32((unsigned int)PARAM1));
  float_set_flags();
  } else l_invalid();
}
INSTRUCTION (lf_madd_s) {
  if (config.cpu.hardfloat) {
  float_set_rm();
  SET_PARAM0(float32_add((unsigned int)PARAM0, float32_mul((unsigned int)PARAM1,(unsigned int)PARAM2)));
  // Note: this ignores flags from the multiply!
  float_set_flags();
  } else l_invalid();
}
INSTRUCTION (lf_mul_s) {
  if (config.cpu.hardfloat) {
  float_set_rm();
  SET_PARAM0(float32_mul((unsigned int)PARAM1,(unsigned int)PARAM2));
  float_set_flags();
  } else l_invalid();
}
INSTRUCTION (lf_rem_s) {
  if (config.cpu.hardfloat) {
  float_set_rm();
  SET_PARAM0(float32_rem((unsigned int)PARAM1,(unsigned int)PARAM2));
  float_set_flags();
  } else l_invalid();
}
INSTRUCTION (lf_sfeq_s) {
  if (config.cpu.hardfloat) {
  float_set_rm();
  if(float32_eq((unsigned int)PARAM0, (unsigned int)PARAM1))
    cpu_state.sprs[SPR_SR] |= SPR_SR_F;
  else
    cpu_state.sprs[SPR_SR] &= ~SPR_SR_F;
  float_set_flags();
  } else l_invalid();
}
INSTRUCTION (lf_sfge_s) {
  if (config.cpu.hardfloat) {
  float_set_rm();
  if((!float32_lt((unsigned int)PARAM0, (unsigned int)PARAM1) & 
      !float32_is_nan( (unsigned int)PARAM0) &
      !float32_is_nan( (unsigned int)PARAM1) ) )
    cpu_state.sprs[SPR_SR] |= SPR_SR_F;  
  else
    cpu_state.sprs[SPR_SR] &= ~SPR_SR_F;
  float_set_flags();
  } else l_invalid();
}
INSTRUCTION (lf_sfgt_s) {
  if (config.cpu.hardfloat) {
  float_set_rm();
  if((!float32_le((unsigned int)PARAM0, (unsigned int)PARAM1)  & 
      !float32_is_nan( (unsigned int)PARAM0) &
      !float32_is_nan( (unsigned int)PARAM1) ) )
    cpu_state.sprs[SPR_SR] |= SPR_SR_F;
  else
    cpu_state.sprs[SPR_SR] &= ~SPR_SR_F;
  float_set_flags();
  } else l_invalid();
}
INSTRUCTION (lf_sfle_s) {
  if (config.cpu.hardfloat) {
  float_set_rm();
  if((float32_le((unsigned int)PARAM0, (unsigned int)PARAM1) & 
      !float32_is_nan( (unsigned int)PARAM0) &
      !float32_is_nan( (unsigned int)PARAM1) ) )    
    cpu_state.sprs[SPR_SR] |= SPR_SR_F;
  else
    cpu_state.sprs[SPR_SR] &= ~SPR_SR_F;
  float_set_flags();
  } else l_invalid();
}
INSTRUCTION (lf_sflt_s) {
  if (config.cpu.hardfloat) {
  float_set_rm();
  if(( float32_lt((unsigned int)PARAM0, (unsigned int)PARAM1) & 
       !float32_is_nan( (unsigned int)PARAM0) &
       !float32_is_nan( (unsigned int)PARAM1) ) )
    cpu_state.sprs[SPR_SR] |= SPR_SR_F;
  else
    cpu_state.sprs[SPR_SR] &= ~SPR_SR_F;
  float_set_flags();
  } else l_invalid();
}
INSTRUCTION (lf_sfne_s) {
  if (config.cpu.hardfloat) {
  float_set_rm();
  if(!float32_eq((unsigned int)PARAM0, (unsigned int)PARAM1))    
    cpu_state.sprs[SPR_SR] |= SPR_SR_F;
  else
    cpu_state.sprs[SPR_SR] &= ~SPR_SR_F;
  float_set_flags();
  } else l_invalid();
}
INSTRUCTION (lf_sub_s) {
  if (config.cpu.hardfloat) {
  float_set_rm();
  SET_PARAM0(float32_sub((unsigned int)PARAM1,(unsigned int)PARAM2));
  float_set_flags();
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
