/* acv-uart.c. UART test for Or1ksim

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

/* UART test using ACV */

#include "spr-defs.h"
#include "support.h"

/* use this macro to comment out nonworking parts */
#define COMPLETE    0

/* Whether to do test in more detail */
#define DETAILED    0

#define UART_ADDR   (0x9c000000)
#define UART_RBR    (UART_ADDR + 0)
#define UART_THR    (UART_ADDR + 0)
#define UART_IER    (UART_ADDR + 1)
#define UART_IIR    (UART_ADDR + 2)
#define UART_FCR    (UART_ADDR + 2)
#define UART_LCR    (UART_ADDR + 3)
#define UART_MCR    (UART_ADDR + 4)
#define UART_LSR    (UART_ADDR + 5)
#define UART_MSR    (UART_ADDR + 6)
#define UART_SCR    (UART_ADDR + 7)

#define UART_DLL    (UART_ADDR + 0)
#define UART_DLH    (UART_ADDR + 1)

#define LCR_DIVL    (0x80)
#define LCR_BREAK   (0x40)
#define LCR_STICK   (0x20)
#define LCR_EVENP   (0x10)
#define LCR_PAREN   (0x08)
#define LCR_NSTOP   (0x04)
#define LCR_NBITS   (0x03)

#define LSR_DR      (0x01)
#define LSR_OE      (0x02)
#define LSR_PE      (0x04)
#define LSR_FE      (0x08)
#define LSR_BREAK   (0x10)
#define LSR_TXFE    (0x20)
#define LSR_TXE     (0x40)
#define LSR_ERR     (0x80)

#define UART_INT_LINE 19 /* To which interrupt is uart connected */

/* fails if x is false */
#define ASSERT(x) ((x)?1: fail (__FUNCTION__, __LINE__))
/* Waits a few cycles that uart can prepare its data */
#define WAIT() {asm ("l.nop");asm ("l.nop");asm ("l.nop");asm ("l.nop");}
/* fails if there is an error */
#define NO_ERROR() { unsigned x = getreg (UART_LSR); if ((x & (LSR_BREAK|LSR_FE|LSR_PE|LSR_OE)) && !(x & LSR_ERR)) \
printf ("LSR7 (0x%02x) ERR @ %i\n", x, __LINE__); ASSERT(!(x & LSR_ERR) && ((x & 0x60) != 0x40));}
#define MARK() printf ("Passed line %i\n", __LINE__)

#ifndef __LINE__
#define __LINE__  0
#endif

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

static volatile int int_cnt;
static volatile unsigned int_iir;
static volatile unsigned int_lsr;
static int int_rbr;

void interrupt_handler ()
{
  unsigned x;
  printf ("Int\n");
  report(0xdeaddead);
  report(int_iir = getreg (UART_IIR));
  report(int_lsr = getreg (UART_LSR));
  int_cnt++;
  ASSERT (int_iir != 1);
  switch (int_iir & 0xf) {
    case 0x6: printf ("Receiver LS int.\n"); break;
    case 0x4: printf ("Received Data available. Expecting %02x, received %02x\n",
                        int_rbr, x = getreg(UART_RBR));
              ASSERT (x == int_rbr);
              report (x);
              report (int_rbr);
              break;
    case 0xc: printf ("Character timeout. Expecting %02x, received %02x\n",
                        int_rbr, x = getreg(UART_RBR));
              ASSERT (x == int_rbr);
              report (x);
              report (int_rbr);
              break;
    case 0x2: printf ("THR empty.\n"); break;
    case 0x0: printf ("Modem Status.\n"); break;
    default:
      printf ("Invalid iir %x @ %i\n", int_iir, __LINE__);
      exit (1);
  }
  mtspr(SPR_PICSR, 0);
}

/* Receives a char and checks for errors */

void recv_char (int ch)
{
  unsigned x;
  char r;
  report (ch);
  /* Wait for rx fifo to be  */
  while (!((x = getreg (UART_LSR)) & LSR_DR));
  if ((x & (LSR_BREAK|LSR_FE|LSR_PE|LSR_OE)) && !(x & LSR_ERR)) printf ("LSR7 (0x%02x) ERR @ recv_char\n", x);
  ASSERT(!(x & LSR_ERR));

  printf ("expected %02x, read %02x\n", ch, r = getreg (UART_RBR));
  ASSERT (r == ch); /* compare character */
}

