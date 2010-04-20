/* atadevice.c -- ATA Device simulation. Simulates a harddisk, using a local
   streams.

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
#ifdef __APPLE__
#include <libkern/OSByteOrder.h>
#define bswap_16(x) OSSwapInt16(x)
#define bswap_32(x) OSSwapInt32(x)
#define bswap_64(x) OSSwapInt64(x)
#else
#include <byteswap.h>
#endif

/* Package includes */
#include "atadevice.h"
#include "atadevice-cmdi.h"
#include "atacmd.h"
#include "abstract.h"


/*
 mandatory commands:
 - execute device diagnostics       (done)
 - flush cache
 - identify device                  (done)
 - initialize device parameters     (done)
 - read dma
 - read multiple
 - read sector(s)                   (done)
 - read verify sector(s)
 - seek
 - set features
 - set multiple mode
 - write dma
 - write multiple
 - write sector(s)


 optional commands:
 - download microcode
 - nop
 - read buffer
 - write buffer


 prohibited commands:
 - device reset
*/


/* Use a file to simulate a hard-disk                                 */
static FILE *
open_file (uint32_t * size, const char *filename)
{
  FILE *fp;
  unsigned long n;

  // TODO:

  /* check if a file with name 'filename' already exists             */
  if (!(fp = fopen (filename, "rb+")))
    if (!(fp = fopen (filename, "wb+")))
      {
	fprintf (stderr, "Warning: ata_open_file, cannot open hd-file %s\n",
		 filename);
	return NULL;
      }
    else
      {
	/* TODO create a file 'size' large */
	/* create a file 'size' large                                  */
	for (n = 0; n < *size; n++)
	  fputc (0, fp);
      }
  else				/* file already exist                                         */
    fprintf (stderr, "file %s already exists. Using existing file.\n",
	     filename);

  /* get the size of the file. This is also the size of the harddisk */
  fseek (fp, 0, SEEK_END);
  *size = ftell (fp);

  return fp;
}


/* Use a the local filesystem as a hard-disk                          */
static FILE *
open_local (void)
{
  // TODO:
  fprintf (stderr, "Warning: Device type LOCAL is not yet supported: "
	   "Defaulting to device type NO_CONNECT.");
  return NULL;
}

static void
ata_device_init (struct ata_device * device, int dev)
{
  /* set DeviceID                                                     */
  device->internals.dev = dev;

  /* generate stream for hd_simulation                                */
  switch (device->conf.type)
    {
    case TYPE_NO_CONNECT:
      device->conf.stream = NULL;
      break;

    case TYPE_FILE:
      device->conf.stream = open_file (&device->conf.size, device->conf.file);
      break;

    case TYPE_LOCAL:
      device->conf.stream = open_local ();
      break;

    default:
      fprintf (stderr, "Warning: Illegal device-type %d: "
	       "Defaulting to type NO_CONNECT.\n", device->conf.type);
      device->conf.stream = NULL;
      break;
    }

  device->conf.size_sect = device->conf.size / BYTES_PER_SECTOR;
}

/*
  D E V I C E S _ I N I T
*/
void
ata_devices_init (struct ata_devices * devices)
{
  ata_device_init (&devices->device[0], 0);

  if (devices->device[0].conf.type)
    ata_device_init (&devices->device[1], ATA_DHR_DEV);
  else
    ata_device_init (&devices->device[1], 0);
}

/*
  D E V I C E S _ H W _ R E S E T
*/
static void
ata_device_hw_reset (struct ata_device * device, int reset_signal,
		     int daspo, int pdiagi, int daspi)
{
  /* check ata-device state                                         */
  if (device->internals.state == ATA_STATE_HW_RST)
    {
      if (!reset_signal)
	{
	  /* hardware reset finished                                    */

	  /* set sectors_per_track & heads_per_cylinders                */
	  device->internals.sectors_per_track = device->conf.sectors;
	  device->internals.heads_per_cylinder = device->conf.heads;

	  /* set device1 input signals                                  */
	  device->sigs.pdiagi = pdiagi;
	  device->sigs.daspi = daspi;

	  ata_execute_device_diagnostics_cmd (device);

	  /* clear busy bit                                             */
	  device->regs.status &= ~ATA_SR_BSY;

	  /* set DRDY bit, when not a PACKET device                     */
	  if (!device->conf.packet)
	    device->regs.status |= ATA_SR_DRDY;

	  /* set new state                                              */
	  device->internals.state = ATA_STATE_IDLE;
	}
    }
  else if (reset_signal)
    {
      /* hardware reset signal asserted, stop what we are doing     */

      /* negate bus signals                                         */
      device->sigs.iordy = 0;
      device->sigs.intrq = 0;
      device->sigs.dmarq = 0;
      device->sigs.pdiago = 0;
      device->sigs.daspo = daspo;

      /* set busy bit                                               */
      device->regs.status |= ATA_SR_BSY;

      /* set new state                                              */
      device->internals.state = ATA_STATE_HW_RST;
    }
}

