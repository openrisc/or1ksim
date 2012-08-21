/* eth.c. Test of Or1ksim Ethernet

   Copyright (C) 1999-2006, 2010 OpenCores
   Copyright (C) 2010 Embecosm Limited

   Contributors various OpenCores participants
   Contributor Jeremy Bennett <jeremy.bennett@embecosm.com>
   Contributor Julius Baxter <julius@opencores.org>

   This file is part of OpenRISC 1000 Architectural Simulator.

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the Free
   Software Foundation; either version 3 of the License, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
   more details.

   You should have received a copy of the GNU General Public License along
   with this program.  If not, see <http:  www.gnu.org/licenses/>.  */

/* ----------------------------------------------------------------------------
   This code is commented throughout for use with Doxygen.
   --------------------------------------------------------------------------*/

/* TODO: Add loopback test. 
*/

#include "spr-defs.h"
#include "support.h"
#include "board.h"

typedef long off_t;

#include "fields.h"
#include "eth.h"

typedef volatile unsigned long *REGISTER;

REGISTER
    eth_moder = (unsigned long *)(ETH_BASE + ETH_MODER),
	eth_int_source = (unsigned long *)(ETH_BASE + ETH_INT_SOURCE),
	eth_int_mask = (unsigned long *)(ETH_BASE + ETH_INT_MASK),
	eth_ipgt = (unsigned long *)(ETH_BASE + ETH_IPGT),
	eth_ipgr1 = (unsigned long *)(ETH_BASE + ETH_IPGR1),
	eth_ipgr2 = (unsigned long *)(ETH_BASE + ETH_IPGR2),
	eth_packetlen = (unsigned long *)(ETH_BASE + ETH_PACKETLEN),
	eth_collconf = (unsigned long *)(ETH_BASE + ETH_COLLCONF),
	eth_tx_bd_num = (unsigned long *)(ETH_BASE + ETH_TX_BD_NUM),
	eth_controlmoder = (unsigned long *)(ETH_BASE + ETH_CTRLMODER),
	eth_miimoder = (unsigned long *)(ETH_BASE + ETH_MIIMODER),
	eth_miicommand = (unsigned long *)(ETH_BASE + ETH_MIICOMMAND),
	eth_miiaddress = (unsigned long *)(ETH_BASE + ETH_MIIADDRESS),
	eth_miitx_data = (unsigned long *)(ETH_BASE + ETH_MIITX_DATA),
	eth_miirx_data = (unsigned long *)(ETH_BASE + ETH_MIIRX_DATA),
	eth_miistatus = (unsigned long *)(ETH_BASE + ETH_MIISTATUS),
	eth_mac_addr0 = (unsigned long *)(ETH_BASE + ETH_MAC_ADDR0),
	eth_mac_addr1 = (unsigned long *)(ETH_BASE + ETH_MAC_ADDR1),
	eth_bd_base = (unsigned long *)(ETH_BASE + ETH_BD_BASE);

volatile unsigned int_happend;
#define R_PACKET_SIZE 2000
unsigned char r_packet[R_PACKET_SIZE];
#define S_PACKET_SIZE 1003
unsigned char s_packet[S_PACKET_SIZE];
unsigned tx_bindex;
unsigned rx_bindex;

void interrupt_handler()
{
    unsigned i,len;
    
    printf ("Int\n");
    switch (*eth_int_source & 0x7f) {
    case 0x2 : 
	printf ("Transmit Error.\n");
	*eth_int_source = 0x2;
	 break;
    case 0x8 : 
	printf ("Receive Error\n");
	*eth_int_source = 0x8;
	break;
    case 0x4 : 
        printf ("Receive Frame\n");
	*eth_int_source = 0x4;

        CLEAR_FLAG(*eth_moder, ETH_MODER, RXEN);
    
        len = GET_FIELD(eth_bd_base[(*eth_tx_bd_num << 1) + 2], ETH_RX_BD, LENGTH);
        for (i=0; i<len; i++)
          if (r_packet[i] != (unsigned char)i)
          {
              printf("Failed at byte %d. expect %d, received %d\n", i, i, r_packet[i]);
              exit(1);
          }
        break;
    case 0x10: 
	printf ("Busy\n");
	*eth_int_source = 0x10;
	break;
    case 0x1 : 
        printf ("Transmit Frame.\n");
        *eth_int_source = 0x1;
	CLEAR_FLAG(*eth_moder, ETH_MODER, TXEN);
        
        break;
    default:
        printf ("Invalid int @ %0x\n", (unsigned int)*eth_int_source & 0x7f);
      	*eth_int_source = 0x7f;
	exit (1);
    }

    mtspr(SPR_PICSR, 0);
    int_happend = 1;
}

static void set_mac( void )
{
	*eth_mac_addr0 = 0x04030201LU;
	*eth_mac_addr1 = 0x00000605LU;
}

