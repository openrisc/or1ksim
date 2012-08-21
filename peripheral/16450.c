/* 16450.c -- Simulation of 8250/16450 serial UART

   Copyright (C) 1999 Damjan Lampret, lampret@opencores.org
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

/* This is functional simulation of 8250/16450/16550 UARTs. Since we RX/TX
   data via file streams, we can't simulate modem control lines coming from
   the DCE and similar details of communication with the DCE.

   This simulated UART device is intended for basic UART device driver
   verification. From device driver perspective this device looks like a
   regular UART but never reports any modem control lines changes (the
   only DCE responses are incoming characters from the file stream). */


/* Autoconf and/or portability configuration */
#include "config.h"
#include "port.h"

/* System includes */
#include <stdlib.h>

/* Package includes */
#include "sim-config.h"
#include "arch.h"
#include "pic.h"
#include "sched.h"
#include "vapi.h"
#include "channel.h"
#include "abstract.h"
#include "toplevel-support.h"
#include "sim-cmd.h"


#define MIN(a,b) ((a) < (b) ? (a) : (b))
/* Definitions */
#define UART_ADDR_SPACE        8	/*!< UART addr space size in bytes */
#define UART_MAX_FIFO_LEN     16	/*!< rx FIFO for uart 16550 */
#define MAX_SKEW               1	/*!< max. clock skew in subclocks */
#define UART_VAPI_BUF_LEN    128	/*!< Size of VAPI command buffer */
#define UART_CLOCK_DIVIDER    16	/*!< UART clock divider */
#define UART_FGETC_SLOWDOWN  100	/*!< fgetc() slowdown factor */

/*
 * Addresses of visible registers
 *
 */
#define UART_RXBUF  0		/* R: Rx buffer, DLAB=0 */
#define UART_TXBUF  0		/* W: Tx buffer, DLAB=0 */
#define UART_DLL  0		/* R/W: Divisor Latch Low, DLAB=1 */
#define UART_DLH  1		/* R/W: Divisor Latch High, DLAB=1 */
#define UART_IER  1		/* R/W: Interrupt Enable Register */
#define UART_IIR  2		/* R: Interrupt ID Register */
#define UART_FCR  2		/* W: FIFO Control Register */
#define UART_LCR  3		/* R/W: Line Control Register */
#define UART_MCR  4		/* W: Modem Control Register */
#define UART_LSR  5		/* R: Line Status Register */
#define UART_MSR  6		/* R: Modem Status Register */
#define UART_SCR  7		/* R/W: Scratch Register */

/*
 * R/W masks for valid bits in 8250/16450 (mask out 16550 and later bits)
 *
 */
#define UART_VALID_LCR  0xff
#define UART_VALID_LSR  0xff
#define UART_VALID_IIR  0x0f
#define UART_VALID_FCR  0xc0
#define UART_VALID_IER  0x0f
#define UART_VALID_MCR  0x1f
#define UART_VALID_MSR  0xff

/*
 * Bit definitions for the Line Control Register
 * 
 */
#define UART_LCR_DLAB 0x80	/* Divisor latch access bit */
#define UART_LCR_SBC  0x40	/* Set break control */
#define UART_LCR_SPAR 0x20	/* Stick parity (?) */
#define UART_LCR_EPAR 0x10	/* Even parity select */
#define UART_LCR_PARITY 0x08	/* Parity Enable */
#define UART_LCR_STOP 0x04	/* Stop bits: 0=1 stop bit, 1= 2 stop bits */
#define UART_LCR_WLEN5  0x00	/* Wordlength: 5 bits */
#define UART_LCR_WLEN6  0x01	/* Wordlength: 6 bits */
#define UART_LCR_WLEN7  0x02	/* Wordlength: 7 bits */
#define UART_LCR_WLEN8  0x03	/* Wordlength: 8 bits */
#define UART_LCR_RESET  0x03
/*
 * Bit definitions for the Line Status Register
 */
#define UART_LSR_RXERR  0x80	/* Error in rx fifo */
#define UART_LSR_TXSERE 0x40	/* Transmitter serial register empty */
#define UART_LSR_TXBUFE 0x20	/* Transmitter buffer register empty */
#define UART_LSR_BREAK  0x10	/* Break interrupt indicator */
#define UART_LSR_FRAME  0x08	/* Frame error indicator */
#define UART_LSR_PARITY 0x04	/* Parity error indicator */
#define UART_LSR_OVRRUN 0x02	/* Overrun error indicator */
#define UART_LSR_RDRDY  0x01	/* Receiver data ready */

/*
 * Bit definitions for the Interrupt Identification Register
 */
#define UART_IIR_NO_INT 0x01	/* No interrupts pending */
#define UART_IIR_ID 0x06	/* Mask for the interrupt ID */

#define UART_IIR_MSI  0x00	/* Modem status interrupt (Low priority) */
#define UART_IIR_THRI 0x02	/* Transmitter holding register empty */
#define UART_IIR_RDI  0x04	/* Receiver data interrupt */
#define UART_IIR_RLSI 0x06	/* Receiver line status interrupt (High p.) */
#define UART_IIR_CTI  0x0c	/* Character timeout */

/*
 * Bit Definitions for the FIFO Control Register
 */
#define UART_FCR_FIE  0x01	/* FIFO enable */
#define UART_FCR_RRXFI 0x02	/* Reset rx FIFO */
#define UART_FCR_RTXFI 0x04	/* Reset tx FIFO */
#define UART_FIFO_TRIGGER(x) /* Trigger values for indexes 0..3 */\
  ((x) == 0 ? 1\
  :(x) == 1 ? 4\
  :(x) == 2 ? 8\
  :(x) == 3 ? 14 : 0)

/*
 * Bit definitions for the Interrupt Enable Register
 */
#define UART_IER_MSI  0x08	/* Enable Modem status interrupt */
#define UART_IER_RLSI 0x04	/* Enable receiver line status interrupt */
#define UART_IER_THRI 0x02	/* Enable Transmitter holding register int. */
#define UART_IER_RDI  0x01	/* Enable receiver data interrupt */

