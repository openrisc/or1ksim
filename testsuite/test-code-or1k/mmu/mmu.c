/* mmu.c. Test of the Or1ksim MMU

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

/* This is MMU test for OpenRISC 1200 */

// Tests most functions of the MMUs.
// The tests attempt to keep the areas of instruction and data memory working 
// as they should, while testing around them, higher in memory. Some linker 
// variables are relied upon by the interrupt routines for checking if areas 
// are within the expected text areas of memory, but usually we just keep 
// above the stack (also set in the linker script) and things go OK.

// The tests for translation setup address translation in the MMU from areas 
// in the 512kB - 1024kB region (64 sets in OR1200, 8kByte per set) to halfway
// through RAM. Usually the sets taht would encompass the actual program are 
// skipped (TLB_TEXT_SET_NB) ie, we say how many sets encompass the program 
// text for itlb tests and only do tests above it (so, on sets 8-64, meaning 
// when we enable the iMMU and say have the execution bits disabled to force a
// page fault the program gets to continue and is translated 1-1 while 
// accesses to the areas we are testing will cause a page fault) but it should
//  still work to test all sets.

// In essense, the tests are aware that they could be operating out of the 
// same memory the tests are being performed in and takes care of this.

// The use of the "match_space" variable is for the addresses we'll access 
// when a non-1-1 translation will occur, otherwise usually the 
// "translate_space" variable by itself is used (it's first setup to the 
// desired address, and either data or the return instructions are placed 
// where we expect the MMU to translate to)

#include "spr-defs.h"
#include "support.h"

// Set this to 1 to enable the DMMU tests
#define DO_DMMU_TESTS 1

// Symbols defined in linker script
extern unsigned long _ram_end;
extern unsigned long _src_beg;

unsigned long text_begin_addr;
unsigned long end_data_addr;

/* For shorter simulation run */
#define RTL_SIM 1

/* Define RAM physical location and size 
Bottom half will be used for this program, the rest 
will be used for testing */
#define RAM_START   0x00000000
// Assume only 2MB memory
#define RAM_SIZE    0x00200000

#define VM_BASE 0xc0000000

/* What is the last address in ram that is used by this program */
#define TEXT_START_ADD text_begin_addr
#define TEXT_END_ADD end_data_addr
#define DATA_END_ADD end_data_addr

// Uncomment the following to determine the page to test from at run-time
unsigned long TLB_TEXT_SET_NB;
unsigned long TLB_DATA_SET_NB;

/* MMU page size */
#define PAGE_SIZE 8192

/* Number of DTLB sets used (power of 2, max is 256) */
#define DTLB_SETS 64

/* Number of DTLB ways (1, 2, 3 etc., max is 4). */
#define DTLB_WAYS 1

/* Number of ITLB sets used (power of 2, max is 256) */
#define ITLB_SETS 64
 
/* Number of ITLB ways (1, 2, 3 etc., max is 4). */
#define ITLB_WAYS 1
 
/* TLB mode codes */
#define TLB_CODE_ONE_TO_ONE     0x00000000
#define TLB_CODE_PLUS_ONE_PAGE  0x10000000
#define TLB_CODE_MINUS_ONE_PAGE 0x20000000

#define TLB_CODE_MASK   0xfffff000
#define TLB_PR_MASK     0x00000fff
#define DTLB_PR_NOLIMIT  (SPR_DTLBTR_URE  | \
                          SPR_DTLBTR_UWE  | \
                          SPR_DTLBTR_SRE  | \
                          SPR_DTLBTR_SWE  )

#define ITLB_PR_NOLIMIT  (SPR_ITLBTR_SXE  | \
                          SPR_ITLBTR_UXE  )



/* fails if x is false */
#define ASSERT(x) ((x)?1: fail (__FUNCTION__, __LINE__))

// iMMU and dMMU enable functions
extern void lo_dmmu_en (void);
extern void lo_immu_en (void);

/* Local functions prototypes */
void dmmu_disable (void);
void immu_disable (void);

// Machine code for l.jr r9 and then l.nop
#define OR32_L_JR_R9 0x44004800
#define OR32_L_NOP 0x15000000

/* Global variables */
extern unsigned long ram_end;

/* DTLB mode status */
volatile unsigned long dtlb_val;

/* ITLB mode status */
volatile unsigned long itlb_val;

/* DTLB miss counter */
volatile int dtlb_miss_count;

/* Data page fault counter */
volatile int dpage_fault_count;

/* ITLB miss counter */
volatile int itlb_miss_count;

/* Instruction page fault counter */
volatile int ipage_fault_count;

/* EA of last DTLB miss exception */
unsigned long dtlb_miss_ea;

/* EA of last data page fault exception */
unsigned long dpage_fault_ea;

/* EA of last ITLB miss exception */
unsigned long itlb_miss_ea;

/* EA of last insn page fault exception */
unsigned long ipage_fault_ea;


#define sys_call() __asm__ __volatile__("l.sys\t0");
/*
void sys_call (void)
{
  asm("l.sys\t0");
}
*/
void fail (char *func, int line)
{
#ifndef __FUNCTION__
#define __FUNCTION__ "?"
#endif

  /* Trigger sys call exception to enable supervisor mode again */
  sys_call ();

  immu_disable ();
  dmmu_disable ();

  report(line);
  report (0xeeeeeeee);
  exit (1);
}

void call(unsigned long add)
{
  asm("l.jalr\t\t%0" : : "r" (add) : "r9", "r11");
  asm("l.nop" : :);
}

void jump(void)
{
  return;
  /*asm("_jr:");
  asm("l.jr\t\tr9") ;
  asm("l.nop" : :);*/
}

void copy_jump(unsigned long phy_add)
{
  memcpy((void *)phy_add, (void *)&jump, (8*4));
}

/* Bus error exception handler */
void bus_err_handler (void)
{
  /* This shouldn't happend */
  printf("Test failed: Bus error\n");
  report (0xeeeeeeee);
  exit (1);
}

/* Illegal insn exception handler */
void ill_insn_handler (void)
{
  /* This shouldn't happend */
  printf("Test failed: Illegal insn\n");
  report (0xeeeeeeee);
  exit (1);
}

/* Sys call exception handler */
void sys_call_handler (void)
{
  /* Set supervisor mode */
  mtspr (SPR_ESR_BASE, mfspr (SPR_ESR_BASE) | SPR_SR_SM);
}

