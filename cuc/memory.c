/* memory.c -- OpenRISC Custom Unit Compiler, memory optimization and scheduling
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
#include "abstract.h"
#include "sim-config.h"
#include "cuc.h"
#include "insn.h"


/* Cleans memory & data dependencies */
void clean_deps (cuc_func *f)
{
  int b, i;
  dep_list *t;
  for (b = 0; b < f->num_bb; b++) {
    for (i = 0; i < f->bb[b].ninsn; i++) {
      t = f->bb[b].insn[i].dep;
      while (t) {
        dep_list *tmp = t;
        t = t->next;
        free (tmp);        
      }
      f->bb[b].insn[i].dep = NULL;
    }

    t = f->bb[b].mdep;
    while (t) {
      dep_list *tmp = t;
      t = t->next;
      free (tmp);        
    }
    f->bb[b].mdep = NULL;
  }
  
  f->nmsched = 0;
}

/* Checks for memory conflicts between two instructions; returns 1 if detected
  0 - exact; 1 - strong; 2 - weak; 3 - none */
static int check_memory_conflict (cuc_func *f, cuc_insn *a, cuc_insn *b, int otype)
{
  switch (otype) {
    case MO_EXACT: /* exact */
    case MO_STRONG: /* strong */
      return 1;
    case MO_WEAK: /* weak */
      assert (a->type & IT_MEMORY);
      assert (b->type & IT_MEMORY);
      if ((a->opt[1] & OPT_REF) && f->INSN(a->op[1]).index == II_ADD
        &&(b->opt[1] & OPT_REF) && f->INSN(b->op[1]).index == II_ADD) {
        int aw, bw;
        assert ((aw = II_MEM_WIDTH (a->index)) >= 0);
        assert ((bw = II_MEM_WIDTH (b->index)) >= 0);

        a = &f->INSN(a->op[1]);
        b = &f->INSN(b->op[1]);
        if (a->opt[1] != b->opt[1] || a->op[1] != b->op[1]
         || a->opt[2] != OPT_CONST || b->opt[2] != OPT_CONST) return 1;
        
        /* Check if they overlap */
        if (a->op[2] >= b->op[2] && a->op[2] < b->op[2] + bw) return 1;
        if (b->op[2] >= a->op[2] && b->op[2] < a->op[2] + aw) return 1;
        return 0;
      } else return 1;
    case MO_NONE: /* none */
      return 0;
    default:
      assert (0);
  }
  return 1;
}

/* Adds memory dependencies based on ordering type:
  0 - exact; 1 - strong; 2 - weak;  3 - none */
void add_memory_dep (cuc_func *f, int otype)
{
  int b, i;
  dep_list *all_mem = NULL;

  for (b = 0; b < f->num_bb; b++) {
    cuc_insn *insn = f->bb[b].insn;
    for (i = 0; i < f->bb[b].ninsn; i++)
      if (insn[i].type & IT_MEMORY) {
        dep_list *tmp = all_mem;
        while (tmp) {
          //PRINTF ("%x %x\n", REF (b,i), tmp->ref);
          if (check_memory_conflict (f, &insn[i], &f->INSN(tmp->ref), otype))
            add_dep (&insn[i].dep, tmp->ref);
          tmp = tmp->next;
        }
        add_dep (&all_mem, REF (b, i));
      }
  }
  dispose_list (&all_mem);
}

