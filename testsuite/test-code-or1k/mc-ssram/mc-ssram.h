/* mc_ssram.h - Memory Controller testbench SSRAM defines

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

#ifndef __MC_SSRAM_H
#define __MC_SSRAM_H

/* should configuration be read form MC? */
#define MC_READ_CONF

/* TEMPLATE SELECTION       */
/* change #undef to #define */
#define _MC_TEST_TEMPLATE1
#undef  _MC_TEST_TEMPLATE2
#undef  _MC_TEST_TEMPLATE3
/* ------------------------ */

/* memory configuration that must reflect mcmem.cfg */
#define MC_SSRAM_CSMASK	0xFE	/* 8 bit mask for 8 chip selects. 1 SSRAM at CS, 0 something else at CS */

typedef struct MC_SSRAM_CS
{
  unsigned long M;
} MC_SSRAM_CS;

MC_SSRAM_CS mc_ssram_cs[8] = {
  {  0x02  /* SELect mask */
    },
  { 0x04 },
  { 0x06 },
  { 0x08 },
  { 0x0A },
  { 0x0C },
  { 0x0E },
  { 0x10 } };

#define MC_SSRAM_TEST0	0x00000001LU /* no parity, bank after column, write enable */
#define MC_SSRAM_TEST1	0x00000002LU /* parity */
#define MC_SSRAM_TEST2	0x00000004LU /* bank after row */
#define MC_SSRAM_TEST3	0x00000008LU /* RO */
#define MC_SSRAM_TEST4	0x00000010LU /* - NOT DEFINED - */
#define MC_SSRAM_TEST5	0x00000020LU /* - NOT DEFINED - */

#ifdef _MC_TEST_TEMPLATE1
  #define MC_SSRAM_FLAGS	0x000002B8LU	/* MC_TEST_ flags... see mc_common.h */ 
  #define MC_SSRAM_TESTS	0x00000005LU
#endif

#ifdef _MC_TEST_TEMPLATE2
  #define MC_SSRAM_FLAGS	0x00000128LU	/* MC_TEST_ flags... see mc_common.h */ 
  #define MC_SSRAM_TESTS	0x00000008LU
#endif

#ifdef _MC_TEST_TEMPLATE3
  #define MC_SSRAM_FLAGS	0x000007FFLU	/* MC_TEST_ flags... see mc_common.h */ 
  #define MC_SSRAM_TESTS	0x0000003FLU
#endif

#endif
