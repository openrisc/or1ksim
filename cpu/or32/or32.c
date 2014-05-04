/* Table of opcodes for the OpenRISC 1000 ISA.

   Copyright 1990, 1991, 1992, 1993 Free Software Foundation, Inc.
   Copyright (C) 2008 Embecosm Limited

   Contributed by Damjan Lampret (lampret@opencores.org).
   Contributor Jeremy Bennett <jeremy.bennett@embecosm.com>
   
   This file is part of Or1ksim, the OpenRISC 1000 Architectural Simulator.
   This file is also part of gen_or1k_isa, GDB and GAS.

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


/* Autoconf and/or portability configuration */
#include "config.h"
#include "port.h"

/* System includes */
#include <stdlib.h>
#include <stdio.h>

/* Package includes */
#include "opcode/or32.h"

/* We treat all letters the same in encode/decode routines so
   we need to assign some characteristics to them like signess etc.*/
CONST struct or32_letter or32_letters[] = {
  {'A', NUM_UNSIGNED},
  {'B', NUM_UNSIGNED},
  {'D', NUM_UNSIGNED},
  {'I', NUM_SIGNED},
  {'K', NUM_UNSIGNED},
  {'L', NUM_UNSIGNED},
  {'N', NUM_SIGNED},
  {'0', NUM_UNSIGNED},
  {'\0', 0}			/* dummy entry */
};

/* Opcode encoding:
   machine[31:30]: first two bits of opcode
   		   00 - neither of source operands is GPR
   		   01 - second source operand is GPR (rB)
   		   10 - first source operand is GPR (rA)
   		   11 - both source operands are GPRs (rA and rB)
   machine[29:26]: next four bits of opcode
   machine[25:00]: instruction operands (specific to individual instruction)

  Recommendation: irrelevant instruction bits should be set with a value of
  bits in same positions of instruction preceding current instruction in the
  code (when assembling).
*/

#ifdef HAVE_EXECUTION
# if SIMPLE_EXECUTION
#  define EFN &l_none
#  define EF(func) &(func)
#  define EFI &l_invalid
# elif COMPLEX_EXECUTION
#  define EFN "l_none"
#  define EFI "l_invalid"
#  ifdef __GNUC__
#   define EF(func) #func
#  else
#   define EF(func) "func"
#  endif
# endif
#else /* HAVE_EXECUTION */
# define EFN &l_none
# define EF(func) EFN
# define EFI EFN
#endif /* HAVE_EXECUTION */

