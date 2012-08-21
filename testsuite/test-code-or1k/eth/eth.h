/* eth.h -- OpenCores ethernet driver defines

   Copyright (C) 2001 Erez Volk, erez@mailandnews.comopencores.org
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

#ifndef ETH__H
#define ETH__H

/* Relative Register Addresses */
#define ETH_MODER	(4 * 0x00)
#define ETH_INT_SOURCE	(4 * 0x01)
#define ETH_INT_MASK	(4 * 0x02)
#define ETH_IPGT	(4 * 0x03)
#define ETH_IPGR1	(4 * 0x04)
#define ETH_IPGR2	(4 * 0x05)
#define ETH_PACKETLEN	(4 * 0x06)
#define ETH_COLLCONF	(4 * 0x07)
#define ETH_TX_BD_NUM	(4 * 0x08)
#define ETH_CTRLMODER	(4 * 0x09)
#define ETH_MIIMODER	(4 * 0x0A)
#define ETH_MIICOMMAND	(4 * 0x0B)
#define ETH_MIIADDRESS	(4 * 0x0C)
#define ETH_MIITX_DATA	(4 * 0x0D)
#define ETH_MIIRX_DATA	(4 * 0x0E)
#define ETH_MIISTATUS	(4 * 0x0F)
#define ETH_MAC_ADDR0	(4 * 0x10)
#define ETH_MAC_ADDR1	(4 * 0x11)
#define ETH_HASH0	(4 * 0x12)
#define ETH_HASH1	(4 * 0x13)

/* Where BD's are stored */
#define ETH_BD_BASE        0x400
#define ETH_BD_COUNT       0x100
#define ETH_BD_SPACE       (4 * ETH_BD_COUNT)

/* Where to point DMA to transmit/receive */
#define ETH_DMA_RX_TX      0x800

/* Field definitions for MODER */
#define ETH_MODER_DMAEN_OFFSET     17
#define ETH_MODER_RECSMALL_OFFSET  16
#define ETH_MODER_PAD_OFFSET       15
#define ETH_MODER_HUGEN_OFFSET     14
#define ETH_MODER_CRCEN_OFFSET     13
#define ETH_MODER_DLYCRCEN_OFFSET  12
#define ETH_MODER_RST_OFFSET       11
#define ETH_MODER_FULLD_OFFSET     10
#define ETH_MODER_EXDFREN_OFFSET   9
#define ETH_MODER_NOBCKOF_OFFSET   8
#define ETH_MODER_LOOPBCK_OFFSET   7
#define ETH_MODER_IFG_OFFSET	   6
#define ETH_MODER_PRO_OFFSET       5
#define ETH_MODER_IAM_OFFSET       4
#define ETH_MODER_BRO_OFFSET       3
#define ETH_MODER_NOPRE_OFFSET     2
#define ETH_MODER_TXEN_OFFSET      1
#define ETH_MODER_RXEN_OFFSET      0

/* Field definitions for INT_SOURCE */
#define ETH_INT_SOURCE_RXC_OFFSET  6
#define ETH_INT_SOURCE_TXC_OFFSET  5
#define ETH_INT_SOURCE_BUSY_OFFSET 4
#define ETH_INT_SOURCE_RXE_OFFSET  3
#define ETH_INT_SOURCE_RXB_OFFSET  2
#define ETH_INT_SOURCE_TXE_OFFSET  1
#define ETH_INT_SOURCE_TXB_OFFSET  0

/* Field definitions for INT_MASK */
#define ETH_INT_MASK_RXC_M_OFFSET  6
#define ETH_INT_MASK_TXC_M_OFFSET  5
#define ETH_INT_MASK_BUSY_M_OFFSET 4
#define ETH_INT_MASK_RXE_M_OFFSET  3
#define ETH_INT_MASK_RXB_M_OFFSET  2
#define ETH_INT_MASK_TXE_M_OFFSET  1
#define ETH_INT_MASK_TXB_M_OFFSET  0