/*
 * Bit definitions for the Modem Control Register
 */
#define UART_MCR_LOOP 0x10	/* Enable loopback mode */
#define UART_MCR_AUX2 0x08	/* Auxilary 2  */
#define UART_MCR_AUX1 0x04	/* Auxilary 1 */
#define UART_MCR_RTS  0x02	/* Force RTS */
#define UART_MCR_DTR  0x01	/* Force DTR */

/*
 * Bit definitions for the Modem Status Register
 */
#define UART_MSR_DCD  0x80	/* Data Carrier Detect */
#define UART_MSR_RI   0x40	/* Ring Indicator */
#define UART_MSR_DSR  0x20	/* Data Set Ready */
#define UART_MSR_CTS  0x10	/* Clear to Send */
#define UART_MSR_DDCD 0x08	/* Delta DCD */
#define UART_MSR_TERI 0x04	/* Trailing edge ring indicator */
#define UART_MSR_DDSR 0x02	/* Delta DSR */
#define UART_MSR_DCTS 0x01	/* Delta CTS */

/*
 * Various definitions
 */
#define UART_BREAK_COUNT   1	/* # of chars to count when doing break */
#define UART_CHAR_TIMEOUT  4	/* # of chars to count when doing timeout int */


/* Registers */

struct dev_16450
{
  struct
  {
    uint8_t   txbuf[UART_MAX_FIFO_LEN];
    uint16_t  rxbuf[UART_MAX_FIFO_LEN];	/* Upper 8-bits is the LCR modifier */
    uint8_t   dll;
    uint8_t   dlh;
    uint8_t   ier;
    uint8_t   iir;
    uint8_t   fcr;
    uint8_t   lcr;
    uint8_t   mcr;
    uint8_t   lsr;
    uint8_t   msr;
    uint8_t   scr;
  } regs;			/* Visible registers */
  struct
  {
    uint8_t   txser;		/* Character just sending */
    uint16_t  rxser;		/* Character just receiving */
    uint8_t   loopback;
  } iregs;			/* Internal registers */
  struct
  {
    int           txbuf_head;
    int           txbuf_tail;
    int           rxbuf_head;
    int           rxbuf_tail;
    unsigned int  txbuf_full;
    unsigned int  rxbuf_full;
    int           receiveing;	/* Receiving a char */
    int           recv_break;	/* Receiving a break */
    int           ints;		/* Which interrupts are pending */
  } istat;			/* Internal status */

  /* Clocks per char */
  unsigned long  char_clks;

  /* VAPI internal registers */
  struct
  {
    unsigned long  char_clks;
    uint8_t        dll;
    uint8_t        dlh;
    uint8_t        lcr;
    int            skew;
  } vapi;

  /* Required by VAPI - circular buffer to store incoming chars, since we
     cannothandle them so fast - we are serial. */
  unsigned long   vapi_buf[UART_VAPI_BUF_LEN];	/* Buffer */
  int             vapi_buf_head_ptr;		/* Where we write to */
  int             vapi_buf_tail_ptr;		/* Where we read from */

  /* Length of FIFO, 16 for 16550, 1 for 16450 */
  int             fifo_len;

  struct channel *channel;

  /* Configuration */
  int             enabled;
  int             jitter;
  oraddr_t        baseaddr;
  int             irq;
  unsigned long   vapi_id;
  int             uart16550;
  char           *channel_str;
};

/* Forward declarations of static functions */
static void uart_recv_break (void *dat);
static void uart_recv_char (void *dat);
static void uart_check_char (void *dat);
static void uart_sched_recv_check (struct dev_16450 *uart);
static void uart_vapi_cmd (void *dat);
static void uart_clear_int (struct dev_16450 *uart, int intr);
static void uart_tx_send (void *dat);


/* Number of clock cycles (one clock cycle is when UART_CLOCK_DIVIDER simulator
 * cycles have elapsed) before a single character is transmitted or received. */
static unsigned long
char_clks (int dll, int dlh, int lcr)
{
  unsigned int bauds_per_char = 2;
  unsigned long char_clks = ((dlh << 8) + dll);

  if (lcr & UART_LCR_PARITY)
    bauds_per_char += 2;

  /* stop bits 1 or two */
  if (lcr & UART_LCR_STOP)
    bauds_per_char += 4;
  else if ((lcr & 0x3) != 0)
    bauds_per_char += 2;
  else
    bauds_per_char += 3;

  bauds_per_char += 10 + ((lcr & 0x3) << 1);

  return (char_clks * bauds_per_char) >> 1;
}

/*---------------------------------------------------[ Interrupt handling ]---*/
/* Signals the specified interrupt.  If a higher priority interrupt is already
 * pending, do nothing */
static void
uart_int_msi (void *dat)
{
  struct dev_16450 *uart = dat;

  uart->istat.ints |= 1 << UART_IIR_MSI;

  if (!(uart->regs.ier & UART_IER_MSI))
    return;

  if ((uart->regs.iir & UART_IIR_NO_INT) || (uart->regs.iir == UART_IIR_MSI))
    {
      uart->regs.iir = UART_IIR_MSI;
      report_interrupt (uart->irq);
    }
}

static void
uart_int_thri (void *dat)
{
  struct dev_16450 *uart = dat;

  uart->istat.ints |= 1 << UART_IIR_THRI;

  if (!(uart->regs.ier & UART_IER_THRI))
    return;

  if ((uart->regs.iir & UART_IIR_NO_INT) || (uart->regs.iir == UART_IIR_MSI)
      || (uart->regs.iir == UART_IIR_THRI))
    {
      uart->regs.iir = UART_IIR_THRI;
      report_interrupt (uart->irq);
    }
}

