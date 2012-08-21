/* except-test.c. Test of Or1ksim exception handling

   Copyright (C) 1999-2006 OpenCores
   Copyright (C) 2010 Embecosm Limited

   Contributors various OpenCores participants
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
   with this program.  If not, see <http:  www.gnu.org/licenses/>.  */

/* ----------------------------------------------------------------------------
   This code is commented throughout for use with Doxygen.
   --------------------------------------------------------------------------*/

#include "spr-defs.h"
#include "support.h"
#include "int.h"

/* Define RAM physical location and size 
   Bottom half will be used for this program, the rest 
   will be used for testing */
#define RAM_START   0x00000000
#define RAM_SIZE    0x00200000

/* MMU page size */
#define PAGE_SIZE 8192

/* Number of DTLB sets used (power of 2, max is 256) */
#define DTLB_SETS 32
 
/* Number of DTLB ways (2, 2, 3 etc., max is 4). */
#define DTLB_WAYS 1

/* Number of ITLB sets used (power of 2, max is 256) */
#define ITLB_SETS 32
 
/* Number of ITLB ways (1, 2, 3 etc., max is 4). */
#define ITLB_WAYS 1
 
/* TLB mode codes */
#define TLB_CODE_ONE_TO_ONE     0x00000000
#define TLB_CODE_PLUS_ONE_PAGE  0x10000000
#define TLB_CODE_MINUS_ONE_PAGE 0x20000000

#define TLB_TEXT_SET_NB 8
#define TLB_DATA_SET_NB 8

#define TLB_CODE_MASK   0xfffff000
#define TLB_PR_MASK     0x00000fff
#define DTLB_PR_NOLIMIT  ( SPR_DTLBTR_CI   | \
                          SPR_DTLBTR_URE  | \
                          SPR_DTLBTR_UWE  | \
                          SPR_DTLBTR_SRE  | \
                          SPR_DTLBTR_SWE  )

#define ITLB_PR_NOLIMIT  ( SPR_ITLBTR_CI   | \
                          SPR_ITLBTR_SXE  | \
                          SPR_ITLBTR_UXE  )

/* fails if x is false */
#define ASSERT(x) ((x)?1: fail (__FUNCTION__, __LINE__))

/* Exception vectors */
#define V_RESET       1
#define V_BERR        2
#define V_DPF         3
#define V_IPF         4
#define V_TICK        5
#define V_ALIGN       6
#define V_ILLINSN     7
#define V_INT         8
#define V_DTLB_MISS   9
#define V_ITLB_MISS   10
#define V_RANGE       11
#define V_SYS         12
#define V_TRAP        14

#define debug printf

/* Extern functions */
extern void lo_dmmu_en (void);
extern void lo_immu_en (void);
extern unsigned long call (unsigned long add, unsigned long val);
extern unsigned long call_with_int (unsigned long add, unsigned long val);
extern void trap (void);
extern void b_trap (void);
extern void range (void);
extern void b_range (void);
extern int except_basic (void);
extern void (*test)(void);
extern int load_acc_32 (unsigned long add);
extern int load_acc_16 (unsigned long add);
extern int store_acc_32 (unsigned long add);
extern int store_acc_16 (unsigned long add);
extern int load_b_acc_32 (unsigned long add);
extern int int_trigger (void);
extern int int_loop (void);
extern int jump_back (void);

/* Local functions prototypes */
void dmmu_disable (void);
void immu_disable (void);

/* DTLB mode status */
volatile unsigned long dtlb_val;

/* ITLB mode status */
volatile unsigned long itlb_val;

/* Exception counter */
volatile int except_count;

/* Exception mask */
volatile unsigned long except_mask;

/* Exception efective address */
volatile unsigned long except_ea;

/* Eception PC */
volatile unsigned long except_pc;

unsigned long  excpt_buserr;
unsigned long excpt_dpfault;
unsigned long excpt_ipfault;
unsigned long excpt_tick;
unsigned long excpt_align;
unsigned long excpt_illinsn;
unsigned long excpt_int;
unsigned long excpt_dtlbmiss;
unsigned long excpt_itlbmiss;
unsigned long excpt_range;
unsigned long excpt_syscall;
unsigned long excpt_break;
unsigned long excpt_trap;


void fail (char *func, int line)
{
#ifndef __FUNCTION__
#define __FUNCTION__ "?"
#endif

  immu_disable ();
  dmmu_disable ();

  debug("Test failed in %s:%i\n", func, line);
  report (0xeeeeeeee);
  exit (1);
}

void test_dummy (void)
{
	asm("test:");
	asm("l.addi\t\tr3,r3,1") ;
	asm("l.nop" : :);
}

void copy_test (unsigned long phy_add)
{
	memcpy((void *)phy_add, (void *)&test, 8);
}

/* Bus error handler */
void bus_err_handler (void)
{

  except_mask |= 1 << V_BERR;
  except_count++;
}
  
/* Illegal insn handler */
void ill_insn_handler (void)
{
  except_mask |= 1 << V_ILLINSN;
  except_count++;
}

/* Low priority interrupt handler */
void tick_handler (void)
{

  /* Disable interrupt recognition */
  mtspr(SPR_ESR_BASE, mfspr(SPR_ESR_BASE) & ~SPR_SR_TEE);

  except_mask |= 1 << V_TICK;
  except_count++;
}

