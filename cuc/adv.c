/* adv.c -- OpenRISC Custom Unit Compiler, Advanced Optimizations
 *    Copyright (C) 2002 Marko Mlinar, markom@opencores.org
 *
 *    This file is part of OpenRISC 1000 Architectural Simulator.
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA. */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>

#include "config.h"

#include "port.h"
#include "arch.h"
#include "sim-config.h"
#include "abstract.h"
#include "cuc.h"
#include "insn.h"
#include "support/profile.h"
#include "misc.h"

/* Marks successor of b with mask m */
static void mark_successors (cuc_func *f, int b, int m, int stopb)
{
  if (b < 0 || b == BBID_END) return;
  if (f->bb[b].tmp & m) return;
  f->bb[b].tmp |= m;
  /* mark stopb also; and stop searching -- we will gen new result in stopb */
  if (b == stopb) return;
  mark_successors (f, f->bb[b].next[0], m, stopb);
  mark_successors (f, f->bb[b].next[1], m, stopb);
}

static unsigned long mask (unsigned long c)
{
  if (c) return (1 << (log2_int (c) + 1)) - 1;
  else return 0;
}

/* Calculates facts, that are determined by conditionals */
void insert_conditional_facts (cuc_func *f)
{
  int b, j;
  int b1, i1, j1;
  cuc_insn n[2];
  for (b = 0; b < f->num_bb; b++) if (f->bb[b].ninsn > 0) {
    cuc_insn *ii = &f->bb[b].insn[f->bb[b].ninsn - 1];
    /* We have following situation
       x <= ...
       sfxx f, x, CONST
       bf ..., f */
    if (ii->type & IT_BRANCH && ii->opt[1] & OPT_REF && REF_BB(ii->op[1]) == b
     && f->INSN(ii->op[1]).opt[2] & OPT_CONST) {
      int ok = 0;
      unsigned long c = f->INSN(ii->op[1]).op[2];
      int rref = f->INSN(ii->op[1]).op[1];
      unsigned long r;
      if (!(f->INSN(ii->op[1]).opt[1] & OPT_REF)) continue;
      r = f->INSN(rref).op[0];

      /* Assignment must be in same basic block */
      if (REF_BB(rref) != b) continue;

      for (j = 0; j < 2; j++) {
        change_insn_type (&n[j], II_ADD);
        n[j].type = 0;
        n[j].dep = NULL;
        n[j].op[0] = r; n[j].opt[0] = OPT_REGISTER | OPT_DEST;
        n[j].op[1] = 0; n[j].opt[1] = OPT_CONST;
        n[j].op[2] = rref; n[j].opt[2] = OPT_REF;
        n[j].opt[3] = OPT_NONE;
        sprintf (n[j].disasm, "conditional %s fact", j ? "false" : "true");
      }
      
      /* First get the conditional and two instruction to place after the current BB */
      switch (f->INSN(ii->op[1]).index) {
        case II_SFEQ:
          change_insn_type (&n[0], II_ADD);
          n[0].op[0] = r; n[0].opt[0] = OPT_REGISTER | OPT_DEST;
          n[0].op[1] = 0; n[0].opt[1] = OPT_CONST;
          n[0].op[2] = c; n[0].opt[2] = OPT_CONST;
          ok = 1;
          break;
        case II_SFNE:
          change_insn_type (&n[1], II_ADD);
          n[1].op[0] = r; n[1].opt[0] = OPT_REGISTER | OPT_DEST;
          n[1].op[1] = 0; n[1].opt[1] = OPT_CONST;
          n[1].op[2] = c; n[1].opt[2] = OPT_CONST;
          ok = 2;
          break;
        case II_SFLT:
          change_insn_type (&n[0], II_AND);
          n[0].op[0] = r; n[0].opt[0] = OPT_REGISTER | OPT_DEST;
          n[0].op[1] = rref; n[0].opt[1] = OPT_REF;
          n[0].op[2] = mask (c); n[0].opt[2] = OPT_CONST;
          ok = 1;
          break;
        case II_SFGT:
          change_insn_type (&n[1], II_ADD);
          n[1].op[0] = r; n[1].opt[0] = OPT_REGISTER | OPT_DEST;
          n[1].op[1] = rref; n[1].opt[1] = OPT_REF;
          n[1].op[2] = mask (c + 1); n[1].opt[2] = OPT_CONST;
          ok = 2;
          break;
        case II_SFLE:
          change_insn_type (&n[0], II_AND);
          n[0].op[0] = r; n[0].opt[0] = OPT_REGISTER | OPT_DEST;
          n[0].op[1] = rref; n[0].opt[1] = OPT_REF;
          n[0].op[2] = mask (c); n[0].opt[2] = OPT_CONST;
          ok = 1;
          break;
        case II_SFGE:
          change_insn_type (&n[1], II_ADD);
          n[1].op[0] = r; n[1].opt[0] = OPT_REGISTER | OPT_DEST;
          n[1].op[1] = rref; n[1].opt[1] = OPT_REF;
          n[1].op[2] = mask (c + 1); n[1].opt[2] = OPT_CONST;
          ok = 2;
          break;
        default:
          ok = 0;
          break;
      }

      /* Now add two BBs at the end and relink */
      if (ok) {
        int cnt = 0;
        cucdebug (1, "%x rref %x cnt %i\n", b, rref, cnt);
        fflush (stdout);
        for (j = 0; j < 2; j++) {
          int nb = f->num_bb++;
          int sb;
          assert (nb < MAX_BB);
          f->bb[nb].type = 0;
          f->bb[nb].first = -1; f->bb[nb].last = -1;
          f->bb[nb].prev[0] = b; f->bb[nb].prev[1] = -1;
          sb = f->bb[nb].next[0] = f->bb[b].next[j]; f->bb[nb].next[1] = -1;
          assert (cnt >= 0);
          cucdebug (2, "%x %x %x rref %x cnt %i\n", b, sb, nb, rref, cnt);
          fflush (stdout);
          assert (sb >= 0);
          f->bb[b].next[j] = nb;
          if (sb != BBID_END) {
            if (f->bb[sb].prev[0] == b) f->bb[sb].prev[0] = nb;
            else if (f->bb[sb].prev[1] == b) f->bb[sb].prev[1] = nb;
            else assert (0);
          }
          f->bb[nb].insn = (cuc_insn *) malloc (sizeof (cuc_insn) * (cnt + 1));
          assert (f->bb[nb].insn);
          f->bb[nb].insn[0] = n[j];
          f->bb[nb].ninsn = cnt + 1;
          f->bb[nb].mdep = NULL;
          f->bb[nb].nmemory = 0;
          f->bb[nb].cnt = 0;
          f->bb[nb].unrolled = 0;
          f->bb[nb].ntim = 0;
          f->bb[nb].selected_tim = -1;
        }
        for (b1 = 0; b1 < f->num_bb; b1++) f->bb[b1].tmp = 0;
 
        /* Find successor blocks and change links accordingly */
        mark_successors (f, f->num_bb - 2, 2, b);
        mark_successors (f, f->num_bb - 1, 1, b);
        for (b1 = 0; b1 < f->num_bb - 2; b1++) if (f->bb[b1].tmp == 1 || f->bb[b1].tmp == 2) {
          int end;
          if (REF_BB (rref) == b1) end = REF_I (rref) + 1;
          else end = f->bb[b1].ninsn;
          for (i1 = 0; i1 < end; i1++)
            for (j1 = 0; j1 < MAX_OPERANDS; j1++)
              if (f->bb[b1].insn[i1].opt[j1] & OPT_REF && f->bb[b1].insn[i1].op[j1] == rref)
                f->bb[b1].insn[i1].op[j1] = REF (f->num_bb - f->bb[b1].tmp, 0);
        }
        if (cuc_debug >= 3) print_cuc_bb (f, "FACT");
      }
    }
  }
}