/* Check if they address the same location, so we can join them */
static int same_transfers (cuc_func *f, int otype)
{
  int i, j;
  int modified = 0;
  if (otype == MO_WEAK || otype == MO_NONE) {
    for (i = 1, j = 1; i < f->nmsched; i++)
      /* Exclude memory stores and different memory types */
      if (f->mtype[i - 1] == f->mtype[i] && f->mtype[i] & MT_LOAD) {
        cuc_insn *a = &f->INSN(f->msched[i - 1]);
        cuc_insn *b = &f->INSN(f->msched[i]);
        if ((a->opt[1] & OPT_REF) && f->INSN(a->op[1]).index == II_ADD
          &&(b->opt[1] & OPT_REF) && f->INSN(b->op[1]).index == II_ADD) {
          a = &f->INSN(a->op[1]);
          b = &f->INSN(b->op[1]);
          /* Not in usual form? */
          if (a->opt[1] != b->opt[1] || a->op[1] != b->op[1]
           || a->opt[2] != OPT_CONST || b->opt[2] != OPT_CONST) goto keep;

          //PRINTF ("%i %i, ", a->op[2], b->op[2]);
          
          /* Check if they are the same => do not copy */
          if (a->op[2] == b->op[2]
            && REF_BB(f->msched[i - 1]) == REF_BB(f->msched[i])) {
            /* yes => remove actual instruction */
            int t1 = MIN (f->msched[i - 1], f->msched[i]);
            int t2 = MAX (f->msched[i - 1], f->msched[i]);
            int b, i, j;
            cucdebug (2, "Removing %x_%x and using %x_%x instead.\n",
              REF_BB(t2), REF_I(t2), REF_BB(t1), REF_I(t1));
            change_insn_type (&f->INSN(t2), II_NOP);
            modified = 1;
            /* Update references */
            for (b = 0; b < f->num_bb; b++)
              for (i = 0; i < f->bb[b].ninsn; i++)
                for (j = 0; j < MAX_OPERANDS; j++)
                  if (f->bb[b].insn[i].opt[j] & OPT_REF && f->bb[b].insn[i].op[j] == t2)
                    f->bb[b].insn[i].op[j] = t1;
            
          } else goto keep;
        } else goto keep;
      } else {
keep:
        f->msched[j] = f->msched[i];
        f->mtype[j++] = f->mtype[i];
      }
    f->nmsched = j;
  }
  return modified;
}

/* Check if two consecutive lb[zs] can be joined into lhz and if
   two consecutive lh[zs] can be joined into lwz */
