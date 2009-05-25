# Makefile -- Makefile for cpu architecture independent simulation
# Copyright (C) 1999 Damjan Lampret, lampret@opencores.org
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

##if OR1K_EXCEPT
##OR1K_SUPPORT_S = except.S
##else
##OR1K_SUPPORT_S =
##endif

noinst_LIBRARIES = libsupport.a
libsupport_a_SOURCES = support.c support.h int.c int.h
## EXTRA_libsupport_a_SOURCES = except.S
