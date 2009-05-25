/* This is MMU test for OpenRISC 1200 */

#include "spr_defs.h"
#include "support.h"

/* For shorter simulation run */
#define RTL_SIM 1

/* Define RAM physical location and size 
   Bottom half will be used for this program, the rest 
   will be used for testing */
#define FLASH_START 0xf0000000
#define FLASH_SIZE  0x00200000
#define RAM_START   0x00000000
#define RAM_SIZE    0x00200000

/* What is the last address in ram that is used by this program */
#define TEXT_END_ADD (FLASH_START + (FLASH_SIZE / 2))
#define DATA_END_ADD (RAM_START + (RAM_SIZE / 2))

#define TLB_TEXT_SET_NB 8
#define TLB_DATA_SET_NB 4

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

#if 1
#define debug printf
#else
#define debug
#endif

/* fails if x is false */
#define ASSERT(x) ((x)?1: fail (__FUNCTION__, __LINE__))

//#define TEST_JUMP(x) testjump( ((x) & (RAM_SIZE/2 - 1)) + DATA_END_ADD, (x))
#define TEST_JUMP(x) copy_jump (((x) & (RAM_SIZE/2 - 1)) + DATA_END_ADD); call (x)

/* Extern functions */
extern void lo_dmmu_en (void);
extern void lo_immu_en (void);
extern int  lo_dtlb_ci_test (unsigned long, unsigned long);
extern int  lo_itlb_ci_test(unsigned long, unsigned long);
extern void testjump(unsigned long phy_addr, unsigned long virt_addr);
extern void (*jr)(void);

/* Local functions prototypes */
void dmmu_disable (void);
void immu_disable (void);

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

void sys_call (void)
{
  asm("l.sys\t0");
}

void fail (char *func, int line)
{
#ifndef __FUNCTION__
#define __FUNCTION__ "?"
#endif

  /* Trigger sys call exception to enable supervisor mode again */
  sys_call ();

  immu_disable ();
  dmmu_disable ();

  debug("Test failed in %s:%i\n", func, line);
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
	asm("_jr:");
	asm("l.jr\t\tr9") ;
	asm("l.nop" : :);
}

void copy_jump(unsigned long phy_add)
{
	memcpy((void *)phy_add, (void *)&jr, 8);
}

/* Bus error exception handler */
void bus_err_handler (void)
{
  /* This shouldn't happend */
  debug("Test failed: Bus error\n");
  report (0xeeeeeeee);
  exit (1);
}