/* Sends a char and checks for errors */

void send_char_no_wait (int ch)
{
  report (ch);
  setreg (UART_THR, ch); /* send character */
}

void send_char (int ch)
{
  report (ch);
  while (!(getreg (UART_LSR) & LSR_TXFE));
  NO_ERROR();
  setreg (UART_THR, ch); /* send character */
  NO_ERROR();
}

void init_8n1 ()
{
  setreg (UART_IER, 0x00); /* disable interrupts */
  WAIT();
  ASSERT(getreg (UART_IIR) == 0xc1); /* nothing should be happening */
  setreg (UART_FCR, 0x07);      /* clear RX and TX FIFOs */
  setreg (UART_LCR, LCR_DIVL);
  setreg (UART_DLH, 2 >> 8);
  setreg (UART_DLL, 2 & 0xff);
  setreg (UART_LCR, 0x03);    /* 8N1 @ 2 */
  ASSERT(getreg (UART_LCR) == 0x03);
  ASSERT (!(getreg (UART_LSR) & 0x1f));
}

/* Test reset values and r/w properties of some registers */

void register_test ()
{
  printf ("register test\n");
  MARK();
  { /* test reset values */
    ASSERT(getreg (UART_RBR) == 0x00); //0
    ASSERT(getreg (UART_IER) == 0x00); //1
    ASSERT(getreg (UART_IIR) == 0xc1); //2
    ASSERT(getreg (UART_LCR) == 0x03); //3
    ASSERT(getreg (UART_MCR) == 0x00); //4
    ASSERT(getreg (UART_LSR) == 0x60); //5
    ASSERT(getreg (UART_MSR) == 0x00); //6
    ASSERT(getreg (UART_SCR) == 0x00); //7

    setreg(UART_LCR, LCR_DIVL); //enable latches
    ASSERT(getreg (UART_DLL) == 0x00); //0
    ASSERT(getreg (UART_DLH) == 0x00); //1
    setreg(UART_LCR, 0x00); //disable latches
  }
    setreg(UART_LCR, 0x00); //disable latches igor
    setreg(UART_LCR, 0x00); //disable latches
    setreg(UART_LCR, 0x00); //disable latches
    setreg(UART_LCR, 0x00); //disable latches
    setreg(UART_LCR, 0x00); //disable latches

  MARK();
  { /* test if status registers are read only */
    unsigned long tmp;
    int i;
    tmp = getreg (UART_LSR);
    setreg (UART_LSR, ~tmp);
    ASSERT(getreg (UART_LSR) == tmp);

    for (i = 0; i < 9; i++) {
      setreg (UART_LSR, 1 << i);
      ASSERT(getreg (UART_LSR) == tmp);
    }
    
    tmp = getreg (UART_MSR);
    setreg (UART_MSR, ~tmp);
    ASSERT(getreg (UART_MSR) == tmp);
    
    for (i = 0; i < 9; i++) {
      setreg (UART_MSR, 1 << i);
      ASSERT(getreg (UART_MSR) == tmp);
    }
  }

  MARK();
  { /* test whether MCR is write only, be careful not to set the loopback bit */
    ASSERT(getreg (UART_MCR) == 0x00);
    setreg (UART_MCR, 0x45);
    ASSERT(getreg (UART_MCR) == 0x00);
    setreg (UART_MCR, 0xaa);
    ASSERT(getreg (UART_MCR) == 0x00);
  }
  ASSERT (!(getreg (UART_LSR) & 0x1f));
  MARK();
  { /* Test if Divisor latch byte holds the data */
    int i;
    setreg(UART_LCR, LCR_DIVL); //enable latches
    ASSERT(getreg (UART_LCR) == LCR_DIVL);
    for (i = 0; i < 16; i++) {
      unsigned short tmp = 0xdead << i;
      setreg (UART_DLH, tmp >> 8);
      setreg (UART_DLL, tmp & 0xff);
      ASSERT(getreg (UART_DLL) == (tmp & 0xff)); //0
      ASSERT(getreg (UART_DLH) == (tmp >> 8)); //1
    }
      setreg (UART_DLH, 0xa1); //igor
      setreg (UART_DLH, 0xa1); //igor
      setreg (UART_DLH, 0xa1); //igor
      setreg (UART_DLH, 0xa1); //igor
      setreg (UART_DLH, 0xa1); //igor

    ASSERT (!(getreg (UART_LSR) & 0x1f));
    for (i = 0; i < 16; i++) {
      unsigned short tmp = 0xdead << i;
      setreg (UART_DLL, tmp >> 8);
      setreg (UART_DLH, tmp & 0xff);
      ASSERT(getreg (UART_DLL) == (tmp >> 8)); //1
      ASSERT(getreg (UART_DLH) == (tmp & 0xff)); //0
    }
      setreg (UART_DLH, 0xa2); //igor
      setreg (UART_DLH, 0xa2); //igor
      setreg (UART_DLH, 0xa2); //igor
      setreg (UART_DLH, 0xa2); //igor
      setreg (UART_DLH, 0xa2); //igor

    setreg(UART_LCR, 0x00); //disable latches
    ASSERT(getreg (UART_LCR) == 0x00);
    ASSERT (!(getreg (UART_LSR) & 0x1f));
  }
  MARK();
  { /* Test LCR, if it holds our data */
    int i;
    for (i = 0; i < 6; i++) {
      unsigned short tmp = (0xde << i) & 0x3f;
      setreg (UART_LCR, tmp);
      ASSERT(getreg (UART_LCR) == tmp);
    }
    ASSERT (!(getreg (UART_LSR) & 0x1f));
  }
      setreg (UART_LCR, 0xa3); //igor
      setreg (UART_LCR, 0xa3); //igor
      setreg (UART_LCR, 0xa3); //igor
      setreg (UART_LCR, 0xa3); //igor
      setreg (UART_LCR, 0xa3); //igor

  MARK ();

  { /* SCR Test :))) */
    int i;
    setreg (UART_SCR, 0);
    ASSERT (getreg (UART_SCR) == 0);
    setreg (UART_SCR, 0xff);
    ASSERT (getreg (UART_SCR) == 0xff);
    for (i = 0; i < 16; i++) {
      unsigned char tmp = 0xdead << i;
      setreg (UART_SCR, tmp);
      ASSERT (getreg (UART_SCR) == tmp);
    }
  }
      setreg (UART_SCR, 0xa5);//igor
      setreg (UART_SCR, 0xa5);//igor
      setreg (UART_SCR, 0xa5);//igor
      setreg (UART_SCR, 0xa5);//igor
      setreg (UART_SCR, 0xa5);//igor

  MARK();
  /* Other registers will be tested later, if they function correctly,
     since we cannot test them now, without destroying anything.  */
}

