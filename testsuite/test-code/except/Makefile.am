# Makefile.am for or1ksim testsuite exception sub-module

# Copyright (C) Damjan Lampret <lampret@opencores.org> 1999
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


# Exception library for use when testing. In practice we don't use the
# library, since it doesn't define any global variables, so as a library won't
# be included when linking, even though the linker script requires it. Instead
# we use the object file created as an intermediate.
check_LTLIBRARIES    = libexcept.la

libexcept_la_SOURCES = except.S
