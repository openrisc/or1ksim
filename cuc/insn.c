/* insn.c -- OpenRISC Custom Unit Compiler, instruction support

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

/* System includes */
#include <stdlib.h>
#include <assert.h>

/* Package includes */
#include "insn.h"
#include "sim-config.h"


/* Table of known instructions.  Watch out for indexes I_*! */
const cuc_known_insn known[II_LAST + 1] = {
  {"add", 1, "assign \1 = \2 + \3;"},
  {"sub", 0, "assign \1 = \2 - \3;"},
  {"and", 1, "assign \1 = \2 & \3;"},
  {"or", 1, "assign \1 = \2 | \3;"},
  {"xor", 1, "assign \1 = \2 ^ \3;"},
  {"mul", 1, "assign \1 = \2 * \3;"},

  {"srl", 0, "assign \1 = \2 >> \3;"},
  {"sll", 0, "assign \1 = \2 << \3;"},
  {"sra", 0, "assign \1 = ({32{\2[31]}} << (6'd32-{1'b0, \3}))\n\
                 | \2 >> \3;"},

  {"lb", 0, "always @(posedge clk)"},
  {"lh", 0, "always @(posedge clk)"},
  {"lw", 0, "always @(posedge clk)"},
  {"sb", 0, "/* mem8[\2] = \1 */"},
  {"sh", 0, "/* mem16[\2] = \1 */"},
  {"sw", 0, "/* mem32[\2] = \1 */"},

  {"sfeq", 1, "assign \1 = \2 == \3;"},
  {"sfne", 1, "assign \1 = \2 != \3;"},
  {"sfle", 0, "assign \1 = \2 <= \3;"},
  {"sflt", 0, "assign \1 = \2 < \3;"},
  {"sfge", 0, "assign \1 = \2 >= \3;"},
  {"sfgt", 0, "assign \1 = \2 > \3;"},
  {"bf", 0, ""},

  {"lrbb", 0, "always @(posedge clk or posedge rst)"},
  {"cmov", 0, "assign \1 = \4 ? \2 : \3;"},
  {"reg", 0, "always @(posedge clk)"},

  {"nop", 1, ""},
  {"call", 0, "/* function call */"}
};

/* Find known instruction and attach them to insn */
void
change_insn_type (cuc_insn * i, int index)
{
  int j;
  assert (index >= 0 && index <= II_LAST);
  i->index = index;
  if (i->index == II_NOP)
    {
      for (j = 0; j < MAX_OPERANDS; j++)
	i->opt[j] = OPT_NONE;
      i->type = 0;
      i->dep = NULL;
      i->disasm[0] = '\0';
    }
}

/* Returns instruction name */
const char *
cuc_insn_name (cuc_insn * ii)
{
  if (ii->index < 0 || ii->index > II_LAST)
    return "???";
  else
    return known[ii->index].name;
}

/* Prints out instructions */
void
print_insns (int bb, cuc_insn * insn, int ninsn, int verbose)
{
  int i, j;
  for (i = 0; i < ninsn; i++)
    {
      char tmp[10];
      dep_list *l = insn[i].dep;
      sprintf (tmp, "[%x_%x]", bb, i);
      PRINTF ("%-8s%c %-4s ", tmp, insn[i].index >= 0 ? ':' : '?',
	      cuc_insn_name (&insn[i]));
      if (verbose)
	{
	  PRINTF ("%-20s insn = %08lx, index = %i, type = %04x ",
		  insn[i].disasm, insn[i].insn, insn[i].index, insn[i].type);
	}
      else
	PRINTF ("type = %04x ", insn[i].type);
      for (j = 0; j < MAX_OPERANDS; j++)
	{
	  if (insn[i].opt[j] & OPT_DEST)
	    PRINTF ("*");
	  switch (insn[i].opt[j] & ~OPT_DEST)
	    {
	    case OPT_NONE:
	      break;
	    case OPT_CONST:
	      if (insn[i].type & IT_COND && (insn[i].index == II_CMOV
					     || insn[i].index == II_ADD))
		PRINTF ("%lx, ", insn[i].op[j]);
	      else
		PRINTF ("0x%08lx, ", insn[i].op[j]);
	      break;
	    case OPT_JUMP:
	      PRINTF ("J%lx, ", insn[i].op[j]);
	      break;
	    case OPT_REGISTER:
	      PRINTF ("r%li, ", insn[i].op[j]);
	      break;
	    case OPT_REF:
	      PRINTF ("[%lx_%lx], ", REF_BB (insn[i].op[j]),
		      REF_I (insn[i].op[j]));
	      break;
	    case OPT_BB:
	      PRINTF ("BB ");
	      print_bb_num (insn[i].op[j]);
	      PRINTF (", ");
	      break;
	    case OPT_LRBB:
	      PRINTF ("LRBB, ");
	      break;
	    default:
	      fprintf (stderr, "Invalid operand type %s(%x_%x) = %x\n",
		       cuc_insn_name (&insn[i]), i, j, insn[i].opt[j]);
	      assert (0);
	    }
	}
      if (l)
	{
	  PRINTF ("\n\tdep:");
	  while (l)
	    {
	      PRINTF (" [%lx_%lx],", REF_BB (l->ref), REF_I (l->ref));
	      l = l->next;
	    }
	}
      PRINTF ("\n");
    }
}

void
add_dep (dep_list ** list, int dep)
{
  dep_list *ndep;
  dep_list **tmp = list;

  while (*tmp)
    {
      if ((*tmp)->ref == dep)
	return;			/* already there */
      tmp = &((*tmp)->next);
    }
  ndep = (dep_list *) malloc (sizeof (dep_list));
  ndep->ref = dep;
  ndep->next = NULL;
  *tmp = ndep;
}

void
dispose_list (dep_list ** list)
{
  while (*list)
    {
      dep_list *tmp = *list;
      *list = tmp->next;
      free (tmp);
    }
}

void
add_data_dep (cuc_func * f)
{
  int b, i, j;
  for (b = 0; b < f->num_bb; b++)
    {
      cuc_insn *insn = f->bb[b].insn;
      for (i = 0; i < f->bb[b].ninsn; i++)
	for (j = 0; j < MAX_OPERANDS; j++)
	  {
	    fflush (stdout);
	    if (insn[i].opt[j] & OPT_REF)
	      {
		/* Copy list from predecessor */
		dep_list *l = f->INSN (insn[i].op[j]).dep;
		while (l)
		  {
		    add_dep (&insn[i].dep, l->ref);
		    l = l->next;
		  }
		/* add predecessor */
		add_dep (&insn[i].dep, insn[i].op[j]);
	      }
	  }
    }
}

/* Inserts n nops before insn 'ref' */
void
insert_insns (cuc_func * f, int ref, int n)
{
  int b1, i, j;
  int b = REF_BB (ref);
  int ins = REF_I (ref);

  assert (b < f->num_bb);
  assert (ins <= f->bb[b].ninsn);
  assert (f->bb[b].ninsn + n < MAX_INSNS);
  if (cuc_debug >= 8)
    print_cuc_bb (f, "PREINSERT");
  f->bb[b].insn = (cuc_insn *) realloc (f->bb[b].insn,
					(f->bb[b].ninsn +
					 n) * sizeof (cuc_insn));

  /* Set up relocations */
  for (i = 0; i < f->bb[b].ninsn; i++)
    if (i < ins)
      reloc[i] = i;
    else
      reloc[i] = i + n;

  /* Move instructions, based on relocations */
  for (i = f->bb[b].ninsn - 1; i >= 0; i--)
    f->bb[b].insn[reloc[i]] = f->bb[b].insn[i];
  for (i = 0; i < n; i++)
    change_insn_type (&f->bb[b].insn[ins + i], II_NOP);

  f->bb[b].ninsn += n;
  for (b1 = 0; b1 < f->num_bb; b1++)
    {
      dep_list *d = f->bb[b1].mdep;
      while (d)
	{
	  if (REF_BB (d->ref) == b && REF_I (d->ref) >= ins)
	    d->ref = REF (b, REF_I (d->ref) + n);
	  d = d->next;
	}
      for (i = 0; i < f->bb[b1].ninsn; i++)
	{
	  d = f->bb[b1].insn[i].dep;
	  while (d)
	    {
	      if (REF_BB (d->ref) == b && REF_I (d->ref) >= ins)
		d->ref = REF (b, REF_I (d->ref) + n);
	      d = d->next;
	    }
	  for (j = 0; j < MAX_OPERANDS; j++)
	    if (f->bb[b1].insn[i].opt[j] & OPT_REF
		&& REF_BB (f->bb[b1].insn[i].op[j]) == b
		&& REF_I (f->bb[b1].insn[i].op[j]) >= ins)
	      f->bb[b1].insn[i].op[j] =
		REF (b, REF_I (f->bb[b1].insn[i].op[j]) + n);
	}
    }
  for (i = 0; i < f->nmsched; i++)
    if (REF_BB (f->msched[i]) == b)
      f->msched[i] = REF (b, reloc[REF_I (f->msched[i])]);
  if (cuc_debug >= 8)
    print_cuc_bb (f, "POSTINSERT");
  cuc_check (f);
}