static void
uart_int_cti (void *dat)
{
  struct dev_16450 *uart = dat;

  uart->istat.ints |= 1 << UART_IIR_CTI;

  if (!(uart->regs.ier & UART_IER_RDI))
    return;

  if ((uart->regs.iir != UART_IIR_RLSI) && (uart->regs.iir != UART_IIR_RDI))
    {
      uart->regs.iir = UART_IIR_CTI;
      report_interrupt (uart->irq);
    }
}

static void
uart_int_rdi (void *dat)
{
  struct dev_16450 *uart = dat;

  uart->istat.ints |= 1 << UART_IIR_RDI;

  if (!(uart->regs.ier & UART_IER_RDI))
    return;

  if (uart->regs.iir != UART_IIR_RLSI)
    {
      uart->regs.iir = UART_IIR_RDI;
      report_interrupt (uart->irq);
    }
}

static void
uart_int_rlsi (void *dat)
{
  struct dev_16450 *uart = dat;

  uart->istat.ints |= 1 << UART_IIR_RLSI;

  if (!(uart->regs.ier & UART_IER_RLSI))
    return;

  /* Highest priority interrupt */
  uart->regs.iir = UART_IIR_RLSI;
  report_interrupt (uart->irq);
}

/* Checks to see if an RLSI interrupt is due and schedules one if need be */
static void
uart_check_rlsi (void *dat)
{
  struct dev_16450 *uart = dat;

  if (uart->regs.lsr & (UART_LSR_OVRRUN | UART_LSR_PARITY | UART_LSR_FRAME |
			UART_LSR_BREAK))
    uart_int_rlsi (uart);
}

/* Checks to see if an RDI interrupt is due and schedules one if need be */
static void
uart_check_rdi (void *dat)
{
  struct dev_16450 *uart = dat;

  if (uart->istat.rxbuf_full >= UART_FIFO_TRIGGER (uart->regs.fcr >> 6))
    {
      uart_int_rdi (uart);
    }
}

/* Raises the next highest priority interrupt */
static void
uart_next_int (struct dev_16450 *uart)
{
  /* Interrupt detection in proper priority order. */
  if ((uart->istat.ints & (1 << UART_IIR_RLSI)) &&
      (uart->regs.ier & UART_IER_RLSI))
    uart_int_rlsi (uart);
  else if ((uart->istat.ints & (1 << UART_IIR_RDI)) &&
	   (uart->regs.ier & UART_IER_RDI))
    uart_int_rdi (uart);
  else if ((uart->istat.ints & (1 << UART_IIR_CTI)) &&
	   (uart->regs.ier & UART_IER_RDI))
    uart_int_cti (uart);
  else if ((uart->istat.ints & (1 << UART_IIR_THRI)) &&
	   (uart->regs.ier & UART_IER_THRI))
    uart_int_thri (uart);
  else if ((uart->istat.ints & (1 << UART_IIR_MSI)) &&
	   (uart->regs.ier & UART_IER_MSI))
    uart_int_msi (uart);
  else
    {
      clear_interrupt (uart->irq);
      uart->regs.iir = UART_IIR_NO_INT;
    }
}

/* Clears potentially pending interrupts */
static void
uart_clear_int (struct dev_16450 *uart, int intr)
{
  uart->istat.ints &= ~(1 << intr);

  /* Some stuff schedule uart_int_cti, therefore remove it here */
  if (intr == UART_IIR_CTI)
    SCHED_FIND_REMOVE (uart_int_cti, uart);

  if (intr != uart->regs.iir)
    return;

  uart_next_int (uart);
}

/*----------------------------------------------------[ Loopback handling ]---*/
static void
uart_loopback (struct dev_16450 *uart)
{
  if (!(uart->regs.mcr & UART_MCR_LOOP))
    return;

  if ((uart->regs.mcr & UART_MCR_AUX2) !=
      ((uart->regs.msr & UART_MSR_DCD) >> 4))
    uart->regs.msr |= UART_MSR_DDCD;

  if ((uart->regs.mcr & UART_MCR_AUX1) <
      ((uart->regs.msr & UART_MSR_RI) >> 4))
    uart->regs.msr |= UART_MSR_TERI;

  if ((uart->regs.mcr & UART_MCR_RTS) !=
      ((uart->regs.msr & UART_MSR_CTS) >> 3))
    uart->regs.msr |= UART_MSR_DCTS;

  if ((uart->regs.mcr & UART_MCR_DTR) !=
      ((uart->regs.msr & UART_MSR_DSR) >> 5))
    uart->regs.msr |= UART_MSR_DDSR;

  uart->regs.msr &=
    ~(UART_MSR_DCD | UART_MSR_RI | UART_MSR_DSR | UART_MSR_CTS);
  uart->regs.msr |= ((uart->regs.mcr & UART_MCR_AUX2) << 4);
  uart->regs.msr |= ((uart->regs.mcr & UART_MCR_AUX1) << 4);
  uart->regs.msr |= ((uart->regs.mcr & UART_MCR_RTS) << 3);
  uart->regs.msr |= ((uart->regs.mcr & UART_MCR_DTR) << 5);

  if (uart->regs.msr & (UART_MSR_DCTS | UART_MSR_DDSR | UART_MSR_TERI |
			UART_MSR_DDCD))
    uart_int_msi (uart);
}

