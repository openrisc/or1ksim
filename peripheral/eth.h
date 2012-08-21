/* eth.h -- Simulation of Ethernet MAC header

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

#if HAVE_NET_ETHERNET_H
# include <net/ethernet.h>
#elif defined(HAVE_SYS_ETHERNET_H)
# include <sys/ethernet.h>
#else /* !HAVE_NET_ETHERNET_H && !HAVE_SYS_ETHERNET_H - */
#include <sys/types.h>
#endif

/* Address space required by one Ethernet MAC */
#define ETH_ADDR_SPACE 0x1000

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
#define ETH_MIIMODER_NOPRE_OFFSET  8
#define ETH_MIIMODER_CLKDIV_OFFSET 0
#define ETH_MIIMODER_CLKDIV_MASK   0xff

/* Field definitions for MIICOMMAND */
#define ETH_MIICOMM_WCDATA_OFFSET  2
#define ETH_MIICOMM_RSTAT_OFFSET   1
#define ETH_MIICOMM_SCANS_OFFSET   0

/* Field definitions for MIIADDRESS */
#define ETH_MIIADDR_RGAD_OFFSET	   8
#define ETH_MIIADDR_RGAD_MASK     0x1f     
#define ETH_MIIADDR_FIAD_OFFSET    0
#define ETH_MIIADDR_FIAD_MASK     0x1f

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

/*
 * Ethernet protocol definitions
 */
#ifdef HAVE_NET_ETHERNET_H
#elif defined(HAVE_SYS_ETHERNET_H)
#ifndef ETHER_ADDR_LEN
#define ETHER_ADDR_LEN ETHERADDRL
#endif
#ifndef ETHER_HDR_LEN
#define ETHER_HDR_LEN sizeof(struct ether_header)
#endif
#else /* !HAVE_NET_ETHERNET_H && !HAVE_SYS_ETHERNET_H - */
#ifdef __CYGWIN__
/* define some missing cygwin defines.
 */
#define ETH_HLEN      14
#endif /* __CYGWIN__ */

#define ETH_ALEN    6

struct ether_addr
{
  u_int8_t ether_addr_octet[ETH_ALEN];
};

struct ether_header
{
  u_int8_t ether_dhost[ETH_ALEN];	/* destination eth addr */
  u_int8_t ether_shost[ETH_ALEN];	/* source ether addr    */
  u_int16_t ether_type;		/* packet type ID field */
};

/* Ethernet protocol ID's */
#define	ETHERTYPE_PUP		0x0200	/* Xerox PUP */
#define	ETHERTYPE_IP		0x0800	/* IP */
#define	ETHERTYPE_ARP		0x0806	/* Address resolution */
#define	ETHERTYPE_REVARP	0x8035	/* Reverse ARP */

#define	ETHER_ADDR_LEN	ETH_ALEN	/* size of ethernet addr */
#define	ETHER_TYPE_LEN	2	/* bytes in type field */
#define	ETHER_CRC_LEN	4	/* bytes in CRC field */
#define	ETHER_HDR_LEN	ETH_HLEN	/* total octets in header */
#define	ETHER_MIN_LEN	(ETH_ZLEN + ETHER_CRC_LEN)	/* min packet length */
#define	ETHER_MAX_LEN	(ETH_FRAME_LEN + ETHER_CRC_LEN)	/* max packet length */

/* make sure ethenet length is valid */
#define	ETHER_IS_VALID_LEN(foo)	\
	((foo) >= ETHER_MIN_LEN && (foo) <= ETHER_MAX_LEN)

/*
 * The ETHERTYPE_NTRAILER packet types starting at ETHERTYPE_TRAIL have
 * (type-ETHERTYPE_TRAIL)*512 bytes of data followed
 * by an ETHER type (as given above) and then the (variable-length) header.
 */
#define	ETHERTYPE_TRAIL		0x1000	/* Trailer packet */
#define	ETHERTYPE_NTRAILER	16

#define	ETHERMTU	ETH_DATA_LEN
#define	ETHERMIN	(ETHER_MIN_LEN-ETHER_HDR_LEN-ETHER_CRC_LEN)

#endif /* HAVE_NET_ETHERNET_H */

/*
 * Implementatino of Ethernet MAC Registers and State
 */
#define ETH_RTX_FILE    0
#define ETH_RTX_TAP     1
#define ETH_RTX_VAPI	2

#define ETH_MAXPL   0x10000

enum
{ ETH_VAPI_DATA = 0,
  ETH_VAPI_CTRL,
  ETH_NUM_VAPI_IDS
};


