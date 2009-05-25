/* insn.h -- OpenRISC Custom Unit Compiler, internal instruction definitions

   Copyright (C) 2002 Marko Mlinar, markom@opencores.org
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


#ifndef INSN__H
#define INSN__H

#include "cuc.h"

/* Instruction types */
#define II_ADD   0
#define II_SUB   1
#define II_AND   2
#define II_OR    3
#define II_XOR   4
#define II_MUL   5
#define II_SRL   6
#define II_SLL   7
#define II_SRA   8
#define II_LB    9
#define II_LH    10
#define II_LW    11
#define II_SB    12
#define II_SH    13
#define II_SW    14
#define II_SFEQ  15
#define II_SFNE  16
#define II_SFLE  17
#define II_SFLT  18
#define II_SFGE  19
#define II_SFGT  20
#define II_BF    21
#define II_LRBB  22
#define II_CMOV  23
#define II_REG   24
#define II_NOP   25
#define II_CALL  26
#define II_LAST  26

/* misc flags */
#define II_MASK   0x0fff
#define II_MEM    0x1000
#define II_SIGNED 0x2000

#define II_IS_LOAD(x) ((x) == II_LB || (x) == II_LH || (x) == II_LW)
#define II_IS_STORE(x) ((x) == II_SB || (x) == II_SH || (x) == II_SW)
#define II_MEM_WIDTH(x) (((x) == II_LB || (x) == II_SB) ? 1 :\
                         ((x) == II_LH || (x) == II_SH) ? 2 :\
                         ((x) == II_LW || (x) == II_SW) ? 4 : -1)

/* List of known instructions and their rtl representation */
typedef struct
{
  char *name;
  int comutative;
  char *rtl;
} cuc_known_insn;

extern const cuc_known_insn known[II_LAST + 1];

/* Timing table -- same indexes as known table */
typedef struct
{
  double delay;
  double size;
  double delayi;
  double sizei;
} cuc_timing_table;

/* Conversion links */
typedef struct
{
  const char *from;
  const int to;
} cuc_conv;

/* normal (not immediate) size of a function */
double ii_size (int index, int imm);

/* Returns instruction size */
double insn_time (cuc_insn * ii);

/* Returns instruction time */
double insn_size (cuc_insn * ii);

/* Find known instruction and attach them to insn */
void change_insn_type (cuc_insn * i, int index);

/* Returns instruction name */
const char *cuc_insn_name (cuc_insn * ii);

/* Loads in the specified timings table */
void load_timing_table (char *filename);

/* Displays shared instructions */
void print_shared (cuc_func * rf, cuc_shared_item * shared, int nshared);

#endif	/* INSN__H */
