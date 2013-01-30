# configure.ac -- Top level autoconf configuration file
#
# Copyright (C) 1999-2008 OpenCores
# Copyright (C) 2008, 2010, 2011 Embecosm Limited
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


# Configure script for the OpenRISC 1000 simulator 

# Process this file with autoconf to produce a configure script.


# Use a full version number (x.y.z, possibly with "rcn" as a suffix) when
# preparing a release, otherwise use a the date version (yyyy-mm-dd).
AC_INIT([or1ksim], [2012-04-27], [openrisc@opencores.org])
AC_CONFIG_MACRO_DIR([m4])

# Generically use extensions such as _GNU_SOURCE if available.
AC_USE_SYSTEM_EXTENSIONS

# Check we have the execute source file present for sanity. Specify a separate
# config for the testsuite OR1K specific programs, since we'll need different
# tool chain there (the OpenRISC one, rather than the native one).
AC_CONFIG_SRCDIR([cpu/or32/execute.c])
AC_CONFIG_SUBDIRS([testsuite/test-code-or1k])

AC_CANONICAL_TARGET([])
AC_PROG_LIBTOOL
AM_INIT_AUTOMAKE
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

AC_CONFIG_HEADERS([config.h])

AC_REVISION([$Id$ using automake version] AC_ACVERSION)

# make sure we are using a recent autoconf version
AC_PREREQ(2.59)

# yuck.
case "$target_cpu" in
or1knd*)
	CPU_ARCH=or32;
	ARCH_ISA=OR32;
	AC_DEFINE(OR32_NODELAY, 0, "The OR32 has no delay slot by default")
	AC_DEFINE(OR32_TYPES, 1, "The compiler uses the OR32 ELF types");;

or1k*)
	CPU_ARCH=or32;
	ARCH_ISA=OR32;
	AC_DEFINE(OR32_TYPES, 1, "The compiler uses the OR32 ELF types");;

*)      AC_MSG_WARN(Unknown target architecture $target_cpu: OR32 ELF assumed);
	CPU_ARCH=or32;
	ARCH_ISA=OR32;
	AC_DEFINE(OR32_TYPES, 1, "The compiler uses the OR32 ELF types");;
esac

# determine endianism from target CPU name. If it has "little" in the name,
# then its litte endian, otherwise its big endian (default for OR1K)
case "$target_cpu" in
*little*)  AC_DEFINE(OR32_LITTLE_ENDIAN, 1, "The OR32 is little endian");;
       *)  AC_DEFINE(OR32_BIG_ENDIAN,    1, "The OR32 is big endian");;
esac

AC_SUBST(CPU_ARCH)

# Set the CFLAGS we want. We put the user flags last, so that if the user
# changes the optimization level, that will take precedence.
# CFLAGS="-g -Wall -Werror -O2 -D$ARCH_ISA $CFLAGS"
# 15-Aug-11: Jeremy Bennett. TODO temporarily remove -Werror to get round
#            GCC 4.6 issues.
CFLAGS="-g -Wall -O2 -D$ARCH_ISA $CFLAGS"

AM_PROG_AS

AC_PROG_CC
AM_PROG_CC_C_O

AC_PROG_MAKE_SET

AC_PROG_INSTALL
AC_CHECK_PROG(AR, ar, ar)

# Set default for ARFLAGS, since autoconf does not have a macro for it.
# This allows people to set it when running configure or make
test -n "$ARFLAGS" || ARFLAGS="cr"

AC_TYPE_SIGNAL
AC_CHECK_HEADERS(unistd.h stdlib.h stdarg.h string.h strings.h      \
		 sys/ptem.h sys/pte.h sys/stream.h sys/stropts.h sys/select.h \
		 termcap.h termios.h termio.h sys/file.h locale.h getopt.h    \
		 net/ethernet.h sys/ethernet.h malloc.h inttypes.h libintl.h  \
                 limits.h linux/if_tun.h)