/* High priority interrupt handler */
void int_handler (void)
{
  
  /* Disable interrupt recognition */
  mtspr(SPR_ESR_BASE, mfspr(SPR_ESR_BASE) & ~SPR_SR_IEE);

  except_mask |= 1 << V_INT;
  except_count++;
}

/* Trap handler */
void trap_handler (void)
{

  except_mask |= 1 << V_TRAP;
  except_count++;
}

/* Align handler */
void align_handler (void)
{

  except_mask |= 1 << V_ALIGN;
  except_count++;
}

/* Range handler */
void range_handler (void)
{
  /* Disable range exception */
  mtspr (SPR_ESR_BASE, mfspr (SPR_ESR_BASE) & ~SPR_SR_OVE);

  except_mask |= 1 << V_RANGE;
  except_count++;
}

/* DTLB miss exception handler */
void dtlb_miss_handler (void)
{
  unsigned long ea;
  int set, way = 0;
  int i;

  /* Get EA that cause the exception */
  ea = mfspr (SPR_EEAR_BASE);

  /* Find TLB set and LRU way */
  set = (ea / PAGE_SIZE) % DTLB_SETS;
  for (i = 0; i < DTLB_WAYS; i++) {
    if ((mfspr (SPR_DTLBMR_BASE(i) + set) & SPR_DTLBMR_LRU) == 0) {
      way = i;
      break;
    }
  }

  mtspr (SPR_DTLBMR_BASE(way) + set, (ea & SPR_DTLBMR_VPN) | SPR_DTLBMR_V);
  mtspr (SPR_DTLBTR_BASE(way) + set, (ea & SPR_DTLBTR_PPN) | dtlb_val);

  except_mask |= 1 << V_DTLB_MISS;
  except_count++;
}

/* ITLB miss exception handler */
void itlb_miss_handler (void)
{
  unsigned long ea;
  int set, way = 0;
  int i;

  /* Get EA that cause the exception */
  ea = mfspr (SPR_EEAR_BASE);

  /* Find TLB set and LRU way */
  set = (ea / PAGE_SIZE) % ITLB_SETS;
  for (i = 0; i < ITLB_WAYS; i++) {
    if ((mfspr (SPR_ITLBMR_BASE(i) + set) & SPR_ITLBMR_LRU) == 0) {
      way = i;
      break;
    }
  }

  mtspr (SPR_ITLBMR_BASE(way) + set, (ea & SPR_ITLBMR_VPN) | SPR_ITLBMR_V);
  mtspr (SPR_ITLBTR_BASE(way) + set, (ea & SPR_ITLBTR_PPN) | itlb_val);
  except_mask |= 1 << V_ITLB_MISS;
  except_count++;
}

/* Data page fault exception handler */
void dpage_fault_handler (void)
{
  unsigned long ea;
  int set, way = 0;
  int i;

  /* Get EA that cause the exception */
  ea = mfspr (SPR_EEAR_BASE);

  /* Find TLB set and way */
  set = (ea / PAGE_SIZE) % DTLB_SETS;
  for (i = 0; i < DTLB_WAYS; i++) {
    if ((mfspr (SPR_DTLBMR_BASE(i) + set) & SPR_DTLBMR_VPN) == (ea & SPR_DTLBMR_VPN)) {
      way = i;
      break;
    }
  }

  /* Give permission */
  mtspr (SPR_DTLBTR_BASE(way) + set, (mfspr (SPR_DTLBTR_BASE(way) + set) & ~DTLB_PR_NOLIMIT) | dtlb_val);

  except_mask |= 1 << V_DPF;
  except_count++;
}

/* Intstruction page fault exception handler */
void ipage_fault_handler (void)
{
  unsigned long ea;
  int set, way = 0;
  int i;

  /* Get EA that cause the exception */
  ea = mfspr (SPR_EEAR_BASE);

  /* Find TLB set and way */
  set = (ea / PAGE_SIZE) % ITLB_SETS;
  for (i = 0; i < ITLB_WAYS; i++) {
    if ((mfspr (SPR_ITLBMR_BASE(i) + set) & SPR_ITLBMR_VPN) == (ea & SPR_ITLBMR_VPN)) {
      way = i;
      break;
    }
  }

  /* Give permission */
  mtspr (SPR_ITLBTR_BASE(way) + set, (mfspr (SPR_ITLBTR_BASE(way) + set) & ~ITLB_PR_NOLIMIT) | itlb_val);

  except_mask |= 1 << V_IPF;
  except_count++;
}

/*Enable DMMU */
void dmmu_enable (void)
{
  /* Enable DMMU */
  lo_dmmu_en ();
}

/* Disable DMMU */
void dmmu_disable (void)
{
  mtspr (SPR_SR, mfspr (SPR_SR) & ~SPR_SR_DME);
}

/* Enable IMMU */
void immu_enable (void)
{
  /* Enable IMMU */
  lo_immu_en ();
}

/* Disable IMMU */
void immu_disable (void)
{
  mtspr (SPR_SR, mfspr (SPR_SR) & ~SPR_SR_IME);
}