/*----------------------------------------------------[ Transmitter logic ]---*/
/* Sends the data in the shift register to the outside world */
static void
send_char (struct dev_16450 *uart, int bits_send)
{
  PRINTF ("%c", (char) uart->iregs.txser);
  if (uart->regs.mcr & UART_MCR_LOOP)
    uart->iregs.loopback = uart->iregs.txser;
  else
    {
      /* Send to either VAPI or to file */
      if (uart->vapi_id)
	{
	  int par, pe, fe, nbits;
	  int j, data;
	  unsigned long packet = 0;

	  nbits = MIN (bits_send, (uart->regs.lcr & UART_LCR_WLEN8) + 5);
	  /* Encode a packet */
	  packet = uart->iregs.txser & ((1 << nbits) - 1);

	  /* Calculate parity */
	  for (j = 0, par = 0; j < nbits; j++)
	    par ^= (packet >> j) & 1;

	  if (uart->regs.lcr & UART_LCR_PARITY)
	    {
	      if (uart->regs.lcr & UART_LCR_SPAR)
		{
		  packet |= 1 << nbits;
		}
	      else
		{
		  if (uart->regs.lcr & UART_LCR_EPAR)
		    packet |= par << nbits;
		  else
		    packet |= (par ^ 1) << nbits;
		}
	      nbits++;
	    }
	  packet |= 1 << (nbits++);
	  if (uart->regs.lcr & UART_LCR_STOP)
	    packet |= 1 << (nbits++);

	  /* Decode a packet */
	  nbits = (uart->vapi.lcr & UART_LCR_WLEN8) + 5;
	  data = packet & ((1 << nbits) - 1);

	  /* Calculate parity, including parity bit */
	  for (j = 0, par = 0; j < nbits + 1; j++)
	    par ^= (packet >> j) & 1;

	  if (uart->vapi.lcr & UART_LCR_PARITY)
	    {
	      if (uart->vapi.lcr & UART_LCR_SPAR)
		{
		  pe = !((packet >> nbits) & 1);
		}
	      else
		{
		  if (uart->vapi.lcr & UART_LCR_EPAR)
		    pe = par != 0;
		  else
		    pe = par != 1;
		}
	      nbits++;
	    }
	  else
	    pe = 0;

	  fe = ((packet >> (nbits++)) & 1) ^ 1;
	  if (uart->vapi.lcr & UART_LCR_STOP)
	    fe |= ((packet >> (nbits++)) & 1) ^ 1;

	  data |=
	    (uart->vapi.lcr << 8) | (pe << 16) | (fe << 17) | (uart->vapi.
							       lcr << 8);
	  vapi_send (uart->vapi_id, data);
	}
      else
	{
	  char buffer[1] = { uart->iregs.txser & 0xFF };
	  channel_write (uart->channel, buffer, 1);
	}
    }
}

/* Called when all the bits have been shifted out of the shift register */
void
uart_char_clock (void *dat)
{
  struct dev_16450 *uart = dat;

  /* We've sent all bits */
  send_char (uart, (uart->regs.lcr & UART_LCR_WLEN8) + 5);

  if (!uart->istat.txbuf_full)
    uart->regs.lsr |= UART_LSR_TXSERE;
  else
    uart_tx_send (uart);
}

/* Called when a break has been shifted out of the shift register */
void
uart_send_break (void *dat)
{
  struct dev_16450 *uart = dat;

  /* Send one break signal */
  vapi_send (uart->vapi_id, UART_LCR_SBC << 8);

  /* Send the next char (if there is one) */
  if (!uart->istat.txbuf_full)
    uart->regs.lsr |= UART_LSR_TXSERE;
  else
    uart_tx_send (uart);
}

/* Scheduled whenever the TX buffer has characters in it and we aren't sending
 * a character. */
void
uart_tx_send (void *dat)
{
  struct dev_16450 *uart = dat;

  uart->iregs.txser = uart->regs.txbuf[uart->istat.txbuf_tail];
  uart->istat.txbuf_tail = (uart->istat.txbuf_tail + 1) % uart->fifo_len;
  uart->istat.txbuf_full--;
  uart->regs.lsr &= ~UART_LSR_TXSERE;

  /* Schedules a char_clock to run in the correct amount of time */
  if (!(uart->regs.lcr & UART_LCR_SBC))
    {
      SCHED_ADD (uart_char_clock, uart, uart->char_clks * UART_CLOCK_DIVIDER);
    }
  else
    {
      SCHED_ADD (uart_send_break, uart, 0);
    }

  /* When UART is in either character mode, i.e. 16450 emulation mode, or FIFO
   * mode, the THRE interrupt is raised when THR transitions from full to empty.
   */
  if (!uart->istat.txbuf_full)
    {
      uart->regs.lsr |= UART_LSR_TXBUFE;
      uart_int_thri (uart);
    }
}

/*-------------------------------------------------------[ Receiver logic ]---*/
/* Adds a character to the RX FIFO */
static void
uart_add_char (struct dev_16450 *uart, int ch)
{
  uart->regs.lsr |= UART_LSR_RDRDY;
  uart_clear_int (uart, UART_IIR_CTI);
  SCHED_ADD (uart_int_cti, uart,
	     uart->char_clks * UART_CHAR_TIMEOUT * UART_CLOCK_DIVIDER);

  if (uart->istat.rxbuf_full + 1 > uart->fifo_len)
    {
      uart->regs.lsr |= UART_LSR_OVRRUN | UART_LSR_RXERR;
      uart_int_rlsi (uart);
    }
  else
    {
      uart->regs.rxbuf[uart->istat.rxbuf_head] = ch;
      uart->istat.rxbuf_head = (uart->istat.rxbuf_head + 1) % uart->fifo_len;
      if (!uart->istat.rxbuf_full++)
	{
	  uart->regs.lsr |= ch >> 8;
	  uart_check_rlsi (uart);
	}
    }
  uart_check_rdi (uart);
}

/* Called when a break sequence is about to start.  It stops receiveing
 * characters and schedules the uart_recv_break to send the break */
static void
uart_recv_break_start (void *dat)
{
  struct dev_16450 *uart = dat;

  uart->istat.receiveing = 0;
  uart->istat.recv_break = 1;

  SCHED_FIND_REMOVE (uart_recv_char, uart);

  if (uart->vapi_id && (uart->vapi_buf_head_ptr != uart->vapi_buf_tail_ptr))
    uart_vapi_cmd (uart);

  SCHED_ADD (uart_recv_break, uart,
	     UART_BREAK_COUNT * uart->vapi.char_clks * UART_CLOCK_DIVIDER);
}

/* Stops sending breaks and starts receiveing characters */
static void
uart_recv_break_stop (void *dat)
{
  struct dev_16450 *uart = dat;

  uart->istat.recv_break = 0;
  SCHED_FIND_REMOVE (uart_recv_break, dat);
}

