/* debug.h -- Trace function declarations

   Copyright 1999 Patrik Stridvall (for the wine project: www.winehq.com)
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


#ifndef DEBUG__H
#define DEBUG__H

enum __ORSIM_DEBUG_CLASS {
  __ORSIM_DBCL_TRACE,
  __ORSIM_DBCL_FIXME,
  __ORSIM_DBCL_WARN,
  __ORSIM_DBCL_ERR,
};

void orsim_dbg_log(enum __ORSIM_DEBUG_CLASS dbcl, const char *dbch,
                   const char *function, const char *format, ...)
                                          __attribute__((format(printf, 4, 5)));
void orsim_dbcl_set_name(enum __ORSIM_DEBUG_CLASS dbcl, const char *dbch, int on);
void parse_dbchs(const char *str);

#ifndef __ORSIM_DBG_USE_FUNC
#define __ORSIM_DBG_USE_FUNC __FUNCTION__
#endif

#define __ORSIM_GET_DEBUGGING_TRACE(dbch) ((dbch)[0] & (1 << __ORSIM_DBCL_TRACE))
#define __ORSIM_GET_DEBUGGING_WARN(dbch)  ((dbch)[0] & (1 << __ORSIM_DBCL_WARN))
#define __ORSIM_GET_DEBUGGING_FIXME(dbch) ((dbch)[0] & (1 << __ORSIM_DBCL_FIXME))
#define __ORSIM_GET_DEBUGGING_ERR(dbch)  ((dbch)[0] & (1 << __ORSIM_DBCL_ERR))

#define __ORSIM_GET_DEBUGGING(dbcl,dbch)  __ORSIM_GET_DEBUGGING##dbcl(dbch)

#define __ORSIM_DPRINTF(dbcl, dbch) \
  do { if(__ORSIM_GET_DEBUGGING(dbcl,(dbch))) { \
       const char * const __dbch = dbch; \
       const enum __ORSIM_DEBUG_CLASS __dbcl = __ORSIM_DBCL##dbcl; \
       __ORSIM_DEBUG_LOG

#define __ORSIM_DEBUG_LOG(args...) \
  orsim_dbg_log(__dbcl, __dbch, __ORSIM_DBG_USE_FUNC, args); } } while(0)
 
#define TRACE_(ch) __ORSIM_DPRINTF(_TRACE, __orsim_dbch_##ch)
#define FIXME_(ch) __ORSIM_DPRINTF(_FIXME, __orsim_dbch_##ch)
#define WARN_(ch) __ORSIM_DPRINTF(_WARN, __orsim_dbch_##ch)
#define ERR_(ch) __ORSIM_DPRINTF(_ERR, __orsim_dbch_##ch)

#define TRACE __ORSIM_DPRINTF(_TRACE, __orsim_dbch___default)
#define FIXME __ORSIM_DPRINTF(_FIXME, __orsim_dbch___default)
#define WARN __ORSIM_DPRINTF(_WARN, __orsim_dbch___default)
#define ERR __ORSIM_DPRINTF(_ERR, __orsim_dbch___default)

#define TRACE_ON(ch)          __ORSIM_GET_DEBUGGING(_TRACE,__orsim_dbch_##ch)
#define WARN_ON(ch)           __ORSIM_GET_DEBUGGING(_WARN,__orsim_dbch_##ch)
#define FIXME_ON(ch)          __ORSIM_GET_DEBUGGING(_FIXME,__orsim_dbch_##ch)
#define ERR_ON(ch)            __ORSIM_GET_DEBUGGING(_ERR,__orsim_dbch_##ch)

#define DEFAULT_DEBUG_CHANNEL(dbch) \
 extern char __orsim_dbch_##dbch[]; \
 static char * const __orsim_dbch___default = __orsim_dbch_##dbch;

#ifndef __ORSIM_NO_DEC_DBCH
#define DECLARE_DEBUG_CHANNEL(dbch) extern char __orsim_dbch_##dbch[];
#endif

/* For debugging purposes (of Or1ksim itself), it helps if debug can take
   advantage of the GNU C __attribute__ feature. */
#ifdef __GNUC__
void debug(int         level,
	   const char *format,...)
  __attribute__((format(printf, 2, 3)));
#else
void debug(int         level,
	   const char *format,...);
#endif

#endif	/* DEBUG__H */
