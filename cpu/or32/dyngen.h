/* dyngen.h -- Definitions for dyngen.c
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


struct reloc {
  unsigned int func_offset;
  unsigned int addend;
  int type;
  const char *name;
};

struct bff {
  void *(*open_obj)(const char *object); /* Open the object file */
  void (*close_obj)(void *);
  char *(*get_func_name)(void *, unsigned int func); /* Gets the name of func */
  void *(*get_func_start)(void *, unsigned int func);
  unsigned int (*get_func_len)(void *, unsigned int func);
  int (*get_func_reloc)(void *, unsigned int func, unsigned int relocn, struct reloc *reloc);
};

extern const struct bff bffs;

struct archf {
  unsigned int (*get_real_func_len)(void *func, unsigned int len, char *name);
  void (*gen_reloc)(FILE *f, struct reloc *reloc, unsigned int param);
  void (*gen_func_reloc)(FILE *f, struct reloc *reloc);
};

extern const struct archf archfs;
