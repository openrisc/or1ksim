/* mc_dram.h - Memory Controller testbench SDRAM defines

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

#ifndef __MC_DRAM_H
#define __MC_DRAM_H

/* should configuration be read from MC? */
#define  MC_READ_CONF

/* TEMPLATE SELECTION       */
/* change #undef to #define */
#undef  _MC_TEST_TEMPLATE1
#define _MC_TEST_TEMPLATE2
#undef  _MC_TEST_TEMPLATE3
/* ------------------------ */


/* memory configuration that must reflect sim.cfg */
#define MC_SDRAM_CSMASK	0x02	/* 8 bit mask for 8 chip selects. 1 SDRAM at CS, 0 something else at CS */
                                /* bits: CS7|CS6|CS5|CS4|CS3|CS2|CS1|CS0 */	
typedef struct MC_SDRAM_CS
{
  unsigned long BW;
  unsigned long MS;
  unsigned long M;
} MC_SDRAM_CS;

MC_SDRAM_CS mc_sdram_cs[8] = {
  { 2,    /* Bus Width : 0 - 8bit, 1 - 16bit, 2 - 32bit */
    2,    /* Memory Size : 0 - 64Mbit, 1 - 128Mbit, 2 - 256Mbit */
    0x20  /* SELect mask */
    },
  { 1, 0, 0x20 },
  { 2, 0, 0x06 },
  { 2, 0, 0x08 },
  { 2, 0, 0x0A },
  { 2, 0, 0x0C },
  { 2, 0, 0x0E },
  { 2, 0, 0x10 } };

/* SDRAM configuration tests flag defines */
#define MC_SDRAM_TEST0	0x00000001LU /*no parity, close row, BankAfterCol, R/W, single loc, seq. burst 1 */
#define MC_SDRAM_TEST1	0x00000002LU /*parity enabled*/
#define MC_SDRAM_TEST2	0x00000004LU /*keep row*/
#define MC_SDRAM_TEST3	0x00000008LU /*BankAfterRow*/
#define MC_SDRAM_TEST4  0x00000010LU /*RO*/
#define MC_SDRAM_TEST5	0x00000020LU /*prog. burst 1*/
#define MC_SDRAM_TEST6	0x00000040LU /*  -||-  2*/
#define MC_SDRAM_TEST7	0x00000080LU /*  -||-  4*/
#define MC_SDRAM_TEST8	0x00000100LU /*  -||-  8*/
#define MC_SDRAM_TEST9	0x00000200LU /*  -||-  fullpage*/
#define MC_SDRAM_TEST10	0x00000400LU /*prog. burst int. 1*/
#define MC_SDRAM_TEST11	0x00000800LU /*  -||-  2*/
#define MC_SDRAM_TEST12	0x00001000LU /*  -||-  4*/
#define MC_SDRAM_TEST13	0x00002000LU /*  -||-  8*/
#define MC_SDRAM_TEST14	0x00004000LU /*  -||-  fullpage*/
#define MC_SDRAM_TEST15	0x00008000LU /*prog. burst int. fullpage, keep row*/
#define MC_SDRAM_TEST16	0x00010000LU /* NOT DEFINED */

/* test type flag defines */
#define MC_SDRAM_SROW	0x00000001LU /* perform sequential row access test */
#define MC_SDRAM_RROW	0x00000010LU /* perform random row access test     */
#define MC_SDRAM_SGRP	0x00000100LU /* perform sequential row-group access test */
#define MC_SDRAM_RGRP	0x00001000LU /* perform random row-group access test */

#define MC_SDRAM_ROWSH_0	16
#define MC_SDRAM_ROWSH_1	16
#define MC_SDRAM_ROWSH_2	15
#define MC_SDRAM_ROWSH_3	16
#define MC_SDRAM_ROWSH_4	16
#define MC_SDRAM_ROWSH_5	16
#define MC_SDRAM_ROWSH_6	17
#define MC_SDRAM_ROWSH_7	17
#define MC_SDRAM_ROWSH_8	17


