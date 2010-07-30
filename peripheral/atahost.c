/* atahost.c -- ATA Host code simulation

   Copyright (C) 2002 Richard Herveille, rherveille@opencores.org
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

/* Package includes */
#include "atahost.h"
#include "sim-config.h"
#include "abstract.h"
#include "pic.h"
#include "toplevel-support.h"
#include "sim-cmd.h"


/* default timing reset values */
#define PIO_MODE0_T1 6
#define PIO_MODE0_T2 28
#define PIO_MODE0_T4 2
#define PIO_MODE0_TEOC 23

#define DMA_MODE0_TM 4
#define DMA_MODE0_TD 21
#define DMA_MODE0_TEOC 21


/* reset and initialize ATA host core(s) */
static void
ata_reset (void *dat)
{
  struct ata_host *ata = dat;

  // reset the core registers
  ata->regs.ctrl = 0x0001;
  ata->regs.stat = (ata->dev_id << 28) | (ata->rev << 24);
  ata->regs.pctr = (ata->pio_mode0_teoc << ATA_TEOC) |
    (ata->pio_mode0_t4 << ATA_T4) |
    (ata->pio_mode0_t2 << ATA_T2) | (ata->pio_mode0_t1 << ATA_T1);
  ata->regs.pftr0 = ata->regs.pctr;
  ata->regs.pftr1 = ata->regs.pctr;
  ata->regs.dtr0 = (ata->dma_mode0_teoc << ATA_TEOC) |
    (ata->dma_mode0_td << ATA_TD) | (ata->dma_mode0_tm << ATA_TM);
  ata->regs.dtr1 = ata->regs.dtr0;
  ata->regs.txb = 0;

  // inform simulator about new read/write delay timings
  adjust_rw_delay (ata->mem, ata_pio_delay (ata->regs.pctr),
		   ata_pio_delay (ata->regs.pctr));

  /* the reset bit in the control register 'ctrl' is set, reset connect ata-devices */
  ata_devices_hw_reset (&ata->devices, 1);
}

/* ========================================================================= */


/* Assert interrupt */
void
ata_int (void *dat)
{
  struct ata_host *ata = dat;
  if (!(ata->regs.stat & ATA_IDEIS))
    {
      ata->regs.stat |= ATA_IDEIS;
      report_interrupt (ata->irq);
    }
}

/* ========================================================================= */


/*
  Read a register
*/
static uint32_t
ata_read32 (oraddr_t addr, void *dat)
{
  struct ata_host *ata = dat;

  /* determine if ata_host or ata_device addressed */
  if (is_ata_hostadr (addr))
    {
      // Accesses to internal register take 2cycles
      adjust_rw_delay (ata->mem, 2, 2);

      switch (addr)
	{
	case ATA_CTRL:
	  return ata->regs.ctrl;

	case ATA_STAT:
	  return ata->regs.stat;

	case ATA_PCTR:
	  return ata->regs.pctr;

	case ATA_PFTR0:
	  return ata->dev_id > 1 ? ata->regs.pftr0 : 0;

	case ATA_PFTR1:
	  return ata->dev_id > 1 ? ata->regs.pftr1 : 0;

	case ATA_DTR0:
	  return ata->dev_id > 2 ? ata->regs.dtr0 : 0;

	case ATA_DTR1:
	  return ata->dev_id > 2 ? ata->regs.dtr1 : 0;

	case ATA_RXB:
	  return ata->dev_id > 2 ? ata->regs.rxb : 0;

	default:
	  return 0;
	}
    }

  /* check if the controller is enabled */
  if (ata->regs.ctrl & ATA_IDE_EN)
    {
      // make sure simulator uses correct read/write delay timings
      if (((addr & 0x7f) == ATA_DR) && ata->dev_id > 1)
	{
	  if (ata->dev_sel)
	    adjust_rw_delay (ata->mem, ata_pio_delay (ata->regs.pftr1),
			     ata_pio_delay (ata->regs.pftr1));
	  else
	    adjust_rw_delay (ata->mem, ata_pio_delay (ata->regs.pftr0),
			     ata_pio_delay (ata->regs.pftr0));
	}
      else
	adjust_rw_delay (ata->mem, ata_pio_delay (ata->regs.pctr),
			 ata_pio_delay (ata->regs.pctr));

      return ata_devices_read (&ata->devices, addr & 0x7f);
    }
  return 0;
}

/* ========================================================================= */


