/* timings.c -- OpenRISC Custom Unit Compiler, timing and size estimation
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
#include <math.h>

#include "config.h"

#include "port.h"
#include "arch.h"
#include "abstract.h"
#include "sim-config.h"
#include "cuc.h"
#include "insn.h"

static cuc_timing_table *timing_table;
static double max_bb_delay;

/* Returns instruction delay */
double insn_time (cuc_insn *ii)
{
  if (ii->opt[2] & OPT_CONST) {
    if (ii->opt[1] & OPT_CONST) return 0.;
    else return timing_table[ii->index].delayi;
  } else return timing_table[ii->index].delay;
}

/* Returns instruction size */
double insn_size (cuc_insn *ii)
{
  double s = (ii->opt[2] & OPT_CONST) ? timing_table[ii->index].sizei
          : timing_table[ii->index].size;
  if (ii->opt[1] & OPT_CONST) return 0.;
  if (ii->type & IT_COND && (ii->index == II_CMOV || ii->index == II_ADD)) return s / 32.;
  else return s;
}

/* Returns normal instruction size */
double ii_size (int index, int imm)
{
  if (imm) return timing_table[index].sizei;
  else return timing_table[index].size;
}

/* Returns dataflow tree height in cycles */
static double max_delay (cuc_func *f, int b)
{
  double max_d = 0.;
  double *d;
  cuc_bb *bb = &f->bb[b];
  int i, j;
  d = (double *) malloc (sizeof (double) * bb->ninsn);
  for (i = 0; i < bb->ninsn; i++) {
    double md = 0.;
    for (j = 0; j < MAX_OPERANDS; j++) {
      int op = bb->insn[i].op[j];
      if (bb->insn[i].opt[j] & OPT_REF && op >= 0 && REF_BB (op) == b && REF_I (op) < i) {
        double t = d[REF_I (op)];
        if (t > md) md = t;
      }
    }
    d[i] = md + insn_time (&bb->insn[i]);
    if (d[i] > max_d) max_d = d[i];
  }
  free (d);
  //PRINTF ("max_d%i=%f\n", b, max_d);
  return max_d;
}

/* Calculates memory delay of a single run of a basic block */
static int memory_delay (cuc_func *f, int b)
{
  int i;
  int d = 0;
  for (i = 0; i < f->nmsched; i++)
    if (REF_BB (f->msched[i]) == b) {
      if (f->mtype[i] & MT_STORE) {
        if (!(f->mtype[i] & MT_BURST) || f->mtype[i] & MT_BURSTE) d += runtime.cuc.mdelay[2];
        else d += runtime.cuc.mdelay[3];
      } else if (f->mtype[i] & MT_LOAD) {
        if (!(f->mtype[i] & MT_BURST) || f->mtype[i] & MT_BURSTE) d += runtime.cuc.mdelay[0];
        else d += runtime.cuc.mdelay[1];
      }
    }
  //PRINTF ("md%i=%i\n", b, d);
  return d;
}

/* Cuts the tree and marks registers */
void cut_tree (cuc_func *f, int b, double sd)
{
  int i, j;
  double *depths;
  cuc_bb *bb = &f->bb[b];
  depths = (double *) malloc (sizeof (double) * bb->ninsn);

  for (i = 0; i < bb->ninsn; i++) {
    double md = 0.;
    int mg = 0;
    for (j = 0; j < MAX_OPERANDS; j++) {
      int op = bb->insn[i].op[j];
      if (bb->insn[i].opt[j] & OPT_REF && op >= 0 && REF_BB (op) == b && REF_I (op) < i) {
        double t = depths[REF_I (op)];
        if (f->INSN(op).type & IT_CUT) {
          if (f->INSN(op).tmp + 1 >= mg) {
            if (f->INSN(op).tmp + 1 > mg) md = 0.;
            mg = f->INSN(op).tmp + 1;
            if (t > md) md = t;
          }
        } else {
          if (f->INSN(op).tmp >= mg) {
            if (f->INSN(op).tmp > mg) md = 0.;
            mg = f->INSN(op).tmp;
            if (t > md) md = t;
          }
        }
      }
    }
    //PRINTF ("%2x md%.1f ", i, md);
    md += insn_time (&bb->insn[i]);
    //PRINTF ("md%.1f mg%i %.1f\n", md, mg, sd);
    bb->insn[i].tmp = mg;
    if (md > sd) {
      bb->insn[i].type |= IT_CUT;
      if (md > runtime.cuc.cycle_duration)
        log ("WARNING: operation t%x_%x may need to be registered inbetween\n", b, i);
      depths[i] = 0.;
    } else depths[i] = md;
  }
  free (depths);
}

/* How many cycles we need now to get through the BB */
static int new_bb_cycles (cuc_func *f, int b, int cut)
{
  long d;
  double x = max_delay (f, b);
  d = ceil (x / runtime.cuc.cycle_duration);
  if (d < 1) d = 1;
  if (cut && x > runtime.cuc.cycle_duration) cut_tree (f, b, x / d);

  if (x / d > max_bb_delay) max_bb_delay = x / d;

  return memory_delay (f, b) + d;
}

