# Makefile.am for or1ksim testsuite CPU test program: atomic

# Copyright (C) 2014, Stefan Kristiansson <stefan.kristiansson@saunalahti.fi>

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


# A test program for the atomic l.lwa and l.swa instructions
check_PROGRAMS = atomic

atomic_SOURCES  = atomic.S

atomic_LDFLAGS  = -T$(srcdir)/../default.ld

atomic_LDADD   = ../except/except.lo      \
	         ../support/libsupport.la
