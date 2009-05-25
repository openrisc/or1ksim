## Makefile for or1ksim subdirectory test
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

LDADD = ../support/libsupport.a
LDFLAGS = -T${top_srcdir}/default.ld

bin_PROGRAMS = uos
uos_SOURCES = except_or32.S support.h spr_defs.h task.c int.h ipc.h tick.c uos.h uos.c

again: clean all
