# Makefile.am for libor1ksim test program: lib-inttest

# Copyright (C) Embecosm Limited, 2010

# Contributor Jeremy Bennett <jeremy.bennett@embecosm.com>

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


# Test programs of the library interrupt driving functions. Two versions, one
# to drive edge based interrupts, one to drive level based interrupts.
check_PROGRAMS            = lib-inttest-edge  \
			    lib-inttest-level

# Edge based interrupt
lib_inttest_edge_SOURCES  = lib-inttest-edge.c

lib_inttest_edge_LDADD    = $(top_builddir)/libsim.la

# Level based interrupt
lib_inttest_level_SOURCES = lib-inttest-level.c

lib_inttest_level_LDADD   = $(top_builddir)/libsim.la