/* Prototypes for external use */
extern void  reg_ethernet_sec ();


/* Defines taken from linux-2.6.36/include/linux/mii.h for PHY register IF */

#define MII_BMCR            0x00        /* Basic mode control register */
#define MII_BMSR            0x01        /* Basic mode status register  */
#define MII_PHYSID1         0x02        /* PHYS ID 1                   */
#define MII_PHYSID2         0x03        /* PHYS ID 2                   */
#define MII_ADVERTISE       0x04        /* Advertisement control reg   */
#define MII_LPA             0x05        /* Link partner ability reg    */
#define MII_EXPANSION       0x06        /* Expansion register          */
#define MII_CTRL1000        0x09        /* 1000BASE-T control          */
#define MII_STAT1000        0x0a        /* 1000BASE-T status           */
#define MII_ESTATUS	    0x0f	/* Extended Status */
#define MII_DCOUNTER        0x12        /* Disconnect counter          */
#define MII_FCSCOUNTER      0x13        /* False carrier counter       */
#define MII_NWAYTEST        0x14        /* N-way auto-neg test reg     */
#define MII_RERRCOUNTER     0x15        /* Receive error counter       */
#define MII_SREVISION       0x16        /* Silicon revision            */
#define MII_RESV1           0x17        /* Reserved...                 */
#define MII_LBRERROR        0x18        /* Lpback, rx, bypass error    */
#define MII_PHYADDR         0x19        /* PHY address                 */
#define MII_RESV2           0x1a        /* Reserved...                 */
#define MII_TPISTATUS       0x1b        /* TPI status for 10mbps       */
#define MII_NCONFIG         0x1c        /* Network interface config    */

/* Basic mode control register. */
#define BMCR_RESV               0x003f  /* Unused...                   */
#define BMCR_SPEED1000		0x0040  /* MSB of Speed (1000)         */
#define BMCR_CTST               0x0080  /* Collision test              */
#define BMCR_FULLDPLX           0x0100  /* Full duplex                 */
#define BMCR_ANRESTART          0x0200  /* Auto negotiation restart    */
#define BMCR_ISOLATE            0x0400  /* Disconnect DP83840 from MII */
#define BMCR_PDOWN              0x0800  /* Powerdown the DP83840       */
#define BMCR_ANENABLE           0x1000  /* Enable auto negotiation     */
#define BMCR_SPEED100           0x2000  /* Select 100Mbps              */
#define BMCR_LOOPBACK           0x4000  /* TXD loopback bits           */
#define BMCR_RESET              0x8000  /* Reset the DP83840           */

/* Basic mode status register. */
#define BMSR_ERCAP              0x0001  /* Ext-reg capability          */
#define BMSR_JCD                0x0002  /* Jabber detected             */
#define BMSR_LSTATUS            0x0004  /* Link status                 */
#define BMSR_ANEGCAPABLE        0x0008  /* Able to do auto-negotiation */
#define BMSR_RFAULT             0x0010  /* Remote fault detected       */
#define BMSR_ANEGCOMPLETE       0x0020  /* Auto-negotiation complete   */
#define BMSR_RESV               0x00c0  /* Unused...                   */
#define BMSR_ESTATEN		0x0100	/* Extended Status in R15 */
#define BMSR_100HALF2           0x0200  /* Can do 100BASE-T2 HDX */
#define BMSR_100FULL2           0x0400  /* Can do 100BASE-T2 FDX */
#define BMSR_10HALF             0x0800  /* Can do 10mbps, half-duplex  */
#define BMSR_10FULL             0x1000  /* Can do 10mbps, full-duplex  */
#define BMSR_100HALF            0x2000  /* Can do 100mbps, half-duplex */
#define BMSR_100FULL            0x4000  /* Can do 100mbps, full-duplex */
#define BMSR_100BASE4           0x8000  /* Can do 100mbps, 4k packets  */

