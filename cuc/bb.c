/* bb.c -- OpenRISC Custom Unit Compiler, Basic Block handling
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

/* prints out bb string */
void print_bb_num (int num)
{
  if (num < 0) PRINTF ("*");
  else if (num == BBID_END) PRINTF ("END");
  else if (num == BBID_START) PRINTF ("START");
  else PRINTF ("%2x", num);
}

/* Print out basic blocks */
void print_cuc_bb (cuc_func *f, char *s)
{
  int i;
  PRINTF ("------- %s -------\n", s);
  for (i = 0; i < f->num_bb; i++) {
    if (f->bb[i].insn) PRINTF ("\n---- BB%-2x * %x ---- ", i, f->bb[i].cnt);
    else PRINTF ("BB%-2x: %4x-%-4x", i, f->bb[i].first, f->bb[i].last);
    PRINTF (" type %02lx tmp %i ", f->bb[i].type, f->bb[i].tmp);
    PRINTF ("next "); print_bb_num (f->bb[i].next[0]);
    PRINTF (" "); print_bb_num (f->bb[i].next[1]);
    PRINTF (" prev "); print_bb_num (f->bb[i].prev[0]);
    PRINTF (" "); print_bb_num (f->bb[i].prev[1]);
    PRINTF ("\n");
    
    if (f->bb[i].insn) print_insns (i, f->bb[i].insn, f->bb[i].ninsn, 0);
  }
  if (f->nmsched) {
    PRINTF ("\nmsched: ");
    for (i = 0; i < f->nmsched; i++)
      PRINTF ("%x ", f->msched[i]);
    PRINTF ("\n\n\n");
  } else PRINTF ("\n");
  fflush (stdout);
}

/* Copies src basic block into destination */
void cpy_bb (cuc_bb *dest, cuc_bb *src)
{
  int i, j;
  dep_list *d;
  assert (dest != src);
  *dest = *src;
  assert (dest->insn = malloc (sizeof (cuc_insn) * src->ninsn));
  for (i = 0; i < src->ninsn; i++) {
    d = src->insn[i].dep;
    dest->insn[i] = src->insn[i];
    dest->insn[i].dep = NULL;
    while (d) {
      add_dep (&dest->insn[i].dep, d->ref);
      d = d->next;
    }
  }
  
  d = src->mdep;
  dest->mdep = NULL;
  while (d) {
    add_dep (&dest->mdep, d->ref);
    d = d->next;
  }
  if (src->ntim) {
    assert (dest->tim = malloc (sizeof (cuc_timings) * src->ntim));
    for (i = 0; i < src->ntim; i++) {
      dest->tim[i] = src->tim[i];
      if (src->tim[i].nshared) {
        assert (dest->tim[i].shared = malloc (sizeof (int) * src->tim[i].nshared));
        for (j = 0; j < src->tim[i].nshared; j++)
          dest->tim[i].shared[j] = src->tim[i].shared[j];
      }
    }
  }
}

/* Duplicates function */
cuc_func *dup_func (cuc_func *f)
{
  cuc_func *n = (cuc_func *) malloc (sizeof (cuc_func));
  int b, i;
  for (b = 0; b < f->num_bb; b++) cpy_bb (&n->bb[b], &f->bb[b]);
  n->num_bb = f->num_bb;
  assert (n->init_bb_reloc = (int *)malloc (sizeof (int) * f->num_init_bb));
  for (b = 0; b < f->num_init_bb; b++) n->init_bb_reloc[b] = f->init_bb_reloc[b];
  n->num_init_bb = f->num_init_bb;
  for (i = 0; i < MAX_REGS; i++) {
    n->saved_regs[i] = f->saved_regs[i];
    n->lur[i] = f->lur[i];
    n->used_regs[i] = f->used_regs[i];
  }
  n->start_addr = f->start_addr;
  n->end_addr = f->end_addr;
  n->orig_time = f->orig_time;
  n->nmsched = f->nmsched;
  n->num_runs = f->num_runs;
  for (i = 0; i < f->nmsched; i++) {
    n->msched[i] = f->msched[i];
    n->mtype[i] = f->mtype[i];
  }
  n->nfdeps = f->nfdeps;
  if (f->nfdeps) {
    f->fdeps = (cuc_func **) malloc (sizeof (cuc_func *) * f->nfdeps);
    for (i = 0; i < f->nfdeps; i++) n->fdeps[i] = f->fdeps[i];
  }
  return n;
}

/* Releases memory allocated by function */
void free_func (cuc_func *f)
{
  int b, i;
  for (b = 0; b < f->num_bb; b++) {
    for (i = 0; i < f->bb[b].ninsn; i++)
      dispose_list (&f->bb[b].insn[i].dep);
    if (f->bb[b].insn) free (f->bb[b].insn);
    for (i = 0; i < f->bb[b].ntim; i++)
      if (f->bb[b].tim[i].nshared && f->bb[b].tim[i].shared)
        free (f->bb[b].tim[i].shared);
    if (f->bb[b].tim && f->bb[b].ntim) free (f->bb[b].tim);
  }
  free (f);
}

/* Recalculates last_used_reg */
void recalc_last_used_reg (cuc_func *f, int b)
{
  int i;
  cuc_bb *bb = &f->bb[b];
  
  /* rebuild last used reg array */
  if (bb->insn[0].index == II_LRBB) bb->last_used_reg[LRBB_REG] = 0;
  else bb->last_used_reg[LRBB_REG] = -1;

  for (i = 1; i < MAX_REGS - 1; i++) bb->last_used_reg[i] = -1;
    
    /* Create references */
  for (i = 0; i < bb->ninsn; i++) {
    int k;
    /* Now check for destination operand(s) */
    for (k = 0; k < MAX_OPERANDS; k++) if (bb->insn[i].opt[k] & OPT_DEST)
      if ((bb->insn[i].opt[k] & ~OPT_DEST) == OPT_REGISTER
        && (int)bb->insn[i].op[k] >= 0) {
        bb->last_used_reg[bb->insn[i].op[k]] = REF (b, i);
      }
  }
}

