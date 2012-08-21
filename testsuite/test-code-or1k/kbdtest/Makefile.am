# Makefile.am for or1ksim testsuite CPU test program: kbdtest

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


# A test program of the Or1ksim keyboard
check_PROGRAMS   = kbdtest

kbdtest_SOURCES  = kbdtest.c

kbdtest_CPPFLAGS = -I$(srcdir)/../../../peripheral

kbdtest_LDFLAGS  = -T$(srcdir)/../default.ld

kbdtest_LDADD    = ../except/except.lo      \
	           ../support/libsupport.la

# For running the check, we'll need the Kbdtest source test file copied to
# the build testsuite directory.
check-local:
	cp $(top_srcdir)/../or1ksim.tests/kbdtest.rx $(top_builddir)/..