/* Illegal insn exception handler */
void ill_insn_handler (void)
{
  /* This shouldn't happend */
  debug("Test failed: Illegal insn\n");
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

  debug("ea = %.8lx set = %d way = %d\n", ea, set, way);

  if (((RAM_START <= ea) && (ea < DATA_END_ADD) ) || ((FLASH_START <= ea) && (ea < TEXT_END_ADD))) {
    /* If this is acces to data of this program set one to one translation */
    mtspr (SPR_DTLBMR_BASE(way) + set, (ea & SPR_DTLBMR_VPN) | SPR_DTLBMR_V);
    mtspr (SPR_DTLBTR_BASE(way) + set, (ea & SPR_DTLBTR_PPN) | DTLB_PR_NOLIMIT);
    return;
  }

  /* Update DTLB miss counter and EA */
  dtlb_miss_count++;
  dtlb_miss_ea = ea;

  /* Whatever access is in progress, translated address have to point to physical RAM */
  ta = (ea & ((RAM_SIZE/2) - 1)) + RAM_START + (RAM_SIZE/2);
  tlbtr = (ta & SPR_DTLBTR_PPN) | (dtlb_val & TLB_PR_MASK);
  debug("tlbtr = %.8lx dtlb_val = %.8lx\n", tlbtr, dtlb_val);

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
    if ((mfspr (SPR_DTLBMR_BASE(i) + set) & SPR_DTLBMR_VPN) == (ea & SPR_DTLBMR_VPN)) {
      way = i;
      break;
    }
  }

  debug("ea = %.8lx set = %d way = %d\n", ea, set, way);

  if (((RAM_START <= ea) && (ea < DATA_END_ADD) ) || ((FLASH_START <= ea) && (ea < TEXT_END_ADD))) {
    /* If this is acces to data of this program set one to one translation */
    mtspr (SPR_DTLBTR_BASE(way) + set, (ea & SPR_DTLBTR_PPN) | DTLB_PR_NOLIMIT);
    return;
  }

  /* Update data page fault counter and EA */
  dpage_fault_count++;
  dpage_fault_ea = ea;

  /* Give permission */
  mtspr (SPR_DTLBTR_BASE(way) + set, (mfspr (SPR_DTLBTR_BASE(way) + set) & ~DTLB_PR_NOLIMIT) | dtlb_val);
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

  debug("ea = %.8lx set = %d way = %d\n", ea, set, way);

  if ((FLASH_START <= ea) && (ea < TEXT_END_ADD)) {
    /* If this is acces to data of this program set one to one translation */
    mtspr (SPR_ITLBMR_BASE(way) + set, (ea & SPR_ITLBMR_VPN) | SPR_ITLBMR_V);
    mtspr (SPR_ITLBTR_BASE(way) + set, (ea & SPR_ITLBTR_PPN) | ITLB_PR_NOLIMIT);
    return;
  }

  /* Update ITLB miss counter and EA */
  itlb_miss_count++;
  itlb_miss_ea = ea;

  /* Whatever access is in progress, translated address have to point to physical RAM */
  ta = (ea & ((RAM_SIZE/2) - 1)) + DATA_END_ADD;
  tlbtr = (ta & SPR_ITLBTR_PPN) | (itlb_val & TLB_PR_MASK);

  debug("ta = %.8lx\n", ta);

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

  debug("ea = %.8lx set = %d way = %d\n", ea, set, way);

  if ((FLASH_START <= ea) && (ea < TEXT_END_ADD)) {
    /* If this is acces to data of this program set one to one translation */
    mtspr (SPR_DTLBTR_BASE(way) + set, (ea & SPR_DTLBTR_PPN) | ITLB_PR_NOLIMIT);
    return;
  }

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
  excpt_dtlbmiss = (unsigned long)dtlb_miss_handler;

  /* Register data page fault handler */
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
  excpt_itlbmiss = (unsigned long)itlb_miss_handler;

  /* Register instruction page fault handler */
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

  /* Set dtlb permisions */
  dtlb_val = DTLB_PR_NOLIMIT;

  /* Write test pattern */
  for (i = 0; i < DTLB_SETS; i++) {
    REG32(RAM_START + (RAM_SIZE/2) + (i*PAGE_SIZE)) = i;
    REG32(RAM_START + (RAM_SIZE/2) + ((i + 1)*PAGE_SIZE) - 4) = 0xffffffff - i;
  }
   
  /* Set one to one translation of the last way of DTLB */
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
    REG32(RAM_START + (RAM_SIZE/2) + (i*PAGE_SIZE)) = 0xffffffff - i;
    REG32(RAM_START + (RAM_SIZE/2) + ((i + 1)*PAGE_SIZE) - 4) = i;
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
    ta = RAM_START + (RAM_SIZE/2) + ((DTLB_SETS - i - 1 + TLB_DATA_SET_NB)*PAGE_SIZE);
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

  return 0;
}

/* EA match register test
   Shifting one in DTLBMR and performing accesses to boundaries
   of the page, checking the triggering of exceptions */
