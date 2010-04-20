/* inst-set-test.c. Instruction set test for Or1ksim

   Copyright (C) 1999-2006 OpenCores
   Copyright (C) 2010 Embecosm Limited

   Contributors various OpenCores participants
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

/* This is a complex instruction test for OR1200 */
/* trap, movhi, mul, nop, rfe, sys instructions not tested*/
/* Currently not working. Compiles with warnings, runs with errors. */

#include "support.h"

volatile unsigned long test = 0xdeaddead;

#define TEST_32(c1,c2,val1,val2,op) \
  test ^= ((c1 (val1)) op (c2 (val2)));  test ^= ((c1 (val2)) op (c2 (val1)));\
  test ^= ((c1 (val1)) op (c2 (val2)));  test ^= ((c1 (val2)) op (c2 (val1)));\
  test ^= ((c1 (val1)) op (c2 (val2)));  test ^= ((c1 (val2)) op (c2 (val1)));\
  test ^= ((c1 (val1)) op (c2 (val2)));  test ^= ((c1 (val2)) op (c2 (val1)));\
  test ^= ((c1 (val1)) op (c2 (val2)));  test ^= ((c1 (val2)) op (c2 (val1)));\
  test ^= ((c1 (val1)) op (c2 (val2)));  test ^= ((c1 (val2)) op (c2 (val1)));\
  test ^= ((c1 (val1)) op (c2 (val2)));  test ^= ((c1 (val2)) op (c2 (val1)));\
  test ^= ((c1 (val1)) op (c2 (val2)));  test ^= ((c1 (val2)) op (c2 (val1)));\

#define TEST_CASTS(val1,val2,op)\
  TEST_32((unsigned long), (unsigned long), val1, val2, op);\
  TEST_32((unsigned long), (signed long), val1, val2, op);\
  TEST_32((unsigned long), (unsigned short), val1, val2, op);\
  TEST_32((unsigned long), (signed short), val1, val2, op);\
  TEST_32((unsigned long), (unsigned char), val1, val2, op);\
  TEST_32((unsigned long), (signed char), val1, val2, op);\
    \
  TEST_32((unsigned short), (unsigned long), val1, val2, op);\
  TEST_32((unsigned short), (signed long), val1, val2, op);\
  TEST_32((unsigned short), (unsigned short), val1, val2, op);\
  TEST_32((unsigned short), (signed short), val1, val2, op);\
  TEST_32((unsigned short), (unsigned char), val1, val2, op);\
  TEST_32((unsigned short), (signed char), val1, val2, op);\
    \
  TEST_32((unsigned char), (unsigned long), val1, val2, op);\
  TEST_32((unsigned char), (signed long), val1, val2, op);\
  TEST_32((unsigned char), (unsigned short), val1, val2, op);\
  TEST_32((unsigned char), (signed short), val1, val2, op)\
  TEST_32((unsigned char), (unsigned char), val1, val2, op);\
  TEST_32((unsigned char), (signed char), val1, val2, op);


void add_test ()
{     
  int i, j;                                          
  TEST_CASTS(0x12345678, 0x12345678, +);
  TEST_CASTS(0x12345678, 0x87654321, +);
  TEST_CASTS(0x87654321, 0x12345678, +);
  TEST_CASTS(0x87654321, 0x87654321, +);
  
  TEST_CASTS(0x1234, -0x1234, +);
  TEST_CASTS(0x1234, -0x1234, +);
  TEST_CASTS(-0x1234, 0x1234, +);
  TEST_CASTS(-0x1234, -0x1234, +);
  
  for (i = -1; i <= 1; i++)
    for (j = -1; j <= 1; j++)
      TEST_CASTS (i, j, +);
  report (test);
}

void and_test ()
{     
/*  TEST_CASTS(0x12345678, 0x12345678, &);
  TEST_CASTS(0x12345678, 0x87654321, &);
  TEST_CASTS(0x87654321, 0x12345678, &);
  TEST_CASTS(0x87654321, 0x87654321, &);
  
  TEST_CASTS(0x12345678, 0x0, &);
  TEST_CASTS(0x12345678, 0xffffffff, &);
  TEST_CASTS(0x87654321, 0x80000000, &);
  TEST_CASTS(0x87654321, 0x08000000, &);
  
  TEST_CASTS(0x12345678, 0x12345678, &&);
  TEST_CASTS(0x12345678, 0x87654321, &&);
  TEST_CASTS(0x87654321, 0x12345678, &&);
  TEST_CASTS(0x87654321, 0x87654321, &&);
  
  TEST_CASTS(0x12345678, 0x0, &&);
  TEST_CASTS(0x12345678, 0xffffffff, &&);
  TEST_CASTS(0x87654321, 0x80000000, &&);
  TEST_CASTS(0x87654321, 0x08000000, &&);
  report (test);*/
}

void branch_test ()
{
        /* bf, bnf, j, jal, jalr, jr, sfeq, sfges, sfgeu, sfgts, sfgtu, sfles, sfleu, sflts, sfltu, sfne */
  report (test);
}

