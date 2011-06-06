# Makefile.am for or1ksim testsuite test-code-or1k

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


# Local macros
ACLOCAL_AMFLAGS = -I m4

# Subdirs for each test program. Support, exception and common machine code
# handling libraries must be first.

# The following test will not build without warning, so its Makefile does
# nothing, unless the --enable-all-tests flag is used with configure.
# - inst-set-test

EXTRA_DIST = default.ld   \
	     except-mc.ld

SUBDIRS = support       \
	  except        \
	  mc-common     \
	  acv-gpio      \
	  acv-uart      \
	  basic         \
	  cache         \
	  cbasic        \
	  cfg           \
	  dhry          \
	  dmatest       \
	  eth           \
	  except-test   \
	  exit          \
	  ext           \
	  fbtest        \
	  fp            \
	  testfloat	\
	  functest      \
	  flag          \
	  inst-set-test \
	  int-logger    \
	  int-test      \
	  kbdtest       \
	  local-global  \
	  loop          \
	  mc-async      \
	  mc-dram       \
	  mc-ssram      \
	  mc-sync       \
	  mem-test      \
	  mmu           \
	  mul           \
	  mycompress    \
	  tick          \
	  uos           \
	  upcalls       \
	  pcu

