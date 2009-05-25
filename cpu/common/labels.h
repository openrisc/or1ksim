/* labels.h -- Abstract entities header file handling labels

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


#ifndef LABELS__H_
#define LABELS__H_

/* Package include */
#include "abstract.h"

/*! Structure for holding one label per particular memory location */
struct label_entry
{
  char               *name;
  oraddr_t            addr;
  struct label_entry *next;
};

/*! Structure repesenting a breakpoint */
struct breakpoint_entry
{
  oraddr_t                 addr;
  struct breakpoint_entry *next;
};

/* Globally visible variables */
extern struct breakpoint_entry *breakpoints;

/* Function prototypes for external use */
extern void                init_labels ();
extern void                add_label (oraddr_t  addr,
				      char     *name);
extern struct label_entry *get_label (oraddr_t addr);
extern struct label_entry *find_label (char *name);
extern oraddr_t            eval_label (char *name);
extern void                add_breakpoint (oraddr_t addr);
extern void                remove_breakpoint (oraddr_t addr);
extern void                print_breakpoints ();
extern int                 has_breakpoint (oraddr_t addr);
extern void                init_breakpoints ();

#endif	/* LABELS__H_ */
