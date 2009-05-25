/* common-i386.h -- Assembler routines used in rec_i386.h and op_i386.h
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


/* This is needed because we can't move an mmx register to a general purpose
 * register. */
static union {
  struct {
    uint32_t low32;
    uint32_t high32;
  } val3232;
  uint64_t val64;
} useless_x86;

/* Sets the PC with a specified value */
static void set_pc(oraddr_t pc)
{
  /* I could just use pc as a memory argument, but if I do that then gcc may put
   * the value of pc onto the stack, in which case gcc would also shift the
   * stack twice, which would result in two add 4, %esp instructions and a
   * mov %eax, *%esp, which would not only be slow but it would take up more
   * space. */
  asm("movq %%mm0, %0\n"
      "\tmovl %2, %1\n"
      "\tmovq %3, %%mm0"
      : "=m" (useless_x86.val64),
        "=m" (useless_x86.val3232.high32)
      : "r" (pc),
        "m" (useless_x86.val64));
}

/* Returns the current value of the pc */
static oraddr_t get_pc(void)
{
  asm("movq %%mm0, %0" : "=m" (useless_x86.val64));
  return useless_x86.val3232.high32;
}

/* Updates the runtime.sim.cycles counter */
static void upd_sim_cycles(void)
{
  asm volatile ("movq %%mm0, %0\n" : "=m" (useless_x86.val64));
  runtime.sim.cycles += scheduler.job_queue->time - useless_x86.val3232.low32;
  scheduler.job_queue->time = useless_x86.val3232.low32;
  cpu_state.pc = useless_x86.val3232.high32;
}

