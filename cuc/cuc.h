/* cuc.h -- OpenRISC Custom Unit Compiler, main header file

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


#ifndef CUC__H
#define CUC__H

/* Package includes */
#include "abstract.h"


/* Maximum number of instructions per function */
#define MAX_INSNS	0x10000
#define MAX_BB          0x1000
#define MAX_REGS	34
#define FLAG_REG	(MAX_REGS - 2)
#define LRBB_REG	(MAX_REGS - 1)
#define CUC_MAX_STACK   0x1000	/* if more, not converted */
#define MAX_PREROLL     32
#define MAX_UNROLL      32

#define IT_BRANCH	0x0001	/* Branch instruction */
#define IT_INDELAY	0x0002	/* Instruction is in delay slot */
#define	IT_BBSTART	0x0004	/* BB start marker */
#define	IT_BBEND  	0x0008	/* BB end marker */
#define IT_OUTPUT	0x0010	/* this instruction holds final value of the register */
#define IT_SIGNED	0x0020	/* Instruction is signed */
#define IT_MEMORY	0x0040	/* Instruction does memory access */
#define IT_UNUSED       0x0080	/* dead instruction marker */
#define IT_FLAG1        0x0100	/* misc flags */
#define IT_FLAG2        0x0200
#define IT_VOLATILE     0x0400	/* Should not be moved/removed */
#define IT_MEMADD       0x0800	/* add before the load -- should not be removed */
#define IT_COND         0x1000	/* Conditional */
#define IT_LATCHED      0x2000	/* Output of this instruction is latched/registered */
#define IT_CUT          0x4000	/* After this instruction register is placed */

#define OPT_NONE	0x00
#define	OPT_CONST	0x01
#define OPT_REGISTER	0x02
#define OPT_REF		0x04
#define OPT_JUMP	0x08	/* Jump to an instruction index */
#define OPT_DEST	0x10	/* This operand is dest */
#define OPT_BB          0x20	/* Jumpt to BB */
#define OPT_LRBB        0x40	/* 0 if we came in from left BB, or 1 otherwise */

#define MT_WIDTH        0x007	/* These bits hold memory access width in bytes */
#define MT_BURST        0x008	/* burst start & end markers */
#define MT_BURSTE       0x010
#define MT_CALL         0x020	/* This is a call */
#define MT_LOAD         0x040	/* This memory access does a read */
#define MT_STORE        0x080	/* This memory access does a write */
#define MT_SIGNED       0x100	/* Signed memory access */

#define MO_NONE         0	/* different memory ordering, even if there are dependencies,
				   burst can be made, width can change */
#define MO_WEAK         1	/* different memory ordering, if there cannot be dependencies,
				   burst can be made, width can change */
#define MO_STRONG       2	/* Same memory ordering, burst can be made, width can change */
#define MO_EXACT        3	/* Exacltly the same memory ordering and widths */

#define BB_INLOOP       0x01	/* This block is inside a loop */
#define BB_OPTIONAL     0x02
#define BB_DEAD         0x08	/* This block is unaccessible -> to be removed */

#define BBID_START      MAX_BB	/* Start BB pointer */
#define BBID_END        (MAX_BB + 1)	/* End BB pointer */

/* Various macros to minimize code size */
#define REF(bb,i)       (((bb) * MAX_INSNS) + (i))
#define REF_BB(r)       ((r) / MAX_INSNS)
#define REF_I(r)        ((r) % MAX_INSNS)
#define INSN(ref)       bb[REF_BB(ref)].insn[REF_I(ref)]

#ifndef MIN
# define MIN(x,y)          ((x) < (y) ? (x) : (y))
#endif

#ifndef MAX
# define MAX(x,y)          ((x) > (y) ? (x) : (y))
#endif

#define log(x...)       {fprintf (flog, x); fflush (flog); }

#define cucdebug(x,s...) {if ((x) <= cuc_debug) PRINTF (s);}

#define CUC_WIDTH_ITERATIONS  256

/* Options */
/* Whether we are debugging cuc (0-9) */
extern int cuc_debug;

/* Temporary registers by software convention */
extern const int caller_saved[MAX_REGS];

typedef struct _dep_list_t
{
  unsigned long ref;
  struct _dep_list_t *next;
} dep_list;

/* Shared list, if marked dead, entry is not used */
typedef struct _csm_list
{
  int ref;
  int cnt;
  int cmovs;
  double size, osize;
  int cmatch;
  int dead;
  int ninsn;			/* Number of associated instructions */
  struct _csm_list *from;
  struct _csm_list *next;
} cuc_shared_list;

/* Shared resource item definition */
typedef struct
{
  int ref;
  int cmatch;
} cuc_shared_item;

/* Implementation specific timings */
typedef struct
{
  int b;			/* Basic block # this timing is referring to */
  int preroll;			/* How many times was this BB pre/unrolled */
  int unroll;
  int nshared;
  cuc_shared_item *shared;	/* List of shared resources */
  int new_time;
  double size;
} cuc_timings;

/* Instructionn entity */
typedef struct
{
  int type;			/* type of the instruction */
  int index;			/* Instruction index */
  int opt[MAX_OPERANDS];	/* operand types */
  unsigned long op[MAX_OPERANDS];	/* operand values */
  dep_list *dep;		/* instruction dependencies */
  unsigned long insn;		/* Instruction opcode */
  char disasm[40];		/* disassembled string */
  unsigned long max;		/* max result value */
  int tmp;
} cuc_insn;

