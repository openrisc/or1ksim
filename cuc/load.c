/* load.c -- OpenRISC Custom Unit Compiler, instruction loading and converting
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
#include "opcode/or32.h"
#include "insn.h"

static const cuc_conv conv[] = {
{"l.add", II_ADD}, {"l.addi", II_ADD},
{"l.movhi", II_OR},
{"l.sub", II_SUB}, {"l.subi", II_SUB},
{"l.and", II_AND}, {"l.andi", II_AND},
{"l.xor", II_XOR}, {"l.xori", II_XOR},
{"l.or",  II_OR }, {"l.ori",  II_OR},
{"l.mul", II_MUL}, {"l.muli", II_MUL},

{"l.sra", II_SRA}, {"l.srai", II_SRA},
{"l.srl", II_SRL}, {"l.srli", II_SRL},
{"l.sll", II_SLL}, {"l.slli", II_SLL},

{"l.lbz",II_LB | II_MEM}, {"l.lbs", II_LB | II_MEM | II_SIGNED},
{"l.lhz",II_LH | II_MEM}, {"l.lhs", II_LH | II_MEM | II_SIGNED},
{"l.lwz",II_LW | II_MEM}, {"l.lws", II_LW | II_MEM | II_SIGNED},
{"l.sb", II_SB | II_MEM}, {"l.sh",  II_SH | II_MEM}, {"l.sw", II_SW | II_MEM},
{"l.sfeq",  II_SFEQ }, {"l.sfeqi", II_SFEQ},
{"l.sfne",  II_SFNE }, {"l.sfnei", II_SFNE},
{"l.sflts", II_SFLT | II_SIGNED}, {"l.sfltis", II_SFLT | II_SIGNED},
{"l.sfltu", II_SFLT}, {"l.sfltiu", II_SFLT},
{"l.sfgts", II_SFGT | II_SIGNED}, {"l.sfgtis", II_SFGT | II_SIGNED},
{"l.sfgtu", II_SFGT}, {"l.sfgtiu", II_SFGT},
{"l.sfges", II_SFGE | II_SIGNED}, {"l.sfgeis", II_SFGE | II_SIGNED},
{"l.sfgeu", II_SFGE}, {"l.sfgeiu", II_SFGE},
{"l.sfles", II_SFLE | II_SIGNED}, {"l.sfleis", II_SFLE | II_SIGNED},
{"l.sfleu", II_SFLE}, {"l.sfleiu", II_SFLE},
{"l.j",     II_BF   },
{"l.bf",    II_BF   },
{"l.jal",   II_CALL },
{"l.nop",   II_NOP  }
};

/* Instructions from function */
cuc_insn insn[MAX_INSNS];
int num_insn;
int reloc[MAX_INSNS];

/* Prints out instructions */
void print_cuc_insns (char *s, int verbose)
{
  PRINTF ("****************** %s ******************\n", s);
  print_insns (0, insn, num_insn,verbose);
  PRINTF ("\n\n");
}

void xchg_insn (int i, int j)
{
  cuc_insn t;
  t = insn[i];
  insn[i] = insn[j];
  insn[j] = t;
}

/* Negates conditional instruction */
void negate_conditional (cuc_insn *ii)
{
  assert (ii->type & IT_COND);
 
  if (ii->index == II_SFEQ) change_insn_type (ii, II_SFNE);
  else if (ii->index == II_SFNE) change_insn_type (ii, II_SFEQ);
  else if (ii->index == II_SFLT) change_insn_type (ii, II_SFGE);
  else if (ii->index == II_SFGT) change_insn_type (ii, II_SFLE);
  else if (ii->index == II_SFLE) change_insn_type (ii, II_SFGT);
  else if (ii->index == II_SFGE) change_insn_type (ii, II_SFLT);
  else assert (0);
}