/* DTLB miss exception handler */
void dtlb_miss_handler (void)
{
  unsigned long ea, ta, tlbtr;
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

  printf("dtlb miss ea = %.8lx set = %d way = %d\n", ea, set, way);

  // Anything under the stack belongs to the program, direct tranlsate it
  if (ea < (unsigned long)&_ram_end){
    /* If this is acces to data of this program set one to one translation */
    mtspr (SPR_DTLBMR_BASE(way) + set, (ea & SPR_DTLBMR_VPN) | SPR_DTLBMR_V);
    mtspr (SPR_DTLBTR_BASE(way) + set, (ea & SPR_DTLBTR_PPN) | DTLB_PR_NOLIMIT);
    return;
  }

  /* Update DTLB miss counter and EA */
  dtlb_miss_count++;
  dtlb_miss_ea = ea;

  // Everything gets translated back to the space halfway through RAM
  ta = (set*PAGE_SIZE) + RAM_START + (RAM_SIZE/2);
  tlbtr = (ta & SPR_DTLBTR_PPN) | (dtlb_val & TLB_PR_MASK);
  printf("ta = %.8lx tlbtr = %.8lx dtlb_val = %.8lx\n",ta, tlbtr, dtlb_val);

  /* Set DTLB entry */
  mtspr (SPR_DTLBMR_BASE(way) + set, (ea & SPR_DTLBMR_VPN) | SPR_DTLBMR_V);
  mtspr (SPR_DTLBTR_BASE(way) + set, tlbtr);
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
    if ((mfspr (SPR_DTLBMR_BASE(i) + set) & SPR_DTLBMR_VPN) == 
	(ea & SPR_DTLBMR_VPN)) {
      way = i;
      break;
    }
  }

  printf("ea = %.8lx set = %d way = %d\n", ea, set, way);

  if (((RAM_START <= ea) & (ea < DATA_END_ADD) ) | 
      ((TEXT_START_ADD <= ea) & (ea < TEXT_END_ADD))) {
    /* If this is acces to data of this program set one to one translation */
    mtspr (SPR_DTLBTR_BASE(way) + set, (ea & SPR_DTLBTR_PPN) | DTLB_PR_NOLIMIT);
    return;
  }

  /* Update data page fault counter and EA */
  dpage_fault_count++;
  dpage_fault_ea = ea;

  /* Give permission */
  mtspr (SPR_DTLBTR_BASE(way) + set, 
	 (mfspr (SPR_DTLBTR_BASE(way) + set) & ~DTLB_PR_NOLIMIT) | dtlb_val);
}

 
/* ITLB miss exception handler */
void itlb_miss_handler (void)
{
  unsigned long ea, ta, tlbtr;
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

  printf("itlb miss ea = %.8lx set = %d way = %d\n", ea, set, way);

  if ((TEXT_START_ADD <= ea) && (ea < TEXT_END_ADD)) {
    /* If this is acces to data of this program set one to one translation */
    mtspr (SPR_ITLBMR_BASE(way) + set, (ea & SPR_ITLBMR_VPN) | SPR_ITLBMR_V);
    mtspr (SPR_ITLBTR_BASE(way) + set, (ea & SPR_ITLBTR_PPN) | ITLB_PR_NOLIMIT);
    return;
  }

  /* Update ITLB miss counter and EA */
  itlb_miss_count++;
  itlb_miss_ea = ea;

  /* Whatever access is in progress, translated address have to point to 
     physical RAM */

  ta = RAM_START + (RAM_SIZE/2) + (set*PAGE_SIZE);
  tlbtr = (ta & SPR_ITLBTR_PPN) | (itlb_val & TLB_PR_MASK);

  printf("ta = %.8lx\n", ta);

  /* Set ITLB entry */
  mtspr (SPR_ITLBMR_BASE(way) + set, (ea & SPR_ITLBMR_VPN) | SPR_ITLBMR_V);
  mtspr (SPR_ITLBTR_BASE(way) + set, tlbtr);
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

  printf("ipage fault: ea = %.8lx set = %d way = %d\n", ea, set, way);

  if ((TEXT_START_ADD <= ea) && (ea < TEXT_END_ADD)) {
    /* If this is acces to data of this program set one to one translation */
    mtspr (SPR_DTLBTR_BASE(way) + set, (ea & SPR_DTLBTR_PPN) | ITLB_PR_NOLIMIT);
    return;
  }
  
  printf("ipage fault was outside of code area\n");
  
  /* Update instruction page fault counter and EA */
  ipage_fault_count++;
  ipage_fault_ea = ea;

  /* Give permission */
  mtspr (SPR_ITLBTR_BASE(way) + set, (mfspr (SPR_ITLBTR_BASE(way) + set) & ~ITLB_PR_NOLIMIT) | itlb_val);
}

/* Invalidate all entries in DTLB and enable DMMU */
void dmmu_enable (void)
{
  /* Register DTLB miss handler */
	//add_handler(0x9, dtlb_miss_handler);
   excpt_dtlbmiss = (unsigned long)dtlb_miss_handler;

  /* Register data page fault handler */
  //add_handler(0x3, dpage_fault_handler);
  excpt_dpfault = (unsigned long)dpage_fault_handler;

  /* Enable DMMU */
  lo_dmmu_en ();

}

/* Disable DMMU */
void dmmu_disable (void)
{
  mtspr (SPR_SR, mfspr (SPR_SR) & ~SPR_SR_DME);
}

/* Invalidate all entries in ITLB and enable IMMU */
void immu_enable (void)
{
  /* Register ITLB miss handler */
  //add_handler(0xa, itlb_miss_handler);
  excpt_itlbmiss = (unsigned long)itlb_miss_handler;
  
  /* Register instruction page fault handler */
  //add_handler(0x4, ipage_fault_handler);
  excpt_ipfault = (unsigned long)ipage_fault_handler;

  /* Enable IMMU */
  lo_immu_en ();

}

/* Disable IMMU */
void immu_disable (void)
{
  mtspr (SPR_SR, mfspr (SPR_SR) & ~SPR_SR_IME);
}

void write_pattern(unsigned long start, unsigned long end)
{
  unsigned long add;
  
  add = start;
  while (add < end) {
    REG32(add) = add;
    add += PAGE_SIZE;
  }

}

