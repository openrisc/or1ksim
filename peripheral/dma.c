/* dma.c -- Simulation of DMA

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

/* This simulation of the DMA core is not meant to be full.  It is written
   only to allow simulating the Ethernet core.  Of course, if anyone feels
   like perfecting it, feel free...  */


/* Autoconf and/or portability configuration */
#include "config.h"
#include "port.h"

/* System includes */
#include <stdlib.h>

/* Package includes */
#include "dma.h"
#include "fields.h"
#include "abstract.h"
#include "sched.h"
#include "pic.h"
#include "toplevel-support.h"
#include "sim-cmd.h"


/* We keep a copy of all our controllers because we have to export an interface
 * to other peripherals eg. ethernet */
static struct dma_controller *dmas = NULL;

static unsigned long dma_read_ch_csr (struct dma_channel *channel);
static void dma_write_ch_csr (struct dma_channel *channel,
			      unsigned long value);
static void dma_load_descriptor (struct dma_channel *channel);
static void dma_init_transfer (struct dma_channel *channel);
static void dma_channel_terminate_transfer (struct dma_channel *channel,
					    int generate_interrupt);

static void dma_channel_clock (void *dat);

static void masked_increase (oraddr_t * value, unsigned long mask);

#define CHANNEL_ND_I(ch) (TEST_FLAG(ch->regs.csr,DMA_CH_CSR,MODE) && TEST_FLAG(ch->regs.csr,DMA_CH_CSR,USE_ED) && ch->dma_nd_i)


/* Reset. Initializes all registers to default and places devices in memory address space. */
static void
dma_reset (void *dat)
{
  struct dma_controller *dma = dat;
  unsigned channel_number;

  memset (dma->ch, 0, sizeof (dma->ch));

  dma->regs.csr = 0;
  dma->regs.int_msk_a = 0;
  dma->regs.int_msk_b = 0;
  dma->regs.int_src_a = 0;
  dma->regs.int_src_b = 0;

  for (channel_number = 0; channel_number < DMA_NUM_CHANNELS;
       ++channel_number)
    {
      dma->ch[channel_number].controller = dma;
      dma->ch[channel_number].channel_number = channel_number;
      dma->ch[channel_number].channel_mask = 1LU << channel_number;
      dma->ch[channel_number].regs.am0 = dma->ch[channel_number].regs.am1 =
	0xFFFFFFFC;
    }
}

/* Print register values on stdout */
static void
dma_status (void *dat)
{
  unsigned j;
  struct dma_controller *dma = dat;

  if (dma->baseaddr == 0)
    return;

  PRINTF ("\nDMA controller at 0x%" PRIxADDR ":\n", dma->baseaddr);
  PRINTF ("CSR       : 0x%08lX\n", dma->regs.csr);
  PRINTF ("INT_MSK_A : 0x%08lX\n", dma->regs.int_msk_a);
  PRINTF ("INT_MSK_B : 0x%08lX\n", dma->regs.int_msk_b);
  PRINTF ("INT_SRC_A : 0x%08lX\n", dma->regs.int_src_a);
  PRINTF ("INT_SRC_B : 0x%08lX\n", dma->regs.int_src_b);

  for (j = 0; j < DMA_NUM_CHANNELS; ++j)
    {
      struct dma_channel *channel = &(dma->ch[j]);
      if (!channel->referenced)
	continue;
      PRINTF ("CH%u_CSR   : 0x%08lX\n", j, channel->regs.csr);
      PRINTF ("CH%u_SZ    : 0x%08lX\n", j, channel->regs.sz);
      PRINTF ("CH%u_A0    : 0x%08lX\n", j, channel->regs.a0);
      PRINTF ("CH%u_AM0   : 0x%08lX\n", j, channel->regs.am0);
      PRINTF ("CH%u_A1    : 0x%08lX\n", j, channel->regs.a1);
      PRINTF ("CH%u_AM1   : 0x%08lX\n", j, channel->regs.am1);
      PRINTF ("CH%u_DESC  : 0x%08lX\n", j, channel->regs.desc);
      PRINTF ("CH%u_SWPTR : 0x%08lX\n", j, channel->regs.swptr);
    }
}