CONST struct or32_opcode or1ksim_or32_opcodes[] = {

  {"l.j", "N", "00 0x0  NNNNN NNNNN NNNN NNNN NNNN NNNN",
   EF (l_j), OR32_IF_DELAY, it_jump},
  {"l.jal", "N", "00 0x1  NNNNN NNNNN NNNN NNNN NNNN NNNN",
   EF (l_jal), OR32_IF_DELAY, it_jump},
  {"l.bnf", "N", "00 0x3  NNNNN NNNNN NNNN NNNN NNNN NNNN",
   EF (l_bnf), OR32_IF_DELAY | OR32_R_FLAG, it_branch},
  {"l.bf", "N", "00 0x4  NNNNN NNNNN NNNN NNNN NNNN NNNN",
   EF (l_bf), OR32_IF_DELAY | OR32_R_FLAG, it_branch},
  {"l.nop", "K", "00 0x5  01--- ----- KKKK KKKK KKKK KKKK",
   EF (l_nop), 0, it_nop},
  {"l.movhi", "rD,K", "00 0x6  DDDDD ----0 KKKK KKKK KKKK KKKK",
   EF (l_movhi), 0, it_movimm},
  {"l.macrc", "rD", "00 0x6  DDDDD ----1 0000 0000 0000 0000",
   EF (l_macrc), 0, it_mac},
  {"l.sys", "K", "00 0x8  00000 00000 KKKK KKKK KKKK KKKK",
   EF (l_sys), 0, it_exception},
  {"l.trap", "K", "00 0x8  01000 00000 KKKK KKKK KKKK KKKK",
   EF (l_trap), 0, it_exception},
  {"l.msync", "", "00 0x8  10000 00000 0000 0000 0000 0000", EFN,
   0, it_unknown},
  {"l.psync", "", "00 0x8  10100 00000 0000 0000 0000 0000", EFN,
   0, it_unknown},
  {"l.csync", "", "00 0x8  11000 00000 0000 0000 0000 0000", EFN,
   0, it_unknown},
  {"l.rfe", "", "00 0x9  ----- ----- ---- ---- ---- ----",
   EF (l_rfe), 0, it_exception},
  {"lv.all_eq.b", "rD,rA,rB", "00 0xA  DDDDD AAAAA BBBB B--- 0x1 0x0", EFI, 0,
   it_unknown},
  {"lv.all_eq.h", "rD,rA,rB", "00 0xA  DDDDD AAAAA BBBB B--- 0x1 0x1", EFI, 0,
   it_unknown},
  {"lv.all_ge.b", "rD,rA,rB", "00 0xA  DDDDD AAAAA BBBB B--- 0x1 0x2", EFI, 0,
   it_unknown},
  {"lv.all_ge.h", "rD,rA,rB", "00 0xA  DDDDD AAAAA BBBB B--- 0x1 0x3", EFI, 0,
   it_unknown},
  {"lv.all_gt.b", "rD,rA,rB", "00 0xA  DDDDD AAAAA BBBB B--- 0x1 0x4", EFI, 0,
   it_unknown},
  {"lv.all_gt.h", "rD,rA,rB", "00 0xA  DDDDD AAAAA BBBB B--- 0x1 0x5", EFI, 0,
   it_unknown},
  {"lv.all_le.b", "rD,rA,rB", "00 0xA  DDDDD AAAAA BBBB B--- 0x1 0x6", EFI, 0,
   it_unknown},
  {"lv.all_le.h", "rD,rA,rB", "00 0xA  DDDDD AAAAA BBBB B--- 0x1 0x7", EFI, 0,
   it_unknown},
  {"lv.all_lt.b", "rD,rA,rB", "00 0xA  DDDDD AAAAA BBBB B--- 0x1 0x8", EFI, 0,
   it_unknown},
  {"lv.all_lt.h", "rD,rA,rB", "00 0xA  DDDDD AAAAA BBBB B--- 0x1 0x9", EFI, 0,
   it_unknown},
  {"lv.all_ne.b", "rD,rA,rB", "00 0xA  DDDDD AAAAA BBBB B--- 0x1 0xA", EFI, 0,
   it_unknown},
  {"lv.all_ne.h", "rD,rA,rB", "00 0xA  DDDDD AAAAA BBBB B--- 0x1 0xB", EFI, 0,
   it_unknown},
  {"lv.any_eq.b", "rD,rA,rB", "00 0xA  DDDDD AAAAA BBBB B--- 0x2 0x0", EFI, 0,
   it_unknown},
  {"lv.any_eq.h", "rD,rA,rB", "00 0xA  DDDDD AAAAA BBBB B--- 0x2 0x1", EFI, 0,
   it_unknown},
  {"lv.any_ge.b", "rD,rA,rB", "00 0xA  DDDDD AAAAA BBBB B--- 0x2 0x2", EFI, 0,
   it_unknown},
  {"lv.any_ge.h", "rD,rA,rB", "00 0xA  DDDDD AAAAA BBBB B--- 0x2 0x3", EFI, 0,
   it_unknown},
  {"lv.any_gt.b", "rD,rA,rB", "00 0xA  DDDDD AAAAA BBBB B--- 0x2 0x4", EFI, 0,
   it_unknown},
  {"lv.any_gt.h", "rD,rA,rB", "00 0xA  DDDDD AAAAA BBBB B--- 0x2 0x5", EFI, 0,
   it_unknown},
  {"lv.any_le.b", "rD,rA,rB", "00 0xA  DDDDD AAAAA BBBB B--- 0x2 0x6", EFI, 0,
   it_unknown},
  {"lv.any_le.h", "rD,rA,rB", "00 0xA  DDDDD AAAAA BBBB B--- 0x2 0x7", EFI, 0,
   it_unknown},
  {"lv.any_lt.b", "rD,rA,rB", "00 0xA  DDDDD AAAAA BBBB B--- 0x2 0x8", EFI, 0,
   it_unknown},
  {"lv.any_lt.h", "rD,rA,rB", "00 0xA  DDDDD AAAAA BBBB B--- 0x2 0x9", EFI, 0,
   it_unknown},
  {"lv.any_ne.b", "rD,rA,rB", "00 0xA  DDDDD AAAAA BBBB B--- 0x2 0xA", EFI, 0,
   it_unknown},
  {"lv.any_ne.h", "rD,rA,rB", "00 0xA  DDDDD AAAAA BBBB B--- 0x2 0xB", EFI, 0,
   it_unknown},
  {"lv.add.b", "rD,rA,rB", "00 0xA  DDDDD AAAAA BBBB B--- 0x3 0x0", EFI, 0,
   it_unknown},
  {"lv.add.h", "rD,rA,rB", "00 0xA  DDDDD AAAAA BBBB B--- 0x3 0x1", EFI, 0,
   it_unknown},
  {"lv.adds.b", "rD,rA,rB", "00 0xA  DDDDD AAAAA BBBB B--- 0x3 0x2", EFI, 0,
   it_unknown},
  {"lv.adds.h", "rD,rA,rB", "00 0xA  DDDDD AAAAA BBBB B--- 0x3 0x3", EFI, 0,
   it_unknown},
  {"lv.addu.b", "rD,rA,rB", "00 0xA  DDDDD AAAAA BBBB B--- 0x3 0x4", EFI, 0,
   it_unknown},
  {"lv.addu.h", "rD,rA,rB", "00 0xA  DDDDD AAAAA BBBB B--- 0x3 0x5", EFI, 0,
   it_unknown},
  {"lv.addus.b", "rD,rA,rB", "00 0xA  DDDDD AAAAA BBBB B--- 0x3 0x6", EFI, 0,
   it_unknown},
  {"lv.addus.h", "rD,rA,rB", "00 0xA  DDDDD AAAAA BBBB B--- 0x3 0x7", EFI, 0,
   it_unknown},
  {"lv.and", "rD,rA,rB", "00 0xA  DDDDD AAAAA BBBB B--- 0x3 0x8", EFI, 0,
   it_unknown},
  {"lv.avg.b", "rD,rA,rB", "00 0xA  DDDDD AAAAA BBBB B--- 0x3 0x9", EFI, 0,
   it_unknown},
  {"lv.avg.h", "rD,rA,rB", "00 0xA  DDDDD AAAAA BBBB B--- 0x3 0xA", EFI, 0,
   it_unknown},
  {"lv.cmp_eq.b", "rD,rA,rB", "00 0xA  DDDDD AAAAA BBBB B--- 0x4 0x0", EFI, 0,
   it_unknown},
  {"lv.cmp_eq.h", "rD,rA,rB", "00 0xA  DDDDD AAAAA BBBB B--- 0x4 0x1", EFI, 0,
   it_unknown},
  {"lv.cmp_ge.b", "rD,rA,rB", "00 0xA  DDDDD AAAAA BBBB B--- 0x4 0x2", EFI, 0,
   it_unknown},
  {"lv.cmp_ge.h", "rD,rA,rB", "00 0xA  DDDDD AAAAA BBBB B--- 0x4 0x3", EFI, 0,
   it_unknown},
  {"lv.cmp_gt.b", "rD,rA,rB", "00 0xA  DDDDD AAAAA BBBB B--- 0x4 0x4", EFI, 0,
   it_unknown},
  {"lv.cmp_gt.h", "rD,rA,rB", "00 0xA  DDDDD AAAAA BBBB B--- 0x4 0x5", EFI, 0,
   it_unknown},
  {"lv.cmp_le.b", "rD,rA,rB", "00 0xA  DDDDD AAAAA BBBB B--- 0x4 0x6", EFI, 0,
   it_unknown},
  {"lv.cmp_le.h", "rD,rA,rB", "00 0xA  DDDDD AAAAA BBBB B--- 0x4 0x7", EFI, 0,
   it_unknown},
  {"lv.cmp_lt.b", "rD,rA,rB", "00 0xA  DDDDD AAAAA BBBB B--- 0x4 0x8", EFI, 0,
   it_unknown},
  {"lv.cmp_lt.h", "rD,rA,rB", "00 0xA  DDDDD AAAAA BBBB B--- 0x4 0x9", EFI, 0,
   it_unknown},
  {"lv.cmp_ne.b", "rD,rA,rB", "00 0xA  DDDDD AAAAA BBBB B--- 0x4 0xA", EFI, 0,
   it_unknown},
  {"lv.cmp_ne.h", "rD,rA,rB", "00 0xA  DDDDD AAAAA BBBB B--- 0x4 0xB", EFI, 0,
   it_unknown},
  {"lv.madds.h", "rD,rA,rB", "00 0xA  DDDDD AAAAA BBBB B--- 0x5 0x4", EFI, 0,
   it_unknown},
  {"lv.max.b", "rD,rA,rB", "00 0xA  DDDDD AAAAA BBBB B--- 0x5 0x5", EFI, 0,
   it_unknown},
  {"lv.max.h", "rD,rA,rB", "00 0xA  DDDDD AAAAA BBBB B--- 0x5 0x6", EFI, 0,
   it_unknown},
  {"lv.merge.b", "rD,rA,rB", "00 0xA  DDDDD AAAAA BBBB B--- 0x5 0x7", EFI, 0,
   it_unknown},
  {"lv.merge.h", "rD,rA,rB", "00 0xA  DDDDD AAAAA BBBB B--- 0x5 0x8", EFI, 0,
   it_unknown},
  {"lv.min.b", "rD,rA,rB", "00 0xA  DDDDD AAAAA BBBB B--- 0x5 0x9", EFI, 0,
   it_unknown},
  {"lv.min.h", "rD,rA,rB", "00 0xA  DDDDD AAAAA BBBB B--- 0x5 0xA", EFI, 0,
   it_unknown},
  {"lv.msubs.h", "rD,rA,rB", "00 0xA  DDDDD AAAAA BBBB B--- 0x5 0xB", EFI, 0,
   it_unknown},
  {"lv.muls.h", "rD,rA,rB", "00 0xA  DDDDD AAAAA BBBB B--- 0x5 0xC", EFI, 0,
   it_unknown},
  {"lv.nand", "rD,rA,rB", "00 0xA  DDDDD AAAAA BBBB B--- 0x5 0xD", EFI, 0,
   it_unknown},
  {"lv.nor", "rD,rA,rB", "00 0xA  DDDDD AAAAA BBBB B--- 0x5 0xE", EFI, 0,
   it_unknown},
  {"lv.or", "rD,rA,rB", "00 0xA  DDDDD AAAAA BBBB B--- 0x5 0xF", EFI, 0,
   it_unknown},
  {"lv.pack.b", "rD,rA,rB", "00 0xA  DDDDD AAAAA BBBB B--- 0x6 0x0", EFI, 0,
   it_unknown},
  {"lv.pack.h", "rD,rA,rB", "00 0xA  DDDDD AAAAA BBBB B--- 0x6 0x1", EFI, 0,
   it_unknown},
  {"lv.packs.b", "rD,rA,rB", "00 0xA  DDDDD AAAAA BBBB B--- 0x6 0x2", EFI, 0,
   it_unknown},
  {"lv.packs.h", "rD,rA,rB", "00 0xA  DDDDD AAAAA BBBB B--- 0x6 0x3", EFI, 0,
   it_unknown},
  {"lv.packus.b", "rD,rA,rB", "00 0xA  DDDDD AAAAA BBBB B--- 0x6 0x4", EFI, 0,
   it_unknown},
  {"lv.packus.h", "rD,rA,rB", "00 0xA  DDDDD AAAAA BBBB B--- 0x6 0x5", EFI, 0,
   it_unknown},
  {"lv.perm.n", "rD,rA,rB", "00 0xA  DDDDD AAAAA BBBB B--- 0x6 0x6", EFI, 0,
   it_unknown},
  {"lv.rl.b", "rD,rA,rB", "00 0xA  DDDDD AAAAA BBBB B--- 0x6 0x7", EFI, 0,
   it_unknown},
  {"lv.rl.h", "rD,rA,rB", "00 0xA  DDDDD AAAAA BBBB B--- 0x6 0x8", EFI, 0,
   it_unknown},
  {"lv.sll.b", "rD,rA,rB", "00 0xA  DDDDD AAAAA BBBB B--- 0x6 0x9", EFI, 0,
   it_unknown},
  {"lv.sll.h", "rD,rA,rB", "00 0xA  DDDDD AAAAA BBBB B--- 0x6 0xA", EFI, 0,
   it_unknown},
  {"lv.sll", "rD,rA,rB", "00 0xA  DDDDD AAAAA BBBB B--- 0x6 0xB", EFI, 0,
   it_unknown},
  {"lv.srl.b", "rD,rA,rB", "00 0xA  DDDDD AAAAA BBBB B--- 0x6 0xC", EFI, 0,
   it_unknown},
  {"lv.srl.h", "rD,rA,rB", "00 0xA  DDDDD AAAAA BBBB B--- 0x6 0xD", EFI, 0,
   it_unknown},
  {"lv.sra.b", "rD,rA,rB", "00 0xA  DDDDD AAAAA BBBB B--- 0x6 0xE", EFI, 0,
   it_unknown},
  {"lv.sra.h", "rD,rA,rB", "00 0xA  DDDDD AAAAA BBBB B--- 0x6 0xF", EFI, 0,
   it_unknown},
  {"lv.srl", "rD,rA,rB", "00 0xA  DDDDD AAAAA BBBB B--- 0x7 0x0", EFI, 0,
   it_unknown},
  {"lv.sub.b", "rD,rA,rB", "00 0xA  DDDDD AAAAA BBBB B--- 0x7 0x1", EFI, 0,
   it_unknown},
  {"lv.sub.h", "rD,rA,rB", "00 0xA  DDDDD AAAAA BBBB B--- 0x7 0x2", EFI, 0,
   it_unknown},
  {"lv.subs.b", "rD,rA,rB", "00 0xA  DDDDD AAAAA BBBB B--- 0x7 0x3", EFI, 0,
   it_unknown},
  {"lv.subs.h", "rD,rA,rB", "00 0xA  DDDDD AAAAA BBBB B--- 0x7 0x4", EFI, 0,
   it_unknown},
  {"lv.subu.b", "rD,rA,rB", "00 0xA  DDDDD AAAAA BBBB B--- 0x7 0x5", EFI, 0,
   it_unknown},
  {"lv.subu.h", "rD,rA,rB", "00 0xA  DDDDD AAAAA BBBB B--- 0x7 0x6", EFI, 0,
   it_unknown},
  {"lv.subus.b", "rD,rA,rB", "00 0xA  DDDDD AAAAA BBBB B--- 0x7 0x7", EFI, 0,
   it_unknown},
  {"lv.subus.h", "rD,rA,rB", "00 0xA  DDDDD AAAAA BBBB B--- 0x7 0x8", EFI, 0,
   it_unknown},
  {"lv.unpack.b", "rD,rA,rB", "00 0xA  DDDDD AAAAA BBBB B--- 0x7 0x9", EFI, 0,
   it_unknown},
  {"lv.unpack.h", "rD,rA,rB", "00 0xA  DDDDD AAAAA BBBB B--- 0x7 0xA", EFI, 0,
   it_unknown},
  {"lv.xor", "rD,rA,rB", "00 0xA  DDDDD AAAAA BBBB B--- 0x7 0xB", EFI, 0,
   it_unknown},
  {"lv.cust1", "", "00 0xA  ----- ----- ---- ---- 0xC ----", EFI, 0,
   it_unknown},
  {"lv.cust2", "", "00 0xA  ----- ----- ---- ---- 0xD ----", EFI, 0,
   it_unknown},
  {"lv.cust3", "", "00 0xA  ----- ----- ---- ---- 0xE ----", EFI, 0,
   it_unknown},
  {"lv.cust4", "", "00 0xA  ----- ----- ---- ---- 0xF ----", EFI, 0,
   it_unknown},

  {"l.jr", "rB", "01 0x1  ----- ----- BBBB B--- ---- ----",
   EF (l_jr), OR32_IF_DELAY, it_jump},
  {"l.jalr", "rB", "01 0x2  ----- ----- BBBB B--- ---- ----",
   EF (l_jalr), OR32_IF_DELAY, it_jump},
  {"l.maci", "rA,I", "01 0x3  ----- AAAAA IIII IIII IIII IIII",
   EF (l_mac), 0, it_mac},
  {"l.lwa", "rD,I(rA)", "01 0xB  DDDDD AAAAA IIII IIII IIII IIII",
   EF (l_lwa), 0, it_load},
  {"l.cust1", "", "01 0xC  ----- ----- ---- ---- ---- ----",
   EF (l_cust1), 0, it_unknown},
  {"l.cust2", "", "01 0xD  ----- ----- ---- ---- ---- ----",
   EF (l_cust2), 0, it_unknown},
  {"l.cust3", "", "01 0xE  ----- ----- ---- ---- ---- ----",
   EF (l_cust3), 0, it_unknown},
  {"l.cust4", "", "01 0xF  ----- ----- ---- ---- ---- ----",
   EF (l_cust4), 0, it_unknown},

  {"l.ld", "rD,I(rA)", "10 0x0  DDDDD AAAAA IIII IIII IIII IIII", EFI,
   0, it_load},
  {"l.lwz", "rD,I(rA)", "10 0x1  DDDDD AAAAA IIII IIII IIII IIII",
   EF (l_lwz), 0, it_load},
  {"l.lws", "rD,I(rA)", "10 0x2  DDDDD AAAAA IIII IIII IIII IIII",
   EF (l_lws), 0, it_load},
  {"l.lbz", "rD,I(rA)", "10 0x3  DDDDD AAAAA IIII IIII IIII IIII",
   EF (l_lbz), 0, it_load},
  {"l.lbs", "rD,I(rA)", "10 0x4  DDDDD AAAAA IIII IIII IIII IIII",
   EF (l_lbs), 0, it_load},
  {"l.lhz", "rD,I(rA)", "10 0x5  DDDDD AAAAA IIII IIII IIII IIII",
   EF (l_lhz), 0, it_load},
  {"l.lhs", "rD,I(rA)", "10 0x6  DDDDD AAAAA IIII IIII IIII IIII",
   EF (l_lhs), 0, it_load},

  {"l.addi", "rD,rA,I", "10 0x7  DDDDD AAAAA IIII IIII IIII IIII",
   EF (l_add), OR32_W_FLAG, it_arith},
  {"l.addic", "rD,rA,I", "10 0x8  DDDDD AAAAA IIII IIII IIII IIII",
   EF (l_addc), OR32_W_FLAG, it_arith},
  {"l.andi", "rD,rA,K", "10 0x9  DDDDD AAAAA KKKK KKKK KKKK KKKK",
   EF (l_and), OR32_W_FLAG, it_arith},
  {"l.ori", "rD,rA,K", "10 0xA  DDDDD AAAAA KKKK KKKK KKKK KKKK",
   EF (l_or), 0, it_arith},
#if HAVE_UNSIGNED_XORI
  {"l.xori", "rD,rA,K", "10 0xB  DDDDD AAAAA KKKK KKKK KKKK KKKK",
   EF (l_xor), 0, it_arith},
#else
  {"l.xori", "rD,rA,I", "10 0xB  DDDDD AAAAA IIII IIII IIII IIII",
   EF (l_xor), 0, it_arith},
#endif
  {"l.muli", "rD,rA,I", "10 0xC  DDDDD AAAAA IIII IIII IIII IIII",
   EF (l_mul), 0, it_arith},
  {"l.mfspr", "rD,rA,K", "10 0xD  DDDDD AAAAA KKKK KKKK KKKK KKKK",
   EF (l_mfspr), 0, it_move},
  {"l.slli", "rD,rA,L", "10 0xE  DDDDD AAAAA ---- ---- 00LL LLLL",
   EF (l_sll), 0, it_shift},
  {"l.srli", "rD,rA,L", "10 0xE  DDDDD AAAAA ---- ---- 01LL LLLL",
   EF (l_srl), 0, it_shift},
  {"l.srai", "rD,rA,L", "10 0xE  DDDDD AAAAA ---- ---- 10LL LLLL",
   EF (l_sra), 0, it_shift},
  {"l.rori", "rD,rA,L", "10 0xE  DDDDD AAAAA ---- ---- 11LL LLLL",
   EF (l_ror), 0, it_shift},

  {"l.sfeqi", "rA,I", "10 0xF  00000 AAAAA IIII IIII IIII IIII",
   EF (l_sfeq), OR32_W_FLAG, it_compare},
  {"l.sfnei", "rA,I", "10 0xF  00001 AAAAA IIII IIII IIII IIII",
   EF (l_sfne), OR32_W_FLAG, it_compare},
  {"l.sfgtui", "rA,I", "10 0xF  00010 AAAAA IIII IIII IIII IIII",
   EF (l_sfgtu), OR32_W_FLAG, it_compare},
  {"l.sfgeui", "rA,I", "10 0xF  00011 AAAAA IIII IIII IIII IIII",
   EF (l_sfgeu), OR32_W_FLAG, it_compare},
  {"l.sfltui", "rA,I", "10 0xF  00100 AAAAA IIII IIII IIII IIII",
   EF (l_sfltu), OR32_W_FLAG, it_compare},
  {"l.sfleui", "rA,I", "10 0xF  00101 AAAAA IIII IIII IIII IIII",
   EF (l_sfleu), OR32_W_FLAG, it_compare},
  {"l.sfgtsi", "rA,I", "10 0xF  01010 AAAAA IIII IIII IIII IIII",
   EF (l_sfgts), OR32_W_FLAG, it_compare},
  {"l.sfgesi", "rA,I", "10 0xF  01011 AAAAA IIII IIII IIII IIII",
   EF (l_sfges), OR32_W_FLAG, it_compare},
  {"l.sfltsi", "rA,I", "10 0xF  01100 AAAAA IIII IIII IIII IIII",
   EF (l_sflts), OR32_W_FLAG, it_compare},
  {"l.sflesi", "rA,I", "10 0xF  01101 AAAAA IIII IIII IIII IIII",
   EF (l_sfles), OR32_W_FLAG, it_compare},

  {"l.mtspr", "rA,rB,K", "11 0x0  KKKKK AAAAA BBBB BKKK KKKK KKKK",
   EF (l_mtspr), 0, it_move},
  {"l.mac", "rA,rB", "11 0x1  ----- AAAAA BBBB B--- ---- 0x1",
   EF (l_mac), 0, it_mac},
  {"l.msb", "rA,rB", "11 0x1  ----- AAAAA BBBB B--- ---- 0x2",
   EF (l_msb), 0, it_mac},

  {"lf.add.s", "rD,rA,rB", "11 0x2  DDDDD AAAAA BBBB B--- 0x0 0x0",
   EF (lf_add_s), 0, it_float},
  {"lf.sub.s", "rD,rA,rB", "11 0x2  DDDDD AAAAA BBBB B--- 0x0 0x1",
   EF (lf_sub_s), 0, it_float},
  {"lf.mul.s", "rD,rA,rB", "11 0x2  DDDDD AAAAA BBBB B--- 0x0 0x2",
   EF (lf_mul_s), 0, it_float},
  {"lf.div.s", "rD,rA,rB", "11 0x2  DDDDD AAAAA BBBB B--- 0x0 0x3",
   EF (lf_div_s), 0, it_float},
  {"lf.itof.s", "rD,rA", "11 0x2  DDDDD AAAAA 0000 0--- 0x0 0x4",
   EF (lf_itof_s), 0, it_float},
  {"lf.ftoi.s", "rD,rA", "11 0x2  DDDDD AAAAA 0000 0--- 0x0 0x5",
   EF (lf_ftoi_s), 0, it_float},
  {"lf.rem.s", "rD,rA,rB", "11 0x2  DDDDD AAAAA BBBB B--- 0x0 0x6",
   EF (lf_rem_s), 0, it_float},
  {"lf.madd.s", "rD,rA,rB", "11 0x2  DDDDD AAAAA BBBB B--- 0x0 0x7",
   EF (lf_madd_s), 0, it_float},
  {"lf.sfeq.s", "rA,rB", "11 0x2  ----- AAAAA BBBB B--- 0x0 0x8",
   EF (lf_sfeq_s), 0, it_float},
  {"lf.sfne.s", "rA,rB", "11 0x2  ----- AAAAA BBBB B--- 0x0 0x9",
   EF (lf_sfne_s), 0, it_float},
  {"lf.sfgt.s", "rA,rB", "11 0x2  ----- AAAAA BBBB B--- 0x0 0xA",
   EF (lf_sfgt_s), 0, it_float},
  {"lf.sfge.s", "rA,rB", "11 0x2  ----- AAAAA BBBB B--- 0x0 0xB",
   EF (lf_sfge_s), 0, it_float},
  {"lf.sflt.s", "rA,rB", "11 0x2  ----- AAAAA BBBB B--- 0x0 0xC",
   EF (lf_sflt_s), 0, it_float},
  {"lf.sfle.s", "rA,rB", "11 0x2  ----- AAAAA BBBB B--- 0x0 0xD",
   EF (lf_sfle_s), 0, it_float},
  {"lf.cust1.s", "rA,rB", "11 0x2  ----- AAAAA BBBB B--- 0xD ----", EFI,
   0, it_float},

  {"lf.add.d", "rD,rA,rB", "11 0x2  DDDDD AAAAA BBBB B--- 0x1 0x0", EFI, 0,
   it_float},
  {"lf.sub.d", "rD,rA,rB", "11 0x2  DDDDD AAAAA BBBB B--- 0x1 0x1", EFI, 0,
   it_float},
  {"lf.mul.d", "rD,rA,rB", "11 0x2  DDDDD AAAAA BBBB B--- 0x1 0x2", EFI, 0,
   it_float},
  {"lf.div.d", "rD,rA,rB", "11 0x2  DDDDD AAAAA BBBB B--- 0x1 0x3", EFI, 0,
   it_float},
  {"lf.itof.d", "rD,rA", "11 0x2  DDDDD AAAAA 0000 0--- 0x1 0x4", EFI, 0,
   it_float},
  {"lf.ftoi.d", "rD,rA", "11 0x2  DDDDD AAAAA 0000 0--- 0x1 0x5", EFI, 0,
   it_float},
  {"lf.rem.d", "rD,rA,rB", "11 0x2  DDDDD AAAAA BBBB B--- 0x1 0x6", EFI, 0,
   it_float},
  {"lf.madd.d", "rD,rA,rB", "11 0x2  DDDDD AAAAA BBBB B--- 0x1 0x7", EFI, 0,
   it_float},
  {"lf.sfeq.d", "rA,rB", "11 0x2  ----- AAAAA BBBB B--- 0x1 0x8", EFI, 0,
   it_float},
  {"lf.sfne.d", "rA,rB", "11 0x2  ----- AAAAA BBBB B--- 0x1 0x9", EFI, 0,
   it_float},
  {"lf.sfgt.d", "rA,rB", "11 0x2  ----- AAAAA BBBB B--- 0x1 0xA", EFI, 0,
   it_float},
  {"lf.sfge.d", "rA,rB", "11 0x2  ----- AAAAA BBBB B--- 0x1 0xB", EFI, 0,
   it_float},
  {"lf.sflt.d", "rA,rB", "11 0x2  ----- AAAAA BBBB B--- 0x1 0xC", EFI, 0,
   it_float},
  {"lf.sfle.d", "rA,rB", "11 0x2  ----- AAAAA BBBB B--- 0x1 0xD", EFI, 0,
   it_float},
  {"lf.cust1.d", "rA,rB", "11 0x2  ----- AAAAA BBBB B--- 0xE ----", EFI, 0,
   it_float},

  {"l.swa", "I(rA),rB", "11 0x3  IIIII AAAAA BBBB BIII IIII IIII",
   EF (l_swa), 0, it_store},
  {"l.sd", "I(rA),rB", "11 0x4  IIIII AAAAA BBBB BIII IIII IIII", EFI,
   0, it_store},
  {"l.sw", "I(rA),rB", "11 0x5  IIIII AAAAA BBBB BIII IIII IIII",
   EF (l_sw), 0, it_store},
  {"l.sb", "I(rA),rB", "11 0x6  IIIII AAAAA BBBB BIII IIII IIII",
   EF (l_sb), 0, it_store},
  {"l.sh", "I(rA),rB", "11 0x7  IIIII AAAAA BBBB BIII IIII IIII",
   EF (l_sh), 0, it_store},

  {"l.add", "rD,rA,rB", "11 0x8  DDDDD AAAAA BBBB B-00 ---- 0x0",
   EF (l_add), OR32_W_FLAG, it_arith},
  {"l.addc", "rD,rA,rB", "11 0x8  DDDDD AAAAA BBBB B-00 ---- 0x1",
   EF (l_addc), OR32_W_FLAG, it_arith},
  {"l.sub", "rD,rA,rB", "11 0x8  DDDDD AAAAA BBBB B-00 ---- 0x2",
   EF (l_sub), 0, it_arith},
  {"l.and", "rD,rA,rB", "11 0x8  DDDDD AAAAA BBBB B-00 ---- 0x3",
   EF (l_and), OR32_W_FLAG, it_arith},
  {"l.or", "rD,rA,rB", "11 0x8  DDDDD AAAAA BBBB B-00 ---- 0x4",
   EF (l_or), 0, it_arith},
  {"l.xor", "rD,rA,rB", "11 0x8  DDDDD AAAAA BBBB B-00 ---- 0x5",
   EF (l_xor), 0, it_arith},
  {"l.mul", "rD,rA,rB", "11 0x8  DDDDD AAAAA BBBB B-11 ---- 0x6",
   EF (l_mul), 0, it_arith},

  {"l.sll", "rD,rA,rB", "11 0x8  DDDDD AAAAA BBBB B-00 00-- 0x8",
   EF (l_sll), 0, it_shift},
  {"l.srl", "rD,rA,rB", "11 0x8  DDDDD AAAAA BBBB B-00 01-- 0x8",
   EF (l_srl), 0, it_shift},
  {"l.sra", "rD,rA,rB", "11 0x8  DDDDD AAAAA BBBB B-00 10-- 0x8",
   EF (l_sra), 0, it_shift},
  {"l.ror", "rD,rA,rB", "11 0x8  DDDDD AAAAA BBBB B-00 11-- 0x8",
   EF (l_ror), 0, it_shift},
  {"l.div", "rD,rA,rB", "11 0x8  DDDDD AAAAA BBBB B-11 ---- 0x9",
   EF (l_div), 0, it_arith},
  {"l.divu", "rD,rA,rB", "11 0x8  DDDDD AAAAA BBBB B-11 ---- 0xA",
   EF (l_divu), 0, it_arith},
  {"l.mulu", "rD,rA,rB", "11 0x8  DDDDD AAAAA BBBB B-11 ---- 0xB",
   EF (l_mulu), 0, it_arith},
  {"l.extbs", "rD,rA", "11 0x8  DDDDD AAAAA ---- --00 01-- 0xC",
   EF (l_extbs), 0, it_move},
  {"l.exths", "rD,rA", "11 0x8  DDDDD AAAAA ---- --00 00-- 0xC",
   EF (l_exths), 0, it_move},
  {"l.extws", "rD,rA", "11 0x8  DDDDD AAAAA ---- --00 00-- 0xD",
   EF (l_extws), 0, it_move},
  {"l.extbz", "rD,rA", "11 0x8  DDDDD AAAAA ---- --00 11-- 0xC",
   EF (l_extbz), 0, it_move},
  {"l.exthz", "rD,rA", "11 0x8  DDDDD AAAAA ---- --00 10-- 0xC",
   EF (l_exthz), 0, it_move},
  {"l.extwz", "rD,rA", "11 0x8  DDDDD AAAAA ---- --00 01-- 0xD",
   EF (l_extwz), 0, it_move},
  {"l.cmov", "rD,rA,rB", "11 0x8  DDDDD AAAAA BBBB B-00 ---- 0xE",
   EF (l_cmov), OR32_R_FLAG, it_move},
  {"l.ff1", "rD,rA", "11 0x8  DDDDD AAAAA ---- --00 ---- 0xF",
   EF (l_ff1), 0, it_arith},
  {"l.fl1", "rD,rA", "11 0x8  DDDDD AAAAA ---- --01 ---- 0xF",
   EF (l_fl1), 0, it_arith},

  {"l.sfeq", "rA,rB", "11 0x9  00000 AAAAA BBBB B--- ---- ----",
   EF (l_sfeq), OR32_W_FLAG, it_compare},
  {"l.sfne", "rA,rB", "11 0x9  00001 AAAAA BBBB B--- ---- ----",
   EF (l_sfne), OR32_W_FLAG, it_compare},
  {"l.sfgtu", "rA,rB", "11 0x9  00010 AAAAA BBBB B--- ---- ----",
   EF (l_sfgtu), OR32_W_FLAG, it_compare},
  {"l.sfgeu", "rA,rB", "11 0x9  00011 AAAAA BBBB B--- ---- ----",
   EF (l_sfgeu), OR32_W_FLAG, it_compare},
  {"l.sfltu", "rA,rB", "11 0x9  00100 AAAAA BBBB B--- ---- ----",
   EF (l_sfltu), OR32_W_FLAG, it_compare},
  {"l.sfleu", "rA,rB", "11 0x9  00101 AAAAA BBBB B--- ---- ----",
   EF (l_sfleu), OR32_W_FLAG, it_compare},
  {"l.sfgts", "rA,rB", "11 0x9  01010 AAAAA BBBB B--- ---- ----",
   EF (l_sfgts), OR32_W_FLAG, it_compare},
  {"l.sfges", "rA,rB", "11 0x9  01011 AAAAA BBBB B--- ---- ----",
   EF (l_sfges), OR32_W_FLAG, it_compare},
  {"l.sflts", "rA,rB", "11 0x9  01100 AAAAA BBBB B--- ---- ----",
   EF (l_sflts), OR32_W_FLAG, it_compare},
  {"l.sfles", "rA,rB", "11 0x9  01101 AAAAA BBBB B--- ---- ----",
   EF (l_sfles), OR32_W_FLAG, it_compare},

  {"l.cust5", "rD,rA,rB,L,K", "11 0xC  DDDDD AAAAA BBBB BLLL LLLK KKKK", EFI,
   0, it_unknown},
  {"l.cust6", "", "11 0xD  ----- ----- ---- ---- ---- ----", EFI,
   0, it_unknown},
  {"l.cust7", "", "11 0xE  ----- ----- ---- ---- ---- ----", EFI,
   0, it_unknown},
  {"l.cust8", "", "11 0xF  ----- ----- ---- ---- ---- ----", EFI,
   0, it_unknown},

/* This section should not be defined in or1ksim, since it contains duplicates,
   which would cause machine builder to complain.  */
#ifdef HAS_CUST
  {"l.cust5_1", "rD", "11 0xC  DDDDD ----- ---- ---- ---- ----", EFI,
   0, it_unknown},
  {"l.cust5_2", "rD,rA", "11 0xC  DDDDD AAAAA ---- ---- ---- ----", EFI,
   0, it_unknown},
  {"l.cust5_3", "rD,rA,rB", "11 0xC  DDDDD AAAAA BBBB B--- ---- ----", EFI,
   0, it_unknown},

  {"l.cust6_1", "rD", "11 0xD  DDDDD ----- ---- ---- ---- ----", EFI,
   0, it_unknown},
  {"l.cust6_2", "rD,rA", "11 0xD  DDDDD AAAAA ---- ---- ---- ----", EFI,
   0, it_unknown},
  {"l.cust6_3", "rD,rA,rB", "11 0xD  DDDDD AAAAA BBBB B--- ---- ----", EFI,
   0, it_unknown},

  {"l.cust7_1", "rD", "11 0xE  DDDDD ----- ---- ---- ---- ----", EFI,
   0, it_unknown},
  {"l.cust7_2", "rD,rA", "11 0xE  DDDDD AAAAA ---- ---- ---- ----", EFI,
   0, it_unknown},
  {"l.cust7_3", "rD,rA,rB", "11 0xE  DDDDD AAAAA BBBB B--- ---- ----", EFI,
   0, it_unknown},

  {"l.cust8_1", "rD", "11 0xF  DDDDD ----- ---- ---- ---- ----", EFI,
   0, it_unknown},
  {"l.cust8_2", "rD,rA", "11 0xF  DDDDD AAAAA ---- ---- ---- ----", EFI,
   0, it_unknown},
  {"l.cust8_3", "rD,rA,rB", "11 0xF  DDDDD AAAAA BBBB B--- ---- ----", EFI,
   0, it_unknown},
#endif

  {"", "", "", EFI, 0, 0}	/* Dummy entry, not included in num_opcodes.  This
				 * lets code examine entry i+1 without checking
				 * if we've run off the end of the table.  */
};