/* Remove delay slots */
void remove_dslots ()
{
  int i;
  int in_delay = 0;
  for (i = 0; i < num_insn; i++) {
    if (in_delay) insn[i].type |= IT_INDELAY;
    in_delay = 0;
    if (insn[i].type & IT_BRANCH) in_delay = 1;
    if (insn[i].type & IT_INDELAY) {
      cuc_insn *ii;
      cuc_insn *bi;
      assert (i >= 2);
      ii = &insn[i - 2];
      bi = &insn[i - 1];
      /* delay slot should not be a branch target! */
      assert ((insn[i].type & IT_BBSTART) == 0);
      assert ((bi->type & IT_INDELAY) == 0);
      insn[i].type &= ~IT_INDELAY; /* no more in delay slot */

      /* Get the value we need before the actual jump */
      if (bi->opt[1] & OPT_REGISTER && bi->op[1] >= 0) {
        int r = bi->op[1];
        assert (ii->index == II_NOP);
        change_insn_type (ii, II_ADD);      
        ii->type = IT_COND;
        ii->dep = NULL;
        ii->op[0] = r; ii->opt[0] = OPT_REGISTER | OPT_DEST;
        ii->op[1] = r; ii->opt[1] = OPT_REGISTER;
        ii->op[2] = 0; ii->opt[2] = OPT_CONST;
        ii->opt[3] = OPT_NONE;
        bi->op[1] = i - 2; bi->opt[1] = OPT_REF;
      }
      xchg_insn (i, i - 1);
    }
  }
  assert (in_delay == 0);
}

/* Convert local variables (uses stack frame -- r1) to internal values */
void detect_locals ()
{
  int stack[CUC_MAX_STACK];
  int i, can_remove_stack = 1;
  int real_stack_size = 0;

  for (i = 0; i < CUC_MAX_STACK; i++) stack[i] = -1;
  
  for (i = 0; i < num_insn; i++) {
    /* sw off (r1),rx */
    if (insn[i].index == II_SW
      && (insn[i].opt[0] & OPT_CONST)
      && insn[i].op[1] == 1 && (insn[i].opt[1] & OPT_REGISTER)) {

      if (insn[i].op[0] < CUC_MAX_STACK/* && insn[i].op[1] >= 4*/) { /* Convert to normal move */
        stack[insn[i].op[0]] = i;
        insn[i].type &= IT_INDELAY | IT_BBSTART;
        change_insn_type (&insn[i], II_ADD);
        insn[i].op[0] = -1; insn[i].opt[0] = OPT_REGISTER | OPT_DEST; 
        insn[i].op[1] = insn[i].op[2]; insn[i].opt[1] = insn[i].opt[2];
        insn[i].op[2] = 0; insn[i].opt[2] = OPT_CONST;
      } else can_remove_stack = 0;
    /* lw rx,off (r1) */
    } else if (insn[i].index == II_LW
      && (insn[i].opt[1] & OPT_CONST)
      && insn[i].op[2] == 1 && (insn[i].opt[2] & OPT_REGISTER)) {
            
      if (insn[i].op[1] < CUC_MAX_STACK && stack[insn[i].op[1]] >= 0) { /* Convert to normal move */
        insn[i].type &= IT_INDELAY | IT_BBSTART;
        change_insn_type (&insn[i], II_ADD);
        insn[i].op[1] = stack[insn[i].op[1]]; insn[i].opt[1] = OPT_REF;
        insn[i].op[2] = 0; insn[i].opt[2] = OPT_CONST;
      } else can_remove_stack = 0;
    /* Check for defined stack size */
    } else if (insn[i].index == II_ADD && !real_stack_size
            && (insn[i].opt[0] & OPT_REGISTER) && insn[i].op[0] == 1
            && (insn[i].opt[1] & OPT_REGISTER) && insn[i].op[1] == 1
            && (insn[i].opt[2] & OPT_CONST)) {
      real_stack_size = -insn[i].op[2];
    }
  } 
  //assert (can_remove_stack); /* TODO */  
}

