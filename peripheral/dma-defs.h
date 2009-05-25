/* dma-defs.h -- Definition of relative addresses/register bits for DMA

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


#ifndef DMA_DEFS__H
#define DMA_DEFS__H

/* Number of channel per DMA controller */
#define DMA_NUM_CHANNELS  31

/* Address space required by one DMA controller */
#define DMA_ADDR_SPACE  0x400

/* Relative Register Addresses */
#define DMA_CSR        0x00
#define DMA_INT_MSK_A  0x04
#define DMA_INT_MSK_B  0x08
#define DMA_INT_SRC_A  0x0C
#define DMA_INT_SRC_B  0x10

/* Channel registers definitions */
#define DMA_CH_BASE  0x20	/*!< Offset of first channel registers */
#define DMA_CH_SIZE  0x20	/*!< Per-channel address space */

/* Per-channel Register Addresses, relative to channel start */
#define DMA_CH_CSR    0x00
#define DMA_CH_SZ     0x04
#define DMA_CH_A0     0x08
#define DMA_CH_AM0    0x0C
#define DMA_CH_A1     0x10
#define DMA_CH_AM1    0x14
#define DMA_CH_DESC   0x18
#define DMA_CH_SWPTR  0x1C

/* Field Definitions for the Main CSR */
#define DMA_CSR_PAUSE_OFFSET             0

/* Field Definitions for the Channel CSR(s) */
#define DMA_CH_CSR_CH_EN_OFFSET           0
#define DMA_CH_CSR_DST_SEL_OFFSET         1
#define DMA_CH_CSR_SRC_SEL_OFFSET         2
#define DMA_CH_CSR_INC_DST_OFFSET         3
#define DMA_CH_CSR_INC_SRC_OFFSET         4
#define DMA_CH_CSR_MODE_OFFSET            5
#define DMA_CH_CSR_ARS_OFFSET             6
#define DMA_CH_CSR_USE_ED_OFFSET          7
#define DMA_CH_CSR_SZ_WB_OFFSET           8
#define DMA_CH_CSR_STOP_OFFSET            9
#define DMA_CH_CSR_BUSY_OFFSET           10
#define DMA_CH_CSR_DONE_OFFSET           11
#define DMA_CH_CSR_ERR_OFFSET            12
#define DMA_CH_CSR_PRIORITY_OFFSET       13
#define DMA_CH_CSR_PRIORITY_WIDTH         3
#define DMA_CH_CSR_REST_EN_OFFSET        16
#define DMA_CH_CSR_INE_ERR_OFFSET        17
#define DMA_CH_CSR_INE_DONE_OFFSET       18
#define DMA_CH_CSR_INE_CHK_DONE_OFFSET   19
#define DMA_CH_CSR_INT_ERR_OFFSET        20
#define DMA_CH_CSR_INT_DONE_OFFSET       21
#define DMA_CH_CSR_INT_CHUNK_DONE_OFFSET 22
#define DMA_CH_CSR_RESERVED_OFFSET       23
#define DMA_CH_CSR_RESERVED_WIDTH         9

/* Masks -- Writable and readonly parts of the register */
#define DMA_CH_CSR_WRITE_MASK            0x000FE3FF

/* Field definitions for Channel Size Registers */
#define DMA_CH_SZ_TOT_SZ_OFFSET           0
#define DMA_CH_SZ_TOT_SZ_WIDTH           12
#define DMA_CH_SZ_CHK_SZ_OFFSET          16
#define DMA_CH_SZ_CHK_SZ_WIDTH            9

/* Field definitions for Channel Address Registers CHn_Am */
#define DMA_CH_A0_ADDR_OFFSET             2
#define DMA_CH_A0_ADDR_WIDTH             30
#define DMA_CH_A1_ADDR_OFFSET             2
#define DMA_CH_A1_ADDR_WIDTH             30

/* Field definitions for Channel Address Mask Registers CHn_AMm */
#define DMA_CH_AM0_MASK_OFFSET            4
#define DMA_CH_AM0_MASK_WIDTH            28
#define DMA_CH_AM1_MASK_OFFSET            4
#define DMA_CH_AM1_MASK_WIDTH            28

/* Field definitions for Channel Linked List descriptor Pointer CHn_DESC */
#define DMA_CH_DESC_ADDR_OFFSET           2
#define DMA_CH_DESC_ADDR_WIDTH           30

/* Field definitions for Channel Software Pointer */
#define DMA_CH_SWPTR_PTR_OFFSET           2
#define DMA_CH_SWPTR_PTR_WIDTH           29
#define DMA_CH_SWPTR_EN_OFFSET           31


/* Structure of linked list descriptors (offsets of elements)  */
#define DMA_DESC_CSR                     0x00
#define DMA_DESC_ADR0                    0x04
#define DMA_DESC_ADR1                    0x08
#define DMA_DESC_NEXT                    0x0C

/* Field definitions for linked list descriptor DESC_CSR */
#define DMA_DESC_CSR_EOL_OFFSET          20
#define DMA_DESC_CSR_INC_SRC_OFFSET      19
#define DMA_DESC_CSR_INC_DST_OFFSET      18
#define DMA_DESC_CSR_SRC_SEL_OFFSET      17
#define DMA_DESC_CSR_DST_SEL_OFFSET      16
#define DMA_DESC_CSR_TOT_SZ_OFFSET        0
#define DMA_DESC_CSR_TOT_SZ_WIDTH        12

#endif	/* DMA_DEFS__H */