/* Set the BB limits */
void detect_bb (cuc_func *f)
{
  int i, j, end_bb = 0, eb = 0;

  /* Mark block starts/ends */
  for (i = 0; i < num_insn; i++)
    {
      insn[i].type |= end_bb ? IT_BBSTART : 0;

      end_bb = 0;

      if (insn[i].type & IT_BRANCH)
	{
	  int jt = insn[i].op[0];
	  insn[i].type |= IT_BBEND;
	  end_bb = 1;

	  if (jt < 0 || jt >= num_insn)
	    {
	      fprintf (stderr, "Instruction #%i:Jump out of function '%s'.\n",
		       i, insn[i].disasm);
	      exit (1);
	    }

	  if (jt > 0)
	    {
	      insn[jt - 1].type |= IT_BBEND;
	    }

	  insn[jt].type     |= IT_BBSTART;
	}
    }

  /* Initialize bb array */
  insn[0].type |= IT_BBSTART;
  if (num_insn > 0)
    {
      insn[num_insn - 1].type |= IT_BBEND;
    }
  f->num_bb = 0;
  for (i = 0; i < num_insn; i++) {
    if (insn[i].type & IT_BBSTART) {
      f->bb[f->num_bb].first = i;
      f->bb[f->num_bb].cnt = 0;
    }
    /* Determine repetitions of a loop */
    if (insn[i].type & IT_BBEND) {
      f->bb[f->num_bb].type = 0;
      f->bb[f->num_bb].last = i;
      f->bb[f->num_bb].next[0] = f->bb[f->num_bb].next[1] = -1;
      f->bb[f->num_bb].tmp = 0;
      f->bb[f->num_bb].ntim = 0;
      f->num_bb++;
      assert (f->num_bb < MAX_BB);
    }
  }
  if (cuc_debug >= 3) print_cuc_bb (f, "AFTER_INIT");

  /* Build forward connections between BBs */
  for (i = 0; i < f->num_bb; i++)
    if (insn[f->bb[i].last].type & IT_BRANCH) {
      int j;
      assert (insn[f->bb[i].last].index == II_BF);
      /* Find block this instruction jumps to */
      for (j = 0; j < f->num_bb; j++)
        if (f->bb[j].first == insn[f->bb[i].last].op[0]) break;
      assert (j < f->num_bb);
      
      /* Convert the jump address to BB link */
      insn[f->bb[i].last].op[0] = j; insn[f->bb[i].last].opt[0] = OPT_BB;

      /* Make a link */
      f->bb[i].next[0] = j;
      if (++f->bb[j].tmp > 2) eb++;
      f->bb[i].next[1] = i + 1;
      if (++f->bb[i + 1].tmp > 2) eb++;
    } else if (f->bb[i].last == num_insn - 1) { /* Last instruction doesn't have to do anything */
    } else {
      f->bb[i].next[0] = i + 1;
      if (++f->bb[i + 1].tmp > 2) eb++;
    }
  
  if (cuc_debug >= 3) print_cuc_bb (f, "AFTER_NEXT");

  /* Build backward connections, but first insert artificial blocks
   * to handle more than 2 connections */
  cucdebug (6, "artificial %i %i\n", f->num_bb, eb);
  end_bb = f->num_bb + eb;
  for (i = f->num_bb - 1; i >= 0; i--) {
    j = f->bb[i].tmp;
    if (f->bb[i].tmp > 2) f->bb[i].tmp = -f->bb[i].tmp;
    f->bb[--end_bb] = f->bb[i];
    reloc[i] = end_bb;
    while (j-- > 2) {
      f->bb[--end_bb].first = f->bb[i].first;
      f->bb[end_bb].last = -1;
      f->bb[end_bb].next[0] = -1;
      f->bb[end_bb].next[1] = -1;
      f->bb[end_bb].tmp = 0;
      f->bb[end_bb].cnt = f->bb[i].cnt;
      f->bb[end_bb].ntim = 0;
    }
  }
  f->num_bb += eb;

  /* relocate jump instructions */
  for (i = 0; i < num_insn; i++)
    for (j = 0; j < MAX_OPERANDS; j++)
      if (insn[i].opt[j] & OPT_BB)
        insn[i].op[j] = reloc[insn[i].op[j]];
  if (cuc_debug >= 3) print_cuc_bb (f, "AFTER_INSERT-reloc");
  for (i = 0; i < f->num_bb; i++) {
    if (f->bb[i].next[0] >= 0) {
      int t = reloc[f->bb[i].next[0]];
      if (f->bb[t].tmp < 0) {
        f->bb[t].tmp = -f->bb[t].tmp;
        t -= f->bb[t].tmp - 2;
      } else if (f->bb[t].tmp > 2) t -= f->bb[t].tmp-- - 2;
      f->bb[i].next[0] = t;
    }
    if (f->bb[i].next[1] >= 0) {
      int t = reloc[f->bb[i].next[1]];
      if (f->bb[t].tmp < 0) {
        f->bb[t].tmp = -f->bb[t].tmp;
        t -= f->bb[t].tmp - 2;
      } else if (f->bb[t].tmp > 2) t -= f->bb[t].tmp-- - 2;
      f->bb[i].next[1] = t;
    }
    /* artificial blocks do not have relocations, hardcode them */
    if (f->bb[i].last < 0) f->bb[i].next[0] = i + 1;    
  }
  if (cuc_debug >= 3) print_cuc_bb (f, "AFTER_INSERT");

  /* Uncoditional branched do not continue to next block */
  for (i = 0; i < f->num_bb; i++) {
    cuc_insn *ii;
    if (f->bb[i].last < 0) continue;
    ii = &insn[f->bb[i].last];
    /* Unconditional branch? */
    if (ii->type & IT_BRANCH && ii->opt[1] & OPT_CONST) {
      change_insn_type (ii, II_NOP);
#if 0
      if (f->bb[i].next[1] == i + 1) f->bb[i].next[0] = f->bb[i].next[1];
#endif
      f->bb[i].next[1] = -1;
    }
  }
  if (cuc_debug >= 3) print_cuc_bb (f, "AFTER_UNCOND_JUMP");

  /* Add backward connections */
  for (i = 0; i < f->num_bb; i++)
    f->bb[i].prev[0] = f->bb[i].prev[1] = -1;

  for (i = 0; i < f->num_bb; i++) {
    if (f->bb[i].next[0] >= 0) {
      int t = f->bb[i].next[0];
      if (f->bb[t].prev[0] < 0) f->bb[t].prev[0] = i;
      else {
        assert (f->bb[t].prev[1] < 0);
        f->bb[t].prev[1] = i;
      }
    }
    if (f->bb[i].next[1] >= 0) {
      int t = f->bb[i].next[1];
      if (f->bb[t].prev[0] < 0) f->bb[t].prev[0] = i;
      else {
        assert (f->bb[t].prev[1] < 0);
        f->bb[t].prev[1] = i;
      }
    }
  }
  /* Add START marker */
  assert (f->bb[0].prev[0] < 0);
  f->bb[0].prev[0] = BBID_START;

  /* Add END marker */
  assert (f->bb[f->num_bb - 1].next[0] < 0);
  assert (f->bb[f->num_bb - 1].next[1] < 0);
  f->bb[f->num_bb - 1].next[0] = BBID_END;
  if (cuc_debug >= 3) print_cuc_bb (f, "AFTER_PREV");
}

/* We do a quick check if there are some anomalies with references */
void cuc_check (cuc_func *f)
{
  int i, j = 0, k = 0;
  cucdebug (1, "cuc_check\n");
  for (i = 0; i < f->num_bb; i++) {
    if (!f->bb[i].insn && f->bb[i].ninsn) goto err;
    for (j = 0; j < f->bb[i].ninsn; j++) {
      cuc_insn *ii = &f->bb[i].insn[j];
      if ((ii->index == II_CMOV || ii->index == II_ADD) && ii->type & IT_COND && ii->opt[0] & OPT_DEST) {
        k = 0;
        assert (ii->opt[k] & OPT_REGISTER);
        if ((signed)ii->op[k] >= 0 && ii->op[k] != FLAG_REG && ii->op[k] != LRBB_REG) {
          cucdebug (1, "Invalid dest conditional type opt%x op%lx\n", ii->opt[0], ii->op[0]);
          goto err;
        }
      }
      for (k = 0; k < MAX_OPERANDS; k++) {
        if (ii->opt[k] & OPT_REF) {
          int t = ii->op[k];
          if (REF_BB(t) >= f->num_bb || REF_I (t) >= f->bb[REF_BB(t)].ninsn ||
              ((ii->index == II_CMOV || ii->index == II_ADD) &&
               (((f->INSN(t).type & IT_COND) != (ii->type & IT_COND) && k < 3) ||
               ((!(f->INSN(t).type & IT_COND) && k == 3))))) {
            cucdebug (1, "Conditional misused\n");
            goto err;
          }
        }
        if (k && ii->opt[k] & OPT_DEST) {
          cucdebug (1, "Destination only allowed for op0!\n");
          goto err;
        }
      }
    }
  }
  return;
err:
  cucdebug (1, "Anomaly detected at [%x_%x].%i\n", i, j, k);
  print_cuc_bb (f, "ANOMALY");
  cucdebug (1, "Anomaly detected at [%x_%x].%i\n", i, j, k);
  exit (1);
}

/* Build basic blocks */
void build_bb (cuc_func *f)
{
  int i, j, k;
  for (i = 0; i < f->num_bb; i++) {
    if (f->bb[i].last < 0) f->bb[i].ninsn = MAX_REGS - 1;
    else f->bb[i].ninsn = f->bb[i].last - f->bb[i].first + 1 + MAX_REGS - 1;
    assert (f->bb[i].ninsn >= MAX_REGS - 1);
    f->bb[i].insn = (cuc_insn *) malloc (sizeof (cuc_insn) * f->bb[i].ninsn);
    assert (f->bb[i].insn);
    f->bb[i].nmemory = 0;
    f->bb[i].unrolled = 1;

    /* Save space for conditional moves, exclude r0, place lrbb instead */
    change_insn_type (&f->bb[i].insn[0], II_LRBB);
    strcpy (f->bb[i].insn[0].disasm, "lrbb");
    f->bb[i].insn[0].type = IT_UNUSED | IT_COND;
    f->bb[i].insn[0].dep = NULL;
    f->bb[i].insn[0].op[0] = LRBB_REG; f->bb[i].insn[0].opt[0] = OPT_REGISTER | OPT_DEST;
    f->bb[i].insn[0].opt[1] = OPT_LRBB;
    f->bb[i].insn[0].opt[2] = f->bb[i].insn[0].opt[3] = OPT_NONE;
    for (j = 1; j < MAX_REGS - 1; j++) {
      change_insn_type (&f->bb[i].insn[j], II_CMOV);
      strcpy (f->bb[i].insn[j].disasm, "cmov");
      f->bb[i].insn[j].type = j == FLAG_REG || j == LRBB_REG ? IT_COND : 0;
      f->bb[i].insn[j].dep = NULL;
      f->bb[i].insn[j].opt[0] = f->bb[i].insn[j].opt[1] = f->bb[i].insn[j].opt[2] = OPT_REGISTER;
      f->bb[i].insn[j].opt[0] |= OPT_DEST;
      f->bb[i].insn[j].op[0] = f->bb[i].insn[j].op[1] = f->bb[i].insn[j].op[2] = j;
      f->bb[i].insn[j].op[3] = LRBB_REG; f->bb[i].insn[j].opt[3] = OPT_REGISTER;
    }
    
    /* Relocate instructions */
    for (j = MAX_REGS - 1; j < f->bb[i].ninsn; j++) {
      f->bb[i].insn[j] = insn[f->bb[i].first + j - (MAX_REGS - 1)];
      for (k = 0; k < MAX_OPERANDS; k++)
        if (f->bb[i].insn[j].opt[k] & OPT_REF) {
          int b1;
          for (b1 = 0; b1 < i; b1++)
            if (f->bb[b1].first <= (signed) f->bb[i].insn[j].op[k]
              && (signed)f->bb[i].insn[j].op[k] <= f->bb[b1].last) break;
          assert (b1 < f->num_bb);
          f->bb[i].insn[j].op[k] = REF (b1, f->bb[i].insn[j].op[k] - f->bb[b1].first + MAX_REGS - 1);
        }
      if (f->bb[i].insn[j].type & IT_MEMORY) f->bb[i].nmemory++;
    }
  }
  cuc_check (f);
}