/* Disassemble one instruction from insn index and generate parameters */
const char *build_insn (unsigned long data, cuc_insn *insn)
{
  const char *name;
  char *s;
  int index = or1ksim_insn_decode (data);
  struct or32_opcode const *opcode;
  int i, argc = 0;

  insn->insn = data;
  insn->index = -1;
  insn->type = 0;
  name = or1ksim_insn_name (index);
  insn->index = index;
  or1ksim_disassemble_index (data, index);
  strcpy (insn->disasm, or1ksim_disassembled);
  insn->dep = NULL;
  for (i = 0; i < MAX_OPERANDS; i++) insn->opt[i] = OPT_NONE;
  
  if (index < 0) {
    fprintf (stderr, "Invalid opcode 0x%08lx!\n", data);
    exit (1);
  }
  opcode = &or1ksim_or32_opcodes[index];

  for (s = opcode->args; *s != '\0'; ++s) {
    switch (*s) {
    case '\0': return name;
    case 'r':
      insn->opt[argc] = OPT_REGISTER | (argc ? 0 : OPT_DEST);
      insn->op[argc++] = or1ksim_or32_extract(*++s, opcode->encoding, data);
      break;

    default:
      if (strchr (opcode->encoding, *s)) {
        unsigned long imm = or1ksim_or32_extract (*s, opcode->encoding, data);
        imm = or1ksim_extend_imm(imm, *s);
        insn->opt[argc] = OPT_CONST;
        insn->op[argc++] = imm;
      }
    }
  }
  return name;
}

/* inserts nop before branch */
void expand_branch ()
{
  int i, j, num_bra = 0, d;
  for (i = 0; i < num_insn; i++) if (insn[i].type & IT_BRANCH) num_bra++;

  d = num_insn + 2 * num_bra;
  assert (d < MAX_INSNS);
  
  /* Add nop before branch */
  for (i = num_insn - 1; i >= 0; i--) if (insn[i].type & IT_BRANCH) {
    insn[--d] = insn[i]; // for delay slot (later)
    if (insn[d].opt[1] & OPT_REGISTER) {
      assert (insn[d].op[1] == FLAG_REG);
      insn[d].op[1] = i; insn[d].opt[1] = OPT_REF;
    }
    insn[--d] = insn[i]; // for branch
    change_insn_type (&insn[d], II_NOP);
    insn[--d] = insn[i]; // save flag & negation of conditional, if required
    change_insn_type (&insn[d], II_CMOV);
    insn[d].op[0] = -1; insn[d].opt[0] = OPT_REGISTER | OPT_DEST;
    insn[d].op[1] = insn[d].type & IT_FLAG1 ? 0 : 1; insn[d].opt[1] = OPT_CONST;
    insn[d].op[2] = insn[d].type & IT_FLAG1 ? 1 : 0; insn[d].opt[2] = OPT_CONST;
    insn[d].op[3] = FLAG_REG; insn[d].opt[3] = OPT_REGISTER;
    insn[d].type = IT_COND;
    if (insn[d].type)
    reloc[i] = d;
  } else {
    insn[--d] = insn[i];
    reloc[i] = d;
  }
  num_insn += 2 * num_bra;
  for (i = 0; i < num_insn; i++)
    for (j = 0; j < MAX_OPERANDS; j++)
      if (insn[i].opt[j] & OPT_REF || insn[i].opt[j] & OPT_JUMP)
        insn[i].op[j] = reloc[insn[i].op[j]];
}

