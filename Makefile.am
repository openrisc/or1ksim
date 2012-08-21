# Makefile.am -- Top level automake configuration file
#
# Copyright (C) 1999 OpenCores
# Copyright (C) 2008 Embecosm Limited
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


# Local macros
ACLOCAL_AMFLAGS = -I m4

DISTCHECK_CONFIGURE_FLAGS = --target=or32-elf

EXTRA_DIST = sim.cfg

SUBDIRS               = argtable2  \
			cpu        \
                        bpb        \
			doc        \
                        support    \
                        cache      \
                        mmu        \
                        peripheral \
                        tick       \
                        pm         \
                        pic        \
                        debug      \
			pcu	   \
                        vapi       \
                        cuc        \
                        port       \
			softfloat  \
                        testsuite

# Build a standalone simulator and a linkable library of the simulator

bin_PROGRAMS          = sim profile mprofile
lib_LTLIBRARIES       = libsim.la
include_HEADERS       = or1ksim.h

# The internal library

noinst_LTLIBRARIES    = libor1ksys.la

libor1ksys_la_SOURCES = cpu-config.c       \
                        mprofiler.c        \
                        profiler.c         \
                        sim-cmd.c          \
                        sim-config.c       \
                        toplevel-support.c \
                        or1ksim.h          \
                        cpu-config.h       \
                        sim-cmd.h          \
                        sim-config.h       \
                        toplevel-support.h

libor1ksys_la_LIBADD  = argtable2/libargtable2.la          \
			cpu/common/libcommon.la            \
                        cpu/$(CPU_ARCH)/libarch.la         \
                        cpu/or1k/libor1k.la                \
                        support/libsupport.la              \
                        mmu/libmmu.la                      \
                        bpb/libbpb.la                      \
                        cache/libcache.la                  \
                        peripheral/libperipheral.la        \
                        peripheral/channels/libchannels.la \
                        tick/libtick.la                    \
                        pm/libpm.la                        \
                        pic/libpic.la                      \
                        debug/libdebug.la                  \
			pcu/libpcu.la                      \
                        vapi/libvapi.la                    \
                        cuc/libcuc.la                      \
			softfloat/libsoftfloat.la	   \
                        port/libport.la

libor1ksys_la_LDFLAGS = -lm

# Standalone simulator

sim_SOURCES 	      = toplevel.c         \
	                or1ksim.h          \
                        toplevel-support.h

sim_LDADD   	      = libor1ksys.la

# Standalone profiling simulator

profile_SOURCES       = toplevel-profile.c    \
	                or1ksim.h          \
                        profiler.h

profile_LDADD         = libor1ksys.la

# Standalone memory profiling simulator

mprofile_SOURCES      = toplevel-mprofile.c   \
                        mprofiler.h        \
	                or1ksim.h

mprofile_LDADD        = libor1ksys.la

# Library

libsim_la_SOURCES     = libtoplevel.c      \
			or1ksim.h          \
                        toplevel-support.h

libsim_la_LIBADD      = libor1ksys.la