/* Field definitions for PACKETLEN */
#define ETH_PACKETLEN_MINFL_OFFSET 16
#define ETH_PACKETLEN_MINFL_WIDTH  16
#define ETH_PACKETLEN_MAXFL_OFFSET 0
#define ETH_PACKETLEN_MAXFL_WIDTH  16

/* Field definitions for COLLCONF */
#define ETH_COLLCONF_MAXRET_OFFSET 16
#define ETH_COLLCONF_MAXRET_WIDTH  4
#define ETH_COLLCONF_COLLVALID_OFFSET 0
#define ETH_COLLCONF_COLLVALID_WIDTH  6

/* Field definitions for CTRLMODER */
#define ETH_CMODER_TXFLOW_OFFSET   2
#define ETH_CMODER_RXFLOW_OFFSET   1
#define ETH_CMODER_PASSALL_OFFSET  0

/* Field definitions for MIIMODER */
#define ETH_MIIMODER_MRST_OFFSET   9
#define ETH_MIIMODER_NOPRE_OFFSET  8
#define ETH_MIIMODER_CLKDIV_OFFSET 0
#define ETH_MIIMODER_CLKDIV_WIDTH  8

/* Field definitions for MIICOMMAND */
#define ETH_MIICOMM_WCDATA_OFFSET  2
#define ETH_MIICOMM_RSTAT_OFFSET   1
#define ETH_MIICOMM_SCANS_OFFSET   0

/* Field definitions for MIIADDRESS */
#define ETH_MIIADDR_RGAD_OFFSET	   8
#define ETH_MIIADDR_RGAD_WIDTH     5
#define ETH_MIIADDR_FIAD_OFFSET    0
#define ETH_MIIADDR_FIAD_WIDTH     5

/* Field definitions for MIISTATUS */
#define ETH_MIISTAT_NVALID_OFFSET  1
#define ETH_MIISTAT_BUSY_OFFSET    1
#define ETH_MIISTAT_FAIL_OFFSET    0

/* Field definitions for TX buffer descriptors */
#define ETH_TX_BD_LENGTH_OFFSET        16
#define ETH_TX_BD_LENGTH_WIDTH         16
#define ETH_TX_BD_READY_OFFSET         15
#define ETH_TX_BD_IRQ_OFFSET           14
#define ETH_TX_BD_WRAP_OFFSET          13
#define ETH_TX_BD_PAD_OFFSET           12
#define ETH_TX_BD_CRC_OFFSET           11
#define ETH_TX_BD_LAST_OFFSET          10
#define ETH_TX_BD_PAUSE_OFFSET         9
#define ETH_TX_BD_UNDERRUN_OFFSET      8
#define ETH_TX_BD_RETRY_OFFSET         4
#define ETH_TX_BD_RETRY_WIDTH          4
#define ETH_TX_BD_RETRANSMIT_OFFSET    3
#define ETH_TX_BD_COLLISION_OFFSET     2
#define ETH_TX_BD_DEFER_OFFSET         1
#define ETH_TX_BD_NO_CARRIER_OFFSET    0


/* Field definitions for RX buffer descriptors */
#define ETH_RX_BD_LENGTH_OFFSET        16
#define ETH_RX_BD_LENGTH_WIDTH         16
#define ETH_RX_BD_READY_OFFSET         15
#define ETH_RX_BD_IRQ_OFFSET           14
#define ETH_RX_BD_WRAP_OFFSET          13
#define ETH_RX_BD_MISS_OFFSET	       7
#define ETH_RX_BD_UVERRUN_OFFSET       6
#define ETH_RX_BD_INVALID_OFFSET       5
#define ETH_RX_BD_DRIBBLE_OFFSET       4
#define ETH_RX_BD_TOOBIG_OFFSET	       3
#define ETH_RX_BD_TOOSHORT_OFFSET      2
#define ETH_RX_BD_CRC_OFFSET           1
#define ETH_RX_BD_COLLISION_OFFSET     0

#endif
