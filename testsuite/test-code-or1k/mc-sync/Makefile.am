# Makefile.am for or1ksim testsuite CPU test program: mc-sync

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


# A test of memory control of synchronous memory. This code currently does
# not work.
check_PROGRAMS   = mc-sync

mc_sync_SOURCES  = mc-sync.c \
	           mc-sync.h

mc_sync_CPPFLAGS = -I$(srcdir)/../../../peripheral \
                   -I$(srcdir)/../../../cpu/common \
                   -I$(srcdir)/../../../cpu/or1k   \
                    -I$(srcdir)/../mc-common		\
                   -I$(srcdir)/../../../port

mc_sync_LDFLAGS  = -T$(srcdir)/../except-mc.ld

mc_sync_LDADD    = ../mc-common/except-mc.lo    \
	 	   ../mc-common/libmc-common.la \
		   ../support/libsupport.la
