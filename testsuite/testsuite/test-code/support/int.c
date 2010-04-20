/* int.c -- Interrupt handling for Or1ksim tests.

   Copyright (C) 2001 Simon Srot, srot@opencores.org
   Copyright (C) 2008, 2010 Embecosm Limited
  
   Contributor Simon Srot <srot@opencores.org>
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
   with this program.  If not, see <http://www.gnu.org/licenses/>. */

/* ----------------------------------------------------------------------------
   This code is commented throughout for use with Doxygen.
   --------------------------------------------------------------------------*/

/* This file is part of test microkernel for OpenRISC 1000. */

#include "support.h"
#include "spr-defs.h"
#include "int.h"

/* Interrupt handlers table */
struct ihnd int_handlers[MAX_INT_HANDLERS];

/* Initialize routine */
int int_init()
{
  int i;

  for(i = 0; i < MAX_INT_HANDLERS; i++) {
    int_handlers[i].handler = 0;
    int_handlers[i].arg = 0;
  }
  
  return 0;
}

/* Add interrupt handler */ 
int int_add(unsigned long vect, void (* handler)(void *), void *arg)
{
  if(vect >= MAX_INT_HANDLERS)
    return -1;

  int_handlers[vect].handler = handler;
  int_handlers[vect].arg = arg;

  mtspr(SPR_PICMR, mfspr(SPR_PICMR) | (0x00000001L << vect));
    
  return 0;
}

/* Disable interrupt */ 
int int_disable(unsigned long vect)
{
  if(vect >= MAX_INT_HANDLERS)
    return -1;

  mtspr(SPR_PICMR, mfspr(SPR_PICMR) & ~(0x00000001L << vect));
  
  return 0;
}

/* Enable interrupt */ 
int int_enable(unsigned long vect)
{
  if(vect >= MAX_INT_HANDLERS)
    return -1;

  mtspr(SPR_PICMR, mfspr(SPR_PICMR) | (0x00000001L << vect));
  
  return 0;
}

/* Main interrupt handler */
void int_main()
{
  unsigned long picsr = mfspr(SPR_PICSR);
  unsigned long i = 0;

  mtspr(SPR_PICSR, 0);

  while(i < 32) {
    if((picsr & (0x01L << i)) && (int_handlers[i].handler != 0)) {
      (*int_handlers[i].handler)(int_handlers[i].arg); 
      mtspr(SPR_PICSR, mfspr(SPR_PICSR) & ~(0x00000001L << i));
    }
    i++;
  }
}
  
