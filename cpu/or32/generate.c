/* generate.c -- generates file execgen.c from instruction set

   Copyright (C) 1999 Damjan Lampret, lampret@opencores.org
   Copyright (C) 2005 György `nog' Jeney, nog@sdf.lonestar.org
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
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

/* Package includes */
#include "opcode/or32.h"

static char *in_file;
static char *out_file;

/* Whether this instruction stores something in register */
static int write_to_reg;

static int out_lines = 0;

static int shift_fprintf(int level, FILE *f, const char *fmt, ...)
{
  va_list ap;
  int i;

  va_start(ap, fmt);
  for(i = 0; i < level; i++)
    fprintf(f, "  ");

  i = vfprintf(f, fmt, ap);
  va_end(ap);

  out_lines++;
  return i + (level * 2);
}

/* Generates a execute sequence for one instruction */
int output_function (FILE *fo, const char *func_name, int level)
{
  FILE *fi;
  int olevel;
  int line_num = 0;

  if ((fi = fopen (in_file, "rt")) == NULL) {
    printf("could not open file\n");
    return 1;
  }

  while (!feof (fi)) {
    char line[10000], *str = line;
    char *res;

    res = fgets (str, sizeof (line), fi);

    if (NULL == res)
      {
	fclose (fi);			/* Mark Jarvin patch */
	return  1;
      }

    line[sizeof (line) - 1] = 0;
    line_num++;
    if (strncmp (str, "INSTRUCTION (", 13) == 0) {
      char *s;
      str += 13;
      while (isspace ((int)*str)) str++;
      s = str;
      while (*s && *s != ')') s++;
      *s = 0;
      while (isspace((int)*(s - 1))) s--;
      *s = 0;
      if (strcmp (str, func_name) == 0) {
        olevel = 1;
        str += strlen (str) + 1;
        while (isspace ((int)*str)) str++;
        s = str;
        while (*s && *s != '\n' && *s != '\r') s++;
        *s = 0;
        while (isspace((int)*(s - 1))) s--;
        *s = 0;
        /*shift_fprintf (level, fo, "#line %i \"%s\"\n", line_num, in_file);*/
        shift_fprintf (level, fo, "%s", str);
        shift_fprintf (level, fo, "   /* \"%s\" */\n", func_name);
        do {
          res = fgets (line, sizeof (line), fi);

	  if (NULL == res)
	    {
	      fclose (fi);
	      return  1;
	    }

          line[sizeof(line) - 1] = 0;
          for (str = line; *str; str++) {
            if (*str == '{') olevel++;
            else if (*str == '}') olevel--;
          }
          shift_fprintf (level, fo, "%s", line);
        } while (olevel);
	fclose(fi);
        /*shift_fprintf (level, fo, "#line %i \"%s\"\n", out_lines, out_file);*/
        return 0;
      }
    }
  }
  shift_fprintf (level, fo, "%s ();\n", func_name);

  fclose(fi);
  return 0;
}

/* Parses operands. */