/* Basic block entity */
typedef struct
{
  unsigned long type;		/* Type of the bb */
  int first, last;		/* Where this block lies */
  int prev[2], next[2];
  int tmp;
  cuc_insn *insn;		/* Instructions lie here */
  int ninsn;			/* Number of instructions */
  int last_used_reg[MAX_REGS];
  dep_list *mdep;		/* Last memory access dependencies */
  int nmemory;
  int cnt;			/* how many times was this block executed */
  int unrolled;			/* how many times has been this block unrolled */

  int ntim;			/* Basic block options */
  cuc_timings *tim;
  int selected_tim;		/* Selected option, -1 if none */
} cuc_bb;

/* Function entity */
typedef struct _cuc_func
{
  /* Basic blocks */
  int num_bb;
  cuc_bb bb[MAX_BB];
  int saved_regs[MAX_REGS];	/* Whether this register was saved */
  int lur[MAX_REGS];		/* Location of last use */
  int used_regs[MAX_REGS];	/* Nonzero if it was used */

  /* Schedule of memory instructions */
  int nmsched;
  int msched[MAX_INSNS];
  int mtype[MAX_INSNS];

  /* initial bb and their relocations to new block numbers */
  int num_init_bb;
  int *init_bb_reloc;
  int orig_time;		/* time in cyc required for SW implementation */
  int num_runs;			/* Number times this function was run */
  cuc_timings timings;		/* Base timings */
  unsigned long start_addr;	/* Address of first instruction inn function */
  unsigned long end_addr;	/* Address of last instruction inn function */
  int memory_order;		/* Memory order */

  int nfdeps;			/* Function dependencies */
  struct _cuc_func **fdeps;

  int tmp;
} cuc_func;

/* Instructions from function */
extern cuc_insn insn[MAX_INSNS];
extern int num_insn;
extern int reloc[MAX_INSNS];
extern FILE *flog;

/* Loads from file into global array insn */
int cuc_load (char *in_fn);

/* Negates conditional instruction */
void negate_conditional (cuc_insn * ii);

/* Scans sequence of BBs and set bb[].cnt */
void generate_bb_seq (cuc_func * f, char *mp_filename, char *bb_filename);

/* Prints out instructions */
void print_insns (int bb, cuc_insn * insn, int size, int verbose);

/* prints out bb string */
void print_bb_num (int num);

/* Print out basic blocks */
void print_cuc_bb (cuc_func * func, char *s);

/* Duplicates function */
cuc_func *dup_func (cuc_func * f);

/* Releases memory allocated by function */
void free_func (cuc_func * f);

/* Common subexpression matching -- resource sharing, analysis pass */
void csm (cuc_func * f);

/* Common subexpression matching -- resource sharing, generation pass */
void csm_gen (cuc_func * f, cuc_func * rf, cuc_shared_item * shared,
	      int nshared);

/* Set the BB limits */
void detect_bb (cuc_func * func);

/* Optimize basic blocks */
int optimize_bb (cuc_func * func);

/* Search and optimize complex cmov assignments */
int optimize_cmovs (cuc_func * func);

/* Optimizes dataflow tree */
int optimize_tree (cuc_func * func);

/* Remove nop instructions */
int remove_nops (cuc_func * func);

/* Removes dead instruction */
int remove_dead (cuc_func * func);

/* Removes trivial register assignments */
int remove_trivial_regs (cuc_func * f);

/* Determine inputs and outputs */
void set_io (cuc_func * func);

/* Removes BBs marked as dead */
int remove_dead_bb (cuc_func * func);

/* Common subexpression elimination */
int cse (cuc_func * f);

/* Detect register dependencies */
void reg_dep (cuc_func * func);

/* Cuts the tree and marks registers */
void mark_cut (cuc_func * f);

/* Unroll loop b times times and return new function. Original
   function is unmodified. */
cuc_func *preunroll_loop (cuc_func * func, int b, int preroll, int unroll,
			  char *bb_filename);

/* Clean memory and data dependencies */
void clean_deps (cuc_func * func);

/* Schedule memory accesses
  0 - exact; 1 - strong; 2 - weak;  3 - none */
int schedule_memory (cuc_func * func, int otype);

/* Recalculates bb[].cnt values, based on generated profile file */
void recalc_cnts (cuc_func * f, char *bb_filename);

/* Calculate timings */
void analyse_timings (cuc_func * func, cuc_timings * timings);

/* Calculates facts, that are determined by conditionals */
void insert_conditional_facts (cuc_func * func);

/* Width optimization -- detect maximum values */
void detect_max_values (cuc_func * f);

/* Inserts n nops before insn 'ref' */
void insert_insns (cuc_func * f, int ref, int n);

/* Checks for some anomalies with references */
void cuc_check (cuc_func * f);

/* Adds memory dependencies based on ordering type */
void add_memory_dep (cuc_func * f, int otype);

/* Prints out instructions */
void print_cuc_insns (char *s, int verbose);

/* Build basic blocks */
void build_bb (cuc_func * f);

/* Latch outputs in loops */
void add_latches (cuc_func * f);

/* split the BB, based on the group numbers in .tmp */
void expand_bb (cuc_func * f, int b);

void add_dep (dep_list ** list, int dep);

void dispose_list (dep_list ** list);

void main_cuc (char *filename);

void add_data_dep (cuc_func * f);

void  reg_cuc_sec ();

#endif /* CUC__H */
