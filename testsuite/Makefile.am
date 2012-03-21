# Makefile.am for or1ksim testsuite

# Copyright (C) Marko Mlinar, 2001
# Copyright (C) Embecosm Limited, 2010

# Contributor Jeremy Bennett <jeremy.bennett@embecosm.com>

# To add new test, edit between marked areas only

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


# C code for programs used only in testing is in 'test-code'. Other subdirs
# are only there to ensure DejaGNU code is distributed.
SUBDIRS = config         \
	  lib            \
	  libsim.tests   \
	  or1ksim.tests  \
	  test-code      \
	  test-code-or1k

EXTRA_DIST = global-conf.exp

# Setup for DejaGNU
AUTOMAKE_OPTIONS    = dejagnu

# Without the "--status" flag, TCL syntax or runtime errors yield a zero exit
# code, which is then taken as an indication of success (!).
AM_RUNTESTFLAGS = --status

export DEJAGNU

DEJATOOL            = libsim  \
		      or1ksim

# Clean up the junk left by the tests
clean-local:
	@$(RM) eth0.tx eth0.rx
	@$(RM) kbdtest.rx
	@$(RM) primary????.bmp
