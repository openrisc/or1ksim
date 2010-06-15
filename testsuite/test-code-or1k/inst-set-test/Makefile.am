# Makefile.am for or1ksim instruction set test programs:

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


# Tests of the instruction set. Broken out into separate tests, to avoid them
# getting too large. The original instruction set test is still here, but not
# built by default.
EXTRA_DIST = inst-set-test.ld  

if BUILD_ALL_TESTS
INST_SET_TEST_OLD = inst-set-test-old
else
INST_SET_TEST_OLD = 
endif

# Support library for use when testing the instruction set.
check_LTLIBRARIES           = libinst-set-test.la

libinst_set_test_la_SOURCES = inst-set-test.S

# The test programs
check_PROGRAMS = is-add-test          \
		 is-and-test          \
		 is-div-test          \
		 is-find-test         \
		 is-jump-test         \
		 is-lws-test          \
		 is-mac-test          \
		 is-mul-test          \
		 is-or-test           \
		 is-ror-test          \
		 is-shift-test        \
		 is-spr-test          \
		 is-sub-test          \
		 is-xor-test          \
	         $(INST_SET_TEST_OLD)

# The new instruction set tests.
is_add_test_SOURCES   = inst-set-test.h \
	                is-add-test.S
is_add_test_LDFLAGS   = -T$(srcdir)/inst-set-test.ld
is_add_test_LDADD     = inst-set-test.lo

is_and_test_SOURCES   = inst-set-test.h \
	                is-and-test.S
is_and_test_LDFLAGS   = -T$(srcdir)/inst-set-test.ld
is_and_test_LDADD     = inst-set-test.lo

is_div_test_SOURCES   = inst-set-test.h \
	                is-div-test.S
is_div_test_LDFLAGS   = -T$(srcdir)/inst-set-test.ld
is_div_test_LDADD     = inst-set-test.lo

is_find_test_SOURCES  = inst-set-test.h \
	                is-find-test.S
is_find_test_LDFLAGS  = -T$(srcdir)/inst-set-test.ld
is_find_test_LDADD    = inst-set-test.lo

is_jump_test_SOURCES  = inst-set-test.h \
	                is-jump-test.S
is_jump_test_LDFLAGS  = -T$(srcdir)/inst-set-test.ld
is_jump_test_LDADD    = inst-set-test.lo

is_lws_test_SOURCES   = inst-set-test.h \
	                is-lws-test.S
is_lws_test_LDFLAGS   = -T$(srcdir)/inst-set-test.ld
is_lws_test_LDADD     = inst-set-test.lo

is_mac_test_SOURCES   = inst-set-test.h \
	                is-mac-test.S
is_mac_test_LDFLAGS   = -T$(srcdir)/inst-set-test.ld
is_mac_test_LDADD     = inst-set-test.lo

is_mul_test_SOURCES   = inst-set-test.h \
	                is-mul-test.S
is_mul_test_LDFLAGS   = -T$(srcdir)/inst-set-test.ld
is_mul_test_LDADD     = inst-set-test.lo

is_or_test_SOURCES    = inst-set-test.h \
	                is-or-test.S
is_or_test_LDFLAGS    = -T$(srcdir)/inst-set-test.ld
is_or_test_LDADD      = inst-set-test.lo

is_ror_test_SOURCES   = inst-set-test.h \
	                is-ror-test.S
is_ror_test_LDFLAGS   = -T$(srcdir)/inst-set-test.ld
is_ror_test_LDADD     = inst-set-test.lo

is_shift_test_SOURCES = inst-set-test.h \
	                 is-shift-test.S
is_shift_test_LDFLAGS = -T$(srcdir)/inst-set-test.ld
is_shift_test_LDADD   = inst-set-test.lo

is_spr_test_SOURCES   = inst-set-test.h \
	                is-spr-test.S
is_spr_test_LDFLAGS   = -T$(srcdir)/inst-set-test.ld
is_spr_test_LDADD     = inst-set-test.lo

is_sub_test_SOURCES   = inst-set-test.h \
	                is-sub-test.S
is_sub_test_LDFLAGS   = -T$(srcdir)/inst-set-test.ld
is_sub_test_LDADD     = inst-set-test.lo

is_xor_test_SOURCES   = inst-set-test.h \
	                is-xor-test.S
is_xor_test_LDFLAGS   = -T$(srcdir)/inst-set-test.ld
is_xor_test_LDADD     = inst-set-test.lo

# The old test which builds with warnings and runs with errors
inst_set_test_old_SOURCES = inst-set-test-old.c

inst_set_test_old_LDFLAGS = -T$(srcdir)/../default.ld

inst_set_test_old_LDADD   = ../except/except.lo      \
	                    ../support/libsupport.la