AC_CHECK_FUNCS(strcasecmp select setenv putenv tcgetattr setlocale lstat)
AC_CHECK_FUNCS(grantpt unlockpt ptsname on_exit)
AC_CHECK_FUNCS(basename)
AC_FUNC_STRCOLL
AC_CHECK_DECLS([I_PUSH, rl_event_hook], [], [], \
	       [#include <readline/readline.h>])
AC_C_BIGENDIAN

# The test for strndup, strcasecmp and isblank fails on modern machines. I
# think it's because GCC 4 does not like the way autoconf overrides the built
# in type declaration. So we must check for them by steam.
AC_MSG_CHECKING(for strndup)
AC_LINK_IFELSE([AC_LANG_PROGRAM([[#ifdef HAVE_STRING_H
				  #include <string.h>
				  #else
				  char *strndup (const char *s,
				                 size_t      n);
				  #endif]],                      \
			        [[const char *s = "test";
				  (void) strndup (s, 3);]])], \
	       [AC_DEFINE(HAVE_STRNDUP, 1, \
 	       	          "Whether the strndup function is available") \
	        AC_MSG_RESULT(yes)], [AC_MSG_RESULT(no)])

AC_MSG_CHECKING(for strcasecmp)
AC_LINK_IFELSE([AC_LANG_PROGRAM([[#ifdef HAVE_STRINGS_H 
			          #include <strings.h>
				  #else
				  int strcasecmp (const char *s1,
				                  const char *s2);
			          #endif]],                       \
			        [[const char *s = "test";
				  const char *t = "TEST";
				  (void) strcasecmp (s, t);]])], \
	       [AC_DEFINE(HAVE_STRCASECMP, 1, \
 	       	          "Whether the strcasecmp function is available") \
	        AC_MSG_RESULT(yes)], [AC_MSG_RESULT(no)])

AC_MSG_CHECKING(for isblank)
AC_LINK_IFELSE([AC_LANG_PROGRAM([[#ifdef HAVE_CTYPE_H 
			          #include <ctype.h>
				  #else
				  int isblank (int  c);
			          #endif]],               \
			        [[return isblank ('x');]])], \
	       [AC_DEFINE(HAVE_ISBLANK, 1, \
 	       	          "Whether the isblank function is available") \
	        AC_MSG_RESULT(yes)], [AC_MSG_RESULT(no)])

# Checks for typedefs, structures, and compiler characteristics (for argtable2)
AC_TYPE_SIZE_T
AC_STRUCT_TM

# Checks for library functions (for argtable2).
AC_FUNC_MALLOC
AC_FUNC_STRFTIME
AC_FUNC_STRTOD
AC_CHECK_FUNCS([bzero strchr strcspn strrchr strtol])
AC_CHECK_FUNC(getopt_long, SYS_GETOPTLONG=1, SYS_GETOPTLONG=0)
AC_CHECK_FUNC(regcomp,     SYS_REGEX=1,      SYS_REGEX=0)
AC_CHECK_FUNC(strptime,    SYS_STRPTIME=1,   SYS_STRPTIME=0)

# Define automake conditionals (for argtable2)
AM_CONDITIONAL(USE_SYS_GETOPTLONG, test "$SYS_GETOPTLONG" = "1")
AM_CONDITIONAL(USE_ARGREX,         test "$SYS_REGEX" = "1")
AM_CONDITIONAL(USE_ARGDATE,        test "$SYS_STRPTIME" = "1")

# check for "long long" (added by Erez)
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[]], [[long long ll; unsigned long long ull;]])],[AC_DEFINE(CC_HAS_LONG_LONG, 1, "Whether the compiler supports 'long long'.")],[])

# check for {u,}int{8,16,32}_t in inttypes.h.
AC_CHECK_TYPES([uint8_t, uint16_t, uint32_t],,, [#include <inttypes.h>])
AC_CHECK_TYPES([int8_t, int16_t, int32_t],,, [#include <inttypes.h>])

AC_CHECK_SIZEOF(char, 1)
AC_CHECK_SIZEOF(short, 1)
AC_CHECK_SIZEOF(int, 1)
AC_CHECK_SIZEOF(long, 1)
AC_CHECK_SIZEOF(long long, 1)

#Check for compare function type for qsort (needed by some Linuxes)
AC_CHECK_TYPES([__compar_fn_t])

# check for GNU readline
AC_CHECK_LIB(readline, add_history)

# yuck
case "$host_os" in
aix*)   prefer_curses=yes ;;
esac

case "$host_cpu" in
*cray*)	LOCAL_CFLAGS=-DCRAY ;;
esac

case "$host_os" in
isc*)	LOCAL_CFLAGS=-Disc386 ;;
esac

case "$host_os" in
solaris*) LIBS="-lsocket -lnsl" ;;
esac

# define options
AC_MSG_CHECKING(whether to enable profiling)
AC_ARG_ENABLE(profiling,
    [  --enable-profiling      generate profiling code], [
    case "$enableval" in
	yes) profile="-pg" ;;
    esac
])
AC_MSG_RESULT(${enable_profiling-no})

execution="1"

INCLUDES="-I\${top_builddir}/cpu/$CPU_ARCH"
AC_MSG_CHECKING(which execution style to use)
AC_ARG_ENABLE(execution,
    [  --enable-execution Execution style to use (simple/complex)],
    [
    case "$enableval" in
    simple)
      INCLUDES="-I\${top_srcdir}/cpu/$CPU_ARCH"
      execution="0"
      AC_MSG_RESULT(simple)
      ;;
    complex)
      INCLUDES="-I\${top_builddir}/cpu/$CPU_ARCH"
      execution="1"
      AC_MSG_RESULT(complex)
      ;;
    *)
      AC_MSG_ERROR("execution must one of simple/complex")
      ;;
    esac
    ],
    [
      AC_MSG_RESULT(complex)
    ])