static int
gen_eval_operands (FILE *fo, int insn_index, int level)
{
  struct insn_op_struct *opd = or1ksim_op_start[insn_index];
  int i;
  int num_ops;
  int nbits = 0;
  int set_param = 0;
  int dis = 0;
  int sbit;
  int dis_op = -1;

  write_to_reg = 0;

  shift_fprintf (level, fo, "uorreg_t ");

  /* Count number of operands */
  for (i = 0, num_ops = 0;; i++) {
    if (!(opd[i].type & OPTYPE_OP))
      continue;
    if (opd[i].type & OPTYPE_DIS)
      continue;
    if (num_ops)
      fprintf(fo, ", ");
    fprintf(fo, "%c", 'a' + num_ops);
    num_ops++;
    if (opd[i].type & OPTYPE_LAST)
      break;
  }

  fprintf (fo, ";\n");

  shift_fprintf (level, fo, "/* Number of operands: %i */\n", num_ops);

  i = 0;
  num_ops = 0;
  do {
/*
    printf("opd[%i].type<last> = %c\n", i, opd->type & OPTYPE_LAST ? '1' : '0');    printf("opd[%i].type<op> = %c\n", i, opd->type & OPTYPE_OP ? '1' : '0');
    printf("opd[%i].type<reg> = %c\n", i, opd->type & OPTYPE_REG ? '1' : '0');
    printf("opd[%i].type<sig> = %c\n", i, opd->type & OPTYPE_SIG ? '1' : '0');
    printf("opd[%i].type<dis> = %c\n", i, opd->type & OPTYPE_DIS ? '1' : '0');
    printf("opd[%i].type<shr> = %i\n", i, opd->type & OPTYPE_SHR);
    printf("opd[%i].type<sbit> = %i\n", i, (opd->type & OPTYPE_SBIT) >> OPTYPE_SBIT_SHR);
    printf("opd[%i].data = %i\n", i, opd->data);
*/

    if (!nbits)
      shift_fprintf (level, fo, "%c = (insn >> %i) & 0x%x;\n", 'a' + num_ops,
                     opd->type & OPTYPE_SHR, (1 << opd->data) - 1);
    else
      shift_fprintf (level, fo, "%c |= ((insn >> %i) & 0x%x) << %i;\n",
                     'a' + num_ops, opd->type & OPTYPE_SHR,
                     (1 << opd->data) - 1, nbits);

    nbits += opd->data;

    if ((opd->type & OPTYPE_DIS) && (opd->type & OPTYPE_OP)) {
      sbit = (opd->type & OPTYPE_SBIT) >> OPTYPE_SBIT_SHR;
      if (opd->type & OPTYPE_SIG)
        shift_fprintf (level, fo, "if(%c & 0x%08x) %c |= 0x%x;\n",
                       'a' + num_ops, 1 << sbit, 'a' + num_ops,
                       0xffffffff << sbit);
      opd++;
      shift_fprintf (level, fo, "*(orreg_t *)&%c += (orreg_t)cpu_state.reg[(insn >> %i) & 0x%x];\n",
                     'a' + num_ops, opd->type & OPTYPE_SHR,
                     (1 << opd->data) - 1);
      dis = 1;
      dis_op = num_ops;
    }

    if (opd->type & OPTYPE_OP) {
      sbit = (opd->type & OPTYPE_SBIT) >> OPTYPE_SBIT_SHR;
      if (opd->type & OPTYPE_SIG)
        shift_fprintf (level, fo, "if(%c & 0x%08x) %c |= 0x%x;\n",
                       'a' + num_ops, 1 << sbit, 'a' + num_ops,
                       0xffffffff << sbit);
      if ((opd->type & OPTYPE_REG) && !dis) {
        if(!i) {
          shift_fprintf (level, fo, "#define SET_PARAM0(val) cpu_state.reg[a] = val\n");
          shift_fprintf (level, fo, "#define REG_PARAM0  a\n");
          set_param = 1;
        }
        shift_fprintf (level, fo, "#define PARAM%i cpu_state.reg[%c]\n", num_ops,
                      'a' + num_ops);
        if(opd->type & OPTYPE_DST)
          write_to_reg = 1;
      } else {
        shift_fprintf (level, fo, "#define PARAM%i %c\n", num_ops,
                       'a' + num_ops);
      }
      num_ops++;
      nbits = 0;
      dis = 0;
    }

    if ((opd->type & OPTYPE_LAST))
      break;
    opd++;
    i++;
  } while (1);

  output_function (fo, or1ksim_or32_opcodes[insn_index].function_name, level);

  if (set_param)
    {
      shift_fprintf (level, fo, "#undef SET_PARAM0\n");
      shift_fprintf (level, fo, "#undef REG_PARAM0\n");
    }

  for (i = 0; i < num_ops; i++)
    shift_fprintf (level, fo, "#undef PARAM%i\n", i);

  return dis_op;
}

