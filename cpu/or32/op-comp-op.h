/* op-comp-op.h -- Micro operations template for comparison operations
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

#ifdef OP_2T
__or_dynop void glue(glue(op_, COMP_NAME), T)(void)
{
  if(COMP_CAST(T0) COMP COMP_CAST(T1))
    env->sprs[SPR_SR] |= SPR_SR_F;
  else
    env->sprs[SPR_SR] &= ~SPR_SR_F;
  FORCE_RET;
}
#endif

#ifdef OP_1T
__or_dynop void glue(glue(glue(op_, COMP_NAME), _imm), T)(void)
{
  if(COMP_CAST(T0) COMP COMP_CAST(OP_PARAM1))
    env->sprs[SPR_SR] |= SPR_SR_F;
  else
    env->sprs[SPR_SR] &= ~SPR_SR_F;
  FORCE_RET;
}

__or_dynop void glue(glue(glue(op_, COMP_NAME), _null), T)(void)
{
  if(COMP_CAST(T0) COMP 0)
    env->sprs[SPR_SR] |= SPR_SR_F;
  else
    env->sprs[SPR_SR] &= ~SPR_SR_F;
  FORCE_RET;
}
#endif

