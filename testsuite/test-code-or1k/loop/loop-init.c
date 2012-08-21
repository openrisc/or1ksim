/* loop-init.c. Or1ksim simple C loop program which initializes data.

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

/* A program which initializes memory and SPR, then just loops. Used in
   testing libor1ksim JTAG read functionality. */

#include "support.h"
#include "spr-defs.h"

int
main ()
{
  unsigned long int  r;
  int                i;

  /* Set the SR to have SUMRA bit set, so that we can access certain regs in
     user mode. */
  r = mfspr (SPR_SR);
  mtspr (SPR_SR, r | SPR_SR_SUMRA);

  /* Set the MACLO and MACHI regs */
  mtspr (SPR_MACLO, 0xdeadbeef);
  mtspr (SPR_MACHI, 0xcafebabe);

  /* Set main memory starting at 0x100000. Set at later intervals 2^16 bytes
     further on as well. */
  for (i = 0; i < 16; i++)
    {
      unsigned char *block0 = (unsigned char *) (0x100000 + i);
      unsigned char *block1 = (unsigned char *) (0x110000 + i);
      unsigned char *block2 = (unsigned char *) (0x120000 + i);

      *block0 = 16 + i;
      *block1 = 32 + i;
      *block2 = 48 + i;
    }

  /* Set the top of each memory block and the bottom of the second memory
     block to a defined sequence. */
  unsigned char *mem1top = (unsigned char *) (0x001ffff8);
  unsigned char *mem2bot = (unsigned char *) (0xffe00000);
  unsigned char *mem2top = (unsigned char *) (0xfffffff8);

  mem1top[0] = 0xde;
  mem1top[1] = 0xad;
  mem1top[2] = 0xbe;
  mem1top[3] = 0xef;
  mem1top[4] = 0xca;
  mem1top[5] = 0xfe;
  mem1top[6] = 0xba;
  mem1top[7] = 0xbe;

  mem2bot[0] = 0xde;
  mem2bot[1] = 0xad;
  mem2bot[2] = 0xbe;
  mem2bot[3] = 0xef;
  mem2bot[4] = 0xca;
  mem2bot[5] = 0xfe;
  mem2bot[6] = 0xba;
  mem2bot[7] = 0xbe;

  mem2top[0] = 0xde;
  mem2top[1] = 0xad;
  mem2top[2] = 0xbe;
  mem2top[3] = 0xef;
  mem2top[4] = 0xca;
  mem2top[5] = 0xfe;
  mem2top[6] = 0xba;
  mem2top[7] = 0xbe;

  /* Loop for ever doing stuff */
  while (1)
    {
    }
}	/* main () */