eth_phy="0"
AC_MSG_CHECKING(whether to enable ethernet phy emulation)
AC_ARG_ENABLE(ethphy,
    [  --enable-ethphy    compiles sim with ethernet phy support], [
    case "$enableval" in
  no)  eth_phy="0" ;;
	yes) eth_phy="1" ;;
    esac
])
AC_MSG_RESULT(${enable_eth_phy-no})

unsigned_xori="0"
AC_MSG_CHECKING(whether l.xori takes an unsigned immediate operand)
AC_ARG_ENABLE(unsigned-xori,
    [  --enable-unsigned-xori    l.xori immediate operand treated as unsigned], [
    case "$enableval" in
  no)  unsigned_xori="0" ;;
	yes) unsigned_xori="1" ;;
    esac
])
AC_MSG_RESULT(${enable_unsigned_xori-no})

raw_range_stats="0"
AC_MSG_CHECKING(whether to use raw range stats)
AC_ARG_ENABLE(range_stats,
    [  --enable-range-stats      Specifies, whether we should do register accesses over time analysis.], [
    case "$enableval" in
  no)  raw_range_stats="0" ;;
	yes) raw_range_stats="1" simple_execution="1" ;;
    esac
])
AC_MSG_RESULT(${enable_range_stats-no})

# check for --enable-debug argument (for argtable2)
AC_ARG_ENABLE(
    [debug],
    AC_HELP_STRING([--enable-debug],[enable library debugging symbols]),
    [
       case $enableval in
          yes) echo "enabling argtable2 debugging symbols" & DEBUGFLAGS="-g -UNDEBUG";;               
          no)  echo "disabling argtable2 debugging symbols" & DEBUGFLAGS="-DNDEBUG";;
          *)   echo "illegal argument to --enable-debug" & exit 1;;
       esac
    ],
    [DEBUGFLAGS="-DNDEBUG"]
    )
AC_SUBST(DEBUGFLAGS)

AC_DEFINE_UNQUOTED(RAW_RANGE_STATS, $raw_range_stats, [ Specifies, whether we should do register accesses over time analysis. Simple execution must be enabled!])
AC_DEFINE_UNQUOTED(HAVE_ETH_PHY, $eth_phy, [Whether we have ethernet PHY support])
AC_DEFINE_UNQUOTED(HAVE_UNSIGNED_XORI, $unsigned_xori, [Whether l.xori takes an unsigned immediate operand])
AC_DEFINE_UNQUOTED(SIMPLE_EXECUTION, $execution == 0, [Whether we are building with simple execution module]) 
AC_DEFINE_UNQUOTED(COMPLEX_EXECUTION, $execution == 1, [Whether we are building with complex execution module]) 
AM_CONDITIONAL(GENERATE_NEEDED, test x$execution = x1)