/* Initializes uart and sends a few bytes to VAPI. It is then activated and send something back. */

void send_recv_test ()
{
  char *s;
  printf ("send_recv_test\n");
  /* Init */
  MARK();
  
  //printf ("basic\n");
  ASSERT (!(getreg (UART_LSR) & LSR_DR));
  MARK();

  /* Send a string */
  s = "send_";
  while (*s) {
    /* Wait for tx fifo to be empty */
    send_char (*s);
    report((unsigned long)*s); 
    s++;
  }
  ASSERT (!(getreg (UART_LSR) & LSR_DR));
  s = "test_";
  while (*s) {
    /* Wait for tx fifo and tx to be empty */
    while (!(getreg (UART_LSR) & LSR_TXE));
    NO_ERROR();
    setreg (UART_THR, *s); /* send character */
    NO_ERROR();
    s++;
  }
  ASSERT (!(getreg (UART_LSR) & LSR_DR));
  MARK();
  
  /* Send characters with delay inbetween */
  s = "is_running";
  while (*s) {
    int i;
    send_char (*s);
// igor   for (i = 0; i < 1600; i++) /* wait at least ten chars before sending next one */
    for (i = 0; i < 16; i++) /* wait at few chars before sending next one */
      asm volatile ("l.nop");
    s++;
  }
  
  send_char (0); /* send terminate char */
  MARK();

  /* Receives and compares the string */
  s = "recv";
  while (*s) recv_char (*s++);
  MARK();
  printf ("OK\n");
}

/* sends break in both directions */

