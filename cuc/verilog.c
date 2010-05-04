/* verilog.c -- OpenRISC Custom Unit Compiler, verilog generator

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


/* Autoconf and/or portability configuration */
#include "config.h"
#include "port.h"

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>

/* Package includes */
#include "arch.h"
#include "abstract.h"
#include "cuc.h"
#include "insn.h"
#include "profiler.h"
#include "sim-config.h"
#include "misc.h"

/* Shortcut */
#define GEN(x...) fprintf (fo, x)

/* Find index of load/store/call */
static int
find_lsc_index (cuc_func * f, int ref)
{
  int c = 0;
  int i;
  int load;

  if (f->INSN (ref).index == II_CALL)
    {
      for (i = 0; i < f->nmsched; i++)
	{
	  if (f->msched[i] == ref)
	    break;
	  if (f->mtype[i] & MT_CALL)
	    c++;
	}
    }
  else
    {
      load = II_IS_LOAD (f->INSN (ref).index);
      for (i = 0; i < f->nmsched; i++)
	{
	  if (f->msched[i] == ref)
	    break;
	  if ((load && (f->mtype[i] & MT_LOAD))
	      || (!load && (f->mtype[i] & MT_STORE)))
	    c++;
	}
    }
  return c;
}

/* Print out dependencies as verilog expression */
static void
print_deps (FILE * fo, cuc_func * f, int b, dep_list * t, int registered)
{
  if (t)
    {
      int first = 0;
      while (t)
	{
	  if (f->INSN (t->ref).type & IT_MEMORY)
	    {
	      GEN ("%s%c_end[%i]", first ? " && " : "",
		   II_IS_LOAD (f->INSN (t->ref).index) ? 'l' : 's',
		   find_lsc_index (f, t->ref));
	    }
	  else if (f->INSN (t->ref).index == II_CALL)
	    {
	      GEN ("%sf_end[%i]", first ? " && " : "",
		   find_lsc_index (f, t->ref));
	    }
	  else
	    {
	      PRINTF ("print_deps: err %lx\n", t->ref);
	      assert (0);
	    }
	  first = 1;
	  t = t->next;
	}
    }
  else
    {
      if (registered)
	GEN ("bb_start_r[%i]", b);
      else
	GEN ("bb_start[%i]", b);
    }
}

static char *
print_op_v (cuc_func * f, char *s, int ref, int j)
{
  unsigned long op = f->INSN (ref).op[j];
  unsigned long opt = f->INSN (ref).opt[j];
  switch (opt & ~OPT_DEST)
    {
    case OPT_NONE:
      assert (0);
      break;
    case OPT_CONST:
      if (f->INSN (ref).type & IT_COND && (f->INSN (ref).index == II_CMOV
					   || f->INSN (ref).index == II_ADD))
	{
	  assert (op == 0 || op == 1);
	  sprintf (s, "1'b%lx", op);
	}
      else
	sprintf (s, "32'h%lx", op);
      break;
    case OPT_REGISTER:
      if (opt & OPT_DEST)
	sprintf (s, "t%x_%x", REF_BB (ref), REF_I (ref));
      else
	sprintf (s, "r%li_%c", op, opt & OPT_DEST ? 'o' : 'i');
      break;
#if 0
    case OPT_FREG:
      assert (opt & OPT_DEST);
      sprintf (s, "fr%i_o", op);
      break;
#endif
    case OPT_REF:
      sprintf (s, "t%lx_%lx", REF_BB (op), REF_I (op));
      break;
    }
  return s;
}

/* Prints out specified instruction */
static void
print_insn_v (FILE * fo, cuc_func * f, int b, int i)
{
  cuc_insn *ii = &f->bb[b].insn[i];
  char *s = known[ii->index].rtl;
  char tmp[200] = "";

  assert (s);
  while (*s)
    {
      if (*s <= MAX_OPERANDS)
	{
	  char t[30];
	  sprintf (tmp, "%s%s", tmp, print_op_v (f, t, REF (b, i), *s - 1));
	}
      else if (*s == '\b')
	sprintf (tmp, "%s%i", tmp, b);
      else
	sprintf (tmp, "%s%c", tmp, *s);
      s++;
    }
  GEN ("%-40s /* %s */\n", tmp, ii->disasm);
  if (ii->type & IT_MEMORY)
    {
      int nls = find_lsc_index (f, REF (b, i));
      if (II_IS_LOAD (ii->index))
	{
	  int nm;
	  for (nm = 0; nm < f->nmsched; nm++)
	    if (f->msched[nm] == REF (b, i))
	      break;
	  assert (nm < f->nmsched);

	  GEN ("  if (l_end[%i]) t%x_%x <= #Tp ", nls, b, i);
	  switch (f->mtype[nm] & (MT_WIDTH | MT_SIGNED))
	    {
	    case 1:
	      GEN ("l_dat_i & 32'hff;\n");
	      break;
	    case 2:
	      GEN ("l_dat_i & 32'hffff;\n");
	      break;
	    case 4 | MT_SIGNED:
	    case 4:
	      GEN ("l_dat_i;\n");
	      break;
	    case 1 | MT_SIGNED:
	      GEN ("{24{l_dat_i[7]}, l_dat_i[7:0]};\n");
	      break;
	    case 2 | MT_SIGNED:
	      GEN ("{16{l_dat_i[15]}, l_dat_i[15:0]};\n");
	      break;
	    default:
	      assert (0);
	    }
	}
    }
  else if (ii->index == II_LRBB)
    {
      GEN ("  if (rst) t%x_%x <= #Tp 1'b0;\n", b, i);
      assert (f->bb[b].prev[0] >= 0);
      if (f->bb[b].prev[0] == BBID_START)
	GEN ("  else if (bb_start[%i]) t%x_%x <= #Tp start_i;\n", b, b, i);
      else
	GEN ("  else if (bb_start[%i]) t%x_%x <= #Tp bb_stb[%i];\n", b, b, i,
	     f->bb[b].prev[0]);
    }
  else if (ii->index == II_REG)
    {
      assert (ii->opt[1] == OPT_REF);
      GEN ("  if (");
      if (f->bb[b].mdep)
	print_deps (fo, f, b, f->bb[b].mdep, 0);
      else
	GEN ("bb_stb[%i]", b);
      GEN (") t%x_%x <= #Tp t%lx_%lx;\n", b, i,
	   REF_BB (ii->op[1]), REF_I (ii->op[1]));
    }
}

