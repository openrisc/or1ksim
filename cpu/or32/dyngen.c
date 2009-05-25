/* dyngen.c -- Generates micro operation generating functions
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
#include <string.h>
#include <stdlib.h>

#include "dyngen.h"

#define OP_FUNC_PREFIX "op_"
#define OP_FUNC_PARAM_PREFIX "__op_param"
/* Have to add to to make sure that the param[] is 0 */
#define MAX_PARAMS (3 + 1)

static const char *c_file_head =
"#include \"config.h\"\n"
"\n"
"#include <inttypes.h>\n"
"\n"
"#include \"arch.h\"\n"
"#include \"opcode/or32.h\"\n"
"#include \"spr-defs.h\"\n"
"#include \"i386-regs.h\"\n"
"#include \"abstract.h\"\n"
"\n"
"#include \"dyn-rec.h\"\n"
"#include \"%s\"\n"
"\n";

static const char *gen_code_proto =
"void gen_code(struct op_queue *opq, struct dyn_page *dp);\n"
"void patch_relocs(struct op_queue *opq, void *host_page);\n"
"\n";

static const char *c_sw_file_head =
"#include <stdlib.h>\n"
"#include <string.h>\n"
"\n"
"#include \"config.h\"\n"
"\n"
"#include <inttypes.h>\n"
"\n"
"#include \"arch.h\"\n"
"#include \"opcode/or32.h\"\n"
"#include \"spr-defs.h\"\n"
"#include \"i386-regs.h\"\n"
"#include \"abstract.h\"\n"
"#include \"dyn-rec.h\"\n"
"#include \"%s\"\n"
"\n"
"void gen_code(struct op_queue *opq, struct dyn_page *dp)\n"
"{\n"
"  unsigned int *ops, i;\n"
"  unsigned int host_len = dp->host_len;\n"
"  void *host_cur = dp->host_page;\n"
"  oraddr_t pc = dp->or_page;\n"
"  void **loc = dp->locs;\n"
"\n"
"  while(opq) {\n"
"    if(opq->next)\n"
"      *loc++ = (void *)(host_cur - dp->host_page);"
"\n"
"    for(i = 0, ops = opq->ops; i < opq->num_ops; i++, ops++) {\n"
"      switch(*ops) {\n"
"      case op_mark_loc_indx:\n"
"        opq->ops_param[0] = host_cur - dp->host_page;\n"
"        break;\n";

static const char *c_sw_file_tail =
"      }\n"
"    }\n"
"    opq = opq->next;\n"
"    pc += 4;\n"
"  }\n"
"\n"
"  dp->host_len = host_cur - dp->host_page;\n"
"  dp->host_page = realloc(dp->host_page, dp->host_len);\n"
"}\n";

static const char *c_rel_file_head =
"#include <stdio.h> /* To get printf... */\n"
"#include <stdlib.h>\n"
"\n"
"#include \"config.h\"\n"
"\n"
"#include <inttypes.h>\n"
"\n"
"#include \"arch.h\"\n"
"#include \"spr-defs.h\"\n"
"#include \"i386-regs.h\"\n"
"#include \"opcode/or32.h\"\n"
"#include \"abstract.h\"\n"
"#include \"tick.h\"\n"
"#include \"execute.h\"\n"
"#include \"sprs.h\"\n"
"#include \"dyn-rec.h\"\n"
"#include \"op-support.h\"\n"
"#include \"%s\"\n"
"\n"
"void do_scheduler(void); /* FIXME: Remove */\n"
"void do_sched_wrap(void); /* FIXME: Remove */\n"
"void do_sched_wrap_delay(void); /* FIXME: Remove */\n"
"void simprintf(oraddr_t stackaddr, unsigned long regparam); /* FIXME: Remove */\n"
"\n"
"void patch_relocs(struct op_queue *opq, void *host_page)\n"
"{\n"
"  unsigned int *ops, *ops_param, i;\n"
"\n"
"  while(opq) {\n"
"    for(i = 0, ops = opq->ops, ops_param = opq->ops_param; i < opq->num_ops; i++, ops++) {\n"
"      switch(*ops) {\n";

static const char *c_rel_file_tail =
"      }\n"
"    }\n"
"    opq = opq->next;\n"
"  }\n"
"}\n";

static void gen_func_proto(FILE *f, const char *name, int *params)
{
  int i;

  fprintf(f, "void gen_%s(struct op_queue *opq, int end", name);
  for(i = 0; (i < MAX_PARAMS) && params[i]; i++) {
    fprintf(f, ", uorreg_t param%i", i + 1);
  }
  fprintf(f, ")");
}

