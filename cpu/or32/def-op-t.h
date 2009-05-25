/* def_op_t.h -- Operation-generation strucutre definitions helpers
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

#define GPR_T(op_name, temp) \
 { NULL, \
   glue(glue(op_name, 1), temp), \
   glue(glue(op_name, 2), temp), \
   glue(glue(op_name, 3), temp), \
   glue(glue(op_name, 4), temp), \
   glue(glue(op_name, 5), temp), \
   glue(glue(op_name, 6), temp), \
   glue(glue(op_name, 7), temp), \
   glue(glue(op_name, 8), temp), \
   glue(glue(op_name, 9), temp), \
   glue(glue(op_name, 10), temp), \
   glue(glue(op_name, 11), temp), \
   glue(glue(op_name, 12), temp), \
   glue(glue(op_name, 13), temp), \
   glue(glue(op_name, 14), temp), \
   glue(glue(op_name, 15), temp), \
   glue(glue(op_name, 16), temp), \
   glue(glue(op_name, 17), temp), \
   glue(glue(op_name, 18), temp), \
   glue(glue(op_name, 19), temp), \
   glue(glue(op_name, 20), temp), \
   glue(glue(op_name, 21), temp), \
   glue(glue(op_name, 22), temp), \
   glue(glue(op_name, 23), temp), \
   glue(glue(op_name, 24), temp), \
   glue(glue(op_name, 25), temp), \
   glue(glue(op_name, 26), temp), \
   glue(glue(op_name, 27), temp), \
   glue(glue(op_name, 28), temp), \
   glue(glue(op_name, 29), temp), \
   glue(glue(op_name, 30), temp), \
   glue(glue(op_name, 31), temp) }

#if NUM_T_REGS == 3

#define OP_ROW(op_name) \
 { glue(op_name, _t0), glue(op_name, _t1), glue(op_name, _t2) }

#define OP_ROW_NEQ0(op_name) \
 { NULL, glue(op_name, _t1), glue(op_name, _t2) }

#define OP_ROW_NEQ1(op_name) \
 { glue(op_name, _t0), NULL, glue(op_name, _t2) }

#define OP_ROW_NEQ2(op_name) \
 { glue(op_name, _t0), glue(op_name, _t1), NULL }

#define OP_ROW_COL(op_name) { \
 OP_ROW(glue(op_name, _t0)), \
 OP_ROW(glue(op_name, _t1)), \
 OP_ROW(glue(op_name, _t2)), }

/* In 3D space: row, column, ??? */
#define OP_ROW_COL_3D(op_name) { \
  OP_ROW_COL(glue(op_name, _t0)), \
  OP_ROW_COL(glue(op_name, _t1)), \
  OP_ROW_COL(glue(op_name, _t2)) }

#define OP_ROW_COL_NEQ(op_name) { \
 OP_ROW_NEQ0(glue(op_name, _t0)), \
 OP_ROW_NEQ1(glue(op_name, _t1)), \
 OP_ROW_NEQ2(glue(op_name, _t2)) }

#define OP_ROW_COL_NEQ0(op_name) { \
 OP_ROW_NEQ0(glue(op_name, _t0)), \
 OP_ROW(glue(op_name, _t1)), \
 OP_ROW(glue(op_name, _t2)) }

#define OP_ROW_COL_NEQ1(op_name) { \
 OP_ROW(glue(op_name, _t0)), \
 OP_ROW_NEQ1(glue(op_name, _t1)), \
 OP_ROW(glue(op_name, _t2)) }

#define OP_ROW_COL_NEQ2(op_name) { \
 OP_ROW(glue(op_name, _t0)), \
 OP_ROW(glue(op_name, _t1)), \
 OP_ROW_NEQ2(glue(op_name, _t2)) }

#define OP_ROW_COL_3D_NEQ(op_name) { \
 OP_ROW_COL_NEQ0(glue(op_name, _t0)), \
 OP_ROW_COL_NEQ1(glue(op_name, _t1)), \
 OP_ROW_COL_NEQ2(glue(op_name, _t2)) }

#define GPR_ROW_COL(op_name) { \
 GPR_T(op_name, _t0), \
 GPR_T(op_name, _t1), \
 GPR_T(op_name, _t2) }

#else
#error Update def_op_t.h for NUM_T_REGS temporaries
#endif

#define DEF_1T_OP(type, name, op_name) \
 static const type name[NUM_T_REGS] = \
  OP_ROW(op_name)

#define DEF_2T_OP(type, name, op_name) \
 static const type name[NUM_T_REGS][NUM_T_REGS] = \
  OP_ROW_COL(op_name)

#define DEF_3T_OP(type, name, op_name) \
 static const type name[NUM_T_REGS][NUM_T_REGS][NUM_T_REGS] = \
  OP_ROW_COL_3D(op_name)

/* Same as above but put NULL in places where T0 == T1 */
#define DEF_2T_OP_NEQ(type, name, op_name) \
 static const type name[NUM_T_REGS][NUM_T_REGS] = \
  OP_ROW_COL_NEQ(op_name)

/* Same as above but put NULL in places where T0 == T1 == T2 */
#define DEF_3T_OP_NEQ(type, name, op_name) \
 static const type name[NUM_T_REGS][NUM_T_REGS][NUM_T_REGS] = \
  OP_ROW_COL_3D_NEQ(op_name)

#define DEF_GPR_OP(type, name, op_name) \
 static const generic_gen_op name[NUM_T_REGS][32] = \
  GPR_ROW_COL(op_name)