#undef EFI
#undef EFN
#undef EF

static CONST int NUM_OPCODES =
  ((sizeof (or1ksim_or32_opcodes)) / (sizeof (struct or32_opcode))) - 1;

/* Calculates instruction length in bytes. Always 4 for OR32. */
int
or1ksim_insn_len (int insn_index)
{
  insn_index = 0;		/* Just to get rid that warning.  */
  return 4;
}

/* Is individual insn's operand signed or unsigned? */
static int
letter_signed (char l)
{
  CONST struct or32_letter *pletter;

  for (pletter = or32_letters; pletter->letter != '\0'; pletter++)
    if (pletter->letter == l)
      return pletter->sign;

  printf ("letter_signed(%c): Unknown letter.\n", l);
  return 0;
}

/* Simple cache for letter ranges */
static int range_cache[256] = { 0 };

/* Number of letters in the individual lettered operand. */
static int
letter_range (char l)
{
  CONST struct or32_opcode *pinsn;
  char *enc;
  int range = 0;

  /* Is value cached? */
  if ((range = range_cache[(unsigned char) l]))
    return range;

  for (pinsn = or1ksim_or32_opcodes; strlen (pinsn->name); pinsn++)
    {
      if (strchr (pinsn->encoding, l))
	{
	  for (enc = pinsn->encoding; *enc != '\0'; enc++)
	    if ((*enc == '0') && (*(enc + 1) == 'x'))
	      enc += 2;
	    else if (*enc == l)
	      range++;
	  return range_cache[(unsigned char) l] = range;
	}
    }

  printf ("\nABORT: letter_range(%c): Never used letter.\n", l);
  exit (1);
}

