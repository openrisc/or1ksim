# Makefile.am for or1ksim OR1K various loop test programs.

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


# Test programs which just loop. One does nothing, one initializes certain
# addresses and SPR's (for testing reading through JTAG), one reports certain
# addresses and SPR's which have been changed (for testing writing through
# JTAG).
check_PROGRAMS = loop        \
		 loop-init   \
		 loop-report

# Simple loop
loop_SOURCES   = loop.c

loop_LDFLAGS   = -T$(srcdir)/../default.ld

loop_LDADD     = ../except/except.lo      \
	         ../support/libsupport.la

# Loop with initialization
loop_init_SOURCES = loop-init.c

loop_init_LDFLAGS = -T$(srcdir)/../default.ld

loop_init_LDADD   = ../except/except.lo      \
	            ../support/libsupport.la

# Loop with reporting
loop_report_SOURCES = loop-report.c

loop_report_LDFLAGS = -T$(srcdir)/../default.ld

loop_report_LDADD   = ../except/except.lo      \
	              ../support/libsupport.la