/* power-on and hardware reset                                        */
void
ata_devices_hw_reset (struct ata_devices * devices, int reset_signal)
{
  /* find device 0                                                    */
  if ((devices->device[0].conf.stream) && (devices->device[1].conf.stream))
    {
      /* this one is simple, device0 is device0                         */

      /* 1) handle device1 first                                        */
      ata_device_hw_reset (&devices->device[1], reset_signal, 1,	/* assert dasp, this is device1          */
			   0,	/* negate pdiag input, no more devices   */
			   0);	/* negate dasp input, no more devices    */

      /* 2) Then handle device0                                         */
      ata_device_hw_reset (&devices->device[0], reset_signal,
			   0,
			   devices->device[1].sigs.pdiago,
			   devices->device[1].sigs.daspo);
    }
  else if (devices->device[0].conf.stream)
    {
      /* device0 is device0, there's no device1                         */
      ata_device_hw_reset (&devices->device[0], reset_signal, 0,	/* negate dasp, this is device0          */
			   0,	/* negate pdiag input, there's no device1 */
			   0);	/* negate dasp input, there's no device1 */
    }
  else if (devices->device[1].conf.stream)
    {
      /* device1 is (logical) device0, there's no (physical) device0    */
      ata_device_hw_reset (&devices->device[1], reset_signal, 0,	/* negate dasp, this is device0          */
			   0,	/* negate pdiag input, there's no device1 */
			   0);	/* negate dasp input, there's no device1 */
    }
}


/*
  D E V I C E S _ D O _ C O N T R O L _ R E G I S T E R

  Handles - software reset
          - Interrupt enable bit
*/
static void
ata_device_do_control_register (struct ata_device * device)
{
  /* TODO respond to new values of nIEN */
  /* if device not selected, respond to new values of nIEN & SRST */

  /* check if SRST bit is set                                         */
  if (device->regs.device_control & ATA_DCR_RST)
    {
      if (device->internals.state == ATA_STATE_IDLE)
	{			/* start software reset                                       */
	  /* negate bus signals                                         */
	  device->sigs.pdiago = 0;
	  device->sigs.intrq = 0;
	  device->sigs.iordy = 0;
	  device->sigs.dmarq = 0;

	  /* set busy bit                                               */
	  device->regs.status |= ATA_SR_BSY;

	  /* set new state                                              */
	  device->internals.state = ATA_STATE_SW_RST;
	}
    }
  else if (device->internals.state == ATA_STATE_SW_RST)
    {				/* are we doing a software reset ??                             */
      /* SRST bit cleared, end of software reset                      */

      /*execute device diagnostics                                    */
      ata_execute_device_diagnostics_cmd (device);

      /* clear busy bit                                               */
      device->regs.status &= ~ATA_SR_BSY;

      /* set DRDY bit, when not a PACKET device                       */
      if (!device->conf.packet)
	device->regs.status |= ATA_SR_DRDY;

      /* set new state                                                */
      device->internals.state = ATA_STATE_IDLE;
    }
  /*
     <else> We are doing a hardware reset (or similar)
     ignore command
   */
}


/*
  D E V I C E S _ D O _ C O M M A N D _ R E G I S T E R
*/
static void
ata_device_do_command_register (struct ata_device * device)
{
  /* check BSY & DRQ                                                  */
  if ((device->regs.status & ATA_SR_BSY)
      || (device->regs.status & ATA_SR_DRQ))
    {
      if (device->regs.command != DEVICE_RESET)
	{
	  fprintf (stderr, "Warning: ata_device_write, writing a command "
		   "while BSY or DRQ asserted.\n");
	}
    }

  /* check if device selected                                         */
  if ((device->regs.device_head & ATA_DHR_DEV) == device->internals.dev)
    ata_device_execute_cmd (device);
  else
    {
      /* if not selected, only respond to EXECUTE DEVICE DIAGNOSTICS  */
      if (device->regs.command == EXECUTE_DEVICE_DIAGNOSTICS)
	ata_device_execute_cmd (device);
    }
}