int dtlb_match_test (int way, int set)
{
  int i, j, tmp;
  unsigned long add, t_add;
  unsigned long ea, ta;

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

  /* Set dtlb permisions */
  dtlb_val = DTLB_PR_NOLIMIT;

  /* Set pattern */
  REG32(RAM_START + (RAM_SIZE/2) + (set*PAGE_SIZE) - 4) = 0x00112233;
  REG32(RAM_START + (RAM_SIZE/2) + (set*PAGE_SIZE)) = 0x44556677;
  REG32(RAM_START + (RAM_SIZE/2) + (set + 1)*PAGE_SIZE - 4) = 0x8899aabb;
  REG32(RAM_START + (RAM_SIZE/2) + (set + 1)*PAGE_SIZE) = 0xccddeeff;
  
  /* Enable DMMU */
  dmmu_enable();

  /* Shifting one in DTLBMR */ 
  i = 0;
  add = (PAGE_SIZE*DTLB_SETS);
  t_add = add + (set*PAGE_SIZE);
  while (add != 0x00000000) { 
    mtspr (SPR_DTLBMR_BASE(way) + set, t_add | SPR_DTLBMR_V);
    mtspr (SPR_DTLBTR_BASE(way) + set, (RAM_START + (RAM_SIZE/2) + (set*PAGE_SIZE)) | DTLB_PR_NOLIMIT);

    /* Reset DTLB miss counter and EA */
    dtlb_miss_count = 0;
    dtlb_miss_ea = 0;

    if (((t_add < RAM_START) || (t_add >= DATA_END_ADD)) && ((t_add < FLASH_START) || (t_add >= TEXT_END_ADD))) {

      /* Read last address of previous page */
      tmp = REG32(t_add - 4);
      ASSERT(tmp == 0x00112233);
      ASSERT(dtlb_miss_count == 1);
      
      /* Read first address of the page */
      tmp = REG32(t_add);
      ASSERT(tmp == 0x44556677);
      ASSERT(dtlb_miss_count == 1);
 
      /* Read last address of the page */
      tmp = REG32(t_add + PAGE_SIZE - 4);
      ASSERT(tmp == 0x8899aabb);
      ASSERT(dtlb_miss_count == 1);

      /* Read first address of next page */
      tmp = REG32(t_add + PAGE_SIZE);
      ASSERT(tmp == 0xccddeeff);
      ASSERT(dtlb_miss_count == 2);
    }

    i++;
    add = (PAGE_SIZE*DTLB_SETS) << i;
    t_add = add + (set*PAGE_SIZE);

    for (j = 0; j < DTLB_WAYS; j++) {
      mtspr (SPR_DTLBMR_BASE(j) + ((set - 1) & (DTLB_SETS - 1)), 0);
      mtspr (SPR_DTLBMR_BASE(j) + ((set + 1) & (DTLB_SETS - 1)), 0);
    }
  }

  /* Disable DMMU */
  dmmu_disable();

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

  return 0;
}

/* Permission test
   Set various permissions, perform r/w access 
   in user and supervisor mode and chack triggering 
   of page fault exceptions */
int dtlb_premission_test (int set)
{ 
  int i, j;
  unsigned long ea, ta, tmp;

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

  return 0;
}

/* Data cache inhibit bit test
   Set and clear CI bit and check the pattern. */
int dtlb_ci_test (void)
{
  int i, j;
  unsigned long ea, ta, ret;
 
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
    mtspr (SPR_DTLBTR_BASE(0) + i, ta | DTLB_PR_NOLIMIT  | SPR_DTLBTR_CI);
  }
 
  /* Testing page */
  ea = RAM_START + (RAM_SIZE/2) + (TLB_DATA_SET_NB*PAGE_SIZE);
  ta = RAM_START + (RAM_SIZE/2) + (TLB_DATA_SET_NB*PAGE_SIZE);
 
  /* Write test pattern */
  REG32(ea) = 0x01234567;
  REG32(ea + PAGE_SIZE - 4) = 0x9abcdef;
 
  /* Set one to one translation with CI bit for testing area */
  mtspr (SPR_DTLBMR_BASE(0) + TLB_DATA_SET_NB, ea | SPR_DTLBMR_V);
  mtspr (SPR_DTLBTR_BASE(0) + TLB_DATA_SET_NB, ta | DTLB_PR_NOLIMIT | SPR_DTLBTR_CI);
 
  ret = lo_dtlb_ci_test(ea, TLB_DATA_SET_NB);
  ASSERT(ret == 0);
 
  return 0;
}

