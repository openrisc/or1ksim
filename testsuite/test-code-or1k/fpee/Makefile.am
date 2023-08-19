# Makefile.am for or1ksim testsuite CPU test program: fpee

# Copyright (C) OpenRISC Developers, 2023

# Contributor Stafford Horne <shorne@gmail.com>

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

# A test program of the floating point exceptions test
check_PROGRAMS = fpee

fpee_SOURCES   = fpee.c

fpee_LDFLAGS   = -T$(srcdir)/../default.ld

fpee_LDADD     = ../except/except.lo \
		 ../support/libsupport.la
