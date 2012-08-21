/* Table of opcodes for the OpenRISC 1000 ISA.

   Copyright 1990, 1991, 1992, 1993 Free Software Foundation, Inc.
   Copyright (C) 2008 Embecosm Limited

   Contributed by Damjan Lampret (lampret@opencores.org).
   Contributor Jeremy Bennett <jeremy.bennett@embecosm.com>

   This file is part of Or1ksim, the OpenRISC 1000 Architectural Simulator.
   This file is also part of or1k_gen_isa, GDB and GAS.

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

/* We treat all letters the same in encode/decode routines so
   we need to assign some characteristics to them like signess etc.*/

#ifndef OR32_H_ISA
#define OR32_H_ISA

#define NUM_UNSIGNED (0)
#define NUM_SIGNED (1)

#ifndef PARAMS
#define PARAMS(x) x
#endif

#ifndef CONST
#define CONST const
#endif

#define MAX_GPRS 32
#define PAGE_SIZE 8192
#undef __HALF_WORD_INSN__

#define OPERAND_DELIM (',')

#define OR32_IF_DELAY (1)
#define OR32_W_FLAG   (2)
#define OR32_R_FLAG   (4)

#if defined(HAVE_EXECUTION)
# if SIMPLE_EXECUTION
#  include "simpl32-defs.h"
# endif
#endif


struct or32_letter {
  char letter;
  int  sign;
  /* int  reloc; relocation per letter ??*/
};

enum insn_type {
 it_unknown,
 it_exception,
 it_arith,
 it_shift,
 it_compare,
 it_branch,
 it_jump,
 it_load,
 it_store,
 it_movimm,
 it_move,
 it_extend,
 it_nop,
 it_mac,
 it_float };

/* Main instruction specification array.  */
struct or32_opcode {
  /* Name of the instruction.  */
  char *name;

  /* A string of characters which describe the operands.
     Valid characters are:
     ,() Itself.  Characters appears in the assembly code.
     rA	 Register operand.
     rB  Register operand.
     rD  Register operand (destination).
     I	 An immediate operand, range -32768 to 32767.
     J	 An immediate operand, range . (unused)
     K	 An immediate operand, range 0 to 65535.
     L	 An immediate operand, range 0 to 63.
     M	 An immediate operand, range . (unused)
     N	 An immediate operand, range -33554432 to 33554431.
     O	 An immediate operand, range . (unused) */
  char *args;

  /* Opcode and operand encoding. */
  char *encoding;

#ifdef HAVE_EXECUTION
# if COMPLEX_EXECUTION
  char *function_name;
# elif SIMPLE_EXECUTION
  void (*exec)(struct iqueue_entry *);
# endif
#else  /* HAVE_EXECUTION */
  void (*exec)(void);
#endif

  unsigned int flags;
  enum insn_type func_unit;
};

/* This operand is the last in the list */
#define OPTYPE_LAST (0x80000000)
/* This operand marks the end of the operand sequence (for things like I(rD)) */
#define OPTYPE_OP   (0x40000000)
/* The operand specifies a register index */
#define OPTYPE_REG  (0x20000000)
/* The operand must be sign extended */
#define OPTYPE_SIG  (0x10000000)
/* Operand is a relative address, the `I' in `I(rD)' */
#define OPTYPE_DIS  (0x08000000)
/* The operand is a destination */
#define OPTYPE_DST  (0x04000000)
/* Which bit of the operand is the sign bit */
#define OPTYPE_SBIT (0x00001F00)
/* Amount to shift the instruction word right to get the operand */
#define OPTYPE_SHR  (0x0000001F)
#define OPTYPE_SBIT_SHR (8)

/* MM: Data how to decode operands.  */
extern struct insn_op_struct {
  unsigned long type;
  unsigned long data;
} **or1ksim_op_start;

/* Leaf flag used in automata building */
#define LEAF_FLAG         (0x80000000)

struct temp_insn_struct
{
  unsigned long insn;
  unsigned long insn_mask;
  int in_pass;
};

  
extern unsigned long *or1ksim_automata;
extern struct temp_insn_struct *or1ksim_ti;

extern CONST struct  or32_opcode  or1ksim_or32_opcodes[];

extern char *or1ksim_disassembled;

/* trace data */
extern int           trace_dest_reg;
extern int           trace_store_addr_reg;
extern unsigned int  trace_store_imm;
extern int           trace_store_val_reg;
extern int           trace_store_width;
extern int           trace_dest_spr;

/* Calculates instruction length in bytes.  Always 4 for OR32. */
extern int or1ksim_insn_len PARAMS((int insn_index));

/* MM: Returns instruction name from index.  */
extern CONST char *or1ksim_insn_name PARAMS ((int index));

/* MM: Constructs new FSM, based on or1ksim_or32_opcodes.  */ 
extern void or1ksim_build_automata PARAMS ((int  quiet));

/* MM: Destructs FSM.  */ 
extern void or1ksim_destruct_automata PARAMS ((void));

/* MM: Decodes instruction using FSM.  Call or1ksim_build_automata first.  */
extern int or1ksim_insn_decode PARAMS((unsigned int insn));

/* Disassemble one instruction from insn to disassemble.
   Return the size of the instruction.  */
extern int or1ksim_disassemble_insn (unsigned long insn);

/* Disassemble one instruction from insn index.
   Return the size of the instruction.  */
int or1ksim_disassemble_index (unsigned long insn, int index);

/* Disassemble one instruction from insn index for tracing. */
void or1ksim_disassemble_trace_index (unsigned long int  insn,
				      int                index);

/* FOR INTERNAL USE ONLY */
/* Automatically does zero- or sign- extension and also finds correct
   sign bit position if sign extension is correct extension. Which extension
   is proper is figured out from letter description. */
unsigned long or1ksim_extend_imm(unsigned long imm, char l);

/* Extracts value from opcode */
unsigned long or1ksim_or32_extract(char param_ch, char *enc_initial, unsigned long insn);

#endif

