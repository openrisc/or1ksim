/* op-swhb-op.h -- Micro operations template for store operations

   Copyright (C) 2005 György `nog' Jeney, nog@sdf.lonestar.org
   Copyright (C) 2008 Embecosm Limited

   Contributor Jeremy Bennett <jeremy.bennett@embecosm.com>

   This file is part of Or1ksim, the OpenRISC 1000 Architectural Simulator.

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the Free
   Software Foundation; either version 3 of the License, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
   more details.

   You should have received a copy of the GNU General Public License along
   with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* This program is commented throughout in a fashion suitable for processing
   with Doxygen. */

/* FIXME: Do something with breakpoint */


#ifdef OP_2T
__or_dynop void glue (glue (op_, S_OP_NAME), T) (void)
{
  int breakpoint;
  S_FUNC (T0 + OP_PARAM1, T1, &breakpoint);
}
#endif

#ifdef OP_1T
__or_dynop void glue (glue (glue (op_, S_OP_NAME), _imm), T) (void)
{
  int breakpoint;
  S_FUNC (OP_PARAM1, T0, &breakpoint);
}

__or_dynop void glue (glue (glue (op_, S_OP_NAME), _clear), T) (void)
{
  int breakpoint;
  S_FUNC (T0 + OP_PARAM1, 0, &breakpoint);
}
#endif

#if !defined(OP_1T) && !defined(OP_2T)
__or_dynop void glue (glue (op_, S_OP_NAME), _clear_imm) (void)
{
  int breakpoint;
  S_FUNC (OP_PARAM1, 0, &breakpoint);
}
#endif
