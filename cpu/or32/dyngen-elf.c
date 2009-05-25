/* dyngen-elf.c -- Elf parser for dyngen
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
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#include <elf.h>

#include "dyngen.h"

struct elf_obj {
  Elf32_Ehdr e_hdr;
  Elf32_Shdr *e_shdrs;
  void **e_sections;
  Elf32_Sym *e_syms; /* The symbol table in the elf file */
  unsigned int e_sym_num; /* The number of symbols */
  unsigned int e_sym_str_tab; /* The string-table associated with symbols */
  Elf32_Rel *e_rels;
  unsigned int e_rel_num; /* The number of relocations (in e_rel) */
  unsigned int e_rel_sym;
  unsigned int e_rel_sec; /* The section to modify */
  Elf32_Rela *e_relas;
  unsigned int e_rela_num; /* The number of relocations (in e_rela) */
  unsigned int e_rela_sym;
  unsigned int e_rela_sec; /* The section to modify */
};

void *elf_open_obj(const char *file)
{
  struct elf_obj *obj;
  FILE *f;
  int i;

  if(!(obj = malloc(sizeof(struct elf_obj)))) {
    fprintf(stderr, "OOM\n");
    return NULL;
  }

  if(!(f = fopen(file, "r"))) {
    free(obj);
    return NULL;
  }

  fread(&obj->e_hdr, sizeof(Elf32_Ehdr), 1, f);

  /* Do some sanity checks */
  if((obj->e_hdr.e_ident[EI_MAG0] != ELFMAG0) ||
     (obj->e_hdr.e_ident[EI_MAG1] != ELFMAG1) ||
     (obj->e_hdr.e_ident[EI_MAG2] != ELFMAG2) ||
     (obj->e_hdr.e_ident[EI_MAG3] != ELFMAG3)) {
    fprintf(stderr, "%s is not an elf file!\n", file);
    goto error_load;
  }

  if(obj->e_hdr.e_ident[EI_CLASS] == ELFCLASSNONE) {
    fprintf(stderr, "Invalid class in ELF header\n");
    goto error_load;
  }

  if(obj->e_hdr.e_ident[EI_DATA] == ELFDATANONE) {
    fprintf(stderr, "Invalid data format in ELF header\n");
    goto error_load;
  }

  /* FIXME: Swap data as necessary */

  if((obj->e_hdr.e_ident[EI_VERSION] != 1) ||
     (obj->e_hdr.e_version != 1)) {
    fprintf(stderr, "Unexpected elf version found: %i (%li)\n",
            obj->e_hdr.e_ident[EI_VERSION], (long int)obj->e_hdr.e_version);
    goto error_load;
  }

  if(obj->e_hdr.e_type != ET_REL) {
    fprintf(stderr, "Appears that we did not receive a object file\n");
    goto error_load;
  }

  if(obj->e_hdr.e_phoff) {
    fprintf(stderr, "What am I supposed to do with a program header??\n");
    goto error_load;
  }

  if(obj->e_hdr.e_ehsize != sizeof(Elf32_Ehdr)) {
    fprintf(stderr, "Unknown size of elf header\n");
    goto error_load;
  }

  if(!obj->e_hdr.e_shoff || !obj->e_hdr.e_shnum) {
    fprintf(stderr, "The elf file contains no sections!\n");
    goto error_load;
  }

  if(obj->e_hdr.e_shentsize != sizeof(Elf32_Shdr)) {
    fprintf(stderr, "Unknown section header size %i\n", obj->e_hdr.e_shentsize);
    goto error_load;
  }

  /* Load the section headers */
  if(!(obj->e_shdrs = malloc(obj->e_hdr.e_shentsize * obj->e_hdr.e_shnum))){
    fprintf(stderr, "OOM\n");
    goto error_load;
  }

  fseek(f, obj->e_hdr.e_shoff, SEEK_SET);
  fread(obj->e_shdrs, obj->e_hdr.e_shentsize, obj->e_hdr.e_shnum, f);

  /* FIXME: swap data */

  /* Load the sections */
  if(!(obj->e_sections = calloc(obj->e_hdr.e_shnum, sizeof(void *)))) {
    fprintf(stderr, "OOM\n");
    free(obj->e_shdrs);
    goto error_load;
  }

  for(i = 0; i < obj->e_hdr.e_shnum; i++) {
    if(obj->e_shdrs[i].sh_type == SHT_NOBITS)
      continue;
    if(!(obj->e_sections[i] = malloc(obj->e_shdrs[i].sh_size))) {
      fprintf(stderr, "OOM\n");
      goto post_sec_error_load;
    }
    fseek(f, obj->e_shdrs[i].sh_offset, SEEK_SET);
    fread(obj->e_sections[i], obj->e_shdrs[i].sh_size, 1, f);
  }

  obj->e_rels = NULL;
  obj->e_syms = NULL;
  obj->e_relas = NULL;

  /* Find the symbol table and relocation table(s) */
  for(i = 0; i < obj->e_hdr.e_shnum; i++) {
    switch(obj->e_shdrs[i].sh_type) {
    case SHT_SYMTAB:
      if(obj->e_syms) {
        fprintf(stderr, "ELF file has more than one symbol table\n");
        goto post_sec_error_load;
      }
      if(obj->e_shdrs[i].sh_entsize != sizeof(Elf32_Sym)) {
        fprintf(stderr, "ELF symbol table entry size is unknown\n");
        goto post_sec_error_load;
      }
      if((obj->e_shdrs[i].sh_size % obj->e_shdrs[i].sh_entsize)) {
        fprintf(stderr, "Symbol table's length is not a multiple of sizeof(Elf32_Sym\n");
        goto post_sec_error_load;
      }
      obj->e_syms = obj->e_sections[i];
      obj->e_sym_num = obj->e_shdrs[i].sh_size / obj->e_shdrs[i].sh_entsize;
      obj->e_sym_str_tab = obj->e_shdrs[i].sh_link;
      break;
    case SHT_REL:
      if(obj->e_rels) {
        fprintf(stderr, "ELF file has more than one relocation table\n");
        goto post_sec_error_load;
      }
      if(obj->e_shdrs[i].sh_entsize != sizeof(Elf32_Rel)) {
        fprintf(stderr, "ELF relocation table entry size is unknown\n");
        goto post_sec_error_load;
      }
      if((obj->e_shdrs[i].sh_size % obj->e_shdrs[i].sh_entsize)) {
        fprintf(stderr, "Relocation table's length is not a multiple of sizeof(Elf32_Rel\n");
        goto post_sec_error_load;
      }
      obj->e_rels = obj->e_sections[i];
      obj->e_rel_sec = obj->e_shdrs[i].sh_info;
      obj->e_rel_sym = obj->e_shdrs[i].sh_link;
      obj->e_rel_num = obj->e_shdrs[i].sh_size / obj->e_shdrs[i].sh_entsize;
      break;
    case SHT_RELA:
      if(obj->e_relas) {
        fprintf(stderr, "ELF file has more than one a-relocation table\n");
        goto post_sec_error_load;
      }
      if(obj->e_shdrs[i].sh_entsize != sizeof(Elf32_Rela)) {
        fprintf(stderr, "ELF a-relocation table entry size is unknown\n");
        goto post_sec_error_load;
      }
      if((obj->e_shdrs[i].sh_size % obj->e_shdrs[i].sh_entsize)) {
        fprintf(stderr, "Relocation table's length is not a multiple of sizeof(Elf32_Rela)\n");
        goto post_sec_error_load;
      }
      obj->e_relas = obj->e_sections[i];
      obj->e_rela_sec = obj->e_shdrs[i].sh_info;
      obj->e_rela_sym = obj->e_shdrs[i].sh_link;
      obj->e_rela_num = obj->e_shdrs[i].sh_size / obj->e_shdrs[i].sh_entsize;
      break;
    }
  }

  fclose(f);
  return obj;

post_sec_error_load:
  for(i = 0; i < obj->e_hdr.e_shnum; i++) {
    if(obj->e_sections[i])
      free(obj->e_sections[i]);
  }
  free(obj->e_sections);
  free(obj->e_shdrs);
error_load:
  free(obj);
  fclose(f);
  return NULL;
}