/* returns nonzero, if instruction was simplified */
int
apply_edge_condition (cuc_insn * ii)
{
  unsigned int c = ii->op[2];

  switch (ii->index)
    {
    case II_AND:
      if (ii->opt[2] & OPT_CONST && c == 0)
	{
	  change_insn_type (ii, II_ADD);
	  ii->op[1] = 0;
	  ii->opt[1] = OPT_CONST;
	  ii->op[2] = 0;
	  ii->opt[2] = OPT_CONST;
	  return 1;
	}
      else if (ii->opt[2] & OPT_CONST && c == 0xffffffff)
	{
	  change_insn_type (ii, II_ADD);
	  ii->op[2] = 0;
	  ii->opt[2] = OPT_CONST;
	  return 1;
	}
      else
	break;
    case II_OR:
      if (ii->opt[2] & OPT_CONST && c == 0x0)
	{
	  change_insn_type (ii, II_ADD);
	  ii->op[1] = c;
	  ii->opt[1] = OPT_CONST;
	  ii->op[2] = 0;
	  ii->opt[2] = OPT_CONST;
	  return 1;
	}
      else if (ii->opt[2] & OPT_CONST && c == 0xffffffff)
	{
	  change_insn_type (ii, II_ADD);
	  ii->op[1] = 0xffffffff;
	  ii->opt[1] = OPT_CONST;
	  ii->op[2] = 0;
	  ii->opt[2] = OPT_CONST;
	  return 1;
	}
      else
	break;
    case II_SUB:
      if (ii->opt[1] == ii->opt[2] && ii->op[1] == ii->op[2])
	{
	  change_insn_type (ii, II_ADD);
	  ii->op[1] = 0;
	  ii->opt[1] = OPT_CONST;
	  ii->op[2] = 0;
	  ii->opt[2] = OPT_CONST;
	  return 1;
	}
      else
	break;
    case II_MUL:
      if (ii->opt[2] & OPT_CONST && c == 0)
	{
	  change_insn_type (ii, II_ADD);
	  ii->op[1] = 0;
	  ii->opt[1] = OPT_CONST;
	  ii->op[2] = 0;
	  ii->opt[2] = OPT_CONST;
	  return 1;
	}
      else if (ii->opt[2] & OPT_CONST && c == 1)
	{
	  change_insn_type (ii, II_ADD);
	  ii->op[1] = c;
	  ii->opt[1] = OPT_CONST;
	  ii->op[2] = 0;
	  ii->opt[2] = OPT_CONST;
	  return 1;
	}
      else if (ii->opt[2] & OPT_CONST && c == 0xffffffff)
	{
	  change_insn_type (ii, II_SUB);
	  ii->op[2] = ii->op[1];
	  ii->opt[2] = ii->opt[1];
	  ii->op[1] = 0;
	  ii->opt[1] = OPT_CONST;
	  return 1;
	}
      else
	break;
    case II_SRL:
      if (ii->opt[2] & OPT_CONST && c == 0)
	{
	  change_insn_type (ii, II_ADD);
	  ii->op[1] = c;
	  ii->opt[1] = OPT_CONST;
	  ii->op[2] = 0;
	  ii->opt[2] = OPT_CONST;
	  return 1;
	}
      else if (ii->opt[2] & OPT_CONST && c >= 32)
	{
	  change_insn_type (ii, II_ADD);
	  ii->op[1] = 0;
	  ii->opt[1] = OPT_CONST;
	  ii->op[2] = 0;
	  ii->opt[2] = OPT_CONST;
	  return 1;
	}
      else
	break;
    case II_SLL:
      if (ii->opt[2] & OPT_CONST && c == 0)
	{
	  change_insn_type (ii, II_ADD);
	  ii->op[1] = c;
	  ii->opt[1] = OPT_CONST;
	  ii->op[2] = 0;
	  ii->opt[2] = OPT_CONST;
	  return 1;
	}
      else if (ii->opt[2] & OPT_CONST && c >= 32)
	{
	  change_insn_type (ii, II_ADD);
	  ii->op[1] = 0;
	  ii->opt[1] = OPT_CONST;
	  ii->op[2] = 0;
	  ii->opt[2] = OPT_CONST;
	  return 1;
	}
      else
	break;
    case II_SRA:
      if (ii->opt[2] & OPT_CONST && c == 0)
	{
	  change_insn_type (ii, II_ADD);
	  ii->op[1] = c;
	  ii->opt[1] = OPT_CONST;
	  ii->op[2] = 0;
	  ii->opt[2] = OPT_CONST;
	  return 1;
	}
      else
	break;
    case II_SFEQ:
      if (ii->opt[1] & OPT_CONST && ii->opt[2] & OPT_CONST)
	{
	  change_insn_type (ii, II_ADD);
	  ii->op[1] = ii->op[1] == ii->op[2];
	  ii->opt[1] = OPT_CONST;
	  ii->op[2] = 0;
	  ii->opt[2] = OPT_CONST;
	  return 1;
	}
      else
	break;
    case II_SFNE:
      if (ii->opt[1] & OPT_CONST && ii->opt[2] & OPT_CONST)
	{
	  change_insn_type (ii, II_ADD);
	  ii->op[1] = ii->op[1] != ii->op[2];
	  ii->opt[1] = OPT_CONST;
	  ii->op[2] = 0;
	  ii->opt[2] = OPT_CONST;
	  return 1;
	}
      else
	break;
    case II_SFLE:
      if (ii->opt[1] & OPT_CONST && ii->opt[2] & OPT_CONST)
	{
	  change_insn_type (ii, II_ADD);
	  ii->op[1] = ii->op[1] <= ii->op[2];
	  ii->opt[1] = OPT_CONST;
	  ii->op[2] = 0;
	  ii->opt[2] = OPT_CONST;
	  return 1;
	}
      else if (ii->opt[2] && OPT_CONST && ii->op[2] == 0)
	{
	  change_insn_type (ii, II_SFEQ);
	}
      else
	break;
    case II_SFLT:
      if (ii->opt[1] & OPT_CONST && ii->opt[2] & OPT_CONST)
	{
	  change_insn_type (ii, II_ADD);
	  ii->op[1] = ii->op[1] < ii->op[2];
	  ii->opt[1] = OPT_CONST;
	  ii->op[2] = 0;
	  ii->opt[2] = OPT_CONST;
	  return 1;
	}
      else if (ii->opt[2] && OPT_CONST && ii->op[2] == 0)
	{
	  change_insn_type (ii, II_ADD);
	  ii->op[1] = 0;
	  ii->opt[1] = OPT_CONST;
	}
      break;
    case II_SFGE:
      if (ii->opt[1] & OPT_CONST && ii->opt[2] & OPT_CONST)
	{
	  change_insn_type (ii, II_ADD);
	  ii->op[1] = ii->op[1] >= ii->op[2];
	  ii->opt[1] = OPT_CONST;
	  ii->op[2] = 0;
	  ii->opt[2] = OPT_CONST;
	  return 1;
	}
      else if (ii->opt[2] && OPT_CONST && ii->op[2] == 0)
	{
	  change_insn_type (ii, II_ADD);
	  ii->op[1] = 1;
	  ii->opt[1] = OPT_CONST;
	}
      else
	break;
    case II_SFGT:
      if (ii->opt[1] & OPT_CONST && ii->opt[2] & OPT_CONST)
	{
	  change_insn_type (ii, II_ADD);
	  ii->op[1] = ii->op[1] > ii->op[2];
	  ii->opt[1] = OPT_CONST;
	  ii->op[2] = 0;
	  ii->opt[2] = OPT_CONST;
	  return 1;
	}
      else if (ii->opt[2] && OPT_CONST && ii->op[2] == 0)
	{
	  change_insn_type (ii, II_SFNE);
	}
      else
	break;
    case II_CMOV:
      if (ii->opt[1] == ii->opt[2] && ii->op[1] == ii->op[2])
	{
	  change_insn_type (ii, II_ADD);
	  ii->op[2] = 0;
	  ii->opt[2] = OPT_CONST;
	  ii->opt[3] = OPT_NONE;
	  return 1;
	}
      if (ii->opt[3] & OPT_CONST)
	{
	  change_insn_type (ii, II_ADD);
	  if (ii->op[3])
	    {
	      ii->op[2] = 0;
	      ii->opt[2] = OPT_CONST;
	    }
	  else
	    {
	      ii->op[1] = 0;
	      ii->opt[1] = OPT_CONST;
	    }
	  ii->opt[3] = OPT_NONE;
	  return 1;
	}
      if (ii->type & IT_COND)
	{
	  if (ii->opt[1] & OPT_CONST && ii->opt[2] & OPT_CONST)
	    {
	      if (ii->op[1] && !ii->op[2])
		{
		  change_insn_type (ii, II_ADD);
		  ii->op[1] = ii->op[3];
		  ii->opt[1] = ii->opt[3];
		  ii->op[2] = 0;
		  ii->opt[2] = OPT_CONST;
		  ii->opt[3] = OPT_NONE;
		  return 1;
		}
	      if (ii->op[1] && ii->op[2])
		{
		  change_insn_type (ii, II_ADD);
		  ii->op[1] = 1;
		  ii->opt[1] = OPT_CONST;
		  ii->op[2] = 0;
		  ii->opt[2] = OPT_CONST;
		  ii->opt[3] = OPT_NONE;
		  return 1;
		}
	      if (!ii->op[1] && !ii->op[2])
		{
		  change_insn_type (ii, II_ADD);
		  ii->op[1] = 0;
		  ii->opt[1] = OPT_CONST;
		  ii->op[2] = 0;
		  ii->opt[2] = OPT_CONST;
		  ii->opt[3] = OPT_NONE;
		  return 1;
		}
	    }
	  if (ii->op[1] == ii->op[3] && ii->opt[1] == ii->opt[3])
	    {
	      ii->op[1] = 1;
	      ii->opt[1] = OPT_CONST;
	      return 1;
	    }
	  if (ii->op[2] == ii->op[3] && ii->opt[2] == ii->opt[3])
	    {
	      ii->op[2] = 0;
	      ii->opt[2] = OPT_CONST;
	      return 1;
	    }
	}
      break;
    }
  return 0;
}