/* Read a register */
static uint32_t
dma_read32 (oraddr_t addr, void *dat)
{
  struct dma_controller *dma = dat;
  uint32_t ret;

  if (addr < DMA_CH_BASE)
    {
      /* case of global (not per-channel) registers */
      switch (addr)
	{
	case DMA_CSR:
	  return dma->regs.csr;
	case DMA_INT_MSK_A:
	  return dma->regs.int_msk_a;
	case DMA_INT_MSK_B:
	  return dma->regs.int_msk_b;
	case DMA_INT_SRC_A:
	  if (dma->regs.int_src_a)
	    clear_interrupt (dma->irq);
	  ret = dma->regs.int_src_a;
	  dma->regs.int_src_a = 0;
	  return ret;
	case DMA_INT_SRC_B:
	  return dma->regs.int_src_b;
	default:
	  fprintf (stderr,
		   "dma_read32( 0x%" PRIxADDR " ): Illegal register\n",
		   addr + dma->baseaddr);
	  return 0;
	}
    }
  else
    {
      /* case of per-channel registers */
      unsigned chno = (addr - DMA_CH_BASE) / DMA_CH_SIZE;
      addr = (addr - DMA_CH_BASE) % DMA_CH_SIZE;
      switch (addr)
	{
	case DMA_CH_CSR:
	  return dma_read_ch_csr (&(dma->ch[chno]));
	case DMA_CH_SZ:
	  return dma->ch[chno].regs.sz;
	case DMA_CH_A0:
	  return dma->ch[chno].regs.a0;
	case DMA_CH_AM0:
	  return dma->ch[chno].regs.am0;
	case DMA_CH_A1:
	  return dma->ch[chno].regs.a1;
	case DMA_CH_AM1:
	  return dma->ch[chno].regs.am1;
	case DMA_CH_DESC:
	  return dma->ch[chno].regs.desc;
	case DMA_CH_SWPTR:
	  return dma->ch[chno].regs.swptr;
	}
    }
  return 0;
}


/* Handle read from a channel CSR */
static unsigned long
dma_read_ch_csr (struct dma_channel *channel)
{
  unsigned long result = channel->regs.csr;

  /* before returning, clear all relevant bits */
  CLEAR_FLAG (channel->regs.csr, DMA_CH_CSR, INT_CHUNK_DONE);
  CLEAR_FLAG (channel->regs.csr, DMA_CH_CSR, INT_DONE);
  CLEAR_FLAG (channel->regs.csr, DMA_CH_CSR, INT_ERR);
  CLEAR_FLAG (channel->regs.csr, DMA_CH_CSR, ERR);

  return result;
}



/* Write a register */
static void
dma_write32 (oraddr_t addr, uint32_t value, void *dat)
{
  struct dma_controller *dma = dat;

  /* case of global (not per-channel) registers */
  if (addr < DMA_CH_BASE)
    {
      switch (addr)
	{
	case DMA_CSR:
	  if (TEST_FLAG (value, DMA_CSR, PAUSE))
	    fprintf (stderr, "dma: PAUSE not implemented\n");
	  break;

	case DMA_INT_MSK_A:
	  dma->regs.int_msk_a = value;
	  break;
	case DMA_INT_MSK_B:
	  dma->regs.int_msk_b = value;
	  break;
	case DMA_INT_SRC_A:
	  dma->regs.int_src_a = value;
	  break;
	case DMA_INT_SRC_B:
	  dma->regs.int_src_b = value;
	  break;
	default:
	  fprintf (stderr,
		   "dma_write32( 0x%" PRIxADDR " ): Illegal register\n",
		   addr + dma->baseaddr);
	  return;
	}
    }
  else
    {
      /* case of per-channel registers */
      unsigned chno = (addr - DMA_CH_BASE) / DMA_CH_SIZE;
      struct dma_channel *channel = &(dma->ch[chno]);
      channel->referenced = 1;
      addr = (addr - DMA_CH_BASE) % DMA_CH_SIZE;
      switch (addr)
	{
	case DMA_CSR:
	  dma_write_ch_csr (&(dma->ch[chno]), value);
	  break;
	case DMA_CH_SZ:
	  channel->regs.sz = value;
	  break;
	case DMA_CH_A0:
	  channel->regs.a0 = value;
	  break;
	case DMA_CH_AM0:
	  channel->regs.am0 = value;
	  break;
	case DMA_CH_A1:
	  channel->regs.a1 = value;
	  break;
	case DMA_CH_AM1:
	  channel->regs.am1 = value;
	  break;
	case DMA_CH_DESC:
	  channel->regs.desc = value;
	  break;
	case DMA_CH_SWPTR:
	  channel->regs.swptr = value;
	  break;
	}
    }
}