/* Translation address register test
Set various translation and check the pattern */
int dtlb_translation_test (void)
{
  int i, j;
  unsigned long ea, ta;

  /* Disable DMMU */
  dmmu_disable();

  printf("dtlb translation test set\n");

  /* Invalidate all entries in DTLB */
  for (i = 0; i < DTLB_WAYS; i++) {
    for (j = 0; j < DTLB_SETS; j++) {
      mtspr (SPR_DTLBMR_BASE(i) + j, 0);
      mtspr (SPR_DTLBTR_BASE(i) + j, 0);
    }
  }

  /* Set one to one translation for program's data space */
  for (i = 0; i < TLB_DATA_SET_NB; i++) {
    ea = RAM_START + (i*PAGE_SIZE);
    ta = RAM_START + (i*PAGE_SIZE);
    mtspr (SPR_DTLBMR_BASE(0) + i, ea | SPR_DTLBMR_V);
    mtspr (SPR_DTLBTR_BASE(0) + i, ta | (DTLB_PR_NOLIMIT));
  } 

  /* Set dtlb permisions */
  dtlb_val = DTLB_PR_NOLIMIT;

  /* Write test pattern */
  for (i = 0; i < DTLB_SETS; i++) {
    ea = RAM_START + (RAM_SIZE/2) + (i*PAGE_SIZE);
    REG32(ea) = i;
    ea = RAM_START + (RAM_SIZE/2) + ((i + 1)*PAGE_SIZE) - 4;
    REG32(ea) = 0xffffffff - i;
  }
   
  /* Set one to one translation */
  for (i = TLB_DATA_SET_NB; i < DTLB_SETS; i++) {
    ea = RAM_START + (RAM_SIZE/2) + (i*PAGE_SIZE);
    ta = RAM_START + (RAM_SIZE/2) + (i*PAGE_SIZE);
    mtspr (SPR_DTLBMR_BASE(DTLB_WAYS - 1) + i, ea | SPR_DTLBMR_V);
    mtspr (SPR_DTLBTR_BASE(DTLB_WAYS - 1) + i, ta | DTLB_PR_NOLIMIT);
  }
  
  /* Enable DMMU */
  dmmu_enable();

  /* Check the pattern */
  for (i = TLB_DATA_SET_NB; i < DTLB_SETS; i++) {
    ea = RAM_START + (RAM_SIZE/2) + (i*PAGE_SIZE);
    ASSERT(REG32(ea) == i);
    ea = RAM_START + (RAM_SIZE/2) + ((i + 1)*PAGE_SIZE) - 4;
    ASSERT(REG32(ea) == (0xffffffff - i));
  }

  /* Write new pattern */
  for (i = TLB_DATA_SET_NB; i < DTLB_SETS; i++) {
    ea = RAM_START + (RAM_SIZE/2) + (i*PAGE_SIZE);
    REG32(ea) = 0xffffffff - i;
    ea = RAM_START + (RAM_SIZE/2) + ((i + 1)*PAGE_SIZE) - 4;
    REG32(ea) = i;
  }

  /* Set 0 -> RAM_START + (RAM_SIZE/2) translation */
  for (i = TLB_DATA_SET_NB; i < DTLB_SETS; i++) {
    ea = i*PAGE_SIZE;
    ta = RAM_START + (RAM_SIZE/2) + (i*PAGE_SIZE);
    mtspr (SPR_DTLBMR_BASE(DTLB_WAYS - 1) + i, ea | SPR_DTLBMR_V);
    mtspr (SPR_DTLBTR_BASE(DTLB_WAYS - 1) + i, ta | DTLB_PR_NOLIMIT);
  }

  /* Check the pattern */
  for (i = TLB_DATA_SET_NB; i < DTLB_SETS; i++) {
    ea = i*PAGE_SIZE;
    ASSERT(REG32(ea) == (0xffffffff - i));
    ea = ((i + 1)*PAGE_SIZE) - 4;
    ASSERT(REG32(ea) == i);
  }

  /* Write new pattern */
  for (i = TLB_DATA_SET_NB; i < DTLB_SETS; i++) {
    REG32(i*PAGE_SIZE) = i;
    REG32(((i + 1)*PAGE_SIZE) - 4) = 0xffffffff - i; 
  }

  /* Set hi -> lo, lo -> hi translation */
  for (i = TLB_DATA_SET_NB; i < DTLB_SETS; i++) {
    ea = RAM_START + (RAM_SIZE/2) + (i*PAGE_SIZE);
    ta = RAM_START + (RAM_SIZE/2) + ((DTLB_SETS - i - 1 + TLB_DATA_SET_NB)*
				     PAGE_SIZE);
    mtspr (SPR_DTLBMR_BASE(DTLB_WAYS - 1) + i, ea | SPR_DTLBMR_V);
    mtspr (SPR_DTLBTR_BASE(DTLB_WAYS - 1) + i, ta | DTLB_PR_NOLIMIT);
  }

  /* Check the pattern */
  for (i = TLB_DATA_SET_NB; i < DTLB_SETS; i++) {
    ea = RAM_START + (RAM_SIZE/2) + (i*PAGE_SIZE);
    ASSERT(REG32(ea) == (DTLB_SETS - i - 1 + TLB_DATA_SET_NB));
    ea = RAM_START + (RAM_SIZE/2) + ((i + 1)*PAGE_SIZE) - 4;
    ASSERT(REG32(ea) == (0xffffffff - DTLB_SETS + i + 1 - TLB_DATA_SET_NB));
  }

  /* Write new pattern */
  for (i = TLB_DATA_SET_NB; i < DTLB_SETS; i++) {
    REG32(RAM_START + (RAM_SIZE/2) + (i*PAGE_SIZE)) = 0xffffffff - i;
    REG32(RAM_START + (RAM_SIZE/2) + ((i + 1)*PAGE_SIZE) - 4) = i;
  }

  /* Disable DMMU */
  dmmu_disable();

  /* Check the pattern */
  for (i = TLB_DATA_SET_NB; i < DTLB_SETS; i++) {
    ea = RAM_START + (RAM_SIZE/2) + (i*PAGE_SIZE);
    ASSERT(REG32(ea) == (0xffffffff - DTLB_SETS + i + 1 - TLB_DATA_SET_NB));
    ea = RAM_START + (RAM_SIZE/2) + ((i + 1)*PAGE_SIZE) - 4;
    ASSERT(REG32(ea) == (DTLB_SETS - i - 1 + TLB_DATA_SET_NB));
  }
  
  printf("OK\n");
  printf("-------------------------------------------\n");

  return 0;
}

/* EA match register test
Shifting one in DTLBMR and performing accesses to boundaries
of the page, checking the triggering of exceptions */
int dtlb_match_test (int way, int set)
{
  int i, j, tmp;
  unsigned long large_offset_addr;
  unsigned long ea, ta;
  unsigned long match_space;
  unsigned long translate_space;
  unsigned long program_space_offset;

  /* Disable DMMU */
  dmmu_disable();

  printf("dtlb_match_test - way %d set %d\n",way, set);

  /* Invalidate all entries in DTLB */
  for (i = 0; i < DTLB_WAYS; i++) {
    for (j = 0; j < DTLB_SETS; j++) {
      mtspr (SPR_DTLBMR_BASE(i) + j, 0);
      mtspr (SPR_DTLBTR_BASE(i) + j, 0);
    }
  }

  // Program text/data pages should be 1-1 translation
  for (i = 0; i < TLB_DATA_SET_NB; i++) {
    ea = RAM_START + (i*PAGE_SIZE);
    ta = RAM_START + (i*PAGE_SIZE);
    mtspr (SPR_DTLBMR_BASE(0) + i, ea | SPR_DTLBMR_V);
    mtspr (SPR_DTLBTR_BASE(0) + i, ta | DTLB_PR_NOLIMIT);
  } 

  /* Set dtlb permisions */
  dtlb_val = DTLB_PR_NOLIMIT;

  // Determine offset required to skip past area in use by program
  program_space_offset = 0; 
  while (program_space_offset <= DATA_END_ADD) 
	  program_space_offset += (PAGE_SIZE*DTLB_SETS);

  // Setup match area address - based at halfway through RAM, 
  // and then offset by the area encompassed by the set we wish to test.
  // TODO: Ensure this is on a page boundary
  translate_space = RAM_START + (RAM_SIZE/2) + (set*PAGE_SIZE);

  /* Set pattern */
  // Last word of page before the one covered by this set
  REG32(translate_space - 4) = 0x00112233;

  // First word of page covered by this set
  REG32(translate_space) = 0x44556677;

  // Last word of page covered by this set
  REG32(translate_space + PAGE_SIZE - 4) = 0x8899aabb;

  // First word of page covered by next set
  REG32(translate_space + PAGE_SIZE) = 0xccddeeff;
  
  /* Enable DMMU */
  dmmu_enable();

  /* Shifting one in DTLBMR */ 
  i = 0;

  // Setup match space - place we will do accesses to, and have them
  // tranlsated into the translate space addresses. Ensure it is above the end
  // of the program's space.
  match_space = program_space_offset + (set*PAGE_SIZE);
  
  // This is used to provide a large match offset (ie. virtual address)
  // We will offset the match address by this each time. This value is 
  // shifted left after each test, ensuring the high bits of address are 
  // tested.
  large_offset_addr = (PAGE_SIZE*DTLB_SETS); // 8kB * 64, 512KB

  while (large_offset_addr != 0x00000000) { 
    // Set MATCH register for the areas we will access explicitly, and 
    // validate them.
    mtspr (SPR_DTLBMR_BASE(way) + set, match_space | SPR_DTLBMR_V);

    // Set TRANSLATE register to the areas where we have set our data
    mtspr (SPR_DTLBTR_BASE(way) + set, translate_space | DTLB_PR_NOLIMIT);

    /* Reset DTLB miss counter and EA */
    dtlb_miss_count = 0;
    dtlb_miss_ea = 0;
    printf("testing match space %8lx translate space %8lx\n",match_space,
	   translate_space);
    // Only do the test if we won't tread on any memory being used for data
    // and stack. It is unlikely this would occur.
    if (match_space > DATA_END_ADD){
      
      /* Read last address of previous page */
      tmp = REG32(match_space - 4);
      ASSERT(tmp == 0x00112233);
      ASSERT(dtlb_miss_count == 1);
      
      /* Read first address of the page */
      tmp = REG32(match_space);
      ASSERT(tmp == 0x44556677);
      ASSERT(dtlb_miss_count == 1);
 
      /* Read last address of the page */
      tmp = REG32(match_space + PAGE_SIZE - 4);
      ASSERT(tmp == 0x8899aabb);
      ASSERT(dtlb_miss_count == 1);

      /* Read first address of next page */
      tmp = REG32(match_space + PAGE_SIZE);
      ASSERT(tmp == 0xccddeeff);
      ASSERT(dtlb_miss_count == 2);
    }

    i++;
    large_offset_addr = (PAGE_SIZE*DTLB_SETS) << i;
    match_space = large_offset_addr + program_space_offset + (set*PAGE_SIZE);

    // Clear MR/T for space we just tested
    for (j = 0; j < DTLB_WAYS; j++) {
      mtspr (SPR_DTLBMR_BASE(j) + ((set - 1) & (DTLB_SETS - 1)), 0);
      mtspr (SPR_DTLBMR_BASE(j) + ((set + 1) & (DTLB_SETS - 1)), 0);
    }
  }

  /* Disable DMMU */
  dmmu_disable();

  printf("OK\n");
  printf("-------------------------------------------\n");

  return 0;
}
 
