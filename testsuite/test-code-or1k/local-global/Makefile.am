# Makefile.am for or1ksim testsuite CPU test program: local-global

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


# A test program for C local and global variables
check_PROGRAMS       = local-global

local_global_SOURCES = local-global.c

local_global_LDFLAGS = -T$(srcdir)/../default.ld

local_global_LDADD   = ../except/except.lo      \
	               ../support/libsupport.la