/* Does simplification on blocks A, B, C:
   A->B->C, A->C to just A->B->C */
static void simplify_bb (cuc_func *f, int pred, int s1, int s2, int neg)
{
  cuc_insn *last;
  int i;
  if (cuc_debug >= 3) print_cuc_bb (f, "BEFORE_SIMPLIFY");
  cucdebug (3, "simplify %x->%x->%x (%i)\n", pred, s1, s2, neg);
  assert (s2 != pred); /* Shouldn't occur => stupid */
  f->bb[pred].next[1] = -1;
  f->bb[pred].next[0] = s1;

  if (f->bb[s2].prev[0] == pred) {
    f->bb[s2].prev[0] = f->bb[s2].prev[1];
    f->bb[s2].prev[1] = -1;
  } else if (f->bb[s2].prev[1] == pred) {
    f->bb[s2].prev[1] = -1;
  } else assert (0);
  
  last = &f->bb[pred].insn[f->bb[pred].ninsn - 1];
  assert (last->type & IT_BRANCH);
  for (i = 0; i < f->bb[s2].ninsn; i++) {
    cuc_insn *ii= &f->bb[s2].insn[i];
    if (ii->index == II_LRBB) {
      change_insn_type (ii, II_CMOV);
      ii->type = IT_COND;
      ii->op[1] = neg ? 0 : 1; ii->opt[1] = OPT_CONST;
      ii->op[2] = neg ? 1 : 0; ii->opt[2] = OPT_CONST;
      ii->op[3] = last->op[1]; ii->opt[3] = last->opt[1];
    }
  }
  change_insn_type (last, II_NOP);
  if (cuc_debug >= 3) print_cuc_bb (f, "AFTER_SIMPLIFY");
}

/* type == 0; keep predecessor condition
 * type == 1; keep successor condition
 * type == 2; join loop unrolled blocks */
