/* dyn-rec.h -- Recompiler specific definitions

   Copyright (C) 2005 György `nog' Jeney, nog@sdf.lonestar.org
   Copyright (C) 2008 Embecosm Limited
  
   Contributor Jeremy Bennett <jeremy.bennett@embecosm.com>
  
   This file is part of OpenRISC 1000 Architectural Simulator.
  
   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the Free
   Software Foundation; either version 3 of the License, or (at your option)
   any later version.
  
   This program is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
   more details.
  
   You should have received a copy of the GNU General Public License along
   with this program.  If not, see <http://www.gnu.org/licenses/>. */

/* This program is commented throughout in a fashion suitable for processing
   with Doxygen. */


#ifndef DYN_REC__H
#define DYN_REC__H

typedef void (*gen_code_ent)(void);

/* Each dynamically recompiled page has one of these */
struct dyn_page {
  oraddr_t or_page;
  void *host_page;
  unsigned int host_len;
  int dirty; /* Is recompiled page invalid? */
  int delayr; /* delayr of memory backing this page */
  uint16_t ts_bound[2049]; /* What registers the temporaries back (on the
                            * begining boundry of the instruction) */
  void **locs; /* Openrisc locations in the recompiled code */
  uint32_t *insns; /* Copy of instructions on this page */
  unsigned int *insn_indexs; /* Decoded instructions on this page */
};

/* Function prototypes for external use */
extern void recompile_page(struct dyn_page *dyn);
extern struct dyn_page *new_dp(oraddr_t page);
extern void add_to_opq(struct op_queue *opq, int end, int op);
extern void add_to_op_params(struct op_queue *opq, int end, unsigned long param);
extern void *enough_host_page(struct dyn_page *dp, void *cur, unsigned int *len,
                       unsigned int amount);
extern void init_dyn_recomp(void);
extern void run_sched_out_of_line(void);
extern void recheck_immu(int got_en_dis);
extern void enter_dyn_code(oraddr_t addr, struct dyn_page *dp);
extern void dyn_checkwrite(oraddr_t addr);
extern void dyn_main(void);

/* Global variables for external use */
extern void *rec_stack_base;

#define IMMU_GOT_ENABLED 1
#define IMMU_GOT_DISABLED 2

#define xglue(x, y) x ## y
#define glue(x, y) xglue(x, y)

#endif	/* DYN_REC__H */