/* Receives a break */
static void
uart_recv_break (void *dat)
{
  struct dev_16450 *uart = dat;
  unsigned lsr = UART_LSR_BREAK | UART_LSR_RXERR | UART_LSR_RDRDY;

  uart_add_char (uart, lsr << 8);
}

/* Moves a character from the serial register to the RX FIFO */
static void
uart_recv_char (void *dat)
{
  struct dev_16450 *uart = dat;
  uint16_t char_to_add;

  /* Set unused character bits to zero and allow lsr register in fifo */
  char_to_add =
    uart->iregs.rxser & (((1 << ((uart->regs.lcr & 3) + 5)) - 1) | 0xff00);

  PRINTF ("%c", (char) char_to_add);

  if (uart->regs.mcr & UART_MCR_LOOP)
    {
      uart->iregs.rxser = uart->iregs.loopback;
      uart->istat.receiveing = 1;
      SCHED_ADD (uart_recv_char, uart, uart->char_clks * UART_CLOCK_DIVIDER);
    }
  else
    {
      uart->istat.receiveing = 0;
      uart_sched_recv_check (uart);
      if (uart->vapi_id
	  && (uart->vapi_buf_head_ptr != uart->vapi_buf_tail_ptr))
	SCHED_ADD (uart_vapi_cmd, uart, 0);
    }

  uart_add_char (uart, char_to_add);
}

/* Checks if there is a character waiting to be received */
static void
uart_check_char (void *dat)
{
  struct dev_16450 *uart = dat;
  uint8_t buffer;
  int retval;

  /* Check if there is something waiting, and put it into rxser */
  retval = channel_read (uart->channel, (char *) &buffer, 1);
  if (retval > 0)
    {
      uart->iregs.rxser = buffer;
      uart->istat.receiveing = 1;
      SCHED_ADD (uart_recv_char, uart, uart->char_clks * UART_CLOCK_DIVIDER);
      return;
    }

  if (!retval)
    {
      SCHED_ADD (uart_check_char, uart,
		 UART_FGETC_SLOWDOWN * UART_CLOCK_DIVIDER);
      return;
    }

  if (retval < 0)
    perror (uart->channel_str);
}

static void
uart_sched_recv_check (struct dev_16450 *uart)
{
  if (!uart->vapi_id)
    SCHED_ADD (uart_check_char, uart,
	       UART_FGETC_SLOWDOWN * UART_CLOCK_DIVIDER);
}

/*----------------------------------------------------[ UART I/O handling ]---*/
/* Set a specific UART register with value. */
static void
uart_write_byte (oraddr_t addr, uint8_t value, void *dat)
{
  struct dev_16450 *uart = dat;

  if (uart->regs.lcr & UART_LCR_DLAB)
    {
      switch (addr)
	{
	case UART_DLL:
	  uart->regs.dll = value;
	  uart->char_clks =
	    char_clks (uart->regs.dll, uart->regs.dlh, uart->regs.lcr);
	  return;
	case UART_DLH:
	  uart->regs.dlh = value;
	  return;
	}
    }

  switch (addr)
    {
    case UART_TXBUF:
      uart->regs.lsr &= ~UART_LSR_TXBUFE;
      if (uart->istat.txbuf_full < uart->fifo_len)
	{
	  uart->regs.txbuf[uart->istat.txbuf_head] = value;
	  uart->istat.txbuf_head =
	    (uart->istat.txbuf_head + 1) % uart->fifo_len;
	  if (!uart->istat.txbuf_full++ && (uart->regs.lsr & UART_LSR_TXSERE))
	    SCHED_ADD (uart_tx_send, uart, 0);
	}
      else
	uart->regs.txbuf[uart->istat.txbuf_head] = value;

      uart_clear_int (uart, UART_IIR_THRI);
      break;
    case UART_FCR:
      uart->regs.fcr = value & UART_VALID_FCR;
      if ((uart->fifo_len == 1 && (value & UART_FCR_FIE))
	  || (uart->fifo_len != 1 && !(value & UART_FCR_FIE)))
	value |= UART_FCR_RRXFI | UART_FCR_RTXFI;
      uart->fifo_len = (value & UART_FCR_FIE) ? 16 : 1;
      if (value & UART_FCR_RTXFI)
	{
	  uart->istat.txbuf_head = uart->istat.txbuf_tail = 0;
	  uart->istat.txbuf_full = 0;
	  uart->regs.lsr |= UART_LSR_TXBUFE;

	  /* For FIFO-mode only, THRE interrupt is set when THR and FIFO are empty
	   */
	  if (uart->fifo_len == 16)
	    uart_int_thri (uart);

	  SCHED_FIND_REMOVE (uart_tx_send, uart);
	}
      if (value & UART_FCR_RRXFI)
	{
	  uart->istat.rxbuf_head = uart->istat.rxbuf_tail = 0;
	  uart->istat.rxbuf_full = 0;
	  uart->regs.lsr &= ~UART_LSR_RDRDY;
	  uart_clear_int (uart, UART_IIR_RDI);
	  uart_clear_int (uart, UART_IIR_CTI);
	}
      break;
    case UART_IER:
      uart->regs.ier = value & UART_VALID_IER;
      uart_next_int (uart);
      break;
    case UART_LCR:
      if ((uart->regs.lcr & UART_LCR_SBC) != (value & UART_LCR_SBC))
	{
	  if ((value & UART_LCR_SBC) && !(uart->regs.lsr & UART_LSR_TXSERE))
	    {
	      /* Schedule a job to send the break char */
	      SCHED_FIND_REMOVE (uart_char_clock, uart);
	      SCHED_ADD (uart_send_break, uart, 0);
	    }
	  if (!(value & UART_LCR_SBC) && !(uart->regs.lsr & UART_LSR_TXSERE))
	    {
	      /* Schedule a job to start sending characters */
	      SCHED_ADD (uart_tx_send, uart, 0);
	      /* Remove the uart_send_break job just in case it has not run yet */
	      SCHED_FIND_REMOVE (uart_char_clock, uart);
	    }
	}
      uart->regs.lcr = value & UART_VALID_LCR;
      uart->char_clks =
	char_clks (uart->regs.dll, uart->regs.dlh, uart->regs.lcr);
      break;
    case UART_MCR:
      uart->regs.mcr = value & UART_VALID_MCR;
      uart_loopback (uart);
      break;
    case UART_SCR:
      uart->regs.scr = value;
      break;
    }
}

