/* arch.h -- OR1K architecture specific macros

   Copyright (C) 1999 Damjan Lampret, lampret@opencores.org
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


#ifndef ARCH__H
#define ARCH__H

/* Autoconf and/or portability configuration */
#include "port.h"

/*! Index of the link register */
#define LINK_REGNO     9

/* Conversion macros */
#define ADDR_C(c)  UINT32_C(c)
#define REG_C(c)   UINT32_C(c)

/* Strings for printing OpenRISC types */
#define PRIxADDR  "08" PRIx32	/*!< print an openrisc address in hex */
#define PRIxREG   "08" PRIx32	/*!< print an openrisc register in hex */
#define PRIdREG   PRId32	/*!< print an openrisc register in decimals */

/* Basic types for openrisc */
typedef uint32_t  oraddr_t;	/*!< Address as addressed by openrisc */
typedef uint32_t  uorreg_t;	/*!< An unsigned register of openrisc */
typedef int32_t   orreg_t;	/*!< A signed register of openrisc */

#endif /* ARCH__H */
