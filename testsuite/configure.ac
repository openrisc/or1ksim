# configure.ac -- Test suite specific autoconf configuration file
#
# Copyright (C) 2010 Embecosm Limited
#
# Contributor Jeremy Bennett <jeremy.bennett@embecosm.com>
#
# This file is part of OpenRISC 1000 Architectural Simulator.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the Free
# Software Foundation; either version 3 of the License, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
# more details.
#
# You should have received a copy of the GNU General Public License along
# with this program.  If not, see <http://www.gnu.org/licenses/>.


# Configure script for the OpenRISC 1000 simulator test suite. Process this
# file with autoconf to produce a configure script.

AC_INIT([or1ksim-testsuite],[0.4.0rc1], [openrisc@opencores.org])
AC_CONFIG_MACRO_DIR([m4])

AC_PROG_LIBTOOL
AM_INIT_AUTOMAKE
AC_CONFIG_HEADERS([config.h])

# make sure we are using a recent autoconf version
AC_PREREQ(2.59)

# check for --enable-all-tests argument, in which case we attempt to build all
# tests, even those that do not compile properly.
AC_ARG_ENABLE(
    [all-tests],
    AC_HELP_STRING([--enable-all-tests],
		   [enable all tests, including those which do not compile]),
    [all_tests=true] [all_tests=false])
AM_CONDITIONAL([BUILD_ALL_TESTS], [tests x$all_tests = xtrue])

# Sort out the tools. Using the target tool chain here, not native. Note that
# AC_CHECK_PROGS will not override an already set variable, so we must
# explicitly unset them first.
unset CC
unset RANLIB
unset LD
unset SIM
AC_CHECK_PROGS(CC, [$target_alias-gcc])
AC_CHECK_PROGS(RANLIB, [$target_alias-ranlib])
AC_CHECK_PROGS(LD, [$target_alias-ld])
AC_CHECK_PROGS(SIM, [$target_alias-sim])
AC_SUBST(CC)
AC_SUBST(RANLIB)
AC_SUBST(LD)
AC_SUBST(SIM)

AC_HEADER_STDC

# We need assembler, installer and per target C flags

AM_PROG_AS
AM_PROG_CC_C_O
AC_PROG_INSTALL

# Specify our baseline CFLAGS
CPPFLAGS="-I\${top_srcdir} -I\${top_srcdir}/test-code/support -I\${top_srcdir}/test-code/mc-common"
CFLAGS="-Wall -Werror -g -nostdlib -mhard-div -mhard-mul"

# Flag to specify whether we want optimization when checking
AC_MSG_CHECKING(whether to enable optimizations when checking)
AC_ARG_ENABLE(opt,
    [  --enable-check-opt      enable optimizations
  --enable-check-opt=level     same as gcc -O switch  ], [
    case "$enableval" in
	yes)
	    CFLAGS="$CFLAGS -O"
	    ;;
	*)
	    CFLAGS="$CFLAGS -O$enableval"
	    ;;
    esac
])
AC_MSG_RESULT(${enable_check_opt-default})

AC_SUBST(CPPFLAGS)
AC_SUBST(CFLAGS)

# This is GNU compliant source and uses GNU libraries
AC_DEFINE(_GNU_SOURCE, 1, "The source code uses the GNU libraries)

# Check for functions doesn't work, since we can't execute code. Define some
# we know are definitely there in OR1K GCC. This is all to allow reused
# headers from the main code body (for the memory controller tests
# principally) to work.
AC_DEFINE([HAVE_STRNDUP], [], [Is strndup () available])
AC_DEFINE([HAVE_ISBLANK], [], [Is isblank () available])

# Check for headers
AC_CHECK_HEADERS(sys/types.h inttypes.h)

# check for types and sizes
AC_CHECK_TYPES([uint8_t, uint16_t, uint32_t],,, [#include <inttypes.h>])
AC_CHECK_TYPES([int8_t, int16_t, int32_t],,, [#include <inttypes.h>])

# Checking sizeof doesn't really work, since it needs to run compiled code. So
# we do it by hand.
AC_DEFINE([SIZEOF_CHAR],  [1], [Result of sizeof (char)])
AC_DEFINE([SIZEOF_SHORT], [2], [Result of sizeof (short)])
AC_DEFINE([SIZEOF_INT],   [4], [Result of sizeof (int)])
AC_DEFINE([SIZEOF_LONG],  [4], [Result of sizeof (long)])

# Check for a DejaGNU global config file. If not set, then set it
# ourselves. This stops runtest complaining
if test x"$DEJAGNU" = x
then
  DEJAGNU="\$(top_srcdir)/default-global-conf.exp"
  AC_SUBST(DEJAGNU)
fi

AC_CONFIG_FILES([Makefile                         \
		 config/Makefile                  \
		 lib/Makefile                     \
		 or1ksim.tests/Makefile           \
                 test-code/Makefile               \
		 test-code/acv-gpio/Makefile      \
		 test-code/acv-uart/Makefile      \
		 test-code/basic/Makefile         \
		 test-code/cache/Makefile         \
		 test-code/cbasic/Makefile        \
		 test-code/cfg/Makefile           \
		 test-code/dhry/Makefile          \
		 test-code/dmatest/Makefile       \
		 test-code/eth/Makefile           \
		 test-code/except/Makefile        \
		 test-code/except-test/Makefile   \
		 test-code/exit/Makefile          \
		 test-code/ext/Makefile           \
		 test-code/fbtest/Makefile        \
		 test-code/functest/Makefile      \
		 test-code/flag/Makefile          \
		 test-code/int-test/Makefile      \
		 test-code/inst-set-test/Makefile \
		 test-code/kbdtest/Makefile       \
		 test-code/local-global/Makefile  \
		 test-code/mc-async/Makefile      \
		 test-code/mc-dram/Makefile       \
		 test-code/mc-ssram/Makefile      \
		 test-code/mc-sync/Makefile       \
		 test-code/mc-common/Makefile     \
		 test-code/mem-test/Makefile      \
		 test-code/mmu/Makefile           \
		 test-code/mul/Makefile           \
		 test-code/mycompress/Makefile    \
		 test-code/support/Makefile       \
		 test-code/tick/Makefile          \
		 test-code/uos/Makefile])

AC_OUTPUT