void break_test ()
{
  unsigned x;
  char *s;
  printf ("break_test\n");
  
  MARK();
  /* Send a break */
  NO_ERROR();
  MARK();
  setreg (UART_LCR, 0x03 | LCR_BREAK); /* 8N1 */
  MARK();
  send_char ('b'); /* make sure it is recognised as a break */
  MARK();
  recv_char ('*');
  setreg (UART_LCR, 0x03); /* deleting break bit, 8N1 */
  MARK();

  /* Receive a break */
  send_char ('!');
  MARK();
  while (!((x = getreg (UART_LSR)) & LSR_DR));
  /* we should receive zero character with broken frame and break bit should be set */
  printf("[%x]\n", (LSR_DR | LSR_BREAK | LSR_ERR | LSR_TXFE | LSR_TXE));
  ASSERT (x == (LSR_DR | LSR_BREAK | LSR_ERR | LSR_TXFE | LSR_TXE));
  ASSERT (getreg (UART_RBR) == 0);
  MARK();

  /* Send a # to release break */
  setreg (UART_THR, '#');
  while (!(getreg (UART_LSR) & LSR_DR));
  NO_ERROR(); /* BREAK bit should be cleared now  */
  ASSERT (getreg (UART_RBR) == '$');
  MARK();
  
  /* Break while sending characters */
  s = "ns";
  while (*s) send_char (*s++);
  ASSERT (!(getreg (UART_LSR) & LSR_DR));
  while (!(getreg (UART_LSR) & LSR_TXE)); /* Wait till we send everything */
  /* this should break the * char, so it should not be received */
  setreg (UART_THR, '*');
  setreg (UART_LCR, 0x3 | LCR_BREAK);
  MARK();

  /* Drop the break, when we get acknowledge */
  recv_char ('?');
  setreg (UART_LCR, 0x3);
  NO_ERROR();
  MARK();

  /* Receive a break */
  send_char ('#');
  while (!((x = getreg (UART_LSR)) & LSR_DR));
  /* we should receive zero character with broken frame and break bit
     should not be set, because we cleared it */
  printf("[%x:%x]\n", x, (LSR_DR | LSR_BREAK |LSR_ERR | LSR_TXFE | LSR_TXE));
  ASSERT (x == (LSR_DR | LSR_BREAK |LSR_ERR | LSR_TXFE | LSR_TXE));
  ASSERT (getreg (UART_RBR) == 0);
  MARK();
  send_char ('?');
  MARK();
  while (!(getreg (UART_LSR) & LSR_DR));
  recv_char ('!');
  printf ("OK\n");
}

/* Tries to send data in different modes in both directions */

/* Utility function, that tests current configuration */
void test_mode (int nbits)
{
  unsigned mask = (1 << nbits) - 1;
  send_char (0x55);
#if DETAILED
  send_char (0x55);
  recv_char (0x55 & mask);
#endif
  recv_char (0x55 & mask);
  send_char ('a');                  // 0x61
#if DETAILED
  send_char ('a');                  // 0x61
  recv_char ('a' & mask);
#endif
  recv_char ('a' & mask);
}

void different_modes_test ()
{
  int speed, parity, length;
  printf ("different modes test\n");
  init_8n1();
 
  /* Init */
  MARK();
  ASSERT(getreg (UART_IIR) == 0xc1); /* nothing should be happening */
  MARK();

  /* Test different speeds */
  for (speed = 1; speed < 5; speed++) {
    setreg (UART_LCR, LCR_DIVL);
    setreg (UART_DLH, speed >> 8);
    setreg (UART_DLL, speed & 0xff);
    setreg (UART_LCR, 0x03);    /* 8N1 @ 10 => 160 instructions for one cycle */
    test_mode (8);
    MARK();
  }
  MARK();
  
  setreg (UART_LCR, LCR_DIVL);
  setreg (UART_DLH, 1 >> 8);
  setreg (UART_DLL, 1 & 0xff);
  MARK();
  
  /* Test all parity modes with different char lengths */
  for (parity = 0; parity < 8; parity++)
    for (length = 0; length < 4; length++) {
      setreg (UART_LCR, length | (0 << 2) | (parity << 3));
      test_mode (5 + length);
      MARK();
    }
  MARK();
  
  /* Test configuration, if we have >1 stop bits */
  for (length = 0; length < 4; length++) {
    setreg (UART_LCR, length | (1 << 2) | (0 << 3));
    test_mode (5 + length);
    MARK();
  }
  MARK();
  
  /* Restore normal mode */
  send_char ('T');
  while (getreg (UART_LSR) != 0x60); /* Wait for THR to be empty */
  setreg (UART_LCR, LCR_DIVL);
  setreg (UART_DLH, 2 >> 8);
  setreg (UART_DLL, 2 & 0xff);
  setreg (UART_LCR, 0x03);    /* 8N1 @ 2 */
  MARK();
  while (!(getreg (UART_LSR) & 1));  /* Receive 'x' char */
  getreg (UART_RBR);
  MARK();
  
  send_char ('T');
  while (getreg (UART_LSR) != 0x60); /* Wait for THR to be empty */
  MARK();
  printf ("OK\n");
}