/* Write a channel CSR
 * This ensures only the writable bits are modified.
 */
static void
dma_write_ch_csr (struct dma_channel *channel, unsigned long value)
{
  /* Check if we should *start* a transfer */
  if (!TEST_FLAG (channel->regs.csr, DMA_CH_CSR, CH_EN) &&
      TEST_FLAG (value, DMA_CH_CSR, CH_EN))
    SCHED_ADD (dma_channel_clock, channel, 1);
  else if (!TEST_FLAG (value, DMA_CH_CSR, CH_EN))
    /* The CH_EN flag is clear, check if we have a transfer in progress and
     * clear it */
    SCHED_FIND_REMOVE (dma_channel_clock, channel);

  /* Copy the writable bits to the channel CSR */
  channel->regs.csr &= ~DMA_CH_CSR_WRITE_MASK;
  channel->regs.csr |= value & DMA_CH_CSR_WRITE_MASK;
}



/* Clock tick for one channel on one DMA controller.
 * This does the actual "DMA" operation.
 * One chunk is transferred per clock.
 */
static void
dma_channel_clock (void *dat)
{
  struct dma_channel *channel = dat;

  /* Do we need to abort? */
  if (TEST_FLAG (channel->regs.csr, DMA_CH_CSR, STOP))
    {
      CLEAR_FLAG (channel->regs.csr, DMA_CH_CSR, CH_EN);
      CLEAR_FLAG (channel->regs.csr, DMA_CH_CSR, BUSY);
      SET_FLAG (channel->regs.csr, DMA_CH_CSR, ERR);

      if (TEST_FLAG (channel->regs.csr, DMA_CH_CSR, INE_ERR) &&
	  (channel->controller->regs.int_msk_a & channel->channel_mask))
	{
	  SET_FLAG (channel->regs.csr, DMA_CH_CSR, INT_ERR);
	  channel->controller->regs.int_src_a = channel->channel_mask;
	  report_interrupt (channel->controller->irq);
	}

      return;
    }

  /* In HW Handshake mode, only work when dma_req_i asserted */
  if (TEST_FLAG (channel->regs.csr, DMA_CH_CSR, MODE) && !channel->dma_req_i)
    {
      /* Reschedule */
      SCHED_ADD (dma_channel_clock, dat, 1);
      return;
    }

  /* If this is the first cycle of the transfer, initialize our state */
  if (!TEST_FLAG (channel->regs.csr, DMA_CH_CSR, BUSY))
    {
      CLEAR_FLAG (channel->regs.csr, DMA_CH_CSR, DONE);
      CLEAR_FLAG (channel->regs.csr, DMA_CH_CSR, ERR);
      SET_FLAG (channel->regs.csr, DMA_CH_CSR, BUSY);

      /* If using linked lists, copy the appropriate fields to our registers */
      if (TEST_FLAG (channel->regs.csr, DMA_CH_CSR, USE_ED))
	dma_load_descriptor (channel);
      else
	channel->load_next_descriptor_when_done = 0;

      /* Set our internal status */
      dma_init_transfer (channel);

      /* Might need to skip descriptor */
      if (CHANNEL_ND_I (channel))
	{
	  dma_channel_terminate_transfer (channel, 0);
	  return;
	}
    }

  /* Transfer one word */
  set_direct32 (channel->destination, eval_direct32 (channel->source, 0, 0),
		0, 0);

  /* Advance the source and destionation pointers */
  masked_increase (&(channel->source), channel->source_mask);
  masked_increase (&(channel->destination), channel->destination_mask);
  ++channel->words_transferred;

  /* Have we finished a whole chunk? */
  channel->dma_ack_o =
    (channel->words_transferred % channel->chunk_size == 0);

  /* When done with a chunk, check for dma_nd_i */
  if (CHANNEL_ND_I (channel))
    {
      dma_channel_terminate_transfer (channel, 0);
      return;
    }

  /* Are we done? */
  if (channel->words_transferred >= channel->total_size)
    {
      dma_channel_terminate_transfer (channel, 1);
      return;
    }

  /* Reschedule to transfer the next chunk */
  SCHED_ADD (dma_channel_clock, dat, 1);
}


