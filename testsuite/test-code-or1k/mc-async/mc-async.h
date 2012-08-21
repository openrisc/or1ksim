/* mc_async.h - Memory Controller testbench ASYNCdevices defines

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

#ifndef __MC_ASYNC_H
#define __MC_ASYNC_H

/* should configuration be readm from MC? */
#define MC_READ_CONF

/* TEMPLATE SELECTION       */
/* change #undef to #define */
#define _MC_TEST_TEMPLATE1
#undef  _MC_TEST_TEMPLATE2
#undef  _MC_TEST_TEMPLATE3
/* ------------------------ */

/* memory configuration that must reflect mcmem.cfg */
#define MC_ASYNC_CSMASK	0xFE	/* 8 bit mask for 8 chip selects. 1 ASYNC at CS, 0 something else at CS */

typedef struct MC_ASYNC_CS
{
  unsigned long BW;
  unsigned long M;
} MC_ASYNC_CS;

MC_ASYNC_CS mc_async_cs[8] = {
  { 2,    /* Bus Width : 0 - 8bit, 1 - 16bit, 2 - 32bit */
    0x02  /* SELect mask */
    },
  { 2, 0x04 },
  { 2, 0x06 },
  { 2, 0x08 },
  { 2, 0x0A },
  { 2, 0x0C },
  { 2, 0x0E },
  { 2, 0x10 } };

#define MC_ASYNC_TEST0	0x00000001LU /* no parity, bank after column, write enable */
#define MC_ASYNC_TEST1	0x00000002LU /* parity */
#define MC_ASYNC_TEST2	0x00000004LU /* bank after row */
#define MC_ASYNC_TEST3	0x00000008LU /* RO */
#define MC_ASYNC_TEST4	0x00000010LU /* - NOT DEFINED - */

#ifdef _MC_TEST_TEMPLATE1 
  #define MC_ASYNC_FLAGS	0x000002B4LU	/* MC_TEST_ flags... see mc_common.h */
  #define MC_ASYNC_TESTS	0x0000000FLU
#endif

#ifdef _MC_TEST_TEMPLATE2 
  #define MC_ASYNC_FLAGS	0x00000128LU	/* MC_TEST_ flags... see mc_common.h */
  #define MC_ASYNC_TESTS	0x00000008LU
#endif

#ifdef _MC_TEST_TEMPLATE3 
  #define MC_ASYNC_FLAGS	0x000007FFLU	/* MC_TEST_ flags... see mc_common.h */
  #define MC_ASYNC_TESTS	0x0000000FLU
#endif

#endif
