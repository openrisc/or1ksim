/* dyn32-defs.h -- Definitions for the dynamic execution model

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


#ifndef DYN32_DEFS__H
#define DYN32_DEFS__H


/* Package includes */
#include "arch.h"


struct op_queue {
  unsigned int num_ops;
  unsigned int ops_len;
  unsigned int *ops;
  unsigned int num_ops_param;
  unsigned int ops_param_len;
  unsigned int *ops_param;
  unsigned int jump_local; /* Parameter index that holds the location of the jump */
  oraddr_t jump_local_loc; /* Location to jump to (relative to start of page */
  unsigned int not_jump_loc; /* Location to jump if not jumping (l.bf/l.bnf) */
  int xref; /* Is this location cross referenced? */
  oraddr_t insn_addr; /* Physical address of the instruction */
  unsigned int reg_t[3]; /* Which registers are in the temporaries (after the instruction)? */
  unsigned int tflags[3];

  int insn_index;
  unsigned int param_type[5]; /* opd->type */
  orreg_t param[5]; /* Value of operand */
  unsigned int param_num; /* Number of operands */
  uint32_t insn; /* Instruction word */

  struct op_queue *prev;
  struct op_queue *next;
};

void gen_l_add PARAMS((struct op_queue *, int *, int));
void gen_l_addc PARAMS((struct op_queue *, int *, int));
void gen_l_and PARAMS((struct op_queue *, int *, int));
void gen_l_bf PARAMS((struct op_queue *, int *, int));
void gen_l_bnf PARAMS((struct op_queue *, int *, int));
void gen_l_cmov PARAMS((struct op_queue *, int *, int));
void gen_l_cust1 PARAMS((struct op_queue *, int *, int));
void gen_l_cust2 PARAMS((struct op_queue *, int *, int));
void gen_l_cust3 PARAMS((struct op_queue *, int *, int));
void gen_l_cust4 PARAMS((struct op_queue *, int *, int));
void gen_l_div PARAMS((struct op_queue *, int *, int));
void gen_l_divu PARAMS((struct op_queue *, int *, int));
void gen_l_extbs PARAMS((struct op_queue *, int *, int));
void gen_l_extbz PARAMS((struct op_queue *, int *, int));
void gen_l_exths PARAMS((struct op_queue *, int *, int));
void gen_l_exthz PARAMS((struct op_queue *, int *, int));
void gen_l_extws PARAMS((struct op_queue *, int *, int));
void gen_l_extwz PARAMS((struct op_queue *, int *, int));
void gen_l_ff1 PARAMS((struct op_queue *, int *, int));
void gen_l_j PARAMS((struct op_queue *, int *, int));
void gen_l_jal PARAMS((struct op_queue *, int *, int));
void gen_l_jr PARAMS((struct op_queue *, int *, int));
void gen_l_jalr PARAMS((struct op_queue *, int *, int));
void gen_l_lbs PARAMS((struct op_queue *, int *, int));
void gen_l_lbz PARAMS((struct op_queue *, int *, int));
void gen_l_lhs PARAMS((struct op_queue *, int *, int));
void gen_l_lhz PARAMS((struct op_queue *, int *, int));
void gen_l_lws PARAMS((struct op_queue *, int *, int));
void gen_l_lwz PARAMS((struct op_queue *, int *, int));
void gen_l_mac PARAMS((struct op_queue *, int *, int));
void gen_l_macrc PARAMS((struct op_queue *, int *, int));
void gen_l_mfspr PARAMS((struct op_queue *, int *, int));
void gen_l_movhi PARAMS((struct op_queue *, int *, int));
void gen_l_msb PARAMS((struct op_queue *, int *, int));
void gen_l_mtspr PARAMS((struct op_queue *, int *, int));
void gen_l_mul PARAMS((struct op_queue *, int *, int));
void gen_l_mulu PARAMS((struct op_queue *, int *, int));
void gen_l_nop PARAMS((struct op_queue *, int *, int));
void gen_l_or PARAMS((struct op_queue *, int *, int));
void gen_l_rfe PARAMS((struct op_queue *, int *, int));
void gen_l_sb PARAMS((struct op_queue *, int *, int));
void gen_l_sh PARAMS((struct op_queue *, int *, int));
void gen_l_sw PARAMS((struct op_queue *, int *, int));
void gen_l_sfeq PARAMS((struct op_queue *, int *, int));
void gen_l_sfges PARAMS((struct op_queue *, int *, int));
void gen_l_sfgeu PARAMS((struct op_queue *, int *, int));
void gen_l_sfgts PARAMS((struct op_queue *, int *, int));
void gen_l_sfgtu PARAMS((struct op_queue *, int *, int));
void gen_l_sfles PARAMS((struct op_queue *, int *, int));
void gen_l_sfleu PARAMS((struct op_queue *, int *, int));
void gen_l_sflts PARAMS((struct op_queue *, int *, int));
void gen_l_sfltu PARAMS((struct op_queue *, int *, int));
void gen_l_sfne PARAMS((struct op_queue *, int *, int));
void gen_l_sll PARAMS((struct op_queue *, int *, int));
void gen_l_sra PARAMS((struct op_queue *, int *, int));
void gen_l_srl PARAMS((struct op_queue *, int *, int));
void gen_l_sub PARAMS((struct op_queue *, int *, int));
void gen_l_sys PARAMS((struct op_queue *, int *, int));
void gen_l_trap PARAMS((struct op_queue *, int *, int));
void gen_l_xor PARAMS((struct op_queue *, int *, int));
void gen_l_invalid PARAMS((struct op_queue *, int *, int));

void gen_lf_add_s PARAMS((struct op_queue *, int *, int));
void gen_lf_div_s PARAMS((struct op_queue *, int *, int));
void gen_lf_ftoi_s PARAMS((struct op_queue *, int *, int));
void gen_lf_itof_s PARAMS((struct op_queue *, int *, int));
void gen_lf_madd_s PARAMS((struct op_queue *, int *, int));
void gen_lf_mul_s PARAMS((struct op_queue *, int *, int));
void gen_lf_rem_s PARAMS((struct op_queue *, int *, int));
void gen_lf_sfeq_s PARAMS((struct op_queue *, int *, int));
void gen_lf_sfge_s PARAMS((struct op_queue *, int *, int));
void gen_lf_sfgt_s PARAMS((struct op_queue *, int *, int));
void gen_lf_sfle_s PARAMS((struct op_queue *, int *, int));
void gen_lf_sflt_s PARAMS((struct op_queue *, int *, int));
void gen_lf_sfne_s PARAMS((struct op_queue *, int *, int));
void gen_lf_sub_s PARAMS((struct op_queue *, int *, int));
void l_none(struct op_queue *opq, int *param_t, int delay_slot);

#endif	/*  DYN32_DEFS__H */

