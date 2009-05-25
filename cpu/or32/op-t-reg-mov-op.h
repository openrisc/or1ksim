/* op-t-reg-mov-op.h -- Micro operations template for reg<->temporary operations
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


__or_dynop void glue(op_gtt_gpr1, T)(void)
{
  T0 = env->reg[1];
}

__or_dynop void glue(op_gtt_gpr2, T)(void)
{
  T0 = env->reg[2];
}

__or_dynop void glue(op_gtt_gpr3, T)(void)
{
  T0 = env->reg[3];
}

__or_dynop void glue(op_gtt_gpr4, T)(void)
{
  T0 = env->reg[4];
}

__or_dynop void glue(op_gtt_gpr5, T)(void)
{
  T0 = env->reg[5];
}

__or_dynop void glue(op_gtt_gpr6, T)(void)
{
  T0 = env->reg[6];
}

__or_dynop void glue(op_gtt_gpr7, T)(void)
{
  T0 = env->reg[7];
}

__or_dynop void glue(op_gtt_gpr8, T)(void)
{
  T0 = env->reg[8];
}

__or_dynop void glue(op_gtt_gpr9, T)(void)
{
  T0 = env->reg[9];
}

__or_dynop void glue(op_gtt_gpr10, T)(void)
{
  T0 = env->reg[10];
}

__or_dynop void glue(op_gtt_gpr11, T)(void)
{
  T0 = env->reg[11];
}

__or_dynop void glue(op_gtt_gpr12, T)(void)
{
  T0 = env->reg[12];
}

__or_dynop void glue(op_gtt_gpr13, T)(void)
{
  T0 = env->reg[13];
}

__or_dynop void glue(op_gtt_gpr14, T)(void)
{
  T0 = env->reg[14];
}

__or_dynop void glue(op_gtt_gpr15, T)(void)
{
  T0 = env->reg[15];
}

__or_dynop void glue(op_gtt_gpr16, T)(void)
{
  T0 = env->reg[16];
}

__or_dynop void glue(op_gtt_gpr17, T)(void)
{
  T0 = env->reg[17];
}

__or_dynop void glue(op_gtt_gpr18, T)(void)
{
  T0 = env->reg[18];
}

__or_dynop void glue(op_gtt_gpr19, T)(void)
{
  T0 = env->reg[19];
}

__or_dynop void glue(op_gtt_gpr20, T)(void)
{
  T0 = env->reg[20];
}

__or_dynop void glue(op_gtt_gpr21, T)(void)
{
  T0 = env->reg[21];
}

__or_dynop void glue(op_gtt_gpr22, T)(void)
{
  T0 = env->reg[22];
}

__or_dynop void glue(op_gtt_gpr23, T)(void)
{
  T0 = env->reg[23];
}

__or_dynop void glue(op_gtt_gpr24, T)(void)
{
  T0 = env->reg[24];
}

__or_dynop void glue(op_gtt_gpr25, T)(void)
{
  T0 = env->reg[25];
}

__or_dynop void glue(op_gtt_gpr26, T)(void)
{
  T0 = env->reg[26];
}

__or_dynop void glue(op_gtt_gpr27, T)(void)
{
  T0 = env->reg[27];
}

__or_dynop void glue(op_gtt_gpr28, T)(void)
{
  T0 = env->reg[28];
}

__or_dynop void glue(op_gtt_gpr29, T)(void)
{
  T0 = env->reg[29];
}

__or_dynop void glue(op_gtt_gpr30, T)(void)
{
  T0 = env->reg[30];
}

__or_dynop void glue(op_gtt_gpr31, T)(void)
{
  T0 = env->reg[31];
}

__or_dynop void glue(op_ttg_gpr1, T)(void)
{
  env->reg[1] = T0;
}

__or_dynop void glue(op_ttg_gpr2, T)(void)
{
  env->reg[2] = T0;
}

__or_dynop void glue(op_ttg_gpr3, T)(void)
{
  env->reg[3] = T0;
}

__or_dynop void glue(op_ttg_gpr4, T)(void)
{
  env->reg[4] = T0;
}

__or_dynop void glue(op_ttg_gpr5, T)(void)
{
  env->reg[5] = T0;
}

__or_dynop void glue(op_ttg_gpr6, T)(void)
{
  env->reg[6] = T0;
}

__or_dynop void glue(op_ttg_gpr7, T)(void)
{
  env->reg[7] = T0;
}

__or_dynop void glue(op_ttg_gpr8, T)(void)
{
  env->reg[8] = T0;
}

__or_dynop void glue(op_ttg_gpr9, T)(void)
{
  env->reg[9] = T0;
}

__or_dynop void glue(op_ttg_gpr10, T)(void)
{
  env->reg[10] = T0;
}

__or_dynop void glue(op_ttg_gpr11, T)(void)
{
  env->reg[11] = T0;
}

__or_dynop void glue(op_ttg_gpr12, T)(void)
{
  env->reg[12] = T0;
}

__or_dynop void glue(op_ttg_gpr13, T)(void)
{
  env->reg[13] = T0;
}

__or_dynop void glue(op_ttg_gpr14, T)(void)
{
  env->reg[14] = T0;
}

__or_dynop void glue(op_ttg_gpr15, T)(void)
{
  env->reg[15] = T0;
}

__or_dynop void glue(op_ttg_gpr16, T)(void)
{
  env->reg[16] = T0;
}

__or_dynop void glue(op_ttg_gpr17, T)(void)
{
  env->reg[17] = T0;
}

__or_dynop void glue(op_ttg_gpr18, T)(void)
{
  env->reg[18] = T0;
}

__or_dynop void glue(op_ttg_gpr19, T)(void)
{
  env->reg[19] = T0;
}

__or_dynop void glue(op_ttg_gpr20, T)(void)
{
  env->reg[20] = T0;
}

__or_dynop void glue(op_ttg_gpr21, T)(void)
{
  env->reg[21] = T0;
}

__or_dynop void glue(op_ttg_gpr22, T)(void)
{
  env->reg[22] = T0;
}

__or_dynop void glue(op_ttg_gpr23, T)(void)
{
  env->reg[23] = T0;
}

__or_dynop void glue(op_ttg_gpr24, T)(void)
{
  env->reg[24] = T0;
}

__or_dynop void glue(op_ttg_gpr25, T)(void)
{
  env->reg[25] = T0;
}

__or_dynop void glue(op_ttg_gpr26, T)(void)
{
  env->reg[26] = T0;
}

__or_dynop void glue(op_ttg_gpr27, T)(void)
{
  env->reg[27] = T0;
}

__or_dynop void glue(op_ttg_gpr28, T)(void)
{
  env->reg[28] = T0;
}

__or_dynop void glue(op_ttg_gpr29, T)(void)
{
  env->reg[29] = T0;
}

__or_dynop void glue(op_ttg_gpr30, T)(void)
{
  env->reg[30] = T0;
}

__or_dynop void glue(op_ttg_gpr31, T)(void)
{
  env->reg[31] = T0;
}