static void join_bb (cuc_func *f, int pred, int succ, int type)
{
  int i, j, k, n1, n2, ninsn, add_cond = 0;
  unsigned long cond_op = 0, cond_opt = 0;
  cuc_insn *insn;

  if (cuc_debug) cuc_check (f);
  cucdebug (3, "%x <= %x+%x (%i)\n", pred, pred, succ, type);
  cucdebug (3, "%x %x\n", f->bb[pred].ninsn, f->bb[succ].ninsn);
  if (cuc_debug >= 3) fflush (stdout);

  n1 = f->bb[pred].ninsn;
  n2 = f->bb[succ].ninsn;
  if (n1 <= 0
   || !(f->bb[pred].insn[n1 - 1].type & IT_BRANCH)) type = 1;
  if (type == 0 && f->bb[succ].prev[0] == f->bb[succ].next[0]) add_cond = 1;
  if (type == 2) add_cond = 1;

  //assert (f->bb[pred].next[0] == f->bb[succ].next[0] || type != 2); /* not supported */
  
  ninsn = n1 + n2 + (type == 1 ? 0 : 1) + (add_cond ? MAX_REGS : 0);

  insn = (cuc_insn *) malloc (ninsn * sizeof (cuc_insn));
  for (i = 0; i < n1; i++) insn[i] = f->bb[pred].insn[i];
  /* when type == 0, we move the last (jump) instruction to the end */
  if (type == 0 || type == 2) {
    /* Move first branch instruction to the end */
    assert (insn[n1 - 1].type & IT_BRANCH);
    insn[ninsn - 1] = insn[n1 - 1];
    cond_op = insn[n1 - 1].op[1];
    cond_opt = insn[n1 - 1].opt[1];
    
    /* Remove old branch */
    change_insn_type (&insn[n1 - 1], II_NOP);
  }
  /* Copy second block */
  for (i = 0; i < n2; i++) insn[i + n1] = f->bb[succ].insn[i];

  /* and when type == 2, we may need to add sfor instruction, to quit when either is true */
  if (type == 2) {
    /* Move second branch instruction to the end */
    if (insn[n1 + n2 - 1].type & IT_BRANCH) {
      insn[ninsn - 1] = insn[n1 + n2 - 1];

      /* Use conditional from cmov FLAG_REG, c_p, c_s, c_p */
      insn[ninsn - 1].op[1] = REF (pred, n1 + n2 + FLAG_REG); insn[ninsn - 1].opt[1] = OPT_REF;

      /* Remove old one */
      change_insn_type (&insn[n1 + n2 - 1], II_NOP);
    } else change_insn_type (&insn[ninsn - 1], II_NOP); /* do not use branch slot */
  }

#if 1
  /* LRBB at start of succ BB is not valid anymore */
  if (n1 > 0 && insn[n1].index == II_LRBB) {
    if (type == 1) {
      /* We have two possibilities, how this could have happened:
         1. we just moved second predecessor of succ to pred,
            pred now having two predecessors => everything is ok
         2. we just moved second predecessor of succ to pred,
            now, having just one predecessor => LRBB is not needed anymore */
      if (f->bb[pred].prev[1] < 0) { /* handle second option */
        change_insn_type (&insn[n1], II_ADD);
        insn[n1].op[1] = 1; insn[n1].opt[1] = OPT_CONST;        
        insn[n1].op[2] = 0; insn[n1].opt[2] = OPT_CONST;        
        insn[n1].opt[3] = OPT_NONE;
      }
    } else {
      assert (0); /* not tested yet */
      change_insn_type (&insn[n1], II_NOP);
      for (i = n1; i < ninsn; i++)
        if (insn[i].index == II_CMOV && insn[i].op[3] == REF (pred, n1)) {
          assert (insn[i].opt[3] == OPT_REF);
          insn[i].op[3] = cond_op;
          insn[i].opt[3] = cond_opt;
          if (f->bb[pred].next[0] != succ) {
            unsigned long t; /* negate conditional -- exchange */
            assert (f->bb[pred].next[1] == succ);
            t = insn[i].op[1];
            insn[i].op[1] = insn[i].op[2];
            insn[i].op[2] = t;
            t = insn[i].opt[1];
            insn[i].opt[1] = insn[i].opt[2];
            insn[i].opt[2] = t;
          }
        }
    }
  }
#endif
  
  for (i = 0; i < ninsn; i++) reloc[i] = -1;

  /* Add conditional instructions if required */
  if (add_cond) {
    recalc_last_used_reg (f, pred);
    recalc_last_used_reg (f, succ);
    
    /* r0 -- add nop for it */
    change_insn_type (&insn[n1 + n2], II_NOP);
    for (i = 1; i < MAX_REGS; i++) {
      cuc_insn *ii = &insn[n1 + n2 + i];
      int a = f->bb[pred].last_used_reg[i];
      int b = f->bb[succ].last_used_reg[i];

      /* We have deleted first branch instruction, now we must setup FLAG_REG,
         to point to conditional */
      if (i == FLAG_REG) {
        change_insn_type (ii, II_CMOV);
        ii->type = i == FLAG_REG || i == LRBB_REG ? IT_COND : 0;
        ii->dep = NULL;
        ii->op[0] = i; ii->opt[0] = OPT_REGISTER | OPT_DEST;
        ii->op[1] = cond_op; ii->opt[1] = cond_opt;
        if (b >= 0) {
          ii->op[2] = b; ii->opt[2] = OPT_REF;
        } else {
          ii->op[2] = cond_op; ii->opt[2] = cond_opt;
        }
        ii->op[3] = cond_op; ii->opt[3] = cond_opt;
        reloc[REF_I(a)] = REF (pred, n1 + n2 + i);
      } else if (b < 0) change_insn_type (ii, II_NOP);
      else if (a < 0) {
        change_insn_type (ii, II_ADD);
        ii->type = i == FLAG_REG || i == LRBB_REG ? IT_COND : 0;
        ii->dep = NULL;
        ii->op[0] = i; ii->opt[0] = OPT_REGISTER | OPT_DEST;
        ii->op[1] = b; ii->opt[1] = OPT_REF;
        ii->op[2] = 0; ii->opt[2] = OPT_CONST;
        ii->opt[3] = OPT_NONE;
      } else if (b >= 0) {
        change_insn_type (ii, II_CMOV);
        ii->type = i == FLAG_REG || i == LRBB_REG ? IT_COND : 0;
        ii->dep = NULL;
        ii->op[0] = i; ii->opt[0] = OPT_REGISTER | OPT_DEST;
        ii->op[1] = a; ii->opt[1] = OPT_REF;
        ii->op[2] = b; ii->opt[2] = OPT_REF;
        ii->op[3] = cond_op; ii->opt[3] = cond_opt;
        reloc[REF_I(a)] = REF (pred, n1 + n2 + i);
      }
      sprintf (ii->disasm, "cmov (join BB)");
    }
  }
  
  if (cuc_debug) cuc_check (f);
  i = 0;
  switch (type) {
  case 0:
    assert (f->bb[pred].next[0] >= 0);
    if (f->bb[pred].next[0] == succ) f->bb[pred].next[0] = f->bb[succ].next[0];
    if (f->bb[pred].next[1] == succ) f->bb[pred].next[1] = f->bb[succ].next[0];
    break;
  case 1:
    assert (f->bb[pred].next[0] >= 0 && f->bb[pred].next[0] != BBID_END);
    f->bb[pred].next[0] = f->bb[succ].next[0];
    f->bb[pred].next[1] = f->bb[succ].next[1];
    break;
  case 2:
    assert (f->bb[pred].next[0] >= 0 && f->bb[pred].next[0] != BBID_END);
    f->bb[pred].next[0] = f->bb[succ].next[0];
    f->bb[pred].next[1] = f->bb[succ].next[1];
    break;
  }
  if (f->bb[pred].next[0] < 0) f->bb[pred].next[0] = f->bb[pred].next[1];
  if (f->bb[pred].next[0] == f->bb[pred].next[1]) f->bb[pred].next[1] = -1;

  if (type == 0) assert (f->bb[succ].next[1] < 0);

  /* We just did something stupid -- we joined two predecessors into one;
     succ may need the information from which block we came.  We will repair
     this by converting LRBB to CMOV */
  for (j = 0; j < 2; j++) {
    int nb = f->bb[pred].next[j];
    int t;

    /* check just valid connections */
    if (nb < 0 || nb == BBID_END) continue;

    /* check type */
    if (f->bb[nb].prev[0] == pred && f->bb[nb].prev[1] == succ) t = 1;
    else if (f->bb[nb].prev[1] == pred && f->bb[nb].prev[0] == succ) t = 0;
    else continue;

    /* check all LRBB instructions.  */
    for (i = 0; i < f->bb[nb].ninsn; i++)
      if (f->bb[nb].insn[i].index == II_LRBB) {
        cuc_insn *lrbb =&f->bb[nb].insn[i];
        change_insn_type (lrbb, II_CMOV);
        lrbb->op[1] = t; lrbb->opt[1] = OPT_CONST;
        lrbb->op[2] = 1 - t; lrbb->opt[2] = OPT_CONST;
        lrbb->op[3] = cond_op; lrbb->opt[3] = cond_opt;
        lrbb->type |= IT_COND;
      }
  }
  
  f->bb[succ].type = BB_DEAD;
  //PRINTF (" %x %x %x %x %x\n", f->bb[pred].next[0], f->bb[pred].next[1], f->bb[succ].next[0], f->bb[succ].next[1], insn[ninsn - 1].type);
  /* remove branch instruction, if there is only one successor */
  if (f->bb[pred].next[1] < 0 && ninsn > 0 && insn[ninsn - 1].type & IT_BRANCH) {
    assert (f->bb[pred].next[0] != pred); /* end BB, loop should not be possible */
    change_insn_type (&insn[ninsn - 1], II_NOP);
  }

  /* Set max count */
  if (f->bb[pred].cnt < f->bb[succ].cnt) f->bb[pred].cnt = f->bb[succ].cnt;
  f->bb[pred].ninsn = ninsn;
  f->bb[succ].ninsn = 0;
  free (f->bb[pred].insn); f->bb[pred].insn = NULL;
  free (f->bb[succ].insn); f->bb[succ].insn = NULL;
  f->bb[pred].insn = insn;
  for (i = 0; i < f->num_bb; i++) if (!(f->bb[i].type & BB_DEAD)) {
    if (f->bb[i].prev[0] == succ) f->bb[i].prev[0] = pred;
    if (f->bb[i].prev[1] == succ) f->bb[i].prev[1] = pred;
    if (f->bb[i].prev[0] == f->bb[i].prev[1]) f->bb[i].prev[1] = -1;
    for (j = 0; j < f->bb[i].ninsn; j++)
      for (k = 0; k < MAX_OPERANDS; k++)
        if (f->bb[i].insn[j].opt[k] & OPT_REF) {
          /* Check if we are referencing successor BB -> relocate to second part of
             the new block */
          if (REF_BB (f->bb[i].insn[j].op[k]) == succ) {
            int t = f->bb[i].insn[j].op[k];
            int ndest = REF (pred, REF_I (t) + n1);
            //PRINTF ("%x: %x %x\n", REF(i, j), t, ndest);

            /* We've found a reference to succ. block, being removed, relocate */
            f->bb[i].insn[j].op[k] = ndest;
          } else if (REF_BB(f->bb[i].insn[j].op[k]) == pred) {
            if (i != pred && reloc[REF_I(f->bb[i].insn[j].op[k])] >= 0) {
              f->bb[i].insn[j].op[k] = reloc[REF_I(f->bb[i].insn[j].op[k])];
            }
          }
        }
  }
  
  if (cuc_debug) cuc_check (f);
  if (cuc_debug >= 3) print_cuc_bb (f, "join");
}