void elf_close_obj(void *e_obj)
{
  struct elf_obj *obj = e_obj;
  int i;

  for(i = 0; i < obj->e_hdr.e_shnum; i++) {
    if(obj->e_sections[i])
      free(obj->e_sections[i]);
  }
  free(obj->e_sections);
  free(obj->e_shdrs);
  free(obj);
}

static Elf32_Sym *elf_find_func(struct elf_obj *obj, unsigned int func)
{
  int i, j;
  Elf32_Sym *cur;

  for(i = 0, j = 0, cur = obj->e_syms; i < obj->e_sym_num; i++, cur++) {
    if(ELF32_ST_BIND(cur->st_info) != STB_GLOBAL)
      continue;
    if(ELF32_ST_TYPE(cur->st_info) != STT_FUNC)
      continue;
    if(j == func)
      return cur;
    j++;
  }
  return NULL;
}

char *elf_get_func_name(void *e_obj, unsigned int func)
{
  struct elf_obj *obj = e_obj;
  Elf32_Sym *func_sym = elf_find_func(obj, func);

  if(func_sym)
    return obj->e_sections[obj->e_sym_str_tab] + func_sym->st_name;

  return NULL;
}

unsigned int elf_get_func_len(void *e_obj, unsigned int func)
{
  struct elf_obj *obj = e_obj;
  Elf32_Sym *func_sym = elf_find_func(obj, func);

  if(func_sym)
    return func_sym->st_size;
  return 0;
}