/* Test various FIFO levels, break and framing error interrupt, etc */

void interrupt_test ()
{
  int i;
  printf ("interrupt_test\n");
  /* Configure UART for interrupt mode */
  ASSERT(getreg (UART_IIR) == 0xc1); /* nothing should be happening */
  setreg (UART_LCR, LCR_DIVL);
  setreg (UART_DLH, 12 >> 8);            /* Set relatively slow speed, so we can hanlde interrupts properly */
  setreg (UART_DLL, 6 & 0xff);
  setreg (UART_LCR, 0x03);    /* 8N1 @ 6 */

  ASSERT (int_cnt == 0);   /* We should not have got any interrupts before this test */
  setreg (UART_FCR, 0x01); /* Set trigger level = 1 char, fifo should not be reset */
  setreg (UART_IER, 0x07); /* Enable interrupts: line status, THR empty, data ready */

  while (!int_cnt); /* Clear previous THR interrupt */
  ASSERT (--int_cnt == 0);
  ASSERT (int_iir == 0xc2);
  ASSERT ((int_lsr & 0xbe) == 0x20);
  MARK();

  /* I am configured - start interrupt test */
  send_char ('I');
  while (!int_cnt); /* Wait for THR to be empty */
  ASSERT (--int_cnt == 0);
  ASSERT (int_iir == 0xc2);
  ASSERT ((int_lsr & 0xbe) == 0x20);
  MARK();
  
  int_rbr = '0';
  while (!int_cnt); /* Wait for DR */
  ASSERT (--int_cnt == 0);
  ASSERT (int_iir == 0xc4);
  ASSERT (int_lsr == 0x61);
  ASSERT (int_cnt == 0); /* no interrupts should be pending */
  MARK();

  setreg (UART_FCR, 0x41); /* Set trigger level = 4 chars, fifo should not be reset */

  /* Everything ok here, send me 4 more */
  send_char ('I');
  while (!int_cnt); /* Wait for THR to be empty */
  ASSERT (--int_cnt == 0);
  ASSERT (int_iir == 0xc2);
  ASSERT ((int_lsr & 0xbe) == 0x20);
  MARK();

  int_rbr = '1';
  while (!int_cnt); /* Wait for DR */
  ASSERT (--int_cnt == 0);
  ASSERT (int_iir == 0xc4);
  ASSERT (int_lsr == 0x61);
  MARK();
  
  setreg (UART_FCR, 0x81); /* Set trigger level = 8 chars, fifo should not be reset */
  
  /* Everything ok here, send me 5 more */
  send_char ('I');
  while (!int_cnt); /* Wait for THR to be empty */
  ASSERT (--int_cnt == 0);
  ASSERT (int_iir == 0xc2);
  ASSERT ((int_lsr & 0xbe) == 0x20);
  MARK();
  
  int_rbr = '2';
  while (!int_cnt); /* Wait for DR */
  ASSERT (--int_cnt == 0);
  ASSERT (int_iir == 0xc4);
  ASSERT (int_lsr == 0x61);
  MARK();
  
  setreg (UART_FCR, 0xc1); /* Set trigger level = 14 chars, fifo should not be reset */
  
  /* Everything ok here, send me 7 more */
  send_char ('I');
  while (!int_cnt); /* Wait for THR to be empty */
  ASSERT (--int_cnt == 0);
  ASSERT (int_iir == 0xc2);
  ASSERT ((int_lsr & 0xbe) == 0x20);
  MARK();
  
  int_rbr = '3';
  while (!int_cnt); /* Wait for DR */
  ASSERT (--int_cnt == 0);
  ASSERT (int_iir == 0xc4);
  ASSERT (int_lsr == 0x61);
  MARK();

  /* Everything ok here, send me 4 more - fifo should be full OE should occur */
  setreg (UART_IER, 0x06); /* Enable interrupts: line status, THR empty */
  send_char ('I');
  while (!int_cnt); /* Wait for THR to be empty */
  ASSERT (--int_cnt == 0);
  ASSERT (int_iir == 0xc2);
  ASSERT ((int_lsr & 0xbe) == 0x20);
  MARK();
  
  while (!int_cnt); /* Wait for OE */
  ASSERT (--int_cnt == 0);
  ASSERT (int_iir == 0xc6);
  ASSERT (int_lsr == 0xe3);           /* OE flag should be set */
  ASSERT (getreg (UART_LSR) == 0x61); /* LSR should be cleared by previous read */
  ASSERT (getreg (UART_IIR) == 0xc1); /* No interrupts should be pending */
  MARK();
  
  /* Check if we got everything */
  ASSERT (int_cnt == 0); /* no interrupts should be pending */
  for (i = 0; i < 3; i++) {
    recv_char ("456"[i]);   /* WARNING: read should not cause interrupt even if we step over trigger */
    MARK ();
  }
  /* It is now safe to enable data ready interrupt */
  setreg (UART_IER, 0x07); /* Enable interrupts: line status, THR empty, data ready */
  
  /* Check if we got everything */
  for (i = 0; i < 13; i++) {
    recv_char ("789abcdefghij"[i]);   /* WARNING: read should not cause interrupt even if we step over trigger */
    MARK ();
  }
  
  ASSERT (int_cnt == 0); /* no interrupts should be pending */
  ASSERT (getreg (UART_LSR) == 0x60); /* FIFO should be empty */
  
  getreg (UART_RBR);      /* check for FIFO counter overflow - fifo must still be empty */
  ASSERT (getreg (UART_LSR) == 0x60); /* FIFO should be empty */

  /* check for break interrupt */
  send_char ('I');
  while (!int_cnt); /* Wait for THR to be empty */
  ASSERT (--int_cnt == 0);
  ASSERT (int_iir == 0xc2);
  ASSERT ((int_lsr & 0xbe) == 0x20);
  MARK();
  
  while (!int_cnt); /* Wait for break interrupt */
  ASSERT (--int_cnt == 0);
  ASSERT (int_iir == 0xc6);
  ASSERT (int_lsr == 0xf1); /* BE flag should be set */
  ASSERT (getreg (UART_LSR) == 0x61); /* BE flag should be cleared by previous read */
  MARK();
  recv_char (0);
  MARK();
  
  send_char ('B');  /* Release break */
  while (!int_cnt); /* Wait for THR to be empty */
  ASSERT (--int_cnt == 0);
  ASSERT (int_iir == 0xc2);
  ASSERT ((int_lsr & 0xbe) == 0x20);
  MARK();
  /* Wait for acknowledge */
  int_rbr = '$';
  while (!int_cnt); /* Wait for timeout */
  ASSERT (--int_cnt == 0);
  ASSERT (int_iir == 0xcc);
  ASSERT (int_lsr == 0x61);
  MARK();
  
  /* TODO: Check for parity error */
  /* TODO: Check for frame error */

  /* Check for timeout */
  send_char ('I');
  while (!int_cnt); /* Wait for THR to be empty */
  ASSERT (--int_cnt == 0);
  ASSERT (int_iir == 0xc2);
  ASSERT ((int_lsr & 0xbe) == 0x20);
  MARK();
  
  int_rbr = 'T';
  while (!int_cnt); /* Wait for timeout */
  ASSERT (--int_cnt == 0);
  ASSERT (int_iir == 0xcc);  /* timeout interrupt occured */
  ASSERT (int_lsr == 0x61); /* DR flag should be set */
  ASSERT (getreg (UART_LSR) == 0x60); /* DR flag should be cleared - timeout occurred */
  MARK();

  send_char ('T');
  while (!int_cnt); /* Wait for THR to be empty */
  ASSERT (--int_cnt == 0);
  ASSERT (int_iir == 0xc2);
  ASSERT ((int_lsr & 0xbe) == 0x20);
  MARK();

  setreg (UART_IER, 0x00); /* Disable interrupts */
  ASSERT (int_cnt == 0); /* no interrupts should be pending */
  NO_ERROR ();
  
  while (getreg (UART_LSR) != 0x60);  /* wait till we sent everynthing and then change mode */
  setreg (UART_LCR, LCR_DIVL);
  setreg (UART_DLH, 2 >> 8);            /* Set relatively slow speed, so we can hanlde interrupts properly */
  setreg (UART_DLL, 2 & 0xff);
  setreg (UART_LCR, 0x03);    /* 8N1 @ 2 */
  send_char ('T');
  
  MARK ();
  printf ("OK\n");
}

