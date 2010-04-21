/* mc_common.c -- Memory Controller testbenck common routines

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

static int r_Xn;
static int r_mod;

void randomin(unsigned long seed)
{
  r_Xn = seed;
  r_mod = 33554431; /* 2^25 - 1*/
}

unsigned long random(unsigned long max)
{
  r_Xn = ((r_Xn * (r_Xn + 1))%r_mod);
  if (r_Xn == 0) r_Xn = 42+1;
  return r_Xn%max;
}

void mc_clear_row(unsigned long nFrom, unsigned long nTo)
{
  MEMLOC32 mem32 = 0;
  unsigned long i;

  for(i=nFrom; i<nTo; i+=4) {
    mem32  = (unsigned long*)i;
    *mem32 = 0UL;
  }
}

unsigned long mc_test_row_8(int run, int seq, unsigned long nFrom,
			    unsigned long nTo)
{
    MEMLOC8  mem8  = 0;
    MEMLOC16 mem16 = 0;
    MEMLOC32 mem32 = 0;

    unsigned char pattern    = 0;
    unsigned short pattern16 = 0;
    unsigned long pattern32  = 0;
    unsigned char read;
    unsigned short read16;
    unsigned long read32;
    int inc = 0;

    unsigned long i;

    /* number of locations to test */
    unsigned long nWords = nTo - nFrom;

    /* JPB. Switch split due to bug in GCC with PIC and larger switches */
    switch (run) {
    case 0:
	pattern = MC_TEST_PAT1_8;
	pattern16 = (pattern << 8); 
	pattern32 = (pattern16 << 16); break;
    case 1:
	pattern = MC_TEST_PAT2_8; 
	pattern16 = (pattern << 8);
	pattern32 = (pattern16 << 16); break;
    case 2:
	pattern = MC_TEST_PAT3_8; 
	pattern16 = (pattern << 8);
	pattern32 = (pattern16 << 16); break;
    } /*switch*/

    switch (run) {
    case 3:
	pattern = MC_TEST_PAT4_8; 
	pattern16 = (pattern << 8);
	pattern32 = (pattern16 << 16); break;
    case 4:
	pattern32 = pattern16 = pattern = 0; inc = 1; break;
    } /*switch*/

    printf ("\tmc_test_row_8(0x%02dX, %d, 0x%08lX, 0x%08lX);\n", pattern, seq,
	    nFrom, nTo);

    switch (seq) {
    case 0: /*seq. row write, row read*/
	for (i=nFrom; i<nTo; i++) {
	    mem8  = (unsigned char*)i;
	    *mem8 = pattern;
	    if (inc) pattern++;
	} /*for*/

	if (inc) pattern = 0;
	for (i=nFrom; i<nTo; i++) {
	    mem8 = (unsigned char*)i;
	    read = *mem8;

	    if ( read != pattern ) {
	        printf("\ti=%lX, read=0x%02dX, pattern=0x%02dX\n", i, read,
		       pattern);
		return (unsigned long)mem8;
	    }

	    if (inc) pattern++;
	} /*for*/
	break;
    case 1: /*seq. word write, word read*/
	for (i=nFrom; i<nTo; i++) {
	    mem8  = (unsigned char*)i;
	    *mem8 = pattern;
	    read = *mem8;

	    if (read != pattern ) {
	        printf("\ti=%lX, read=0x%02dX, pattern=0x%02dX\n", i, read,
		       pattern);
		return (unsigned long)mem8;
	    }

	    mem16  = (unsigned short*)(i & 0xFFFFFFFELU);
	    read16 = *mem16;
	    if (read16 != pattern16) {
	        printf("\ti=%lX, read16=0x%04dX, pattern16=0x%04dX\n", i,
		       read16, pattern16);
		return (unsigned long)mem16;
	    }
	    mem32  = (unsigned long*)(i & 0xFFFFFFFCLU);
	    read32 = *mem32;
	    if (read32 != pattern32) {
	        printf("\ti=%lX, read32=0x%08lX, pattern32=0x%08lX\n", i,
		       read32, pattern32);
		return (unsigned long)mem32;
	    }

	    if (inc) pattern++;
	    switch ((i+1)%4) {
	    case 0:
		pattern16 = (pattern << 8);
		pattern32 = (pattern << 24); break;
	    case 1:
		pattern16 += pattern;
		pattern32 = pattern16 << 16; break;
	    case 2:
		pattern16 = (pattern << 8);
		pattern32 += pattern16;break;
	    case 3:
		pattern16 += pattern;
		pattern32 += pattern; break;
	    } /*switch i%4*/
	} /*for*/
	break;
    case 2: /*rand. word write, word read*/
	for (i=0; i<nWords; i++) {
	    mem8  = (unsigned char*)(nFrom + random(nWords));
	    *mem8 = pattern;
	    read  = *mem8;
	    
	    if (read != pattern ) {
		printf("\ti=%lX, read=0x%02dX, pattern=0x%02dX\n", i, read,
		       pattern);
		return (unsigned long)mem8;
	    }

	    if (inc) pattern++;
	}
	break;
    } /*switch seq*/

    return 0;
} /* mc_test_row_8 */