/* Translation address register test
   Set various translation and check the pattern */
int itlb_translation_test (void)
{
  int i, j;
  unsigned long ea, ta;

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
    ea = FLASH_START + (i*PAGE_SIZE);
    ta = FLASH_START + (i*PAGE_SIZE);
    mtspr (SPR_ITLBMR_BASE(0) + i, ea | SPR_ITLBMR_V);
    mtspr (SPR_ITLBTR_BASE(0) + i, ta | ITLB_PR_NOLIMIT);
  } 

  /* Set itlb permisions */
  itlb_val = ITLB_PR_NOLIMIT;

  /* Write test program */
  for (i = TLB_TEXT_SET_NB; i < ITLB_SETS; i++) {
    copy_jump (RAM_START + (RAM_SIZE/2) + (i*PAGE_SIZE) + (i*0x10));
  }
   
  /* Set one to one translation of the last way of ITLB */
  for (i = TLB_TEXT_SET_NB; i < ITLB_SETS; i++) {
    ea = RAM_START + (RAM_SIZE/2) + (i*PAGE_SIZE);
    ta = RAM_START + (RAM_SIZE/2) + (i*PAGE_SIZE);
    mtspr (SPR_ITLBMR_BASE(ITLB_WAYS - 1) + i, ea | SPR_ITLBMR_V);
    mtspr (SPR_ITLBTR_BASE(ITLB_WAYS - 1) + i, ta | ITLB_PR_NOLIMIT);
  }
  
  /* Enable IMMU */
  immu_enable();

  /* Check the pattern */
  for (i = TLB_TEXT_SET_NB; i < ITLB_SETS; i++) {
    call (RAM_START + (RAM_SIZE/2) + (i*PAGE_SIZE) + (i*0x10));
  }

  /* Set FLASH_END -> RAM_START + (RAM_SIZE/2) translation */
  for (i = TLB_TEXT_SET_NB; i < ITLB_SETS; i++) {
    ea = FLASH_START + FLASH_SIZE + i*PAGE_SIZE;
    ta = RAM_START + (RAM_SIZE/2) + (i*PAGE_SIZE);
    mtspr (SPR_ITLBMR_BASE(ITLB_WAYS - 1) + i, ea | SPR_ITLBMR_V);
    mtspr (SPR_ITLBTR_BASE(ITLB_WAYS - 1) + i, ta | ITLB_PR_NOLIMIT);
  }

  /* Check the pattern */
  for (i = TLB_TEXT_SET_NB; i < ITLB_SETS; i++) {
    call (FLASH_START + FLASH_SIZE + (i*PAGE_SIZE) + (i*0x10));
  }

  /* Set hi -> lo, lo -> hi translation */
  for (i = TLB_TEXT_SET_NB; i < ITLB_SETS; i++) {
    ea = RAM_START + (RAM_SIZE/2) + (i*PAGE_SIZE);
    ta = RAM_START + (RAM_SIZE/2) + ((ITLB_SETS - i - 1 + TLB_TEXT_SET_NB)*PAGE_SIZE);
    mtspr (SPR_ITLBMR_BASE(ITLB_WAYS - 1) + i, ea | SPR_ITLBMR_V);
    mtspr (SPR_ITLBTR_BASE(ITLB_WAYS - 1) + i, ta | ITLB_PR_NOLIMIT);
  }

  /* Check the pattern */
  for (i = TLB_TEXT_SET_NB; i < ITLB_SETS; i++) {
    call (RAM_START + (RAM_SIZE/2) + (i*PAGE_SIZE) + ((ITLB_SETS - i - 1 + TLB_TEXT_SET_NB)*0x10));
  }

  /* Disable IMMU */
  immu_disable ();

  /* Check the pattern */
  for (i = TLB_TEXT_SET_NB; i < ITLB_SETS; i++) {
    call (RAM_START + (RAM_SIZE/2) + (i*PAGE_SIZE) + (i*0x10));
  }

  return 0;
}