/* Valid bit test
Set all ways of one set to be invalid, perform
access so miss handler will set them to valid, 
try access again - there should be no miss exceptions */
int dtlb_valid_bit_test (int set)
{
  int i, j;
  unsigned long ea, ta;

  /* Disable DMMU */
  dmmu_disable();

  printf("dtlb_valid_bit_test, set %d\n", set);

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
    mtspr (SPR_DTLBMR_BASE(0) + i, ea | SPR_DTLBMR_V);
    mtspr (SPR_DTLBTR_BASE(0) + i, ta | DTLB_PR_NOLIMIT);
  } 

  /* Reset DTLB miss counter and EA */
  dtlb_miss_count = 0;
  dtlb_miss_ea = 0;
 
  /* Set dtlb permisions */
  dtlb_val = DTLB_PR_NOLIMIT;

  /* Resetv DTLBMR for every way */
  for (i = 0; i < DTLB_WAYS; i++) {
    mtspr (SPR_DTLBMR_BASE(i) + set, 0);
  }

  /* Enable DMMU */
  dmmu_enable();

  /* Perform writes to address, that is not in DTLB */
  for (i = 0; i < DTLB_WAYS; i++) {
    REG32(RAM_START + RAM_SIZE + (i*DTLB_SETS*PAGE_SIZE) + (set*PAGE_SIZE)) = i;
    
    /* Check if there was DTLB miss */
    ASSERT(dtlb_miss_count == (i + 1));
    ASSERT(dtlb_miss_ea == (RAM_START + RAM_SIZE + (i*DTLB_SETS*PAGE_SIZE) + (set*PAGE_SIZE)));
  }

  /* Reset DTLB miss counter and EA */
  dtlb_miss_count = 0;
  dtlb_miss_ea = 0;

  /* Perform reads to address, that is now in DTLB */
  for (i = 0; i < DTLB_WAYS; i++) {
    ASSERT(REG32(RAM_START + RAM_SIZE + (i*DTLB_SETS*PAGE_SIZE) + (set*PAGE_SIZE)) == i);
    
    /* Check if there was DTLB miss */
    ASSERT(dtlb_miss_count == 0);
  }

  /* Reset valid bits */
  for (i = 0; i < DTLB_WAYS; i++) {
    mtspr (SPR_DTLBMR_BASE(i) + set, mfspr (SPR_DTLBMR_BASE(i) + set) & ~SPR_DTLBMR_V);
  }

  /* Perform reads to address, that is now in DTLB but is invalid */
  for (i = 0; i < DTLB_WAYS; i++) {
    ASSERT(REG32(RAM_START + RAM_SIZE + (i*DTLB_SETS*PAGE_SIZE) + (set*PAGE_SIZE)) == i);
    
    /* Check if there was DTLB miss */
    ASSERT(dtlb_miss_count == (i + 1));
    ASSERT(dtlb_miss_ea == (RAM_START + RAM_SIZE + (i*DTLB_SETS*PAGE_SIZE) + (set*PAGE_SIZE)));
  }

  /* Disable DMMU */
  dmmu_disable();

  printf("OK\n");
  printf("-------------------------------------------\n");

  return 0;
}

