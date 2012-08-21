/* support.h Support headers for testing Or1ksim.

   Copyright (C) 1999 Damjan Lampret, lampret@opencores.org
   Copyright (C) 2010 Embecosm Limited

   Contributor Damjan Lampret <lampret@opencores.org>
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

/* This file should is included in each C test. It calls main () function and
   add support for basic functions */

#ifndef SUPPORT_H
#define SUPPORT_H

#include <stdarg.h>
#include <stddef.h>
#include <limits.h>


/*! Convenience macros for accessing memory. */
#define REG8(add)  *((volatile unsigned char *)  (add))
#define REG16(add) *((volatile unsigned short *) (add))
#define REG32(add) *((volatile unsigned long *)  (add))

/* Start function */
extern void  reset ();

/* Return a value by making a syscall */
extern void exit (int i) __attribute__ ((__noreturn__));

/* Version of putchar that works with Or1ksim */
extern int  putchar (int  c);

/* Version of puts that works with Or1ksim */
extern int  puts (const char *str);

/* Restricted version of printf that works with Or1ksim */
extern int  printf (const char *fmt,
		    ...);

/* Prints out a value */
extern void  report (unsigned long int  value);

/* Read the simulator timer */
extern unsigned long int  read_timer ();

/* For writing into SPR. */
extern void  mtspr (unsigned long int  spr,
		    unsigned long int  value);

/* For reading SPR. */
extern unsigned long int  mfspr (unsigned long int  spr);

/* memcpy clone */
extern void *memcpy (void *__restrict         __dest,
                     __const void *__restrict __src,
		     size_t                   __n);

/* pseudo-random number generator */
extern unsigned long int rand ();

/* Externally used exception handlers */
extern unsigned long int  excpt_buserr;
extern unsigned long int  excpt_dpfault;
extern unsigned long int  excpt_ipfault;
extern unsigned long int  excpt_tick;
extern unsigned long int  excpt_align;
extern unsigned long int  excpt_illinsn;
extern unsigned long int  excpt_int;
extern unsigned long int  excpt_dtlbmiss;
extern unsigned long int  excpt_itlbmiss;
extern unsigned long int  excpt_range;
extern unsigned long int  excpt_syscall;
extern unsigned long int  excpt_break;
extern unsigned long int  excpt_trap;

#endif
