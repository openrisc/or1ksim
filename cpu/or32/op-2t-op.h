/* op-1t-op.h -- Micro operations useing two temporaries
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

__or_dynop void glue(op_move, T)(void)
{
  T0 = T1;
}

__or_dynop void glue(op_ff1, T)(void)
{
  int i;

  for(i = 0; i < 32; i++, T0 >>= 1) {
    if(T0 & 1) {
      T1 = i;
      break;
    }
  }

  FORCE_RET;
}


__or_dynop void glue(op_neg, T)(void)
{
  T0 = -T1;
}