/* Optimize basic blocks */
int optimize_bb (cuc_func *f)
{
  int modified = 0;
  int i, j;
remove_lrbb:
  /* we can remove lrbb instructions from blocks with just one predecessor */
  for (i = 0; i < f->num_bb; i++) if (!(f->bb[i].type & BB_DEAD)) {
    if (f->bb[i].prev[0] >= 0 && f->bb[i].prev[1] < 0) { /* exactly one predecessor */
      for (j = 0; j < f->bb[i].ninsn; j++)
        if (f->bb[i].insn[j].index == II_LRBB) {
          cuc_insn *t;
          cucdebug (4, "-lrbb %x.%x\n", i, j);

          /* Change to add LRBB, 0, 0 */
          change_insn_type (&f->bb[i].insn[j], II_ADD);
          f->bb[i].insn[j].type &= ~IT_VOLATILE;
          f->bb[i].insn[j].opt[1] = f->bb[i].insn[j].opt[2] = OPT_CONST;
          f->bb[i].insn[j].op[1] = f->bb[i].insn[j].op[2] = 0; /* always use left block */
          f->bb[i].insn[j].opt[3] = OPT_NONE;
          modified = 1;
          if (f->bb[i].prev[0] != BBID_START && f->bb[f->bb[i].prev[0]].ninsn > 0) {
            t = &f->bb[f->bb[i].prev[0]].insn[f->bb[f->bb[i].prev[0]].ninsn - 1];
          
            /* If the predecessor still has a conditional jump instruction, we must be careful.
               If next[0] == next[1] join them. Now we will link lrbb and correct the situation */
            if (t->type & IT_BRANCH) { /* We must set a reference to branch result */
              f->bb[i].insn[j].opt[1] = t->opt[1];
              f->bb[i].insn[j].op[1] = t->op[1];
              /* sometimes branch is not needed anymore */
              if (f->bb[f->bb[i].prev[0]].next[1] < 0) change_insn_type (t, II_NOP);
            }
          }
        }
    }
  }
  
  /* Ordering of joining types is cruical -- we should concat all directly connected BBs
     together first, so when we do a type != 1 joining, we can remove LRBB, directly by
     looking at number of its predeccessors */

  /* Type 1 joining
     1. link between pred & succ
     2. no other pred's successors
     3. no other succ's predecessors, except if pred has max one */
  for (i = 0; i < f->num_bb; i++) if (!(f->bb[i].type & BB_DEAD)) {
    int p = f->bb[i].prev[0];
    if (p < 0 || p == BBID_START) continue;
    /* one successor and max sum of 3 predecessors */
    if (f->bb[p].next[0] >= 0 && f->bb[p].next[1] < 0
     && (f->bb[p].prev[1] < 0 || f->bb[i].prev[1] < 0)) {
      /* First we will move all predecessors from succ to pred, and then we will do
         real type 1 joining */
      if (f->bb[i].prev[1] >= 0 && f->bb[i].prev[1] != BBID_START) {
        int p1 = f->bb[i].prev[1];
        /* joining is surely not worth another extra memory access */
        if (f->bb[p].nmemory) continue;
        if (f->bb[p].prev[0] >= 0) {
           assert (f->bb[p].prev[1] < 0);
           f->bb[p].prev[1] = p1;
        } else f->bb[p].prev[0] = p1;
        if (f->bb[p1].next[0] == i) f->bb[p1].next[0] = p;
        else if (f->bb[p1].next[1] == i) f->bb[p1].next[1] = p;
        else assert (0);
        f->bb[i].prev[1] = -1;
      }
      assert (p >= 0 && f->bb[i].prev[1] < 0); /* one predecessor */
      join_bb (f, p, i, 1);
      modified = 1;
      goto remove_lrbb;
    }
  }
  
  /* Type 0 joining
     1. link between pred & succ
     2. no memory accesses in succ
     3. optional pred's second successors
     4. max. one succ's successors */
  for (i = 0; i < f->num_bb; i++) if (!(f->bb[i].type & BB_DEAD))
    if (f->bb[i].prev[0] >= 0 && f->bb[i].prev[0] != BBID_START
     && f->bb[i].prev[1] < 0 /* one predecessor */
     && f->bb[i].next[1] < 0 /* max. one successor */
     && f->bb[i].nmemory == 0) {                  /* and no memory acceses */
      join_bb (f, f->bb[i].prev[0], i, 0);
      modified = 1;
      goto remove_lrbb;
    }

  /* Type 2 joining
     1. link between pred & succ
     2. succ has exactly one predeccessor
     3. pred & succ share common successor
     4. optional succ's second successor */
  for (i = 0; i < f->num_bb; i++) if (!(f->bb[i].type & BB_DEAD))
    if (f->bb[i].prev[0] >= 0 && f->bb[i].prev[1] < 0) { /* one predecessor */
      int p = f->bb[i].prev[0];
      if (p == BBID_START) continue;
#if 0 /* not yet supported */
      if (f->bb[p].next[0] == i
       && (f->bb[i].next[1] == f->bb[p].next[1]
        || f->bb[i].next[1] == f->bb[p].next[0])) {
        join_bb (f, p, i, 2);
        goto remove_lrbb;
      }
#endif
      if (f->bb[p].next[1] == i
       && (f->bb[p].next[0] == f->bb[i].next[1]
        || f->bb[p].next[0] == f->bb[i].next[0])) {
        join_bb (f, p, i, 2);
        modified = 1;
        goto remove_lrbb;
      }
    }

  /* BB simplify:
     1. a block has exactly 2 successors A and B
     2. A has exactly one successor -- B
     3. A has no memory accesses
     to:
     flow always goes though A, LRBB is replaced by current block conditional
    */
  for (i = 0; i < f->num_bb; i++) if (!(f->bb[i].type & BB_DEAD))
    if (f->bb[i].next[0] >= 0 && f->bb[i].next[0] != BBID_END
      && f->bb[i].next[1] >= 0 && f->bb[i].next[1] != BBID_END) {
      int a = f->bb[i].next[0];
      int b = f->bb[i].next[1];
      int neg = 0;
      /* Exchange? */
      if (f->bb[b].next[0] == a && f->bb[b].next[1] < 0) {
        int t = a;
        a = b;
        b = t;
        neg = 1;
      }
      /* Do the simplification if possible */
      if (f->bb[a].next[0] == b && f->bb[a].next[1] < 0
       && f->bb[a].nmemory == 0) {
        simplify_bb (f, i, a, b, neg);
        modified = 1;
        goto remove_lrbb;
      }
    }
    
  return modified;
}

/* Removes BBs marked as dead */
int remove_dead_bb (cuc_func *f)
{
  int i, j, k, d = 0;

  for (i = 0; i < f->num_bb; i++) if (f->bb[i].type & BB_DEAD) {
    if (f->bb[i].insn) free (f->bb[i].insn);
    f->bb[i].insn = NULL;
    reloc[i] = -1;
  } else {
    reloc[i] = d;
    f->bb[d++] = f->bb[i];
  }
  if (f->num_bb == d) return 0;
  f->num_bb = d;

  /* relocate initial blocks */
  for (i = 0; i < f->num_init_bb; i++)
    f->init_bb_reloc[i] = reloc[f->init_bb_reloc[i]];
  
  /* repair references */
  for (i = 0; i < f->num_bb; i++) if (!(f->bb[i].type & BB_DEAD)) {
          cucdebug (5, "%x %x %x %x %x\n", i, f->bb[i].prev[0], f->bb[i].prev[1], f->bb[i].next[0], f->bb[i].next[1]);
          fflush (stdout);
    if (f->bb[i].prev[0] >= 0 && f->bb[i].prev[0] != BBID_START)
      assert ((f->bb[i].prev[0] = reloc[f->bb[i].prev[0]]) >= 0);
    if (f->bb[i].prev[1] >= 0 && f->bb[i].prev[1] != BBID_START)
      assert ((f->bb[i].prev[1] = reloc[f->bb[i].prev[1]]) >= 0);
    if (f->bb[i].next[0] >= 0 && f->bb[i].next[0] != BBID_END)
      assert ((f->bb[i].next[0] = reloc[f->bb[i].next[0]]) >= 0);
    if (f->bb[i].next[1] >= 0 && f->bb[i].next[1] != BBID_END)
      assert ((f->bb[i].next[1] = reloc[f->bb[i].next[1]]) >= 0);
    if (f->bb[i].prev[0] == f->bb[i].prev[1]) f->bb[i].prev[1] = -1;
    if (f->bb[i].next[0] == f->bb[i].next[1]) f->bb[i].next[1] = -1;

    for (j = 0; j < f->bb[i].ninsn; j++)
      for (k = 0; k < MAX_OPERANDS; k++)
        if ((f->bb[i].insn[j].opt[k] & OPT_BB) &&
                                        ((signed)f->bb[i].insn[j].op[k] >= 0)) {
          if (f->bb[i].insn[j].op[k] != BBID_END)
            assert ((f->bb[i].insn[j].op[k] = reloc[f->bb[i].insn[j].op[k]]) >= 0);
        } else if (f->bb[i].insn[j].opt[k] & OPT_REF) {
          int t = f->bb[i].insn[j].op[k];
          assert (reloc[REF_BB(t)] >= 0);
          f->bb[i].insn[j].op[k] = REF (reloc[REF_BB(t)], REF_I (t));
        }
  }
  return 1;
}

/* Recursive calculation of dependencies */
static void reg_dep_rec (cuc_func *f, int cur)
{
  int i, j;
  cuc_insn *insn = f->bb[cur].insn;
 
  //PRINTF ("\n %i", cur); 
  /* Spread only, do not loop */
  if (f->bb[cur].tmp) return;
  f->bb[cur].tmp = 1;
  //PRINTF ("!   ");

  for (i = 0; i < f->bb[cur].ninsn; i++) {
    /* Check for destination operand(s) */
    for (j = 0; j < MAX_OPERANDS; j++) if (insn[i].opt[j] & OPT_DEST)
      if ((insn[i].opt[j] & ~OPT_DEST) == OPT_REGISTER && (signed)insn[i].op[j] >= 0) {
        //PRINTF ("%i:%i,%x ", insn[i].op[j], i, REF (cur, i));
        assert (insn[i].op[j] > 0 && insn[i].op[j] < MAX_REGS); /* r0 should never be dest */
 	f->bb[cur].last_used_reg[insn[i].op[j]] = REF (cur, i);
      }
  }

  if (f->bb[cur].next[0] >= 0 && f->bb[cur].next[0] != BBID_END)
    reg_dep_rec (f, f->bb[cur].next[0]);
  if (f->bb[cur].next[1] >= 0 && f->bb[cur].next[1] != BBID_END)
    reg_dep_rec (f, f->bb[cur].next[1]);
}