int main(int argc, char **argv)
{
  void *obj;
  int i, j;
  unsigned int len;
  char *name;
  int params[MAX_PARAMS];
  struct reloc reloc;

  FILE *c_file;
  FILE *h_file;
  FILE *c_sw_file;
  FILE *c_rel_file;

  if(argc != 6) {
    fprintf(stderr, "Usage: %s <object file> <c file> <c file with gen_code()> <c file with patch_reloc()> <h file>\n", argv[0]);
    return 1;
  }

  obj = bffs.open_obj(argv[1]);
  if(!obj) {
    fprintf(stderr, "Unable to open object file %s\n", argv[1]);
    return 1;
  }

  if(!(c_file = fopen(argv[2], "w"))) {
    fprintf(stderr, "Unable to open %s for writting\n", argv[2]);
    bffs.close_obj(obj);
    return 1;
  }

  if(!(c_sw_file = fopen(argv[3], "w"))) {
    fprintf(stderr, "Unable to open %s for writting\n", argv[2]);
    fclose(c_file);
    bffs.close_obj(obj);
  }

  if(!(c_rel_file = fopen(argv[4], "w"))) {
    fprintf(stderr, "Unable to open %s for writting\n", argv[3]);
    fclose(c_file);
    fclose(c_sw_file);
    bffs.close_obj(obj);
    return 1;
  }

  if(!(h_file = fopen(argv[5], "w"))) {
    fprintf(stderr, "Unable to open %s for writting\n", argv[3]);
    fclose(c_file);
    fclose(c_sw_file);
    fclose(c_rel_file);
    bffs.close_obj(obj);
    return 1;
  }

  fprintf(c_file, c_file_head, argv[5]);
  fprintf(c_sw_file, c_sw_file_head, argv[5]);
  fprintf(c_rel_file, c_rel_file_head, argv[5]);
  fprintf(h_file, "%s", gen_code_proto);

  /* Itterate through the functions in the object file */
  for(i = 0; (name = bffs.get_func_name(obj, i)); i++) {
    if(strncmp(name, OP_FUNC_PREFIX, strlen(OP_FUNC_PREFIX)))
      continue;

    /* Find all the relocations associated with this function */
    memset(params, 0, sizeof(params));
    for(j = 0; bffs.get_func_reloc(obj, i, j, &reloc); j++) {
      //fprintf(stderr, "%s %i %i\n", reloc.name, reloc.func_offset, reloc.addend);
      if(strncmp(reloc.name, OP_FUNC_PARAM_PREFIX, strlen(OP_FUNC_PARAM_PREFIX))) {
        continue;
      }
      params[atoi(reloc.name + strlen(OP_FUNC_PARAM_PREFIX)) - 1] = 1;
    }

    len = archfs.get_real_func_len(bffs.get_func_start(obj, i),
                                   bffs.get_func_len(obj, i), name);

    gen_func_proto(h_file, name, params);
    fprintf(h_file, ";\n");

    if(len) {
      /* Prototype the operation */
      fprintf(h_file, "void %s(void);\n", name);
      fprintf(h_file, "#define %s_indx %i\n", name, i);
    }

    gen_func_proto(c_file, name, params);
    fprintf(c_file, "\n{\n");
    if(len) {
      fprintf(c_file, "  add_to_opq(opq, end, %s_indx);\n", name);

      for(j = 0; params[j]; j++)
        fprintf(c_file, "  add_to_op_params(opq, end, param%i);\n", j + 1);
    }
    fprintf(c_file, "}\n\n");

    if(!len) {
      /* If the operation is of length 0, then just ignore it */
      fprintf(stderr, "WARNING: Useless operation (size == 0): %s\n", name);
      continue;
    }

    fprintf(c_sw_file, "      case %s_indx:\n", name);
    fprintf(c_sw_file, "        host_cur = enough_host_page(dp, host_cur, &host_len, %i);\n", len);
    fprintf(c_sw_file, "        memcpy(host_cur, %s, %i);\n", name, len);

    fprintf(c_rel_file, "      case %s_indx:\n", name);
    for(j = 0; bffs.get_func_reloc(obj, i, j, &reloc); j++) {
      /* Ignore nameless relocations, they appear to be absolute */
      if(!*reloc.name)
        continue;
      if(strncmp(reloc.name, OP_FUNC_PARAM_PREFIX, strlen(OP_FUNC_PARAM_PREFIX))) {
        /* We have to manually do the relocations when the relocation is
         * relative to the PC as the code gets copied to a different location
         * during recompilation, where the relative addresses shall be waay out.
         */
        archfs.gen_func_reloc(c_rel_file, &reloc);
      } else {
        archfs.gen_reloc(c_rel_file, &reloc,
                         atoi(reloc.name + strlen(OP_FUNC_PARAM_PREFIX)));
      }
    }

    fprintf(c_sw_file, "        host_cur += %i;\n        break;\n\n", len);
    fprintf(c_rel_file, "        host_page += %i;\n", len);
    /* Count how many parameters where used */
    for(j = 0; params[j]; j++);
    fprintf(c_rel_file, "        ops_param += %i;\n        break;\n\n", j);
  }

  fprintf(c_sw_file, "%s", c_sw_file_tail);
  fprintf(c_rel_file, "%s", c_rel_file_tail);

  fprintf(h_file, "\n#define op_mark_loc_indx %i\n", i);

  fclose(h_file);
  fclose(c_file);
  fclose(c_sw_file);
  fclose(c_rel_file);
  bffs.close_obj(obj);
  return 0;
}