/* Permission test
Set various permissions, perform r/w access 
in user and supervisor mode and chack triggering 
of page fault exceptions */
int dtlb_permission_test (int set)
{ 
  int i, j;
  unsigned long ea, ta, tmp;

  printf("dtlb_permission_test, set %d\n", set);

  /* Disable DMMU */
  dmmu_disable();

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
    mtspr (SPR_DTLBMR_BASE(0) + i, ea | SPR_DTLBMR_V);
    mtspr (SPR_DTLBTR_BASE(0) + i, ta | DTLB_PR_NOLIMIT);
  } 

  /* Testing page */
  ea = RAM_START + (RAM_SIZE/2) + (set*PAGE_SIZE);

  /* Set match register */
  mtspr (SPR_DTLBMR_BASE(DTLB_WAYS - 1) + set, ea | SPR_DTLBMR_V);

  /* Reset page fault counter and EA */
  dpage_fault_count = 0;
  dpage_fault_ea = 0;

  /* Enable DMMU */
  dmmu_enable();

  /* Write supervisor */
  dtlb_val = DTLB_PR_NOLIMIT | SPR_DTLBTR_SWE;
  mtspr (SPR_DTLBTR_BASE(DTLB_WAYS - 1) + set, ea | (DTLB_PR_NOLIMIT & ~SPR_DTLBTR_SWE));
  REG32(RAM_START + (RAM_SIZE/2) + (set*PAGE_SIZE) + 0) = 0x00112233;
  ASSERT(dpage_fault_count == 1);
  REG32(RAM_START + (RAM_SIZE/2) + (set*PAGE_SIZE) + 4) = 0x44556677;
  ASSERT(dpage_fault_count == 1);
  REG32(RAM_START + (RAM_SIZE/2) + (set*PAGE_SIZE) + 8) = 0x8899aabb;
  ASSERT(dpage_fault_count == 1);
  REG32(RAM_START + (RAM_SIZE/2) + (set*PAGE_SIZE) + 12) = 0xccddeeff;
  ASSERT(dpage_fault_count == 1);

  /* Read supervisor */
  dtlb_val = DTLB_PR_NOLIMIT | SPR_DTLBTR_SRE;
  mtspr (SPR_DTLBTR_BASE(DTLB_WAYS - 1) + set, ea | (DTLB_PR_NOLIMIT & ~SPR_DTLBTR_SRE));
  tmp = REG32(RAM_START + (RAM_SIZE/2) + (set*PAGE_SIZE) + 0);
  ASSERT(dpage_fault_count == 2);
  ASSERT(tmp == 0x00112233);
  tmp = REG32(RAM_START + (RAM_SIZE/2) + (set*PAGE_SIZE) + 4);
  ASSERT(dpage_fault_count == 2);
  ASSERT(tmp == 0x44556677);
  tmp = REG32(RAM_START + (RAM_SIZE/2) + (set*PAGE_SIZE) + 8);
  ASSERT(dpage_fault_count == 2);
  ASSERT(tmp == 0x8899aabb);
  tmp = REG32(RAM_START + (RAM_SIZE/2) + (set*PAGE_SIZE) + 12);
  ASSERT(dpage_fault_count == 2);
  ASSERT(tmp == 0xccddeeff);

  /* Write user */
  dtlb_val = DTLB_PR_NOLIMIT | SPR_DTLBTR_UWE;
  mtspr (SPR_DTLBTR_BASE(DTLB_WAYS - 1) + set, ea | (DTLB_PR_NOLIMIT & ~SPR_DTLBTR_UWE));

  /* Set user mode */
  mtspr (SPR_SR, mfspr (SPR_SR) & ~SPR_SR_SM);

  REG32(RAM_START + (RAM_SIZE/2) + (set*PAGE_SIZE) + 0) = 0xffeeddcc;
  ASSERT(dpage_fault_count == 3);
  REG32(RAM_START + (RAM_SIZE/2) + (set*PAGE_SIZE) + 4) = 0xbbaa9988;
  ASSERT(dpage_fault_count == 3);
  REG32(RAM_START + (RAM_SIZE/2) + (set*PAGE_SIZE) + 8) = 0x77665544;
  ASSERT(dpage_fault_count == 3);
  REG32(RAM_START + (RAM_SIZE/2) + (set*PAGE_SIZE) + 12) = 0x33221100;
  ASSERT(dpage_fault_count == 3);

  /* Trigger sys call exception to enable supervisor mode again */
  sys_call ();

  /* Read user mode */
  dtlb_val = DTLB_PR_NOLIMIT | SPR_DTLBTR_URE;
  mtspr (SPR_DTLBTR_BASE(DTLB_WAYS - 1) + set, ea | (DTLB_PR_NOLIMIT & ~SPR_DTLBTR_URE));

  /* Set user mode */
  mtspr (SPR_SR, mfspr (SPR_SR) & ~SPR_SR_SM);

  tmp = REG32(RAM_START + (RAM_SIZE/2) + (set*PAGE_SIZE) + 0);
  ASSERT(dpage_fault_count == 4);
  ASSERT(tmp == 0xffeeddcc);
  tmp = REG32(RAM_START + (RAM_SIZE/2) + (set*PAGE_SIZE) + 4);
  ASSERT(dpage_fault_count == 4);
  ASSERT(tmp == 0xbbaa9988);
  tmp = REG32(RAM_START + (RAM_SIZE/2) + (set*PAGE_SIZE) + 8);
  ASSERT(dpage_fault_count == 4);
  ASSERT(tmp == 0x77665544);
  tmp = REG32(RAM_START + (RAM_SIZE/2) + (set*PAGE_SIZE) + 12);
  ASSERT(dpage_fault_count == 4);
  ASSERT(tmp == 0x33221100);

  /* Trigger sys call exception to enable supervisor mode again */
  sys_call ();

  /* Disable DMMU */
  dmmu_disable();

  printf("OK\n");
  printf("-------------------------------------------\n");

  return 0;
}


/* Dcache test - check inhibit
Write data with cache inhibit on and off, check for coherency
 */
int dtlb_dcache_test (int set)
{ 
  int i, j;
  unsigned long ea, ta, vmea;

  // Check data cache is present and enabled
  if (!(mfspr(SPR_UPR)& SPR_UPR_DCP))
    return 0;
  
  if (!(mfspr(SPR_SR) & SPR_SR_DCE))
    return 0;

  printf("dtlb_dcache_test, set %d\n", set);

  /* Disable DMMU */
  dmmu_disable();

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
    mtspr (SPR_DTLBMR_BASE(0) + i, ea | SPR_DTLBMR_V);
    mtspr (SPR_DTLBTR_BASE(0) + i, ta | DTLB_PR_NOLIMIT);
  } 

  /* Testing page */
  ea = RAM_START + (RAM_SIZE/2) + (set*PAGE_SIZE);

  ta = RAM_START + (RAM_SIZE/2) + (set*PAGE_SIZE);

  vmea = VM_BASE + RAM_START + (RAM_SIZE/2) + ((DTLB_SETS-1)*PAGE_SIZE);

  // Set a 1-1 translation for this page without cache inhibited

  /* Set match register */
  mtspr (SPR_DTLBMR_BASE(0) + set, ea | SPR_DTLBMR_V);  
  /* Set translate register */
  mtspr (SPR_DTLBTR_BASE(0) + set, ta | DTLB_PR_NOLIMIT);  

  // Now set a far-off translation, VM_BASE, for this page with cache inhibited
  // Use the last set

  /* Set match register */
  mtspr (SPR_DTLBMR_BASE(0) + (DTLB_SETS-1), 
	 vmea | SPR_DTLBMR_V);
  /* Set translate register */
  mtspr (SPR_DTLBTR_BASE(0) + (DTLB_SETS-1), 
	 ta | DTLB_PR_NOLIMIT | SPR_DTLBTR_CI);
  
  /* Enable DMMU */
  dmmu_enable();
  
  // First do a write with the cache inhibited mapping
  unsigned long int testwrite_to_be_cached = 0xfeca1d0d ^ set;
  REG32((vmea)) = testwrite_to_be_cached;
  // Read it back to check that it's the same, this read should get cached
  ASSERT(REG32(ea) == testwrite_to_be_cached);
  // Now write again to the cache inhibited location
  unsigned long int testwrite_not_to_be_cached = 0xbaadbeef ^ set;
  REG32((vmea)) = testwrite_not_to_be_cached;
  // Now check that the cached mapping doesn't read this value back
  ASSERT(REG32(ea) == testwrite_to_be_cached);
    
  // Now disable cache inhibition on the 1-1 mapping
  /* Set translate register */
  mtspr (SPR_DTLBTR_BASE(0) + set, 
	 ea | DTLB_PR_NOLIMIT | SPR_DTLBTR_CI);

  // Check that we now get the second value we wrote
  testwrite_to_be_cached = testwrite_not_to_be_cached;
  ASSERT(REG32(ea) == testwrite_to_be_cached);
  
  /* Disable DMMU */
  dmmu_disable();

  printf("OK\n");
  printf("-------------------------------------------\n");
  
  return 0;

}