/* Detect register dependencies */
void reg_dep (cuc_func *f)
{
  int i, b, c;

  /* Set dead blocks */
  for (b = 0; b < f->num_bb; b++) {
    f->bb[b].tmp = 0;
    for (i = 0; i < MAX_REGS; i++) f->bb[b].last_used_reg[i] = -1;
  }

  /* Start with first block and set dependecies of all reachable blocks */
  /* At the same time set last_used_regs */
  reg_dep_rec (f, 0);

  for (i = 0; i < f->num_bb; i++)
    if (f->bb[i].tmp) f->bb[i].tmp = 0;
    else f->bb[i].type |= BB_DEAD;
  
  /* Detect loops; mark BBs where loops must be broken */  
  for (c = 0; c < f->num_bb; c++) {
    int min = 3, minb = 0;

    /* search though all non-visited for minimum number of unvisited predecessors */
    for (b = 0; b < f->num_bb; b++) if (!f->bb[b].tmp) {
      int tmp = 0;
      if (f->bb[b].prev[0] >= 0 && f->bb[b].prev[0] != BBID_START
       && !f->bb[f->bb[b].prev[0]].tmp) tmp++;
      if (f->bb[b].prev[1] >= 0 && f->bb[b].prev[1] != BBID_START
       && !f->bb[f->bb[b].prev[1]].tmp) tmp++;
      if (tmp < min) {
        minb = b;
        min = tmp;
        if (tmp == 0) break; /* We already have the best one */
      }
    }
    b = minb;
    f->bb[b].tmp = 1; /* Mark visited */
    cucdebug (3, "minb %i min %i\n", minb, min);
    if (min) { /* We just broke the loop */
      f->bb[b].type |= BB_INLOOP;
    }
  }
  
  /* Set real predecessors in cmov instructions to previous blocks */
  for (b = 0; b < f->num_bb; b++)
    for (i = 1; i < MAX_REGS - 1; i++) {
      int pa, pb;
      assert (f->bb[b].insn[i].index ==  II_CMOV);
      assert (f->bb[b].insn[i].opt[0] == (OPT_REGISTER | OPT_DEST));
      assert (f->bb[b].insn[i].op[0] == i);
      if (f->bb[b].prev[0] < 0 || f->bb[b].prev[0] == BBID_START) pa = -1;
      else pa = f->bb[f->bb[b].prev[0]].last_used_reg[i]; 
      if (f->bb[b].prev[1] < 0 || f->bb[b].prev[1] == BBID_START) pb = -1;
      else pb = f->bb[f->bb[b].prev[1]].last_used_reg[i]; 
 
      /* We do some very simple optimizations right away to make things more readable */
      if (pa < 0 && pb < 0) {
        /* Was not used at all */
        change_insn_type (&f->bb[b].insn[i], II_ADD);
        f->bb[b].insn[i].op[2] = 0; f->bb[b].insn[i].opt[2] = OPT_CONST;
        f->bb[b].insn[i].opt[3] = OPT_NONE;
      } else if (pa < 0) {
        change_insn_type (&f->bb[b].insn[i], II_ADD);
        assert (f->INSN(pb).opt[0] == (OPT_REGISTER | OPT_DEST));
        f->bb[b].insn[i].op[1] = pb; f->bb[b].insn[i].opt[1] = OPT_REF;
        f->bb[b].insn[i].op[2] = 0; f->bb[b].insn[i].opt[2] = OPT_CONST;
        f->bb[b].insn[i].opt[3] = OPT_NONE;
      } else if (pb < 0) {
        change_insn_type (&f->bb[b].insn[i], II_ADD);
        assert (f->INSN(pa).opt[0] == (OPT_REGISTER | OPT_DEST));
        f->bb[b].insn[i].op[1] = pa; f->bb[b].insn[i].opt[1] = OPT_REF;
        f->bb[b].insn[i].op[2] = 0; f->bb[b].insn[i].opt[2] = OPT_CONST;
        f->bb[b].insn[i].opt[3] = OPT_NONE;
      } else {
  	int t = REF (b, 0); /* lrbb should be first instruction */
        assert (f->INSN(t).index == II_LRBB);

        f->bb[b].insn[i].op[1] = pa; f->bb[b].insn[i].opt[1] = OPT_REF;
        assert (f->INSN(pa).opt[0] == (OPT_REGISTER | OPT_DEST));
        
        f->bb[b].insn[i].op[2] = pb; f->bb[b].insn[i].opt[2] = OPT_REF;
        assert (f->INSN(pb).opt[0] == (OPT_REGISTER | OPT_DEST));
        
        /* Update op[3] -- flag register */
        assert (f->bb[b].insn[i].opt[3] == OPT_REGISTER);
        assert (f->bb[b].insn[i].op[3] == LRBB_REG);
        assert (t >= 0);
    	f->bb[b].insn[i].opt[3] = OPT_REF; /* Convert already used regs to references */
  	f->bb[b].insn[i].op[3] = t;
        assert (f->INSN(t).opt[0] == (OPT_REGISTER | OPT_DEST));
      }
    }

  /* assign register references */
  for (b = 0; b < f->num_bb; b++) {
    /* rebuild last used reg array */
    f->bb[b].last_used_reg[0] = -1;
    if (f->bb[b].insn[0].index == II_LRBB) f->bb[b].last_used_reg[LRBB_REG] = 0;
    else f->bb[b].last_used_reg[LRBB_REG] = -1;

    for (i = 1; i < MAX_REGS - 1; i++)
      f->bb[b].last_used_reg[i] = -1;
    
    /* Create references */
    for (i = 0; i < f->bb[b].ninsn; i++) {
      int k;
      /* Check for source operands first */
      for (k = 0; k < MAX_OPERANDS; k++) {
        if (!(f->bb[b].insn[i].opt[k] & OPT_DEST)) {
          if (f->bb[b].insn[i].opt[k] & OPT_REGISTER) {
            int t = f->bb[b].last_used_reg[f->bb[b].insn[i].op[k]];

            if (f->bb[b].insn[i].op[k] == 0) { /* Convert r0 to const0 */
              f->bb[b].insn[i].opt[k] = OPT_CONST;
              f->bb[b].insn[i].op[k] = 0;
            } else if (t >= 0) {
              f->bb[b].insn[i].opt[k] = OPT_REF; /* Convert already used regs to references */
  	      f->bb[b].insn[i].op[k] = t;
              assert (f->INSN(t).opt[0] == (OPT_REGISTER | OPT_DEST));
              //f->INSN(t).op[0] = -1;
  	    }
          } else if (f->bb[b].insn[i].opt[k] & OPT_REF) {
            //f->INSN(f->bb[b].insn[i].op[k]).op[0] = -1; /* Mark referenced */
            f->INSN(f->bb[b].insn[i].op[k]).type &= ~IT_UNUSED;
          }
        }
      }
      
      /* Now check for destination operand(s) */
      for (k = 0; k < MAX_OPERANDS; k++) if (f->bb[b].insn[i].opt[k] & OPT_DEST)
        if ((f->bb[b].insn[i].opt[k] & ~OPT_DEST) == OPT_REGISTER
          && (int)f->bb[b].insn[i].op[k] >= 0) {
          assert (f->bb[b].insn[i].op[k] != 0); /* r0 should never be dest */
   	  f->bb[b].last_used_reg[f->bb[b].insn[i].op[k]] = REF (b, i);
        }
    }
  }

  /* Remove all unused lrbb */
  for (b = 0; b < f->num_bb; b++)
    for (i = 0; i < f->bb[b].ninsn; i++)
      if (f->bb[b].insn[i].type & IT_UNUSED) change_insn_type (&f->bb[b].insn[i], II_NOP);

  /* SSAs with final register value are marked as outputs */
  assert (f->bb[f->num_bb - 1].next[0] == BBID_END);
  for (i = 0; i < MAX_REGS; i++)
    {
      if (!caller_saved[i])
	{
	  int t = f->bb[f->num_bb - 1].last_used_reg[i];
	  /* Mark them volatile, so optimizer does not remove them */
	  if (t >= 0)
	    {
	      f->bb[REF_BB(t)].insn[REF_I(t)].type |= IT_OUTPUT;
	    }
	}
    }
}