/* Test if all control bits are set correctly.  Lot of this was already tested
   elsewhere and tests are not duplicated.  */

void control_register_test ()
{
  /* RBR already tested in send_recv_test() */
  /* THR already tested in send_recv_test() */
  /* IER already tested in interrupt_test() */
  /* IIR already tested in interrupt_test() */
  /* FCR0 - uart 16450 specific, not tested */
  
  /* FCR1 - reset rx FIFO */
  send_char ('*');
  NO_ERROR ();
  while (!(getreg (UART_LSR) & 0x01)); /* Wait for data ready */
  setreg (UART_FCR, 2); /* Clears rx fifo */
  ASSERT (getreg (UART_LSR) == 0x60);  /* nothing happening */
  send_char ('!');
  recv_char ('!');
  MARK ();

  /* Make sure the TX fifo and the TX serial reg. are empty */
  ASSERT (getreg (UART_LSR) & LSR_TXFE);
  ASSERT (getreg (UART_LSR) & LSR_TXE);
  
  /* FCR2 - reset tx FIFO */
send_char_no_wait ('1');
send_char_no_wait ('2');
//  send_char ('1');
//  send_char ('2');
  setreg (UART_FCR, 4); /* Should clear '2' from fifo, but '1' should be sent OK */
  ASSERT (getreg (UART_LSR) == 0x20);  /* we should still be sending '1' */
  NO_ERROR();
  send_char ('*');
  recv_char ('*');
  MARK ();
  
  /* LCR already tested in different_modes_test () */
  /* TODO: MSR */
  /* LSR already tested in different_modes_test () and interrupt_test() */
  /* SCR already tested in register_test () */
  
  MARK ();
  printf ("OK\n");
}