/* First primary input */
static unsigned long tmp_op, tmp_opt;

/* Recursive function that searches for primary inputs;
   returns 0 if cmov can be replaced by add */
static int
cmov_needed (cuc_func * f, int ref)
{
  cuc_insn *ii = &f->INSN (ref);
  int j;

  cucdebug (4, " %x", ref);
  /* mark visited, if already marked, we have a loop, ignore */
  if (ii->tmp)
    return 0;
  ii->tmp = 1;

  /* handle normal movs separately */
  if (ii->index == II_ADD && !(ii->type & IT_VOLATILE)
      && ii->opt[2] == OPT_CONST && ii->op[2] == 0)
    {
      if (ii->opt[1] == OPT_REF)
	{
	  if (cmov_needed (f, ii->op[1]))
	    {
	      ii->tmp = 0;
	      return 1;
	    }
	}
      else
	{
	  if (tmp_opt == OPT_NONE)
	    {
	      tmp_op = ii->op[1];
	      tmp_opt = ii->opt[1];
	    }
	  else if (tmp_opt != ii->opt[1] || tmp_op != ii->op[1])
	    {
	      ii->tmp = 0;
	      return 1;
	    }
	}
      ii->tmp = 0;
      return 0;
    }

  /* Is this instruction CMOV? no => add to primary inputs */
  if ((ii->index != II_CMOV) || (ii->type & IT_VOLATILE))
    {
      if (tmp_opt == OPT_NONE)
	{
	  tmp_op = ref;
	  tmp_opt = OPT_REF;
	  ii->tmp = 0;
	  return 0;
	}
      else if (tmp_opt != OPT_REF || tmp_op != ref)
	{
	  ii->tmp = 0;
	  return 1;
	}
      else
	{
	  ii->tmp = 0;
	  return 0;
	}
    }

  for (j = 1; j < 3; j++)
    {
      cucdebug (4, "(%x:%i)", ref, j);
      if (ii->opt[j] == OPT_REF)
	{
	  if (cmov_needed (f, ii->op[j]))
	    {
	      ii->tmp = 0;
	      return 1;
	    }
	}
      else
	{
	  if (tmp_opt == OPT_NONE)
	    {
	      tmp_op = ii->op[j];
	      tmp_opt = ii->opt[j];
	    }
	  else if (tmp_opt != ii->opt[j] || tmp_op != ii->op[j])
	    {
	      ii->tmp = 0;
	      return 1;
	    }
	}
    }

  ii->tmp = 0;
  return 0;
}

/* Search and optimize complex cmov assignments */
int
optimize_cmovs (cuc_func * f)
{
  int modified = 0;
  int b, i;

  /* Mark all instructions unvisited */
  for (b = 0; b < f->num_bb; b++)
    if (!(f->bb[b].type & BB_DEAD))
      for (i = 0; i < f->bb[b].ninsn; i++)
	f->bb[b].insn[i].tmp = 0;

  for (b = 0; b < f->num_bb; b++)
    if (!(f->bb[b].type & BB_DEAD))
      {
	for (i = 0; i < f->bb[b].ninsn; i++)
	  {
	    cuc_insn *ii = &f->bb[b].insn[i];
	    if (ii->index == II_CMOV && !(ii->type & IT_VOLATILE))
	      {
		tmp_opt = OPT_NONE;
		cucdebug (4, "\n");
		if (!cmov_needed (f, REF (b, i)))
		  {
		    assert (tmp_opt != OPT_NONE);
		    change_insn_type (ii, II_ADD);
		    ii->op[1] = tmp_op;
		    ii->opt[1] = tmp_opt;
		    ii->op[2] = 0;
		    ii->opt[2] = OPT_CONST;
		    ii->opt[3] = OPT_NONE;
		    modified = 1;
		  }
	      }
	  }
      }
  return modified;
}

/* returns number of instructions, using instruction ref */
static int
insn_uses (cuc_func * f, int ref)
{
  int b, i, j;
  int cnt = 0;
  for (b = 0; b < f->num_bb; b++)
    for (i = 0; i < f->bb[b].ninsn; i++)
      for (j = 0; j < MAX_OPERANDS; j++)
	if (f->bb[b].insn[i].opt[j] & OPT_REF
	    && f->bb[b].insn[i].op[j] == ref)
	  cnt++;
  return cnt;
}

/* handles some common CMOV, CMOV-CMOV cases;
   returns nonzero if anything optimized */