/* split the BB, based on the group numbers in .tmp */
void expand_bb (cuc_func *f, int b)
{
  int n = f->num_bb;
  int mg = 0;
  int b1, i, j;

  for (i = 0; i < f->bb[b].ninsn; i++)
    if (f->bb[b].insn[i].tmp > mg) mg = f->bb[b].insn[i].tmp;
  
  /* Create copies */
  for (b1 = 1; b1 <= mg; b1++) {
    assert (f->num_bb < MAX_BB);
    cpy_bb (&f->bb[f->num_bb], &f->bb[b]);
    f->num_bb++;
  }

  /* Relocate */
  for (b1 = 0; b1 < f->num_bb; b1++)
    for (i = 0; i < f->bb[b1].ninsn; i++) {
      dep_list *d = f->bb[b1].insn[i].dep;
      for (j = 0; j < MAX_OPERANDS; j++)
        if (f->bb[b1].insn[i].opt[j] & OPT_REF) {
          int t = f->bb[b1].insn[i].op[j];
          if (REF_BB(t) == b && f->INSN(t).tmp != 0)
            f->bb[b1].insn[i].op[j] = REF (n + f->INSN(t).tmp - 1, REF_I(t));
        }
      while (d) {
        if (REF_BB (d->ref) == b && f->INSN(d->ref).tmp != 0)
          d->ref = REF (n + f->INSN(d->ref).tmp - 1, REF_I(d->ref));
        d = d->next;
      }
    }

  /* Delete unused instructions */
  for (j = 0; j <= mg; j++) {
    if (j == 0) b1 = b;
    else b1 = n + j - 1;
    for (i = 0; i < f->bb[b1].ninsn; i++) {
      if (f->bb[b1].insn[i].tmp != j)
        change_insn_type (&f->bb[b1].insn[i], II_NOP);
      f->bb[b1].insn[i].tmp = 0;
    }
    if (j < mg) {
      f->bb[b1].next[0] = n + j;
      f->bb[b1].next[1] = -1;
      f->bb[n + j].prev[0] = b1;
      f->bb[n + j].prev[1] = -1;
    } else {
      i = f->bb[b1].next[0];
      f->bb[n + j].prev[0] = j == 1 ? b : b1 - 1;
      f->bb[n + j].prev[1] = -1;
      if (i >= 0 && i != BBID_END) {
        if (f->bb[i].prev[0] == b) f->bb[i].prev[0] = b1;
        if (f->bb[i].prev[1] == b) f->bb[i].prev[1] = b1;
      }
      i = f->bb[b1].next[1];
      if (i >= 0 && i != BBID_END) {
        if (f->bb[i].prev[0] == b) f->bb[i].prev[0] = b1;
        if (f->bb[i].prev[1] == b) f->bb[i].prev[1] = b1;
      }
    }
  }
}

/* Scans sequence of BBs and set bb[].cnt */
void generate_bb_seq (cuc_func *f, char *mp_filename, char *bb_filename)
{
  FILE *fi, *fo;
  struct mprofentry_struct *buf;
  const int bufsize = 256;
  unsigned long *bb_start;
  unsigned long *bb_end;
  int b, i, r;
  int curbb, prevbb = -1;
  unsigned long addr = -1;
  unsigned long prevaddr = -1;
  int mssum = 0;
  int mlsum = 0;
  int mscnt = 0;
  int mlcnt = 0;
  int reopened = 0;

  /* Use already opened stream? */
  if (runtime.sim.fmprof) {
    fi = runtime.sim.fmprof;
    reopened = 1;
    rewind (fi);
  } else assert (fi = fopen (mp_filename, "rb"));
  assert (fo = fopen (bb_filename, "wb+"));
  
  assert (bb_start = (unsigned long *) malloc (sizeof (unsigned long) * f->num_bb));
  assert (bb_end = (unsigned long *) malloc (sizeof (unsigned long) * f->num_bb));
  for (b = 0; b < f->num_bb; b++) {
    bb_start[b] = f->start_addr + f->bb[b].first * 4;
    bb_end[b] = f->start_addr + f->bb[b].last * 4;
    //PRINTF ("%i %x %x\n", b, bb_start[b], bb_end[b]);
    f->bb[0].cnt = 0;
  }
  
  buf = (struct mprofentry_struct *) malloc (sizeof (struct mprofentry_struct) * bufsize);
  assert (buf);

  //PRINTF ("BBSEQ:\n");
  do {
    r = fread (buf, sizeof (struct mprofentry_struct), bufsize, fi);
    //PRINTF ("r%i : ", r);
    for (i = 0; i < r; i++) {
      if (buf[i].type & MPROF_FETCH) {
        //PRINTF ("%x, ", buf[i].addr);
        if (buf[i].addr >= f->start_addr && buf[i].addr <= f->end_addr) {
          assert (buf[i].type & MPROF_32);
          prevaddr = addr;
          addr = buf[i].addr;
          for (b = 0; b < f->num_bb; b++)
            if (bb_start[b] <= addr && addr <= bb_end[b]) break;
          assert (b < f->num_bb);
          curbb = b;
          if (prevaddr + 4 != addr) prevbb = -1;
        } else curbb = -1;
        
        /* TODO: do not count interrupts */
        if (curbb != prevbb && curbb >= 0) {
          fwrite (&curbb, sizeof (unsigned long), 1, fo);
          //PRINTF (" [%i] ", curbb);
          f->bb[curbb].cnt++;
          prevbb = curbb;
        }
      } else {
        if (verify_memoryarea(buf[i].addr)) {
          if (buf[i].type & MPROF_WRITE) mscnt++, mssum += cur_area->ops.delayw;
          else mlcnt++, mlsum += cur_area->ops.delayr;
        }
      }
    }
    //PRINTF ("\n");
  } while (r == bufsize);
  //PRINTF ("\n");

  runtime.cuc.mdelay[0] = (1. * mlsum) / mlcnt;
  runtime.cuc.mdelay[1] = (1. * mssum) / mscnt;
  runtime.cuc.mdelay[2] = runtime.cuc.mdelay[3] = 1;
  f->num_runs = f->bb[0].cnt;
  if (!reopened) fclose (fi);
  fclose (fo);
  free (buf);
  free (bb_end);
  free (bb_start);

  /* Initialize basic block relocations */
  f->num_init_bb = f->num_bb;
  //PRINTF ("num_init_bb = %i\n", f->num_init_bb);
  assert (f->init_bb_reloc = (int *)malloc (sizeof (int) * f->num_init_bb));
  for (b = 0; b < f->num_init_bb; b++) f->init_bb_reloc[b] = b;
}

/* Scans sequence of BBs and set counts for pre/unrolled loop for BB b */
void count_bb_seq (cuc_func *f, int b, char *bb_filename, int *counts, int preroll, int unroll)
{
  FILE *fi;
  const int bufsize = 256;
  int i, r;
  int *buf;
  int cnt = 0;
  int times = preroll - 1 + unroll;

  assert (fi = fopen (bb_filename, "rb"));
  for (i = 0; i < times; i++) counts[i] = 0;
  assert (buf = (int *) malloc (sizeof (int) * bufsize));

  do {
    r = fread (buf, sizeof (int), bufsize, fi);
    for (i = 0; i < r; i++) {
      /* count consecutive acesses */
      if (f->init_bb_reloc[buf[i]] == b) {
        counts[cnt]++;
        if (++cnt >= times) cnt = preroll - 1;
      } else cnt = 0;
    }
  } while (r == bufsize);

  log ("Counts %i,%i :", preroll, unroll);
  for (i = 0; i < times; i++) log ("%x ", counts[i]);
  log ("\n");

  fclose (fi);
  free (buf);
}

/* relocate all accesses inside of BB b to back/fwd */
static void relocate_bb (cuc_bb *bb, int b, int back, int fwd)
{
  int i, j;
  for (i = 0; i < bb->ninsn; i++)
    for (j = 0; j < MAX_OPERANDS; j++)
      if (bb->insn[i].opt[j] & OPT_REF
       && REF_BB (bb->insn[i].op[j]) == b) {
        int t = REF_I (bb->insn[i].op[j]);
        if (t < i) bb->insn[i].op[j] = REF (back, t);
        else bb->insn[i].op[j] = REF (fwd, t);
      }
}

/* Preroll if type == 1 or unroll if type == 0 loop in BB b `ntimes' times and return
   new function. Original function is unmodified. */