/* Tests parity error and frame error behaviour */

void line_error_test ()
{
  printf ("line_error_test\n");
  
  /* Test framing error if we change speed */
  setreg (UART_LCR, LCR_DIVL);
  setreg (UART_DLH, 12 >> 8);
  setreg (UART_DLL, 12 & 0xff);
  setreg (UART_LCR, 0x03);    /* 8N1 @ 3 */
  MARK();
  
  send_char ('c');
  ASSERT (int_cnt == 0);
  setreg (UART_IER, 0x04); /* Enable interrupts: line status */
  while (!int_cnt); /* Wait for THR to be empty */
  ASSERT (--int_cnt == 0);
  ASSERT (int_iir == 0xc6);
  ASSERT (int_lsr == 0xe9); /* Framing error and FIFO error */
  getreg (UART_RBR);        /* Ignore the data */
  MARK ();
  recv_char ('b');
  MARK ();

#if COMPLETE  
  /* Test framing error if we change stop bits */
  send_char ('*');
  while (getreg (UART_LSR));  /* wait till we sent everynthing and then change mode */
  setreg (UART_LCR, 0x07); /* 8N2 */
  send_char ('*');
  MARK ();

  ASSERT (int_cnt == 0);
  setreg (UART_IER, 0x04); /* Enable interrupts: line status */
  while (!int_cnt); /* Wait for THR to be empty */
  ASSERT (--int_cnt == 0);
  ASSERT (int_iir == 0xc6);
  ASSERT (int_lsr == 0xe9); /* Framing error and FIFO error */
  getreg (UART_RBR);        /* Ignore the data */
  recv_char ('b');
  MARK();
#endif

  MARK ();
  printf ("OK\n");
}

int main ()
{
  /* Use our low priority interrupt handler */
  excpt_int = (unsigned long)interrupt_handler;
  
  /* Enable interrupts */
  mtspr (SPR_SR, mfspr(SPR_SR) | SPR_SR_IEE);
  mtspr (SPR_PICMR, mfspr(SPR_PICMR) | (0x00000001L << UART_INT_LINE));

  int_cnt = 0;
  int_iir = 0;
  int_lsr = 0;
  int_rbr = 0;

  register_test ();
  init_8n1 ();
  send_recv_test ();
  break_test ();
  different_modes_test ();
  interrupt_test ();
  control_register_test ();
  line_error_test ();

  /* loopback_test ();
  modem_test ();
  modem_error_test ();*/
  recv_char ('@');
  printf ("ALL TESTS PASSED\n");
  return 0;
}
