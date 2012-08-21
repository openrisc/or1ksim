# Makefile.am for or1ksim OR1K test program: upcalls

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


# Test programs for upcalls
check_PROGRAMS       = upcall-basic      \
		       upcall-misaligned

# Basic test
upcall_basic_SOURCES = upcall-basic.c

upcall_basic_LDFLAGS = -T$(srcdir)/../default.ld

upcall_basic_LDADD   = ../except/except.lo      \
	               ../support/libsupport.la

# Test for misaligned calls
upcall_misaligned_SOURCES = upcall-misaligned.c

upcall_misaligned_LDFLAGS = -T$(srcdir)/../default.ld

upcall_misaligned_LDADD   = ../except/except.lo      \
	                    ../support/libsupport.la
