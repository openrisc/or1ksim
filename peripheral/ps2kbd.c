/* ps2kbd.c -- Very simple (and limited) PS/2 keyboard simulation

   Copyright (C) 2002 Marko Mlinar, markom@opencores.org
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


/* Autoconf and/or portability configuration */
#include "config.h"
#include "port.h"

/* System includes */
#include <stdlib.h>
#include <stdio.h>

/* Package includes */
#include "arch.h"
#include "pic.h"
#include "sim-config.h"
#include "abstract.h"
#include "sched.h"
#include "toplevel-support.h"
#include "sim-cmd.h"


/* Device registers */
#define KBD_CTRL              4
#define KBD_DATA              0
#define KBD_SPACE             8

/* Keyboard commands */
#define KBD_KCMD_RST       0xFF
#define KBD_KCMD_DK        0xF5
#define KBD_KCMD_EK        0xF4
#define KBD_KCMD_ECHO      0xFF
#define KBD_KCMD_SRL       0xED

/* Keyboard responses */
#define KBD_KRESP_RSTOK    0xAA
#define KBD_KRESP_ECHO     0xEE
#define KBD_KRESP_ACK      0xFA

/* Controller commands */
#define KBD_CCMD_RCB       0x20
#define KBD_CCMD_WCB       0x60
#define KBD_CCMD_ST1       0xAA
#define KBD_CCMD_ST2       0xAB
#define KBD_CCMD_DKI       0xAD
#define KBD_CCMD_EKI       0xAE

/* Status register bits */
#define KBD_STATUS_OBF     0x01
#define KBD_STATUS_IBF     0x02
#define KBD_STATUS_SYS     0x04
#define KBD_STATUS_A2      0x08
#define KBD_STATUS_INH     0x10
#define KBD_STATUS_MOBF    0x20
#define KBD_STATUS_TO      0x40
#define KBD_STATUS_PERR    0x80

/* Command byte register bits */
#define KBD_CCMDBYTE_INT   0x01
#define KBD_CCMDBYTE_INT2  0x02
#define KBD_CCMDBYTE_SYS   0x04
#define KBD_CCMDBYTE_EN    0x10
#define KBD_CCMDBYTE_EN2   0x20
#define KBD_CCMDBYTE_XLAT  0x40

/* Length of internal scan code fifo */
#define KBD_MAX_BUF       0x100

/* Keyboard is checked every KBD_SLOWDOWN cycle */
#define KBD_BAUD_RATE      1200