static int
optimize_cmov_more (cuc_func * f, int ref)
{
  int t = 0;
  cuc_insn *ii = &f->INSN (ref);
  assert (ii->index == II_CMOV);

  /* In case of x = cmov x, y; or x = cmov y, x; we have
     asynchroneous loop -> remove it */
  if ((ii->opt[1] & OPT_REF) && ii->op[1] == ref)
    t = 1;
  if ((ii->opt[2] & OPT_REF) && ii->op[2] == ref)
    t = 2;
  if (ii->opt[1] == ii->opt[2] && ii->op[1] == ii->op[2])
    t = 2;
  if (t)
    {
      change_insn_type (ii, II_ADD);
      cucdebug (2, "%8x:cmov     %i\n", ref, t);
      ii->opt[t] = OPT_CONST;
      ii->op[t] = 0;
      ii->opt[3] = OPT_NONE;
      return 1;
    }
  if (!(ii->type & IT_COND))
    {
      for (t = 1; t <= 2; t++)
	{
	  /*   cmov L, X, Y, C1
	     cmov Z, L, Y, C2
	     can be replaced with simpler:
	     cmov L, C1, C2, C2
	     cmov Z, X, Y, L */
	  if (ii->opt[t] == OPT_REF && f->INSN (ii->op[t]).index == II_CMOV)
	    {
	      int r = ii->op[t];
	      unsigned long x, xt, y, yt;
	      cuc_insn *prev = &f->INSN (r);
	      cuc_check (f);
	      cucdebug (3, "%x-%x\n", ref, r);
	      assert (!(prev->type & IT_COND));
	      if (prev->op[3 - t] != ii->op[3 - t]
		  || prev->opt[3 - t] != ii->opt[3 - t]
		  || insn_uses (f, r) > 1)
		continue;
	      cucdebug (3, "%x-%x cmov more\n", ref, r);
	      prev->type |= IT_COND;
	      x = prev->op[t];
	      xt = prev->opt[t];
	      y = prev->op[3 - t];
	      yt = prev->opt[3 - t];
	      prev->op[t] = ii->op[3];
	      prev->opt[t] = ii->opt[3];	/* C2 */
	      ii->op[3] = r;
	      ii->opt[3] = OPT_REF;	/* L */
	      prev->op[3 - t] = prev->op[3];
	      prev->opt[3 - t] = prev->opt[3];	/* C1 */
	      prev->op[3] = prev->op[t];
	      prev->opt[3] = prev->opt[t];	/* C2 */
	      ii->op[t] = x;
	      ii->opt[t] = xt;	/* X */
	      ii->op[3 - t] = y;
	      ii->opt[3 - t] = yt;	/* Y */
	      prev->op[0] = -1;
	      prev->opt[0] = OPT_REGISTER | OPT_DEST;
	      cuc_check (f);
	      return 1;
	    }
	}
    }

  if (ii->opt[3] & OPT_REF)
    {
      cuc_insn *prev = &f->INSN (ii->op[3]);
      assert (prev->type & IT_COND);
      if (prev->index == II_CMOV)
	{
	  /* negated conditional:
	     cmov x, 0, 1, y
	     cmov z, a, b, x
	     is replaced by
	     cmov z, b, a, y */
	  if (prev->opt[1] & OPT_CONST && prev->opt[2] & OPT_CONST
	      && !prev->op[1] && prev->op[2])
	    {
	      unsigned long t;
	      t = ii->op[1];
	      ii->op[1] = ii->op[2];
	      ii->op[2] = t;
	      t = ii->opt[1];
	      ii->opt[1] = ii->opt[2];
	      ii->opt[2] = t;
	      ii->op[3] = prev->op[3];
	      ii->opt[3] = prev->opt[3];
	    }
	}
      else if (prev->index == II_ADD)
	{
	  /*   add x, y, 0
	     cmov z, a, b, x
	     is replaced by
	     cmov z, a, b, y */
	  if (prev->opt[2] & OPT_CONST && prev->op[2] == 0)
	    {
	      ii->op[3] = prev->op[1];
	      ii->opt[3] = prev->opt[1];
	      return 1;
	    }
	}
    }

  /* Check if both choices can be pushed through */
  if (ii->opt[1] & OPT_REF && ii->opt[2] & OPT_REF
      /* Usually doesn't make sense to move conditionals though => more area */
      && !(ii->type & IT_COND))
    {
      cuc_insn *a, *b;
      a = &f->INSN (ii->op[1]);
      b = &f->INSN (ii->op[2]);
      if (a->index == b->index && !(a->type & IT_VOLATILE)
	  && !(b->type & IT_VOLATILE))
	{
	  int diff = -1;
	  int i;
	  for (i = 0; i < MAX_OPERANDS; i++)
	    if (a->opt[i] != b->opt[i]
		|| !(a->op[i] == b->op[i] || a->opt[i] & OPT_REGISTER))
	      {
		if (diff == -1)
		  diff = i;
		else
		  diff = -2;
	      }
	  /* If diff == -1, it will be eliminated by CSE */
	  if (diff >= 0)
	    {
	      cuc_insn tmp, cmov;
	      int ref2 = REF (REF_BB (ref), REF_I (ref) + 1);
	      insert_insns (f, ref, 1);
	      a = &f->INSN (f->INSN (ref2).op[1]);
	      b = &f->INSN (f->INSN (ref2).op[2]);
	      cucdebug (4, "ref = %x %lx %lx\n", ref, f->INSN (ref2).op[1],
			f->INSN (ref2).op[2]);
	      if (cuc_debug >= 7)
		{
		  print_cuc_bb (f, "AAA");
		  cuc_check (f);
		}
	      tmp = *a;
	      cmov = f->INSN (ref2);
	      tmp.op[diff] = ref;
	      tmp.opt[diff] = OPT_REF;
	      cmov.op[0] = -1;
	      cmov.opt[0] = OPT_REGISTER | OPT_DEST;
	      cmov.op[1] = a->op[diff];
	      cmov.opt[1] = a->opt[diff];
	      cmov.op[2] = b->op[diff];
	      cmov.opt[2] = b->opt[diff];
	      change_insn_type (&cmov, II_CMOV);
	      cmov.type &= ~IT_COND;
	      cucdebug (4, "ref2 = %x %lx %lx\n", ref2, cmov.op[1],
			cmov.op[2]);
	      if (cmov.opt[1] & OPT_REF && cmov.opt[2] & OPT_REF
		  && f->INSN (cmov.op[1]).type & IT_COND)
		{
		  assert (f->INSN (cmov.op[2]).type & IT_COND);
		  cmov.type |= IT_COND;
		}
	      f->INSN (ref) = cmov;
	      f->INSN (ref2) = tmp;
	      if (cuc_debug >= 6)
		print_cuc_bb (f, "BBB");
	      cuc_check (f);
	      return 1;
	    }
	}
    }
  return 0;
}