/* Outputs binary number */
/*
static char *bin_str (unsigned long x, int len)
{
  static char bx[33];
  char *s = bx;
  while (len > 0) *s++ = '0' + ((x >> --len) & 1);
  *s = '\0';
  return bx;
}
*/

/* Returns index of branch instruction inside a block b */
static int
branch_index (cuc_bb * bb)
{
  int i;
  for (i = bb->ninsn - 1; i >= 0; i--)
    if (bb->insn[i].type & IT_BRANCH)
      return i;
  return -1;
}

static void
print_turn_off_dep (FILE * fo, cuc_func * f, dep_list * dep)
{
  while (dep)
    {
      assert (f->INSN (dep->ref).type & IT_MEMORY
	      || f->INSN (dep->ref).index == II_CALL);
      GEN ("      %c_stb[%i] <= #Tp 1'b0;\n",
	   f->INSN (dep->ref).index ==
	   II_CALL ? 'f' : II_IS_LOAD (f->INSN (dep->ref).index) ? 'l' : 's',
	   find_lsc_index (f, dep->ref));
      dep = dep->next;
    }
}

static int
func_index (cuc_func * f, int ref)
{
  int i;
  unsigned long addr;
  assert (f->INSN (ref).index == II_CALL && f->INSN (ref).opt[0] & OPT_CONST);
  addr = f->INSN (ref).op[0];
  for (i = 0; i < f->nfdeps; i++)
    if (f->fdeps[i]->start_addr == addr)
      return i;

  assert (0);
  return -1;
}

