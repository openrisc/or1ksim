# Makefile.am for or1ksim testsuite CPU test program: mc-dram

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


# A test of memory control of DRAM
check_PROGRAMS   = mc-dram

mc_dram_SOURCES  = mc-dram.c \
	           mc-dram.h

mc_dram_CPPFLAGS = -I$(srcdir)/../../../peripheral \
                   -I$(srcdir)/../../../cpu/common \
                   -I$(srcdir)/../../../cpu/or1k   \
                   -I$(srcdir)/../mc-common		\
                   -I$(srcdir)/../../../port

mc_dram_LDFLAGS  = -T$(srcdir)/../except-mc.ld

mc_dram_LDADD    = ../mc-common/except-mc.lo    \
		   ../mc-common/libmc-common.la \
		   ../support/libsupport.la
