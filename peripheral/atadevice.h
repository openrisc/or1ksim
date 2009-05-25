/* atadevice.h -- ATA Device code simulation

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

/*
 * Definitions for the Opencores ATA Controller Core, Device model
 */

#ifndef ATADEVICE__H
#define ATADEVICE__H

/* Autoconf and/or portability configuration */
#include "port.h"

/* System includes */
#include <stdio.h>

/* --- Register definitions --- */
/* ----- ATA Registers                                                */
/* These are actually the memory locations where the ATA registers    */
/* can be found in the host system; i.e. as seen from the CPU.        */
/* However, this doesn't matter for the simulator.                    */
#define ATA_ASR   0x78		/* Alternate Status Register      (R)  */
#define ATA_CR    0x5c		/* Command Register               (W)  */
#define ATA_CHR   0x54		/* Cylinder High Register       (R/W)  */
#define ATA_CLR   0x50		/* Cylinder Low Register        (R/W)  */
#define ATA_DR    0x40		/* Data Register                       */
#define ATA_DCR   0x78		/* Device Control Register        (W)  */
#define ATA_DHR   0x58		/* Device/Head Register         (R/W)  */
#define ATA_ERR   0x44		/* Error Register                 (R)  */
#define ATA_FR    0x44		/* Features Register              (W)  */
#define ATA_SCR   0x48		/* Sector Count Register        (R/W)  */
#define ATA_SNR   0x4c		/* Sector Number Register       (R/W)  */
#define ATA_SR    0x5c		/* Status Register                (R)  */
#define ATA_DA    0x7c		/* Device Address Register        (R)  */
	     /* ATA/ATAPI-5 does not describe Device Status Register  */

/* --------------------------------------                             */
/* ----- ATA Device bit defenitions -----                             */
/* --------------------------------------                             */

/* ----- ATA (Alternate) Status Register                              */
#define ATA_SR_BSY  0x80	/* Busy                               */
#define ATA_SR_DRDY 0x40	/* Device Ready                       */
#define ATA_SR_DF   0x20	/* Device Fault                       */
#define ATA_SR_DSC  0x10	/* Device Seek Complete               */
#define ATA_SR_DRQ  0x08	/* Data Request                       */
#define ATA_SR_COR  0x04	/* Corrected data (obsolete)          */
#define ATA_SR_IDX  0x02	/*                (obsolete)          */
#define ATA_SR_ERR  0x01	/* Error                              */

/* ----- ATA Device Control Register                                  */
				/* bits 7-3 are reserved              */
#define ATA_DCR_RST 0x04	/* Software reset   (RST=1, reset)    */
#define ATA_DCR_IEN 0x02	/* Interrupt Enable (IEN=0, enabled)  */
				/* always write a '0' to bit0         */

/* ----- ATA Device Address Register                                  */
/* All values in this register are one's complement (i.e. inverted)   */
#define ATA_DAR_WTG 0x40	/* Write Gate                         */
#define ATA_DAR_H   0x3c	/* Head Select                        */
#define ATA_DAR_DS1 0x02	/* Drive select 1                     */
#define ATA_DAR_DS0 0x01	/* Drive select 0                     */

/* ----- Device/Head Register                                         */
#define ATA_DHR_LBA 0x40	/* LBA/CHS mode ('1'=LBA mode)        */
#define ATA_DHR_DEV 0x10	/* Device       ('0'=dev0, '1'=dev1)  */
#define ATA_DHR_H   0x0f	/* Head Select                        */

/* ----- Error Register                                               */
#define ATA_ERR_BBK  0x80	/* Bad Block                          */
#define ATA_ERR_UNC  0x40	/* Uncorrectable Data Error           */
#define ATA_ERR_IDNF 0x10	/* ID Not Found                       */
#define ATA_ERR_ABT  0x04	/* Aborted Command                    */
#define ATA_ERR_TON  0x02	/* Track0 Not Found                   */
#define ATA_ERR_AMN  0x01	/* Address Mark Not Found             */

/* --------------------------                                         */
/* ----- Device Defines -----                                         */
/* --------------------------                                         */

/* types for hard disk simulation                                     */
#define TYPE_NO_CONNECT 0
#define TYPE_FILE       1
#define TYPE_LOCAL      2


/* -----------------------------                                      */
/* ----- Statemachine defines --                                      */
/* -----------------------------                                      */
#define ATA_STATE_IDLE   0x00
#define ATA_STATE_SW_RST 0x01
#define ATA_STATE_HW_RST 0x02


/* ----------------------------                                       */
/* ----- Structs          -----                                       */
/* ----------------------------                                       */
struct ata_device
{

  /******* Housekeeping *****************************************/
  struct
  {
    /* Pointer to host that device is attached to         */
    void *host;
    /* device number                                      */
    int dev;

    /* current PIO mode                                   */
    int pio_mode;

    /* current DMA mode                                   */
    int dma_mode;

    /* databuffer                                         */
    uint16_t dbuf[4096];
    uint16_t *dbuf_ptr;
    uint16_t dbuf_cnt;

    /* current statemachine state                         */
    int state;

    /* current CHS translation settings                   */
    unsigned int heads_per_cylinder;
    unsigned int sectors_per_track;

    /* Current byte being read                            */
    uint32_t lba;
    /* Number of sectors still needing to be read         */
    int nr_sect;
    /* function to call when block of data has been transfered */
    void (*end_t_func) (struct ata_device *);
  } internals;


	/******* ATA Device Registers *********************************/
  struct
  {
    uint8_t command;
    uint8_t cylinder_low;
    uint8_t cylinder_high;
    uint8_t device_control;
    uint8_t device_head;
    uint8_t error;
    uint8_t features;
    uint8_t sector_count;
    uint8_t sector_number;
    uint8_t status;

    uint16_t dataport_i;
  } regs;

	/******** ata device output signals **************************/
  struct
  {
    int iordy;
    int intrq;
    int dmarq;
    int pdiagi, pdiago;
    int daspi, daspo;
  } sigs;

	/******** simulator settings **********************************/
  struct
  {
    char *file;		/* Filename (if type == FILE)                   */
    FILE *stream;	/* stream where the simulated device connects to */
    int type;		/* Simulate device using                        */
    /* NO_CONNECT: no device connected (dummy)      */
    /* FILE      : a file                           */
    /* LOCAL     : a local stream, e.g./dev/hda1    */
    uint32_t size;	/* size in bytes of the simulated device    */
    uint32_t size_sect;	/* size in sectors of the simulated device */
    int packet;		/* device implements PACKET command set         */

    unsigned int heads;
    unsigned int sectors;

    char *firmware;
    unsigned int mwdma;
    unsigned int pio;
  } conf;
};

struct ata_devices
{
  struct ata_device device[2];
};

/* all devices                                                        */
void ata_devices_init (struct ata_devices *devices);
void ata_devices_hw_reset (struct ata_devices *devices, int reset_signal);
short ata_devices_read (struct ata_devices *devices, char adr);
void ata_devices_write (struct ata_devices *devices, char adr, short value);

#endif	/* ATADEVICE__H */
