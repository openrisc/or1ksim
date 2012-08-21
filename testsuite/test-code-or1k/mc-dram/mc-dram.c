/* mc_dram.c - Memory Controller testbench dram test

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

#include "support.h"

#include "mc-common.h"
#include "mc-dram.h"

#include "config.h"
#include "mc-defines.h"
#include "gpio.h"
#include "fields.h"

#define T_ROW_SIZE	8
#define T_ROW_OFF	5
#define T_ROWS		25
#define T_GROUPS	3

typedef volatile unsigned long *REGISTER;

/*
unsigned long nRowSize = 0;
unsigned long nColumns = 0;
*/
REGISTER mc_poc        = (unsigned long*)(MC_BASE + MC_POC);
REGISTER mc_csr        = (unsigned long*)(MC_BASE + MC_CSR);
REGISTER mc_ba_mask    = (unsigned long*)(MC_BASE + MC_BA_MASK);

REGISTER rgpio_out     = (unsigned long*)(GPIO_BASE + RGPIO_OUT);
REGISTER rgpio_in      = (unsigned long*)(GPIO_BASE + RGPIO_IN);

unsigned long lpoc;
unsigned char mc_cs;

unsigned long set_config()
{
    REGISTER mc_csc;
    unsigned char ch;

    for (ch=0; ch<8; ch++) {
        if (MC_SDRAM_CSMASK & (0x01 << ch) ) {
	    mc_csc = (unsigned long*)(MC_BASE + MC_CSC(ch));
	    SET_FIELD(*mc_csc, MC_CSC, MS,  mc_sdram_cs[ch].MS);
	    SET_FIELD(*mc_csc, MC_CSC, BW,  mc_sdram_cs[ch].BW);
	    SET_FIELD(*mc_csc, MC_CSC, SEL, mc_sdram_cs[ch].M);
	    SET_FLAG(*mc_csc, MC_CSC, EN);
	    printf ("Channel Config %d - CSC = 0x%08lX\n", ch, *mc_csc);
	}
    }

    return 0;
}

unsigned long get_config()
{
  REGISTER mc_csc;
  REGISTER mc_tms;
  unsigned char ch;

  mc_cs = 0;
  for (ch=0; ch<8; ch++) {
    mc_csc = (unsigned long*)(MC_BASE + MC_CSC(ch));
    mc_tms = (unsigned long*)(MC_BASE + MC_TMS(ch));
    if ( (GET_FIELD(*mc_csc, MC_CSC, MEMTYPE) == 0) && 
         (TEST_FLAG(*mc_csc, MC_CSC, EN) == 1     ) ) {
      mc_sdram_cs[ch].MS = GET_FIELD(*mc_csc, MC_CSC, MS);
      mc_sdram_cs[ch].BW = GET_FIELD(*mc_csc, MC_CSC, BW);
      mc_sdram_cs[ch].M  = GET_FIELD(*mc_csc, MC_CSC, SEL);
      mc_cs |= (1 << ch);

      printf("get_config(%d) : MS=0x%0lx, BW=0x%0lx, M=0x%0lx\n", ch,
	     mc_sdram_cs[ch].MS, 
	     mc_sdram_cs[ch].BW, 
	     mc_sdram_cs[ch].M);
    }
  }
  printf("get_config() : cs=0x%0x\n", mc_cs);
  return 0;
}

