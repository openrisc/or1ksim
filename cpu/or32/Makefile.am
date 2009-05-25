# Makefile -- Makefile for OR32 dependent simulation
#
# Copyright (C) 1999 Damjan Lampret, lampret@opencores.org
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


if DYNAMIC_EXECUTION

noinst_LTLIBRARIES = libarch.la

noinst_PROGRAMS    = dyngen

libarch_la_SOURCES = execute.c         \
		     or32.c            \
		     op.c              \
		     dyn-rec.c         \
		     op-support.c      \
		     common-i386.h     \
		     dyn32-defs.h      \
		     dyngen.h          \
		     dyn-rec.h         \
	             i386-regs.h       \
		     op-arith-op.h     \
		     op-comp-op.h      \
		     op-extend-op.h    \
		     op-ff1-op.h       \
		     op-i386.h         \
		     op-lwhb-op.h      \
		     op-mac-op.h       \
		     op-mftspr-op.h    \
		     op-support.h      \
		     op-swhb-op.h      \
		     op-t-reg-mov-op.h \
		     rec-i386.h        \
		     sched-i386.h      \
		     simpl32-defs.h 

nodist_libarch_la_SOURCES = gen-ops.h     \
			    gen-ops.c     \
			    gen-ops-gen.c \
			    gen-ops-rel.c

BUILT_SOURCES      = gen-ops.h     \
		     gen-ops.c     \
		     gen-ops-gen.c \
		     gen-ops-rel.c

dyngen_SOURCES     = dyngen.c          \
		     dyngen-i386.c     \
		     dyngen-elf.c      \
		     common-i386.h     \
		     dyn32-defs.h      \
		     dyngen.h          \
		     dyn-rec.h         \
		     i386-regs.h       \
		     op-arith-op.h     \
		     op-comp-op.h      \
		     op-extend-op.h    \
		     op-ff1-op.h       \
		     op-i386.h         \
		     op-lwhb-op.h      \
		     op-mac-op.h       \
		     op-mftspr-op.h    \
		     op-support.h      \
		     op-swhb-op.h      \
		     op-t-reg-mov-op.h \
		     rec-i386.h        \
		     sched-i386.h      \
		     simpl32-defs.h 
dyngen_CFLAGS      = $(AM_CFLAGS)

gen-ops.h gen-ops.c gen-ops-gen.c gen-ops-rel.c: dyngen$(EXEEXT) op-tmp.o
	./dyngen$(EXEEXT) op-tmp.o gen-ops.c gen-ops-gen.c gen-ops-rel.c gen-ops.h

OP_CFLAGS          = -Wall -fomit-frame-pointer -fno-reorder-blocks -O2

op-tmp.o: op.c op-t-reg-mov-op.h op-i386.h op-arith-op.h op-comp-op.h \
      op-extend-op.h op-ff1-op.h op-mac-op.h op-mftspr-op.h op-lwhb-op.h \
      op-swhb-op.h
	$(CC) $(INCLUDES)  -I$(top_builddir) $(OP_CFLAGS) -o $@ -c $<
else

noinst_LTLIBRARIES = libarch.la

if GENERATE_NEEDED

EXTRA_DIST         = insnset.c

libarch_la_SOURCES = execute.c         \
                     or32.c            \
		     common-i386.h     \
		     dyn32-defs.h      \
		     dyngen.h          \
		     dyn-rec.h         \
		     i386-regs.h       \
		     op-arith-op.h     \
		     op-comp-op.h      \
		     op-extend-op.h    \
		     op-ff1-op.h       \
		     op-i386.h         \
		     op-lwhb-op.h      \
		     op-mac-op.h       \
		     op-mftspr-op.h    \
		     op-support.h      \
		     op-swhb-op.h      \
		     op-t-reg-mov-op.h \
		     rec-i386.h        \
		     sched-i386.h      \
		     simpl32-defs.h 

BUILT_SOURCES      = execgen.c

noinst_PROGRAMS    = generate$(EXEEXT)

generate_SOURCES   = or32.c            \
                     generate.c        \
		     common-i386.h     \
                     dyn32-defs.h      \
                     dyngen.h          \
                     dyn-rec.h         \
		     i386-regs.h       \
                     op-arith-op.h     \
                     op-comp-op.h      \
		     op-extend-op.h    \
                     op-ff1-op.h       \
                     op-i386.h         \
                     op-lwhb-op.h      \
		     op-mac-op.h       \
                     op-mftspr-op.h    \
                     op-support.h      \
                     op-swhb-op.h      \
		     op-t-reg-mov-op.h \
                     rec-i386.h        \
                     sched-i386.h      \
                     simpl32-defs.h 

generate_CFLAGS    = $(AM_CFLAGS)

execgen.c: generate$(EXEEXT) insnset.c
	./generate$(EXEEXT) $(srcdir)/insnset.c execgen.c

else

libarch_la_SOURCES = execute.c         \
                     or32.c            \
		     common-i386.h     \
		     dyn32-defs.h      \
		     dyngen.h          \
		     dyn-rec.h         \
		     i386-regs.h       \
		     op-arith-op.h     \
		     op-comp-op.h      \
		     op-extend-op.h    \
		     op-ff1-op.h       \
		     op-i386.h         \
		     op-lwhb-op.h      \
		     op-mac-op.h       \
		     op-mftspr-op.h    \
		     op-support.h      \
		     op-swhb-op.h      \
		     op-t-reg-mov-op.h \
		     rec-i386.h        \
		     sched-i386.h      \
		     simpl32-defs.h 

dist_EXTRA_libarch_la_SOURCES = insnset.c

endif
endif

# If the simulator was first built without --enable-simple and then with it,
# then also remove these two files

CLEANFILES = execgen.c \
             generate \
             gen-ops.c \
             gen-ops.h \
             gen-ops-rel.c \
             gen-ops-gen.c