/*
  Write a register
*/
static void
ata_write32 (oraddr_t addr, uint32_t value, void *dat)
{
  struct ata_host *ata = dat;

  /* determine if ata_host or ata_device addressed */
  if (is_ata_hostadr (addr))
    {
      // Accesses to internal register take 2cycles
      adjust_rw_delay (ata->mem, 2, 2);

      switch (addr)
	{
	case ATA_CTRL:
	  ata->regs.ctrl = value;

	  /* check if reset bit set, if so reset ata-devices    */
	  if (value & ATA_RST)
	    ata_devices_hw_reset (&ata->devices, 1);
	  else
	    ata_devices_hw_reset (&ata->devices, 0);
	  break;

	case ATA_STAT:
	  if (!(value & ATA_IDEIS) && (ata->regs.stat & ATA_IDEIS))
	    {
	      clear_interrupt (ata->irq);
	      ata->regs.stat &= ~ATA_IDEIS;
	    }
	  break;

	case ATA_PCTR:
	  ata->regs.pctr = value;
	  break;

	case ATA_PFTR0:
	  ata->regs.pftr0 = value;
	  break;

	case ATA_PFTR1:
	  ata->regs.pftr1 = value;
	  break;

	case ATA_DTR0:
	  ata->regs.dtr0 = value;
	  break;

	case ATA_DTR1:
	  ata->regs.dtr1 = value;
	  break;

	case ATA_TXB:
	  ata->regs.txb = value;
	  break;

	default:
	  /* ERROR-macro currently only supports simple strings. */
	  /*
	     fprintf(stderr, "ERROR  : Unknown register for OCIDEC(%1d).\n", DEV_ID );

	     Tried to show some useful info here.
	     But when using 'DM'-simulator-command, the screen gets filled with these messages.
	     Thereby eradicating the usefulness of the message
	   */
	  break;
	}
      return;
    }

  /* check if the controller is enabled */
  if (ata->regs.ctrl & ATA_IDE_EN)
    {
      // make sure simulator uses correct read/write delay timings
      if (((addr & 0x7f) == ATA_DR) && (ata->dev_id > 1))
	{
	  if (ata->dev_sel)
	    adjust_rw_delay (ata->mem, ata_pio_delay (ata->regs.pftr1),
			     ata_pio_delay (ata->regs.pftr1));
	  else
	    adjust_rw_delay (ata->mem, ata_pio_delay (ata->regs.pftr0),
			     ata_pio_delay (ata->regs.pftr0));
	}
      else
	adjust_rw_delay (ata->mem, ata_pio_delay (ata->regs.pctr),
			 ata_pio_delay (ata->regs.pctr));

      if ((addr & 0x7f) == ATA_DHR)
	ata->dev_sel = value & ATA_DHR_DEV;

      ata_devices_write (&ata->devices, addr & 0x7f, value);
    }
}

/* ========================================================================= */


/* Dump status */
static void
ata_status (void *dat)
{
  struct ata_host *ata = dat;

  if (ata->baseaddr == 0)
    return;

  PRINTF ("\nOCIDEC-%1d at: 0x%" PRIxADDR "\n", ata->dev_id, ata->baseaddr);
  PRINTF ("ATA CTRL     : 0x%08" PRIx32 "\n", ata->regs.ctrl);
  PRINTF ("ATA STAT     : 0x%08" PRIx32 "\n", ata->regs.stat);
  PRINTF ("ATA PCTR     : 0x%08" PRIx32 "n", ata->regs.pctr);

  if (ata->dev_id > 1)
    {
      PRINTF ("ATA FCTR0    : 0x%08" PRIx32 "\n", ata->regs.pftr0);
      PRINTF ("ATA FCTR1    : 0x%08" PRIx32 "\n", ata->regs.pftr1);
    }

  if (ata->dev_id > 2)
    {
      PRINTF ("ATA DTR0     : 0x%08" PRIx32 "\n", ata->regs.dtr0);
      PRINTF ("ATA DTR1     : 0x%08" PRIx32 "\n", ata->regs.dtr1);
      PRINTF ("ATA TXD      : 0x%08" PRIx32 "\n", ata->regs.txb);
      PRINTF ("ATA RXD      : 0x%08" PRIx32 "\n", ata->regs.rxb);
    }
}

/* ========================================================================= */

/*----------------------------------------------------[ ATA Configuration ]---*/
static unsigned int conf_dev;