int main()
{
    unsigned long ret;
    unsigned char ch;

    unsigned long j, i;
    unsigned long test;
    unsigned long gpio_pat = 0;

    unsigned long nRowSize = 0;
    unsigned long nRows    = 0;
    unsigned long nRowSh   = 0;
    unsigned long nRowGrp  = 0;

    unsigned long nAddress;
    unsigned long mc_sel;
    REGISTER mc_tms;
    REGISTER mc_csc;
    
    printf ("Memory controller test with DRAM\n");

    *rgpio_out = 0xFFFFFFFF;

    /* set configuration */
    randomin(7435);
    lpoc = *mc_poc;

#ifdef MC_READ_CONF
    if (get_config()) {
      printf("Error reading MC configuration\n");
      report(0x00000001);
      return(1);
    }
#else
    mc_cs = MC_SDRAM_CSMASK;
#endif

    for (ch=0; ch<8; ch++) {
	if (mc_cs & (0x01 << ch) ) {
	    printf ("--- Begin Test on CS%d ---\n", ch);

	    mc_csc = (unsigned long*)(MC_BASE + MC_CSC(ch));
	    mc_tms = (unsigned long*)(MC_BASE + MC_TMS(ch));
	    mc_sel = GET_FIELD(*mc_csc, MC_CSC, SEL);
	    
	    SET_FIELD(*mc_tms, MC_TMS_SDRAM, OM, 0); /*normal op*/
	    SET_FIELD(*mc_tms, MC_TMS_SDRAM, CL, 3); /*CAS*/
	    
	    switch ( mc_sdram_cs[ch].BW + (3 * mc_sdram_cs[ch].MS) ) {
	    case 0:
	    case 4:
		nRowSize = MC_SDRAM_ROWSIZE_0;
		nRows    = MC_SDRAM_ROWS_0; 
		nRowSh   = MC_SDRAM_ROWSH_0; break;
	    case 1:
	    case 5:
		nRowSize = MC_SDRAM_ROWSIZE_1;
		nRows    = MC_SDRAM_ROWS_1;
		nRowSh   = MC_SDRAM_ROWSH_1; break;
	    case 2:
		nRowSize = MC_SDRAM_ROWSIZE_2;
		nRows    = MC_SDRAM_ROWS_2;
		nRowSh   = MC_SDRAM_ROWSH_2;  break;
	    case 3:
		nRowSize = MC_SDRAM_ROWSIZE_3;
		nRows    = MC_SDRAM_ROWS_3; 
		nRowSh   = MC_SDRAM_ROWSH_3; break;
	    case 6:
		nRowSize = MC_SDRAM_ROWSIZE_6;
		nRows    = MC_SDRAM_ROWS_6; 
		nRowSh   = MC_SDRAM_ROWSH_6; break;
	    case 7:
		nRowSize = MC_SDRAM_ROWSIZE_7;
		nRows    = MC_SDRAM_ROWS_7; 
		nRowSh   = MC_SDRAM_ROWSH_7; break;
	    case 8:
		nRowSize = MC_SDRAM_ROWSIZE_8;
		nRows    = MC_SDRAM_ROWS_8; 
		nRowSh   = MC_SDRAM_ROWSH_8; break;
	    }

	    printf ("CS configuration : CSC - 0x%08lX, TMS - 0x%08lX, rs = %lu, nr = %lu, sh = %lu, sel = %lu\n",
		    *mc_csc, *mc_tms, nRowSize, nRows, nRowSh, mc_sel);

	    /*nRows -= MC_SDRAM_ROW_OFF;*/
	    for (test=0; test<16; test++) {
		/* configure MC*/
		CLEAR_FLAG(*mc_csc, MC_CSC, PEN); /* no parity */
		CLEAR_FLAG(*mc_csc, MC_CSC, KRO); /* close row */
		CLEAR_FLAG(*mc_csc, MC_CSC, BAS); /* bank after column */
		CLEAR_FLAG(*mc_csc, MC_CSC, WP);  /* write enable */
		SET_FLAG(*mc_tms, MC_TMS_SDRAM, WBL); /* single loc access */
		CLEAR_FLAG(*mc_tms, MC_TMS_SDRAM, BT); /* sequential burst */
		SET_FIELD(*mc_tms, MC_TMS_SDRAM, BL, 0); /* 1 */
		switch (test) {
		case 0:
		    if ((MC_SDRAM_TESTS & MC_SDRAM_TEST0) != MC_SDRAM_TEST0)
			continue;
		    break;
		case 1:
		    if ((MC_SDRAM_TESTS & MC_SDRAM_TEST1) != MC_SDRAM_TEST1)
			continue;
		    SET_FLAG(*mc_csc, MC_CSC, PEN); /* parity */
		    break;
		case 2:
		    if ((MC_SDRAM_TESTS & MC_SDRAM_TEST2) != MC_SDRAM_TEST2)
			continue;
		    SET_FLAG(*mc_csc, MC_CSC, KRO); /* keep row */
		    break;
		case 3:
		    if ((MC_SDRAM_TESTS & MC_SDRAM_TEST3) != MC_SDRAM_TEST3)
			continue;
		    SET_FLAG(*mc_csc, MC_CSC, BAS); /* bank after row*/
		    break;
		case 4:
		    if ((MC_SDRAM_TESTS & MC_SDRAM_TEST4) != MC_SDRAM_TEST4)
			continue;
		    SET_FLAG(*mc_csc, MC_CSC, WP);  /* RO */
		    break;
		case 5:
		    if ((MC_SDRAM_TESTS & MC_SDRAM_TEST5) != MC_SDRAM_TEST5)
			continue;
		    CLEAR_FLAG(*mc_tms, MC_TMS_SDRAM, WBL); /* burst */
		    break;
		case 6:
		    if ((MC_SDRAM_TESTS & MC_SDRAM_TEST6) != MC_SDRAM_TEST6)
			continue;
		    CLEAR_FLAG(*mc_tms, MC_TMS_SDRAM, WBL); /* burst */
		    SET_FIELD(*mc_tms, MC_TMS_SDRAM, BL, 1); /* 2 */
		    break;
		case 7:
		    if ((MC_SDRAM_TESTS & MC_SDRAM_TEST7) != MC_SDRAM_TEST7)
			continue;
		    CLEAR_FLAG(*mc_tms, MC_TMS_SDRAM, WBL); /* burst */
		    SET_FIELD(*mc_tms, MC_TMS_SDRAM, BL, 2); /* 4 */
		    break;
		case 8:
		    if ((MC_SDRAM_TESTS & MC_SDRAM_TEST8) != MC_SDRAM_TEST8)
			continue;
		    CLEAR_FLAG(*mc_tms, MC_TMS_SDRAM, WBL); /* burst */
		    SET_FIELD(*mc_tms, MC_TMS_SDRAM, BL, 3); /* 8 */
		    break;
		case 9:
		    if ((MC_SDRAM_TESTS & MC_SDRAM_TEST9) != MC_SDRAM_TEST9)
			continue;
		    CLEAR_FLAG(*mc_tms, MC_TMS_SDRAM, WBL); /* burst */
		    SET_FIELD(*mc_tms, MC_TMS_SDRAM, BL, 7); /* full page */
		    break;
		case 10:
		    if ((MC_SDRAM_TESTS & MC_SDRAM_TEST10) != MC_SDRAM_TEST10)
			continue;
		    CLEAR_FLAG(*mc_tms, MC_TMS_SDRAM, WBL); /* burst */
		    SET_FLAG(*mc_tms, MC_TMS_SDRAM, BT); /* interleaved burst */
		    break;
		case 11:
		    if ((MC_SDRAM_TESTS & MC_SDRAM_TEST11) != MC_SDRAM_TEST11)
			continue;
		    CLEAR_FLAG(*mc_tms, MC_TMS_SDRAM, WBL); /* burst */
		    SET_FLAG(*mc_tms, MC_TMS_SDRAM, BT); /* interleaved burst */
		    SET_FIELD(*mc_tms, MC_TMS_SDRAM, BL, 1); /* 2 */
		    break;
		case 12:
		    if ((MC_SDRAM_TESTS & MC_SDRAM_TEST12) != MC_SDRAM_TEST12)
			continue;
		    CLEAR_FLAG(*mc_tms, MC_TMS_SDRAM, WBL); /* burst */
		    SET_FLAG(*mc_tms, MC_TMS_SDRAM, BT); /* interleaved burst */
		    SET_FIELD(*mc_tms, MC_TMS_SDRAM, BL, 2); /* 4 */
		    break;
		case 13:
		    if ((MC_SDRAM_TESTS & MC_SDRAM_TEST13) != MC_SDRAM_TEST13)
			continue;
		    CLEAR_FLAG(*mc_tms, MC_TMS_SDRAM, WBL); /* burst */
		    SET_FLAG(*mc_tms, MC_TMS_SDRAM, BT); /* interleaved burst */
		    SET_FIELD(*mc_tms, MC_TMS_SDRAM, BL, 3); /* 8 */
		    break;
		case 14:
		    if ((MC_SDRAM_TESTS & MC_SDRAM_TEST14) != MC_SDRAM_TEST14)
			continue;
		    CLEAR_FLAG(*mc_tms, MC_TMS_SDRAM, WBL); /* burst */
		    SET_FLAG(*mc_tms, MC_TMS_SDRAM, BT); /* interleaved burst */
		    SET_FIELD(*mc_tms, MC_TMS_SDRAM, BL, 7); /* fullrow */
		    break;
		case 15:
		    if ((MC_SDRAM_TESTS & MC_SDRAM_TEST15) != MC_SDRAM_TEST15)
			continue;
		    SET_FLAG(*mc_csc, MC_CSC, KRO);  /* keep row */
		    CLEAR_FLAG(*mc_tms, MC_TMS_SDRAM, WBL); /* burst */
		    SET_FLAG(*mc_tms, MC_TMS_SDRAM, BT); /* interleaved burst */
		    SET_FIELD(*mc_tms, MC_TMS_SDRAM, BL, 1); /* 2 */
		    break;
		} /*switch test*/

		printf ("Begin TEST %lu : CSC - 0x%08lX, TMS - 0x%08lX\n", test, *mc_csc, *mc_tms);

		if (MC_SDRAM_ACC & MC_SDRAM_SROW) { 
		    /* perform sequential row access */
		    printf("Seuential Row\n");
		    for (j=0; j<T_ROWS; j++) {
			nAddress  = mc_sel << 21;
			nAddress |= MC_MEM_BASE;
			nAddress += ( (j + T_ROW_OFF) << nRowSh);

			gpio_pat ^= 0x00000008;
			*rgpio_out = gpio_pat;
			ret = mc_test_row(nAddress, nAddress + T_ROW_SIZE, MC_SDRAM_FLAGS);

			printf("\trow - %lu: nAddress = 0x%08lX, ret = 0x%08lX\n", j, nAddress, ret);

			if (ret) {
			    gpio_pat ^= 0x00000080;
			    *rgpio_out = gpio_pat;
			    report(ret);
			    return ret;
			}
		    }
		}

		if (MC_SDRAM_ACC & MC_SDRAM_RROW) {
		    /* perform random row access */
		    printf("Random Row\n");
		    for (j=0; j<T_ROWS; j++) {
		        nAddress = mc_sel << 21;
			nAddress |= MC_MEM_BASE;
			nAddress += ( (T_ROW_OFF + random(nRows)) << nRowSh);
			  
			gpio_pat ^= 0x00000008;
			*rgpio_out = gpio_pat;
			ret = mc_test_row(nAddress, nAddress + T_ROW_SIZE, MC_SDRAM_FLAGS);
			
			printf("\trow - %lu: nAddress = 0x%08lX, ret = 0x%08lX\n", j, nAddress, ret);
		
			if (ret) {
			    gpio_pat ^= 0x00000080;
			    *rgpio_out = gpio_pat;
			    report(ret);
			    return ret;
			}
		    }
		}

		if (MC_SDRAM_ACC & MC_SDRAM_SGRP) {
		    /* perform sequential row in group access */
		    printf("Sequential Group ");
		   
		    printf("Group Size = %d\n", MC_SDRAM_GROUPSIZE);
		    for (i=0; i<T_GROUPS; i++) {
			nRowGrp = random(nRows - MC_SDRAM_GROUPSIZE) + T_ROW_OFF;
			for (j=0; j<MC_SDRAM_GROUPSIZE; j++) {
			    nAddress = mc_sel << 21;
			    nAddress |= MC_MEM_BASE;
			    nAddress += ((nRowGrp+j) << nRowSh);

			    gpio_pat ^= 0x00000008;
			    *rgpio_out = gpio_pat;
			    ret = mc_test_row(nAddress, nAddress + T_ROW_SIZE, MC_SDRAM_FLAGS);
			    
			    printf("\trow - %lu: nAddress = 0x%08lX, ret = 0x%08lX\n", j, nAddress, ret);

			    if (ret) {
			        gpio_pat ^= 0x00000080;
			        *rgpio_out = gpio_pat;
				report(ret);
				return ret;
			    }
			}
		    }
		}

		if (MC_SDRAM_ACC & MC_SDRAM_RGRP) {
		    /* perform random row in group access */
		    printf("Random Group ");

		    printf("Group Size = %d\n", MC_SDRAM_GROUPSIZE);
		    for (i=0; i<T_GROUPS; i++) {
			nRowGrp = random(nRows - T_GROUPS) + T_ROW_OFF;
			for (j=0; j<MC_SDRAM_GROUPSIZE; j++) {
			    nAddress = mc_sel << 21;
			    nAddress |= MC_MEM_BASE;
			    nAddress += ((nRowGrp + random(MC_SDRAM_GROUPSIZE)) << nRowSh);
			    
			    gpio_pat ^= 0x00000008;
			    *rgpio_out = gpio_pat;
			    ret = mc_test_row(nAddress, nAddress + T_ROW_SIZE, MC_SDRAM_FLAGS);
			    
			    printf("\trow - %lu: nAddress = 0x%08lX, ret = 0x%08lX\n", j, nAddress, ret);

			    if (ret) {
			        gpio_pat ^= 0x00000080;
			        *rgpio_out = gpio_pat;
				report(ret);
				return ret;
			    }
			}
		    }
		} /*for groups*/

	    } /*for test*/
	} /*if*/
    } /*for CS*/
    printf("--- End SDRAM tests ---\n");
    report(0xDEADDEAD);

    gpio_pat ^= 0x00000020;
    *rgpio_out = gpio_pat;

    return 0;
} /* main */