/* ASCII to scan code conversion table */
const static struct
{
  /* Whether shift must be pressed */
  unsigned char shift;
  /* Scan code to be generated */
  unsigned char code;
} scan_table[128] =
{
/* 0 - 15 */
  {
  0, 0x00},
  {
  0, 0x00},
  {
  0, 0x00},
  {
  0, 0x00},
  {
  0, 0x00},
  {
  0, 0x00},
  {
  0, 0x00},
  {
  0, 0x00},
  {
  0, 0x0E},
  {
  0, 0x0F},
  {
  0, 0x1C},
  {
  0, 0x00},
  {
  0, 0x00},
  {
  0, 0x00},
  {
  0, 0x00},
  {
  0, 0x00},
/* 16 - 31 */
  {
  0, 0x00},
  {
  0, 0x00},
  {
  0, 0x00},
  {
  0, 0x00},
  {
  0, 0x00},
  {
  0, 0x00},
  {
  0, 0x00},
  {
  0, 0x00},
  {
  0, 0x00},
  {
  0, 0x00},
  {
  0, 0x00},
  {
  0, 0x01},
  {
  0, 0x00},
  {
  0, 0x00},
  {
  0, 0x00},
  {
  0, 0x00},
/* 32 - 47 */
  {
  0, 0x39},
  {
  1, 0x02},
  {
  1, 0x28},
  {
  1, 0x04},
  {
  1, 0x05},
  {
  1, 0x06},
  {
  1, 0x08},
  {
  0, 0x28},
  {
  1, 0x0A},
  {
  1, 0x0B},
  {
  1, 0x09},
  {
  1, 0x0D},
  {
  0, 0x33},
  {
  0, 0x0C},
  {
  0, 0x34},
  {
  0, 0x35},
/* 48 - 63 */
  {
  0, 0x0B},
  {
  0, 0x02},
  {
  0, 0x03},
  {
  0, 0x04},
  {
  0, 0x05},
  {
  0, 0x06},
  {
  0, 0x07},
  {
  0, 0x08},
  {
  0, 0x09},
  {
  0, 0x0A},
  {
  1, 0x27},
  {
  0, 0x27},
  {
  1, 0x33},
  {
  0, 0x0D},
  {
  1, 0x34},
  {
  1, 0x35},
/* 64 - 79 */
  {
  1, 0x03},
  {
  1, 0x1E},
  {
  1, 0x30},
  {
  1, 0x2E},
  {
  1, 0x20},
  {
  1, 0x12},
  {
  1, 0x21},
  {
  1, 0x22},
  {
  1, 0x23},
  {
  1, 0x17},
  {
  1, 0x24},
  {
  1, 0x25},
  {
  1, 0x26},
  {
  1, 0x32},
  {
  1, 0x31},
  {
  1, 0x18},
/* 80 - 95 */
  {
  1, 0x19},
  {
  1, 0x10},
  {
  1, 0x13},
  {
  1, 0x1F},
  {
  1, 0x14},
  {
  1, 0x16},
  {
  1, 0x2F},
  {
  1, 0x11},
  {
  1, 0x2D},
  {
  1, 0x15},
  {
  1, 0x2C},
  {
  0, 0x1A},
  {
  0, 0x2B},
  {
  0, 0x1B},
  {
  1, 0x07},
  {
  1, 0x0C},
/* 96 - 111 */
  {
  0, 0x29},
  {
  0, 0x1E},
  {
  0, 0x30},
  {
  0, 0x2E},
  {
  0, 0x20},
  {
  0, 0x12},
  {
  0, 0x21},
  {
  0, 0x22},
  {
  0, 0x23},
  {
  0, 0x17},
  {
  0, 0x24},
  {
  0, 0x25},
  {
  0, 0x26},
  {
  0, 0x32},
  {
  0, 0x31},
  {
  0, 0x18},
/* 112 - 127 */
  {
  0, 0x19},
  {
  0, 0x10},
  {
  0, 0x13},
  {
  0, 0x1F},
  {
  0, 0x14},
  {
  0, 0x16},
  {
  0, 0x2F},
  {
  0, 0x11},
  {
  0, 0x2D},
  {
  0, 0x15},
  {
  0, 0x2C},
  {
  1, 0x1A},
  {
  1, 0x2B},
  {
  1, 0x1B},
  {
  1, 0x29},
  {
  0, 0x00}
};

struct kbd_state
{
  /* Temporary buffer to store incoming scan codes */
  uint8_t buf[KBD_MAX_BUF];

  /* Number of scan codes in buffer */
  unsigned long buf_count;
  unsigned long buf_head;
  unsigned long buf_tail;

  /* Input stream */
  FILE *rxfs;

  /* Controller Command (write into 0x64) */
  int ccmd;

  /* Keyboard Command (write into 0x60) */
  uint8_t kcmd;

  /* Controller Command Byte */
  uint8_t ccmdbyte;

  /* Keyboard response pending */
  unsigned long kresp;

  /* Keyboard slowdown factor */
  long slowdown;

  /* Cofiguration */
  int enabled;
  int irq;
  oraddr_t baseaddr;
  char *rxfile;
};

