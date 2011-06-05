# Makefile -- Makefile for OR32 dependent simulation
#
# Copyright (C) 1999 Damjan Lampret, lampret@opencores.org
# Copyright (C) 2008 Embecosm Limited
#
# Contributor Jeremy Bennett <jeremy.bennett@embecosm.com>
#
# This file is part of OpenRISC 1000 Architectural Simulator.
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


noinst_LTLIBRARIES = libarch.la

if GENERATE_NEEDED

EXTRA_DIST         = insnset.c

libarch_la_SOURCES = execute.c         \
                     or32.c            \
		     simpl32-defs.h

BUILT_SOURCES      = execgen.c

noinst_PROGRAMS    = generate$(EXEEXT)

generate_SOURCES   = or32.c            \
                     generate.c        \
                     simpl32-defs.h

generate_CFLAGS    = $(AM_CFLAGS)

execgen.c: generate$(EXEEXT) insnset.c
	./generate$(EXEEXT) $(srcdir)/insnset.c execgen.c

else

libarch_la_SOURCES = execute.c         \
                     or32.c            \
		     simpl32-defs.h 

dist_EXTRA_libarch_la_SOURCES = insnset.c

endif


# If the simulator was first built without --enable-simple and then with it,
# then also remove these two files

CLEANFILES = execgen.c \
             generate