/* Returns name of the specified instruction index */
CONST char *
or1ksim_insn_name (int index)
{
  if (index >= 0 && index < NUM_OPCODES)
    return or1ksim_or32_opcodes[index].name;
  else
    return "???";
}

/*** Finite automata for instruction decoding building code ***/

/* Find symbols in encoding.  */
static unsigned long
insn_extract (param_ch, enc_initial)
     char param_ch;
     char *enc_initial;
{
  char *enc;
  unsigned long ret = 0;
  unsigned opc_pos = 32;
  for (enc = enc_initial; *enc != '\0';)
    if ((*enc == '0') && (*(enc + 1) == 'x'))
      {
	unsigned long tmp = strtol (enc + 2, NULL, 16);
	opc_pos -= 4;
	if (param_ch == '0' || param_ch == '1')
	  {
	    if (param_ch == '0')
	      tmp = 15 - tmp;
	    ret |= tmp << opc_pos;
	  }
	enc += 3;
      }
    else
      {
	if (*enc == '0' || *enc == '1' || *enc == '-' || isalpha ((int)*enc))
	  {
	    opc_pos--;
	    if (param_ch == *enc)
	      ret |= 1U << opc_pos;
	  }
	enc++;
      }
  return ret;
}

#define MAX_AUTOMATA_SIZE (1200)
#define MAX_OP_TABLE_SIZE (1200)
#define MAX_LEN           (8)