/* Tick timer init */
void tick_init (int period, int hp_int)
{
  /* Disable tick timer exception recognition */
  mtspr(SPR_SR, mfspr(SPR_SR) & ~SPR_SR_TEE);

  /* Set period of one cycle, restartable mode */
  mtspr(SPR_TTMR, SPR_TTMR_IE | SPR_TTMR_RT | (period & SPR_TTMR_TP));
  
  /* Reset counter */
  mtspr(SPR_TTCR, 0);
}

/* Interrupt test */
int interrupt_test (void)
{
  unsigned long ret;
  int i;

  /* Init tick timer */
  tick_init (1, 1);

  /* Reset except counter */
  except_count = 0;
  except_mask = 0;
  except_pc = 0;
  except_ea = 0;

  /* Test normal high priority interrupt trigger */
  ret = call ((unsigned long)&int_trigger, 0);
  ASSERT(except_count == 1);
  ASSERT(except_mask == (1 << V_TICK));
  ASSERT(ret == 0);
  ASSERT(except_pc == (unsigned long)int_trigger + 16);

  /* Reset except counter */
  except_count = 0;
  except_mask = 0;
  except_pc = 0;
  except_ea = 0;

  /* Test inetrrupt in delay slot */
  tick_init (100, 1);

  /* Hopefully we will have interrupt recognition between branch insn and delay slot */
  except_pc = (unsigned long)&int_loop;
  for (i = 0; i < 10; i++) {
    call_with_int (except_pc, RAM_START);
    ASSERT(except_pc == (unsigned long)&int_loop);
  }
 
  return 0;
}

/* ITLB miss test */
int itlb_test (void)
{
  int i, j, ret;
  unsigned long ea, ta;

  /* Invalidate all entries in ITLB */
  for (i = 0; i < ITLB_WAYS; i++) {
    for (j = 0; j < ITLB_SETS; j++) {
      mtspr (SPR_ITLBMR_BASE(i) + j, 0);
      mtspr (SPR_ITLBTR_BASE(i) + j, 0);
    }
  }

  /* Set one to one translation for the use of this program */
  for (i = 0; i < TLB_TEXT_SET_NB; i++) {
    ea = RAM_START + (i*PAGE_SIZE);
    ta = RAM_START + (i*PAGE_SIZE);
    mtspr (SPR_ITLBMR_BASE(0) + i, ea | SPR_ITLBMR_V);
    mtspr (SPR_ITLBTR_BASE(0) + i, ta | ITLB_PR_NOLIMIT);
  } 

  /* Set dtlb no permisions */
  itlb_val = SPR_ITLBTR_CI; 

  /* Reset except counter */
  except_count = 0;
  except_mask = 0;
  except_pc = 0;
  except_ea = 0;

  /* Enable IMMU */
  immu_enable ();

  /* Copy jump instruction to last location of a page */
  ea = RAM_START + (RAM_SIZE/2) + ((TLB_TEXT_SET_NB + 1)*PAGE_SIZE) - 8;
  memcpy((void *)ea, (void *)&jump_back, 12);

  /* Check if there was ITLB miss exception */
  ret = call (ea, 0);
  ASSERT(except_count == 1);
  ASSERT(except_mask == (1 << V_ITLB_MISS));
  ASSERT(except_pc == ea);
  ASSERT(ret == 0);

  /* Set dtlb no permisions */
  itlb_val = SPR_ITLBTR_CI | SPR_ITLBTR_SXE; 

  /* Reset except counter */
  except_count = 0;
  except_mask = 0;
  except_pc = 0;
  except_ea = 0;

  /* Check if there was IPF miss exception */
  ret = call (ea, 0);
  ASSERT(except_count == 1);
  ASSERT(except_mask == (1 << V_IPF));
  ASSERT(except_pc == ea);
  ASSERT(ret == 0);

  /* Set dtlb no permisions */
  itlb_val = SPR_ITLBTR_CI; 

  /* Reset except counter */
  except_count = 0;
  except_mask = 0;
  except_pc = 0;
  except_ea = 0;

  /* Check if there was ITLB miss exception */
  ret = call (ea, 0);
  ASSERT(except_count == 1);
  ASSERT(except_mask == (1 << V_ITLB_MISS));
  ASSERT(except_pc == ea + 4);
  ASSERT(ret == 0);

  /* Set dtlb no permisions */
  itlb_val = SPR_ITLBTR_CI | SPR_ITLBTR_SXE; 

  /* Reset except counter */
  except_count = 0;
  except_mask = 0;
  except_pc = 0;
  except_ea = 0;

  /* Check if there was IPF exception */
  ret = call (ea, 0);
  ASSERT(except_count == 1);
  ASSERT(except_mask == (1 << V_IPF));
  ASSERT(except_pc == ea + 4);
  ASSERT(ret == 0);

  /* Reset except counter */
  except_count = 0;
  except_mask = 0;
  except_pc = 0;
  except_ea = 0;

  ret = call (ea, 0);
  ASSERT(except_count == 0);
  ASSERT(ret == 1);

  /* Disable IMMU */
  immu_disable ();

  return 0;
}

