/* int.c -- Header for interrupt handling for Or1ksim tests.

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

/* Number of interrupt handlers */
#define MAX_INT_HANDLERS	32

/* Handler entry */
struct ihnd {
	void 	(*handler)(void *);
	void	*arg;
};

/* Add interrupt handler */ 
int int_add(unsigned long vect, void (* handler)(void *), void *arg);

/* Initialize routine */
int int_init();