static void
ata_baseaddr (union param_val val, void *dat)
{
  struct ata_host *ata = dat;
  ata->baseaddr = val.addr_val;
}

static void
ata_irq (union param_val val, void *dat)
{
  struct ata_host *ata = dat;
  ata->irq = val.int_val;
}

static void
ata_dev_id (union param_val val, void *dat)
{
  struct ata_host *ata = dat;
  if (val.int_val < 1 || val.int_val > 3)
    {
      fprintf (stderr, "Peripheral ATA: Unknown device id %d, useing 1\n",
	       val.int_val);
      ata->dev_id = 1;
      return;
    }

  ata->dev_id = val.int_val;
}


/*---------------------------------------------------------------------------*/
/*!Set the ATA revision

   This must be in the range 0-15, to fit in the relevant field. Anything
   larger is truncated with a warning.

   @param[in] val  The value to use
   @param[in] dat  The config data structure                                 */
/*---------------------------------------------------------------------------*/
static void
ata_rev (union param_val val, void *dat)
{
  struct ata_host *ata = dat;

  if (val.int_val > 0xf)
    {
      fprintf (stderr, "Warning: ATA rev > 4 bits: truncated\n");
    }

  ata->rev = val.int_val & 0xf;
}

static void
ata_pio_mode0_t1 (union param_val val, void *dat)
{
  struct ata_host *ata = dat;

  if (val.int_val < 0 || val.int_val > 255)
    {
      fprintf (stderr, "Peripheral ATA: Invalid pio_mode0_t1: %d\n",
	       val.int_val);
      return;
    }

  ata->pio_mode0_t1 = val.int_val;
}

static void
ata_pio_mode0_t2 (union param_val val, void *dat)
{
  struct ata_host *ata = dat;

  if (val.int_val < 0 || val.int_val > 255)
    {
      fprintf (stderr, "Peripheral ATA: Invalid pio_mode0_t2: %d\n",
	       val.int_val);
      return;
    }

  ata->pio_mode0_t2 = val.int_val;
}

static void
ata_pio_mode0_t4 (union param_val val, void *dat)
{
  struct ata_host *ata = dat;

  if (val.int_val < 0 || val.int_val > 255)
    {
      fprintf (stderr, "Peripheral ATA: Invalid pio_mode0_t4: %d\n",
	       val.int_val);
      return;
    }

  ata->pio_mode0_t4 = val.int_val;
}

static void
ata_pio_mode0_teoc (union param_val val, void *dat)
{
  struct ata_host *ata = dat;

  if (val.int_val < 0 || val.int_val > 255)
    {
      fprintf (stderr, "Peripheral ATA: Invalid pio_mode0_teoc: %d\n",
	       val.int_val);
      return;
    }

  ata->pio_mode0_teoc = val.int_val;
}

static void
ata_dma_mode0_tm (union param_val val, void *dat)
{
  struct ata_host *ata = dat;

  if (val.int_val < 0 || val.int_val > 255)
    {
      fprintf (stderr, "Peripheral ATA: Invalid dma_mode0_tm: %d\n",
	       val.int_val);
      return;
    }

  ata->dma_mode0_tm = val.int_val;
}

static void
ata_dma_mode0_td (union param_val val, void *dat)
{
  struct ata_host *ata = dat;

  if (val.int_val < 0 || val.int_val > 255)
    {
      fprintf (stderr, "Peripheral ATA: Invalid dma_mode0_td: %d\n",
	       val.int_val);
      return;
    }

  ata->dma_mode0_td = val.int_val;
}

static void
ata_dma_mode0_teoc (union param_val val, void *dat)
{
  struct ata_host *ata = dat;

  if (val.int_val < 0 || val.int_val > 255)
    {
      fprintf (stderr, "Peripheral ATA: Invalid dma_mode0_teoc: %d\n",
	       val.int_val);
      return;
    }

  ata->dma_mode0_teoc = val.int_val;
}

static void
ata_type (union param_val val, void *dat)
{
  struct ata_host *ata = dat;
  if (conf_dev <= 1)
    ata->devices.device[conf_dev].conf.type = val.int_val;
}


/*---------------------------------------------------------------------------*/
/*!Set the ATA file

   Free any previously allocated value. Only used if device type is 1.

   @param[in] val  The value to use
   @param[in] dat  The config data structure                                 */