static void
kbd_put (struct kbd_state *kbd, unsigned char c)
{
  if (kbd->buf_count >= KBD_MAX_BUF)
    {
      fprintf (stderr, "WARNING: Keyboard buffer overflow.\n");
    }
  else
    {
      kbd->buf[kbd->buf_head] = c;
      kbd->buf_head = (kbd->buf_head + 1) % KBD_MAX_BUF;
      kbd->buf_count++;
    }
}

/* Decodes ascii code c into multiple scan codes, placed into buf, length is returned */
static void
scan_decode (struct kbd_state *kbd, unsigned char c)
{
  /* Do not handle special characters and extended ascii */
  if (c >= 128 || !scan_table[c].code)
    return;

  /* Make shift? */
  if (scan_table[c].shift)
    kbd_put (kbd, 0x2a);
  /* Make char */
  kbd_put (kbd, scan_table[c].code);
  /* Break char */
  kbd_put (kbd, scan_table[c].code | 0x80);
  /* Break shift? */
  if (scan_table[c].shift)
    kbd_put (kbd, 0xaa);
}

/* Write a register */
static void
kbd_write8 (oraddr_t addr, uint8_t value, void *dat)
{
  struct kbd_state *kbd = dat;
  switch (addr)
    {
    case KBD_CTRL:
      kbd->ccmd = value & 0xff;
      if (kbd->ccmd == KBD_CCMD_RCB)
	kbd->kresp = 0x1;
      if (kbd->ccmd == KBD_CCMD_ST1)
	kbd->kresp = 0x1;
      if (kbd->ccmd == KBD_CCMD_ST2)
	kbd->kresp = 0x1;
      if (kbd->ccmd == KBD_CCMD_DKI)
	{
	  clear_interrupt (kbd->irq);
	  kbd->ccmdbyte |= KBD_CCMDBYTE_EN;
	}
      if (kbd->ccmd == KBD_CCMD_EKI)
	kbd->ccmdbyte &= ~KBD_CCMDBYTE_EN;
      if (config.sim.verbose)
	PRINTF ("kbd_write8(%" PRIxADDR ") %02x\n", addr, value);
      break;
    case KBD_DATA:
      if (kbd->ccmd == KBD_CCMD_WCB)
	{
	  kbd->ccmdbyte = value & 0xff;
	  kbd->ccmd = 0x00;
	}
      else
	kbd->kcmd = value & 0xff;
      if (kbd->kcmd == KBD_KCMD_DK)
	kbd->ccmdbyte |= KBD_CCMDBYTE_EN;
      if (kbd->kcmd == KBD_KCMD_EK)
	kbd->ccmdbyte &= ~KBD_CCMDBYTE_EN;
      kbd->kresp = 0x1;
      kbd->ccmd = 0x00;
      if (config.sim.verbose)
	PRINTF ("kbd_write8(%" PRIxADDR ") %02x\n", addr, value);
      break;
    default:
      fprintf (stderr, "Write out of keyboard space (0x%" PRIxADDR ")!\n",
	       addr);
      break;
    }
}

