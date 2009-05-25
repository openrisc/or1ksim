/* op-mac-op.h -- Micro operations template for mac operations
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


__or_dynop void glue(glue(op_, OP_NAME), T)(void)
{
  int64_t temp = env->sprs[SPR_MACLO] | ((int64_t)env->sprs[SPR_MACHI] << 32);

  temp OP (int64_t)T0 * (int64_t)T1;

  env->sprs[SPR_MACLO] = temp & 0xffffffff;
  env->sprs[SPR_MACHI] = temp >> 32;
}

