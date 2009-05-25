/* op-ff1-op.h -- Micro operations template for the ff1 instruction
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


__or_dynop void glue(glue(glue(op_ff1_, DST_T), _), SRC_T)(void)
{
  int i;

  for(i = 0; i < 32; i++, SRC_T >>= 1) {
    if(SRC_T & 1) {
      DST_T = i;
      break;
    }
  }

  FORCE_RET;
}


