/* abstract.h -- Abstract entities header file

   Copyright (C) 1999 Damjan Lampret, lampret@opencores.org
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


#ifndef ABSTRACT__H
#define ABSTRACT__H

/* Autoconf and/or portability configuration */
#include "port.h"

/* System includes */
#include <stdio.h>

/* Package includes */
#include "arch.h"

#define DEFAULT_MEMORY_START         0
#define DEFAULT_MEMORY_LEN    0x800000
#define STACK_SIZE                  20
#define LABELNAME_LEN               50
#define INSNAME_LEN                 15
#define OPERANDNAME_LEN             50
#define MAX_OPERANDS                 5

#define OP_MEM_ACCESS       0x80000000

/* Cache tag types.  */
#define CT_NONE                      0
#define CT_VIRTUAL                   1
#define CT_PHYSICAL                  2

/* History of execution */
#define HISTEXEC_LEN               200

/* Added by MM */
#ifndef LONGEST
#define LONGEST long long
#endif /* LONGEST */

#ifndef ULONGEST
#define ULONGEST unsigned long long
#endif /* ULONGEST */

/* Endianness convenience macros */
#define LE16(x) bswap_16(x)

/*! Instruction queue */
struct iqueue_entry
{
  int       insn_index;
  uint32_t  insn;
  oraddr_t  insn_addr;
};

/*! All the memory operations possible */
struct mem_ops
{

  /* Read functions */
  uint32_t    (*readfunc32) (oraddr_t, void *);
  uint16_t    (*readfunc16) (oraddr_t, void *);
  uint8_t     (*readfunc8) (oraddr_t, void *);

  /* Read functions' data */
  void       *read_dat8;
  void       *read_dat16;
  void       *read_dat32;

  /* Write functions */
  void        (*writefunc32) (oraddr_t, uint32_t, void *);
  void        (*writefunc16) (oraddr_t, uint16_t, void *);
  void        (*writefunc8) (oraddr_t, uint8_t, void *);

  /* Write functions' data */
  void       *write_dat8;
  void       *write_dat16;
  void       *write_dat32;

  /* Program load function.  If you have unwritteable memory but you would like
   * it if a program would be loaded here, make sure to set this.  If this is
   * not set, then writefunc8 will be called to load the program */
  void        (*writeprog32) (oraddr_t, uint32_t, void *);
  void        (*writeprog8) (oraddr_t, uint8_t, void *);

  /* Program load functions' data */
  void       *writeprog32_dat;
  void       *writeprog8_dat;

  /* Read/Write delays */
  int         delayr;
  int         delayw;

  /* Name of log file */
  const char *log;
};

/*! Memory regions assigned to devices */
struct dev_memarea
{
  struct dev_memarea *next;
  oraddr_t            addr_mask;
  oraddr_t            addr_compare;
  uint32_t            size;
  oraddr_t            size_mask;	/* Addr mask, calculated out of size */
  int                 valid;		/* Mirrors memory controler valid bit */
  FILE               *log;		/* log file (NULL if no logging) */
  struct mem_ops      ops;
  struct mem_ops      direct_ops;
};

/* Externally visible data */
extern struct dev_memarea *cur_area;
extern int                 data_ci;
extern int                 insn_ci;
extern struct hist_exec   *hist_exec_tail;

/* Function prototypes for external use */
extern uint32_t            eval_mem32 (oraddr_t memaddr, int *);
extern uint16_t            eval_mem16 (oraddr_t memaddr, int *);
extern uint8_t             eval_mem8 (oraddr_t memaddr, int *);
extern void                set_mem32 (oraddr_t memaddr, uint32_t value, int *);
extern void                set_mem16 (oraddr_t memaddr, uint16_t value, int *);
extern void                set_mem8 (oraddr_t memaddr, uint8_t value, int *);
extern void                dump_memory (oraddr_t  from,
					oraddr_t  to);
extern void                disassemble_memory (oraddr_t  from,
					       oraddr_t  to,
					       int       nl);
extern void                disassemble_instr (oraddr_t  phyaddr,
					      oraddr_t  virtaddr,
					      uint32_t  insn);
extern uint32_t            evalsim_mem32 (oraddr_t, oraddr_t);
extern uint16_t            evalsim_mem16 (oraddr_t, oraddr_t);
extern uint8_t             evalsim_mem8 (oraddr_t, oraddr_t);
extern void                setsim_mem32 (oraddr_t, oraddr_t, uint32_t);
extern void                setsim_mem16 (oraddr_t, oraddr_t, uint16_t);
extern void                setsim_mem8 (oraddr_t, oraddr_t, uint8_t);
extern void                done_memory_table ();
extern void                memory_table_status (void);
extern struct dev_memarea *reg_mem_area (oraddr_t        addr,
					 uint32_t        size,
					 unsigned        mc_dev,
					 struct mem_ops *ops);
extern void                adjust_rw_delay (struct dev_memarea *mem,
					    int                 delayr,
					    int                 delayw);
extern void                set_mem_valid (struct dev_memarea *mem,
					  int                 valid);
extern struct dev_memarea *verify_memoryarea (oraddr_t addr);
extern char               *generate_time_pretty (char *dest,
						 long  time_ps);
extern uint32_t            eval_insn (oraddr_t, int *);
extern uint32_t            eval_direct32 (oraddr_t  addr,
					  int       through_mmu,
					  int       through_dc);
extern uint16_t            eval_direct16 (oraddr_t  memaddr,
					  int       through_mmu,
					  int       through_dc);
extern uint8_t             eval_direct8 (oraddr_t   memaddr,
					 int        through_mmu,
					 int        through_dc);
extern void                set_direct8 (oraddr_t, uint8_t, int, int);
extern void                set_direct16 (oraddr_t, uint16_t, int, int);
extern void                set_direct32 (oraddr_t, uint32_t, int, int);
extern void                set_program32 (oraddr_t  memaddr,
					  uint32_t  value);
extern void                set_program8 (oraddr_t  memaddr,
					 uint8_t   value);

#endif /*  ABSTRACT__H */