static unsigned long max_op (cuc_func *f, int ref, int o)
{
  if (f->INSN(ref).opt[o] & OPT_REF) return f->INSN(f->INSN(ref).op[o]).max;
  else if (f->INSN(ref).opt[o] & OPT_CONST) return f->INSN(ref).op[o];
  else if (f->INSN(ref).opt[o] & OPT_REGISTER) return 0xffffffff;
  else assert (0);
  return 0;
}

/* Returns maximum value, based on inputs */
static unsigned long calc_max (cuc_func *f, int ref)
{
  cuc_insn *ii = &f->INSN(ref);
  if (ii->type & IT_COND) return 1;
  switch (ii->index) {
    case II_ADD : return MIN ((unsigned long long) max_op (f, ref, 1)
                            + (unsigned long long)max_op (f, ref, 2), 0xffffffff);
    case II_SUB : return 0xffffffff;
    case II_AND : return MIN (max_op (f, ref, 1), max_op (f, ref, 2));
    case II_OR  : return max_op (f, ref, 1) | max_op (f, ref, 2);
    case II_XOR : return max_op (f, ref, 1) | max_op (f, ref, 2);
    case II_MUL : return MIN ((unsigned long long) max_op (f, ref, 1)
                            * (unsigned long long)max_op (f, ref, 2), 0xffffffff);
    case II_SLL : if (ii->opt[2] & OPT_CONST) return max_op (f, ref, 1) << ii->op[2];
                  else return max_op (f, ref, 1);
    case II_SRA : return max_op (f, ref, 1);
    case II_SRL : if (ii->opt[2] & OPT_CONST) return max_op (f, ref, 1) >> ii->op[2];
                  else return max_op (f, ref, 1);
    case II_LB  : return 0xff;
    case II_LH  : return 0xffff;
    case II_LW  : return 0xffffffff;
    case II_SB  :
    case II_SH  :
    case II_SW  : return 0;
    case II_SFEQ:
    case II_SFNE:
    case II_SFLE:
    case II_SFLT:
    case II_SFGE:
    case II_SFGT: return 1;
    case II_BF  : return 0;
    case II_LRBB: return 1;
    case II_CMOV: return MAX (max_op (f, ref, 1), max_op (f, ref, 2));
    case II_REG : return max_op (f, ref, 1);
    case II_NOP : assert (0);
    case II_CALL: assert (0);
    default:  assert (0);
  }
  return -1;
}