/* EA match register test
   Shifting one in ITLBMR and performing accesses to boundaries
   of the page, checking the triggering of exceptions */
int itlb_match_test (int way, int set)
{
  int i, j;
  unsigned long add, t_add;
  unsigned long ea, ta;

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
    ea = FLASH_START + (i*PAGE_SIZE);
    ta = FLASH_START + (i*PAGE_SIZE);
    mtspr (SPR_ITLBMR_BASE(0) + i, ea | SPR_ITLBMR_V);
    mtspr (SPR_ITLBTR_BASE(0) + i, ta | ITLB_PR_NOLIMIT);
  } 

  /* Set dtlb permisions */
  itlb_val = ITLB_PR_NOLIMIT;

  /* Set pattern */
  copy_jump (RAM_START + (RAM_SIZE/2) + (set*PAGE_SIZE) - 8);
  copy_jump (RAM_START + (RAM_SIZE/2) + (set*PAGE_SIZE));
  copy_jump (RAM_START + (RAM_SIZE/2) + (set + 1)*PAGE_SIZE - 8);
  copy_jump (RAM_START + (RAM_SIZE/2) + (set + 1)*PAGE_SIZE);
  
  /* Enable IMMU */
  immu_enable();

  /* Shifting one in ITLBMR */ 
  i = 0;
  add = (PAGE_SIZE*ITLB_SETS);
  t_add = add + (set*PAGE_SIZE);
  while (add != 0x00000000) { 
    mtspr (SPR_ITLBMR_BASE(way) + set, t_add | SPR_ITLBMR_V);
    mtspr (SPR_ITLBTR_BASE(way) + set, (RAM_START + (RAM_SIZE/2) + (set*PAGE_SIZE)) | ITLB_PR_NOLIMIT);

    /* Reset ITLB miss counter and EA */
    itlb_miss_count = 0;
    itlb_miss_ea = 0;

    if (((t_add < RAM_START) || (t_add >= DATA_END_ADD)) && ((t_add < FLASH_START) || (t_add >= TEXT_END_ADD))) {

      /* Jump on last address of previous page */
      call (t_add - 8);
      ASSERT(itlb_miss_count == 1);
      
      /* Jump on first address of the page */
      call (t_add);
      ASSERT(itlb_miss_count == 1);
 
      /* Jump on last address of the page */
      call (t_add + PAGE_SIZE - 8);
      ASSERT(itlb_miss_count == 1);

      /* Jump on first address of next page */
      call (t_add + PAGE_SIZE);
      ASSERT(itlb_miss_count == 2);
    }

    i++;
    add = (PAGE_SIZE*ITLB_SETS) << i;
    t_add = add + (set*PAGE_SIZE);

    for (j = 0; j < ITLB_WAYS; j++) {
      mtspr (SPR_ITLBMR_BASE(j) + ((set - 1) & (ITLB_SETS - 1)), 0);
      mtspr (SPR_ITLBMR_BASE(j) + ((set + 1) & (ITLB_SETS - 1)), 0);
    }
  }

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
    ea = FLASH_START + (i*PAGE_SIZE);
    ta = FLASH_START + (i*PAGE_SIZE);
    mtspr (SPR_ITLBMR_BASE(0) + i, ea | SPR_ITLBMR_V);
    mtspr (SPR_ITLBTR_BASE(0) + i, ta | ITLB_PR_NOLIMIT);
  } 

  /* Reset ITLB miss counter and EA */
  itlb_miss_count = 0;
  itlb_miss_ea = 0;
 
  /* Set itlb permisions */
  itlb_val = ITLB_PR_NOLIMIT;

  /* Resetv ITLBMR for every way */
  for (i = 0; i < ITLB_WAYS; i++) {
    mtspr (SPR_ITLBMR_BASE(i) + set, 0);
  }

  /* Enable IMMU */
  immu_enable();

  /* Perform jumps to address, that is not in ITLB */
  for (i = 0; i < ITLB_WAYS; i++) {
    TEST_JUMP(RAM_START + RAM_SIZE + (i*ITLB_SETS*PAGE_SIZE) + (set*PAGE_SIZE));
    
    /* Check if there was ITLB miss */
    ASSERT(itlb_miss_count == (i + 1));
    ASSERT(itlb_miss_ea == (RAM_START + RAM_SIZE + (i*ITLB_SETS*PAGE_SIZE) + (set*PAGE_SIZE)));
  }

  /* Reset ITLB miss counter and EA */
  itlb_miss_count = 0;
  itlb_miss_ea = 0;

  /* Perform jumps to address, that is now in ITLB */
  for (i = 0; i < ITLB_WAYS; i++) {
    TEST_JUMP(RAM_START + RAM_SIZE + (i*ITLB_SETS*PAGE_SIZE) + (set*PAGE_SIZE));
    
    /* Check if there was ITLB miss */
    ASSERT(itlb_miss_count == 0);
  }

  /* Reset valid bits */
  for (i = 0; i < ITLB_WAYS; i++) {
    mtspr (SPR_ITLBMR_BASE(i) + set, mfspr (SPR_ITLBMR_BASE(i) + set) & ~SPR_ITLBMR_V);
  }

  /* Perform jumps to address, that is now in ITLB but is invalid */
  for (i = 0; i < ITLB_WAYS; i++) {
    TEST_JUMP(RAM_START + RAM_SIZE + (i*ITLB_SETS*PAGE_SIZE) + (set*PAGE_SIZE));
    
    /* Check if there was ITLB miss */
    ASSERT(itlb_miss_count == (i + 1));
    ASSERT(itlb_miss_ea == (RAM_START + RAM_SIZE + (i*ITLB_SETS*PAGE_SIZE) + (set*PAGE_SIZE)));
  }

  /* Disable IMMU */
  immu_disable ();

  return 0;
}