/* TEMPLATE 1 */
#ifdef _MC_TEST_TEMPLATE1
 #define MC_SDRAM_FLAGS	0x000004A8LU 	/* MC_TEST_ flags... see mc_common.h */
 #define MC_SDRAM_TESTS	0x00000001LU 	/* mask for SDRAM configuration, see conf. test flag defines */
 #define MC_SDRAM_ACC	0x00001010LU 	/* mask for test types */

 /* memory sizes*/
 #define MC_SDRAM_GROUPSIZE	5
                                   /* MAX */
 #define MC_SDRAM_ROWSIZE_0	8/*  16 * 1024 * 4 */
 #define MC_SDRAM_ROWSIZE_1	8/*  16 * 1024 * 4 */
 #define MC_SDRAM_ROWSIZE_2	8/*   8 * 1024 * 4 */
 #define MC_SDRAM_ROWSIZE_3	8/*  16 * 1024 * 4 */
 #define MC_SDRAM_ROWSIZE_4	8/*  16 * 1024 * 4 */
 #define MC_SDRAM_ROWSIZE_5	8/*  16 * 1024 * 4 */
 #define MC_SDRAM_ROWSIZE_6	8/*  32 * 1024 * 4 */
 #define MC_SDRAM_ROWSIZE_7	8/*  32 * 1024 * 4 */
 #define MC_SDRAM_ROWSIZE_8	8/*  32 * 1024 * 4 */
                                  /*  MAX */
 #define MC_SDRAM_ROWS_0	25/*  512 */
 #define MC_SDRAM_ROWS_1	25/*  256 */
 #define MC_SDRAM_ROWS_2	25/*  256 */
 #define MC_SDRAM_ROWS_3	25/*  1024 */
 #define MC_SDRAM_ROWS_4	25/*  512 */
 #define MC_SDRAM_ROWS_5	25/*  256 */
 #define MC_SDRAM_ROWS_6	25/*  1024 */
 #define MC_SDRAM_ROWS_7	25/*  512 */
 #define MC_SDRAM_ROWS_8	25/*  256 */   

 #define MC_SDRAM_ROW_OFF	5
#endif /*_MC_TEST_TEMPLATE1*/

/* TEMPLATE 2 */
#ifdef _MC_TEST_TEMPLATE2
 #define MC_SDRAM_FLAGS	0x000004A8LU 	/* MC_TEST_ flags... see mc_common.h */
 #define MC_SDRAM_TESTS	0x00000001LU 	/* mask for SDRAM configuration, see conf. test flag defines */
 #define MC_SDRAM_ACC	0x00001010LU 	/* mask for test types */

 /* memory sizes*/
 #define MC_SDRAM_GROUPSIZE	5
                                 
 #define MC_SDRAM_ROWSIZE_0	16 * 1024 * 4
 #define MC_SDRAM_ROWSIZE_1	16 * 1024 * 4
 #define MC_SDRAM_ROWSIZE_2	 8 * 1024 * 4
 #define MC_SDRAM_ROWSIZE_3	16 * 1024 * 4
 #define MC_SDRAM_ROWSIZE_4	16 * 1024 * 4
 #define MC_SDRAM_ROWSIZE_5	16 * 1024 * 4
 #define MC_SDRAM_ROWSIZE_6	32 * 1024 * 4
 #define MC_SDRAM_ROWSIZE_7	32 * 1024 * 4
 #define MC_SDRAM_ROWSIZE_8	32 * 1024 * 4
                                  
 #define MC_SDRAM_ROWS_0	 512
 #define MC_SDRAM_ROWS_1	 256
 #define MC_SDRAM_ROWS_2	 256
 #define MC_SDRAM_ROWS_3	1024
 #define MC_SDRAM_ROWS_4	 512
 #define MC_SDRAM_ROWS_5	 256
 #define MC_SDRAM_ROWS_6	1024
 #define MC_SDRAM_ROWS_7	 512
 #define MC_SDRAM_ROWS_8	 256

#endif /*_MC_TEST_TEMPLATE2*/

/* TEMPLATE 3 */
#ifdef _MC_TEST_TEMPLATE3
 #define MC_SDRAM_FLAGS	0x000004A8LU 	/* MC_TEST_ flags... see mc_common.h */
 #define MC_SDRAM_TESTS	0x00000001LU 	/* mask for SDRAM configuration, see conf. test flag defines */
 #define MC_SDRAM_ACC	0x00001010LU 	/* mask for test types */

 /* memory sizes*/
 #define MC_SDRAM_GROUPSIZE	5
                                 /* MAX */
 #define MC_SDRAM_ROWSIZE_0	16 * 1024 * 4
 #define MC_SDRAM_ROWSIZE_1	16 * 1024 * 4
 #define MC_SDRAM_ROWSIZE_2	 8 * 1024 * 4
 #define MC_SDRAM_ROWSIZE_3	16 * 1024 * 4
 #define MC_SDRAM_ROWSIZE_4	16 * 1024 * 4
 #define MC_SDRAM_ROWSIZE_5	16 * 1024 * 4
 #define MC_SDRAM_ROWSIZE_6	32 * 1024 * 4
 #define MC_SDRAM_ROWSIZE_7	32 * 1024 * 4
 #define MC_SDRAM_ROWSIZE_8	32 * 1024 * 4
                                  /*  MAX */
 #define MC_SDRAM_ROWS_0	10/*  512 */
 #define MC_SDRAM_ROWS_1	10/*  256 */
 #define MC_SDRAM_ROWS_2	10/*  256 */
 #define MC_SDRAM_ROWS_3	10/*  1024 */
 #define MC_SDRAM_ROWS_4	10/*  512 */
 #define MC_SDRAM_ROWS_5	10/*  256 */
 #define MC_SDRAM_ROWS_6	10/*  1024 */
 #define MC_SDRAM_ROWS_7	10/*  512 */
 #define MC_SDRAM_ROWS_8	10/*  256 */

#endif /*_MC_TEST_TEMPLATE3*/

#endif
