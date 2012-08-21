/* mc_defines.h -- Defines for memory controller model

   Copyright (C) 2001 by Marko Mlinar, markom@opencores.org
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
#ifndef MC_DEFINES__H
#define MC_DEFINES__H

#define N_CE        8

#define MC_CSR      0x00
#define MC_POC      0x04
#define MC_BA_MASK  0x08
#define MC_CSC(i)   (0x10 + (i) * 8)
#define MC_TMS(i)   (0x14 + (i) * 8)

#define MC_ADDR_SPACE (MC_CSC(N_CE))

/* POC register field definition */
#define MC_POC_EN_BW_OFFSET  	  0
#define MC_POC_EN_BW_WIDTH	  2
#define MC_POC_EN_MEMTYPE_OFFSET  2
#define MC_POC_EN_MEMTYPE_WIDTH	  2

/* CSC register field definition */
#define MC_CSC_EN_OFFSET	  0
#define MC_CSC_MEMTYPE_OFFSET	  1
#define MC_CSC_MEMTYPE_WIDTH	  2 
#define MC_CSC_BW_OFFSET	  4
#define MC_CSC_BW_WIDTH		  2
#define MC_CSC_MS_OFFSET	  6
#define MC_CSC_MS_WIDTH		  2
#define MC_CSC_WP_OFFSET	  8
#define MC_CSC_BAS_OFFSET	  9
#define MC_CSC_KRO_OFFSET	 10
#define MC_CSC_PEN_OFFSET	 11
#define MC_CSC_SEL_OFFSET	 16
#define MC_CSC_SEL_WIDTH	  8

#define MC_CSC_MEMTYPE_SDRAM      0
#define MC_CSC_MEMTYPE_SSRAM      1
#define MC_CSC_MEMTYPE_ASYNC      2
#define MC_CSC_MEMTYPE_SYNC       3

#define MC_CE_VALID             (N_CE - 1)
#define MC_CSR_VALID		0xFF000703LU
#define MC_POC_VALID		0x0000000FLU
#define MC_BA_MASK_VALID	0x000003FFLU
#define MC_CSC_VALID		0x00FF0FFFLU
#define MC_TMS_SDRAM_VALID	0x0FFF83FFLU
#define MC_TMS_SSRAM_VALID	0x00000000LU
#define MC_TMS_ASYNC_VALID	0x03FFFFFFLU
#define MC_TMS_SYNC_VALID	0x01FFFFFFLU
#define MC_TMS_VALID		0xFFFFFFFFLU	/* reg test compat. */

/* TMS register field definition SDRAM */
#define MC_TMS_SDRAM_TRFC_OFFSET	24
#define MC_TMS_SDRAM_TRFC_WIDTH		 4
#define MC_TMS_SDRAM_TRP_OFFSET		20
#define MC_TMS_SDRAM_TRP_WIDTH		 4
#define MC_TMS_SDRAM_TRCD_OFFSET	17
#define MC_TMS_SDRAM_TRCD_WIDTH		 4
#define MC_TMS_SDRAM_TWR_OFFSET		15
#define MC_TMS_SDRAM_TWR_WIDTH		 2
#define MC_TMS_SDRAM_WBL_OFFSET		 9
#define MC_TMS_SDRAM_OM_OFFSET		 7
#define MC_TMS_SDRAM_OM_WIDTH		 2
#define MC_TMS_SDRAM_CL_OFFSET		 4
#define MC_TMS_SDRAM_CL_WIDTH		 3
#define MC_TMS_SDRAM_BT_OFFSET		 3
#define MC_TMS_SDRAM_BL_OFFSET		 0
#define MC_TMS_SDRAM_BL_WIDTH		 3

/* TMS register field definition ASYNC */
#define MC_TMS_ASYNC_TWWD_OFFSET	20
#define MC_TMS_ASYNC_TWWD_WIDTH		 6
#define MC_TMS_ASYNC_TWD_OFFSET		16
#define MC_TMS_ASYNC_TWD_WIDTH		 4
#define MC_TMS_ASYNC_TWPW_OFFSET	12
#define MC_TMS_ASYNC_TWPW_WIDTH		 4
#define MC_TMS_ASYNC_TRDZ_OFFSET	 8
#define MC_TMS_ASYNC_TRDZ_WIDTH		 4
#define MC_TMS_ASYNC_TRDV_OFFSET	 0
#define MC_TMS_ASYNC_TRDV_WIDTH		 8
 
/* TMS register field definition SYNC  */
#define MC_TMS_SYNC_TTO_OFFSET		16
#define MC_TMS_SYNC_TTO_WIDTH		 9
#define MC_TMS_SYNC_TWR_OFFSET		12
#define MC_TMS_SYNC_TWR_WIDTH		 4
#define MC_TMS_SYNC_TRDZ_OFFSET		 8
#define MC_TMS_SYNC_TRDZ_WIDTH		 4
#define MC_TMS_SYNC_TRDV_OFFSET		 0
#define MC_TMS_SYNC_TRDV_WIDTH		 8

#endif