/* Copy relevant valued from linked list descriptor to channel registers */
static void
dma_load_descriptor (struct dma_channel *channel)
{
  unsigned long desc_csr =
    eval_direct32 (channel->regs.desc + DMA_DESC_CSR, 0, 0);

  channel->load_next_descriptor_when_done =
    !TEST_FLAG (desc_csr, DMA_DESC_CSR, EOL);

  ASSIGN_FLAG (channel->regs.csr, DMA_CH_CSR, INC_SRC,
	       TEST_FLAG (desc_csr, DMA_DESC_CSR, INC_SRC));
  ASSIGN_FLAG (channel->regs.csr, DMA_CH_CSR, INC_DST,
	       TEST_FLAG (desc_csr, DMA_DESC_CSR, INC_DST));
  ASSIGN_FLAG (channel->regs.csr, DMA_CH_CSR, SRC_SEL,
	       TEST_FLAG (desc_csr, DMA_DESC_CSR, SRC_SEL));
  ASSIGN_FLAG (channel->regs.csr, DMA_CH_CSR, DST_SEL,
	       TEST_FLAG (desc_csr, DMA_DESC_CSR, DST_SEL));

  SET_FIELD (channel->regs.sz, DMA_CH_SZ, TOT_SZ,
	     GET_FIELD (desc_csr, DMA_DESC_CSR, TOT_SZ));

  channel->regs.a0 = eval_direct32 (channel->regs.desc + DMA_DESC_ADR0, 0, 0);
  channel->regs.a1 = eval_direct32 (channel->regs.desc + DMA_DESC_ADR1, 0, 0);

  channel->current_descriptor = channel->regs.desc;
  channel->regs.desc =
    eval_direct32 (channel->regs.desc + DMA_DESC_NEXT, 0, 0);
}


/* Initialize internal parameters used to implement transfers */
static void
dma_init_transfer (struct dma_channel *channel)
{
  channel->source = channel->regs.a0;
  channel->destination = channel->regs.a1;
  channel->source_mask =
    TEST_FLAG (channel->regs.csr, DMA_CH_CSR,
	       INC_SRC) ? channel->regs.am0 : 0;
  channel->destination_mask =
    TEST_FLAG (channel->regs.csr, DMA_CH_CSR,
	       INC_DST) ? channel->regs.am1 : 0;
  channel->total_size = GET_FIELD (channel->regs.sz, DMA_CH_SZ, TOT_SZ);
  channel->chunk_size = GET_FIELD (channel->regs.sz, DMA_CH_SZ, CHK_SZ);
  if (!channel->chunk_size || (channel->chunk_size > channel->total_size))
    channel->chunk_size = channel->total_size;
  channel->words_transferred = 0;
}


/* Take care of transfer termination */
static void
dma_channel_terminate_transfer (struct dma_channel *channel,
				int generate_interrupt)
{
  /* Might be working in a linked list */
  if (channel->load_next_descriptor_when_done)
    {
      dma_load_descriptor (channel);
      dma_init_transfer (channel);
      /* Reschedule */
      SCHED_ADD (dma_channel_clock, channel, 1);
      return;
    }

  /* Might be in auto-restart mode */
  if (TEST_FLAG (channel->regs.csr, DMA_CH_CSR, ARS))
    {
      dma_init_transfer (channel);
      return;
    }

  /* If needed, write amount of data transferred back to memory */
  if (TEST_FLAG (channel->regs.csr, DMA_CH_CSR, SZ_WB) &&
      TEST_FLAG (channel->regs.csr, DMA_CH_CSR, USE_ED))
    {
      /* TODO: What should we write back? Doc says "total number of remaining bytes" !? */
      unsigned long remaining_words =
	channel->total_size - channel->words_transferred;
      SET_FIELD (channel->regs.sz, DMA_DESC_CSR, TOT_SZ, remaining_words);
    }

  /* Mark end of transfer */
  CLEAR_FLAG (channel->regs.csr, DMA_CH_CSR, CH_EN);
  SET_FLAG (channel->regs.csr, DMA_CH_CSR, DONE);
  CLEAR_FLAG (channel->regs.csr, DMA_CH_CSR, ERR);
  CLEAR_FLAG (channel->regs.csr, DMA_CH_CSR, BUSY);

  /* If needed, generate interrupt */
  if (generate_interrupt)
    {
      /* TODO: Which channel should we interrupt? */
      if (TEST_FLAG (channel->regs.csr, DMA_CH_CSR, INE_DONE) &&
	  (channel->controller->regs.int_msk_a & channel->channel_mask))
	{
	  SET_FLAG (channel->regs.csr, DMA_CH_CSR, INT_DONE);
	  channel->controller->regs.int_src_a = channel->channel_mask;
	  report_interrupt (channel->controller->irq);
	}
    }
}