/* DTLB miss test */
int dtlb_test (void)
{
  int i, j, ret;
  unsigned long ea, ta;

  /* Invalidate all entries in DTLB */
  for (i = 0; i < DTLB_WAYS; i++) {
    for (j = 0; j < DTLB_SETS; j++) {
      mtspr (SPR_DTLBMR_BASE(i) + j, 0);
      mtspr (SPR_DTLBTR_BASE(i) + j, 0);
    }
  }

  /* Set one to one translation for the use of this program */
  for (i = 0; i < TLB_DATA_SET_NB; i++) {
    ea = RAM_START + (i*PAGE_SIZE);
    ta = RAM_START + (i*PAGE_SIZE);
    mtspr (SPR_DTLBMR_BASE(0) + i, ea | SPR_ITLBMR_V);
    mtspr (SPR_DTLBTR_BASE(0) + i, ta | DTLB_PR_NOLIMIT);
  } 

  /* Set dtlb no permisions */
  dtlb_val = SPR_DTLBTR_CI;

  /* Reset except counter */
  except_count = 0;
  except_mask = 0;
  except_pc = 0;
  except_ea = 0;

  /* Set pattern */
  ea = RAM_START + (RAM_SIZE/2) + ((TLB_DATA_SET_NB)*PAGE_SIZE);
  REG32(ea) = 0x87654321;

  /* Enable DMMU */
  dmmu_enable ();

  /* Check if there was DTLB miss exception */
  ret = call ((unsigned long)&load_b_acc_32, ea);
  ASSERT(except_count == 1);
  ASSERT(except_mask == (1 << V_DTLB_MISS));
  ASSERT(except_pc == (unsigned long)load_b_acc_32 + 8);
  ASSERT(except_ea == ea);
  ASSERT(ret == 0x12345678);

  /* Set dtlb no permisions */
  dtlb_val = SPR_DTLBTR_CI | SPR_DTLBTR_SRE; 

  /* Reset except counter */
  except_count = 0;
  except_mask = 0;
  except_pc = 0;
  except_ea = 0;

  /* Check if there was DPF miss exception */
  ret = call ((unsigned long)&load_b_acc_32, ea);
  ASSERT(except_count == 1);
  ASSERT(except_mask == (1 << V_DPF));
  ASSERT(except_pc == (unsigned long)load_b_acc_32 + 8);
  ASSERT(except_ea == ea);
  ASSERT(ret == 0x12345678);

  /* Reset except counter */
  except_count = 0;
  except_mask = 0;
  except_pc = 0;
  except_ea = 0;

  ret = call ((unsigned long)&load_b_acc_32, ea);
  ASSERT(except_count == 0);
  ASSERT(ret == 0x87654321);

  /* Disable DMMU */
  dmmu_disable ();

  return 0;
}

/* Bus error test */
int buserr_test (void)
{
  int i, j, ret;
  unsigned long ea, ta;

  /* Invalidate all entries in ITLB */
  for (i = 0; i < ITLB_WAYS; i++) {
    for (j = 0; j < ITLB_SETS; j++) {
      mtspr (SPR_ITLBMR_BASE(i) + j, 0);
      mtspr (SPR_ITLBTR_BASE(i) + j, 0);
    }
  }

  /* Set one to one translation for the use of this program */
  for (i = 0; i < TLB_TEXT_SET_NB; i++) {
    ea = RAM_START + (i*PAGE_SIZE);
    ta = RAM_START + (i*PAGE_SIZE);
    mtspr (SPR_ITLBMR_BASE(0) + i, ea | SPR_ITLBMR_V);
    mtspr (SPR_ITLBTR_BASE(0) + i, ta | ITLB_PR_NOLIMIT);
  } 

  /* Invalidate all entries in DTLB */
  for (i = 0; i < DTLB_WAYS; i++) {
    for (j = 0; j < DTLB_SETS; j++) {
      mtspr (SPR_DTLBMR_BASE(i) + j, 0);
      mtspr (SPR_DTLBTR_BASE(i) + j, 0);
    }
  }

  /* Set one to one translation for the use of this program */
  for (i = 0; i < TLB_DATA_SET_NB; i++) {
    ea = RAM_START + (i*PAGE_SIZE);
    ta = RAM_START + (i*PAGE_SIZE);
    mtspr (SPR_DTLBMR_BASE(0) + i, ea | SPR_ITLBMR_V);
    mtspr (SPR_DTLBTR_BASE(0) + i, ta | DTLB_PR_NOLIMIT);
  } 

  /* Reset except counter */
  except_count = 0;
  except_mask = 0;
  except_pc = 0;
  except_ea = 0;

  /* Set IMMU translation */
  ea = RAM_START + (RAM_SIZE) + ((TLB_TEXT_SET_NB)*PAGE_SIZE);
  itlb_val = SPR_ITLBTR_CI | SPR_ITLBTR_SXE;
  mtspr (SPR_ITLBMR_BASE(0) + TLB_TEXT_SET_NB, (ea & SPR_ITLBMR_VPN) | 
	 SPR_ITLBMR_V);
  // Set translate to invalid address: 0xee000000
  mtspr (SPR_ITLBTR_BASE(0) + TLB_TEXT_SET_NB, (0xee000000 & SPR_ITLBTR_PPN) | 
	 itlb_val);

  /* Enable IMMU */
  immu_enable ();

  /* Check if there was bus error exception */
  ret = call (ea, 0);
  ASSERT(except_count == 1);
  ASSERT(except_mask == (1 << V_BERR));
  ASSERT(except_pc == ea);
  ASSERT(except_ea == ea);

  /* Disable IMMU */
  immu_disable ();

  /* Reset except counter */
  except_count = 0;
  except_mask = 0;
  except_pc = 0;
  except_ea = 0;

  /* Set EA as an invalid memory location */
  ea  = 0xee000000;

  /* Check if there was bus error exception */
  ret = call (ea, 0);
  ASSERT(except_count == 1);
  ASSERT(except_mask == (1 << V_BERR));
  ASSERT(except_pc == ea); 
  ASSERT(except_ea == ea);

  /* Reset except counter */
  except_count = 0;
  except_mask = 0;
  except_pc = 0;
  except_ea = 0;

  /* Set DMMU translation */
  ea = RAM_START + (RAM_SIZE) + ((TLB_DATA_SET_NB)*PAGE_SIZE);
  dtlb_val = SPR_DTLBTR_CI | SPR_DTLBTR_SRE;
  mtspr (SPR_DTLBMR_BASE(0) + TLB_DATA_SET_NB, (ea & SPR_DTLBMR_VPN) | 
	 SPR_DTLBMR_V);
  // Set translate to invalid address: 0xee000000
  mtspr (SPR_DTLBTR_BASE(0) + TLB_DATA_SET_NB, (0xee000000 & SPR_DTLBTR_PPN) | 
	 dtlb_val);

  /* Enable DMMU */
  dmmu_enable ();

  /* Check if there was bus error exception */
  ret = call ((unsigned long)&load_acc_32, ea );
  ASSERT(except_count == 1);
  ASSERT(except_mask == (1 << V_BERR));
  ASSERT(except_pc == (unsigned long)load_acc_32 + 8);
  ASSERT(except_ea == ea);
  ASSERT(ret == 0x12345678);

  /* Disable DMMU */
  dmmu_disable ();

  /* Reset except counter */
  except_count = 0;
  except_mask = 0;
  except_pc = 0;
  except_ea = 0;

  // Set ea to invalid address
  ea = 0xee000000;
  
  /* Check if there was bus error exception */
  ret = call ((unsigned long)&load_acc_32, ea );
  ASSERT(except_count == 1);
  ASSERT(except_mask == (1 << V_BERR));
  ASSERT(except_pc == (unsigned long)load_acc_32 + 8);
  ASSERT(except_ea == ea);
  ASSERT(ret == 0x12345678);

  return 0;
}

