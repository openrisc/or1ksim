/* icache-model.h -- instruction cache header file

   Copyright (C) 1999 Damjan Lampret, lampret@opencores.org
   Copyright (C) 2008 Embecosm Limited
  
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

/* This program is commented throughout in a fashion suitable for processing
   with Doxygen. */


#ifndef ICACHE_MODEL__H
#define ICACHE_MODEL__H

/* Autoconf and/or portability configuration */
#include "port.h"

/* Package includes */
#include "sim-config.h"


struct ic
{
  uint8_t      *mem;
  unsigned int *lrus;
  oraddr_t     *tags;

  int           enabled;	    /* Whether instruction cache is enabled */
  unsigned int  nways;		    /* Number of IC ways */
  unsigned int  nsets;		    /* Number of IC sets */
  unsigned int  blocksize;	    /* IC entry size */
  unsigned int  ustates;	    /* number of IC usage states */
  int           missdelay;	    /* How much cycles does the miss cost */
  int           hitdelay;	    /* How much cycles does the hit cost */
  unsigned int  blocksize_log2;	    /* log2(blocksize) */
  oraddr_t      set_mask;	    /* Mask to get set number */
  oraddr_t      tagaddr_mask;	    /* Mask to get tag address */
  oraddr_t      last_way;	    /* nways * nsets */
  oraddr_t      block_offset_mask;  /* mask to get offset into block */
  oraddr_t      block_mask;	    /* mask to get block number */
  unsigned int  ustates_reload;	    /* ustates - 1 */
};

/* External variables */
extern struct ic *ic_state;

/* Prototypes for external use */
extern uint32_t  ic_simulate_fetch (oraddr_t fetchaddr,
				    oraddr_t virt_addr);
extern void      ic_inv (oraddr_t dataaddr);
extern void      reg_ic_sec ();

#endif	/* ICACHE_MODEL__H */