/* Read a specific UART register. */
static uint8_t
uart_read_byte (oraddr_t addr, void *dat)
{
  struct dev_16450 *uart = dat;
  uint8_t value = 0;

  if (uart->regs.lcr & UART_LCR_DLAB)
    {
      switch (addr)
	{
	case UART_DLL:
	  value = uart->regs.dll;
	  return value;
	case UART_DLH:
	  value = uart->regs.dlh;
	  return value;
	}
    }

  switch (addr)
    {
    case UART_RXBUF:
      if (uart->istat.rxbuf_full)
	{
	  value = uart->regs.rxbuf[uart->istat.rxbuf_tail];
	  uart->istat.rxbuf_tail =
	    (uart->istat.rxbuf_tail + 1) % uart->fifo_len;
	  uart->istat.rxbuf_full--;
	}

      uart_clear_int (uart, UART_IIR_RDI);
      uart_clear_int (uart, UART_IIR_CTI);

      if (uart->istat.rxbuf_full)
	{
	  uart->regs.lsr |=
	    UART_LSR_RDRDY | uart->regs.rxbuf[uart->istat.rxbuf_tail] >> 8;
	  SCHED_ADD (uart_int_cti, uart,
		     uart->char_clks * UART_CHAR_TIMEOUT *
		     UART_CLOCK_DIVIDER);
	  uart_check_rlsi (uart);
	  uart_check_rdi (uart);
	}
      else
	{
	  uart->regs.lsr &= ~UART_LSR_RDRDY;
	}
      break;
    case UART_IER:
      value = uart->regs.ier & UART_VALID_IER;
      break;
    case UART_IIR:
      value = (uart->regs.iir & UART_VALID_IIR) | 0xc0;
      /* Only clear the thri interrupt if it is the one we are repporting */
      if (uart->regs.iir == UART_IIR_THRI)
	uart_clear_int (uart, UART_IIR_THRI);
      break;
    case UART_LCR:
      value = uart->regs.lcr & UART_VALID_LCR;
      break;
    case UART_MCR:
      value = 0;
      break;
    case UART_LSR:
      value = uart->regs.lsr & UART_VALID_LSR;
      uart->regs.lsr &=
	~(UART_LSR_OVRRUN | UART_LSR_BREAK | UART_LSR_PARITY
	  | UART_LSR_FRAME | UART_LSR_RXERR);
      /* Clear potentially pending RLSI interrupt */
      uart_clear_int (uart, UART_IIR_RLSI);
      break;
    case UART_MSR:
      value = uart->regs.msr & UART_VALID_MSR;
      uart->regs.msr = 0;
      uart_clear_int (uart, UART_IIR_MSI);
      uart_loopback (uart);
      break;
    case UART_SCR:
      value = uart->regs.scr;
      break;
    }
  return value;
}

/*--------------------------------------------------------[ VAPI handling ]---*/
/* Decodes the read vapi command */
static void
uart_vapi_cmd (void *dat)
{
  struct dev_16450 *uart = dat;
  int received = 0;

  while (!received)
    {
      if (uart->vapi_buf_head_ptr != uart->vapi_buf_tail_ptr)
	{
	  unsigned long data = uart->vapi_buf[uart->vapi_buf_tail_ptr];
	  uart->vapi_buf_tail_ptr =
	    (uart->vapi_buf_tail_ptr + 1) % UART_VAPI_BUF_LEN;
	  switch (data >> 24)
	    {
	    case 0x00:
	      uart->vapi.lcr = (data >> 8) & 0xff;
	      /* Put data into rx fifo */
	      uart->iregs.rxser = data & 0xff;
	      uart->vapi.char_clks =
		char_clks (uart->vapi.dll, uart->vapi.dlh, uart->vapi.lcr);
	      if ((uart->vapi.lcr & ~UART_LCR_SBC) !=
		  (uart->regs.lcr & ~UART_LCR_SBC)
		  || uart->vapi.char_clks != uart->char_clks
		  || uart->vapi.skew < -MAX_SKEW
		  || uart->vapi.skew > MAX_SKEW)
		{
		  if ((uart->vapi.lcr & ~UART_LCR_SBC) !=
		      (uart->regs.lcr & ~UART_LCR_SBC))
		    fprintf (stderr, "Warning: Unmatched VAPI (%02" PRIx8
			     ") and uart (%02" PRIx8
			     ") modes.\n", uart->vapi.lcr & ~UART_LCR_SBC,
			     uart->regs.lcr & ~UART_LCR_SBC);
		  if (uart->vapi.char_clks != uart->char_clks)
		    {
		      fprintf (stderr, "Warning: Unmatched VAPI (%li) and "
			       "UART (%li) char clocks.\n",
			       uart->vapi.char_clks, uart->char_clks);
		      fprintf (stderr, "VAPI: lcr: %02" PRIx8 ", dll: %02"
			       PRIx8 ", dlh: %02" PRIx8 "\n", uart->vapi.lcr,
			       uart->vapi.dll, uart->vapi.dlh);
		      fprintf (stderr, "UART: lcr: %02" PRIx8 ", dll: %02"
			       PRIx8 ", dlh: %02" PRIx8 "\n", uart->regs.lcr,
			       uart->regs.dll, uart->vapi.dlh);
		    }
		  if (uart->vapi.skew < -MAX_SKEW
		      || uart->vapi.skew > MAX_SKEW)
		    {
		      fprintf(stderr, "VAPI skew is beyond max: %i\n",
			      uart->vapi.skew);
		    }
		  /* Set error bits */
		  uart->iregs.rxser |= (UART_LSR_FRAME | UART_LSR_RXERR) << 8;
		  if (uart->regs.lcr & UART_LCR_PARITY)
		    uart->iregs.rxser |= UART_LSR_PARITY << 8;
		}
	      if (!uart->istat.recv_break)
		{
		  uart->istat.receiveing = 1;
		  SCHED_ADD (uart_recv_char, uart,
			     uart->char_clks * UART_CLOCK_DIVIDER);
		}
	      received = 1;
	      break;
	    case 0x01:
	      uart->vapi.dll = (data >> 0) & 0xff;
	      uart->vapi.dlh = (data >> 8) & 0xff;
	      break;
	    case 0x02:
	      uart->vapi.lcr = (data >> 8) & 0xff;
	      break;
	    case 0x03:
	      uart->vapi.skew = (signed short) (data & 0xffff);
	      break;
	    case 0x04:
	      if ((data >> 16) & 1)
		{
		  /* If data & 0xffff is 0 then set the break imediatly and handle the
		   * following commands as appropriate */
		  if (!(data & 0xffff))
		    uart_recv_break_start (uart);
		  else
		    /* Schedule a job to start sending breaks */
		    SCHED_ADD (uart_recv_break_start, uart,
			       (data & 0xffff) * UART_CLOCK_DIVIDER);
		}
	      else
		{
		  /* If data & 0xffff is 0 then release the break imediatly and handle
		   * the following commands as appropriate */
		  if (!(data & 0xffff))
		    uart_recv_break_stop (uart);
		  else
		    /* Schedule a job to stop sending breaks */
		    SCHED_ADD (uart_recv_break_stop, uart,
			       (data & 0xffff) * UART_CLOCK_DIVIDER);
		}
	      break;
	    default:
	      fprintf (stderr, "WARNING: Invalid vapi command %02lx\n",
		       data >> 24);
	      break;
	    }
	}
      else
	break;
    }
}