/* Optimizes dataflow tree */
int
optimize_tree (cuc_func * f)
{
  int b, i, j;
  int modified;
  int gmodified = 0;

  do
    {
      modified = 0;
      if (cuc_debug)
	cuc_check (f);
      for (b = 0; b < f->num_bb; b++)
	if (!(f->bb[b].type & BB_DEAD))
	  {
	    for (i = 0; i < f->bb[b].ninsn; i++)
	      {
		cuc_insn *ii = &f->bb[b].insn[i];
		/* We tend to have the third parameter const if instruction is cumutative */
		if ((ii->opt[1] & OPT_CONST) && !(ii->opt[2] & OPT_CONST))
		  {
		    int cond = ii->index == II_SFEQ || ii->index == II_SFNE
		      || ii->index == II_SFLT || ii->index == II_SFLE
		      || ii->index == II_SFGT || ii->index == II_SFGE;
		    if (known[ii->index].comutative || cond)
		      {
			unsigned long t = ii->opt[1];
			ii->opt[1] = ii->opt[2];
			ii->opt[2] = t;
			t = ii->op[1];
			ii->op[1] = ii->op[2];
			ii->op[2] = t;
			modified = 1;
			cucdebug (2, "%08x:<>\n", REF (b, i));
			if (cond)
			  {
			    if (ii->index == II_SFEQ)
			      ii->index = II_SFNE;
			    else if (ii->index == II_SFNE)
			      ii->index = II_SFEQ;
			    else if (ii->index == II_SFLE)
			      ii->index = II_SFGT;
			    else if (ii->index == II_SFLT)
			      ii->index = II_SFGE;
			    else if (ii->index == II_SFGE)
			      ii->index = II_SFLT;
			    else if (ii->index == II_SFGT)
			      ii->index = II_SFLE;
			    else
			      assert (0);
			  }
		      }
		  }

		/* Try to do the promotion */
		/* We have two consecutive expressions, containing constants,
		 * if previous is a simple expression we can handle it simply: */
		for (j = 0; j < MAX_OPERANDS; j++)
		  if (ii->opt[j] & OPT_REF)
		    {
		      cuc_insn *t = &f->INSN (ii->op[j]);
		      if (f->INSN (ii->op[j]).index == II_ADD
			  && f->INSN (ii->op[j]).opt[2] & OPT_CONST
			  && f->INSN (ii->op[j]).op[2] == 0
			  && !(ii->type & IT_MEMORY && t->type & IT_MEMADD))
			{
			  /* do not promote through add-mem, and branches */
			  modified = 1;
			  cucdebug (2, "%8x:promote%i %8lx %8lx\n",
				    REF (b, i), j, ii->op[j], t->op[1]);
			  ii->op[j] = t->op[1];
			  ii->opt[j] = t->opt[1];
			}
		    }

		/* handle some CMOV cases more deeply */
		if (ii->index == II_CMOV
		    && optimize_cmov_more (f, REF (b, i)))
		  {
		    modified = 1;
		    continue;
		  }

		/* Do nothing to volatile instructions */
		if (ii->type & IT_VOLATILE)
		  continue;

		/* Check whether we can simplify the instruction */
		if (apply_edge_condition (ii))
		  {
		    modified = 1;
		    continue;
		  }
		/* We cannot do anything more if at least one is not constant */
		if (!(ii->opt[2] & OPT_CONST))
		  continue;

		if (ii->opt[1] & OPT_CONST)
		  {		/* We have constant expression */
		    unsigned long value;
		    int ok = 1;
		    /* Was constant expression already? */
		    if (ii->index == II_ADD && !ii->op[2])
		      continue;

		    if (ii->index == II_ADD)
		      value = ii->op[1] + ii->op[2];
		    else if (ii->index == II_SUB)
		      value = ii->op[1] - ii->op[2];
		    else if (ii->index == II_SLL)
		      value = ii->op[1] << ii->op[2];
		    else if (ii->index == II_SRL)
		      value = ii->op[1] >> ii->op[2];
		    else if (ii->index == II_MUL)
		      value = ii->op[1] * ii->op[2];
		    else if (ii->index == II_OR)
		      value = ii->op[1] | ii->op[2];
		    else if (ii->index == II_XOR)
		      value = ii->op[1] ^ ii->op[2];
		    else if (ii->index == II_AND)
		      value = ii->op[1] & ii->op[2];
		    else
		      ok = 0;
		    if (ok)
		      {
			change_insn_type (ii, II_ADD);
			ii->op[0] = -1;
			ii->opt[0] = OPT_REGISTER | OPT_DEST;
			ii->op[1] = value;
			ii->opt[1] = OPT_CONST;
			ii->op[2] = 0;
			ii->opt[2] = OPT_CONST;
			modified = 1;
			cucdebug (2, "%8x:const\n", REF (b, i));
		      }
		  }
		else if (ii->opt[1] & OPT_REF)
		  {
		    cuc_insn *prev = &f->INSN (ii->op[1]);
		    /* Is this just a move? */
		    if (ii->index == II_ADD
			&& !(ii->type & IT_MEMADD) && ii->op[2] == 0)
		      {
			int b1, i1, j1;
			cucdebug (2, "%8x:link      %8lx: ", REF (b, i),
				  ii->op[1]);
			if (!(prev->type & (IT_OUTPUT | IT_VOLATILE)))
			  {
			    assert (ii->opt[0] & OPT_DEST);
			    prev->op[0] = ii->op[0];
			    prev->opt[0] = ii->opt[0];
			    prev->type |= ii->type & IT_OUTPUT;
			    for (b1 = 0; b1 < f->num_bb; b1++)
			      if (!(f->bb[b1].type & BB_DEAD))
				for (i1 = 0; i1 < f->bb[b1].ninsn; i1++)
				  for (j1 = 0; j1 < MAX_OPERANDS; j1++)
				    if ((f->bb[b1].insn[i1].opt[j1] & OPT_REF)
					&& f->bb[b1].insn[i1].op[j1] ==
					REF (b, i))
				      {
					cucdebug (2, "%x ", REF (b1, i1));
					f->bb[b1].insn[i1].op[j1] = ii->op[1];
				      }
			    cucdebug (2, "\n");
			    change_insn_type (ii, II_NOP);
			  }
		      }
		    else if (prev->opt[2] & OPT_CONST)
		      {
			/* Handle some common cases */
			/* add - add joining */
			if (ii->index == II_ADD && prev->index == II_ADD)
			  {
			    ii->op[1] = prev->op[1];
			    ii->opt[1] = prev->opt[1];
			    ii->op[2] += prev->op[2];
			    modified = 1;
			    cucdebug (2, "%8x: add-add\n", REF (b, i));
			  }
			else /* add - sub joining */ if (ii->index == II_ADD
							 && prev->index ==
							 II_SUB)
			  {
			    change_insn_type (&insn[i], II_SUB);
			    ii->op[1] = prev->op[1];
			    ii->opt[1] = prev->opt[1];
			    ii->op[2] += prev->op[2];
			    modified = 1;
			    cucdebug (2, "%8x: add-sub\n", REF (b, i));
			  }
			else /* sub - add joining */ if (ii->index == II_SUB
							 && prev->index ==
							 II_ADD)
			  {
			    ii->op[1] = prev->op[1];
			    ii->opt[1] = prev->opt[1];
			    ii->op[2] += prev->op[2];
			    modified = 1;
			    cucdebug (2, "%8x: sub-add\n", REF (b, i));
			  }
			else /* add - sfxx joining */ if (prev->index ==
							  II_ADD && (
								       ii->
								       index
								       ==
								       II_SFEQ
								       || ii->
								       index
								       ==
								       II_SFNE
								       || ii->
								       index
								       ==
								       II_SFLT
								       || ii->
								       index
								       ==
								       II_SFLE
								       || ii->
								       index
								       ==
								       II_SFGT
								       || ii->
								       index
								       ==
								       II_SFGE))
			  {
			    if (ii->opt[2] & OPT_CONST
				&& ii->op[2] < 0x80000000)
			      {
				ii->op[1] = prev->op[1];
				ii->opt[1] = prev->opt[1];
				ii->op[2] -= prev->op[2];
				modified = 1;
				cucdebug (2, "%8x: add-sfxx\n", REF (b, i));
			      }
			  }
			else /* sub - sfxx joining */ if (prev->index ==
							  II_SUB && (
								       ii->
								       index
								       ==
								       II_SFEQ
								       || ii->
								       index
								       ==
								       II_SFNE
								       || ii->
								       index
								       ==
								       II_SFLT
								       || ii->
								       index
								       ==
								       II_SFLE
								       || ii->
								       index
								       ==
								       II_SFGT
								       || ii->
								       index
								       ==
								       II_SFGE))
			  {
			    if (ii->opt[2] & OPT_CONST
				&& ii->op[2] < 0x80000000)
			      {
				ii->op[1] = prev->op[1];
				ii->opt[1] = prev->opt[1];
				ii->op[2] += prev->op[2];
				modified = 1;
				cucdebug (2, "%8x: sub-sfxx\n", REF (b, i));
			      }
			  }
		      }
		  }
	      }
	  }
      if (modified)
	gmodified = 1;
    }
  while (modified);
  return gmodified;
}

/* Remove nop instructions */
int
remove_nops (cuc_func * f)
{
  int b;
  int modified = 0;
  for (b = 0; b < f->num_bb; b++)
    {
      int c, d = 0, i, j;
      cuc_insn *insn = f->bb[b].insn;
      for (i = 0; i < f->bb[b].ninsn; i++)
	if (insn[i].index != II_NOP)
	  {
	    reloc[i] = d;
	    insn[d++] = insn[i];
	  }
	else
	  {
	    reloc[i] = d;	/* For jumps only */
	  }
      if (f->bb[b].ninsn != d)
	modified = 1;
      f->bb[b].ninsn = d;

      /* Relocate references from all basic blocks */
      for (c = 0; c < f->num_bb; c++)
	for (i = 0; i < f->bb[c].ninsn; i++)
	  {
	    dep_list *d = f->bb[c].insn[i].dep;
	    for (j = 0; j < MAX_OPERANDS; j++)
	      if ((f->bb[c].insn[i].opt[j] & OPT_REF)
		  && REF_BB (f->bb[c].insn[i].op[j]) == b)
		f->bb[c].insn[i].op[j] =
		  REF (b, reloc[REF_I (f->bb[c].insn[i].op[j])]);

	    while (d)
	      {
		if (REF_BB (d->ref) == b)
		  d->ref = REF (b, reloc[REF_I (d->ref)]);
		d = d->next;
	      }
	  }
    }
  return modified;
}

