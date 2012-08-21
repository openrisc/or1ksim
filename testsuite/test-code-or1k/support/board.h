/* board.h -- Board definitions to match Or1ksim.

   Copyright (C) 2001 Simon Srot, srot@opencores.org
   Copyright (C) 2008, 2010 Embecosm Limited
  
   Contributor Simon Srot <srot@opencores.org>
   Contributor Jeremy Bennett <jeremy.bennett@embecosm.com>
  
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
   with this program.  If not, see <http://www.gnu.org/licenses/>. */

/* ----------------------------------------------------------------------------
   This code is commented throughout for use with Doxygen.
   --------------------------------------------------------------------------*/

#ifndef _BOARD_H_
#define _BOARD_H_

#define MC_CSR_VAL      0x0B000300
#define MC_MASK_VAL     0x000003f0
#define FLASH_BASE_ADDR 0xf0000000
#define FLASH_TMS_VAL   0x00000103
#define SDRAM_BASE_ADDR 0x00000000
#define SDRAM_TMS_VAL   0x19220057


#define UART_BASE  	0x90000000
#define UART_IRQ        2
#define ETH_BASE       	0x92000000
#define ETH_IRQ         4
#define KBD_BASE_ADD    0x94000000
#define KBD_IRQ         5
#define MC_BASE_ADDR    0x93000000
#define GENERIC_BASE    0x98000000
#define DMA_BASE        0xb8000000

#endif
