/* profiler.h -- profiling utility

   Copyright (C) 2001 Marko Mlinar, markom@opencores.org
   Copyright (C) 2008 Embecosm Limited

   Contributor Jeremy Bennett <jeremy.bennett@embecosm.com>

   This file is part of Or1ksim, the OpenRISC 1000 Architectural Simulator.

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the Free
   Software Foundation; either version 3 of the License, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
   more details.

   You should have received a copy of the GNU General Public License along
   with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* This program is commented throughout in a fashion suitable for processing
   with Doxygen. */


#ifndef PROFILER__H
#define PROFILER__H

/*! Maximum number of functions that can be profiled */
#define MAX_FUNCS 8192

/*! Data structure for information about functions */
struct func_struct {
  unsigned int  addr;		/*!< Start address of function */
  char          name[33];	/*!< Name of the function */
  long          cum_cycles;	/*!< Total cycles spent in function */
  long          calls;		/*!< Calls to this function */
};

/* Global data structures for external use */
extern struct func_struct  prof_func[MAX_FUNCS];
extern int                 prof_nfuncs;
extern int                 prof_cycles;

/* Function prototypes for external use */
int   prof_acquire (const char *fprofname);
void  prof_set (int  _quiet,
		int  _cumulative);
int   main_profiler (int   argc,
		     char *argv[],
		     int   just_help);

#endif	/* PROFILER__H */