/* expands immediate memory instructions to two */
void expand_memory ()
{
  int i, j, num_mem = 0, d;
  for (i = 0; i < num_insn; i++) if (insn[i].type & IT_MEMORY) num_mem++;

  d = num_insn + num_mem;
  assert (d < MAX_INSNS);
  
  /* Split memory commands */
  for (i = num_insn - 1; i >= 0; i--) if (insn[i].type & IT_MEMORY) {
    insn[--d] = insn[i];
    insn[--d] = insn[i];
    reloc[i] = d;
    switch (insn[d].index) {
    case II_SW:
    case II_SH:
    case II_SB:
              insn[d + 1].op[1] = d; insn[d + 1].opt[1] = OPT_REF; /* sw rx,(t($-1)) */
              insn[d + 1].op[0] = insn[i].op[2]; insn[d + 1].opt[0] = insn[d + 1].opt[2];
              insn[d + 1].opt[2] = OPT_NONE;
              insn[d + 1].type &= ~IT_BBSTART;
              insn[d].op[2] = insn[d].op[0]; insn[d].opt[2] = insn[d].opt[0];
              insn[d].op[0] = -1; insn[d].opt[0] = OPT_REGISTER | OPT_DEST; /* add rd, ra, rb */
              insn[d].opt[3] = OPT_NONE;
              insn[d].type &= IT_INDELAY | IT_BBSTART;
              insn[d].type |= IT_MEMADD;
              change_insn_type (&insn[d], II_ADD);
              break;
    case II_LW:
    case II_LH:
    case II_LB:
              insn[d].op[0] = -1; insn[d].opt[0] = OPT_REGISTER | OPT_DEST; /* add rd, ra, rb */
              insn[d].type &= IT_INDELAY | IT_BBSTART;
              insn[d].type |= IT_MEMADD;
              change_insn_type (&insn[d], II_ADD);
              insn[d + 1].op[1] = d; insn[d + 1].opt[1] = OPT_REF; /* lw (t($-1)),rx */
              insn[d + 1].opt[2] = OPT_NONE;
              insn[d + 1].opt[3] = OPT_NONE;
              insn[d + 1].type &= ~IT_BBSTART;
              break;
    default:  fprintf (stderr, "%4i, %4i: %s\n", i, d, cuc_insn_name (&insn[d]));
              assert (0);
    }
  } else {
    insn[--d] = insn[i];
    reloc[i] = d;
  }
  num_insn += num_mem;
  for (i = 0; i < num_insn; i++) if (!(insn[i].type & IT_MEMORY))
    for (j = 0; j < MAX_OPERANDS; j++)
      if (insn[i].opt[j] & OPT_REF || insn[i].opt[j] & OPT_JUMP)
        insn[i].op[j] = reloc[insn[i].op[j]];
}

/* expands signed comparisons to three instructions */
void expand_signed ()
{
  int i, j, num_sig = 0, d;
  for (i = 0; i < num_insn; i++)
    if (insn[i].type & IT_SIGNED && !(insn[i].type & IT_MEMORY)) num_sig++;

  d = num_insn + num_sig * 2;
  assert (d < MAX_INSNS);
  
  /* Split signed instructions */
  for (i = num_insn - 1; i >= 0; i--)
    /* We will expand signed memory later */
    if (insn[i].type & IT_SIGNED && !(insn[i].type & IT_MEMORY)) {
      insn[--d] = insn[i];
      insn[d].op[1] = d - 2; insn[d].opt[1] = OPT_REF;
      insn[d].op[2] = d - 1; insn[d].opt[2] = OPT_REF;

      insn[--d] = insn[i];
      change_insn_type (&insn[d], II_ADD);
      insn[d].type = 0;
      insn[d].op[0] = -1; insn[d].opt[0] = OPT_REGISTER | OPT_DEST;
      insn[d].op[1] = insn[d].op[2]; insn[d].opt[1] = insn[d].opt[2];
      insn[d].op[2] = 0x80000000; insn[d].opt[2] = OPT_CONST;
      insn[d].opt[3] = OPT_NONE;

      insn[--d] = insn[i];
      change_insn_type (&insn[d], II_ADD);
      insn[d].type = 0;
      insn[d].op[0] = -1; insn[d].opt[0] = OPT_REGISTER | OPT_DEST;
      insn[d].op[1] = insn[d].op[1]; insn[d].opt[1] = insn[d].opt[1];
      insn[d].op[2] = 0x80000000; insn[d].opt[2] = OPT_CONST;
      insn[d].opt[3] = OPT_NONE;

      reloc[i] = d;
    } else {
      insn[--d] = insn[i];
      reloc[i] = d;
    }
  num_insn += num_sig * 2;
  for (i = 0; i < num_insn; i++) if (insn[i].type & IT_MEMORY || !(insn[i].type & IT_SIGNED)) {
    for (j = 0; j < MAX_OPERANDS; j++)
      if (insn[i].opt[j] & OPT_REF || insn[i].opt[j] & OPT_JUMP)
        insn[i].op[j] = reloc[insn[i].op[j]];
  } else insn[i].type &= ~IT_SIGNED;
}

