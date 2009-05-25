/* atahost.h -- ATA Host code simulation

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

/* Definitions for the Opencores ATA Host Controller Core */


#ifndef ATAHOST__H
#define ATAHOST__H

#include "arch.h"
#include "atadevice.h"

/* --- Register definitions --- */

/* ----- Core Registers                                              */
#define ATA_CTRL  0x00		/* Control register                   */
#define ATA_STAT  0x04		/* Status register                    */
#define ATA_PCTR  0x08		/* PIO command timing register        */
#define ATA_PFTR0 0x0c		/* PIO Fast Timing register Device0   */
#define ATA_PFTR1 0x10		/* PIO Fast Timing register Device1   */
#define ATA_DTR0  0x14		/* DMA Timing register Device2        */
#define ATA_DTR1  0x18		/* DMA Timing register Device1        */
#define ATA_TXB   0x3c		/* DMA Transmit buffer                */
#define ATA_RXB   0x3c		/* DMA Receive buffer                 */


/* ----------------------------                                       */
/* ----- Bits definitions -----                                       */
/* ----------------------------                                       */

/* ----- Core Control register                                        */
				/* bits 31-16 are reserved            */
#define ATA_DMA_EN  (0<<15)	/* DMAen, DMA enable bit              */
				/* bit 14 is reserved                 */
#define ATA_DMA_WR  (1<<14)	/* DMA Write transaction              */
#define ATA_DMA_RD  (0<<14)	/* DMA Read transaction               */
				/* bits 13-10 are reserved            */
#define ATA_BELEC1  (1<< 9)	/* Big-Little endian conversion       */
				/* enable bit for Device1             */
#define ATA_BELEC0  (1<< 8)	/* Big-Little endian conversion       */
				/* enable bit for Device0             */
#define ATA_IDE_EN  (1<< 7)	/* IDE core enable bit                */
#define ATA_FTE1    (1<< 6)	/* Device1 Fast PIO Timing Enable bit */
#define ATA_FTE0    (1<< 5)	/* Device0 Fast PIO Timing Enable bit */
#define ATA_PWPP    (1<< 4)	/* PIO Write Ping-Pong Enable bit     */
#define ATA_IORDY_FTE1 (1<< 3)	/* Device1 Fast PIO Timing IORDY      */
				/* enable bit                         */
#define ATA_IORDY_FTE0 (1<< 2)	/* Device0 Fast PIO Timing IORDY      */
				/* enable bit                         */
#define ATA_IORDY   (1<< 1)	/* PIO Command Timing IORDY enable bit */
#define ATA_RST     (1<< 0)	/* ATA Reset bit                      */

/* ----- Core Status register                                         */
#define ATA_DEVID   0xf0000000	/* bits 31-28 Device-ID               */
#define ATA_REVNO   0x0f000000	/* bits 27-24 Revision number         */
				/* bits 23-16 are reserved            */
#define ATA_DMA_TIP (1<<15)	/* DMA Transfer in progress           */
				/* bits 14-10 are reserved            */
#define ATA_DRBE    (1<<10)	/* DMA Receive buffer empty           */
#define ATA_DTBF    (1<< 9)	/* DMA Transmit buffer full           */
#define ATA_DMARQ   (1<< 8)	/* DMARQ Line status                  */
#define ATA_PIO_TIP (1<< 7	/* PIO Transfer in progress           */
#define ATA_PWPPF   (1<< 6)	/* PIO write ping-pong full           */
				/* bits 5-1 are reserved              */
#define ATA_IDEIS  (1<< 0)	/* IDE Interrupt status               */


/* -----  Core Timing registers                                       */
#define ATA_TEOC       24	/* End of cycle time          DMA/PIO */
#define ATA_T4         16	/* DIOW- data hold time           PIO */
#define ATA_T2          8	/* DIOR-/DIOW- pulse width        PIO */
#define ATA_TD          8	/* DIOR-/DIOW- pulse width        DMA */
#define ATA_T1          0	/* Address valid to DIOR-/DIOW-   PIO */
#define ATA_TM          0	/* CS[1:0]valid to DIOR-/DIOW-    DMA */


/* -----------------------------                                      */
/* ----- Simulator defines -----                                      */
/* -----------------------------                                      */
#define ATA_ADDR_SPACE 0x80


/* ----------------------------                                       */
/* ----- Structs          -----                                       */
/* ----------------------------                                       */
struct ata_host
{
  /* Is peripheral enabled? */
  int enabled;

  /* Base address in memory                                     */
  oraddr_t baseaddr;

  /* Registered memory area                                     */
  struct dev_memarea *mem;

  /* Which IRQ to generate                                      */
  int irq;

  /* Which ocidec version is implemented                        */
  int dev_id;

  /* OCIDEC revision                                            */
  int rev;

  /* Current selected device                                    */
  int dev_sel;

  /* PIO T1 reset value                                         */
  uint8_t pio_mode0_t1;

  /* PIO T2 reset value                                         */
  uint8_t pio_mode0_t2;

  /* PIO T4 reset value                                         */
  uint8_t pio_mode0_t4;

  /* PIO Teoc reset value                                       */
  uint8_t pio_mode0_teoc;

  /* DMA Tm reset value                                         */
  uint8_t dma_mode0_tm;

  /* DMA Td reset value                                         */
  uint8_t dma_mode0_td;

  /* DMA Teoc reset value                                       */
  uint8_t dma_mode0_teoc;

  /* ata host registers                                         */
  struct
  {
    uint32_t ctrl;
    uint32_t stat;
    uint32_t pctr;
    uint32_t pftr0;
    uint32_t pftr1;
    uint32_t dtr0;
    uint32_t dtr1;
    uint32_t txb;
    uint32_t rxb;
  } regs;

  /* connected ATA devices (slaves)                             */
  struct ata_devices  devices;
};

/* ----------------------------                                       */
/* ----- Prototypes       -----                                       */
/* ----------------------------                                       */
void ata_int (void *dat);


/* ----------------------------                                       */
/* ----- Macros           -----                                       */
/* ----------------------------                                       */
#define is_ata_hostadr(adr) (!(adr & 0x40))

// FIXME
#define ata_pio_delay(pioreg) ( (((pioreg >> ATA_T1) & 0xff) +1) + (((pioreg >> ATA_T2) & 0xff) +1) + (((pioreg >> ATA_T4) & 0xff) +1) +1 )
#define ata_dma_delay(dmareg) ( (((dmareg >> ATA_TD) & 0xff) +1) + (((pioreg >> ATA_TM) & 0xff) +1) +1 )

/* Prototypes for external use */
void reg_ata_sec ();

#endif /* ATAHOST__H */