void *elf_get_func_start(void *e_obj, unsigned int func)
{
  struct elf_obj *obj = e_obj;
  Elf32_Sym *func_sym = elf_find_func(obj, func);

  if(!func_sym)
    return NULL;

  if(func_sym->st_shndx == SHN_COMMON) {
    fprintf(stderr, "Don't know how to handle SHN_COMMON section header\n");
    return NULL;
  }

  return obj->e_sections[func_sym->st_shndx] + func_sym->st_value;
}

static char *elf_get_sym_name(struct elf_obj *obj, unsigned int sym)
{
  char *name;

  name = obj->e_sections[obj->e_sym_str_tab];
  name += obj->e_syms[sym].st_name;

  return name;
}

int elf_get_func_reloc(void *e_obj, unsigned int func, unsigned int relocn,
                       struct reloc *reloc)
{
  struct elf_obj *obj = e_obj;
  Elf32_Sym *func_sym = elf_find_func(obj, func);
  Elf32_Rel *cur;
  Elf32_Rela *cura;
  int i, j;

/*
  if(obj->e_rel_sec != func_sym->st_shndx) {
    fprintf(stderr, "Don't know what to do: Function does not have a relocation table\n");
    return 0;
  }
*/

  for(i = 0, j = 0, cur = obj->e_rels; i < obj->e_rel_num; i++, cur++) {
    if((cur->r_offset - func_sym->st_value) > func_sym->st_size)
      continue;
    if(relocn == j) {
      reloc->name = elf_get_sym_name(obj, ELF32_R_SYM(cur->r_info));
      reloc->func_offset = cur->r_offset - func_sym->st_value;
      reloc->type = ELF32_R_TYPE(cur->r_info);
      /* FIXME: Byte-swap */
      reloc->addend = *(uint32_t *)(obj->e_sections[obj->e_rel_sec] + cur->r_offset);
      return 1;
    }
    j++;
  }

  if(!obj->e_relas)
    return 0;

  for(i = 0, cura = obj->e_relas; i < obj->e_rela_num; i++, cura++) {
    if((cura->r_offset - func_sym->st_value) > func_sym->st_size)
      continue;
    if(relocn == j) {
      reloc->name = elf_get_sym_name(obj, ELF32_R_SYM(cur->r_info));
      reloc->func_offset = cura->r_offset - func_sym->st_value;
      reloc->type = ELF32_R_TYPE(cur->r_info);
      reloc->addend = cura->r_addend;
      return 1;
    }
    j++;
  }

  return 0;
}

const struct bff bffs = {
  elf_open_obj,
  elf_close_obj,
  elf_get_func_name,
  elf_get_func_start,
  elf_get_func_len,
  elf_get_func_reloc };
