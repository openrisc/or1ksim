/* dyngen-i386.c -- i386 parts of dyngen
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


#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include <elf.h>

#include "dyngen.h"

#define RET_OPCODE 0xc3

unsigned int i386_get_real_func_len(void *f_start, unsigned int f_len, char *name)
{
  if(((uint8_t *)f_start)[f_len - 1] != RET_OPCODE) {
    fprintf(stderr, "`%s' does not have a ret at the end!\n", name);
    exit(1);
  }

  return f_len - 1;
}

void i386_gen_reloc(FILE *f, struct reloc *reloc, unsigned int param)
{
  switch(reloc->type) {
  case R_386_32:
    fprintf(f, "        *(uint32_t *)(host_page + %d) = *(ops_param + %u) + %d;\n",
            reloc->func_offset, param - 1, reloc->addend);
    break;
  default:
    fprintf(stderr, "Unknown i386 relocation: %i (%s)\n", reloc->type,
            reloc->name);
  }
}

void i386_gen_func_reloc(FILE *f, struct reloc *reloc)
{
  switch(reloc->type) {
  case R_386_32:
    /* This relocation is absolute.  There is nothing to relocate (The linker
     * handles this fine). */
    break;
  case R_386_PC32:
    fprintf(f, "        *(uint32_t *)(host_page + %d) = (uint32_t)((%s + %d) - (unsigned int)(host_page + %d));\n",
            reloc->func_offset, reloc->name, reloc->addend, reloc->func_offset);
    break;
  default:
    fprintf(stderr, "Unknown i386 relocation: %i (symbol: %s)\n", reloc->type,
            reloc->name);
  }
}

const struct archf archfs = {
  i386_get_real_func_len,
  i386_gen_reloc,
  i386_gen_func_reloc
};