static cuc_func *roll_loop (cuc_func *f, int b, int ntimes, int type)
{
  int b1, t, i, prevb, prevart_b;
  cuc_func *n = dup_func (f);
  cuc_bb *ob = &f->bb[b];
  cuc_insn *ii;

  assert (ntimes > 1);
  cucdebug (3, "roll type = %i, BB%i x %i (num_bb %i)\n", type, b, ntimes, n->num_bb);
  ntimes--;
  assert (n->num_bb + ntimes * 2 < MAX_BB);
  
  prevb = b;
  prevart_b = b;

  /* point to first artificial block */
  if (n->bb[b].next[0] != b) {
    n->bb[b].next[0] = n->num_bb + 1;
  } else if (n->bb[b].next[1] != b) {
    n->bb[b].next[1] = n->num_bb + 1;
  }

  /* Duplicate the BB */
  for (t = 0; t < ntimes; t++) {
    cuc_bb *pb = &n->bb[prevart_b];
    /* Add new block and set links */
    b1 = n->num_bb++;
    cpy_bb (&n->bb[b1], ob);
    /* Only one should be in loop, so we remove any INLOOP flags from duplicates */
    n->bb[b1].type &= ~BB_INLOOP;
    print_cuc_bb (n, "prerollA");
    
    printf ("prevb %i b1 %i prevart %i\n", prevb, b1, prevart_b);
    /* Set predecessor's successor */
    if (n->bb[prevb].next[0] == b) {
      n->bb[prevb].next[0] = b1;
      if (pb->next[0] < 0) pb->next[0] = b1 + 1;
      else pb->next[1] = b1 + 1;
      n->bb[b1].next[1] = b1 + 1;
    } else if (n->bb[prevb].next[1] == b) {
      if (pb->next[0] < 0) pb->next[0] = b1 + 1;
      else pb->next[1] = b1 + 1;
      n->bb[b1].next[0] = b1 + 1;
      n->bb[prevb].next[1] = b1;
    } else assert (0);

    /* Set predecessor */
    n->bb[b1].prev[0] = prevb;
    n->bb[b1].prev[1] = -1;

    /* Relocate backward references to current instance and forward references
       to previous one */
    relocate_bb (&n->bb[b1], b, b1, prevb);

    /* add artificial block, just to join accesses */
    b1 = n->num_bb++;
    cpy_bb (&n->bb[b1], ob);
    n->bb[b1].cnt = 0;
    
    for (i = 0; i < ob->ninsn - 1; i++) {
      ii = &n->bb[b1].insn[i];
      if (ob->insn[i].opt[0] & OPT_DEST) {
        change_insn_type (ii, II_CMOV);
        ii->op[0] = -1; ii->opt[0] = OPT_REGISTER | OPT_DEST;
        ii->op[1] = REF (prevart_b, i); ii->opt[1] = OPT_REF;
        ii->op[2] = REF (b1 - 1, i); ii->opt[2] = OPT_REF;

        /* Take left one, if we should have finished the first iteration*/
        if (pb->insn[pb->ninsn - 1].type & IT_BRANCH) {
          ii->op[3] = pb->insn[pb->ninsn - 1].op[1]; ii->opt[3] = pb->insn[pb->ninsn - 1].opt[1];
        } else {
          assert (pb->insn[pb->ninsn - 1].type & IT_COND);
          ii->op[3] = REF (prevart_b, pb->ninsn - 1); ii->opt[3] = OPT_REF;
        }
        ii->dep = NULL;
        ii->type = ob->insn[i].type & IT_COND;
      } else {
        change_insn_type (ii, II_NOP);
      }
    }

    /* Add conditional or instruction at the end, prioritizing flags */
    ii = &n->bb[b1].insn[ob->ninsn - 1];
    change_insn_type (ii, II_CMOV);
    ii->op[0] = FLAG_REG; ii->opt[0] = OPT_REGISTER | OPT_DEST;
    if (pb->insn[pb->ninsn - 1].type & IT_BRANCH) {
      ii->op[1] = pb->insn[pb->ninsn - 1].op[1];
      ii->opt[1] = pb->insn[pb->ninsn - 1].opt[1];
    } else {
      ii->op[1] = REF (prevart_b, pb->ninsn - 1);
      ii->opt[1] = OPT_REF;
    }
    if (n->bb[b1 - 1].insn[pb->ninsn - 1].type & IT_BRANCH) {
      ii->op[2] = n->bb[b1 - 1].insn[pb->ninsn - 1].op[1];
      ii->opt[2] = n->bb[b1 - 1].insn[pb->ninsn - 1].opt[1];
    } else {
      ii->op[2] = REF (b1 - 1, pb->ninsn - 1);
      ii->opt[2] = OPT_REF;
    }
    /* {z = x || y;} is same as {z = x ? x : y;} */
    ii->op[3] = ii->op[1]; ii->opt[3] = ii->opt[1];
    ii->type = IT_COND;

    /* Only one should be in loop, so we remove any INLOOP flags from duplicates */
    n->bb[b1].type &= ~BB_INLOOP;
    n->bb[b1].prev[0] = prevart_b;
    n->bb[b1].prev[1] = b1 - 1;
    n->bb[b1].next[0] = -1;
    n->bb[b1].next[1] = -1;

    prevb = b1 - 1;
    prevart_b = b1;
    print_cuc_bb (n, "prerollB");
  }
  
  print_cuc_bb (n, "preroll0");
  n->bb[prevart_b].next[0] = ob->next[0] == b ? ob->next[1] : ob->next[0];

  print_cuc_bb (n, "preroll1");
  /* repair BB after loop, to point back to latest artificial BB */
  b1 = n->bb[prevart_b].next[0];
  if (b1 >= 0 && b1 != BBID_END) {
    if (n->bb[b1].prev[0] == b) n->bb[b1].prev[0] = prevart_b;
    else if (n->bb[b1].prev[1] == b) n->bb[b1].prev[1] = prevart_b;
    else assert (0);
  }

  if (type) {
    /* Relink to itself */
    /* Set predecessor's successor */
    if (n->bb[prevb].next[0] == b) n->bb[prevb].next[0] = prevb;
    else if (n->bb[prevb].next[1] == b) n->bb[prevb].next[1] = prevb;
    else assert (0);
    n->bb[prevb].prev[1] = prevb;
    
    /* Set predecessor */
    if (n->bb[b].prev[0] == b) {
      n->bb[b].prev[0] = n->bb[b].prev[1];
      n->bb[b].prev[1] = -1;
    } else if (n->bb[b].prev[1] == b) n->bb[b].prev[1] = -1;
    else assert (0);
  } else {
    /* Relink back to start of the loop */
    /* Set predecessor's successor */
    if (n->bb[prevb].next[0] == b) n->bb[prevb].next[0] = b;
    else if (n->bb[prevb].next[1] == b) n->bb[prevb].next[1] = b;
    else assert (0);

    /* Set predecessor */
    if (n->bb[b].prev[0] == b) n->bb[b].prev[0] = prevb;
    else if (n->bb[b].prev[1] == b) n->bb[b].prev[1] = prevb;
    else assert (0);
  }

  print_cuc_bb (n, "preroll2");

  /* Relocate backward references to current instance and forward references
     to previous one */
  relocate_bb (&n->bb[b], b, b, prevb);

  /* Relocate all other blocks to point to latest prevart_b */
  for (i = 0; i < f->num_bb; i++)
    if (i != b) relocate_bb (&n->bb[i], b, prevart_b, prevart_b); 
 
  return n;
}

/* Unroll loop b unroll times and return new function. Original
   function is unmodified. */
cuc_func *preunroll_loop (cuc_func *f, int b, int preroll, int unroll, char *bb_filename)
{
  int b1, i;
  cuc_func *n, *t;
  int *counts;

  if (preroll > 1) {
    t = roll_loop (f, b, preroll, 1);
    b1 = t->num_bb - 2;
    if (unroll > 1) {
      //print_cuc_bb (t, "preunroll1");
      n = roll_loop (t, b1, unroll, 0);
      free_func (t);
    } else n = t;
  } else {
    b1 = b;
    if (unroll > 1) n = roll_loop (f, b1, unroll, 0);
    else return dup_func (f);
  }  
 
  /* Assign new counts to functions */
  assert (counts = (int *)malloc (sizeof (int) * (preroll - 1 + unroll)));
  count_bb_seq (n, b, bb_filename, counts, preroll, unroll);
  for (i = 0; i < preroll - 1 + unroll; i++) {
    if (i == 0) b1 = b;
    else b1 = f->num_bb + (i - 1) * 2;
    n->bb[b1].cnt = counts[i];
  }

  //print_cuc_bb (n, "preunroll");
  free (counts);
  return n;
}

