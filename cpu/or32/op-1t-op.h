/* op-1t-op.h -- Micro operations useing only one temporary
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

__or_dynop void glue(op_imm, T)(void)
{
  T0 = OP_PARAM1;
}

__or_dynop void glue(op_clear, T)(void)
{
  T0 = 0;
}

__or_dynop void glue(op_check_null_except_delay, T)(void)
{
  if(!T0) {
    /* Do exception */
    env->sprs[SPR_EEAR_BASE] = env->pc - 4;
    env->delay_insn = 0;
    do_jump(EXCEPT_ILLEGAL);
  }
}

__or_dynop void glue(op_check_null_except, T)(void)
{
  if(!T0) {
    /* Do exception */
    env->sprs[SPR_EEAR_BASE] = env->pc;
    do_jump(EXCEPT_ILLEGAL);
  }
}

__or_dynop void glue(op_calc_insn_ea, T)(void)
{
  env->insn_ea = T0 + OP_PARAM1;
}

__or_dynop void glue(op_macrc, T)(void)
{
  /* FIXME: How is this supposed to work?  The architechture manual says that
   * the low 32-bits shall be saved into rD.  I have just copied this code from
   * insnset.c to make testbench/mul pass */
  int64_t temp = env->sprs[SPR_MACLO] | ((int64_t)env->sprs[SPR_MACHI] << 32);

  T0 = (orreg_t)(temp >> 28);
  env->sprs[SPR_MACLO] = 0;
  env->sprs[SPR_MACHI] = 0;
}

__or_dynop void glue(op_mac_imm, T)(void)
{
  int64_t temp = env->sprs[SPR_MACLO] | ((int64_t)env->sprs[SPR_MACHI] << 32);

  temp += (int64_t)T0 * (int64_t)OP_PARAM1;

  env->sprs[SPR_MACLO] = temp & 0xffffffff;
  env->sprs[SPR_MACHI] = temp >> 32;
}