/* Illegal instruction test */
int illegal_insn_test (void)
{
  int ret;

  /* Reset except counter */
  except_count = 0;
  except_mask = 0;
  except_pc = 0;
  except_ea = 0;

  /* Set illegal insn code.

     Original code had two bugs. First it jumped to the illegal instruction,
     rather than the immediately preceding l.jr r9 (allowing us to
     recover). Secondly it used 0xffffffff as an illegal instruction. Except
     it isn't - it's l.cust8 0x3ffffff.

     Fixed by a) jumping to the correct location and b) really using an
     illegal instruction (opcode 0x3a. */
  REG32(RAM_START + (RAM_SIZE/2)) = REG32((unsigned long)jump_back + 4);
  REG32(RAM_START + (RAM_SIZE/2) + 4) = 0xe8000000;

  /* Check if there was illegal insn exception. Note that if an illegal
     instruction occurs in a delay slot (like this one), then the exception
     PC is the address of the jump instruction. */
  ret = call (RAM_START + (RAM_SIZE/2), 0 );	/* JPB */

  ASSERT(except_count == 1);
  ASSERT(except_mask == (1 << V_ILLINSN));
  ASSERT(except_pc == (RAM_START + (RAM_SIZE/2)));

  return 0;
}

/* Align test */
int align_test (void)
{
  int ret;

  /* Reset except counter */
  except_count = 0;
  except_mask = 0;
  except_pc = 0;
  except_ea = 0;

  /* Check if there was alignment exception on read insn */
  ret = call ((unsigned long)&load_acc_32, RAM_START + (RAM_SIZE) + (TLB_DATA_SET_NB*PAGE_SIZE) + 1);
  ASSERT(except_count == 1);
  ASSERT(except_mask == (1 << V_ALIGN));
  ASSERT(ret == 0x12345678);
  ASSERT(except_pc == ((unsigned long)(load_acc_32) + 8));
  ASSERT(except_ea == (RAM_START + (RAM_SIZE) + (TLB_DATA_SET_NB*PAGE_SIZE + 1)));

  ret = call ((unsigned long)&load_acc_32, RAM_START + (RAM_SIZE) + (TLB_DATA_SET_NB*PAGE_SIZE) + 2);
  ASSERT(except_count == 2);
  ASSERT(except_mask == (1 << V_ALIGN));
  ASSERT(ret == 0x12345678);
  ASSERT(except_pc == ((unsigned long)(load_acc_32) + 8));
  ASSERT(except_ea == (RAM_START + (RAM_SIZE) + (TLB_DATA_SET_NB*PAGE_SIZE + 2)));

  ret = call ((unsigned long)&load_acc_32, RAM_START + (RAM_SIZE) + (TLB_DATA_SET_NB*PAGE_SIZE) + 3);
  ASSERT(except_count == 3);
  ASSERT(except_mask == (1 << V_ALIGN));
  ASSERT(ret == 0x12345678);
  ASSERT(except_pc == ((unsigned long)(load_acc_32) + 8));
  ASSERT(except_ea == (RAM_START + (RAM_SIZE) + (TLB_DATA_SET_NB*PAGE_SIZE + 3)));

  ret = call ((unsigned long)&load_acc_16, RAM_START + (RAM_SIZE) + (TLB_DATA_SET_NB*PAGE_SIZE) + 1);
  ASSERT(except_count == 4);
  ASSERT(except_mask == (1 << V_ALIGN));
  ASSERT(ret == 0x12345678);
  ASSERT(except_pc == ((unsigned long)(load_acc_16) + 8));
  ASSERT(except_ea == (RAM_START + (RAM_SIZE) + (TLB_DATA_SET_NB*PAGE_SIZE + 1)));

  /* Check alignment exception on write  insn */
  call ((unsigned long)&store_acc_32, RAM_START + (RAM_SIZE) + (TLB_DATA_SET_NB*PAGE_SIZE) + 1);
  ASSERT(except_count == 5);
  ASSERT(except_mask == (1 << V_ALIGN));
  ASSERT(except_pc == ((unsigned long)(store_acc_32) + 8));
  ASSERT(except_ea == (RAM_START + (RAM_SIZE) + (TLB_DATA_SET_NB*PAGE_SIZE + 1)));

  call ((unsigned long)&store_acc_32, RAM_START + (RAM_SIZE) + (TLB_DATA_SET_NB*PAGE_SIZE) + 2);
  ASSERT(except_count == 6);
  ASSERT(except_mask == (1 << V_ALIGN));
  ASSERT(except_pc == ((unsigned long)(store_acc_32) + 8));
  ASSERT(except_ea == (RAM_START + (RAM_SIZE) + (TLB_DATA_SET_NB*PAGE_SIZE + 2)));

  call ((unsigned long)&store_acc_32, RAM_START + (RAM_SIZE) + (TLB_DATA_SET_NB*PAGE_SIZE) + 3);
  ASSERT(except_count == 7);
  ASSERT(except_mask == (1 << V_ALIGN));
  ASSERT(except_pc == ((unsigned long)(store_acc_32) + 8));
  ASSERT(except_ea == (RAM_START + (RAM_SIZE) + (TLB_DATA_SET_NB*PAGE_SIZE + 3)));

  call ((unsigned long)&store_acc_16, RAM_START + (RAM_SIZE) + (TLB_DATA_SET_NB*PAGE_SIZE) + 1);
  ASSERT(except_count == 8);
  ASSERT(except_mask == (1 << V_ALIGN));
  ASSERT(except_pc == ((unsigned long)(store_acc_16) + 8));
  ASSERT(except_ea == (RAM_START + (RAM_SIZE) + (TLB_DATA_SET_NB*PAGE_SIZE + 1)));

  
  ret = call ((unsigned long)&load_b_acc_32, RAM_START + (RAM_SIZE) + (TLB_DATA_SET_NB*PAGE_SIZE) + 1);
  ASSERT(except_count == 9);
  ASSERT(except_mask == (1 << V_ALIGN));
  ASSERT(ret == 0x12345678);
  ASSERT(except_pc == ((unsigned long)(load_b_acc_32) + 8));
  ASSERT(except_ea == (RAM_START + (RAM_SIZE) + (TLB_DATA_SET_NB*PAGE_SIZE + 1)));


  return 0;
}