#ifndef MIN
# define MIN(x,y)          ((x) < (y) ? (x) : (y))
#endif

unsigned long *or1ksim_automata;
int nuncovered;
int curpass = 0;

/* MM: Struct that holds runtime build information about instructions.  */
struct temp_insn_struct *or1ksim_ti;

static struct insn_op_struct *op_data;

struct insn_op_struct **or1ksim_op_start;

static void
or32_debug (int level, const char *format, ...)
{
#if DEBUG
  char *p;
  va_list ap;

  if ((p = malloc (1000)) == NULL)
    return;
  va_start (ap, format);
  (void) vsnprintf (p, 1000, format, ap);
  va_end (ap);
  printf ("%s\n", p);
  fflush (stdout);
  free (p);
#endif
}

/* Recursive utility function used to find best match and to build automata.  */
static unsigned long *
cover_insn (unsigned long *cur, int pass, unsigned int mask)
{
  int best_first = 0, best_len = 0, i, last_match = -1, ninstr = 0;
  unsigned long cur_mask = mask;
  unsigned long *next;

  for (i = 0; i < NUM_OPCODES; i++)
    if (or1ksim_ti[i].in_pass == pass)
      {
	cur_mask &= or1ksim_ti[i].insn_mask;
	ninstr++;
	last_match = i;
      }

  or32_debug (8, "%08X %08lX\n", mask, cur_mask);
  if (ninstr == 0)
    return 0;
  if (ninstr == 1)
    {
      /* Leaf holds instruction index. */
      or32_debug (8, "%i>I%i %s\n", cur - or1ksim_automata, last_match,
	     or1ksim_or32_opcodes[last_match].name);
      *cur = LEAF_FLAG | last_match;
      cur++;
      nuncovered--;
    }
  else
    {
      /* Find longest match.  */
      for (i = 0; i < 32; i++)
	{
	  int len;
	  for (len = best_len + 1; len < MIN (MAX_LEN, 33 - i); len++)
	    {
	      unsigned long m = (1UL << ((unsigned long) len)) - 1;
	      or32_debug (9, " (%i(%08lX & %08lX>>%i = %08lX, %08lX)", len, m,
		     cur_mask, i, (cur_mask >> (unsigned) i),
		     (cur_mask >> (unsigned) i) & m);
	      if ((m & (cur_mask >> (unsigned) i)) == m)
		{
		  best_len = len;
		  best_first = i;
		  or32_debug (9, "!");
		}
	      else
		break;
	    }
	}
      or32_debug (9, "\n");
      if (!best_len)
	{
	  fprintf (stderr, "%i instructions match mask 0x%08X:\n", ninstr,
		   mask);
	  for (i = 0; i < NUM_OPCODES; i++)
	    if (or1ksim_ti[i].in_pass == pass)
	      fprintf (stderr, "%s ", or1ksim_or32_opcodes[i].name);

	  fprintf (stderr, "\n");
	  exit (1);
	}
      or32_debug (8, "%i> #### %i << %i (%i) ####\n", cur - or1ksim_automata, best_len,
	     best_first, ninstr);
      *cur = best_first;
      cur++;
      *cur = (1 << best_len) - 1;
      cur++;
      next = cur;
      /* Allocate space for pointers.  */
      cur += 1 << best_len;
      cur_mask = (1 << (unsigned long) best_len) - 1;

      for (i = 0; i < (1 << (unsigned long) best_len); i++)
	{
	  int j;
	  unsigned long *c;
	  curpass++;
	  for (j = 0; j < NUM_OPCODES; j++)
	    if (or1ksim_ti[j].in_pass == pass
		&& ((or1ksim_ti[j].insn >> best_first) & cur_mask) ==
		(unsigned long) i
		&& ((or1ksim_ti[j].insn_mask >> best_first) & cur_mask) == cur_mask)
	      or1ksim_ti[j].in_pass = curpass;

	  or32_debug (9, "%08X %08lX %i\n", mask, cur_mask, best_first);
	  c = cover_insn (cur, curpass, mask & (~(cur_mask << best_first)));
	  if (c)
	    {
	      or32_debug (8, "%i> #%X -> %u\n", next - or1ksim_automata, i,
		     cur - or1ksim_automata);
	      *next = cur - or1ksim_automata;
	      cur = c;
	    }
	  else
	    {
	      or32_debug (8, "%i> N/A\n", next - or1ksim_automata);
	      *next = 0;
	    }
	  next++;
	}
    }
  return cur;
}