static int join_transfers (cuc_func *f, int otype)
{
  int i, j;
  int modified = 0;

  /* We can change width even with strong memory ordering */
  if (otype == MO_WEAK || otype == MO_NONE || otype == MO_STRONG) {
    for (i = 1, j = 1; i < f->nmsched; i++)
      /* Exclude memory stores and different memory types */
      if (f->mtype[i - 1] == f->mtype[i] && f->mtype[i] & MT_LOAD) {
        cuc_insn *a = &f->INSN(f->msched[i - 1]);
        cuc_insn *b = &f->INSN(f->msched[i]);
        int aw = f->mtype[i - 1] & MT_WIDTH;
        if ((a->opt[1] & OPT_REF) && f->INSN(a->op[1]).index == II_ADD
          &&(b->opt[1] & OPT_REF) && f->INSN(b->op[1]).index == II_ADD) {
          a = &f->INSN(a->op[1]);
          b = &f->INSN(b->op[1]);

          /* Not in usual form? */
          if (a->opt[1] != b->opt[1] || a->op[1] != b->op[1]
           || a->opt[2] != OPT_CONST || b->opt[2] != OPT_CONST) goto keep;

          /* Check if they touch together */
          if (a->op[2] + aw == b->op[2]
            && REF_BB(f->msched[i - 1]) == REF_BB(f->msched[i])) {
            /* yes => remove second instruction */
            int t1 = MIN (f->msched[i - 1], f->msched[i]);
            int t2 = MAX (f->msched[i - 1], f->msched[i]);
            dep_list *t1dep = f->INSN(t1).dep;
            int x, p;
            cuc_insn *ii;
            
            cucdebug (2, "Joining %x and %x.\n", t1, t2);
            if (cuc_debug >= 8) print_cuc_bb (f, "PREJT");
            change_insn_type (&f->INSN(t1), II_NOP);
            change_insn_type (&f->INSN(t2), II_NOP);
            /* We will reuse the memadd before the first load, and add some
               custom code at the end */
            insert_insns (f, t1, 10);
            if (cuc_debug > 8) print_cuc_bb (f, "PREJT2");
            
            /* Remove all dependencies to second access */
            for (x = 0; x < f->num_bb; x++) {
              int i;
              for (i = 0; i < f->bb[x].ninsn; i++) {
                dep_list *d = f->bb[x].insn[i].dep;
                dep_list **old = &f->bb[x].insn[i].dep;
                while (d) {
                  if (d->ref == t2) {
                    d = d->next;
                    *old = d;
                  } else { 
                    d = d->next;
                    old = &((*old)->next);
                  }
                }
              }
            }

            /* Build the folowing code:
               l[hw]z p-1
               and p-1, 0xff
               sfle p-1, 0x7f
               or p-2, 0xffffff00
               cmov p-3, p-1, p-2
               shr p-5, 8
               and p-1, 0xff
               sfle p-1 0x7f
               or p-2 0xffffff00
               cmov p-3, p-1, p-2*/
            p = REF_I(t1);
            cucdebug (8, "%x %x\n", f->mtype[i - 1], f->mtype[i]);
            for (x = 0; x < 2; x++) {
              int t = f->mtype[i - 1 + x];
              ii = &f->bb[REF_BB(t1)].insn[p];
              if (!x) {
                change_insn_type (ii, aw == 1 ? II_LH : II_LW);
                ii->type = IT_MEMORY | IT_VOLATILE;
                ii->op[0] = -1; ii->opt[0] = OPT_REGISTER | OPT_DEST;
                ii->op[1] = t1 - 1; ii->opt[1] = OPT_REF;
                ii->opt[2] = ii->opt[3] = OPT_NONE;
                ii->dep = t1dep;
                f->mtype[i - 1] = MT_LOAD | (aw == 1 ? 2 : 4);
                f->msched[i - 1] = REF (REF_BB(t1), p);
              } else {
                change_insn_type (ii, II_SRL);
                ii->type = 0;
                ii->op[0] = -1; ii->opt[0] = OPT_REGISTER | OPT_DEST;
                ii->op[1] = t1; ii->opt[1] = OPT_REF;
                ii->op[2] = 8; ii->opt[2] = OPT_CONST;
                ii->opt[3] = OPT_NONE;
              }
              
              ii = &f->bb[REF_BB(t1)].insn[++p];
              change_insn_type (ii, II_AND);
              ii->type = 0;
              ii->op[0] = -1; ii->opt[0] = OPT_REGISTER | OPT_DEST;
              ii->op[1] = REF (REF_BB(t1), p - 1); ii->opt[1] = OPT_REF;
              ii->op[2] = 0xff; ii->opt[2] = OPT_CONST;
              ii->opt[3] = OPT_NONE;

              ii = &f->bb[REF_BB(t1)].insn[++p];
              change_insn_type (ii, II_SFLE);
              ii->type = IT_COND;
              ii->op[0] = -1; ii->opt[0] = OPT_REGISTER | OPT_DEST;
              ii->op[1] = REF (REF_BB(t1), p - 1); ii->opt[1] = OPT_REF;
              ii->op[2] = 0x7f; ii->opt[2] = OPT_CONST;
              ii->opt[3] = OPT_NONE;

              ii = &f->bb[REF_BB(t1)].insn[++p];
              change_insn_type (ii, II_OR);
              ii->type = 0;
              ii->op[0] = -1; ii->opt[0] = OPT_REGISTER | OPT_DEST;
              ii->op[1] = REF (REF_BB(t1), p - 2); ii->opt[1] = OPT_REF;
              if (t & MT_SIGNED) ii->op[2] = 0xffffff00;
              else ii->op[2] = 0;
              ii->opt[2] = OPT_CONST;
              ii->opt[3] = OPT_NONE;

              ii = &f->bb[REF_BB(t1)].insn[++p];
              change_insn_type (ii, II_CMOV);
              ii->type = 0;
              ii->op[0] = -1; ii->opt[0] = OPT_REGISTER | OPT_DEST;
              ii->op[1] = REF (REF_BB(t1), p - 1); ii->opt[1] = OPT_REF;
              ii->op[2] = REF (REF_BB(t1), p - 3); ii->opt[2] = OPT_REF;
              ii->op[3] = REF (REF_BB(t1), p - 2); ii->opt[3] = OPT_REF;
              p++;
            }

            modified = 1;
            
            {
              int b, i, j;
              /* Update references */
              for (b = 0; b < f->num_bb; b++)
                for (i = 0; i < f->bb[b].ninsn; i++)
                  for (j = 0; j < MAX_OPERANDS; j++)
                    if (REF_I (f->bb[b].insn[i].op[j]) < REF_I (t1)
                     || REF_I(f->bb[b].insn[i].op[j]) >= REF_I (t1) + 10) {
                      if (f->bb[b].insn[i].opt[j] & OPT_REF && f->bb[b].insn[i].op[j] == t1)
                        f->bb[b].insn[i].op[j] = t1 + 4;
                      else if (f->bb[b].insn[i].opt[j] & OPT_REF && f->bb[b].insn[i].op[j] == t2)
                        f->bb[b].insn[i].op[j] = t1 + 9;
                    }
            }
            if (cuc_debug >= 8) print_cuc_bb (f, "POSTJT");
          } else goto keep;
        } else goto keep;
      } else {
keep:
        f->msched[j] = f->msched[i];
        f->mtype[j++] = f->mtype[i];
      }
    f->nmsched = j;
  }
  return modified;
}