/* Generates verilog file out of insn dataflow */
void
output_verilog (cuc_func * f, char *filename, char *funcname)
{
  FILE *fo;
  int b, i, j;
  int ci = 0, co = 0;
  int nloads = 0, nstores = 0, ncalls = 0;
  char tmp[256];
  sprintf (tmp, "%s.v", filename);

  log ("Generating verilog file \"%s\"\n", tmp);
  PRINTF ("Generating verilog file \"%s\"\n", tmp);
  if ((fo = fopen (tmp, "wt+")) == NULL)
    {
      fprintf (stderr, "Cannot open '%s'\n", tmp);
      exit (1);
    }

  /* output header */
  GEN ("/* %s -- generated by Custom Unit Compiler\n", tmp);
  GEN ("   (C) 2002 Opencores\n");
  GEN ("   function   \"%s\"\n", funcname);
  GEN ("   at         %08lx - %08lx\n", f->start_addr, f->end_addr);
  GEN ("   num BBs    %i */\n\n", f->num_bb);

  GEN ("`include \"timescale.v\"\n\n");
  GEN ("module %s (clk, rst,\n", filename);
  GEN ("              l_adr_o, l_dat_i, l_req_o,\n");
  GEN ("              l_sel_o, l_linbrst_o, l_rdy_i,\n");
  GEN ("              s_adr_o, s_dat_o, s_req_o,\n");
  GEN ("              s_sel_o, s_linbrst_o, s_rdy_i,\n");

  GEN ("/* inputs */  ");
  for (i = 0; i < MAX_REGS; i++)
    if (f->used_regs[i])
      {
	GEN ("r%i_i, ", i);
	ci++;
      }
  if (!ci)
    GEN ("/* NONE */");

  GEN ("\n/* outputs */ ");
  for (i = 0; i < MAX_REGS; i++)
    if (f->lur[i] >= 0 && !f->saved_regs[i])
      {
	GEN ("r%i_o, ", i);
	co++;
      }

  if (!co)
    GEN ("/* NONE */");
  if (f->nfdeps)
    {
      GEN ("\n/* f. calls */, fstart_o, %sfend_i, fr11_i, ",
	   log2_int (f->nfdeps) > 0 ? "fid_o, " : "");
      for (i = 0; i < 6; i++)
	GEN ("fr%i_o, ", i + 3);
    }
  GEN ("\n              start_i, end_o, busy_o);\n\n");

  GEN ("parameter Tp = 1;\n\n");

  GEN ("input         clk, rst;\n");
  GEN ("input         start_i;\t/* Module starts when set to 1 */ \n");
  GEN
    ("output        end_o;\t/* Set when module finishes, cleared upon start_i == 1 */\n");
  GEN
    ("output        busy_o;\t/* Set when module should not be interrupted */\n");
  GEN ("\n/* Bus signals */\n");
  GEN ("output        l_req_o, s_req_o;\n");
  GEN ("input         l_rdy_i, s_rdy_i;\n");
  GEN ("output  [3:0] l_sel_o, s_sel_o;\n");
  GEN ("output [31:0] l_adr_o, s_adr_o;\n");
  GEN ("output        l_linbrst_o, s_linbrst_o;\n");
  GEN ("input  [31:0] l_dat_i;\n");
  GEN ("output [31:0] s_dat_o;\n\n");

  GEN ("reg           l_req_o, s_req_o;\n");
  GEN ("reg    [31:0] l_adr_o, s_adr_o;\n");
  GEN ("reg     [3:0] l_sel_o, s_sel_o;\n");
  GEN ("reg    [31:0] s_dat_o;\n");
  GEN ("reg           l_linbrst_o, s_linbrst_o;\n");

  if (ci || co)
    GEN ("\n/* module ports */\n");
  if (ci)
    {
      int first = 1;
      GEN ("input  [31:0]");
      for (i = 0; i < MAX_REGS; i++)
	if (f->used_regs[i])
	  {
	    GEN ("%sr%i_i", first ? " " : ", ", i);
	    first = 0;
	  }
      GEN (";\n");
    }

  if (co)
    {
      int first = 1;
      GEN ("output [31:0]");
      for (i = 0; i < MAX_REGS; i++)
	if (f->lur[i] >= 0 && !f->saved_regs[i])
	  {
	    GEN ("%sr%i_o", first ? " " : ", ", i);
	    first = 0;
	  }
      GEN (";\n");
    }

  if (f->nfdeps)
    {
      GEN ("\n/* Function calls */\n");
      GEN ("output [31:0] fr3_o");
      for (i = 1; i < 6; i++)
	GEN (", fr%i_o", i + 3);
      GEN (";\n");
      GEN ("input  [31:0] fr11_i;\n");
      if (log2_int (f->nfdeps) > 0)
	GEN ("output [%i:0] fid_o;\n", log2_int (f->nfdeps));
      GEN ("output        fstart_o;\n");
      GEN ("input         fend_i;\n");
    }

  /* Count loads & stores */
  for (i = 0; i < f->nmsched; i++)
    if (f->mtype[i] & MT_STORE)
      nstores++;
    else if (f->mtype[i] & MT_LOAD)
      nloads++;
    else if (f->mtype[i] & MT_CALL)
      ncalls++;

  /* Output internal registers for loads */
  if (nloads)
    {
      int first = 1;
      int num = 0;
      GEN ("\n/* internal registers for loads */\n");
      for (i = 0; i < f->nmsched; i++)
	if (f->mtype[i] & MT_LOAD)
	  {
	    GEN ("%st%x_%x", first ? "reg    [31:0] " : ", ",
		 REF_BB (f->msched[i]), REF_I (f->msched[i]));

	    if (num >= 8)
	      {
		GEN (";\n");
		first = 1;
		num = 0;
	      }
	    else
	      {
		first = 0;
		num++;
	      }
	  }
      if (!first)
	GEN (";\n");
    }

  /* Internal register for function return value */
  if (f->nfdeps)
    {
      GEN ("\n/* Internal register for function return value */\n");
      GEN ("reg     [31:0] fr11_r;\n");
    }

  GEN ("\n/* 'zero or one' hot state machines */\n");
  if (nloads)
    GEN ("reg     [%i:0] l_stb; /* loads */\n", nloads - 1);
  if (nstores)
    GEN ("reg     [%i:0] s_stb; /* stores */\n", nstores - 1);
  GEN ("reg     [%i:0] bb_stb; /* basic blocks */\n", f->num_bb - 1);

  {
    int first = 2;
    int num = 0;
    for (b = 0; b < f->num_bb; b++)
      for (i = 0; i < f->bb[b].ninsn; i++)
	if (f->bb[b].insn[i].type & IT_COND
	    && f->bb[b].insn[i].index != II_REG
	    && f->bb[b].insn[i].index != II_LRBB)
	  {
	    if (first == 2)
	      GEN ("\n/* basic block condition wires */\n");
	    GEN ("%st%x_%x", first ? "wire          " : ", ", b, i);
	    if (num >= 8)
	      {
		GEN (";\n");
		first = 1;
		num = 0;
	      }
	    else
	      {
		first = 0;
		num++;
	      }
	  }
    if (!first)
      GEN (";\n");

    GEN ("\n/* forward declaration of normal wires */\n");
    num = 0;
    first = 1;
    for (b = 0; b < f->num_bb; b++)
      for (i = 0; i < f->bb[b].ninsn; i++)
	if (!(f->bb[b].insn[i].type & (IT_COND | IT_BRANCH))
	    && f->bb[b].insn[i].index != II_REG
	    && f->bb[b].insn[i].index != II_LRBB)
	  {
	    /* Exclude loads */
	    if (f->bb[b].insn[i].type & IT_MEMORY
		&& II_IS_LOAD (f->bb[b].insn[i].index))
	      continue;
	    GEN ("%st%x_%x", first ? "wire   [31:0] " : ", ", b, i);
	    if (num >= 8)
	      {
		GEN (";\n");
		first = 1;
		num = 0;
	      }
	    else
	      {
		first = 0;
		num++;
	      }
	  }
    if (!first)
      GEN (";\n");

    GEN ("\n/* forward declaration registers */\n");
    num = 0;
    first = 1;
    for (b = 0; b < f->num_bb; b++)
      for (i = 0; i < f->bb[b].ninsn; i++)
	if (f->bb[b].insn[i].index == II_REG
	    && f->bb[b].insn[i].index != II_LRBB)
	  {
	    GEN ("%st%x_%x", first ? "reg    [31:0] " : ", ", b, i);
	    if (num >= 8)
	      {
		GEN (";\n");
		first = 1;
		num = 0;
	      }
	    else
	      {
		first = 0;
		num++;
	      }
	  }
    if (!first)
      GEN (";\n");

    num = 0;
    first = 1;
    for (b = 0; b < f->num_bb; b++)
      for (i = 0; i < f->bb[b].ninsn; i++)
	if (f->bb[b].insn[i].index != II_REG
	    && f->bb[b].insn[i].index == II_LRBB)
	  {
	    GEN ("%st%x_%x", first ? "reg           " : ", ", b, i);
	    if (num >= 8)
	      {
		GEN (";\n");
		first = 1;
		num = 0;
	      }
	    else
	      {
		first = 0;
		num++;
	      }
	  }
    if (!first)
      GEN (";\n");
  }

  if (nloads || nstores)
    GEN ("\n/* dependencies */\n");
  if (nloads)
    GEN ("wire    [%i:0] l_end = l_stb & {%i{l_rdy_i}};\n",
	 nloads - 1, nloads);
  if (nstores)
    GEN ("wire    [%i:0] s_end = s_stb & {%i{s_rdy_i}};\n",
	 nstores - 1, nstores);
  if (ncalls)
    GEN ("wire    [%i:0] f_end = f_stb & {%i{fend_i}};\n",
	 ncalls - 1, ncalls);

  GEN ("\n/* last dependency */\n");
  GEN ("wire   end_o = ");
  for (b = 0; b < f->num_bb; b++)
    {
      for (i = 0; i < 2; i++)
	if (f->bb[b].next[i] == BBID_END)
	  {
	    GEN ("bb_stb[%i]", b);
	    if (f->bb[b].mdep)
	      {
		GEN (" && ");
		print_deps (fo, f, b, f->bb[b].mdep, 0);
	      }
	    /* Is branch to BBID_END conditional? */
	    if (f->bb[b].next[1 - i] >= 0)
	      {
		int bidx = branch_index (&f->bb[b]);
		char t[30];
		print_op_v (f, t, REF (b, bidx), 1);
		GEN (" && %s%s", i ? "" : "!", t);
	      }
	  }
    }
  GEN (";\n");
  GEN ("wire   busy_o = |bb_stb;\n");


  GEN ("\n/* Basic block triggers */\n");
  GEN ("wire   [%2i:0] bb_start = {\n", f->num_bb - 1);
  for (b = f->num_bb - 1; b >= 0; b--)
    {
      GEN ("    /* bb_start[%2i] */ ", b);
      for (i = 0; i < 2; i++)
	if (f->bb[b].prev[i] >= 0 && f->bb[b].prev[i] != BBID_START)
	  {
	    cuc_bb *prev = &f->bb[f->bb[b].prev[i]];
	    int t;
	    if (i)
	      GEN ("\n                    || ");
	    if (prev->mdep)
	      {
		print_deps (fo, f, f->bb[b].prev[i], prev->mdep, 0);
		GEN (" && ");
	      }
	    GEN ("bb_stb[%i]", f->bb[b].prev[i]);
	    if (prev->next[0] >= 0 && prev->next[0] != BBID_END
		&& prev->next[1] >= 0 && prev->next[1] != BBID_END)
	      {
		int bi =
		  REF (f->bb[b].prev[i],
		       branch_index (&f->bb[f->bb[b].prev[i]]));
		int ci;
		assert (bi >= 0);
		ci = f->INSN (bi).op[1];
		t = prev->next[0] == b;
		GEN (" && ");
		if (f->INSN (bi).opt[1] & OPT_REF)
		  {
		    GEN ("%st%x_%x", t ? "" : "!", REF_BB (ci), REF_I (ci));
		  }
		else
		  {
		    cucdebug (5, "%x!%x!%x\n", bi, ci, f->INSN (bi).opt[1]);
		    assert (f->INSN (bi).opt[1] & OPT_CONST);
		    GEN ("%s%i", t ? "" : "!", ci);
		  }
	      }
	  }
	else
	  break;
      if (!i)
	GEN ("start_i");
      if (b == 0)
	GEN ("};\n");
      else
	GEN (",\n");
    }

  GEN ("\n/* Register the bb_start */\n");
  GEN ("reg   [%2i:0] bb_start_r;\n\n", f->num_bb - 1);
  GEN ("always @(posedge rst or posedge clk)\n");
  GEN ("begin\n");
  GEN ("  if (rst) bb_start_r <= #Tp %i'b0;\n", f->num_bb);
  GEN ("  else if (end_o) bb_start_r <= #Tp %i'b0;\n", f->num_bb);
  GEN ("  else bb_start_r <= #Tp bb_start;\n");
  GEN ("end\n");

  GEN ("\n/* Logic */\n");
  /* output body */
  for (b = 0; b < f->num_bb; b++)
    {
      GEN ("\t\t/* BB%i */\n", b);
      for (i = 0; i < f->bb[b].ninsn; i++)
	print_insn_v (fo, f, b, i);
      GEN ("\n");
    }

  if (co)
    {
      GEN ("\n/* Outputs */\n");
      for (i = 0; i < MAX_REGS; i++)
	if (f->lur[i] >= 0 && !f->saved_regs[i])
	  GEN ("assign r%i_o = t%x_%x;\n", i, REF_BB (f->lur[i]),
	       REF_I (f->lur[i]));
    }

  if (nstores)
    {
      int cur_store;
      GEN ("\n/* Memory stores */\n");
      GEN ("always @(s_stb");
      for (i = 0; i < f->nmsched; i++)
	if (f->mtype[i] & MT_STORE)
	  {
	    char t[30];
	    unsigned long opt = f->INSN (f->msched[i]).opt[0];
	    if ((opt & ~OPT_DEST) != OPT_CONST)
	      {
		GEN (" or %s", print_op_v (f, t, f->msched[i], 0));
	      }
	  }

      cur_store = 0;
      GEN (")\nbegin\n");
      for (i = 0; i < f->nmsched; i++)
	if (f->mtype[i] & MT_STORE)
	  {
	    char t[30];
	    GEN ("  %sif (s_stb[%i]) s_dat_o = %s;\n",
		 cur_store == 0 ? "" : "else ", cur_store, print_op_v (f, t,
								       f->
								       msched
								       [i],
								       0));
	    cur_store++;
	    //PRINTF ("msched[%i] = %x (mtype %x) %x\n", i, f->msched[i], f->mtype[i], f->INSN(f->msched[i]).op[0]);
	  }
      GEN ("  else s_dat_o = 32'hx;\n");
      GEN ("end\n");
    }

  /* Generate load and store state machine */
#if 0
  GEN ("\n/* Load&store state machine */\n");
  GEN ("always @(posedge clk or posedge rst)\n");
  GEN ("  if (rst) begin\n");
  if (nloads)
    GEN ("    l_stb <= #Tp %i'h0;\n", nloads);
  if (nstores)
    GEN ("    s_stb <= #Tp %i'h0;\n", nstores);
  GEN ("  end else begin\n");
  for (i = 0; i < f->nmsched; i++)
    if (f->mtype[i] & MT_LOAD || f->mtype[i] & MT_STORE)
      {
	int cur = 0;
	dep_list *dep = f->INSN (f->msched[i]).dep;
	assert (f->INSN (f->msched[i]).opt[1] & (OPT_REF | OPT_REGISTER));
	GEN ("    if (");
	print_deps (fo, f, REF_BB (f->msched[i]), f->INSN (f->msched[i]).dep,
		    1);
	GEN (") begin\n");
	print_turn_off_dep (fo, f, dep);
	GEN ("      %c_stb[%i] <= #Tp 1'b1;\n",
	     f->mtype[i] & MT_LOAD ? 'l' : 's', cur++);
	GEN ("    end\n");
      }
  GEN ("    if (%c_end[%i]) %c_stb <= #Tp %i'h0;\n", c, cur - 1, c, cur);
  GEN ("  end\n");
#endif

  /* Generate state generator machine */
  for (j = 0; j < 2; j++)
    {
      char c = 0;		/* Mark Jarvin patch to initialize values. */
      char *s = NULL;

      switch (j)
	{
	case 0:
	  c = 'l';
	  s = "Load";
	  break;
	case 1:
	  c = 's';
	  s = "Store";
	  break;
	case 2:
	  c = 'c';
	  s = "Calls";
	  break;
	}
      if ((j == 0 && nloads) || (j == 1 && nstores) || (j == 2 && ncalls))
	{
	  int cur = 0;
	  char t[30];

	  GEN ("\n/* %s state generator machine */\n", s);
	  GEN ("always @(");
	  for (i = 0; i < f->nmsched; i++)
	    {
	      print_op_v (f, t, f->msched[i], 1);
	      GEN ("%s or ", t);
	    }
	  GEN ("bb_start_r");
	  if (nloads)
	    GEN (" or l_end");
	  if (nstores)
	    GEN (" or s_end");
	  GEN (")\n");
	  GEN ("begin\n  ");
	  cucdebug (1, "%s\n", s);
	  for (i = 0; i < f->nmsched; i++)
	    if ((j == 0 && f->mtype[i] & MT_LOAD)
		|| (j == 1 && f->mtype[i] & MT_STORE)
		|| (j == 2 && f->mtype[i] & MT_CALL))
	      {
		cucdebug (1, "msched[%i] = %x (mtype %x)\n", i, f->msched[i],
			  f->mtype[i]);
		assert (f->INSN (f->msched[i]).
			opt[1] & (OPT_REF | OPT_REGISTER));
		GEN ("if (");
		print_deps (fo, f, REF_BB (f->msched[i]),
			    f->INSN (f->msched[i]).dep, 1);
		GEN (") begin\n");
		GEN ("    %c_req_o = 1'b1;\n", c);
		GEN ("    %c_sel_o[3:0] = 4'b", c);
		switch (f->mtype[i] & MT_WIDTH)
		  {
		  case 1:
		    GEN ("0001 << (%s & 32'h3);\n",
			 print_op_v (f, t, f->msched[i], 1));
		    break;
		  case 2:
		    GEN ("0011 << ((%s & 32'h1) << 1);\n",
			 print_op_v (f, t, f->msched[i], 1));
		    break;
		  case 4:
		    GEN ("1111;\n");
		    break;
		  default:
		    assert (0);
		  }
		GEN ("    %c_linbrst_o = 1'b%i;\n", c,
		     (f->mtype[i] & MT_BURST)
		     && !(f->mtype[i] & MT_BURSTE) ? 1 : 0);
		GEN ("    %c_adr_o = t%lx_%lx & ~32'h3;\n", c,
		     REF_BB (f->INSN (f->msched[i]).op[1]),
		     REF_I (f->INSN (f->msched[i]).op[1]));
		GEN ("  end else ");
	      }
	  GEN ("if (%c_end[%i]) begin\n", c, cur - 1);
	  GEN ("    %c_req_o = 1'b0;\n", c);
	  GEN ("    %c_sel_o[3:0] = 4'bx;\n", c);
	  GEN ("    %c_linbrst_o = 1'b0;\n", c);
	  GEN ("    %c_adr_o = 32'hx;\n", c);
	  GEN ("  end else begin\n");
	  GEN ("    %c_req_o = 1'b0;\n", c);
	  GEN ("    %c_sel_o[3:0] = 4'bx;\n", c);
	  GEN ("    %c_linbrst_o = 1'b0;\n", c);
	  GEN ("    %c_adr_o = 32'hx;\n", c);
	  GEN ("  end\n");
	  GEN ("end\n");
	}
    }

  if (ncalls)
    {
      int cur_call = 0;
      GEN ("\n/* Function calls state machine */\n");
      GEN ("always @(posedge clk or posedge rst)\n");
      GEN ("begin\n");
      GEN ("  if (rst) begin\n");
      GEN ("    f_stb <= #Tp %i'h0;\n", nstores);
      for (i = 0; i < 6; i++)
	GEN ("    fr%i_o <= #Tp 32'h0;\n", i + 3);
      if (log2_int (ncalls))
	GEN ("    fid_o <= #Tp %i'h0;\n", log2_int (f->nfdeps));
      GEN ("    fstart_o <= #Tp 1'b0;\n");
      //GEN ("    f11_r <= #Tp 32'h0;\n");
      GEN ("  end else begin\n");
      cucdebug (1, "calls \n");
      for (i = 0; i < f->nmsched; i++)
	if (f->mtype[i] & MT_CALL)
	  {
	    dep_list *dep = f->INSN (f->msched[i]).dep;
	    cucdebug (1, "msched[%i] = %x (mtype %x)\n", i, f->msched[i],
		      f->mtype[i]);
	    assert (f->INSN (f->msched[i]).opt[1] & (OPT_REF | OPT_REGISTER));
	    GEN ("    if (");
	    print_deps (fo, f, REF_BB (f->msched[i]),
			f->INSN (f->msched[i]).dep, 1);
	    GEN (") begin\n");
	    print_turn_off_dep (fo, f, dep);
	    GEN ("      f_stb[%i] <= #Tp 1'b1;\n", cur_call++);
	    GEN ("      fstart_o <= #Tp 1'b1;\n");
	    if (log2_int (f->nfdeps))
	      GEN ("      fid_o <= #Tp %i'h%x;\n", log2_int (f->nfdeps),
		   func_index (f, f->msched[i]));

	    for (j = 0; j < 6; j++)
	      GEN ("      fr%i_o <= #Tp t%x_%x;\n", j + 3,
		   REF_BB (f->msched[i]), REF_I (f->msched[i]) - 6 + i);
	    GEN ("    end\n");
	  }
      GEN ("    if (f_end[%i]) begin\n", ncalls - 1);
      GEN ("      f_stb <= #Tp %i'h0;\n", ncalls);
      GEN ("      f_start_o <= #Tp 1'b0;\n");
      GEN ("    end\n");
      GEN ("  end\n");
      GEN ("end\n");
    }

  GEN ("\n/* Basic blocks state machine */\n");
  GEN ("always @(posedge clk or posedge rst)\n");
  GEN ("begin\n");
  GEN ("  if (rst) bb_stb <= #Tp %i'h%x;\n", f->num_bb, 0);
  GEN ("  else if (end_o) begin\n");
  GEN ("    bb_stb <= #Tp %i'h%x;\n", f->num_bb, 0);
  for (i = 0; i < f->num_bb; i++)
    {
      GEN ("  end else if (bb_start[%i]) begin\n", i);
      GEN ("    bb_stb <= #Tp %i'h%x;\n", f->num_bb, 1 << i);
    }
  GEN ("  end else if (end_o) begin\n");
  GEN ("    bb_stb <= #Tp %i'h%x;\n", f->num_bb, 0);
  GEN ("  end\n");
  GEN ("end\n");

  /* output footer */
  GEN ("\nendmodule\n");

  fclose (fo);
}