/* Returns number of nonzero bits. */
static int
num_ones (unsigned long value)
{
  int c = 0;
  while (value)
    {
      if (value & 1)
	c++;
      value >>= 1;
    }
  return c;
}

/* Utility function, which converts parameters from or32_opcode format to more binary form.  
   Parameters are stored in or1ksim_ti struct.  */

static struct insn_op_struct *
parse_params (CONST struct or32_opcode *opcode, struct insn_op_struct *cur)
{
  char *args = opcode->args;
  int i, type;
  int num_cur_op = 0;;

  i = 0;
  type = 0;
  /* In case we don't have any parameters, we add dummy read from r0.  */
  if (!(*args))
    {
      cur->type = OPTYPE_REG | OPTYPE_OP | OPTYPE_LAST;
      cur->data = 0;
      or32_debug (9, "#%08lX %08lX\n", cur->type, cur->data);
      cur++;
      return cur;
    }

  while (*args != '\0')
    {
      if (*args == 'r')
	{
	  args++;
	  type |= OPTYPE_REG;
	  if (*args == 'D')
	    type |= OPTYPE_DST;
	}
      else if (isalpha ((int)*args))
	{
	  unsigned long arg;
	  arg = insn_extract (*args, opcode->encoding);
	  or32_debug (9, "%s : %08lX ------\n", opcode->name, arg);
	  if (letter_signed (*args))
	    {
	      type |= OPTYPE_SIG;
	      type |= ((num_ones (arg) - 1) << OPTYPE_SBIT_SHR) & OPTYPE_SBIT;
	    }

	  num_cur_op = 0;
	  /* Split argument to sequences of consecutive ones.  */
	  while (arg)
	    {
	      int shr = 0;
	      unsigned long tmp = arg, mask = 0;
	      while ((tmp & 1) == 0)
		{
		  shr++;
		  tmp >>= 1;
		}
	      while (tmp & 1)
		{
		  mask++;
		  tmp >>= 1;
		}
	      cur->type = type | shr;
	      cur->data = mask;
	      arg &= ~(((1 << mask) - 1) << shr);
	      or32_debug (6, "|%08lX %08lX\n", cur->type, cur->data);
	      cur++;
	      num_cur_op++;
	    }
	  args++;
	}
      else if (*args == '(')
	{
	  /* Next param is displacement.  Later we will treat them as one operand.  */
	  /* Set the OPTYPE_DIS flag on all insn_op_structs that belong to this
	   * operand */
	  while (num_cur_op > 0)
	    {
	      cur[-num_cur_op].type |= type | OPTYPE_DIS;
	      num_cur_op--;
	    }
	  cur[-1].type |= OPTYPE_OP;
	  or32_debug (9, ">%08lX %08lX\n", cur->type, cur->data);
	  type = 0;
	  i++;
	  args++;
	}
      else if (*args == OPERAND_DELIM)
	{
	  cur--;
	  cur->type = type | cur->type | OPTYPE_OP;
	  or32_debug (9, ">%08lX %08lX\n", cur->type, cur->data);
	  cur++;
	  type = 0;
	  i++;
	  args++;
	}
      else if (*args == '0')
	{
	  cur->type = type;
	  cur->data = 0;
	  or32_debug (9, ">%08lX %08lX\n", cur->type, cur->data);
	  cur++;
	  type = 0;
	  i++;
	  args++;
	}
      else if (*args == ')')
	args++;
      else
	{
	  fprintf (stderr, "%s : parse error in args.\n", opcode->name);
	  exit (1);
	}
    }
  cur--;
  cur->type = type | cur->type | OPTYPE_OP | OPTYPE_LAST;
  or32_debug (9, "#%08lX %08lX\n", cur->type, cur->data);
  cur++;
  return cur;
}

