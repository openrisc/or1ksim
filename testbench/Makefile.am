## Makefile for or1ksim testsuite
## (c) Marko Mlinar, 2001
## To add new test, edit between marked areas only
#
# This file is part of OpenRISC 1000 Architectural Simulator.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#

################### Tests #####################
# tests in this directory
OR1K_TESTS = basic cache cfg dmatest eth mmu except_test int_test flag fbtest kbdtest tick ext
IND_TESTS = exit cbasic local_global mul mycompress dhry functest mem_test
# inst_set_test
ACV_TESTS = acv_uart acv_gpio
MC_TESTS = mc_dram mc_ssram mc_async mc_sync 
# Subdirectory tests
SUB_TESTS =
OR1K_SUB_TESTS = uos
###############################################

ALL_TESTS = $(IND_TESTS) $(OR1K_TESTS) $(ACV_TESTS) $(MC_TESTS)
TESTS = $(IND_TESTS) $(OR1K_TESTS)
bin_PROGRAMS = $(ALL_TESTS)

######### Platform Independent Tests ##########
exit_SOURCES = $(OR1K_SUPPORT_S) support.h exit.c
cbasic_SOURCES = $(OR1K_SUPPORT_S) support.h cbasic.c
local_global_SOURCES = $(OR1K_SUPPORT_S) support.h local_global.c
mul_SOURCES = $(OR1K_SUPPORT_S) support.h mul.c
dhry_SOURCES = $(OR1K_SUPPORT_S) support.h dhry.h dhry.c
mycompress_SOURCES = $(OR1K_SUPPORT_S) support.h mycompress.c
functest_SOURCES = $(OR1K_SUPPORT_S) support.h functest.c
mem_test_SOURCES = $(OR1K_SUPPORT_S) support.h mem_test.c
inst_set_test_SOURCES = $(OR1K_SUPPORT_S) support.h inst_set_test.c
tick_SOURCES = $(OR1K_SUPPORT_S) support.h tick.c
###############################################

if OR1K_EXCEPT

exit_LDFLAGS = -T$(OR1K_SRCDIR)/default.ld
cbasic_LDFLAGS = -T$(OR1K_SRCDIR)/default.ld
local_global_LDFLAGS = -T$(OR1K_SRCDIR)/default.ld
mul_LDFLAGS = -T$(OR1K_SRCDIR)/default.ld
dhry_LDFLAGS = -T$(OR1K_SRCDIR)/default.ld
mycompress_LDFLAGS = -T$(OR1K_SRCDIR)/default.ld
functest_LDFLAGS = -T$(OR1K_SRCDIR)/default.ld
mem_test_LDFLAGS = -T$(OR1K_SRCDIR)/default.ld
inst_set_test_LDFLAGS = -T$(OR1K_SRCDIR)/default.ld
tick_LDFLAGS = -T$(OR1K_SRCDIR)/default.ld

######### MC Tests ############################
mc_dram_SOURCES = except_mc.S mc_common.h mc_common.c mc_dram.h mc_dram.c
mc_dram_LDFLAGS = -T$(OR1K_SRCDIR)except_mc.ld
mc_ssram_SOURCES = except_mc.S mc_common.h mc_common.c mc_ssramh mc_ssram.c
mc_ssram_LDFLAGS = -T$(OR1K_SRCDIR)except_mc.ld 
mc_async_SOURCES = except_mc.S mc_common.h mc_common.c mc_async.h mc_async.c
mc_async_LDFLAGS = -T$(OR1K_SRCDIR)except_mc.ld
mc_sync_SOURCES = except_mc.S mc_common.h mc_common.c mc_sync.h mc_sync.c
mc_sync_LDFLAGS = -T$(OR1K_SRCDIR)except_mc.ld
###############################################

####### Platform Dependent Tests - OR1K ########
basic_SOURCES = basic.S spr_defs.h
basic_LDFLAGS = -T$(OR1K_SRCDIR)/default.ld
basic_LDADD =
flag_SOURCES = flag.S spr_defs.h
flag_LDFLAGS = -T$(OR1K_SRCDIR)/default.ld
flag_LDADD =
cache_SOURCES = support.h cache.c cache_asm.S
cache_LDFLAGS = -T$(OR1K_SRCDIR)/cache.ld
cfg_SOURCES = cfg.S spr_defs.h
cfg_LDFLAGS = -T$(OR1K_SRCDIR)/default.ld
cfg_LDADD = 
dmatest_SOURCES = $(OR1K_SUPPORT_S) support.h dmatest.c
dmatest_LDFLAGS = -T$(OR1K_SRCDIR)/default.ld
eth_SOURCES = $(OR1K_SUPPORT_S) support.h eth.c
eth_LDFLAGS = -T$(OR1K_SRCDIR)/default.ld
acv_uart_SOURCES = $(OR1K_SUPPORT_S) support.h acv_uart.c
acv_uart_LDFLAGS = -T$(OR1K_SRCDIR)/default.ld
acv_gpio_SOURCES = $(OR1K_SUPPORT_S) support.h acv_gpio.c
acv_gpio_LDFLAGS = -T$(OR1K_SRCDIR)/default.ld
mmu_SOURCES = $(OR1K_SUPPORT_S) support.h mmu.c mmu_asm.S
mmu_LDFLAGS = -T$(OR1K_SRCDIR)/default.ld
except_test_SOURCES = except_test_s.S except_test.c spr_defs.h
except_test_LDFLAGS = -T$(OR1K_SRCDIR)/default.ld
int_test_SOURCES = spr_defs.h int_test.S
int_test_LDFLAGS = -T$(OR1K_SRCDIR)/int_test.ld
fbtest_SOURCES = $(OR1K_SUPPORT_S) support.h fbtest.c
fbtest_LDFLAGS = -T$(OR1K_SRCDIR)/default.ld
kbdtest_SOURCES = $(OR1K_SUPPORT_S) support.h kbdtest.c
kbdtest_LDFLAGS = -T$(OR1K_SRCDIR)/default.ld
ext_SOURCES = ext.S
ext_LDFLAGS = -T$(OR1K_SRCDIR)/default.ld
################################################

else

##### Platform Dependent Tests - not OR1K #####
basic_SOURCES =
flag_SOURCES =
cache_SOURCES =
cfg_SOURCES =
dmatest_SOURCES =
eth_SOURCES =
acv_uart_SOURCES =
acv_gpio_SOURCES =
mmu_SOURCES =
except_test_SOURCES =
int_test_SOURCES =
fbtest_SOURCES =
kbdtest_SOURCES =
###############################################

endif

## Neccessary stuff

if OR1K_EXCEPT
SUBDIRS = support $(SUB_TESTS) $(OR1K_SUB_TESTS)
else
SUBDIRS = support $(SUB_TESTS)
endif

if OR1K_EXCEPT
OR1K_SUPPORT_S = except.S
else
OR1K_SUPPORT_S =
endif

LDADD = support/libsupport.a
LDFLAGS = 
TESTS_ENVIRONMENT = $(SHELL) ${top_srcdir}/test $(TESTS_ENV)

again: clean all