void
generate_main (int nfuncs, cuc_func ** f, char *filename)
{
  FILE *fo;
  int i, j, nrf, first;
  char tmp[256];
  int ncallees[MAX_FUNCS];
  int nl[MAX_FUNCS], ns[MAX_FUNCS];
  int maxncallees = 0;
  sprintf (tmp, "%s_top.v", filename);

  for (i = 0, nrf = 0; i < nfuncs; i++)
    {
      nl[i] = ns[i] = 0;
      ncallees[i] = 0;
      if (f[i])
	{
	  f[i]->tmp = nrf++;
	  for (j = 0; j < f[i]->nmsched; j++)
	    if (f[i]->mtype[j] & MT_LOAD)
	      nl[i]++;
	    else if (f[i]->mtype[j] & MT_STORE)
	      ns[i]++;
	  for (j = 0; j < f[i]->nfdeps; j++)
	    ncallees[f[i]->fdeps[j]->tmp]++;
	}
    }
  if (!nrf)
    return;

  for (i = 0; i < nrf; i++)
    if (maxncallees < ncallees[i])
      maxncallees = ncallees[i];

  log ("Generating verilog file \"%s\"\n", tmp);
  PRINTF ("Generating verilog file \"%s\"\n", tmp);
  if ((fo = fopen (tmp, "wt+")) == NULL)
    {
      fprintf (stderr, "Cannot open '%s'\n", tmp);
      exit (1);
    }

  /* output header */
  GEN ("/* %s -- generated by Custom Unit Compiler\n", tmp);
  GEN ("   (C) 2002 Opencores */\n\n");
  GEN ("/* Includes %i functions:", nrf);
  for (i = 0; i < nfuncs; i++)
    if (f[i])
      GEN ("\n%s", prof_func[i].name);
  GEN (" */\n\n");

  GEN ("`include \"timescale.v\"\n\n");
  GEN ("module %s (clk, rst,\n", filename);
  GEN ("              /* Load and store master Wishbone ports */\n");
  GEN ("              l_adr_o, l_dat_i, l_cyc_o, l_stb_o,\n");
  GEN ("              l_sel_o, l_linbrst_o, l_rdy_i, l_we_o,\n");
  GEN ("              s_adr_o, s_dat_o, s_cyc_o, s_stb_o,\n");
  GEN ("              s_sel_o, s_linbrst_o, s_rdy_i, s_we_o,\n\n");
  GEN ("              /* cuc interface */\n");
  GEN
    ("              cuc_stb_i, cuc_adr_i, cuc_dat_i, cuc_dat_o, cuc_we_i, cuc_rdy_o);\n\n");

  GEN ("parameter Tp = 1;\n");
  GEN ("\n/* module ports */\n");
  GEN ("input         clk, rst, cuc_stb_i, cuc_we_i;\n");
  GEN ("input         l_rdy_i, s_rdy_i;\n");
  GEN ("output        l_cyc_o, l_stb_o, l_we_o, l_linbrst_o;\n");
  GEN ("reg           l_cyc_o, l_stb_o, l_we_o, l_linbrst_o;\n");
  GEN ("output        s_cyc_o, s_stb_o, s_we_o, s_linbrst_o;\n");
  GEN ("reg           s_cyc_o, s_stb_o, s_we_o, s_linbrst_o;\n");
  GEN ("output        cuc_rdy_o; /* Not registered ! */\n");
  GEN ("output  [3:0] l_sel_o, s_sel_o;\n");
  GEN ("reg     [3:0] l_sel_o, s_sel_o;\n");
  GEN ("output [31:0] l_adr_o, s_adr_o, s_dat_o, cuc_dat_o;\n");
  GEN ("reg    [31:0] l_adr_o, s_adr_o, s_dat_o, cuc_dat_o;\n");
  GEN ("input  [15:0] cuc_adr_i;\n");
  GEN ("input  [31:0] l_dat_i, cuc_dat_i;\n\n");

  GEN ("wire   [%2i:0] i_we, i_re, i_finish, i_selected, i_first_reg;\n",
       nrf - 1);
  GEN
    ("wire   [%2i:0] i_bidok, i_start_bid, i_start_bidok, main_start, main_end;\n",
     nrf - 1);
  GEN ("wire   [%2i:0] i_start, i_end, i_start_block, i_busy;\n", nrf - 1);
  GEN ("wire   [%2i:0] i_l_req, i_s_req;\n", nrf - 1);
  GEN ("reg    [%2i:0] i_go_bsy, main_start_r;\n", nrf - 1);

  GEN ("assign i_selected = {\n");
  for (i = 0; i < nrf; i++)
    GEN ("    cuc_adr_i[15:6] == %i%s\n", i, i < nrf - 1 ? "," : "};");

  GEN ("assign i_first_reg = {\n");
  for (i = 0; i < nfuncs; i++)
    if (f[i])
      {
	for (j = 0; j <= MAX_REGS; j++)
	  if (f[i]->used_regs[j])
	    break;
	GEN ("    cuc_adr_i[5:0] == %i%s\n", j,
	     f[i]->tmp < nrf - 1 ? "," : "};");
      }

  GEN ("assign i_we = {%i{cuc_stb_i && cuc_we_i}} & i_selected;\n", nrf);
  GEN ("assign i_re = {%i{cuc_stb_i && !cuc_we_i}} & i_selected;\n", nrf);

  GEN ("assign i_start = i_go_bsy & {%i{cuc_rdy_o}};\n", nrf);
  GEN ("assign i_start_bidok = {\n");
  for (i = 0; i < nrf; i++)
    GEN ("    i_start_bid[%i] < %i%s\n", i, i, i < nrf - 1 ? "," : "};");
  GEN ("assign main_start = i_start & i_selected & i_first_reg & i_we;\n");
  GEN ("assign main_end = {%i{i_end}} & i_selected;\n",
       nrf - 1);	/* JPB guess at missing args */

  GEN ("\nalways @(posedge clk or posedge rst)\n");
  GEN ("begin\n");
  GEN ("  if (rst) i_go_bsy <= #Tp %i'b0;\n", nrf);
  GEN ("  else i_go_bsy <= #Tp i_we | ~i_finish & i_go_bsy;\n");
  GEN ("end\n");


  /* Function specific data */
  for (i = 0; i < nfuncs; i++)
    if (f[i])
      {
	int ci = 0, co = 0;
	int fn = f[i]->tmp;
	GEN ("\n/* Registers for function %s */\n", prof_func[i].name);
	for (j = 0, first = 1; j < MAX_REGS; j++)
	  if (f[i]->used_regs[j])
	    {
	      GEN ("%s i%i_r%ii", first ? "/* inputs */\nreg    [31:0]" : ",",
		   fn, j);
	      first = 0;
	      ci++;
	    }
	if (ci)
	  GEN (";\n");

	for (j = 0, first = 1; j < MAX_REGS; j++)
	  if (f[i]->lur[j] >= 0 && !f[i]->saved_regs[j])
	    {
	      GEN ("%s i%i_r%io",
		   first ? "/* outputs */\nwire   [31:0]" : ",", fn, j);
	      first = 0;
	      co++;
	    }
	if (co)
	  GEN (";\n");
	GEN ("wire [31:0] i%i_l_adr, i%i_s_adr;\n", fn, fn);

	GEN ("always @(posedge clk or posedge rst)\n");
	GEN ("  if (rst) main_start_r <= #Tp %i'b0;\n", nrf);
	GEN
	  ("  else main_start_r <= #Tp main_start & i_start_bidok | i_busy | ~i_end & main_start_r;\n");

	if (ci)
	  {
	    GEN ("\n/* write register access */\n");
	    GEN ("always @(posedge clk or posedge rst)\n");
	    GEN ("begin\n");
	    GEN ("  if (rst) begin\n");
	    for (j = 0; j < MAX_REGS; j++)
	      if (f[i]->used_regs[j])
		GEN ("    i%i_r%ii <= #Tp 32'h0;\n", fn, j);
	    GEN ("  end else if (!i_go_bsy[%i] && i_we[%i])\n", fn, fn);
	    GEN ("    case (cuc_adr_i[5:0])\n");
	    for (j = 0; j < MAX_REGS; j++)
	      if (f[i]->used_regs[j])
		GEN ("      %-2i: i%i_r%ii <= #Tp cuc_dat_i;\n", j, fn, j);
	    GEN ("    endcase\n");
	    GEN ("end\n");
	  }

	GEN ("\n");
      }

  /* Generate machine for reading all function registers. Register read can be
     delayed till function completion */
  {
    int co;
    GEN ("/* read register access - data */\n");
    GEN ("always @(posedge clk or posedge rst)\n");
    GEN ("  if (rst) cuc_dat_o <= #Tp 32'h0;\n");
    GEN ("  else if (cuc_stb_i && cuc_we_i) begin\n");
    GEN ("    ");

    for (i = 0; i < nfuncs; i++)
      if (f[i])
	{
	  co = 0;
	  for (j = 0; j < MAX_REGS; j++)
	    if (f[i]->lur[j] >= 0 && !f[i]->saved_regs[j])
	      co++;

	  GEN ("if (cuc_adr_i[15:6] == %i)", f[i]->tmp);
	  if (co)
	    {
	      first = 1;
	      GEN ("\n      case (cuc_adr_i[5:0])\n");
	      for (j = 0; j < MAX_REGS; j++)
		if (f[i]->lur[j] >= 0 && !f[i]->saved_regs[j])
		  GEN ("        %-2i: cuc_dat_o <= #Tp i%i_r%io;\n", j,
		       f[i]->tmp, j);
	      GEN ("      endcase\n");
	    }
	  else
	    {
	      GEN ("      cuc_dat_o <= #Tp 32'hx;\n");
	    }
	  GEN ("    else ");
	}
    GEN ("cuc_dat_o <= #Tp 32'hx;\n");
    GEN ("  end else cuc_dat_o <= #Tp 32'hx;\n");

    GEN ("\n/* read register access - acknowledge */\n");
    GEN
      ("assign cuc_rdy_o = cuc_stb_i && cuc_we_i && |(i_selected & main_end);\n");
  }

  /* Store/load Wishbone bridge */
  for (j = 0; j < 2; j++)
    {
      char t = j ? 's' : 'l';
      GEN ("\n/* %s Wishbone bridge */\n", j ? "store" : "load");
      GEN ("reg [%i:0] %cm_sel;\n", log2_int (nrf), t);
      GEN ("reg [%i:0] %cm_bid;\n", log2_int (nrf), t);
      GEN ("reg       %ccyc_ip;\n\n", t);
      GEN ("always @(posedge clk)\n");
      GEN ("begin\n");
      GEN ("  %c_we_o <= #Tp 1'b%i;\n", t, j);
      GEN ("  %c_cyc_o <= #Tp |i_%c_req;\n", t, t);
      GEN ("  %c_stb_o <= #Tp |i_%c_req;\n", t, t);
      GEN ("end\n");

      GEN ("\n/* highest bid */\n");
      GEN ("always @(");
      for (i = 0; i < nrf; i++)
	GEN ("%si_%c_req", i > 0 ? " or " : "", t);
      GEN (")\n");
      for (i = 0; i < nrf; i++)
	GEN ("  %sif (i_%c_req) %cm_bid = %i'h%x;\n",
	     i ? "else " : "", t, t, log2_int (nrf) + 1, i);

      GEN ("\n/* selected transfer */\n");
      GEN ("always @(posedge clk or posedge rst)\n");
      GEN ("  if (rst) %cm_sel <= #Tp %i'h0;\n", t, log2_int (nrf) + 1);
      GEN ("  else if (%c_rdy_i) %cm_sel <= #Tp %i'h0;\n", t, t,
	   log2_int (nrf) + 1);
      GEN ("  else if (!%ccyc_ip) %cm_sel <= #Tp %cm_bid;\n", t, t, t);

      GEN ("\n/* Cycle */\n");
      GEN ("\nalways @(posedge clk or posedge rst)\n");
      GEN ("  if (rst) %ccyc_ip <= #Tp 1'b0;\n", t);
      GEN ("  else if (%c_rdy_i) %ccyc_ip <= #Tp 1'b0;\n", t, t);
      GEN ("  else %ccyc_ip <= #Tp %c_cyc_o;\n", t, t);
    }

  GEN ("\n/* Acknowledge */\n");
  for (i = 0; i < nrf; i++)
    {
      GEN
	("wire i%i_s_rdy = ((sm_bid == %i & !scyc_ip) | sm_sel == %i) & s_rdy_i;\n",
	 i, i, i);
      GEN
	("wire i%i_l_rdy = ((lm_bid == %i & !lcyc_ip) | lm_sel == %i) & l_rdy_i;\n",
	 i, i, i);
    }

  GEN ("\n/* data, address selects and burst enables */\n");
  for (i = 0; i < nrf; i++)
    GEN ("wire [31:0] i%i_s_dat;\n", i);
  for (i = 0; i < nrf; i++)
    GEN ("wire i%i_s_linbrst, i%i_l_linbrst;\n", i, i);
  for (i = 0; i < nrf; i++)
    GEN ("wire [3:0]  i%i_s_sel, i%i_l_sel;\n", i, i);
  for (i = 0; i < nrf; i++)
    GEN ("wire [31:0] i%i_l_dat = l_dat_i;\n", i);
  GEN ("\nalways @(posedge clk)\n");
  GEN ("begin\n");
  GEN ("  s_dat_o <= #Tp ");
  for (i = 0; i < nrf - 1; i++)
    GEN ("\n    sm_bid == %i ? i%i_s_dat : ", i, i);
  GEN ("i%i_s_dat;\n", nrf - 1);
  GEN ("  s_adr_o <= #Tp ");
  for (i = 0; i < nrf - 1; i++)
    GEN ("\n    sm_bid == %i ? i%i_s_adr : ", i, i);
  GEN ("i%i_s_adr;\n", nrf - 1);
  GEN ("  s_sel_o <= #Tp ");
  for (i = 0; i < nrf - 1; i++)
    GEN ("\n    sm_bid == %i ? i%i_s_sel : ", i, i);
  GEN ("i%i_s_sel;\n", nrf - 1);
  GEN ("  s_linbrst_o <= #Tp ");
  for (i = 0; i < nrf - 1; i++)
    GEN ("\n    sm_bid == %i ? i%i_s_linbrst : ", i, i);
  GEN ("i%i_s_linbrst;\n", nrf - 1);
  GEN ("end\n\n");

  GEN ("always @(posedge clk)\n");
  GEN ("begin\n");
  GEN ("  l_adr_o <= #Tp ");
  for (i = 0; i < nrf - 1; i++)
    GEN ("\n    lm_bid == %i ? i%i_l_adr : ", i, i);
  GEN ("i%i_l_adr;\n", nrf - 1);
  GEN ("  l_sel_o <= #Tp ");
  for (i = 0; i < nrf - 1; i++)
    GEN ("\n    lm_bid == %i ? i%i_l_sel : ", i, i);
  GEN ("i%i_l_sel;\n", nrf - 1);
  GEN ("  l_linbrst_o <= #Tp ");
  for (i = 0; i < nrf - 1; i++)
    GEN ("\n    lm_bid == %i ? i%i_l_linbrst : ", i, i);
  GEN ("i%i_l_linbrst;\n", nrf - 1);
  GEN ("end\n\n");

  /* start/end signals */
  GEN ("\n\n/* start/end signals */\n");
  for (i = 0; i < nrf; i++)
    {
      if (log2_int (maxncallees + 1))
	GEN
	  ("wire [%i:0] i%i_current = i%i_busy ? i%i_current_r : i%i_start_bid;\n",
	   log2_int (maxncallees + 1), i, i, i, i);
      else
	GEN ("wire i%i_current = 0;\n", i);
    }
  GEN ("\n");

  for (i = 0, j = 0; i < nfuncs; i++)
    if (f[i])
      {
	if (log2_int (ncallees[i]))
	  {
	    GEN ("reg [%i:0] i%i_start_bid;\n", log2_int (ncallees[i]), j);
	    GEN ("always @(start%i", f[i]->tmp);
	    for (j = 0, first = 1; j < f[i]->nfdeps; j++)
	      if (f[i]->fdeps[j])
		GEN (", ");
	    GEN (")\n");
	    GEN ("begin !!!\n");	//TODO
	    GEN ("  \n");
	    GEN ("end\n");
	  }
	GEN ("wire i%i_start = main_start[%i];\n", j, j);
	j++;
      }
  GEN ("\n");

  for (i = 0; i < nfuncs; i++)
    if (f[i])
      {
	int nf = f[i]->tmp;
	GEN ("\n%s%s i%i(.clk(clk), .rst(rst),\n", filename,
	     prof_func[i].name, nf);
	GEN
	  ("  .l_adr_o(i%i_l_adr), .l_dat_i(i%i_l_dat), .l_req_o(i_l_req[%i]),\n",
	   nf, nf, nf);
	GEN
	  ("  .l_sel_o(i%i_l_sel), .l_linbrst_o(i%i_l_linbrst), .l_rdy_i(i%i_l_rdy),\n",
	   nf, nf, nf);
	GEN
	  ("  .s_adr_o(i%i_s_adr), .s_dat_o(i%i_s_dat), .s_req_o(i_s_req[%i]),\n",
	   nf, nf, nf);
	GEN
	  ("  .s_sel_o(i%i_s_sel), .s_linbrst_o(i%i_s_linbrst), .s_rdy_i(i%i_s_rdy),\n",
	   nf, nf, nf);
	GEN ("  ");
	for (j = 0, first = 1; j < MAX_REGS; j++)
	  if (f[i]->used_regs[j])
	    GEN (".r%i_i(i%i_r%ii), ", j, nf, j), first = 0;

	if (first)
	  GEN ("\n  ");
	for (j = 0, first = 1; j < MAX_REGS; j++)
	  if (f[i]->lur[j] >= 0 && !f[i]->saved_regs[j])
	    GEN (".r%i_o(i%i_r%io), ", j, nf, j), first = 0;
	if (first)
	  GEN ("\n  ");
	if (f[i]->nfdeps)
	  {
	    GEN
	      (".fstart_o(i_fstart[%i]), .fend_i(i_fend[%i]), .fid_o(i%i_fid),\n",
	       i, i, i),
	      GEN
	      ("  .fr3_o(i%i_fr3), .fr4_o(i%i_fr4), .fr5_o(i%i_fr5), .fr6_o(i%i_fr6),\n",
	       i, i, i, i);	/* JPB guess at missing args */
	    GEN
	      ("  .fr7_o(i%i_fr7), .fr8_o(i%i_fr8), .fr11_i(i%i_fr11i),\n  ",
	       i, i, i);	/* JPB guess at missing args */
	  }
	GEN
	  (".start_i(i_start[%i]), .end_o(i_end[%i]), .busy_o(i_busy[%i]));\n",
	   nf, nf, nf);
      }

  /* output footer */
  GEN ("\nendmodule\n");

  fclose (fo);
}