/* Translation address register test
Set various translation and check the pattern */
int itlb_translation_test (void)
{
  int i, j;
  //unsigned long ea, ta;
  unsigned long translate_space;  
  unsigned long match_space;
  printf("itlb_translation_test\n");

  /* Disable IMMU */
  immu_disable();

  /* Invalidate all entries in DTLB */
  for (i = 0; i < ITLB_WAYS; i++) {
    for (j = 0; j < ITLB_SETS; j++) {
      mtspr (SPR_ITLBMR_BASE(i) + j, 0);
      mtspr (SPR_ITLBTR_BASE(i) + j, 0);
    }
  }

  /* Set one to one translation for the use of this program */
  for (i = 0; i < TLB_TEXT_SET_NB; i++) {
	  match_space = /*TEXT_START_ADD +*/ (i*PAGE_SIZE);
	  translate_space = /*TEXT_START_ADD +*/ (i*PAGE_SIZE);
    mtspr (SPR_ITLBMR_BASE(0) + i, match_space | SPR_ITLBMR_V);
    mtspr (SPR_ITLBTR_BASE(0) + i, translate_space | ITLB_PR_NOLIMIT);
  } 

  /* Set itlb permisions */
  itlb_val = ITLB_PR_NOLIMIT;
  
  /* Write test program */
  for (i = TLB_TEXT_SET_NB; i < ITLB_SETS; i++) {
    translate_space = (RAM_START + (RAM_SIZE/2) + (i*PAGE_SIZE) + (i*0x10));
    //printf("writing app to 0x%8lx to be translated in set %d\n",translate_space, i);
    REG32(translate_space) = OR32_L_JR_R9;
    REG32(translate_space + 4) = OR32_L_NOP;
    // Now flush this in case DC isn't on write-through
    mtspr(SPR_DCBFR, translate_space);
    mtspr(SPR_DCBFR, translate_space+4);
  }
   
  /* Set one to one translation of the last ways of ITLB */
  for (i = TLB_TEXT_SET_NB; i < ITLB_SETS; i++) {
    translate_space = (RAM_START + (RAM_SIZE/2) + (i*PAGE_SIZE) + (i*0x10));
    //printf("translate_space: 0x%8lx set %d\n",translate_space, i);
    mtspr (SPR_ITLBMR_BASE(ITLB_WAYS - 1) + i, translate_space | SPR_ITLBMR_V);
    mtspr (SPR_ITLBTR_BASE(ITLB_WAYS - 1) + i, 
	   translate_space | ITLB_PR_NOLIMIT);
  }
  
  /* Enable IMMU */
  immu_enable();

  /* Check the tranlsation works by jumping there */
  for (i = TLB_TEXT_SET_NB; i < ITLB_SETS; i++) {
    translate_space = (RAM_START + (RAM_SIZE/2) + (i*PAGE_SIZE) + (i*0x10));
    //printf("calling 0x%.8lx - should use set %d\n",translate_space, i);
    call (translate_space);
  }

  /* Set hi -> lo, lo -> hi translation */
  for (i = TLB_TEXT_SET_NB; i < ITLB_SETS; i++) {
    match_space = RAM_START + (RAM_SIZE/2) + (i*PAGE_SIZE);
    translate_space = RAM_START + (RAM_SIZE/2) + 
      (((ITLB_SETS-1)+TLB_TEXT_SET_NB - i)*PAGE_SIZE);
    printf("setting itlb set %d match -> trans = 0x%.8lx -> 0x%.8lx\n", 
	   i, match_space, translate_space);
    mtspr (SPR_ITLBMR_BASE(ITLB_WAYS - 1) + i, match_space | SPR_ITLBMR_V);
    mtspr (SPR_ITLBTR_BASE(ITLB_WAYS - 1) + i, translate_space|ITLB_PR_NOLIMIT);
  }

  /* Check the pattern */
  for (i = TLB_TEXT_SET_NB; i < ITLB_SETS; i++) {
    match_space = RAM_START + (RAM_SIZE/2) + (((ITLB_SETS-1)+TLB_TEXT_SET_NB - i)*PAGE_SIZE) + (i*0x10);
    printf("immu hi->lo check - calling 0x%.8lx\n",match_space);
    call(match_space);
  }

  /* Disable IMMU */
  immu_disable ();

  /* Check the pattern */
  for (i = TLB_TEXT_SET_NB; i < ITLB_SETS; i++) {
    translate_space = (RAM_START + (RAM_SIZE/2) + (i*PAGE_SIZE) + (i*0x10));
    //call (RAM_START + (RAM_SIZE/2) + (i*PAGE_SIZE) + (i*0x10));
    //printf("immu disabled check of set %d - calling 0x%.8lx\n",i, translate_space);
    call (translate_space);
  }
  
  printf("OK\n");
  printf("-------------------------------------------\n");

  return 0;
}