/* Permission test
   Set various permissions, perform r/w access 
   in user and supervisor mode and check triggering 
   of page fault exceptions */
int itlb_premission_test (int set)
{ 
  int i, j;
  unsigned long ea, ta;

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
    ea = FLASH_START + (i*PAGE_SIZE);
    ta = FLASH_START + (i*PAGE_SIZE);
    mtspr (SPR_ITLBMR_BASE(0) + i, ea | SPR_ITLBMR_V);
    mtspr (SPR_ITLBTR_BASE(0) + i, ta | ITLB_PR_NOLIMIT);
  } 

  /* Testing page */
  ea = RAM_START + (RAM_SIZE/2) + (set*PAGE_SIZE);

  /* Set match register */
  mtspr (SPR_ITLBMR_BASE(ITLB_WAYS - 1) + set, ea | SPR_ITLBMR_V);

  /* Reset page fault counter and EA */
  ipage_fault_count = 0;
  ipage_fault_ea = 0;

  /* Copy the code */
  copy_jump (ea);
  copy_jump (ea + 8);

  /* Enable IMMU */
  immu_enable ();

  /* Execute supervisor */
  itlb_val = SPR_ITLBTR_CI | SPR_ITLBTR_SXE;
  mtspr (SPR_ITLBTR_BASE(ITLB_WAYS - 1) + set, ea | (ITLB_PR_NOLIMIT & ~SPR_ITLBTR_SXE));
  
  call (ea);
  ASSERT(ipage_fault_count == 1);
  call (ea + 8);
  ASSERT(ipage_fault_count == 1); 

  /* Execute user */
  itlb_val = SPR_ITLBTR_CI | SPR_ITLBTR_UXE;
  mtspr (SPR_ITLBTR_BASE(ITLB_WAYS - 1) + set, ea | (ITLB_PR_NOLIMIT & ~SPR_ITLBTR_UXE));

  /* Set user mode */
  mtspr (SPR_SR, mfspr (SPR_SR) & ~SPR_SR_SM);

  call (ea);
  ASSERT(ipage_fault_count == 2);
  call (ea + 8);
  ASSERT(ipage_fault_count == 2);

  /* Trigger sys call exception to enable supervisor mode again */
  sys_call ();

  /* Disable IMMU */
  immu_disable ();

  return 0;
}