/* -------------------------------------------------------------------------- */
/*!Constructs new automata based on or1ksim_or32_opcodes array.

   @param[in] quiet  If non-zero (TRUE) do not print informational messages. */
/* -------------------------------------------------------------------------- */
void
or1ksim_build_automata (int  quiet)
{
  int i;
  unsigned long *end;
  struct insn_op_struct *cur;

  or1ksim_automata =
    (unsigned long *) malloc (MAX_AUTOMATA_SIZE * sizeof (unsigned long));
  or1ksim_ti =
    (struct temp_insn_struct *) malloc (sizeof (struct temp_insn_struct) *
					NUM_OPCODES);

  nuncovered = NUM_OPCODES;

#ifdef HAVE_EXECUTION
  if (!quiet)
    {
      printf ("Building automata... ");
    }
#endif

  /* Build temporary information about instructions.  */
  for (i = 0; i < NUM_OPCODES; i++)
    {
      unsigned long ones, zeros;
      char *encoding = or1ksim_or32_opcodes[i].encoding;
      ones = insn_extract ('1', encoding);
      zeros = insn_extract ('0', encoding);
      or1ksim_ti[i].insn_mask = ones | zeros;
      or1ksim_ti[i].insn = ones;
      or1ksim_ti[i].in_pass = curpass = 0;
      /*debug(9, "%s: %s %08X %08X\n", or1ksim_or32_opcodes[i].name,
         or1ksim_or32_opcodes[i].encoding, or1ksim_ti[i].insn_mask, or1ksim_ti[i].insn); */
    }

  /* Until all are covered search for best criteria to separate them.  */
  end = cover_insn (or1ksim_automata, curpass, 0xFFFFFFFF);
  if (end - or1ksim_automata > MAX_AUTOMATA_SIZE)
    {
      fprintf (stderr, "Automata too large. Increase MAX_AUTOMATA_SIZE.");
      exit (1);
    }
#ifdef HAVE_EXECUTION
  if (!quiet)
    {
      printf ("done, num uncovered: %i/%i.\n", nuncovered, NUM_OPCODES);
    }
#endif

#ifdef HAVE_EXECUTION
  if (!quiet)
    {
      printf ("Parsing operands data... ");
    }
#endif
  op_data =
    (struct insn_op_struct *) malloc (MAX_OP_TABLE_SIZE *
				      sizeof (struct insn_op_struct));
  or1ksim_op_start =
    (struct insn_op_struct **) malloc (NUM_OPCODES *
				       sizeof (struct insn_op_struct *));
  cur = op_data;
  for (i = 0; i < NUM_OPCODES; i++)
    {
      or1ksim_op_start[i] = cur;
      cur = parse_params (&or1ksim_or32_opcodes[i], cur);
      if (cur - op_data > MAX_OP_TABLE_SIZE)
	{
	  fprintf (stderr,
		   "Operands table too small, increase MAX_OP_TABLE_SIZE.\n");
	  exit (1);
	}
    }
#ifdef HAVE_EXECUTION
  if (!quiet)
    {
      printf ("done.\n");
    }
#endif
}

void
or1ksim_destruct_automata ()
{
  free (or1ksim_ti);
  free (or1ksim_automata);
  free (op_data);
  free (or1ksim_op_start);
}

/* Decodes instruction and returns instruction index.  */
int
or1ksim_insn_decode (unsigned int insn)
{
  unsigned long *a = or1ksim_automata;
  int i;
  while (!(*a & LEAF_FLAG))
    {
      unsigned int first = *a;
      //debug(9, "%i ", a - or1ksim_automata);
      a++;
      i = (insn >> first) & *a;
      a++;
      if (!*(a + i))
	{			/* Invalid instruction found?  */
	  //debug(9, "XXX\n", i);
	  return -1;
	}
      a = or1ksim_automata + *(a + i);
    }
  i = *a & ~LEAF_FLAG;
  //debug(9, "%i\n", i);
  /* Final check - do we have direct match?
     (based on or1ksim_or32_opcodes this should be the only possibility,
     but in case of invalid/missing instruction we must perform a check)  */
  if ((or1ksim_ti[i].insn_mask & insn) == or1ksim_ti[i].insn)
    return i;
  else
    return -1;
}

static char disassembled_str[50];
char *or1ksim_disassembled = &disassembled_str[0];

/* trace data */
int           trace_dest_reg;
int           trace_store_addr_reg;
unsigned int  trace_store_imm;
int           trace_store_val_reg;
int           trace_store_width;
int           trace_dest_spr;

/* Automagically does zero- or sign- extension and also finds correct
   sign bit position if sign extension is correct extension. Which extension
   is proper is figured out from letter description. */

unsigned long
or1ksim_extend_imm (unsigned long imm, char l)
{
  unsigned long mask;
  int letter_bits;

  /* First truncate all bits above valid range for this letter
     in case it is zero extend. */
  letter_bits = letter_range (l);
  mask = (1 << letter_bits) - 1;
  imm &= mask;

  /* Do sign extend if this is the right one. */
  if (letter_signed (l) && (imm >> (letter_bits - 1)))
    imm |= (~mask);

  return imm;
}