/* expands calls to 7 instructions */
void expand_calls ()
{
  int i, j, num_call = 0, d;
  for (i = 0; i < num_insn; i++)
    if (insn[i].index == II_CALL) num_call++;

  d = num_insn + num_call * 6; /* 6 parameters */
  assert (d < MAX_INSNS);
  
  /* Split call instructions */
  for (i = num_insn - 1; i >= 0; i--)
    /* We will expand signed memory later */
    if (insn[i].index == II_CALL) {
      insn[--d] = insn[i];
      insn[d].op[0] = insn[d].op[1]; insn[d].opt[0] = OPT_CONST;
      insn[d].opt[1] = OPT_NONE;
      insn[d].type |= IT_VOLATILE;

      for (j = 0; j < 6; j++) {
        insn[--d] = insn[i];
        change_insn_type (&insn[d], II_ADD);
        insn[d].type = IT_VOLATILE;
        insn[d].op[0] = 3 + j; insn[d].opt[0] = OPT_REGISTER | OPT_DEST;
        insn[d].op[1] = 3 + j; insn[d].opt[1] = OPT_REGISTER;
        insn[d].op[2] = 0x80000000; insn[d].opt[2] = OPT_CONST;
        insn[d].opt[3] = OPT_NONE;
      }

      reloc[i] = d;
    } else {
      insn[--d] = insn[i];
      reloc[i] = d;
    }
  num_insn += num_call * 6;
  for (i = 0; i < num_insn; i++)
    for (j = 0; j < MAX_OPERANDS; j++)
      if (insn[i].opt[j] & OPT_REF || insn[i].opt[j] & OPT_JUMP)
        insn[i].op[j] = reloc[insn[i].op[j]];
}

/* Loads function from file into global array insn.
   Function returns nonzero if function cannot be converted. */