/* Instruction cache inhibit bit test
   Set and clear CI bit and check the pattern. */
int itlb_ci_test(void)
{
  int i, j;
  unsigned long ea, ta, ret;
 
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
    ea = FLASH_START + (i*PAGE_SIZE);
    ta = FLASH_START + (i*PAGE_SIZE);
    mtspr (SPR_ITLBMR_BASE(0) + i, ea | SPR_ITLBMR_V);
    mtspr (SPR_ITLBTR_BASE(0) + i, ta | ITLB_PR_NOLIMIT);
  }
 
  /* Testing page */
  ea = RAM_START + (RAM_SIZE/2) + (TLB_TEXT_SET_NB*PAGE_SIZE);
 
  ret = lo_itlb_ci_test (ea, TLB_TEXT_SET_NB);
  ASSERT(ret == 0);
 
  return 0;
}

int main (void)
{
  int i, j;

  i = j = 0; /* Get rid of warnings */
  
  /* Register bus error handler */
  excpt_buserr = (unsigned long)bus_err_handler;

  /* Register illegal insn handler */
  excpt_illinsn = (unsigned long)ill_insn_handler;

  /* Register illegal insn handler */
  excpt_syscall = (unsigned long)sys_call_handler;

#if 1
  /* Translation test */
  dtlb_translation_test ();

  /* Virtual address match test */
#ifndef RTL_SIM
  for (j = 0; j < DTLB_WAYS; j++) {
    for (i = TLB_DATA_SET_NB; i < (DTLB_SETS - 1); i++)
      dtlb_match_test (j, i);
  }
#else
  dtlb_match_test (0, DTLB_SETS - 2);
#endif

  /* Valid bit testing */
#ifndef RTL_SIM
  for (i = TLB_DATA_SET_NB; i < (DTLB_SETS - 1); i++)
    dtlb_valid_bit_test (i);
#else
  dtlb_valid_bit_test (DTLB_SETS - 2);
#endif

  /* Permission test */
#ifndef RTL_SIM
  for (i = TLB_DATA_SET_NB; i < (DTLB_SETS - 1); i++)
    dtlb_premission_test (i);
#else
  dtlb_premission_test (DTLB_SETS - 2);
#endif

  dtlb_ci_test();
#endif

#if 1
  /* Translation test */
  itlb_translation_test ();

  /* Virtual address match test */
#ifndef RTL_SIM
  for (j = 0; j < DTLB_WAYS; j++) {
    for (i = TLB_TEXT_SET_NB + 1; i < (DTLB_SETS - 1); i++)
      itlb_match_test (j, i);
  }
#else
  itlb_match_test (0, DTLB_SETS - 2);
#endif

  /* Valid bit testing */
#ifndef RTL_SIM
  for (i = TLB_TEXT_SET_NB; i < (ITLB_SETS); i++)
    itlb_valid_bit_test (i);
#else
  itlb_valid_bit_test (ITLB_SETS-1);
#endif

  /* Permission test */
#ifndef RTL_SIM
  for (i = TLB_TEXT_SET_NB; i < (ITLB_SETS - 1); i++)
    itlb_premission_test (i);
#else
  itlb_premission_test (ITLB_SETS - 2);
#endif

  itlb_ci_test();
#endif

  report (0xdeaddead);
  exit (0);
  return 0;
}