/* returns nonzero if a < b */
int mem_ordering_cmp (cuc_func *f, cuc_insn *a, cuc_insn *b)
{
  assert (a->type & IT_MEMORY);
  assert (b->type & IT_MEMORY);
  if ((a->opt[1] & OPT_REF) && f->INSN(a->op[1]).index == II_ADD
    &&(b->opt[1] & OPT_REF) && f->INSN(b->op[1]).index == II_ADD) {
    a = &f->INSN(a->op[1]);
    b = &f->INSN(b->op[1]);
    if (a->opt[1] != b->opt[1] || a->op[1] != b->op[1]
     || a->opt[2] != OPT_CONST || b->opt[2] != OPT_CONST) return 0;
    
    /* Order linearly, we can then join them to bursts */
    return a->op[2] < b->op[2];
  } else return 0;
}

/* Schedule memory accesses
  0 - exact; 1 - strong; 2 - weak;  3 - none */
int schedule_memory (cuc_func *f, int otype)
{
  int b, i, j;
  int modified = 0;
  f->nmsched = 0;
  
  for (b = 0; b < f->num_bb; b++) {
    cuc_insn *insn = f->bb[b].insn;
    for (i = 0; i < f->bb[b].ninsn; i++)
      if (insn[i].type & IT_MEMORY) {
        f->msched[f->nmsched++] = REF (b, i);
        if (otype == MO_NONE || otype == MO_WEAK) insn[i].type |= IT_FLAG1; /* mark unscheduled */
      }
  }

  for (i = 0; i < f->nmsched; i++)
    cucdebug (2, "[%x]%x%c ", f->msched[i], f->mtype[i] & MT_WIDTH, (f->mtype[i] & MT_BURST) ? (f->mtype[i] & MT_BURSTE) ? 'E' : 'B' : ' ');
  cucdebug (2, "\n");
  
  /* We can reorder just more loose types
     We assume, that memory accesses are currently in valid (but not neccesserly)
     optimal order */
  if (otype == MO_WEAK || otype == MO_NONE) {
    for (i = 0; i < f->nmsched; i++) {
      int best = i;
      int tmp;
      for (j = i + 1; j < f->nmsched; j++) if (REF_BB(f->msched[j]) == REF_BB(f->msched[best])) {
        if (mem_ordering_cmp (f, &f->INSN (f->msched[j]), &f->INSN(f->msched[best]))) {
          /* Check dependencies */
          dep_list *t = f->INSN(f->msched[j]).dep;
          while (t) {
            if (f->INSN(t->ref).type & IT_FLAG1) break;
            t = t->next;
          }
          if (!t) best = j; /* no conflicts -> ok */
        }
      }
      
      /* we have to shift instructions up, to maintain valid dependencies
         and make space for best candidate */

      /* make local copy */
      tmp = f->msched[best];
      for (j = best; j > i; j--) f->msched[j] = f->msched[j - 1];
      f->msched[i] = tmp;
      f->INSN(f->msched[i]).type &= ~IT_FLAG1; /* mark scheduled */
    }
  }
  
  for (i = 0; i < f->nmsched; i++)
    cucdebug (2, "[%x]%x%c ", f->msched[i], f->mtype[i] & MT_WIDTH, (f->mtype[i] & MT_BURST) ? (f->mtype[i] & MT_BURSTE) ? 'E' : 'B' : ' ');
  cucdebug (2, "\n");
  
  /* Assign memory types */
  for (i = 0; i < f->nmsched; i++) {
    cuc_insn *a = &f->INSN(f->msched[i]);
    f->mtype[i] = !II_IS_LOAD(a->index) ? MT_STORE : MT_LOAD;
    f->mtype[i] |= II_MEM_WIDTH (a->index);
    if (a->type & IT_SIGNED) f->mtype[i] |= MT_SIGNED;
  }

  if (same_transfers (f, otype)) modified = 1;
  if (join_transfers (f, otype)) modified = 1;

  for (i = 0; i < f->nmsched; i++)
    cucdebug (2, "[%x]%x%c ", f->msched[i], f->mtype[i] & MT_WIDTH, (f->mtype[i] & MT_BURST) ? (f->mtype[i] & MT_BURSTE) ? 'E' : 'B' : ' ');
  cucdebug (2, "\n");
  if (cuc_debug > 5) print_cuc_bb (f, "AFTER_MEM_REMOVAL");
  
  if (config.cuc.enable_bursts) {
    //PRINTF ("\n");
    for (i = 1; i < f->nmsched; i++) {
      cuc_insn *a = &f->INSN(f->msched[i - 1]);
      cuc_insn *b = &f->INSN(f->msched[i]);
      int aw = f->mtype[i - 1] & MT_WIDTH;

      /* Burst can only be out of words */
      if (aw != 4) continue;

      if ((a->opt[1] & OPT_REF) && f->INSN(a->op[1]).index == II_ADD
        &&(b->opt[1] & OPT_REF) && f->INSN(b->op[1]).index == II_ADD) {
        a = &f->INSN(a->op[1]);
        b = &f->INSN(b->op[1]);
        /* Not in usual form? */
        if (a->opt[1] != b->opt[1] || a->op[1] != b->op[1]
         || a->opt[2] != OPT_CONST || b->opt[2] != OPT_CONST) continue; 

        //PRINTF ("%i %i, ", a->op[2], b->op[2]);
        
        /* Check if they touch together */
        if (a->op[2] + aw == b->op[2]
          && REF_BB(f->msched[i - 1]) == REF_BB(f->msched[i])) {
          /* yes => do burst */
          f->mtype[i - 1] &= ~MT_BURSTE;
          f->mtype[i - 1] |= MT_BURST;
          f->mtype[i] |= MT_BURST | MT_BURSTE;
        }
      }
    }
  }

  for (i = 0; i < f->nmsched; i++)
    cucdebug (2, "[%x]%x%c ", f->msched[i], f->mtype[i] & MT_WIDTH, (f->mtype[i] & MT_BURST) ? (f->mtype[i] & MT_BURSTE) ? 'E' : 'B' : ' ');
  cucdebug (2, "\n");
  
  /* We don't need dependencies in non-memory instructions */
  for (b = 0; b < f->num_bb; b++) {
    cuc_insn *insn = f->bb[b].insn;
    for (i = 0; i < f->bb[b].ninsn; i++) if (!(insn[i].type & IT_MEMORY))
      dispose_list (&insn[i].dep);
  }

  if (cuc_debug > 5) print_cuc_bb (f, "AFTER_MEM_REMOVAL2");
  /* Reduce number of dependecies, keeping just direct dependencies, based on memory schedule */
  {
    int lastl[3] = {-1, -1, -1};
    int lasts[3] = {-1, -1, -1};
    int lastc[3] = {-1, -1, -1};
    int last_load = -1, last_store = -1, last_call = -1;
    for (i = 0; i < f->nmsched; i++) {
      int t = f->mtype[i] & MT_LOAD ? 0 : f->mtype[i] & MT_STORE ? 1 : 2;
      int maxl = lastl[t];
      int maxs = lasts[t];
      int maxc = lastc[t];
      dep_list *tmp = f->INSN(f->msched[i]).dep;
      cucdebug (7, "!%i %x %p\n", i, f->msched[i], tmp);
      while (tmp) {
        if (f->INSN(tmp->ref).type & IT_MEMORY && REF_BB(tmp->ref) == REF_BB(f->msched[i])) {
          cucdebug (7, "%i %x %lx\n", i, f->msched[i], tmp->ref);
          /* Search for the reference */
          for (j = 0; j < f->nmsched; j++) if (f->msched[j] == tmp->ref) break;
          assert (j < f->nmsched);
          if (f->mtype[j] & MT_STORE) {
            if (maxs < j) maxs = j;
          } else if (f->mtype[j] & MT_LOAD) {
            if (maxl < j) maxl = j;
          } else if (f->mtype[j] & MT_CALL) {
            if (maxc < j) maxc = j;
          }
        }
        tmp = tmp->next;
      }
      dispose_list (&f->INSN(f->msched[i]).dep);
      if (f->mtype[i] & MT_STORE) {
        maxs = last_store;
        last_store = i;
      } else if (f->mtype[i] & MT_LOAD) {
        maxl = last_load;
        last_load = i;
      } else if (f->mtype[i] & MT_CALL) {
        maxc = last_call;
        last_call = i;
      }
      
      if (maxl > lastl[t]) {
        add_dep (&f->INSN(f->msched[i]).dep, f->msched[maxl]);
        lastl[t] = maxl;
      }
      if (maxs > lasts[t]) {
        add_dep (&f->INSN(f->msched[i]).dep, f->msched[maxs]);
        lasts[t] = maxs;
      }
      if (maxc > lastc[t]) {
        add_dep (&f->INSN(f->msched[i]).dep, f->msched[maxc]);
        lastc[t] = maxc;
      }
      //PRINTF ("%i(%i)> ml %i(%i) ms %i(%i) lastl %i %i lasts %i %i last_load %i last_store %i\n", i, f->msched[i], maxl, f->msched[maxl], maxs, f->msched[maxs], lastl[0], lastl[1], lasts[0], lasts[1], last_load, last_store);
      
      /* What we have to wait to finish this BB? */
      if (i + 1 >= f->nmsched || REF_BB(f->msched[i + 1]) != REF_BB(f->msched[i])) {
        if (last_load > lastl[t]) {
          add_dep (&f->bb[REF_BB(f->msched[i])].mdep, f->msched[last_load]);
          lastl[t] = last_load;
        }
        if (last_store > lasts[t]) {
          add_dep (&f->bb[REF_BB(f->msched[i])].mdep, f->msched[last_store]);
          lasts[t] = last_store;
        }
        if (last_call > lastc[t]) {
          add_dep (&f->bb[REF_BB(f->msched[i])].mdep, f->msched[last_call]);
          lastc[t] = last_call;
        }
      }
    }
  }
  return modified;
}