int cuc_load (char *in_fn)
{
  int i, j;
  FILE *fi;
  int func_return = 0;
  num_insn = 0;
  
  log ("Loading filename %s\n", in_fn);
  if ((fi = fopen (in_fn, "rt")) == NULL) {
    fprintf (stderr, "Cannot open '%s'\n", in_fn);
    exit (1);
  }
  /* Read in the function and decode the instructions */
  for (i = 0;; i++) {
    unsigned long data;
    const char *name;

    if (fscanf (fi, "%08lx\n", &data) != 1) break;
    
    /* build params */
    name = build_insn (data, &insn[i]);
    if (func_return) func_return++;
    //PRINTF ("%s\n", name);

    if (or1ksim_or32_opcodes[insn[i].index].flags & OR32_IF_DELAY) {
      int f;
      if (strcmp (name, "l.bnf") == 0) f = 1;
      else if (strcmp (name, "l.bf") == 0) f = 0;
      else if (strcmp (name, "l.j") == 0) {
	f = -1;
      } else if (strcmp (name, "l.jr") == 0 && func_return == 0) {
        func_return = 1;
        change_insn_type (&insn[i], II_NOP);
        continue;
      } else {
        cucdebug (1, "Instruction #%i: \"%s\" not supported.\n", i, name);
        log ("Instruction #%i: \"%s\" not supported.\n", i, name);
        return 1;
      }
      if (f < 0) { /* l.j */
	/* repair params */
        change_insn_type (&insn[i], II_BF);
        insn[i].op[0] = i + insn[i].op[0]; insn[i].opt[0] = OPT_JUMP;
        insn[i].op[1] = 1; insn[i].opt[1] = OPT_CONST;
        insn[i].type |= IT_BRANCH | IT_VOLATILE;
      } else {
        change_insn_type (&insn[i], II_BF);
        insn[i].op[0] = i + insn[i].op[0]; insn[i].opt[0] = OPT_JUMP;
        insn[i].op[1] = FLAG_REG; insn[i].opt[1] = OPT_REGISTER;
        insn[i].type |= IT_BRANCH | IT_VOLATILE;
        if (f) insn[i].type |= IT_FLAG1;
      }
    } else {
      insn[i].index = -1;
      for (j = 0; j < sizeof (conv) / sizeof (cuc_conv); j++)
        if (strcmp (conv[j].from, name) == 0) {
	  if (conv[j].to & II_SIGNED) insn[i].type |= IT_SIGNED;
	  if (conv[j].to & II_MEM) insn[i].type |= IT_MEMORY | IT_VOLATILE;
          change_insn_type (&insn[i], conv[j].to & II_MASK);
          break;
        }
      if (strcmp (name, "l.movhi") == 0) {
        insn[i].op[1] <<= 16;
        insn[i].op[2] = 0;
        insn[i].opt[2] = OPT_CONST;
      }
      if (insn[i].index == II_SFEQ || insn[i].index == II_SFNE
       || insn[i].index == II_SFLE || insn[i].index == II_SFGT
       || insn[i].index == II_SFGE || insn[i].index == II_SFLT) {
	/* repair params */
        insn[i].op[2] = insn[i].op[1]; insn[i].opt[2] = insn[i].opt[1] & ~OPT_DEST; 
        insn[i].op[1] = insn[i].op[0]; insn[i].opt[1] = insn[i].opt[0] & ~OPT_DEST;
        insn[i].op[0] = FLAG_REG; insn[i].opt[0] = OPT_DEST | OPT_REGISTER;
        insn[i].opt[3] = OPT_NONE;
        insn[i].type |= IT_COND;
      }
      if ((insn[i].index < 0) ||
	  ((insn[i].index == II_NOP) && (insn[i].op[0] != 0))) {
        cucdebug (1, "Instruction #%i: \"%s\" not supported (2).\n", i, name);
        log ("Instruction #%i: \"%s\" not supported (2).\n", i, name);
        return 1;
      }
    }
  }
  num_insn = i;
  fclose (fi);
  if (func_return != 2) {
    cucdebug (1, "Unsupported function structure.\n");
    log ("Unsupported function structure.\n");
    return 1;
  }

  log ("Number of instructions loaded = %i\n", num_insn);
  if (cuc_debug >= 3) print_cuc_insns ("INITIAL", 1);

  log ("Converting.\n");
  expand_branch ();
  if (cuc_debug >= 6) print_cuc_insns ("AFTER_EXP_BRANCH", 0);

  remove_dslots ();
  if (cuc_debug >= 6) print_cuc_insns ("NO_DELAY_SLOTS", 0);
  
  if (config.cuc.calling_convention) {
    detect_locals ();
    if (cuc_debug >= 7) print_cuc_insns ("AFTER_LOCALS", 0);
  }
  expand_memory ();
  if (cuc_debug >= 3) print_cuc_insns ("AFTER_EXP_MEM", 0);

  expand_signed ();
  if (cuc_debug >= 3) print_cuc_insns ("AFTER_EXP_SIG", 0);
  
  expand_calls ();
  if (cuc_debug >= 3) print_cuc_insns ("AFTER_EXP_CALLS", 0);

  return 0;
}