/* Generates decode and execute for one instruction instance */
static int output_call (FILE *fo, int index, int level)
{
  int dis_op = -1;

  /*printf ("%i:%s\n", index, insn_name (index));*/

  shift_fprintf (level++, fo, "{\n");

  if (index >= 0)
    dis_op = gen_eval_operands (fo, index, level);

  if (index < 0) output_function (fo, "l_invalid", level);

  fprintf (fo, "\n");

  shift_fprintf (level++, fo, "if (do_stats) {\n");

  if (dis_op >= 0)
    shift_fprintf (level, fo, "cpu_state.insn_ea = %c;\n", 'a' + dis_op);

  shift_fprintf (level, fo, "current->insn_index = %i;   /* \"%s\" */\n", index,
                 or1ksim_insn_name (index));

  shift_fprintf (level, fo, "analysis(current);\n");
  shift_fprintf (--level, fo, "}\n");

  if (write_to_reg)
    shift_fprintf (level, fo, "cpu_state.reg[0] = 0; /* Repair in case we changed it */\n");
  shift_fprintf (--level, fo, "}\n");
  return 0;
}


/* Generates .c file header */
static int generate_header (FILE *fo)
{
  fprintf (fo, "/* execgen.c -- Automatically generated decoder\n");
  fprintf (fo, "\n");
  fprintf (fo, "   Copyright (C) 1999 Damjan Lampret, lampret@opencores.org\n");
  fprintf (fo, "   Copyright (C) 2008 Embecosm Limited\n");
  fprintf (fo, "\n");
  fprintf (fo, "   Contributor Jeremy Bennett <jeremy.bennett@embecosm.com>\n");
  fprintf (fo, "\n");
  fprintf (fo, "   This file is part of Or1ksim, the OpenRISC 1000 Architectural Simulator.\n");
  fprintf (fo, "\n");
  fprintf (fo, "   This program is free software; you can redistribute it and/or modify it\n");
  fprintf (fo, "   under the terms of the GNU General Public License as published by the Free\n");
  fprintf (fo, "   Software Foundation; either version 3 of the License, or (at your option)\n");
  fprintf (fo, "   any later version.\n");
  fprintf (fo, "\n");
  fprintf (fo, "   This program is distributed in the hope that it will be useful, but WITHOUT\n");
  fprintf (fo, "   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or\n");
  fprintf (fo, "   FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for\n");
  fprintf (fo, "   more details.\n");
  fprintf (fo, "\n");
  fprintf (fo, "   You should have received a copy of the GNU General Public License along\n");
  fprintf (fo, "   with this program.  If not, see <http://www.gnu.org/licenses/>.  */\n");
  fprintf (fo, "\n");
  fprintf (fo, "/* This program is commented throughout in a fashion suitable for processing\n");
  fprintf (fo, "   with Doxygen. */\n");
  fprintf (fo, "\n");

  fprintf (fo, "/* This file was automatically generated by generate (see\n");
  fprintf (fo, "   cpu/or32/generate.c) */\n\n");


  return 0;
}

/* Generates .c file decode function hedaer */
static int generate_decode_function_header (FILE *fo)
{
  fprintf (fo, "typedef union {\n\tfloat fval;\n\tuint32_t hval;\n} FLOAT;\n\n");
  fprintf (fo, "static void decode_execute (struct iqueue_entry *current)\n{\n");
  fprintf (fo, "  uint32_t insn = current->insn;\n");
  out_lines +=3;
  return 0;
}

// List of strings which will be printed on a line after "#include "
char *include_strings[] = { "<math.h>",
			    "<stdint.h>",
			    ""}; // Last one must be empty
			    

/* Generates .c file includes */
static int generate_includes (FILE *fo)
{
  int i;
  for (i=0;(strcmp(include_strings[i], "")!=0);i++)
    fprintf (fo, "#include %s\n\n", include_strings[i]);
  out_lines +=i;
  return 0;
  
}

/* Generates .c file footer */
static int 
generate_footer (FILE *fo)
{
  fprintf (fo, "}\n");
  return 0;
}

/* Decodes all instructions and generates code for that.  This function
   is similar to or1ksim_insn_decode, except it decodes all instructions.

   JPB: Added code to generate an illegal instruction exception for invalid
   instructions. */
