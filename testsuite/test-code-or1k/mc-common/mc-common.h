/* mc_common.h - Memory Controller testbench common routines defines

   Copyright (C) 2001 Ivan Guzvinec
   Copyright (C) 2010 Embecosm Limited
   
   Contributor Ivan Guzvinec <ivang@opencores.org>
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
   with this program.  If not, see <http:  www.gnu.org/licenses/>.  */

/* ----------------------------------------------------------------------------
   This code is commented throughout for use with Doxygen.
   --------------------------------------------------------------------------*/

#ifndef __MC_COMMON_H
#define __MC_COMMON_H

#define GPIO_BASE	0xA0000000LU
#define MC_BASE 	0x60000000LU
#define MC_MEM_BASE	0x04000000LU

/* Row Test flags */
#define	MC_TEST_RUN0	0x00000001LU
#define MC_TEST_RUN1	0x00000002LU
#define MC_TEST_RUN01	0x00000004LU
#define MC_TEST_RUN10	0x00000008LU
#define MC_TEST_RUNINC	0x00000010LU
#define MC_TEST_8	0x00000020LU
#define MC_TEST_16	0x00000040LU
#define MC_TEST_32	0x00000080LU
#define MC_TEST_SEQ	0x00000100LU
#define MC_TEST_SEQ1	0x00000200LU
#define MC_TEST_RAND	0x00000400LU

/* test pattern defines */
#define MC_TEST_PAT1_8	0x00U
#define MC_TEST_PAT2_8  0xFFU
#define MC_TEST_PAT3_8  0x55U
#define MC_TEST_PAT4_8  0xAAU
#define MC_TEST_PAT1_16	0x0000U
#define MC_TEST_PAT2_16	0xFFFFU
#define MC_TEST_PAT3_16	0x5555U
#define MC_TEST_PAT4_16	0xAAAAU
#define MC_TEST_PAT1_32 0x00000000LU
#define MC_TEST_PAT2_32 0xFFFFFFFFLU
#define MC_TEST_PAT3_32 0x55555555LU
#define MC_TEST_PAT4_32 0xAAAAAAAALU

/* test device defines */
#define MC_TEST_DEV_SDRAM	0
#define MC_TEST_DEV_SSRAM	1
#define MC_TEST_DEV_ASYNC	2
#define MC_TEST_DEV_SYNC	3

typedef volatile unsigned char  *MEMLOC8;
typedef volatile unsigned short *MEMLOC16;
typedef volatile unsigned long  *MEMLOC32;

/* Prototypes */
unsigned long mc_test_row(unsigned long nFrom, unsigned long nTo, unsigned long flags);
void randomin(unsigned long seed);
unsigned long random(unsigned long max);

#endif