/* Trap test */
int trap_test (void)
{
  /* Reset except counter */
  except_count = 0;
  except_mask = 0;
  except_pc = 0;
  except_ea = 0;

  /* Check if there was trap exception */
  call ((unsigned long)&trap, 0);
  ASSERT(except_count == 1);
  ASSERT(except_mask == (1 << V_TRAP));
  ASSERT(except_pc == (unsigned long)(trap));

  /* Check if there was trap exception */
  call ((unsigned long)&b_trap, 0);
  ASSERT(except_count == 2);
  ASSERT(except_mask == (1 << V_TRAP));
  ASSERT(except_pc == (unsigned long)(b_trap));

  return 0;
}

/* Range test */
int range_test (void)
{
  /* Reset except counter */
  except_count = 0;
  except_mask = 0;
  except_pc = 0;
  except_ea = 0;

  /* Check if there was range exception */
  mtspr (SPR_SR, mfspr (SPR_SR) | SPR_SR_OVE);
  call ((unsigned long)&range, 0);
  ASSERT(except_count == 1);
  ASSERT(except_mask == (1 << V_RANGE));
  ASSERT(except_pc == (unsigned long)(range));
  ASSERT(except_ea == 0);

  /* Check if there was range exception */
  mtspr (SPR_SR, mfspr (SPR_SR) | SPR_SR_OVE);
  call ((unsigned long)&b_range, 0);
  ASSERT(except_count == 2);
  ASSERT(except_mask == (1 << V_RANGE));
  ASSERT(except_pc == (unsigned long)(b_range));

  return 0;
}