unsigned long mc_test_row_16(int run, int seq, unsigned long nFrom,
			     unsigned long nTo)
{
    MEMLOC16 mem16 = 0;
    MEMLOC32 mem32 = 0;

    unsigned short pattern16 = 0;
    unsigned long pattern32  = 0;
    unsigned short read16;
    unsigned long read32;
    int inc = 0;

    unsigned long i;

    unsigned long nWords = (nTo - nFrom)/2;

    /* JPB. Switch split due to bug in GCC with PIC and larger switches */
    switch (run) {
    case 0:
	pattern16 = MC_TEST_PAT1_16;
	pattern32 = (pattern16 << 16); break;
    case 1:
	pattern16 = MC_TEST_PAT2_16;
	pattern32 = (pattern16 << 16); break;
    case 2:
	pattern16 = MC_TEST_PAT3_16;
	pattern32 = (pattern16 << 16);break;
    } /*switch*/

    switch (run) {
    case 3:
	pattern16 = MC_TEST_PAT4_16;
	pattern32 = (pattern16 << 16); break;
    case 4:
	pattern16 = pattern32 = 0; inc = 1; break;
    } /*switch*/

    printf ("\tmc_test_row_16(0x%04dX, %d, 0x%08lX, 0x%08lX);\n", pattern16,
	    seq, nFrom, nTo);

    switch (seq) {
    case 0: /*seq. row write, row read*/
	for (i=nFrom; i<nTo; i+=2) {
	    mem16  = (unsigned short*)i;
	    *mem16 = pattern16;
	    if (inc) pattern16++;
	} /*for*/

	if (inc) pattern16 = 0;
	for (i=nFrom; i<nTo; i+=2) {
	    mem16  = (unsigned short*)i;
	    read16 = *mem16;

	    if ( read16 != pattern16 ) {
		printf("\ti=%lX, read16=0x%04dX, pattern16=0x%04dX\n", i,
		       read16, pattern16);
		return (unsigned long)mem16;
	    }

	    if (inc) pattern16++;
	} /*for*/
	break;
    case 1: /*seq. word write, word read*/
	for (i=nFrom; i<nTo; i+=2) {
	    mem16  = (unsigned short*)i;
	    *mem16 = pattern16;
	    read16 = *mem16;

	    if (read16 != pattern16 ) {
	        printf("\ti=%lX, read16=0x%04dX, pattern16=0x%04dX\n", i,
		       read16, pattern16);
		return (unsigned long)mem16;
	    }

	    mem32  = (unsigned long*)(i & 0xFFFFFFFCLU);
	    read32 = *mem32;
	    if (read32 != pattern32) {
	        printf("\ti=%lX, read32=0x%08lX, pattern32=0x%08lX\n", i,
		       read32, pattern32);
		return (unsigned long)mem32;
	    }

	    if (inc) pattern16++;
	    switch ((i+2)%4) {
	    case 0:
	        pattern32 = (pattern16 << 16); break;
	    case 2:
		pattern32 += pattern16;
	    } /*switch i%4*/
	} /*for*/
	break;
    case 2: /*rand. word write, word read*/
	for (i=0; i<nWords; i++) {
	    mem16  = (unsigned short*)(nFrom + random(nWords)*2);
	    *mem16 = pattern16;
	    read16 = *mem16;

	    if (read16 != pattern16 ) {
		printf("\ti=%lX, read16=0x%04dX, pattern16=0x%04dX\n", i,
		       read16, pattern16);
		return (unsigned long)mem16;
	    }

	    if (inc) pattern16++;
	}
	break;
    } /*switch seq*/
    return 0;
} /* mc_test_row_16 */