/* Width optimization -- detect maximum values; 
   these values are actually estimates, since the problem
   is to hard otherwise...
   We calculate these maximums iteratively -- we are slowly
   approaching final solution. This algorithm is surely finite,
   but can be very slow; so we stop after some iterations;
   normal loops should be in this range */
void detect_max_values (cuc_func *f)
{
  int b, i;
  int modified = 0;
  int iteration = 0;

  for (b = 0; b < f->num_bb; b++) {
    for (i = 0; i < f->bb[b].ninsn; i++) f->bb[b].insn[i].max = 0;
    f->bb[b].tmp = 1;
  }

  /* Repeat until something is changing */
  do {
    modified = 0;
    for (b = 0; b < f->num_bb; b++) {
      if (f->bb[b].tmp) {
        for (i = 0; i < f->bb[b].ninsn; i++) {
          unsigned long m = calc_max (f, REF (b, i));
          if (m > f->bb[b].insn[i].max) {
            f->bb[b].insn[i].max = m;
            modified = 1;
          }
        }
      }
    }
    if (iteration++ > CUC_WIDTH_ITERATIONS) break;
  } while (modified);

  /* Something bad has happened; now we will assign 0xffffffff to all unsatisfied
     instructions; this one is stoppable in O(n ^ 2) */
  if (iteration > CUC_WIDTH_ITERATIONS) {
    do {
      modified = 0;
      for (b = 0; b < f->num_bb; b++)
        for (i = 0; i < f->bb[b].ninsn; i++) {
          unsigned long m = calc_max (f, REF (b, i));
          if (m > f->bb[b].insn[i].max) {
            f->bb[b].insn[i].max = 0xffffffff;
            modified = 1;
          }
        }
    } while (modified);
  }
  cucdebug (1, "detect_max_values %i iterations\n", iteration);
}

