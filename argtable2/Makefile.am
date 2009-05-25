# Makefile.am -- automake configuration file for argtable2
#
# Copyright (C) 1998-2001,2003-2008 Stewart Heitmann
# Copyright (C) 1989-1994, 1996-1999,2001,2003 Free Software Foundation, Inc.
# Copyright (C) 2008 Embecosm Limited
#
# argtable was written by Stewart Heitmann <sheitmann@users.sourceforge.net>
#
# This file is part of the argtable2 library, which in turn is part of the
# OpenRISC 1000 Architectural Simulator
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the Free
# Software Foundation; either version 3 of the License, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
# more details.
#
# You should have received a copy of the GNU General Public License along
# with this program.  If not, see <http://www.gnu.org/licenses/>.

# This is the source sub-directory of argtable2. The header comment and the
# change to make the library and header non-installed were added by Jeremy
# Bennett <jeremy.bennett@embecosm.com> to facilitate use in Or1ksim, the
# OpenRISC 1000 Architectural Simulator

noinst_LTLIBRARIES = libargtable2.la

libargtable2_la_SOURCES = argtable2.c arg_end.c arg_rem.c arg_lit.c arg_int.c arg_dbl.c \
			arg_str.c arg_file.c

libargtable2_la_CFLAGS  = $(DEBUGFLAGS)

#libtool library versioning uses current:revision:age where
#   current  = most recent interface number
#   revision = minor version of current interface
#   age      = how many previous interfaces this version supports (first=current-age)
# argtable versions prior to 2.4 did not use libtool versions thus
# argtable-2.4 is 0:0:0
# argtable-2.5 is 1:0:1	(interface 1 is back-compatible with interface 0, ie:argtable-2.4)
# argtable-2.6 is 1:1:1	(this code is identical to argtable2-5)
# argtable-2.7 is 1:2:1 (some minor bug fixes, interface is unchanged)
# argtable-2.8 is 1:3:1 (some minor bug fixes, interface is unchanged)
# argtable-2.9 is 1:4:1 (makefile bug fix only, src code is identical to argtable-2.8)
libargtable2_la_LDFLAGS = -version-info 1:4:1

noinst_HEADERS = argtable2.h

# If the system provides getopt_long then we use it,
# otherwise we compile in the version supplied with argtable.
if !USE_SYS_GETOPTLONG
libargtable2_la_SOURCES += getopt.c getopt1.c getopt.h
endif

# If the system provides regex then we may compile arg_rex
# otherwise we must omit it.
if USE_ARGREX
libargtable2_la_SOURCES += arg_rex.c
endif

# If the system provides strptime then we may compile arg_date
# otherwise we must omit it.
if USE_ARGDATE
libargtable2_la_SOURCES += arg_date.c
endif