static int 
generate_body (FILE *fo, unsigned long *a, unsigned long cur_mask, int level)
{
  unsigned long shift = *a;
  unsigned long mask;
  int i;
  int prev_inv = 0;

  if (!(*a & LEAF_FLAG)) {
    shift = *a++;
    mask = *a++;
    shift_fprintf (level, fo, "switch((insn >> %i) & 0x%x) {\n", shift,
                   mask);
    for (i = 0; i <= mask; i++, a++) {
      if (!*a) {
        shift_fprintf (level, fo, "case 0x%x:\n", i);
        prev_inv = 1;
      } else {
        if(prev_inv) {
          shift_fprintf (++level, fo, "/* Invalid instruction(s) */\n");
	  shift_fprintf (level, fo,
			 "except_handle (EXCEPT_ILLEGAL, cpu_state.pc);\n");
          shift_fprintf (level--, fo, "break;\n");
        }
        shift_fprintf (level, fo, "case 0x%x:\n", i);
        generate_body (fo, or1ksim_automata + *a, cur_mask | (mask << shift), ++level);
        shift_fprintf (level--, fo, "break;\n");
        prev_inv = 0;
      }
    }
    if (prev_inv) {
      shift_fprintf (++level, fo, "/* Invalid instruction(s) */\n");
      shift_fprintf (level, fo,
		     "except_handle (EXCEPT_ILLEGAL, cpu_state.pc);\n");
      shift_fprintf (level--, fo, "break;\n");
    }
    shift_fprintf (level, fo, "}\n");
  } else {
    i = *a & ~LEAF_FLAG;

    /* Final check - do we have direct match?
       (based on or1ksim_or32_opcodes this should be the only possibility,
       but in case of invalid/missing instruction we must perform a check)  */

    if (or1ksim_ti[i].insn_mask != cur_mask) {
      shift_fprintf (level, fo, "/* Not unique: real mask %08lx and current mask %08lx differ - do final check */\n", or1ksim_ti[i].insn_mask, cur_mask);
      shift_fprintf (level++, fo, "if((insn & 0x%x) == 0x%x) {\n",
                     or1ksim_ti[i].insn_mask, or1ksim_ti[i].insn);
    }
    shift_fprintf (level, fo, "/* Instruction: %s */\n", or1ksim_or32_opcodes[i].name);

    output_call (fo, i, level);

    if (or1ksim_ti[i].insn_mask != cur_mask) {
      shift_fprintf (--level, fo, "} else {\n");
      shift_fprintf (++level, fo, "/* Invalid insn */\n");
      output_call (fo, -1, level);
      shift_fprintf (--level, fo, "}\n");
    }
  }
  return 0;
}
      
/* Main function; it takes two parameters:
   input_file(possibly insnset.c) output_file(possibly execgen.c)*/
int main (int argc, char *argv[])
{
  FILE *fo;
 
  if (argc != 3) {
    fprintf (stderr, "USAGE: generate input_file(possibly insnset.c) output_file(possibly execgen.c)\n");
    exit (-1);
  }
  
  in_file = argv[1];
  out_file = argv[2];
  if (!(fo = fopen (argv[2], "wt+"))) {
    fprintf (stderr, "Cannot create '%s'.\n", argv[2]);
    exit (1);
  }

  or1ksim_build_automata (0);
  if (generate_header (fo)) {
    fprintf (stderr, "generate_header\n");
    return 1;
  }

  if (generate_includes (fo)) {
    fprintf (stderr, "generate_includes\n");
    return 1;
  }

  if (generate_decode_function_header (fo)) {
    fprintf (stderr, "generate_decode_function_header\n");
    return 1;
  }
  
  if (generate_body (fo, or1ksim_automata, 0, 1)) {
    fprintf (stderr, "generate_body\n");
    return 1;
  }

  if (generate_footer (fo)) {
    fprintf (stderr, "generate_footer\n");
    return 1;
  }

  fclose (fo);
  or1ksim_destruct_automata ();
  return 0;
}

