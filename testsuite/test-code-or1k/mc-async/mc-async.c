/* mc_async.c - Memory Controller testbench ASYNCdevice test

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
#include "mc-async.h"

#include "config.h"
#include "mc-defines.h"
#include "gpio.h"
#include "fields.h"

typedef volatile unsigned long *REGISTER;

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

    lpoc = *mc_poc;

    for (ch=0; ch<8; ch++) {
        if (MC_ASYNC_CSMASK & (0x01 << ch) ) {
	    mc_csc = (unsigned long*)(MC_BASE + MC_CSC(ch));
	    SET_FIELD(*mc_csc, MC_CSC, SEL, mc_async_cs[ch].M);
	    SET_FIELD(*mc_csc, MC_CSC, BW,  mc_async_cs[ch].BW);
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
    if ( (GET_FIELD(*mc_csc, MC_CSC, MEMTYPE) == 2) && 
         (TEST_FLAG(*mc_csc, MC_CSC, EN) == 1     ) ) {
      mc_async_cs[ch].BW = GET_FIELD(*mc_csc, MC_CSC, BW);
      mc_async_cs[ch].M  = GET_FIELD(*mc_csc, MC_CSC, SEL);
      mc_cs |= (1 << ch);

      printf("get_config(%d) : BW=0x%0lx, M=0x%0lx\n", ch,
	     mc_async_cs[ch].BW, 
	     mc_async_cs[ch].M);
    }
  }
  printf("get_config() : cs=0x%0x\n", mc_cs);
  return 0;
}

int main()
{
    unsigned long ret;
    unsigned char ch;

    unsigned long test;
    unsigned long gpio_pat = 0;

    unsigned long nAddress;
    unsigned long nMemSize;
    unsigned long mc_sel;
    REGISTER mc_tms;
    REGISTER mc_csc;

    *rgpio_out = 0xFFFFFFFF;
#ifdef MC_READ_CONF
    if (get_config()) {
      printf("Error reading MC configuration.\n");
      report(1);
      return(1);
    }
#else
    mc_cs = MC_ASYNC_CSMASK;
#endif

    for (ch=0; ch<8; ch++) {
	if (mc_cs & (0x01 << ch) ) {
	    printf ("--- Begin Test on CS%d ---\n", ch);

	    mc_csc  = (unsigned long*)(MC_BASE + MC_CSC(ch));
	    mc_tms = (unsigned long*)(MC_BASE + MC_TMS(ch));
	    mc_sel = GET_FIELD(*mc_csc, MC_CSC, SEL);

	    printf ("CS configuration : CSC - 0x%08lX, TMS - 0x%08lXu\n",
		    *mc_csc, *mc_tms);

	    for (test=0; test<4; test++) {
		/* configure MC*/
		CLEAR_FLAG(*mc_csc, MC_CSC, PEN); /* no parity */
		CLEAR_FLAG(*mc_csc, MC_CSC, BAS); /* bank after column */
		CLEAR_FLAG(*mc_csc, MC_CSC, WP);  /* write enable */
		
		switch (test) {
		case 0:
		    if ((MC_ASYNC_TESTS & MC_ASYNC_TEST0) != MC_ASYNC_TEST0)
			continue;
		    break;
		case 1:
		    if ((MC_ASYNC_TESTS & MC_ASYNC_TEST1) != MC_ASYNC_TEST1)
			continue;
		    SET_FLAG(*mc_csc, MC_CSC, PEN); /* parity */
		    break;
		case 2:
		    if ((MC_ASYNC_TESTS & MC_ASYNC_TEST2) != MC_ASYNC_TEST2)
			continue;
		    SET_FLAG(*mc_csc, MC_CSC, BAS); /* bank after row */
		    break;
		case 3:
		    if ((MC_ASYNC_TESTS & MC_ASYNC_TEST3) != MC_ASYNC_TEST3)
			continue;
		    SET_FLAG(*mc_csc, MC_CSC, WP);  /* RO */
		    break;
		} /*switch test*/

		printf ("Begin TEST %lu : CSC - 0x%08lX, TMS - 0x%08lX\n", test, *mc_csc, *mc_tms);

		nAddress = mc_sel << 21;
		nAddress |= MC_MEM_BASE;
		nMemSize = ( ((*mc_ba_mask & 0x000000FF) + 1) << 21);

		gpio_pat ^= 0x00000008;
		*rgpio_out = gpio_pat;
		ret = mc_test_row(nAddress, nAddress + nMemSize, MC_ASYNC_FLAGS);

		printf("\trow tested: nAddress = 0x%08lX, ret = 0x%08lX\n", nAddress, ret);

		if (ret) {
		    gpio_pat ^= 0x00000080;
		    *rgpio_out = gpio_pat;
		    report(ret);
		    return ret;
		}
		
	    } /*for test*/
	} /*if*/
    } /*for CS*/
    printf("--- End ASYNC tests ---\n");
    report(0xDEADDEAD);

    gpio_pat ^= 0x00000020;
    *rgpio_out = gpio_pat;

    return 0;
} /* main */
