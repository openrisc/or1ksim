/* simpl32-defs.h -- Definitions for the simple execution model

   Copyright (C) 1999 Damjan Lampret, lampret@opencores.org
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


#ifndef SIMPLE32_DEFS__H
#define SIMPLE32_DEFS__H

/* Package includes */
#include "abstract.h"


extern void l_invalid PARAMS((struct iqueue_entry *));
extern void l_sfne PARAMS((struct iqueue_entry *));
extern void l_bf PARAMS((struct iqueue_entry *));
extern void l_add PARAMS((struct iqueue_entry *));
extern void l_addc PARAMS((struct iqueue_entry *));
extern void l_sw PARAMS((struct iqueue_entry *));
extern void l_sb PARAMS((struct iqueue_entry *));
extern void l_sh PARAMS((struct iqueue_entry *));
extern void l_lwz PARAMS((struct iqueue_entry *));
extern void l_lbs PARAMS((struct iqueue_entry *));
extern void l_lbz PARAMS((struct iqueue_entry *));
extern void l_lhs PARAMS((struct iqueue_entry *));
extern void l_lhz PARAMS((struct iqueue_entry *));
extern void l_movhi PARAMS((struct iqueue_entry *));
extern void l_and PARAMS((struct iqueue_entry *));
extern void l_or PARAMS((struct iqueue_entry *));
extern void l_xor PARAMS((struct iqueue_entry *));
extern void l_sub PARAMS((struct iqueue_entry *));
extern void l_mul PARAMS((struct iqueue_entry *));
extern void l_div PARAMS((struct iqueue_entry *));
extern void l_divu PARAMS((struct iqueue_entry *));
extern void l_sll PARAMS((struct iqueue_entry *));
extern void l_sra PARAMS((struct iqueue_entry *));
extern void l_srl PARAMS((struct iqueue_entry *));
extern void l_j PARAMS((struct iqueue_entry *));
extern void l_jal PARAMS((struct iqueue_entry *));
extern void l_jalr PARAMS((struct iqueue_entry *));
extern void l_jr PARAMS((struct iqueue_entry *));
extern void l_rfe PARAMS((struct iqueue_entry *));
extern void l_nop PARAMS((struct iqueue_entry *));
extern void l_bnf PARAMS((struct iqueue_entry *));
extern void l_sfeq PARAMS((struct iqueue_entry *));
extern void l_sfgts PARAMS((struct iqueue_entry *));
extern void l_sfges PARAMS((struct iqueue_entry *));
extern void l_sflts PARAMS((struct iqueue_entry *));
extern void l_sfles PARAMS((struct iqueue_entry *));
extern void l_sfgtu PARAMS((struct iqueue_entry *));
extern void l_sfgeu PARAMS()(struct iqueue_entry *);
extern void l_sfltu PARAMS((struct iqueue_entry *));
extern void l_sfleu PARAMS((struct iqueue_entry *));
extern void l_extbs PARAMS((struct iqueue_entry *));
extern void l_extbz PARAMS((struct iqueue_entry *));
extern void l_exths PARAMS((struct iqueue_entry *));
extern void l_exthz PARAMS((struct iqueue_entry *));
extern void l_extws PARAMS((struct iqueue_entry *));
extern void l_extwz PARAMS((struct iqueue_entry *));
extern void l_mtspr PARAMS((struct iqueue_entry *));
extern void l_mfspr PARAMS((struct iqueue_entry *));
extern void l_sys PARAMS((struct iqueue_entry *));
extern void l_trap PARAMS((struct iqueue_entry *)); /* CZ 21/06/01 */
extern void l_macrc PARAMS((struct iqueue_entry *));
extern void l_mac PARAMS((struct iqueue_entry *));
extern void l_msb PARAMS((struct iqueue_entry *));
extern void l_cmov PARAMS ((struct iqueue_entry *));
extern void l_ff1 PARAMS ((struct iqueue_entry *));
extern void l_cust1 PARAMS ((struct iqueue_entry *));
extern void l_cust2 PARAMS ((struct iqueue_entry *));
extern void l_cust3 PARAMS ((struct iqueue_entry *));
extern void l_cust4 PARAMS ((struct iqueue_entry *));
extern void lf_add_s PARAMS ((struct iqueue_entry *));
extern void lf_div_s PARAMS ((struct iqueue_entry *));
extern void lf_ftoi_s PARAMS ((struct iqueue_entry *));
extern void lf_itof_s PARAMS ((struct iqueue_entry *));
extern void lf_madd_s PARAMS ((struct iqueue_entry *));
extern void lf_mul_s PARAMS ((struct iqueue_entry *));
extern void lf_rem_s PARAMS ((struct iqueue_entry *));
extern void lf_sfeq_s PARAMS ((struct iqueue_entry *));
extern void lf_sfge_s PARAMS ((struct iqueue_entry *));
extern void lf_sfgt_s PARAMS ((struct iqueue_entry *));
extern void lf_sfle_s PARAMS ((struct iqueue_entry *));
extern void lf_sflt_s PARAMS ((struct iqueue_entry *));
extern void lf_sfne_s PARAMS ((struct iqueue_entry *));
extern void lf_sub_s PARAMS((struct iqueue_entry *));
extern void l_none PARAMS((struct iqueue_entry *));

#endif	/* SIMPLE32_DEFS__H */
