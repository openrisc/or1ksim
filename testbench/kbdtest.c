/* Simple keyboard test. Outputs scan codes. */
#include "support.h"
#include "spr_defs.h"
#include "board.h"

/* Whether this test should be run in interactive mode; scan codes are not check against real ones */
#define INTERACTIVE 0

/* fails if x is false */
#define ASSERT(x) ((x)?1: fail (__FUNCTION__, __LINE__))
/* Waits a few cycles that uart can prepare its data */
#define WAIT() {asm ("l.nop");asm ("l.nop");asm ("l.nop");asm ("l.nop");}
#define MARK() printf ("Passed line %i\n", __LINE__)

#ifndef __LINE__
#define __LINE__  0
#endif

#if !INTERACTIVE
static const unsigned char incoming_scan[] = {
  0x2a, 0x14, 0x94, 0xaa, 0x12, 0x92, 0x04, 0x84, 0x2a, 0x0d,
  0x8d, 0xaa, 0x0d, 0x8d, 0x0c, 0x8c, 0x2a, 0x33, 0xb3, 0xaa,
  0x35, 0xb5, 0x34, 0xb4, 0x2b, 0xab, 0x2a, 0x2b, 0xab, 0xaa,
  0x2a, 0x28, 0xa8, 0xaa, 0x28, 0xa8, 0x29, 0xa9, 0x2a, 0x1b,
  0x9b, 0xaa, 0x1b, 0x9b, 0x0f, 0x8f, 0x39, 0xb9, 0x2a, 0x02,
  0x82, 0xaa, 0x2a, 0x06, 0x86, 0xaa, 0x2a, 0x07, 0x87, 0xaa,
  0x2a, 0x08, 0x88, 0xaa, 0x2a, 0x0b, 0x8b, 0xaa, 0x2a, 0x09,
  0x89, 0xaa, 0x1c, 0x9c, 0x39, 0xb9, 0x00};
static int current_scan = 0;
#endif

static volatile int done;

void fail (char *func, int line)
{
#ifndef __FUNCTION__
#define __FUNCTION__ "?"
#endif
  printf ("Test failed in %s:%i\n", func, line);
  report(0xeeeeeeee);
  exit (1);
}

inline void setreg (unsigned long addr, unsigned char value)
{
  *((volatile unsigned char *)addr) = value;
}

inline unsigned long getreg (unsigned long addr)
{
  return *((volatile unsigned char *)addr);
}

void interrupt_handler ()
{
  unsigned x;
  printf ("Int\n");
  do {
    x = getreg (KBD_BASE_ADD);
    if (x) printf ("0x%02x, ", x);
    report(x);
    if (x == 1) done = 1;
#if !INTERACTIVE
    printf ("expecting (0x%02x), ", incoming_scan[current_scan]);
    if (x) {
      ASSERT (incoming_scan[current_scan++] == x);
    }
    if ((current_scan + 1) >= sizeof (incoming_scan) / sizeof (char)) done = 1;
#endif
  } while (x);
  printf ("%i", done);
  mtspr(SPR_PICSR, 0);
}

int main ()
{
  /* Use our low priority interrupt handler */
  excpt_int = (unsigned long)interrupt_handler;

  printf ("Reading from keyboard.\n");
  printf ("Enabling interrupts.\n");
  done = 0;
  
  /* Enable interrupts */
  mtspr (SPR_SR, mfspr(SPR_SR) | SPR_SR_IEE);
  mtspr (SPR_PICMR, mfspr(SPR_PICMR) | (0x00000001L << KBD_IRQ));

  while (!done) printf ("[%i]", done);
  report (0xdeaddead);
  return 0;
}