static void transmit_one_packet( void )
{
	unsigned i;

	/* Initialize packet */
	for ( i = 0; i < S_PACKET_SIZE; ++ i )
		s_packet[i] = (unsigned char)i;

	/* Set Ethernet BD */
	SET_FIELD(eth_bd_base[tx_bindex], ETH_TX_BD, LENGTH, S_PACKET_SIZE);
	eth_bd_base[tx_bindex + 1] = (unsigned long)s_packet;

	/* Start Ethernet */
	SET_FLAG(eth_bd_base[tx_bindex], ETH_TX_BD, READY);
	SET_FLAG(*eth_moder, ETH_MODER, TXEN);
	
	/* Now wait till sent */
	while ( TEST_FLAG( eth_bd_base[tx_bindex], ETH_TX_BD, READY ) );
	CLEAR_FLAG(*eth_moder, ETH_MODER, TXEN);
	*eth_int_source = 0x7f;
}

static void transmit_one_packet_int( void )
{
	unsigned i;	
	
	int_happend = 0;
	/* Initialize packet */
	printf("Init\n");
	for ( i = 0; i < S_PACKET_SIZE; ++ i )
		s_packet[i] = (unsigned char)i;

	/* Set Ethernet BD */
	printf("Set BD\n");
	SET_FIELD(eth_bd_base[tx_bindex], ETH_TX_BD, LENGTH, S_PACKET_SIZE);
	eth_bd_base[tx_bindex + 1] = (unsigned long)s_packet;

	/* Start Ethernet */
	printf("Set Flags\n");
	SET_FLAG(eth_bd_base[tx_bindex], ETH_TX_BD, IRQ);
	SET_FLAG(eth_bd_base[tx_bindex], ETH_TX_BD, READY);
	SET_FLAG(*eth_moder, ETH_MODER, TXEN);	
}


static void receive_one_packet(void)
{
  unsigned int i;
  unsigned int len;  
  
  eth_bd_base[rx_bindex + 1] = (unsigned long)r_packet;
  
  SET_FLAG(eth_bd_base[rx_bindex], ETH_RX_BD, READY);
  SET_FLAG(*eth_moder, ETH_MODER, RXEN);

  while ( TEST_FLAG( eth_bd_base[rx_bindex], ETH_RX_BD, READY ) );  
  CLEAR_FLAG(*eth_moder, ETH_MODER, RXEN);
  *eth_int_source = 0x7f;

  len = GET_FIELD(eth_bd_base[rx_bindex], ETH_RX_BD, LENGTH);
  for (i=0; i<len; i++)
      if (r_packet[i] != (unsigned char)i)
      {
          printf("Failed at byte %d. expect %d, received %d\n", i, i, r_packet[i]);
          exit(1);
      }
}

static void receive_one_packet_int(void)
{
  int_happend = 0;
  printf("Set BD\n");
  eth_bd_base[rx_bindex + 1] = (unsigned long)r_packet;
  
  printf("SetFlags\n");
  SET_FLAG(eth_bd_base[rx_bindex], ETH_RX_BD, IRQ);
  SET_FLAG(eth_bd_base[rx_bindex], ETH_RX_BD, READY);
  SET_FLAG(*eth_moder, ETH_MODER, RXEN);
}

int main()
{
	printf( "Starting Ethernet test\n" );

	/* Buffer descriptor indexes. These are not changed in between the 
	polling tests, as the RXEN and TXEN bits in the MODER are disabled
	between tests, resetting the respective buffer descriptor indexes,
	and so these should stay at their initial values. */
	tx_bindex = 0;
	rx_bindex = *eth_tx_bd_num << 1;

	set_mac();

	/* Set promiscuous mode */
	*eth_moder |= (1 << ETH_MODER_PRO_OFFSET);

	/*-------------------*/
	/* non iterrupt test */
	transmit_one_packet();

	receive_one_packet();

	transmit_one_packet();

	receive_one_packet();
	
	
	/*-------------------*/
	/* interrupt test    */
	excpt_int = (unsigned long)interrupt_handler;
	/* Enable interrup	ts */
	printf("enable ints\n");
	mtspr (SPR_SR, mfspr(SPR_SR) | SPR_SR_IEE);
	mtspr (SPR_PICMR, mfspr(SPR_PICMR) | (0x00000001L << ETH_IRQ));
	
	printf("set mask flags TX\n");
	SET_FLAG(*eth_int_mask, ETH_INT_MASK, TXB_M);
	transmit_one_packet_int();
	tx_bindex += 2;
	/* printf("waiting for int\n"); */
	while (!int_happend);
	CLEAR_FLAG(*eth_int_mask, ETH_INT_MASK, TXB_M);
	
	printf("seting mask flag RX\n");
	SET_FLAG(*eth_int_mask, ETH_INT_MASK, RXB_M);
	receive_one_packet_int();
	rx_bindex += 2;
	/* printf("waiting for int\n"); */
	while (!int_happend);
	CLEAR_FLAG(*eth_int_mask, ETH_INT_MASK, RXB_M);
	
	
	printf( "Ending Ethernet test\n" );
    
	report (0xdeaddead);
	exit(0);
}