static void
unmark_tree (cuc_func * f, int ref)
{
  cuc_insn *ii = &f->INSN (ref);
  cucdebug (5, "%x ", ref);
  if (ii->type & IT_UNUSED)
    {
      int j;
      ii->type &= ~IT_UNUSED;
      for (j = 0; j < MAX_OPERANDS; j++)
	if (ii->opt[j] & OPT_REF)
	  unmark_tree (f, ii->op[j]);
    }
}

/* Remove unused assignments */
int
remove_dead (cuc_func * f)
{
  int b, i;
  for (b = 0; b < f->num_bb; b++)
    for (i = 0; i < f->bb[b].ninsn; i++)
      f->bb[b].insn[i].type |= IT_UNUSED;

  for (b = 0; b < f->num_bb; b++)
    for (i = 0; i < f->bb[b].ninsn; i++)
      {
	cuc_insn *ii = &f->bb[b].insn[i];
	if (ii->type & IT_VOLATILE || ii->type & IT_OUTPUT
	    || (II_IS_LOAD (ii->index)
		&& (f->memory_order == MO_NONE || f->memory_order == MO_WEAK))
	    || II_IS_STORE (ii->index))
	  {
	    unmark_tree (f, REF (b, i));
	    cucdebug (5, "\n");
	  }
      }

  for (b = 0; b < f->num_bb; b++)
    for (i = 0; i < f->bb[b].ninsn; i++)
      if (f->bb[b].insn[i].type & IT_UNUSED)
	{
	  change_insn_type (&f->bb[b].insn[i], II_NOP);
	}

  return remove_nops (f);
}

/* Removes trivial register assignments */
int
remove_trivial_regs (cuc_func * f)
{
  int b, i;
  for (i = 0; i < MAX_REGS; i++)
    f->saved_regs[i] = caller_saved[i];

  for (b = 0; b < f->num_bb; b++)
    {
      cuc_insn *insn = f->bb[b].insn;
      for (i = 0; i < f->bb[b].ninsn; i++)
	{
	  if (insn[i].index == II_ADD
	      && insn[i].opt[0] & OPT_REGISTER
	      && insn[i].opt[1] & OPT_REGISTER
	      && insn[i].op[0] == insn[i].op[1] && insn[i].opt[2] & OPT_CONST
	      && insn[i].op[2] == 0)
	    {
	      if (insn[i].type & IT_OUTPUT)
		f->saved_regs[insn[i].op[0]] = 1;
	      change_insn_type (&insn[i], II_NOP);
	    }
	}
    }
  if (cuc_debug >= 2)
    {
      PRINTF ("saved regs ");
      for (i = 0; i < MAX_REGS; i++)
	PRINTF ("%i:%i ", i, f->saved_regs[i]);
      PRINTF ("\n");
    }
  return remove_nops (f);
}

/* Determine inputs and outputs */
void
set_io (cuc_func * f)
{
  int b, i, j;
  /* Determine register usage */
  for (i = 0; i < MAX_REGS; i++)
    {
      f->lur[i] = -1;
      f->used_regs[i] = 0;
    }
  if (cuc_debug > 5)
    print_cuc_bb (f, "SET_IO");
  for (b = 0; b < f->num_bb; b++)
    {
      for (i = 0; i < f->bb[b].ninsn; i++)
	for (j = 0; j < MAX_OPERANDS; j++)
	  if (f->bb[b].insn[i].opt[j] & OPT_REGISTER
	      && f->bb[b].insn[i].op[j] >= 0)
	    {
	      if (f->bb[b].insn[i].opt[j] & OPT_DEST)
		f->lur[f->bb[b].insn[i].op[j]] = REF (b, i);
	      else
		f->used_regs[f->bb[b].insn[i].op[j]] = 1;
	    }
    }
}

/* relocate all accesses inside of BB b to back/fwd */
#if 0
static void
relocate_bb (cuc_bb * bb, int b, int back, int fwd)
{
  int i, j;
  for (i = 0; i < bb->ninsn; i++)
    for (j = 0; j < MAX_OPERANDS; j++)
      if (bb->insn[i].opt[j] & OPT_REF && REF_BB (bb->insn[i].op[j]) == b)
	{
	  int t = REF_I (bb->insn[i].op[j]);
	  if (t < i)
	    bb->insn[i].op[j] = REF (back, t);
	  else
	    bb->insn[i].op[j] = REF (fwd, t);
	}
}
#endif

/* Latch outputs in loops */
void
add_latches (cuc_func * f)
{
  int b, i, j;

  //print_cuc_bb (f, "ADD_LATCHES a");
  /* Cuts the tree and marks registers */
  mark_cut (f);

  /* Split BBs with more than one group */
  for (b = 0; b < f->num_bb; b++)
    expand_bb (f, b);
  remove_nops (f);
  //print_cuc_bb (f, "ADD_LATCHES 0");

  /* Convert accesses in BB_INLOOP type block to latched */
  for (b = 0; b < f->num_bb; b++)
    {
      int j;
      for (i = 0; i < f->bb[b].ninsn; i++)
	for (j = 0; j < MAX_OPERANDS; j++)
	  if (f->bb[b].insn[i].opt[j] == OPT_REF)
	    {
	      int t = f->bb[b].insn[i].op[j];
	      /* If we are pointing to a INLOOP block from outside, or forward
	         (= previous loop iteration) we must register that data */
	      if ((f->bb[REF_BB (t)].type & BB_INLOOP
		   || config.cuc.no_multicycle)
		  && !(f->INSN (t).type & (IT_BRANCH | IT_COND))
		  && (REF_BB (t) != b || REF_I (t) >= i))
		{
		  f->INSN (t).type |= IT_LATCHED;
		}
	    }
    }
  //print_cuc_bb (f, "ADD_LATCHES 1");

  /* Add latches at the end of blocks as needed */
  for (b = 0; b < f->num_bb; b++)
    {
      int nreg = 0;
      cuc_insn *insn;
      for (i = 0; i < f->bb[b].ninsn; i++)
	if (f->bb[b].insn[i].type & IT_LATCHED)
	  nreg++;
      if (nreg)
	{
	  insn =
	    (cuc_insn *) malloc (sizeof (cuc_insn) * (f->bb[b].ninsn + nreg));
	  j = 0;
	  for (i = 0; i < f->bb[b].ninsn; i++)
	    {
	      insn[i] = f->bb[b].insn[i];
	      if (insn[i].type & IT_LATCHED)
		{
		  cuc_insn *ii = &insn[f->bb[b].ninsn + j++];
		  change_insn_type (ii, II_REG);
		  ii->op[0] = -1;
		  ii->opt[0] = OPT_DEST | OPT_REGISTER;
		  ii->op[1] = REF (b, i);
		  ii->opt[1] = OPT_REF;
		  ii->opt[2] = ii->opt[3] = OPT_NONE;
		  ii->dep = NULL;
		  ii->type = IT_VOLATILE;
		  sprintf (ii->disasm, "reg %i_%i", b, i);
		}
	    }
	  f->bb[b].ninsn += nreg;
	  free (f->bb[b].insn);
	  f->bb[b].insn = insn;
	}
    }
  //print_cuc_bb (f, "ADD_LATCHES 2");

  /* Repair references */
  for (b = 0; b < f->num_bb; b++)
    for (i = 0; i < f->bb[b].ninsn; i++)
      for (j = 0; j < MAX_OPERANDS; j++)
	/* If destination instruction is latched, use register instead */
	if (f->bb[b].insn[i].opt[j] == OPT_REF
	    && f->INSN (f->bb[b].insn[i].op[j]).type & IT_LATCHED)
	  {
	    int b1, i1;
	    b1 = REF_BB (f->bb[b].insn[i].op[j]);
	    //cucdebug (2, "%i.%i.%i %x\n", b, i, j, f->bb[b].insn[i].op[j]);
	    if (b1 != b || REF_I (f->bb[b].insn[i].op[j]) >= i)
	      {
		for (i1 = f->bb[b1].ninsn - 1; i1 >= 0; i1--)
		  {
		    assert (f->bb[b1].insn[i1].index == II_REG);
		    if (f->bb[b1].insn[i1].op[1] == f->bb[b].insn[i].op[j])
		      {
			f->bb[b].insn[i].op[j] = REF (b1, i1);
			break;
		      }
		  }
	      }
	  }
}

