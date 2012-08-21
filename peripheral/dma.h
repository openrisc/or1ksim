/* dma.h -- Definition of DMA<->peripheral interface

   Copyright (C) 2001 by Erez Volk, erez@opencores.org
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

#ifndef DMA__H
#define DMA__H

/* Package includes */
#include "dma-defs.h"
#include "sim-config.h"

/* Implementation of DMA Channel Registers and State */
struct dma_channel
{
  /* The controller we belong to */
  struct dma_controller *controller;

  /* Our channel number and bit mask */
  unsigned channel_number;
  unsigned long channel_mask;

  /* Used for dump, to save dumping all 32 channels */
  unsigned referenced;

  /* Inner state of transfer etc. */
  unsigned load_next_descriptor_when_done;
  unsigned long current_descriptor;
  oraddr_t source, destination, source_mask, destination_mask;
  unsigned long chunk_size, total_size, words_transferred;

  /* The interface registers */
  struct
  {
    unsigned long csr;
    unsigned long sz;
    unsigned long a0;
    unsigned long am0;
    unsigned long a1;
    unsigned long am1;
    unsigned long desc;
    unsigned long swptr;
  } regs;

  /* Some control signals */
  unsigned dma_req_i;
  unsigned dma_ack_o;
  unsigned dma_nd_i;
};


/* Implementation of DMA Controller Registers and State */
struct dma_controller
{
  /* Is peripheral enabled */
  int enabled;

  /* Base address in memory */
  oraddr_t baseaddr;

  /* Which interrupt number we generate */
  unsigned irq;

  /* VAPI id */
  int vapi_id;

  /* Controller Registers */
  struct
  {
    unsigned long csr;
    unsigned long int_msk_a;
    unsigned long int_msk_b;
    unsigned long int_src_a;
    unsigned long int_src_b;
  } regs;

  /* Channels */
  struct dma_channel ch[DMA_NUM_CHANNELS];

  struct dma_controller *next;
};

void set_dma_req_i (struct dma_channel *channel);
void clear_dma_req_i (struct dma_channel *channel);
void set_dma_nd_i (struct dma_channel *channel);
void clear_dma_nd_i (struct dma_channel *channel);
unsigned check_dma_ack_o (struct dma_channel *channel);
struct dma_channel *find_dma_controller_ch (unsigned controller,
					    unsigned channel);

/* Prototype for external use */
extern void  reg_dma_sec ();

#endif	/* DMA__H */