unsigned long
or1ksim_or32_extract (param_ch, enc_initial, insn)
     char param_ch;
     char *enc_initial;
     unsigned long insn;
{
  char *enc;
  unsigned long ret = 0;
  int opc_pos = 0;
  int param_pos = 0;

  for (enc = enc_initial; *enc != '\0'; enc++)
    if (*enc == param_ch)
      {
	if (enc - 2 >= enc_initial && (*(enc - 2) == '0')
	    && (*(enc - 1) == 'x'))
	  continue;
	else
	  param_pos++;
      }

#if DEBUG
  printf ("or1ksim_or32_extract: %x ", param_pos);
#endif
  opc_pos = 32;
  for (enc = enc_initial; *enc != '\0';)
    if ((*enc == '0') && (*(enc + 1) == 'x'))
      {
	opc_pos -= 4;
	if ((param_ch == '0') || (param_ch == '1'))
	  {
	    unsigned long tmp = strtol (enc, NULL, 16);
#if DEBUG
	    printf (" enc=%s, tmp=%x ", enc, tmp);
#endif
	    if (param_ch == '0')
	      tmp = 15 - tmp;
	    ret |= tmp << opc_pos;
	  }
	enc += 3;
      }
    else if ((*enc == '0') || (*enc == '1'))
      {
	opc_pos--;
	if (param_ch == *enc)
	  ret |= 1 << opc_pos;
	enc++;
      }
    else if (*enc == param_ch)
      {
	opc_pos--;
	param_pos--;
#if DEBUG
	printf ("\n  ret=%x opc_pos=%x, param_pos=%x\n", ret, opc_pos,
		param_pos);
#endif
	if (islower ((int) param_ch))
	  ret -= ((insn >> opc_pos) & 0x1) << param_pos;
	else
	  ret += ((insn >> opc_pos) & 0x1) << param_pos;
	enc++;
      }
    else if (isalpha ((int)*enc))
      {
	opc_pos--;
	enc++;
      }
    else if (*enc == '-')
      {
	opc_pos--;
	enc++;
      }
    else
      enc++;

#if DEBUG
  printf ("ret=%x\n", ret);
#endif
  return ret;
}

/* Print register. Used only by print_insn. */

static char *
or32_print_register (dest, param_ch, encoding, insn)
     char *dest;
     char param_ch;
     char *encoding;
     unsigned long insn;
{
  int regnum = or1ksim_or32_extract (param_ch, encoding, insn);

  sprintf (dest, "r%d", regnum);
  while (*dest)
    dest++;
  return dest;
}

/* Print immediate. Used only by print_insn. */

static char *
or32_print_immediate (dest, param_ch, encoding, insn)
     char *dest;
     char param_ch;
     char *encoding;
     unsigned long insn;
{
  int imm = or1ksim_or32_extract (param_ch, encoding, insn);

  imm = or1ksim_extend_imm (imm, param_ch);

  if (letter_signed (param_ch))
    {
      if (imm < 0)
	sprintf (dest, "%d", imm);
      else
	sprintf (dest, "0x%x", imm);
    }
  else
    sprintf (dest, "%#x", imm);
  while (*dest)
    dest++;
  return dest;
}

/* Disassemble one instruction from insn to disassemble.
   Return the size of the instruction.  */

int
or1ksim_disassemble_insn (insn)
     unsigned long insn;
{
  return or1ksim_disassemble_index (insn, or1ksim_insn_decode (insn));
}

/* Disassemble one instruction from insn index.
   Return the size of the instruction.  */

int
or1ksim_disassemble_index (insn, index)
     unsigned long insn;
     int index;
{
  char *dest = or1ksim_disassembled;
  if (index >= 0)
    {
      struct or32_opcode const *opcode = &or1ksim_or32_opcodes[index];
      char *s;

      strcpy (dest, opcode->name);
      while (*dest)
	dest++;
      *dest++ = ' ';
      *dest = 0;

      for (s = opcode->args; *s != '\0'; ++s)
	{
	  switch (*s)
	    {
	    case '\0':
	      return or1ksim_insn_len (insn);

	    case 'r':
	      dest = or32_print_register (dest, *++s, opcode->encoding, insn);
	      break;

	    default:
	      if (strchr (opcode->encoding, *s))
		dest =
		  or32_print_immediate (dest, *s, opcode->encoding, insn);
	      else
		{
		  *dest++ = *s;
		  *dest = 0;
		}
	    }
	}
    }
  else
    {
      /* This used to be %8x for binutils.  */
      sprintf (dest, ".word 0x%08lx", insn);
      while (*dest)
	dest++;
    }
  return or1ksim_insn_len (insn);
}


/* -------------------------------------------------------------------------- */
/*!Disassemble one instruction for tracing.

   A simplified form of  or1ksim_disassemble_index.

   @param[in] insn  The instruction to disassemble
   @param[in] index  Index into the opcode table.                             */
/* -------------------------------------------------------------------------- */
void
or1ksim_disassemble_trace_index (unsigned long int  insn,
				 int                index)
{
  int                i;
  char              *dest        = or1ksim_disassembled;

  /* Set trace result defaults. */
  trace_dest_reg       = -1;
  trace_dest_spr       = -1;
  trace_store_addr_reg = -1;
  trace_store_imm      =  0;
  trace_store_val_reg  = -1;
  trace_store_width    =  0;	/* Non-zero if we have a store */

  if (index >= 0)
    {
      const int  OPC_WIDTH = 8;

      struct or32_opcode const *opcode = &or1ksim_or32_opcodes[index];
      char                     *s;

      /* Is it a store opcode? */
      if (0 == strcmp ("l.sb", opcode->name))
	{
	  trace_store_width = 1;
	}
      else if (0 == strcmp ("l.sh", opcode->name))
	{
	  trace_store_width = 2;
	}
      else if (0 == strcmp ("l.sw", opcode->name))
	{
	  trace_store_width = 4;
	}

      /* Is it a move to SPR opcode? */
      if (0 == strcmp ("l.mtspr", opcode->name))
	{
	  trace_dest_spr = 1;
	}

      /* Copy the opcode and pad */
      strcpy (dest, opcode->name);

      for (i = 0; i < OPC_WIDTH; i++)
	{
	  if ('\0' == dest[0])
	    {
	      dest[0] = ' ';
	      dest[1] = '\0';
	    }

	  dest++;
	}

      for (s = opcode->args; *s != '\0'; ++s)
	{
	  switch (*s)
	    {
	    case '\0':
	      break;

	    case 'r':
	      dest = or32_print_register (dest, *++s, opcode->encoding, insn);

	      switch (*s)
		{
		case 'D':
		  trace_dest_reg =
		    or1ksim_or32_extract (*s, opcode->encoding, insn);
		  break;

		case 'A':
		  if (0 != trace_store_width)
		    {
		      trace_store_addr_reg =
			or1ksim_or32_extract (*s, opcode->encoding, insn);
		    }
		  break;

		case 'B':
		  if (0 != trace_store_width)
		    {
		      trace_store_val_reg =
			or1ksim_or32_extract (*s, opcode->encoding, insn);
		    }
		  break;
		  
		}

	      break;

	    default:
	      if (strchr (opcode->encoding, *s))
		{
		  dest = or32_print_immediate (dest, *s, opcode->encoding,
					       insn);

		  /* If we have a store instruction, save the immediate. */
		  if (0 != trace_store_width)
		    {
		      trace_store_imm = 
			or1ksim_or32_extract (*s, opcode->encoding, insn);
		      trace_store_imm =
			or1ksim_extend_imm (trace_store_imm, *s);
		    }

		  if (1 == trace_dest_spr)
		  {
			  trace_dest_spr = 
				  or1ksim_or32_extract ('K', opcode->encoding, 
							insn);
			  trace_dest_reg =
				  or1ksim_or32_extract ('A', opcode->encoding, 
							insn);
		  }
		  
		}
	      else
		{
		  *dest++ = *s;
		  *dest = 0;
		}
	    }
	}
    }
  else
    {
      /* This used to be %8x for binutils.  */
      sprintf (dest, ".word 0x%08lx", insn);
      while (*dest)
	dest++;
    }
}	/* or1ksim_disassemble_trace_index () */