/* CSE -- common subexpression elimination */
int
cse (cuc_func * f)
{
  int modified = 0;
  int b, i, j, b1, i1, b2, i2;
  for (b1 = 0; b1 < f->num_bb; b1++)
    for (i1 = 0; i1 < f->bb[b1].ninsn; i1++)
      if (f->bb[b1].insn[i1].index != II_NOP
	  && f->bb[b1].insn[i1].index != II_LRBB
	  && !(f->bb[b1].insn[i1].type & IT_MEMORY)
	  && !(f->bb[b1].insn[i1].type & IT_MEMADD))
	for (b2 = 0; b2 < f->num_bb; b2++)
	  for (i2 = 0; i2 < f->bb[b2].ninsn; i2++)
	    if (f->bb[b2].insn[i2].index != II_NOP
		&& f->bb[b2].insn[i2].index != II_LRBB
		&& !(f->bb[b2].insn[i2].type & IT_MEMORY)
		&& !(f->bb[b2].insn[i2].type & IT_MEMADD) && (b1 != b2
							      || i2 > i1))
	      {
		cuc_insn *ii1 = &f->bb[b1].insn[i1];
		cuc_insn *ii2 = &f->bb[b2].insn[i2];
		int ok = 1;

		/* Do we have an exact match? */
		if (ii1->index != ii2->index)
		  continue;
		if (ii2->type & IT_VOLATILE)
		  continue;

		/* Check all operands also */
		for (j = 0; j < MAX_OPERANDS; j++)
		  {
		    if (ii1->opt[j] != ii2->opt[j])
		      {
			ok = 0;
			break;
		      }
		    if (ii1->opt[j] & OPT_DEST)
		      continue;
		    if (ii1->opt[j] != OPT_NONE && ii1->op[j] != ii2->op[j])
		      {
			ok = 0;
			break;
		      }
		  }

		if (ok)
		  {
		    /* remove duplicated instruction and relink the references */
		    cucdebug (3, "%x - %x are same\n", REF (b1, i1),
			      REF (b2, i2));
		    change_insn_type (ii2, II_NOP);
		    modified = 1;
		    for (b = 0; b < f->num_bb; b++)
		      for (i = 0; i < f->bb[b].ninsn; i++)
			for (j = 0; j < MAX_OPERANDS; j++)
			  if (f->bb[b].insn[i].opt[j] & OPT_REF
			      && f->bb[b].insn[i].op[j] == REF (b2, i2))
			    f->bb[b].insn[i].op[j] = REF (b1, i1);
		  }
	      }
  return modified;
}

static int
count_cmovs (cuc_insn * ii, int match)
{
  int c = 0, j;
  if (match & 2)
    {
      for (j = 0; j < MAX_OPERANDS; j++)
	if (ii->opt[j] & OPT_DEST)
	  c++;
    }
  if (match & 1)
    {
      for (j = 0; j < MAX_OPERANDS; j++)
	if (!(ii->opt[j] & OPT_DEST) && ii->opt[j] & OPT_REF)
	  c++;
    }
  else
    {
      for (j = 0; j < MAX_OPERANDS; j++)
	if (!(ii->opt[j] & OPT_DEST) && ii->opt[j] != OPT_NONE)
	  c++;
    }
  return c;
}

static void search_csm (int iter, cuc_func * f, cuc_shared_list * list);
static cuc_shared_list *main_list;
static int *iteration;

/* CSM -- common subexpression matching -- resource sharing
   We try to match tree of instruction inside a BB with as many
   matches as possible. All possibilities are collected and
   options, making situation worse are removed */