void load_store_test ()
{
  volatile long a;
  volatile short b;
  volatile char c;
  unsigned long *pa = (unsigned long *)&a;
  unsigned short *pb = (unsigned short *)&b;
  unsigned char *pc = (unsigned char *)&c;
  
  test ^= a = 0xdeadbeef;
  test ^= b = 0x12345678;
  test ^= c = 0x87654321;
  test ^= a = b;
  test ^= b = c;
  test ^= a;
  test ^= (unsigned long)a;
  test ^= (unsigned short)a;
  test ^= (unsigned char)a;
  
  test ^= (unsigned long)b;
  test ^= (unsigned short)b;
  test ^= (unsigned char)b;
  
  test ^= (unsigned long)c;
  test ^= (unsigned short)c;
  test ^= (unsigned char)c;
  
  test ^= *pa = 0xabcdef12;
  test ^= *pb = 0x12345678;
  test ^= *pc = 0xdeadbeef;

  test ^= (signed long)c;
  test ^= (signed short)c;
  test ^= (signed char)c;
  
  test ^= (signed long)a;
  test ^= (signed short)a;
  test ^= (signed char)a;
  
  test ^= (signed long)b;
  test ^= (signed short)b;
  test ^= (signed char)b;
  
  test ^= *pa = 0xaabbccdd;
  test ^= *pb = 0x56789012;
  test ^= *pc = 0xb055b055;

  test ^= (unsigned long)b;
  test ^= (signed long)c;
  test ^= (unsigned long)a;
  test ^= (unsigned short)c;
  test ^= (unsigned short)a;
  test ^= (unsigned char)c;
  test ^= (unsigned short)b;
  test ^= (unsigned char)b;
  test ^= (unsigned char)a;
  report (test);
}

void or_test ()
{     
/*  TEST_CASTS(0x12345678, 0x12345678, |);
  TEST_CASTS(0x12345678, 0x87654321, |);
  TEST_CASTS(0x87654321, 0x12345678, |);
  TEST_CASTS(0x87654321, 0x87654321, |);
  
  TEST_CASTS(0x12345678, 0x0, |);
  TEST_CASTS(0x12345678, 0xffffffff, |);
  TEST_CASTS(0x87654321, 0x80000000, |);
  TEST_CASTS(0x87654321, 0x08000000, |);
  
  TEST_CASTS(0x12345678, 0x12345678, ||);
  TEST_CASTS(0x12345678, 0x87654321, ||);
  TEST_CASTS(0x87654321, 0x12345678, ||);
  TEST_CASTS(0x87654321, 0x87654321, ||);
  
  TEST_CASTS(0x12345678, 0x0, ||);
  TEST_CASTS(0x12345678, 0xffffffff, ||);
  TEST_CASTS(0x87654321, 0x80000000, ||);
  TEST_CASTS(0x87654321, 0x08000000, ||);*/
  report (test);
}

void xor_test ()
{     
/*  TEST_CASTS(0x12345678, 0x12345678, ^);
  TEST_CASTS(0x12345678, 0x87654321, ^);
  TEST_CASTS(0x87654321, 0x12345678, ^);
  TEST_CASTS(0x87654321, 0x87654321, ^);
  
  TEST_CASTS(0x12345678, 0x0, ^);
  TEST_CASTS(0x12345678, 0xffffffff, ^);
  TEST_CASTS(0x87654321, 0x80000000, ^);
  TEST_CASTS(0x87654321, 0x08000000, ^);*/
  report (test);
}

void sll_test ()
{
  int i;
  for (i = -1; i < 40; i++)
    TEST_CASTS(0xdeaf1234, i, <<);
  for (i = -1; i < 33; i++)
    TEST_CASTS(0x12345678, i, <<);
  for (i = -1; i < 33; i++)
    TEST_CASTS(0xdea12345, i, <<);
  
  test ^= (unsigned long)0xabcd4321 << test;
  test ^= (signed long)0xabcd4321 << test;
  test ^= (unsigned long)0xabcd << test;
  test ^= (signed long)0xabcd << test;
  report (test);
}

void srl_sra_test ()
{
  int i;
  for (i = -1; i < 40; i++)
    TEST_CASTS(0xdeaf1234, i, >>);
  for (i = -1; i < 33; i++)
    TEST_CASTS(0x12345678, i, >>);
  for (i = -1; i < 33; i++)
    TEST_CASTS(0xdea12345, i, >>);
  
  test ^= (unsigned long)0xabcd4321 >> test;
  test ^= (signed long)0xabcd4321 >> test;
  test ^= (unsigned long)0xabcd >> test;
  test ^= (signed long)0xabcd >> test;
  report (test);
}


void ror_test ()
{
  unsigned long a;
  int i;
  for (i = -1; i < 40; i++) {
    asm ("l.ror %0, %1, %2" : "=r" (a) : "r" (0x12345678), "r" (i));
    test ^= a;
    asm ("l.ror %0, %1, %2" : "=r" (a) : "r" (0xabcdef), "r" (i));
    test ^= a;
  }
  asm ("l.ror %0, %1, %2" : "=r" (a) : "r" (0x12345678), "r" (0x12345678));
  test ^= a;
  report (test);
}

void sub_test ()
{     
/*  int i, j;                                          
  TEST_CASTS(0x12345678, 0x12345678, -);
  TEST_CASTS(0x12345678, 0x87654321, -);
  TEST_CASTS(0x87654321, 0x12345678, -);
  TEST_CASTS(0x87654321, 0x87654321, -);
  
  TEST_CASTS(0x1234, -0x1234, -);
  TEST_CASTS(0x1234, -0x1234, -);
  TEST_CASTS(-0x1234, 0x1234, -);
  TEST_CASTS(-0x1234, -0x1234, -);
  
  for (i = -1; i <= 1; i++)
    for (j = -1; j <= 1; j++)
      TEST_CASTS (i, j, -);
  report (test);*/
}

int main ()
{
  add_test ();
  and_test ();
  branch_test ();
  load_store_test ();
  or_test ();
  sll_test ();
  srl_sra_test ();
  xor_test ();
  sub_test ();
  return 0;
}