/* Read a register */
static uint8_t
kbd_read8 (oraddr_t addr, void *dat)
{
  struct kbd_state *kbd = dat;
  switch (addr)
    {
    case KBD_CTRL:
      {
	unsigned long c = 0x0;
	if (kbd->kresp || kbd->buf_count)
	  c |= KBD_STATUS_OBF;
	c |= kbd->ccmdbyte & KBD_CCMDBYTE_SYS;
	c |= KBD_STATUS_INH;
	if (config.sim.verbose)
	  PRINTF ("kbd_read8(%" PRIxADDR ") %lx\n", addr, c);
	return c;
      }
    case KBD_DATA:
      clear_interrupt (kbd->irq);
      if (kbd->ccmd)
	{
	  unsigned long rc = 0;
	  if (kbd->ccmd == KBD_CCMD_RCB)
	    rc = kbd->ccmdbyte;
	  if (kbd->ccmd == KBD_CCMD_ST1)
	    rc = 0x55;
	  if (kbd->ccmd == KBD_CCMD_ST2)
	    rc = 0x00;
	  kbd->ccmd = 0x00;
	  kbd->kresp = 0x0;
	  if (config.sim.verbose)
	    PRINTF ("kbd_read8(%" PRIxADDR ") %lx\n", addr, rc);
	  return rc;
	}
      else if (kbd->kresp)
	{
	  unsigned long rc;
	  if (kbd->kresp == 0x2)
	    {
	      kbd->kresp = 0x0;
	      rc = KBD_KRESP_RSTOK;
	    }
	  else if (kbd->kcmd == KBD_KCMD_RST)
	    {
	      kbd->kresp = 0x2;
	      rc = KBD_KRESP_ACK;
	    }
	  else if (kbd->kcmd == KBD_KCMD_ECHO)
	    {
	      kbd->kresp = 0x0;
	      rc = KBD_KRESP_ECHO;
	    }
	  else
	    {
	      kbd->kresp = 0x0;
	      rc = KBD_KRESP_ACK;
	    }
	  kbd->kcmd = 0x00;
	  if (config.sim.verbose)
	    PRINTF ("kbd_read8(%" PRIxADDR ") %lx\n", addr, rc);
	  return rc;
	}
      else if (kbd->buf_count)
	{
	  unsigned long c = kbd->buf[kbd->buf_tail];
	  kbd->buf_tail = (kbd->buf_tail + 1) % KBD_MAX_BUF;
	  kbd->buf_count--;
	  kbd->kresp = 0x0;
	  if (config.sim.verbose)
	    PRINTF ("kbd_read8(%" PRIxADDR ") %lx\n", addr, c);
	  return c;
	}
      kbd->kresp = 0x0;
      if (config.sim.verbose)
	PRINTF ("kbd_read8(%" PRIxADDR ") fifo empty\n", addr);
      return 0;
    default:
      fprintf (stderr, "Read out of keyboard space (0x%" PRIxADDR ")!\n",
	       addr);
      return 0;
    }
}


/* Simulation hook. Must be called every couple of clock cycles to simulate incomming data. */
static void
kbd_job (void *dat)
{
  struct kbd_state *kbd = dat;
  int c;
  int kbd_int = 0;

  /* Check if there is something waiting, and decode it into kdb_buf */
  if ((c = fgetc (kbd->rxfs)) != EOF)
    {
      scan_decode (kbd, c);
    }
  kbd_int = kbd->kresp
    || kbd->buf_count ? kbd->ccmdbyte & KBD_CCMDBYTE_INT : 0;
/*
  if (config.sim.verbose && kbd_int)
    PRINTF("Keyboard Interrupt.... kbd_kresp %lx  kbd_buf_count %lx \n",
           kbd->kresp, kbd->buf_count);
*/
  if (kbd_int)
    report_interrupt (kbd->irq);
  SCHED_ADD (kbd_job, dat, kbd->slowdown);
}

/* Reset all (simulated) ps2 controlers/keyboards */
static void
kbd_reset (void *dat)
{
  struct kbd_state *kbd = dat;
  long int system_kfreq =
    (long) ((1000000000.0 / (double) config.sim.clkcycle_ps));

  system_kfreq = (system_kfreq < 1) ? 1 : system_kfreq;

  kbd->buf_count = 0;
  kbd->buf_head = 0;
  kbd->buf_tail = 0;
  kbd->kresp = 0x0;
  kbd->ccmdbyte = 0x65;		/* We reset into default normal operation. */

  if (!(kbd->rxfs = fopen (kbd->rxfile, "r"))
      && !(kbd->rxfs = fopen (kbd->rxfile, "r+")))
    {
      /* Bug 1723 fixed: Clearer message */
      fprintf (stderr,
	       "Warning: PS2 keyboard unable to open RX file stream.\n");
      return;
    }
  kbd->slowdown = (long) ((system_kfreq * 1000.0) / KBD_BAUD_RATE);
  if (kbd->slowdown <= 0)
    kbd->slowdown = 1;
  SCHED_ADD (kbd_job, dat, kbd->slowdown);
}


