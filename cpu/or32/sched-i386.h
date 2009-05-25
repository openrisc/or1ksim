/* sched-i386.h -- i386 specific support routines for the scheduler
   Copyright (C) 2005 György `nog' Jeney, nog@sdf.lonestar.org

This file is part of OpenRISC 1000 Architectural Simulator.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA. */


/* Sets the cycle counter to a specific value */
static void set_sched_cycle(int32_t job_time)
{
  union {
    uint64_t val64;
    union {
      uint32_t low32;
      uint32_t high32;
    } val3232;
  } time_pc;

  asm("movq %%mm0, %0\n"
      "\tmovl %2, %1\n"
      "\tmovq %3, %%mm0\n"
      : "=m" (time_pc.val64),
        "=m" (time_pc.val3232.low32)
      : "r" (job_time),
        "m" (time_pc.val64));
}