/* Function that handles incoming VAPI data.  */
static void
uart_vapi_read (unsigned long id, unsigned long data, void *dat)
{
  struct dev_16450 *uart = dat;
  uart->vapi_buf[uart->vapi_buf_head_ptr] = data;
  uart->vapi_buf_head_ptr = (uart->vapi_buf_head_ptr + 1) % UART_VAPI_BUF_LEN;
  if (uart->vapi_buf_tail_ptr == uart->vapi_buf_head_ptr)
    {
      fprintf (stderr, "FATAL: uart VAPI buffer to small.\n");
      exit (1);
    }
  if (!uart->istat.receiveing)
    uart_vapi_cmd (uart);
}

/*--------------------------------------------------------[ Sim callbacks ]---*/
/* Reset.  It initializes all registers of all UART devices to zero values,
 * (re)opens all RX/TX file streams and places devices in memory address
 * space.  */
void
uart_reset (void *dat)
{
  struct dev_16450 *uart = dat;

  if (uart->vapi_id)
    {
      vapi_install_handler (uart->vapi_id, uart_vapi_read, dat);
    }
  else if (uart->channel_str && uart->channel_str[0])
    {				/* Try to create stream. */
      if (uart->channel)
	channel_close (uart->channel);
      else
	uart->channel = channel_init (uart->channel_str);
      if (channel_open (uart->channel) < 0)
	{
	  fprintf (stderr, "Warning: problem with channel \"%s\" detected.\n",
		   uart->channel_str);
	}
      else if (config.sim.verbose)
	PRINTF ("UART at 0x%" PRIxADDR "\n", uart->baseaddr);
    }
  else
    {
      fprintf (stderr, "Warning: UART at %" PRIxADDR
	       " has no vapi nor channel specified\n", uart->baseaddr);
    }

  if (uart->uart16550)
    uart->fifo_len = 16;
  else
    uart->fifo_len = 1;

  uart->istat.rxbuf_head = uart->istat.rxbuf_tail = 0;
  uart->istat.txbuf_head = uart->istat.txbuf_tail = 0;

  uart->istat.txbuf_full = uart->istat.rxbuf_full = 0;

  uart->char_clks = 0;

  uart->iregs.txser = 0;
  uart->iregs.rxser = 0;
  uart->iregs.loopback = 0;
  uart->istat.receiveing = 0;
  uart->istat.recv_break = 0;
  uart->istat.ints = 0;

  memset (uart->regs.txbuf, 0, sizeof (uart->regs.txbuf));
  memset (uart->regs.rxbuf, 0, sizeof (uart->regs.rxbuf));

  uart->regs.dll = 0;
  uart->regs.dlh = 0;
  uart->regs.ier = 0;
  uart->regs.iir = UART_IIR_NO_INT;
  uart->regs.fcr = 0;
  uart->regs.lcr = UART_LCR_RESET;
  uart->regs.mcr = 0;
  uart->regs.lsr = UART_LSR_TXBUFE | UART_LSR_TXSERE;
  uart->regs.msr = 0;
  uart->regs.scr = 0;

  uart->vapi.skew = 0;
  uart->vapi.lcr = 0;
  uart->vapi.dll = 0;
  uart->vapi.char_clks = 0;

  uart->vapi_buf_head_ptr = 0;
  uart->vapi_buf_tail_ptr = 0;
  memset (uart->vapi_buf, 0, sizeof (uart->vapi_buf));

  uart_sched_recv_check (uart);
}

