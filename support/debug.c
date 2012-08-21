/* debug.c -- Debug channel support code

   Copyright (C) 2005 György `nog' Jeney, nog@sdf.lonestar.org
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
#include "port.h"

/* System includes */
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

/* Package includes */
#include "arch.h"
#include "sim-config.h"

#define __ORSIM_NO_DEC_DBCH
#include "debug.h"

#define DECLARE_DEBUG_CHANNEL(dbch) char __orsim_dbch_##dbch[] = "\0"#dbch;
#include "dbchs.h"
#undef DECLARE_DEBUG_CHANNEL

#define DECLARE_DEBUG_CHANNEL(dbch) __orsim_dbch_##dbch,
static char *__orsim_dbchs[] = {
#include "dbchs.h"
NULL };
#undef DECLARE_DEBUG_CHANNEL

static const char *debug_classes[] = { "trace", "fixme", "warn", "err" };


void orsim_dbg_log(enum __ORSIM_DEBUG_CLASS dbcl, const char *dbch,
                   const char *function, const char *format, ...)
{
  va_list ap;
  static int last_lf = 1; /* There *has* to be a better way */

  if(last_lf)  {
    if(!TRACE_ON(cycles))
      fprintf(stderr, "%s:%s:%s: ", debug_classes[dbcl], dbch + 1, function);
    else
      fprintf(stderr, "%lld:%s:%s:%s: ", runtime.sim.cycles,
              debug_classes[dbcl], dbch + 1, function);
  }
  last_lf = format[strlen(format) - 1] == '\n'; /* This is wrong... */
  va_start(ap, format);
  vfprintf(stderr, format, ap);
  va_end(ap);
}

void orsim_dbcl_set(enum __ORSIM_DEBUG_CLASS dbcl, char *dbch, int on)
{
  if(on)
    dbch[0] |= 1 << dbcl;
  else
    dbch[0] &= ~(1 << dbcl);
}

void orsim_dbcl_set_name(enum __ORSIM_DEBUG_CLASS dbcl, const char *dbch, int on)
{
  char **dbchs = __orsim_dbchs; 

  for(dbchs = __orsim_dbchs; *dbchs; dbchs++) {
    if(!strcmp(*dbchs + 1, dbch)) {
      orsim_dbcl_set(dbcl, *dbchs, on);
      break;
    }
  }
}

void parse_dbchs(const char *str)
{
  enum __ORSIM_DEBUG_CLASS dbcl = 0;
  int i;
  int disen;
  int all;
  const char *cend;
  const char *chan_end;

  while(*str) {
    cend = strpbrk(str, "+-");
    chan_end = strchr(str, ',');
    if(!chan_end)
      chan_end = str + strlen(str);

    if(!cend || (cend > chan_end)) {
      disen = 1;
      cend = --str;
    } else
      disen = *cend == '+' ? 1 : 0;

    if(cend == str) {
      all = 1;
    } else {
      for(i = 0; i < 4; i++) {
        if(!strncmp(str, debug_classes[i], cend - str)) {
          dbcl = i;
          break;
        }
      }
      if(i >= 4)
        fprintf(stderr, "Unknown class specified\n");
      all = 0;
    }
    cend++;

    for(i = 0; __orsim_dbchs[i]; i++)
      if(!strncmp(cend, __orsim_dbchs[i] + 1, chan_end - cend))
        break;

    if(!__orsim_dbchs[i])
      fprintf(stderr, "Unknown channel specified\n");
    else if(all) {
      orsim_dbcl_set(__ORSIM_DBCL_TRACE, __orsim_dbchs[i], disen);
      orsim_dbcl_set(__ORSIM_DBCL_FIXME, __orsim_dbchs[i], disen);
      orsim_dbcl_set(__ORSIM_DBCL_WARN, __orsim_dbchs[i], disen);
      orsim_dbcl_set(__ORSIM_DBCL_ERR, __orsim_dbchs[i], disen);
    } else
      orsim_dbcl_set(dbcl, __orsim_dbchs[i], disen);
    if(*chan_end)
      str = chan_end + 1;
    else
      str = chan_end;
  }
}


/*---------------------------------------------------------------------------*/
/*!Internal debug function

   Print the message if the level is greater than or equal to that specified
   in the configuration.

   @param[in] level   The debug level of this message
   @param[in] format  Varargs format string
   @param[in] ...     The varargs required by the string                     */
/*---------------------------------------------------------------------------*/
void
debug (int         level,
       const char *format,
                   ...)
{
  if (config.sim.debug >= level)
    {
      va_list  ap;

      va_start (ap, format);
      vfprintf (runtime.sim.fout, format, ap);
      fflush (runtime.sim.fout);
    }
}	/* debug() */


