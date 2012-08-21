/* dbchs.h -- Debug channel declarations

   Copyright (C) 2005 György `nog' Jeney, nog@sdf.lonestar.org
   Copyright (C) 2008 Embecosm Limited
  
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
   with this program.  If not, see <http://www.gnu.org/licenses/>. */

/* This program is commented throughout in a fashion suitable for processing
   with Doxygen. */

/* NOTE. Don't protect this with #ifndef. It may be included more than once. */

/* Declarations of all debug channels */

DECLARE_DEBUG_CHANNEL(sched)
DECLARE_DEBUG_CHANNEL(except)
DECLARE_DEBUG_CHANNEL(cycles)
DECLARE_DEBUG_CHANNEL(sched_jobs)
DECLARE_DEBUG_CHANNEL(spr)
DECLARE_DEBUG_CHANNEL(immu)
DECLARE_DEBUG_CHANNEL(dmmu)
DECLARE_DEBUG_CHANNEL(pic)
DECLARE_DEBUG_CHANNEL(tick)
DECLARE_DEBUG_CHANNEL(generic)		/* JPB */
DECLARE_DEBUG_CHANNEL(uart)
DECLARE_DEBUG_CHANNEL(eth)
DECLARE_DEBUG_CHANNEL(config)
DECLARE_DEBUG_CHANNEL(ata)
DECLARE_DEBUG_CHANNEL(gpio)
DECLARE_DEBUG_CHANNEL(mc)
DECLARE_DEBUG_CHANNEL(dma)
DECLARE_DEBUG_CHANNEL(vapi)
DECLARE_DEBUG_CHANNEL(jtag)
DECLARE_DEBUG_CHANNEL(coff)