/* EA match register test
Shifting one in ITLBMR and performing accesses to boundaries
of the page, checking the triggering of exceptions */
int itlb_match_test (int way, int set)
{
  int i, j;
  unsigned long add;
  unsigned long ea, ta;
  
  printf("itlb_match_test\n");

  /* Disable IMMU */
  immu_disable();

  /* Invalidate all entries in ITLB */
  for (i = 0; i < ITLB_WAYS; i++) {
    for (j = 0; j < ITLB_SETS; j++) {
      mtspr (SPR_ITLBMR_BASE(i) + j, 0);
      mtspr (SPR_ITLBTR_BASE(i) + j, 0);
    }
  }

  /* Set one to one translation for the use of this program */
  for (i = 0; i < TLB_TEXT_SET_NB; i++) {
	  ea = /*TEXT_START_ADD +*/ (i*PAGE_SIZE);
	  ta = /*TEXT_START_ADD +*/ (i*PAGE_SIZE);
    mtspr (SPR_ITLBMR_BASE(0) + i, ea | SPR_ITLBMR_V);
    mtspr (SPR_ITLBTR_BASE(0) + i, ta | ITLB_PR_NOLIMIT);
  } 

  /* Set dtlb permisions */
  itlb_val = ITLB_PR_NOLIMIT;

  
  // Write program which will just return into these places
  unsigned long translate_space = RAM_START + (RAM_SIZE/2) + (set*PAGE_SIZE);
  //copy_jump (RAM_START + (RAM_SIZE/2) + (set*PAGE_SIZE) - 8);
  REG32(translate_space-8) = OR32_L_JR_R9;
  REG32(translate_space-4) = OR32_L_NOP;
  //copy_jump (RAM_START + (RAM_SIZE/2) + (set*PAGE_SIZE));
  REG32(translate_space) = OR32_L_JR_R9;
  REG32(translate_space+4) = OR32_L_NOP;  
  //copy_jump (RAM_START + (RAM_SIZE/2) + (set + 1)*PAGE_SIZE - 8);
  REG32(translate_space + PAGE_SIZE - 8) = OR32_L_JR_R9;
  REG32(translate_space + PAGE_SIZE - 4) = OR32_L_NOP;  
  //copy_jump (RAM_START + (RAM_SIZE/2) + (set + 1)*PAGE_SIZE);
  REG32(translate_space + PAGE_SIZE) = OR32_L_JR_R9;
  REG32(translate_space + PAGE_SIZE + 4) = OR32_L_NOP;  
  // Flush these areas incase cache doesn't write them through immediately
  mtspr(SPR_DCBFR, translate_space-8);
  mtspr(SPR_DCBFR, translate_space);
  mtspr(SPR_DCBFR, translate_space+4);
  mtspr(SPR_DCBFR, translate_space + PAGE_SIZE - 8);
  mtspr(SPR_DCBFR, translate_space + PAGE_SIZE - 4);
  mtspr(SPR_DCBFR, translate_space + PAGE_SIZE);
  mtspr(SPR_DCBFR, translate_space + PAGE_SIZE + 4);
  
  /* Enable IMMU */
  immu_enable();

  /* Shifting one in ITLBMR */ 
  i = 0;
  add = (PAGE_SIZE*ITLB_SETS);
  //t_add = add + (set*PAGE_SIZE);
  // Space we'll access and expect the MMU to translate our requests
  unsigned long match_space = (PAGE_SIZE*DTLB_SETS) + (set*PAGE_SIZE); 
  while (add != 0x00000000) { 
    mtspr (SPR_ITLBMR_BASE(way) + set,  match_space | SPR_ITLBMR_V);
    mtspr (SPR_ITLBTR_BASE(way) + set,  translate_space | ITLB_PR_NOLIMIT);

    /* Reset ITLB miss counter and EA */
    itlb_miss_count = 0;
    itlb_miss_ea = 0;

    //if (((t_add < RAM_START) || (t_add >= DATA_END_ADD)) && ((t_add < TEXT_START_ADD) || (t_add >= TEXT_END_ADD))) {

      /* Jump on last address of previous page */
      call (match_space - 8);
      ASSERT(itlb_miss_count == 1);
      
      /* Jump on first address of the page */
      call (match_space);
      ASSERT(itlb_miss_count == 1);
 
      /* Jump on last address of the page */
      call (match_space + PAGE_SIZE - 8);
      ASSERT(itlb_miss_count == 1);

      /* Jump on first address of next page */
      call (match_space + PAGE_SIZE);
      ASSERT(itlb_miss_count == 2);
      //}

    i++;
    add = (PAGE_SIZE*ITLB_SETS) << i;
    match_space = add + (set*PAGE_SIZE);

    for (j = 0; j < ITLB_WAYS; j++) {
      mtspr (SPR_ITLBMR_BASE(j) + ((set - 1) & (ITLB_SETS - 1)), 0);
      mtspr (SPR_ITLBMR_BASE(j) + ((set + 1) & (ITLB_SETS - 1)), 0);
    }
  }

  printf("OK\n");
  printf("-------------------------------------------\n");

  /* Disable IMMU */
  immu_disable();

  return 0;
}

/* Valid bit test
Set all ways of one set to be invalid, perform
access so miss handler will set them to valid, 
try access again - there should be no miss exceptions */
int itlb_valid_bit_test (int set)
{
  int i, j;
  unsigned long ea, ta;

  printf("itlb_valid_bit_test set = %d\n", set);

  /* Disable IMMU */
  immu_disable();

  /* Invalidate all entries in ITLB */
  for (i = 0; i < ITLB_WAYS; i++) {
    for (j = 0; j < ITLB_SETS; j++) {
      mtspr (SPR_ITLBMR_BASE(i) + j, 0);
      mtspr (SPR_ITLBTR_BASE(i) + j, 0);
    }
  }

  /* Set one to one translation for the use of this program */
  for (i = 0; i < TLB_TEXT_SET_NB; i++) {
	  ea = /*TEXT_START_ADD +*/ (i*PAGE_SIZE);
	  ta = /*TEXT_START_ADD +*/ (i*PAGE_SIZE);
    mtspr (SPR_ITLBMR_BASE(0) + i, ea | SPR_ITLBMR_V);
    mtspr (SPR_ITLBTR_BASE(0) + i, ta | ITLB_PR_NOLIMIT);
  } 

  /* Reset ITLB miss counter and EA */
  itlb_miss_count = 0;
  itlb_miss_ea = 0;
 
  /* Set itlb permisions */
  itlb_val = ITLB_PR_NOLIMIT;

  /* Resetv ITLBMR for every way after the program code */
  for (i = TLB_TEXT_SET_NB; i < ITLB_WAYS; i++) {
    mtspr (SPR_ITLBMR_BASE(i) + set, 0);
  }

  /* Enable IMMU */
  immu_enable();
  // Address we'll jump to and expect it to be translated
  unsigned long match_space = (PAGE_SIZE*ITLB_SETS) + (set*PAGE_SIZE);
  // Address that we will actually access
  unsigned long translate_space = RAM_START + (RAM_SIZE/2) + (set*PAGE_SIZE);
  /* Perform jumps to address, that is not in ITLB */
  printf("writing program to 0x%.8lx\n", translate_space);
  REG32(translate_space) = OR32_L_JR_R9;
  REG32(translate_space+4) = OR32_L_NOP;
  mtspr(SPR_DCBFR, translate_space);
  mtspr(SPR_DCBFR, translate_space+4);
  printf("jumping to 0x%.8lx, should be itlb miss\n",match_space);
  call (match_space);
  
  /* Check if there was ITLB miss */
  ASSERT(itlb_miss_count == 1);
  ASSERT(itlb_miss_ea == match_space);

  /* Reset ITLB miss counter and EA */
  itlb_miss_count = 0;
  itlb_miss_ea = 0;
  printf("jumping to 0x%.8lx again - should not be a miss\n", match_space);
  /* Perform jumps to address, that is now in ITLB */
  call (match_space);
    
  /* Check if there was ITLB miss */
  ASSERT(itlb_miss_count == 0);

  /* Reset valid bits */
  for (i = 0; i < ITLB_WAYS; i++) {
    mtspr (SPR_ITLBMR_BASE(i) + set, mfspr (SPR_ITLBMR_BASE(i) + set) & ~SPR_ITLBMR_V);
  }
  printf("jumping to 0x%.8lx again - mmu entries invalidated, so should be a miss\n", match_space);
  /* Perform jumps to address, that is now in ITLB but is invalid */
  call (match_space);
  
  /* Check if there was ITLB miss */
  ASSERT(itlb_miss_count == 1);
  ASSERT(itlb_miss_ea == match_space);

  /* Disable IMMU */
  immu_disable ();

  printf("OK\n");
  printf("-------------------------------------------\n");

  return 0;
}

