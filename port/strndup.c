/* strndup.c -- Substitute implementation of strndup ()

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


/* Autoconf and/or portability configuration */
#include "config.h"


#ifndef HAVE_STRNDUP

/* the definition of size_t is provided in stddef.h.

   Mark Jarvin patch notes that stdlib.h and string.h are also needed. */
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/* Taken from glibc */
char *
strndup (const char *s, size_t n)
{
	char *new;
	size_t len = strlen (s);
	if (len>n)
		len=n;

	new = (char *) malloc (len + 1);

	if (new == NULL)
		return NULL;

	new[len] = '\0';
	return (char *) memcpy (new, s, len);
}

#endif	/* HAVE_STRNDUP */