/* Exception priority test */
void except_priority_test (void)
{
  int i, j;
  unsigned long ea, ta, ret;

  /* Invalidate all entries in ITLB */
  for (i = 0; i < ITLB_WAYS; i++) {
    for (j = 0; j < ITLB_SETS; j++) {
      mtspr (SPR_ITLBMR_BASE(i) + j, 0);
      mtspr (SPR_ITLBTR_BASE(i) + j, 0);
    }
  }
  
  /* Set one to one translation for the use of this program */
  for (i = 0; i < TLB_TEXT_SET_NB; i++) {
    ea = RAM_START + (i*PAGE_SIZE);
    ta = RAM_START + (i*PAGE_SIZE);
    mtspr (SPR_ITLBMR_BASE(0) + i, ea | SPR_ITLBMR_V);
    mtspr (SPR_ITLBTR_BASE(0) + i, ta | ITLB_PR_NOLIMIT);
  } 

  /* Set dtlb no permisions */
  itlb_val = SPR_ITLBTR_CI;

  /* Invalidate all entries in DTLB */
  for (i = 0; i < DTLB_WAYS; i++) {
    for (j = 0; j < DTLB_SETS; j++) {
      mtspr (SPR_DTLBMR_BASE(i) + j, 0);
      mtspr (SPR_DTLBTR_BASE(i) + j, 0);
    }
  }

  /* Set one to one translation for the use of this program */
  for (i = 0; i < TLB_DATA_SET_NB; i++) {
    ea = RAM_START + (i*PAGE_SIZE);
    ta = RAM_START + (i*PAGE_SIZE);
    mtspr (SPR_DTLBMR_BASE(0) + i, ea | SPR_ITLBMR_V);
    mtspr (SPR_DTLBTR_BASE(0) + i, ta | DTLB_PR_NOLIMIT);
  } 

  /* Init tick timer */
  tick_init (1, 1);

  /* Set dtlb no permisions */
  dtlb_val = SPR_DTLBTR_CI;

  /* Reset except counter */
  except_count = 0;
  except_mask = 0;
  except_pc = 0;
  except_ea = 0;

  /* Enable IMMU */
  immu_enable ();

  /* The following is currently disabled due to differing behavior between
     or1ksim and the OR1200 RTL. Or1ksim appears to receive only 1 exception
     during the call_with_int() call. The OR1200 correctly, in my opionion,
     reports 2 exceptions - ITLB miss and tick timer. -- Julius
     
     TODO: Investigate why or1ksim isn't reporting ITLB miss.

  */

#if 0
  /* Check if there was INT exception */
  call_with_int (RAM_START + (RAM_SIZE) + (TLB_TEXT_SET_NB*PAGE_SIZE), 0);
  printf("ec:%d 0x%lx\n",except_count,except_mask);  ASSERT(except_count == 2);
  ASSERT(except_mask == ((1 << V_TICK) | (1 << V_ITLB_MISS)));
  printf("epc %8lx\n",except_pc);
  ASSERT(except_pc == (RAM_START + (RAM_SIZE) + (TLB_TEXT_SET_NB*PAGE_SIZE)));

  /* Reset except counter */
  except_count = 0;
  except_mask = 0;
  except_pc = 0;
  except_ea = 0;
#endif

  /* Check if there was ITLB exception */
  call (RAM_START + (RAM_SIZE) + (TLB_TEXT_SET_NB*PAGE_SIZE), 0);
  ASSERT(except_count == 1);
  ASSERT(except_mask == (1 << V_ITLB_MISS));
  ASSERT(except_pc == (RAM_START + (RAM_SIZE) + (TLB_TEXT_SET_NB*PAGE_SIZE)));
  ASSERT(except_ea == (RAM_START + (RAM_SIZE) + (TLB_TEXT_SET_NB*PAGE_SIZE)));

  /* Set dtlb permisions */
  itlb_val |= SPR_ITLBTR_SXE;

  /* Reset except counter */
  except_count = 0;
  except_mask = 0;
  except_pc = 0;
  except_ea = 0;

  /* Check if there was IPF exception */
  call (RAM_START + (RAM_SIZE) + (TLB_TEXT_SET_NB*PAGE_SIZE), 0);
  ASSERT(except_count == 1);
  ASSERT(except_mask == (1 << V_IPF));
  ASSERT(except_pc == (RAM_START + (RAM_SIZE) + (TLB_TEXT_SET_NB*PAGE_SIZE)));
  ASSERT(except_ea == (RAM_START + (RAM_SIZE) + (TLB_TEXT_SET_NB*PAGE_SIZE)));

  /* Reset except counter */
  except_count = 0;
  except_mask = 0;
  except_pc = 0;
  except_ea = 0;
  
  /* Disable MMU */
  immu_disable ();

  /* Set illegal instruction. JPB. Use a really illegal instruction, not
     l.cust8 0x3ffffff. */
  REG32(RAM_START + (RAM_SIZE/2) + (TLB_TEXT_SET_NB*PAGE_SIZE) + 0) = 0x00000000;
  REG32(RAM_START + (RAM_SIZE/2) + (TLB_TEXT_SET_NB*PAGE_SIZE) + 4) = 0xe8000000;
  REG32(RAM_START + (RAM_SIZE/2) + (TLB_TEXT_SET_NB*PAGE_SIZE) + 8) = 0x00000000;

  /* Check if there was illegal insn exception */
  call (RAM_START + (RAM_SIZE/2) + (TLB_TEXT_SET_NB*PAGE_SIZE) + 4, 0);
  ASSERT(except_count == 1);
  ASSERT(except_mask == (1 << V_ILLINSN));
  ASSERT(except_pc == (RAM_START + (RAM_SIZE/2) + (TLB_TEXT_SET_NB*PAGE_SIZE) + 4));
  ASSERT(except_ea == (RAM_START + (RAM_SIZE/2) + (TLB_TEXT_SET_NB*PAGE_SIZE) + 4 ));

  /* Reset except counter */
  except_count = 0;
  except_mask = 0;
  except_pc = 0;
  except_ea = 0;

  /* Enable DMMU */
  dmmu_enable ();

  /* Check if there was alignment exception on read insn */
  ret = call ((unsigned long)&load_acc_32, RAM_START + (RAM_SIZE) + (TLB_DATA_SET_NB*PAGE_SIZE) + 1);
  ASSERT(except_count == 1);
  ASSERT(except_mask == (1 << V_ALIGN));
  ASSERT(ret == 0x12345678);
  ASSERT(except_pc == ((unsigned long)(load_acc_32) + 8));
  ASSERT(except_ea == (RAM_START + (RAM_SIZE) + (TLB_DATA_SET_NB*PAGE_SIZE + 1)));

  /* Reset except counter */
  except_count = 0;
  except_mask = 0;
  except_pc = 0;
  except_ea = 0;

  /* Check if there was DTLB exception */
  ret = call ((unsigned long)&load_acc_32, RAM_START + (RAM_SIZE) + (TLB_DATA_SET_NB*PAGE_SIZE));
  ASSERT(except_count == 1);
  ASSERT(except_mask == (1 << V_DTLB_MISS));
  ASSERT(ret == 0x12345678);
  ASSERT(except_pc == ((unsigned long)(load_acc_32) + 8));
  ASSERT(except_ea == (RAM_START + (RAM_SIZE) + (TLB_DATA_SET_NB*PAGE_SIZE)));

  /* Reset except counter */
  except_count = 0;
  except_mask = 0;
  except_pc = 0;
  except_ea = 0;

  /* Set dtlb permisions */
  dtlb_val |= SPR_DTLBTR_SRE;

  /* Check if there was DPF exception */
  ret = call ((unsigned long)&load_acc_32, RAM_START + (RAM_SIZE) + (TLB_DATA_SET_NB*PAGE_SIZE));
  ASSERT(except_count == 1);
  ASSERT(except_mask == (1 << V_DPF));
  ASSERT(ret == 0x12345678);
  ASSERT(except_pc == ((unsigned long)(load_acc_32) + 8));
  ASSERT(except_ea == (RAM_START + (RAM_SIZE) + (TLB_DATA_SET_NB*PAGE_SIZE)));
 
  /* Reset except counter */
  except_count = 0;
  except_mask = 0;
  except_pc = 0;
  except_ea = 0;

  /* Check if there was trap exception */
  call ((unsigned long)&trap, 0);
  ASSERT(except_count == 1);
  ASSERT(except_mask == (1 << V_TRAP));
  ASSERT(except_pc == (unsigned long)(trap));

}