/* Cuts the tree and marks registers */
void mark_cut (cuc_func *f)
{
  int b, i;
  for (b = 0; b < f->num_bb; b++)
    for (i = 0; i < f->bb[b].ninsn; i++)
      f->bb[b].insn[i].tmp = 0; /* Set starting groups */
  if (config.cuc.no_multicycle)
    for (b = 0; b < f->num_bb; b++)
      new_bb_cycles (f, b, 1);
}

/* Returns basic block circuit area */
static double bb_size (cuc_bb *bb)
{
  int i;
  double d = 0.;
  for (i = 0; i < bb->ninsn; i++) {
    if (bb->insn[i].opt[2] & OPT_CONST)
      d = d + timing_table[bb->insn[i].index].sizei;
    else d = d + timing_table[bb->insn[i].index].size;
  }
  return d;
}

/* Recalculates bb[].cnt values, based on generated profile file */
void recalc_cnts (cuc_func *f, char *bb_filename)
{
  int i, r, b, prevbb = -1, prevcnt = 0;
  int buf[256];
  const int bufsize = 256;
  FILE *fi = fopen (bb_filename, "rb");
  
  assert (fi);
  
  /* initialize counts */
  for (b = 0; b < f->num_bb; b++) f->bb[b].cnt = 0;
  
  /* read control flow from file and set counts */
  do {
    r = fread (buf, sizeof (int), bufsize, fi); 
    for (i = 0; i < r; i++) {
      b = f->init_bb_reloc[buf[i]];
      if (b < 0) continue;
      /* Were we in the loop? */
      if (b == prevbb) {
        prevcnt++;
      } else {
        /* End the block */
        if (prevbb >= 0 && prevbb != BBID_START)
          f->bb[prevbb].cnt += prevcnt / f->bb[prevbb].unrolled + 1;
        prevcnt = 0;
        prevbb = b;
      }
    }
  } while (r == bufsize);

  fclose (fi);
}

/* Analizes current version of design and places results into timings structure */
void analyse_timings (cuc_func *f, cuc_timings *timings)
{
  long new_time = 0;
  double size = 0.;
  int b, i;
  
  /* Add time needed for mtspr/mfspr */
  for (i = 0; i < MAX_REGS; i++) if (f->used_regs[i]) new_time++;
  new_time++; /* always one mfspr at the end */
  new_time *= f->num_runs;

  max_bb_delay = 0.;
  for (b = 0; b < f->num_bb; b++) {
    new_time += new_bb_cycles (f, b, 0) * f->bb[b].cnt;
    size = size + bb_size (&f->bb[b]);
  }
  timings->new_time = new_time;
  timings->size = size;
  log ("Max circuit delay %.2fns; max circuit clock speed %.1fMHz\n",
                  max_bb_delay, 1000. / max_bb_delay);
}

/* Loads in the specified timings table */
void load_timing_table (char *filename)
{
  int i;
  FILE *fi;

  log ("Loading timings from %s\n", filename);
  log ("Using clock delay %.2fns (frequency %.0fMHz)\n", runtime.cuc.cycle_duration,
                 1000. / runtime.cuc.cycle_duration);
  assert (fi = fopen (filename, "rt"));

  timing_table = (cuc_timing_table *)malloc ((II_LAST + 1) * sizeof (cuc_timing_table));
  assert (timing_table);
  for (i = 0; i <= II_LAST; i++) {
    timing_table[i].size = -1.;
    timing_table[i].sizei = -1.;
    timing_table[i].delay = -1.;
    timing_table[i].delayi = -1.;
  }

  while (!feof(fi)) {
    char tmp[256];
    int index = 0;
    if (fscanf (fi, "%s", tmp) != 1) break;
    if (tmp[0] == '#') {
      while (!feof (fi) && fgetc (fi) != '\n');
      continue;
    }
    for (i = 0; i <= II_LAST; i++)
      if (strcmp (known[i].name, tmp) == 0) {
        index = i;
        break;
      }
    assert (index <= II_LAST);
    i = index;
    if (fscanf (fi, "%lf%lf%lf%lf\n", &timing_table[i].size,
                &timing_table[i].sizei, &timing_table[i].delay, &timing_table[i].delayi) != 4) break;
    /*PRINTF ("!%s size %f,%f delay %f,%f\n", known[i].name, timing_table[i].size,
                    timing_table[i].sizei, timing_table[i].delay, timing_table[i].delayi);*/
  }

  /* Was everything initialized? */
  for (i = 0; i <= II_LAST; i++) {
    assert (timing_table[i].size >= 0 && timing_table[i].sizei >= 0
     && timing_table[i].delay >= 0 && timing_table[i].delayi >= 0);
    /*PRINTF ("%s size %f,%f delay %f,%f\n", known[i], timing_table[i].size,
                    timing_table[i].sizei, timing_table[i].delay, timing_table[i].delayi);*/
  }
  
  fclose (fi);
}