/*---------------------------------------------------------------------------*/
static void
ata_file (union param_val val, void *dat)
{
  struct ata_host *ata = dat;

  if (conf_dev <= 1)
    {
      if (NULL != ata->devices.device[conf_dev].conf.file)
	{
	  free (ata->devices.device[conf_dev].conf.file);
	  ata->devices.device[conf_dev].conf.file = NULL;
	}

      if (!(ata->devices.device[conf_dev].conf.file = strdup (val.str_val)))
	{
	  fprintf (stderr, "Peripheral ATA: Run out of memory\n");
	  exit (-1);
	}
    }
}	/* ata_file() */


static void
ata_size (union param_val val, void *dat)
{
  struct ata_host *ata = dat;
  if (conf_dev <= 1)
    ata->devices.device[conf_dev].conf.size = val.int_val << 20;
}

static void
ata_packet (union param_val val, void *dat)
{
  struct ata_host *ata = dat;
  if (conf_dev <= 1)
    ata->devices.device[conf_dev].conf.packet = val.int_val;
}

static void
ata_enabled (union param_val val, void *dat)
{
  struct ata_host *ata = dat;
  ata->enabled = val.int_val;
}

static void
ata_heads (union param_val val, void *dat)
{
  struct ata_host *ata = dat;
  if (conf_dev <= 1)
    ata->devices.device[conf_dev].conf.heads = val.int_val;
}

static void
ata_sectors (union param_val val, void *dat)
{
  struct ata_host *ata = dat;
  if (conf_dev <= 1)
    ata->devices.device[conf_dev].conf.sectors = val.int_val;
}

static void
ata_firmware (union param_val val, void *dat)
{
  struct ata_host *ata = dat;
  if (conf_dev <= 1)
    if (!(ata->devices.device[conf_dev].conf.firmware = strdup (val.str_val)))
      {
	fprintf (stderr, "Peripheral ATA: Run out of memory\n");
	exit (-1);
      }
}


/*---------------------------------------------------------------------------*/
/*!Set the ATA multi-word DMA mode

   Must be -1, 0, 1 or 2.

   @param[in] val  The value to use
   @param[in] dat  The config data structure                                 */
/*---------------------------------------------------------------------------*/
static void
ata_mwdma (union param_val  val,
	   void            *dat)
{
  struct ata_host *ata = dat;

  if ((val.int_val >= -1) && (val.int_val <= 2))
    {
      ata->devices.device[conf_dev].conf.mwdma = val.int_val;
    }
  else
    {
      fprintf (stderr, "Warning: invalid ATA multi-word DMA mode: ignored\n");
    }
}	/* ata_mwdma() */


/*---------------------------------------------------------------------------*/
/*!Set the ATA programmed I/O mode

   Must be 0, 1, 2, 3 or 4.

   @param[in] val  The value to use
   @param[in] dat  The config data structure                                 */
/*---------------------------------------------------------------------------*/
static void
ata_pio (union param_val  val,
	 void            *dat)
{
  struct ata_host *ata = dat;

  if ((val.int_val >= 0) && (val.int_val <= 4))
    {
      ata->devices.device[conf_dev].conf.pio = val.int_val;
    }
  else
    {
      fprintf (stderr, "Warning: invalid ATA programmed I/O mode: ignored\n");
    }
}	/* ata_pio() */


static void
ata_start_device (union param_val val, void *dat)
{
  conf_dev = val.int_val;

  if (conf_dev > 1)
    fprintf (stderr, "Device %d out-of-range\n", conf_dev);
}

static void
ata_enddevice (union param_val val, void *dat)
{
  conf_dev = 2;
}

/*---------------------------------------------------------------------------*/
/*!Initialize a new ATA configuration

   ALL parameters are set explicitly to default values.                      */