test -n "$profile"  && CFLAGS="$CFLAGS $profile" LDFLAGS="$LDFLAGS $profile"

# Check for a DejaGNU global config file. If not set, then set it
# ourselves. This stops runtest complaining
DEJAGNU="\$(top_srcdir)/testsuite/global-conf.exp"

AC_SUBST(DEJAGNU)

# The following line will override the default definition of the srcdir, and
# ensure that short names are used for the test names.
RUNTESTDEFAULTFLAGS="--tool \$\$tool"
AC_SUBST(RUNTESTDEFAULTFLAGS)

BUILD_DIR=`pwd`
AC_SUBST(BUILD_DIR)

AC_SUBST(CFLAGS)
AC_SUBST(LOCAL_CFLAGS)
AC_SUBST(LOCAL_LDFLAGS)
AC_SUBST(LOCAL_DEFS)

AC_SUBST(AR)
AC_SUBST(ARFLAGS)

AC_SUBST(host_cpu)
AC_SUBST(host_os)
AC_SUBST(host)

AC_SUBST(SUMVERSION)
AC_SUBST(TERMCAP_LIB)

AC_DEFINE(HAVE_EXECUTION, 1, [ Some shared files require to know, whether we have execution functions defined.])

# yuck
INCLUDES="$INCLUDES -I\${top_srcdir} -I\${top_srcdir}/cpu/common \
-I\${top_srcdir}/cpu/or1k -I\${top_srcdir}/cache -I\${top_srcdir}/mmu \
-I\${top_srcdir}/bpb -I\${top_srcdir}/peripheral -I\${top_srcdir}/tick \
-I\${top_srcdir}/peripheral/channels -I\${top_srcdir}/pm -I\${top_srcdir}/pic \
-I\${top_srcdir}/debug -I\${top_srcdir}/vapi -I\${top_srcdir}/support \
-I\${top_srcdir}/cuc -I\${top_srcdir}/port -I\${top_srcdir}/argtable2 \
-I\${top_srcdir}/softfloat -I\${top_srcdir}/pcu"
AC_SUBST(INCLUDES)

AC_CONFIG_FILES([Makefile                                 \
                 argtable2/Makefile                       \
                 bpb/Makefile                             \
                 cache/Makefile                           \
                 cpu/Makefile                             \
                 cpu/common/Makefile                      \
                 cpu/or1k/Makefile                        \
                 cuc/Makefile                             \
                 softfloat/Makefile                       \
                 debug/Makefile                           \
                 doc/Makefile                             \
                 mmu/Makefile                             \
                 peripheral/Makefile                      \
                 peripheral/channels/Makefile             \
                 pm/Makefile                              \
                 pic/Makefile                             \
                 pcu/Makefile                             \
                 port/Makefile                            \
                 support/Makefile                         \
                 testsuite/Makefile                       \
		 testsuite/config/Makefile                \
		 testsuite/lib/Makefile                   \
		 testsuite/libsim.tests/Makefile          \
		 testsuite/or1ksim.tests/Makefile         \
		 testsuite/test-code/Makefile             \
		 testsuite/test-code/lib-iftest/Makefile  \
		 testsuite/test-code/lib-inttest/Makefile \
		 testsuite/test-code/lib-jtag/Makefile    \
		 testsuite/test-code/lib-upcalls/Makefile \
                 tick/Makefile                            \
                 vapi/Makefile])

# yuck. I don't know why I cannot just substitute $CPU_ARCH in the above
case "$CPU_ARCH" in
*)	AC_CONFIG_FILES([cpu/or32/Makefile]);;
esac

# Generate the output
AC_COPYRIGHT([Copyright (C) 1999-2008 OpenCores
Copyright (C) 2008, 2010 Embecosm Limited

Contributor Jeremy Bennett <jeremy.bennett@embecosm.com>

This file is part of OpenRISC 1000 Architectural Simulator.

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the Free
Software Foundation; either version 3 of the License, or (at your option)
any later version.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
more details.

You should have received a copy of the GNU General Public License along
with this program.  If not, see <http://www.gnu.org/licenses/>.])

AC_OUTPUT
