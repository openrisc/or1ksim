# Makefile.am for or1ksim OR1K test programs: int-logger-{edge,level}

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


# Test program which reports on interrupts received.
check_PROGRAMS      = int-logger

int_logger_SOURCES  = int-logger.c

int_logger_LDFLAGS  = -T$(srcdir)/../default.ld

int_logger_LDADD    = ../except/except.lo      \
	              ../support/libsupport.la