/* Utility function: Add 4 to a value with a mask */
static void
masked_increase (oraddr_t * value, unsigned long mask)
{
  *value = (*value & ~mask) | ((*value + 4) & mask);
}

/*-------------------------------------------[ DMA<->Peripheral interface ]---*/
/*
 * Simulation of control signals
 * To be used by simulations for other devices, e.g. ethernet
 */

void
set_dma_req_i (struct dma_channel *channel)
{
  channel->dma_req_i = 1;
}

void
clear_dma_req_i (struct dma_channel *channel)
{
  channel->dma_req_i = 0;
}

void
set_dma_nd_i (struct dma_channel *channel)
{
  channel->dma_nd_i = 1;
}

void
clear_dma_nd_i (struct dma_channel *channel)
{
  channel->dma_nd_i = 0;
}

unsigned
check_dma_ack_o (struct dma_channel *channel)
{
  return channel->dma_ack_o;
}

struct dma_channel *
find_dma_controller_ch (unsigned controller, unsigned channel)
{
  struct dma_controller *cur = dmas;

  while (cur && controller)
    {
      cur = cur->next;
      controller--;
    }

  if (!cur)
    return NULL;

  return &(cur->ch[channel]);
}


/*----------------------------------------------------[ DMA configuration ]---*/
static void
dma_baseaddr (union param_val val, void *dat)
{
  struct dma_controller *dma = dat;
  dma->baseaddr = val.addr_val;
}

static void
dma_irq (union param_val val, void *dat)
{
  struct dma_controller *dma = dat;
  dma->irq = val.int_val;
}

static void
dma_vapi_id (union param_val val, void *dat)
{
  struct dma_controller *dma = dat;
  dma->vapi_id = val.int_val;
}

static void
dma_enabled (union param_val val, void *dat)
{
  struct dma_controller *dma = dat;
  dma->enabled = val.int_val;
}


/*---------------------------------------------------------------------------*/
/*!Initialize a new DMA configuration

   ALL parameters are set explicitly to default values.                      */
/*---------------------------------------------------------------------------*/
static void *
dma_sec_start ()
{
  struct dma_controller *new = malloc (sizeof (struct dma_controller));

  if (!new)
    {
      fprintf (stderr, "Peripheral DMA: Run out of memory\n");
      exit (-1);
    }

  new->next = NULL;
  new->enabled  = 1;
  new->baseaddr = 0;
  new->irq      = 0;
  new->vapi_id  = 0;

  return new;

}	/* dma_sec_start() */

static void
dma_sec_end (void *dat)
{
  struct dma_controller *dma = dat;
  struct dma_controller *cur;
  struct mem_ops ops;

  if (!dma->enabled)
    {
      free (dat);
      return;
    }

  memset (&ops, 0, sizeof (struct mem_ops));

  ops.readfunc32 = dma_read32;
  ops.writefunc32 = dma_write32;
  ops.read_dat32 = dat;
  ops.write_dat32 = dat;

  /* FIXME: Correct delay?? */
  ops.delayr = 2;
  ops.delayw = 2;

  reg_mem_area (dma->baseaddr, DMA_ADDR_SPACE, 0, &ops);
  reg_sim_reset (dma_reset, dat);
  reg_sim_stat (dma_status, dat);

  if (dmas)
    {
      for (cur = dmas; cur->next; cur = cur->next);
      cur->next = dma;
    }
  else
    dmas = dma;
}

void
reg_dma_sec (void)
{
  struct config_section *sec =
    reg_config_sec ("dma", dma_sec_start, dma_sec_end);

  reg_config_param (sec, "enabled",  PARAMT_INT, dma_enabled);
  reg_config_param (sec, "baseaddr", PARAMT_ADDR, dma_baseaddr);
  reg_config_param (sec, "irq",      PARAMT_INT, dma_irq);
  reg_config_param (sec, "vapi_id",  PARAMT_ADDR, dma_vapi_id);
}