/*
  D E V I C E S _ R E A D
*/
/* Read from devices                                                  */
short
ata_devices_read (struct ata_devices * devices, char adr)
{
  struct ata_device *device;

  /* check for no connected devices                                 */
  if ((!devices->device[0].conf.stream) && (!devices->device[1].conf.stream))
    {
      fprintf (stderr, "Warning: ata_devices_read, no ata devices "
	       "connected.\n");
    }
  else
    {
      /* check if both device0 and device1 are connected              */
      if ((devices->device[0].conf.stream)
	  && (devices->device[1].conf.stream))
	{
	  /* get the current active device                            */
	  if (devices->device[1].regs.device_head & ATA_DHR_DEV)
	    device = &devices->device[1];
	  else
	    device = &devices->device[0];
	}
      else
	{
	  /* only one device connected                                */
	  if (devices->device[1].conf.stream)
	    device = &devices->device[1];
	  else
	    device = &devices->device[0];
	}

      /* return data provided by selected device                      */
      switch (adr)
	{
	case ATA_ASR:
	  if ((device->regs.device_head & ATA_DHR_DEV) ==
	      device->internals.dev)
	    return device->regs.status;
	  else
	    {
	      return 0;		// return 0 when device0 responds for device1
	    }

	case ATA_CHR:
	  return device->regs.cylinder_high;

	case ATA_CLR:
	  return device->regs.cylinder_low;

	case ATA_DR:
	  if (!(device->regs.status & ATA_SR_DRQ))
	    {
	      return 0;
	    }
	  else
	    {
	      uint16_t val = LE16 (*device->internals.dbuf_ptr++);
	      if (!--device->internals.dbuf_cnt)
		{
		  device->regs.status &= ~ATA_SR_DRQ;
		  if (device->internals.end_t_func)
		    device->internals.end_t_func (device);
		}
	      return val;
	    }

	case ATA_DHR:
	  return device->regs.device_head;

	case ATA_ERR:
	  return device->regs.error;

	case ATA_SCR:
	  return device->regs.sector_count;

	case ATA_SNR:
	  return device->regs.sector_number;

	case ATA_SR:
	  if ((device->regs.device_head & ATA_DHR_DEV) ==
	      device->internals.dev)
	    {
	      device->sigs.intrq = 0;
	      return device->regs.status;
	    }

	  return 0;		// return 0 when device0 responds for device1

//        case ATA_DA   :
//          return device -> regs.status;
	}
    }
  return 0;
}


/*
  D E V I C E S _ W R I T E
*/
/* write to a single device                                           */
static void
ata_device_write (struct ata_device * device, char adr, short value)
{
  switch (adr)
    {
    case ATA_CR:
      device->regs.command = value;

      /* check command register settings and execute command    */
      ata_device_do_command_register (device);
      break;


    case ATA_CHR:
      device->regs.cylinder_high = value;
      break;

    case ATA_CLR:
      device->regs.cylinder_low = value;
      break;

    case ATA_DR:
      if (!(device->regs.status & ATA_SR_DRQ))
	{
	  break;
	}
      else
	{
	  *device->internals.dbuf_ptr++ = LE16 (value);
	  if (!--device->internals.dbuf_cnt)
	    {
	      device->regs.status &= ~ATA_SR_DRQ;
	      if (device->internals.end_t_func)
		device->internals.end_t_func (device);
	    }
	}
      device->regs.dataport_i = value;
      break;

    case ATA_DCR:
      device->regs.device_control = value;
      ata_device_do_control_register (device);
      break;

    case ATA_DHR:
      device->regs.device_head = value;
      break;

    case ATA_FR:
      device->regs.features = value;
      break;

    case ATA_SCR:
      device->regs.sector_count = value;
      break;

    case ATA_SNR:
      device->regs.sector_number = value;
      break;

    }				//endcase
}

/* Write to devices                                                   */
void
ata_devices_write (struct ata_devices * devices, char adr, short value)
{
  /* check for no connected devices                                 */
  if (!devices->device[0].conf.stream && !devices->device[1].conf.stream)
    {
      fprintf (stderr, "Warning: ata_devices_write, no ata devices "
	       "connected.\n");
    }
  else
    {
      /* first device                                                 */
      if (devices->device[0].conf.stream)
	ata_device_write (&devices->device[0], adr, value);

      /* second device                                                */
      if (devices->device[1].conf.stream)
	ata_device_write (&devices->device[1], adr, value);
    }
}
