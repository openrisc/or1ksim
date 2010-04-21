# Makefile.am for or1ksim uos test code

# Copyright (C) Marko Mlinar, 2001
# Copyright (C) Embecosm Limited, 2010

# Contributor Marko Mlinar
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


# A test program to show that basic C functionality works
check_PROGRAMS = uos

uos_SOURCES    = except-or32.S \
                 ipc.h         \
                 task.c        \
                 tick.c        \
                 uos.h         \
                 uos.c

uos_LDFLAGS    = -T$(srcdir)/../default.ld

uos_LDADD      = ../support/libsupport.la