/* Print register values on stdout. */
void
uart_status (void *dat)
{
  struct dev_16450 *uart = dat;
  int i;

  PRINTF ("\nUART visible registers at 0x%" PRIxADDR ":\n", uart->baseaddr);
  PRINTF ("RXBUF: ");
  for (i = uart->istat.rxbuf_head; i != uart->istat.rxbuf_tail;
       i = (i + 1) % uart->fifo_len)
    PRINTF (" %.2x", uart->regs.rxbuf[i]);
  PRINTF ("TXBUF: ");
  for (i = uart->istat.txbuf_head; i != uart->istat.txbuf_tail;
       i = (i + 1) % uart->fifo_len)
    PRINTF (" %.2x", uart->regs.txbuf[i]);
  PRINTF ("\n");
  PRINTF ("DLL  : %.2x  DLH  : %.2x\n", uart->regs.dll, uart->regs.dlh);
  PRINTF ("IER  : %.2x  IIR  : %.2x\n", uart->regs.ier, uart->regs.iir);
  PRINTF ("LCR  : %.2x  MCR  : %.2x\n", uart->regs.lcr, uart->regs.mcr);
  PRINTF ("LSR  : %.2x  MSR  : %.2x\n", uart->regs.lsr, uart->regs.msr);
  PRINTF ("SCR  : %.2x\n", uart->regs.scr);

  PRINTF ("\nInternal registers (sim debug):\n");
  PRINTF ("RXSER: %.2" PRIx16 "  TXSER: %.2" PRIx8 "\n", uart->iregs.rxser,
	  uart->iregs.txser);

  PRINTF ("\nInternal status (sim debug):\n");
  PRINTF ("char_clks: %ld\n", uart->char_clks);
  PRINTF ("rxbuf_full: %d  txbuf_full: %d\n", uart->istat.rxbuf_full,
	  uart->istat.txbuf_full);
  PRINTF ("Using IRQ%i\n", uart->irq);
  if (uart->vapi_id)
    PRINTF ("Connected to vapi ID=%lx\n\n", uart->vapi_id);
  /* TODO: replace by a channel_status
     else
     PRINTF("RX fs: %p  TX fs: %p\n\n", uart->rxfs, uart->txfs);
   */
}

/*---------------------------------------------------[ UART configuration ]---*/
static void
uart_baseaddr (union param_val val, void *dat)
{
  struct dev_16450 *uart = dat;
  uart->baseaddr = val.addr_val;
}

static void
uart_jitter (union param_val val, void *dat)
{
  struct dev_16450 *uart = dat;
  uart->jitter = val.int_val;
}

static void
uart_irq (union param_val val, void *dat)
{
  struct dev_16450 *uart = dat;
  uart->irq = val.int_val;
}

static void
uart_16550 (union param_val val, void *dat)
{
  struct dev_16450 *uart = dat;
  uart->uart16550 = val.int_val;
}

/*---------------------------------------------------------------------------*/
/*!Set the channel description

   Free any existing string.

   @param[in] val  The value to use
   @param[in] dat  The config data structure                                 */
/*---------------------------------------------------------------------------*/
static void
uart_channel (union param_val  val,
	      void            *dat)
{
  struct dev_16450 *uart = dat;

  if (NULL != uart->channel_str)
    {
      free (uart->channel_str);
      uart->channel_str = NULL;
    }

  if (!(uart->channel_str = strdup (val.str_val)))
    {
      fprintf (stderr, "Peripheral 16450: Run out of memory\n");
      exit (-1);
    }

}	/* uart_channel() */


static void
uart_newway (union param_val val, void *dat)
{
  fprintf (stderr, "Warning: txfile and rxfile now obsolete and ignored. "
	   "Use 'channel = \"file:rxfile,txfile\"'.");
}

static void
uart_vapi_id (union param_val val, void *dat)
{
  struct dev_16450 *uart = dat;
  uart->vapi_id = val.int_val;
}

static void
uart_enabled (union param_val val, void *dat)
{
  struct dev_16450 *uart = dat;
  uart->enabled = val.int_val;
}


/*---------------------------------------------------------------------------*/
/*!Initialize a new UART configuration

   ALL parameters are set explicitly to default values.                      */
/*---------------------------------------------------------------------------*/
static void *
uart_sec_start ()
{
  struct dev_16450 *new = malloc (sizeof (struct dev_16450));

  if (!new)
    {
      fprintf (stderr, "Peripheral 16450: Run out of memory\n");
      exit (-1);
    }

  new->enabled = 1;
  new->baseaddr = 0;
  new->irq = 0;
  new->uart16550 = 0;
  new->jitter = 0;
  new->channel_str = strdup ("xterm:");
  new->vapi_id = 0;

  new->channel = NULL;

  return new;

}	/* uart_sec_start() */


static void
uart_sec_end (void *dat)
{
  struct dev_16450 *uart = dat;
  struct mem_ops ops;

  if (!uart->enabled)
    {
      free (dat);
      return;
    }

  memset (&ops, 0, sizeof (struct mem_ops));

  ops.readfunc8 = uart_read_byte;
  ops.writefunc8 = uart_write_byte;
  ops.read_dat8 = dat;
  ops.write_dat8 = dat;

  /* FIXME: What should these be? */
  ops.delayr = 2;
  ops.delayw = 2;

  reg_mem_area (uart->baseaddr, UART_ADDR_SPACE, 0, &ops);

  reg_sim_reset (uart_reset, dat);
  reg_sim_stat (uart_status, dat);
}

void
reg_uart_sec (void)
{
  struct config_section *sec = reg_config_sec ("uart", uart_sec_start,
					       uart_sec_end);

  reg_config_param (sec, "enabled",  PARAMT_INT, uart_enabled);
  reg_config_param (sec, "baseaddr", PARAMT_ADDR, uart_baseaddr);
  reg_config_param (sec, "irq",      PARAMT_INT, uart_irq);
  reg_config_param (sec, "16550",    PARAMT_INT, uart_16550);
  reg_config_param (sec, "jitter",   PARAMT_INT, uart_jitter);
  reg_config_param (sec, "channel",  PARAMT_STR, uart_channel);
  reg_config_param (sec, "txfile",   PARAMT_STR, uart_newway);
  reg_config_param (sec, "rxfile",   PARAMT_STR, uart_newway);
  reg_config_param (sec, "vapi_id",  PARAMT_INT, uart_vapi_id);
}
