/* port.h -- Portability interface header

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


#ifndef PORT__H
#define PORT__H

/* System includes */
#include <string.h>
#include <ctype.h>

#if HAVE_INTTYPES_H
#include <inttypes.h>
#endif


/* Substitute strndup () if necessary */
#ifndef HAVE_STRNDUP
char * strndup (const char *s, size_t n);
#endif

/* Substitute isblank () if necessary */
#ifndef HAVE_ISBLANK
int isblank(int c);
#endif

/* Substitute all the inttypes fields that are missing */

#ifndef PRIx32
# if SIZEOF_INT == 4
#  define PRIx32 "x"
# elif SIZEOF_LONG == 4
#  define PRIx32 "lx"
# endif
#endif

#ifndef PRIx16
# if SIZEOF_SHORT == 2
#  define PRIx16 "hx"
# else
#  define PRIx16 "x"
# endif
#endif

#ifndef PRIx8
# if SIZEOF_CHAR == 1
#  define PRIx8 "hhx"
# else
#  define PRIx8 "x"
# endif
#endif

#ifndef PRIi32
# if SIZEOF_INT == 4
#  define PRIi32 "i"
# elif SIZEOF_LONG == 4
#  define PRIi32 "li"
# endif
#endif

#ifndef PRId32
# if SIZEOF_INT == 4
#  define PRId32 "d"
# elif SIZEOF_LONG == 4
#  define PRId32 "ld"
# endif
#endif

#ifndef UINT32_C
# if SIZEOF_INT == 4
#  define UINT32_C(c) c
# elif SIZEOF_LONG == 4
#  define UINT32_C(c) c l
# endif
#endif

#ifndef HAVE_UINT32_T
# if SIZEOF_INT == 4
typedef unsigned int uint32_t;
# elif SIZEOF_LONG == 4
typedef unsigned long uint32_t;
# else
#  error "Can't find a 32-bit type"
# endif
#endif

#ifndef HAVE_INT32_T
# if SIZEOF_INT == 4
typedef signed int int32_t;
# elif SIZEOF_LONG == 4
typedef signed long int32_t;
# endif
#endif

#ifndef HAVE_UINT16_T
# if SIZEOF_SHORT == 2
typedef unsigned short uint16_t;
# else
#  error "Can't find a 16-bit type"
# endif
#endif

#ifndef HAVE_INT16_T
# if SIZEOF_SHORT == 2
typedef signed short int16_t;
# endif
#endif

#ifndef HAVE_UINT8_T
# if SIZEOF_CHAR == 1
typedef unsigned char uint8_t;
# else
#  error "Can't find a 8-bit type"
# endif
#endif

#ifndef HAVE_INT8_T
# if SIZEOF_CHAR == 1
typedef signed char int8_t;
# endif
#endif


#endif	/* PORT__H */