/* Advertisement control register. */
#define ADVERTISE_SLCT          0x001f  /* Selector bits               */
#define ADVERTISE_CSMA          0x0001  /* Only selector supported     */
#define ADVERTISE_10HALF        0x0020  /* Try for 10mbps half-duplex  */
#define ADVERTISE_1000XFULL     0x0020  /* Try for 1000BASE-X full-duplex */
#define ADVERTISE_10FULL        0x0040  /* Try for 10mbps full-duplex  */
#define ADVERTISE_1000XHALF     0x0040  /* Try for 1000BASE-X half-duplex */
#define ADVERTISE_100HALF       0x0080  /* Try for 100mbps half-duplex */
#define ADVERTISE_1000XPAUSE    0x0080  /* Try for 1000BASE-X pause    */
#define ADVERTISE_100FULL       0x0100  /* Try for 100mbps full-duplex */
#define ADVERTISE_1000XPSE_ASYM 0x0100  /* Try for 1000BASE-X asym pause */
#define ADVERTISE_100BASE4      0x0200  /* Try for 100mbps 4k packets  */
#define ADVERTISE_PAUSE_CAP     0x0400  /* Try for pause               */
#define ADVERTISE_PAUSE_ASYM    0x0800  /* Try for asymetric pause     */
#define ADVERTISE_RESV          0x1000  /* Unused...                   */
#define ADVERTISE_RFAULT        0x2000  /* Say we can detect faults    */
#define ADVERTISE_LPACK         0x4000  /* Ack link partners response  */
#define ADVERTISE_NPAGE         0x8000  /* Next page bit               */

#define ADVERTISE_FULL (ADVERTISE_100FULL | ADVERTISE_10FULL | \
			ADVERTISE_CSMA)
#define ADVERTISE_ALL (ADVERTISE_10HALF | ADVERTISE_10FULL | \
                       ADVERTISE_100HALF | ADVERTISE_100FULL)

/* Link partner ability register. */
#define LPA_SLCT                0x001f  /* Same as advertise selector  */
#define LPA_10HALF              0x0020  /* Can do 10mbps half-duplex   */
#define LPA_1000XFULL           0x0020  /* Can do 1000BASE-X full-duplex */
#define LPA_10FULL              0x0040  /* Can do 10mbps full-duplex   */
#define LPA_1000XHALF           0x0040  /* Can do 1000BASE-X half-duplex */
#define LPA_100HALF             0x0080  /* Can do 100mbps half-duplex  */
#define LPA_1000XPAUSE          0x0080  /* Can do 1000BASE-X pause     */
#define LPA_100FULL             0x0100  /* Can do 100mbps full-duplex  */
#define LPA_1000XPAUSE_ASYM     0x0100  /* Can do 1000BASE-X pause asym*/
#define LPA_100BASE4            0x0200  /* Can do 100mbps 4k packets   */
#define LPA_PAUSE_CAP           0x0400  /* Can pause                   */
#define LPA_PAUSE_ASYM          0x0800  /* Can pause asymetrically     */
#define LPA_RESV                0x1000  /* Unused...                   */
#define LPA_RFAULT              0x2000  /* Link partner faulted        */
#define LPA_LPACK               0x4000  /* Link partner acked us       */
#define LPA_NPAGE               0x8000  /* Next page bit               */

#define LPA_DUPLEX		(LPA_10FULL | LPA_100FULL)
#define LPA_100			(LPA_100FULL | LPA_100HALF | LPA_100BASE4)

/* Expansion register for auto-negotiation. */
#define EXPANSION_NWAY          0x0001  /* Can do N-way auto-nego      */
#define EXPANSION_LCWP          0x0002  /* Got new RX page code word   */
#define EXPANSION_ENABLENPAGE   0x0004  /* This enables npage words    */
#define EXPANSION_NPCAPABLE     0x0008  /* Link partner supports npage */
#define EXPANSION_MFAULTS       0x0010  /* Multiple faults detected    */
#define EXPANSION_RESV          0xffe0  /* Unused...                   */

#define ESTATUS_1000_TFULL	0x2000	/* Can do 1000BT Full */
#define ESTATUS_1000_THALF	0x1000	/* Can do 1000BT Half */

/* N-way test register. */
#define NWAYTEST_RESV1          0x00ff  /* Unused...                   */
#define NWAYTEST_LOOPBACK       0x0100  /* Enable loopback for N-way   */
#define NWAYTEST_RESV2          0xfe00  /* Unused...                   */

/* 1000BASE-T Control register */
#define ADVERTISE_1000FULL      0x0200  /* Advertise 1000BASE-T full duplex */
#define ADVERTISE_1000HALF      0x0100  /* Advertise 1000BASE-T half duplex */

/* 1000BASE-T Status register */
#define LPA_1000LOCALRXOK       0x2000  /* Link partner local receiver status */
#define LPA_1000REMRXOK         0x1000  /* Link partner remote receiver status */
#define LPA_1000FULL            0x0800  /* Link partner 1000BASE-T full duplex */
#define LPA_1000HALF            0x0400  /* Link partner 1000BASE-T half duplex */

/* Flow control flags */
#define FLOW_CTRL_TX		0x01
#define FLOW_CTRL_RX		0x02



#endif /* ETH__H */