void
csm (cuc_func * f)
{
  int b, i, j;
  int cnt;
  cuc_shared_list *list;
  cuc_timings timings;

  analyse_timings (f, &timings);
  main_list = NULL;
  for (b = 0; b < f->num_bb; b++)
    {
      assert (iteration = (int *) malloc (sizeof (int) * f->bb[b].ninsn));
      for (i = 0; i < f->bb[b].ninsn; i++)
	{
	  int cnt = 0, cntc = 0;
	  double size = 0., sizec = 0.;
	  int j2 = 0;
	  for (j = 0; j < f->bb[b].ninsn; j++)
	    if (f->bb[b].insn[i].index == f->bb[b].insn[j].index)
	      {
		int ok = 1;
		for (j2 = 0; j2 < MAX_OPERANDS; j2++)
		  if (!(f->bb[b].insn[j].opt[j2] & OPT_REF))
		    if (f->bb[b].insn[j].opt[j2] != f->bb[b].insn[i].opt[j2]
			|| f->bb[b].insn[j].op[j2] !=
			f->bb[b].insn[i].opt[j2])
		      {
			ok = 0;
			break;
		      }
		if (ok)
		  {
		    cntc++;
		    sizec = sizec + insn_size (&f->bb[b].insn[j]);
		  }
		else
		  {
		    cnt++;
		    size = size + insn_size (&f->bb[b].insn[j]);
		  }
		iteration[j] = 0;
	      }
	    else
	      iteration[j] = -1;
	  if (cntc > 1)
	    {
	      assert (list =
		      (cuc_shared_list *) malloc (sizeof (cuc_shared_list)));
	      list->next = main_list;
	      list->from = NULL;
	      list->ref = REF (b, i);
	      list->cnt = cnt;
	      list->cmatch = 1;
	      list->cmovs = count_cmovs (&f->bb[b].insn[i], 3);
	      list->osize = sizec;
	      list->size = ii_size (f->bb[b].insn[i].index, 1);
	      main_list = list;
	      search_csm (0, f, list);
	    }
	  if (cnt > 1)
	    {
	      assert (list =
		      (cuc_shared_list *) malloc (sizeof (cuc_shared_list)));
	      list->next = main_list;
	      list->from = NULL;
	      list->ref = REF (b, i);
	      list->cnt = cnt + cntc;
	      list->cmatch = 0;
	      list->cmovs = count_cmovs (&f->bb[b].insn[i], 2);
	      list->osize = size + sizec;
	      list->size = ii_size (f->bb[b].insn[i].index, 0);
	      main_list = list;
	      search_csm (0, f, list);
	    }
	}
      free (iteration);
    }

  for (list = main_list; list; list = list->next)
    list->dead = 0;
  cnt = 0;
  for (list = main_list; list; list = list->next)
    if (!list->dead)
      cnt++;
  cucdebug (1, "noptions = %i\n", cnt);

  /* Now we will check the real size of the 'improvements'; if the size
     actually increases, we abandom the option */
  for (list = main_list; list; list = list->next)
    if (list->cmovs * ii_size (II_CMOV, 0) * (list->cnt - 1) + list->size >=
	list->osize)
      list->dead = 1;

  cnt = 0;
  for (list = main_list; list; list = list->next)
    if (!list->dead)
      cnt++;
  cucdebug (1, "noptions = %i\n", cnt);

  /* Count number of instructions grouped */
  for (list = main_list; list; list = list->next)
    {
      cuc_shared_list *l = list;
      int c = 0;
      while (l)
	{
	  c++;
	  if (f->INSN (l->ref).type & (IT_VOLATILE | IT_MEMORY | IT_MEMADD))
	    list->dead = 1;
	  l = l->from;
	}
      list->ninsn = c;
    }

  cnt = 0;
  for (list = main_list; list; list = list->next)
    if (!list->dead)
      cnt++;
  cucdebug (1, "noptions = %i\n", cnt);

#if 1
  /* We can get a lot of options here, so we will delete duplicates */
  for (list = main_list; list; list = list->next)
    if (!list->dead)
      {
	cuc_shared_list *l;
	for (l = list->next; l; l = l->next)
	  if (!l->dead)
	    {
	      int ok = 1;
	      cuc_shared_list *t1 = list;
	      cuc_shared_list *t2 = l;
	      while (ok && t1 && t2)
		{
		  if (f->INSN (t1->ref).index == f->INSN (t2->ref).index)
		    {
		      /* If other operands are matching, we must check for them also */
		      if (t1->cmatch)
			{
			  int j;
			  for (j = 0; j < MAX_OPERANDS; j++)
			    if (!(f->INSN (t1->ref).opt[j] & OPT_REF)
				|| !(f->INSN (t2->ref).opt[j] & OPT_REF)
				|| f->INSN (t1->ref).opt[j] !=
				f->INSN (t2->ref).opt[j]
				|| f->INSN (t1->ref).op[j] !=
				f->INSN (t2->ref).op[j])
			      {
				ok = 0;
				break;
			      }
			}

		      /* This option is duplicate, remove */
		      if (ok)
			t1->dead = 1;
		    }
		  t1 = t1->from;
		  t2 = t2->from;
		}
	    }
      }
  cnt = 0;
  for (list = main_list; list; list = list->next)
    if (!list->dead)
      cnt++;
  cucdebug (1, "noptions = %i\n", cnt);
#endif
  /* Print out */
  for (list = main_list; list; list = list->next)
    if (!list->dead)
      {
	cuc_shared_list *l = list;
	cucdebug (1,
		  "%-4s cnt %3i ninsn %3i size %8.1f osize %8.1f cmovs %3i @",
		  cuc_insn_name (&f->INSN (list->ref)), list->cnt,
		  list->ninsn, list->cmovs * ii_size (II_CMOV,
						      0) * (list->cnt - 1) +
		  list->size, list->osize, list->cmovs);
	while (l)
	  {
	    cucdebug (1, "%c%x,", l->cmatch ? '.' : '!', l->ref);
	    l = l->from;
	  }
	cucdebug (1, "\n");
      }

  /* Calculate estimated timings */
  for (b = 0; b < f->num_bb; b++)
    {
      cnt = 0;
      for (list = main_list; list; list = list->next)
	if (!list->dead && REF_BB (list->ref) == b)
	  cnt++;

      f->bb[b].ntim = cnt;
      if (!cnt)
	{
	  f->bb[b].tim = NULL;
	  continue;
	}
      assert (f->bb[b].tim =
	      (cuc_timings *) malloc (sizeof (cuc_timings) * cnt));

      cnt = 0;
      for (list = main_list; list; list = list->next)
	if (!list->dead && REF_BB (list->ref) == b)
	  {
	    cuc_shared_list *l = list;
	    f->bb[b].tim[cnt].b = b;
	    f->bb[b].tim[cnt].preroll = f->bb[b].tim[cnt].unroll = 1;
	    f->bb[b].tim[cnt].nshared = list->ninsn;
	    assert (f->bb[b].tim[cnt].shared = (cuc_shared_item *)
		    malloc (sizeof (cuc_shared_item) * list->ninsn));
	    for (i = 0; i < list->ninsn; i++, l = l->from)
	      {
		f->bb[b].tim[cnt].shared[i].ref = l->ref;
		f->bb[b].tim[cnt].shared[i].cmatch = l->cmatch;
	      }
	    f->bb[b].tim[cnt].new_time =
	      timings.new_time + f->bb[b].cnt * (list->cnt - 1);
	    f->bb[b].tim[cnt].size =
	      timings.size + list->cmovs * ii_size (II_CMOV,
						    0) * (list->cnt - 1) +
	      list->size - list->osize;
	    cnt++;
	  }
    }
}

/* Recursive function for searching through instruction graph */
static void
search_csm (int iter, cuc_func * f, cuc_shared_list * list)
{
  int b, i, j, i1;
  cuc_shared_list *l;
  b = REF_BB (list->ref);
  i = REF_I (list->ref);

  for (j = 0; j < MAX_OPERANDS; j++)
    if (f->bb[b].insn[i].opt[j] & OPT_REF)
      {
	int t = f->bb[b].insn[i].op[j];
	int cnt = 0, cntc = 0;
	double size = 0., sizec = 0.;

	/* Mark neighbours */
	for (i1 = 0; i1 < f->bb[b].ninsn; i1++)
	  {
	    if (iteration[i1] == iter && f->bb[b].insn[i1].opt[j] & OPT_REF)
	      {
		int t2 = f->bb[b].insn[i1].op[j];
		if (f->INSN (t).index == f->INSN (t2).index
		    && f->INSN (t2).opt[j] & OPT_REF)
		  {
		    int j2;
		    int ok = 1;
		    iteration[REF_I (t2)] = iter + 1;
		    for (j2 = 0; j2 < MAX_OPERANDS; j2++)
		      if (!(f->bb[b].insn[i1].opt[j2] & OPT_REF))
			if (f->bb[b].insn[i1].opt[j2] !=
			    f->bb[b].insn[i].opt[j2]
			    || f->bb[b].insn[i1].op[j2] !=
			    f->bb[b].insn[i].opt[j2])
			  {
			    ok = 0;
			    break;
			  }
		    if (ok)
		      {
			cntc++;
			sizec = sizec + insn_size (&f->bb[b].insn[i1]);
		      }
		    else
		      {
			cnt++;
			size = size + insn_size (&f->bb[b].insn[i1]);
		      }
		  }
	      }
	  }

	if (cntc > 1)
	  {
	    assert (l =
		    (cuc_shared_list *) malloc (sizeof (cuc_shared_list)));
	    l->next = main_list;
	    main_list = l;
	    l->from = list;
	    l->ref = t;
	    l->cnt = cnt;
	    l->cmatch = 1;
	    l->cmovs = list->cmovs + count_cmovs (&f->bb[b].insn[i], 1) - 1;
	    l->size = list->size + ii_size (f->bb[b].insn[i].index, 1);
	    l->osize = sizec;
	    search_csm (iter + 1, f, l);
	  }
	if (cnt > 1)
	  {
	    assert (l =
		    (cuc_shared_list *) malloc (sizeof (cuc_shared_list)));
	    l->next = main_list;
	    main_list = l;
	    l->from = list;
	    l->ref = t;
	    l->cnt = cnt + cntc;
	    l->cmatch = 0;
	    l->osize = size + sizec;
	    l->cmovs = list->cmovs + count_cmovs (&f->bb[b].insn[i], 0) - 1;
	    l->size = list->size + ii_size (f->bb[b].insn[i].index, 0);
	    search_csm (iter + 1, f, l);
	  }

	/* Unmark them back */
	for (i1 = 0; i1 < f->bb[b].ninsn; i1++)
	  if (iteration[i1] > iter)
	    iteration[i1] = -1;
      }
}

/* Displays shared instructions */
void
print_shared (cuc_func * rf, cuc_shared_item * shared, int nshared)
{
  int i, first = 1;
  for (i = 0; i < nshared; i++)
    {
      PRINTF ("%s%s%s", first ? "" : "-",
	      cuc_insn_name (&rf->INSN (shared[i].ref)),
	      shared[i].cmatch ? "!" : "");
      first = 0;
    }
}

/* Common subexpression matching -- resource sharing, generation pass

   Situation here is much simpler than with analysis -- we know the instruction sequence
   we are going to share, but we are going to do this on whole function, not just one BB.
   We can find sequence in reference function, as pointed from "shared" */
void
csm_gen (cuc_func * f, cuc_func * rf, cuc_shared_item * shared, int nshared)
{
  int b, i, cnt = 0;
  /* FIXME: some code here (2) */
  PRINTF ("Replacing: ");
  print_shared (rf, shared, nshared);

  for (b = 0; b < f->num_bb; b++)
    for (i = 0; i < f->bb[b].ninsn; i++)
      {
      }

  PRINTF ("\nFound %i matches.\n", cnt);
}
