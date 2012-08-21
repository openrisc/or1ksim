/* fbtest.c. Test of Or1ksim frame buffer

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

/* Simple frame buffer test. Draws some horizontal with different colors. */
#include "support.h"

#define BUFADDR   0x00100000
#define BASEADDR  0x97000000
#define BUF_PTR  ((unsigned char *)BUFADDR)
#define PAL_PTR  ((unsigned long *)(BASEADDR + 0x400))
#define SIZEX     640
#define SIZEY     480

#define putxy(x,y) *(BUF_PTR + (x) + (y) * SIZEX)
#define setpal(i,r,g,b) *(PAL_PTR + (i)) = (((unsigned long)(r) & 0xff) << 16) | (((unsigned long)(g) & 0xff) << 8) | (((unsigned long)(b) & 0xff) << 0)

void hline (int y, int x1, int x2, unsigned char c)
{
  int x;
  for (x = x1; x < x2; x++)
    putxy(x, y) = c;
}

int main(void)
{
  unsigned i;
  
  /* Set address of buffer at predefined location */
  *((unsigned long *)(BASEADDR) + 0x1) = BUFADDR;
  for (i = 0; i < 256; i++)
    setpal (i, 256 - i, i, 128 ^ i);

  /* Turn display on */  
  *((unsigned long *)(BASEADDR) + 0x0) = 0xffffffff;
    
  for (i = 0; i < 16; i++) {
    hline (i, 0, i, i);
    hline (256 - i, 256 - i, 256 + i, i);
  }
  
  report (0xdeaddead);
  return 0;
}