/*---------------------------------------------------------------------------*/
static void *
ata_sec_start (void)
{
  struct ata_host *new = malloc (sizeof (struct ata_host));

  if (!new)
    {
      fprintf (stderr, "Peripheral ATA: Run out of memory\n");
      exit (-1);
    }

  memset (new, 0, sizeof (struct ata_host));

  new->enabled        = 1;
  new->baseaddr       = 0;
  new->irq            = 0;
  new->dev_id         = 1;
  new->rev            = 0;

  new->pio_mode0_t1   = PIO_MODE0_T1;
  new->pio_mode0_t2   = PIO_MODE0_T2;
  new->pio_mode0_t4   = PIO_MODE0_T4;
  new->pio_mode0_teoc = PIO_MODE0_TEOC;

  new->dma_mode0_tm   = DMA_MODE0_TM;
  new->dma_mode0_td   = DMA_MODE0_TD;
  new->dma_mode0_teoc = DMA_MODE0_TEOC;

  new->devices.device[0].conf.type     = 0;
  new->devices.device[0].conf.file     = strdup ("ata_file0");
  new->devices.device[0].conf.size     = 0;
  new->devices.device[0].conf.packet   = 0;
  new->devices.device[0].conf.heads    = 7;
  new->devices.device[0].conf.sectors  = 32;
  new->devices.device[0].conf.firmware = "02207031";
  new->devices.device[0].conf.mwdma    = 2;
  new->devices.device[0].conf.pio      = 4;

  new->devices.device[1].conf.type     = 0;
  new->devices.device[1].conf.file     = strdup ("ata_file1");
  new->devices.device[1].conf.size     = 0;
  new->devices.device[1].conf.packet   = 0;
  new->devices.device[1].conf.heads    = 7;
  new->devices.device[1].conf.sectors  = 32;
  new->devices.device[1].conf.firmware = "02207031";
  new->devices.device[1].conf.mwdma    = 2;
  new->devices.device[1].conf.pio      = 4;

  return new;

}	/* ata_sec_start() */


static void
ata_sec_end (void *dat)
{
  struct ata_host *ata = dat;
  struct mem_ops ops;

  if (!ata->enabled)
    {
      free (dat);
      return;
    }

  /* Connect ata_devices.                                            */
  ata_devices_init (&ata->devices);
  ata->devices.device[0].internals.host = ata;
  ata->devices.device[1].internals.host = ata;

  memset (&ops, 0, sizeof (struct mem_ops));

  ops.readfunc32 = ata_read32;
  ops.read_dat32 = dat;
  ops.writefunc32 = ata_write32;
  ops.write_dat32 = dat;

  /* Delays will be readjusted later */
  ops.delayr = 2;
  ops.delayw = 2;

  ata->mem = reg_mem_area (ata->baseaddr, ATA_ADDR_SPACE, 0, &ops);

  reg_sim_reset (ata_reset, dat);
  reg_sim_stat (ata_status, dat);
}

void
reg_ata_sec ()
{
  struct config_section *sec =
    reg_config_sec ("ata", ata_sec_start, ata_sec_end);

  reg_config_param (sec, "enabled",        PARAMT_INT, ata_enabled);
  reg_config_param (sec, "baseaddr",       PARAMT_ADDR, ata_baseaddr);
  reg_config_param (sec, "irq",            PARAMT_INT, ata_irq);
  reg_config_param (sec, "dev_id",         PARAMT_INT, ata_dev_id);
  reg_config_param (sec, "rev",            PARAMT_INT, ata_rev);

  reg_config_param (sec, "pio_mode0_t1",   PARAMT_INT, ata_pio_mode0_t1);
  reg_config_param (sec, "pio_mode0_t2",   PARAMT_INT, ata_pio_mode0_t2);
  reg_config_param (sec, "pio_mode0_t4",   PARAMT_INT, ata_pio_mode0_t4);
  reg_config_param (sec, "pio_mode0_teoc", PARAMT_INT, ata_pio_mode0_teoc);

  reg_config_param (sec, "dma_mode0_tm",   PARAMT_INT, ata_dma_mode0_tm);
  reg_config_param (sec, "dma_mode0_td",   PARAMT_INT, ata_dma_mode0_td);
  reg_config_param (sec, "dma_mode0_teoc", PARAMT_INT, ata_dma_mode0_teoc);

  reg_config_param (sec, "device",         PARAMT_INT, ata_start_device);
  reg_config_param (sec, "enddevice",      PARAMT_INT, ata_enddevice);

  reg_config_param (sec, "type",           PARAMT_INT, ata_type);
  reg_config_param (sec, "file",           PARAMT_STR, ata_file);
  reg_config_param (sec, "size",           PARAMT_INT, ata_size);
  reg_config_param (sec, "packet",         PARAMT_INT, ata_packet);
  reg_config_param (sec, "heads",          PARAMT_INT, ata_heads);
  reg_config_param (sec, "sectors",        PARAMT_INT, ata_sectors);
  reg_config_param (sec, "firmware",       PARAMT_STR, ata_firmware);
  reg_config_param (sec, "mwdma",          PARAMT_INT, ata_mwdma);
  reg_config_param (sec, "pio",            PARAMT_INT, ata_pio);
}