static void
kbd_info (void *dat)
{
  struct kbd_state *kbd = dat;
  PRINTF ("kbd_kcmd: %x\n", kbd->kcmd);
  PRINTF ("kbd_ccmd: %x\n", kbd->ccmd);
  PRINTF ("kbd_ccmdbyte: %x\n", kbd->ccmdbyte);
  PRINTF ("kbd_kresp: %lx\n", kbd->kresp);
  PRINTF ("kbd_buf_count: %lx\n", kbd->buf_count);
}

/*----------------------------------------------------[ KBD Configuration ]---*/


static void
kbd_enabled (union param_val val, void *dat)
{
  struct kbd_state *kbd = dat;
  kbd->enabled = val.int_val;
}


static void
kbd_baseaddr (union param_val val, void *dat)
{
  struct kbd_state *kbd = dat;
  kbd->baseaddr = val.addr_val;
}


static void
kbd_irq (union param_val val, void *dat)
{
  struct kbd_state *kbd = dat;
  kbd->irq = val.int_val;
}


/*---------------------------------------------------------------------------*/
/*!Set the keyboard input file

   Free any previously allocated value.

   @param[in] val  The value to use
   @param[in] dat  The config data structure                                 */
/*---------------------------------------------------------------------------*/
static void
kbd_rxfile (union param_val val, void *dat)
{
  struct kbd_state *kbd = dat;

  if (NULL != kbd->rxfile)
    {
      free (kbd->rxfile);
      kbd->rxfile = NULL;
    }

  if (!(kbd->rxfile = strdup (val.str_val)))
    {
      fprintf (stderr, "Peripheral KBD: Run out of memory\n");
      exit (-1);
    }
}				/* kbd_rxfile() */


/*---------------------------------------------------------------------------*/
/*!Initialize a new keyboard configuration

   ALL parameters are set explicitly to default values.                      */
/*---------------------------------------------------------------------------*/
static void *
kbd_sec_start ()
{
  struct kbd_state *new = malloc (sizeof (struct kbd_state));

  if (!new)
    {
      fprintf (stderr, "Peripheral KBD: Run out of memory\n");
      exit (-1);
    }

  new->enabled = 1;
  new->baseaddr = 0;
  new->irq = 0;
  new->rxfile = strdup ("kbd_in");

  new->buf_count = 0;
  new->buf_head = 0;
  new->buf_tail = 0;
  new->rxfs = NULL;

  return new;

}				/* kbd_sec_start() */


static void
kbd_sec_end (void *dat)
{
  struct kbd_state *kbd = dat;
  struct mem_ops ops;

  if (!kbd->enabled)
    {
      free (kbd->rxfile);
      free (kbd);
      return;
    }

  memset (&ops, 0, sizeof (struct mem_ops));

  ops.readfunc8 = kbd_read8;
  ops.writefunc8 = kbd_write8;
  ops.read_dat8 = dat;
  ops.write_dat8 = dat;

  /* FIXME: Correct delay? */
  ops.delayr = 2;
  ops.delayw = 2;

  reg_mem_area (kbd->baseaddr, KBD_SPACE, 0, &ops);
  reg_sim_reset (kbd_reset, dat);
  reg_sim_stat (kbd_info, dat);
}

void
reg_kbd_sec ()
{
  struct config_section *sec =
    reg_config_sec ("kbd", kbd_sec_start, kbd_sec_end);

  reg_config_param (sec, "baseaddr", PARAMT_ADDR, kbd_baseaddr);
  reg_config_param (sec, "enabled",  PARAMT_INT, kbd_enabled);
  reg_config_param (sec, "irq",      PARAMT_INT, kbd_irq);
  reg_config_param (sec, "rxfile",   PARAMT_STR, kbd_rxfile);
}