/* Permission test
Set various permissions, perform r/w access 
in user and supervisor mode and chack triggering 
of page fault exceptions */
int itlb_permission_test (int set)
{ 
  int i, j;
  unsigned long ea, ta;

  printf("itlb_permission_test set = %d\n", set);

  /* Disable IMMU */
  immu_disable();

  /* Invalidate all entries in ITLB */
  for (i = 0; i < ITLB_WAYS; i++) {
    for (j = 0; j < ITLB_SETS; j++) {
      mtspr (SPR_ITLBMR_BASE(i) + j, 0);
      mtspr (SPR_ITLBTR_BASE(i) + j, 0);
    }
  }

  /* Set one to one translation for the use of this program */
  for (i = 0; i < TLB_TEXT_SET_NB; i++) {
	  ea = /*TEXT_START_ADD +*/ (i*PAGE_SIZE);
	  ta = /*TEXT_START_ADD +*/ (i*PAGE_SIZE);
    mtspr (SPR_ITLBMR_BASE(0) + i, ea | SPR_ITLBMR_V);
    mtspr (SPR_ITLBTR_BASE(0) + i, ta | ITLB_PR_NOLIMIT);
  } 

  // Address that we will actually access
  unsigned long match_space = RAM_START + (RAM_SIZE/2) + (set*PAGE_SIZE);
  
  /* Set match register */
  mtspr (SPR_ITLBMR_BASE(ITLB_WAYS - 1) + set, match_space | SPR_ITLBMR_V);

  /* Reset page fault counter and EA */
  ipage_fault_count = 0;
  ipage_fault_ea = 0;

  /* Copy the code */
  REG32(match_space+0x0) = OR32_L_JR_R9;
  REG32(match_space+0x4) = OR32_L_NOP;
  REG32(match_space+0x8) = OR32_L_JR_R9;
  REG32(match_space+0xc) = OR32_L_NOP;
  mtspr(SPR_DCBFR, match_space);
  mtspr(SPR_DCBFR, match_space+4);
  mtspr(SPR_DCBFR, match_space+8);
  mtspr(SPR_DCBFR, match_space+12);

  /* Enable IMMU */
  immu_enable ();

  /* Execute supervisor */
  printf("execute disable for supervisor - should cause ipage fault\n");
  itlb_val = SPR_ITLBTR_CI | SPR_ITLBTR_SXE;
  mtspr (SPR_ITLBTR_BASE(ITLB_WAYS - 1) + set, match_space | (ITLB_PR_NOLIMIT & ~SPR_ITLBTR_SXE));
  printf("calling address 0x%.8lx\n", match_space);
  call (match_space);
  ASSERT(ipage_fault_count == 1);
  call (match_space + 8);
  ASSERT(ipage_fault_count == 1); 

  /* Execute user */
  printf("execute disable for user - should cause ipage fault\n");  
  itlb_val = SPR_ITLBTR_CI | SPR_ITLBTR_UXE;
  mtspr (SPR_ITLBTR_BASE(ITLB_WAYS - 1) + set, match_space | (ITLB_PR_NOLIMIT & ~SPR_ITLBTR_UXE));

  /* Set user mode */
  printf("disabling supervisor mode\n");
  mtspr (SPR_SR, mfspr (SPR_SR) & ~SPR_SR_SM);
  printf("calling address 0x%.8lx\n", match_space);
  printf("instruction at jump space: 0x%.8lx\n",REG32(match_space));
  call (match_space);
  ASSERT(ipage_fault_count == 2);
  call (match_space + 8);
  ASSERT(ipage_fault_count == 2);

  /* Trigger sys call exception to enable supervisor mode again */
  sys_call ();

  /* Disable IMMU */
  immu_disable ();

  printf("OK\n");
  printf("-------------------------------------------\n");

  return 0;
}

int main (void)
{
  int i, j;

  text_begin_addr = (unsigned long)&_src_beg;
  end_data_addr = (unsigned long)&_ram_end;

  TLB_TEXT_SET_NB =  TLB_DATA_SET_NB = (end_data_addr+PAGE_SIZE) / PAGE_SIZE;

  // Some tests check page below, and if this page corresponds with set being
  // used by program's data section, then could mean an incorrect mapping ends
  // up in the TLB cache, which causes the program to do bad things, so to be
  // safe incrementing the number of pages being used by the program.
  TLB_TEXT_SET_NB++;
  TLB_DATA_SET_NB++;

  i = j = 0; /* Get rid of warnings */

  /* Register bus error handler */
  excpt_buserr = (unsigned long)bus_err_handler;

  /* Register illegal insn handler */
  excpt_illinsn = (unsigned long)ill_insn_handler;

  /* Register illegal insn handler */
  excpt_syscall = (unsigned long)sys_call_handler;

#if DO_DMMU_TESTS==1

  /* Translation test */
  dtlb_translation_test ();
  
  printf("DTLB translation tests OK\n");
  
  /* Virtual address match test */
  for (j = 0; j < DTLB_WAYS; j++) {
#ifdef SHORT_TEST
    for (i = TLB_DATA_SET_NB; i < (TLB_DATA_SET_NB+4 - 1); i++)
#else
    for (i = TLB_DATA_SET_NB; i < (DTLB_SETS - 1); i++)
#endif
      dtlb_match_test (j, i);
  }
  printf("DTLB match tests OK\n");

  /* Valid bit testing */

#ifdef SHORT_TEST
  for (i = TLB_DATA_SET_NB; i < (TLB_DATA_SET_NB+4 - 1); i++)
#else
  for (i = TLB_DATA_SET_NB; i < (DTLB_SETS - 1); i++)
#endif
      dtlb_valid_bit_test (i);

  printf("DTLB valid bit tests OK\n");

  /* Permission test */
#ifdef SHORT_TEST
  for (i = TLB_DATA_SET_NB; i < (TLB_DATA_SET_NB+4 - 1); i++)
#else
  for (i = TLB_DATA_SET_NB; i < (DTLB_SETS - 1); i++)
#endif
    dtlb_permission_test (i);

  printf("DTLB permission bit tests OK\n");

  /* Data cache test - Disabled for now */

#if 0 
#ifdef SHORT_TEST
  for (i = TLB_DATA_SET_NB; i < (TLB_DATA_SET_NB+4 - 1); i++)
#else
  for (i = TLB_DATA_SET_NB; i < (DTLB_SETS - 2); i++)
#endif
    dtlb_dcache_test (i);
#endif

#endif /* end of no DTLB tests */


  /* Translation test */
  itlb_translation_test ();
  
  printf("ITLB translation tests OK\n");

  /* Virtual address match test */

  for (j = 0; j < DTLB_WAYS; j++) {
#ifdef SHORT_TEST
  for (i = TLB_DATA_SET_NB + 1; i < (TLB_DATA_SET_NB+4 - 1); i++)
#else
  for (i = TLB_DATA_SET_NB + 1; i < (ITLB_SETS - 1); i++)
#endif
      itlb_match_test (j, i);
  }

  printf("ITLB match tests OK\n");

  /* Valid bit testing */
#ifdef SHORT_TEST
  for (i = TLB_DATA_SET_NB; i < (TLB_DATA_SET_NB+4 - 1); i++)
#else
  for (i = TLB_DATA_SET_NB; i < (ITLB_SETS - 1); i++)
#endif
    itlb_valid_bit_test (i);

  printf("ITLB valid bit tests OK\n");


  /* Permission test */
#ifdef SHORT_TEST
  for (i = TLB_TEXT_SET_NB; i < (TLB_TEXT_SET_NB + 4); i++)
#else
  for (i = TLB_TEXT_SET_NB; i < (ITLB_SETS - 1); i++)
#endif
    itlb_permission_test (i);

  printf("ITLB permission tests OK\n");

  printf("Tests completed\n");
  report (0xdeaddead);
  report (0x8000000d);
  exit (0);
}