unsigned long mc_test_row_32(int run, int seq, unsigned long nFrom,
			     unsigned long nTo)
{
    MEMLOC32 mem32 = 0;

    unsigned long pattern32 = 0;
    unsigned long read32;
    int inc = 0;

    unsigned long i;

    unsigned long nWords = (nTo - nFrom)/4;

    /* JPB. Switch split due to bug in GCC with PIC and larger switches */
    switch (run) {
    case 0:
	pattern32 = MC_TEST_PAT1_32; break;
    case 1:
	pattern32 = MC_TEST_PAT2_32; break;
    case 2:
	pattern32 = MC_TEST_PAT3_32; break;
    } /*switch*/

    switch (run) {
    case 3:
	pattern32 = MC_TEST_PAT4_32; break;
    case 4:
	pattern32 = nFrom; inc = 1; break;
    } /*switch*/

    printf ("\tmc_test_row_32(0x%08lX, %d, 0x%08lX, 0x%08lX);\n", pattern32,
	    seq, nFrom, nTo);

    switch (seq) {
    case 0: /*seq. row write, row read*/
	for (i=nFrom; i<nTo; i+=4) {
	    mem32  = (unsigned long*)i;
	    *mem32 = pattern32;
	    if (inc) pattern32 += 4;
	} /*for*/

	if (inc) pattern32 = nFrom;
	for (i=nFrom; i<nTo; i+=4) {
	    mem32 = (unsigned long *)i;
	    read32 = *mem32;

	    if ( read32 != pattern32 ) {
		printf("\ti=%lX, read32=0x%08lX, pattern32=0x%08lX\n", i,
		       read32, pattern32);
		return (unsigned long)mem32;
	    }

	    if (inc) pattern32 += 4;
	} /*for*/
	break;
    case 1: /*seq. word write, word read*/
	for (i=nFrom; i<nTo; i+=4) {
	    mem32  = (unsigned long*)i;
	    *mem32 = pattern32;
	    read32 = *mem32;

	    if (read32 != pattern32 ) {
	        printf("\ti=%lX, read32=0x%08lX, pattern32=0x%08lX\n", i,
		       read32, pattern32);
		return (unsigned long)mem32;
	    }

	    if (inc) pattern32 += 4;

	}
	break;
    case 2: /*rand. word write, word read*/
	for (i=0; i<nWords; i++) {
	    mem32 = (unsigned long*)(nFrom + random(nWords)*4);
	    if (inc) 
	        pattern32 = (unsigned long)mem32;
	    *mem32 = pattern32;
	    read32  = *mem32;

	    if (read32 != pattern32 ) {
		printf("\ti=%lX, read32=0x%08lX, pattern32=0x%08lX\n", i,
		       read32, pattern32);
		return (unsigned long)mem32;
	    }
	} /*for*/
	break;
    } /*switch seq*/
    return 0;
} /* mc_test_row_32 */


/*---------------------------------------------------------------------------*/


/*-----[ Common Functions ]--------------------------------------------------*/
unsigned long mc_test_row(unsigned long nFrom, unsigned long nTo,
			  unsigned long flags)
{
    int nRun, nSize, nSeq;
    int ret = 0;

    printf("\nmc_test_row(0x%08lX, 0x%08lX)\n", nFrom, nTo);

    for (nRun = 0; nRun < 5; nRun++) {
      /* JPB. Switch split due to bug in GCC with PIC and larger switches */
	switch (nRun) {
	case 0:
	    if ( (flags & MC_TEST_RUN0) != MC_TEST_RUN0 ) 
		continue;
	    break;
	case 1:
	    if ( (flags & MC_TEST_RUN1) != MC_TEST_RUN1 )
		continue;
	    break;
	case 2:
	    if ( (flags & MC_TEST_RUN01) != MC_TEST_RUN01 )
		continue;
	    break;
	} /*switch*/

	switch (nRun) {
	case 3:
	    if ( (flags & MC_TEST_RUN10) != MC_TEST_RUN10 )
		continue;
	    break;
	case 4:
	    if ( (flags & MC_TEST_RUNINC) != MC_TEST_RUNINC )
		continue;
	} /*switch*/

	for (nSeq = 0; nSeq < 3; nSeq++) {
	    switch (nSeq) {
	    case 0:
		if ( (flags & MC_TEST_SEQ) != MC_TEST_SEQ )
		    continue;
		break;
	    case 1:
		if ( (flags & MC_TEST_SEQ1) != MC_TEST_SEQ1 )
		    continue;
		break;
	    case 2:
	      if ( (flags & MC_TEST_RAND) != MC_TEST_RAND )
		    continue;
	    } /*switch*/

	    for (nSize = 0; nSize < 3; nSize++) {
		switch (nSize) {
		case 0:
		    if ( (flags & MC_TEST_8) != MC_TEST_8 )
			continue;
		    ret = mc_test_row_8(nRun, nSeq, nFrom, nTo);
		    break;
		case 1:
		    if ( (flags & MC_TEST_16) != MC_TEST_16)
			continue;
		    ret = mc_test_row_16(nRun, nSeq, nFrom, nTo);
		    break;
		case 2:
		    if ( (flags & MC_TEST_32) != MC_TEST_32)
			continue;
		    ret = mc_test_row_32(nRun, nSeq, nFrom, nTo);
		} /*switch*/

		if (ret) return ret;
		mc_clear_row(nFrom, nTo);
	    } /*for nSize*/
	} /*for nSeq*/
	/*report(0xDEADDEAD);*/
    } /*for nRun*/

    return 0;
} /* mc_test_row */
