# Makefile.am for or1ksim testsuite CPU test program: testsoftfloat, testfloat

# Copyright (C) ORSoC AB, 2010

# Contributor Julius Baxter <julius.baxter@orsoc.se>

# This file is part of OpenRISC 1000 Architectural Simulator.

# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the Free
# Software Foundation; either version 3 of the License, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
# more details.

# You should have received a copy of the GNU General Public License along
# with this program.  If not, see <http:#www.gnu.org/licenses/>.  */

# -----------------------------------------------------------------------------
# This code is commented throughout for use with Doxygen.
# -----------------------------------------------------------------------------


# A test program of the OR1K Ethernet
check_PROGRAMS = testsoftfloat testfloat

common_testfloat_sources = milieu.h \
			or1k-gcc.h 	\
			fail.h		\
			fail.c		\
			random.h	\
			random.c	\
			writeHex.h	\
			writeHex.c	\
			softfloat.h	\
			softfloat.c	\
			testCases.h	\
			testCases.c	\
			testLoops.h	\
			testLoops.c

testsoftfloat_SOURCES    = $(common_testfloat_sources)	\
			slowfloat.h	\
			slowfloat.c	\
			testsoftfloat.c

testsoftfloat_CFLAGS    = -O3
testsoftfloat_LDFLAGS    = -T$(srcdir)/../default.ld -lgcc

testsoftfloat_LDADD      = ../except/except.lo      \
				../support/libsupport.la

testfloat_SOURCES    = $(common_testfloat_sources)	\
			systmodes.h	\
			systmodes.c	\
			systflags.h	\
			systflags.c	\
			systfloat.h	\
			systfloat.S	\
			testFunction.h	\
			testFunction.c	\
			testfloat.c
testfloat_CFLAGS    = -O2
testfloat_LDFLAGS    = -T$(srcdir)/../default.ld -lgcc

testfloat_LDADD      =  ../except/except.lo      \
			../support/libsupport.la

