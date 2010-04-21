/* functest.c. Function parameter test for Or1ksim

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

/* Simple test, that test function parameters */
#include "support.h"

int gk = 0;
int c = 0;
int fun1 (int a, int b, int c, int d, int e, int f, int g)
{
  int j = 0;
          
  while(j < 2) {
    a++;
    j++;
  }

  return a;
}

int main(void)
{
  int i, j, k;
  i = j = k = 0;

  while (c++ < 10) {
    j = fun1(gk, k + 1, k + 2, k + 3, k + 4, k + 5, k + 6);
    printf ("%i\n", gk);
    if(j > 40)
      gk = j - 20;
    else
      gk = j;
  }
  report (gk);
  if (gk == 20)
    report(0xdeaddead);
  return (gk != 20);
}