int main (void)
{
  int ret;

  printf("except_test\n");

  /* Register bus error handler */
  excpt_buserr = (unsigned long)bus_err_handler;

  /* Register illegal insn handler */
  excpt_illinsn = (unsigned long)ill_insn_handler;

  /* Register tick timer exception handler */
  excpt_tick = (unsigned long)tick_handler;

  /* Register external interrupt handler */
  excpt_int = (unsigned long)int_handler;

  /* Register ITLB miss handler */
  excpt_itlbmiss = (unsigned long)itlb_miss_handler;

  /* Register instruction page fault handler */
  excpt_ipfault = (unsigned long)ipage_fault_handler;

  /* Register DTLB miss handler */
  excpt_dtlbmiss = (unsigned long)dtlb_miss_handler;

  /* Register data page fault handler */
  excpt_dpfault = (unsigned long)dpage_fault_handler;

  /* Register trap handler */
  excpt_trap = (unsigned long)trap_handler;

  /* Register align handler */
  excpt_align = (unsigned long)align_handler;

  /* Register range handler */
  excpt_range = (unsigned long)range_handler;

  /* Exception basic test */
  ret = except_basic ();
  ASSERT(ret == 0);
printf("interupt_test\n");  
  /* Interrupt exception test */
  interrupt_test ();

printf("itlb_test\n");  
  /* ITLB exception test */
  itlb_test ();

printf("dtlb_test\n");
  /* DTLB exception test */
  dtlb_test ();

printf("buserr_test\n");
  /* Bus error exception test */
  buserr_test ();

printf("illegal_insn_test\n");
  /* Bus error exception test */
  /* Illegal insn test */
  illegal_insn_test ();

printf("align_test\n");
  /* Alignment test */
  align_test ();

printf("trap_test\n");
  /* Trap test */
  trap_test ();

printf("except_priority_test\n");
  /* Range test */
//  range_test ();

  /* Exception priority test */
  except_priority_test ();

  report (0xdeaddead);
  exit (0);
 
  return 0;
}

