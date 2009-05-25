/* sim-config.c -- Simulator configuration

   Copyright (C) 1999 Damjan Lampret, lampret@opencores.org
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

/* Simulator configuration. Eventually this one will be a lot bigger. Updated
   to use argtable2 for argument parsing. */


/* Autoconf and/or portability configuration */
#include "config.h"
#include "port.h"

/* System includes */
#include <stdlib.h>

/* Package includes */
#include "sim-config.h"
#include "vapi.h"
#include "cuc.h"
#include "cpu-config.h"
#include "memory.h"
#include "dmmu.h"
#include "immu.h"
#include "dcache-model.h"
#include "icache-model.h"
#include "pic.h"
#include "pm.h"
#include "branch-predict.h"
#include "debug-unit.h"
#include "mc.h"
#include "16450.h"
#include "dma.h"
#include "eth.h"
#include "gpio.h"
#include "vga.h"
#include "fb.h"
#include "ps2kbd.h"
#include "atahost.h"
#include "generic.h"
#include "execute.h"
#include "spr-defs.h"
#include "debug.h"
#include "misc.h"
#include "argtable2.h"


struct config config;
struct runtime runtime;

struct config_section *cur_section;

struct config_section *sections = NULL;

/* Forward declarations */
static void read_script_file (const char *filename);



/*---------------------------------------------------------------------------*/
/*!Set default configuration parameters for fixed components

   These values are held in the global config variable. Parameter orders
   match the order in the corresponding section registration function and
   documentation.

   Also set some starting values for runtime elements.                       */
/*---------------------------------------------------------------------------*/
void
init_defconfig ()
{
  int  set_bits;		/* Temporaries for computing bit fields */
  int  way_bits;

  memset (&config, 0, sizeof (config));

  /* External linkage disabled here. */
  config.ext.class_ptr = NULL;
  config.ext.read_up   = NULL;
  config.ext.write_up  = NULL;

  /* Sim */
  config.sim.verbose        = 0;
  config.sim.debug          = 0;
  config.sim.profile        = 0;
  config.sim.prof_fn        = strdup ("sim.profile");
  config.sim.mprofile       = 0;
  config.sim.mprof_fn       = strdup ("sim.mprofile");
  config.sim.history        = 0;
  config.sim.exe_log        = 0;
  config.sim.exe_log_type   = EXE_LOG_HARDWARE;
  config.sim.exe_log_start  = 0;
  config.sim.exe_log_end    = 0;
  config.sim.exe_log_marker = 0;
  config.sim.exe_log_fn     = strdup ("executed.log");
  config.sim.clkcycle_ps    = 4000;	/* 4000 for 4ns (250MHz) */

  /* VAPI */
  config.vapi.enabled        = 0;
  config.vapi.server_port    = 50000;
  config.vapi.log_enabled    = 0;
  config.vapi.hide_device_id = 0;
  config.vapi.vapi_fn        = strdup ("vapi.log");

  /* CUC */
  config.cuc.calling_convention = 1;
  config.cuc.enable_bursts      = 1;
  config.cuc.no_multicycle      = 1;
  config.cuc.memory_order       = MO_STRONG;
  config.cuc.timings_fn         = strdup ("virtex.tim");

  /* CPU */
  cpu_state.sprs[SPR_VR]      = 0;
  cpu_state.sprs[SPR_UPR]     = SPR_UPR_UP | SPR_UPR_TTP;
  cpu_state.sprs[SPR_SR]      = SPR_SR_FO  | SPR_SR_SM;
  cpu_state.sprs[SPR_CPUCFGR] = SPR_CPUCFGR_OB32S;
  config.cpu.superscalar      = 0;
  config.cpu.hazards          = 0;
  config.cpu.dependstats      = 0;
  config.cpu.sbuf_len         = 0;

  /* Data cache (IC is set dynamically). Also set relevant SPR bits */
  config.dc.enabled         = 0;
  config.dc.nsets           = 1;
  config.dc.nways           = 1;
  config.dc.blocksize       = 1;
  config.dc.ustates         = 2;
  config.dc.load_hitdelay   = 2;
  config.dc.load_missdelay  = 2;
  config.dc.store_hitdelay  = 0;
  config.dc.store_missdelay = 0;

  if (config.dc.enabled)
    {
      cpu_state.sprs[SPR_UPR] |= SPR_UPR_DCP;
    }
  else
    {
      cpu_state.sprs[SPR_UPR] &= ~SPR_UPR_DCP;
    }

  set_bits = log2_int (config.dc.nsets);
  cpu_state.sprs[SPR_DCCFGR] &= ~SPR_DCCFGR_NCS;
  cpu_state.sprs[SPR_DCCFGR] |= set_bits << SPR_DCCFGR_NCS_OFF;

  way_bits = log2_int (config.dc.nways);
  cpu_state.sprs[SPR_DCCFGR] &= ~SPR_DCCFGR_NCW;
  cpu_state.sprs[SPR_DCCFGR] |= way_bits << SPR_DCCFGR_NCW_OFF;

  if (MIN_DC_BLOCK_SIZE == config.dc.blocksize)
    {
      cpu_state.sprs[SPR_DCCFGR] &= ~SPR_DCCFGR_CBS;
    }
  else
    {
      cpu_state.sprs[SPR_DCCFGR] |= SPR_DCCFGR_CBS;
    }

  /* Power management */
  config.pm.enabled = 0;

  if (config.pm.enabled)
    {
      cpu_state.sprs[SPR_UPR] |= SPR_UPR_PMP;
    }
  else
    {
      cpu_state.sprs[SPR_UPR] &= ~SPR_UPR_PMP;
    }

  /* Programmable Interrupt Controller */
  config.pic.enabled      = 0;
  config.pic.edge_trigger = 1;

  if (config.pic.enabled)
    {
      cpu_state.sprs[SPR_UPR] |= SPR_UPR_PICP;
    }
  else
    {
      cpu_state.sprs[SPR_UPR] &= ~SPR_UPR_PICP;
    }

  /* Branch Prediction */
  config.bpb.enabled     = 0;
  config.bpb.btic        = 0;
  config.bpb.sbp_bnf_fwd = 0;
  config.bpb.sbp_bf_fwd  = 0;
  config.bpb.missdelay   = 0;
  config.bpb.hitdelay    = 0;
  
  /* Debug */
  config.debug.enabled     = 0;
  config.debug.gdb_enabled = 0;
  config.debug.rsp_enabled = 0;
  config.debug.server_port = 51000;
  config.debug.rsp_port    = 51000;
  config.debug.vapi_id     = 0;

  cpu_state.sprs[SPR_DCFGR] = SPR_DCFGR_WPCI |
                              MATCHPOINTS_TO_NDP (MAX_MATCHPOINTS);

  if (config.debug.enabled)
    {
      cpu_state.sprs[SPR_UPR] |= SPR_UPR_DUP;
    }
  else
    {
      cpu_state.sprs[SPR_UPR] &= ~SPR_UPR_DUP;
    }

  /* Configure runtime */
  memset (&runtime, 0, sizeof (runtime));

  /* Sim */
  runtime.sim.fexe_log              = NULL;
  runtime.sim.iprompt               = 0;
  runtime.sim.fprof                 = NULL;
  runtime.sim.fmprof                = NULL;
  runtime.sim.fout                  = stdout;

  /* NPC state. Set to 1 when NPC is changed while the processor is stalled. */
  cpu_state.npc_not_valid = 0;

  /* VAPI */
  runtime.vapi.vapi_file = NULL;
  runtime.vapi.enabled   = 0;

}	/* init_defconfig() */


/*---------------------------------------------------------------------------*/
/*! Parse the arguments for the standalone simulator

    Updated by Jeremy Bennett to use argtable2.

    @param[in] argc  Number of command args
    @param[in] argv  Vector of the command args

    @return  0 on success, 1 on failure                                      */
/*---------------------------------------------------------------------------*/
int
parse_args (int argc, char *argv[])
{
  struct arg_lit *vercop;
  struct arg_lit *help;
  struct arg_file *cfg_file;
  struct arg_lit *nosrv;
  struct arg_int *srv;
  struct arg_str *dbg;
  struct arg_lit *command;
  struct arg_lit *strict_npc;
  struct arg_lit *profile;
  struct arg_lit *mprofile;
  struct arg_file *load_file;
  struct arg_end *end;

  void *argtab[12];
  int nerrors;

  /* Specify each argument, with fall back values */
  vercop = arg_lit0 ("v", "version", "version and copyright notice");
  help = arg_lit0 ("h", "help", "print this help message");
  cfg_file = arg_file0 ("f", "file", "<file>",
			"config file (default \"sim.cfg\")");
  cfg_file->filename[0] = "sim.cfg";
  nosrv = arg_lit0 (NULL, "nosrv", "do not launch JTAG proxy server");
  srv = arg_int0 (NULL, "srv", "<n>", "port number (default random)");
  srv->ival[0] = rand () % (65536 - 49152) + 49152;
  srv->hdr.flag |= ARG_HASOPTVALUE;
  dbg = arg_str0 ("d", "debug-config", "<str>", "Debug config string");
  command = arg_lit0 ("i", "interactive", "launch interactive prompt");
  strict_npc = arg_lit0 (NULL, "strict-npc", "setting NPC flushes pipeline");
  profile = arg_lit0 (NULL, "enable-profile", "enable profiling");
  mprofile = arg_lit0 (NULL, "enable-mprofile", "enable memory profiling");
  load_file = arg_file0 (NULL, NULL, "<file>", "OR32 executable");
  end = arg_end (20);

  /* Set up the argument table */
  argtab[ 0] = vercop;
  argtab[ 1] = help;
  argtab[ 2] = cfg_file;
  argtab[ 3] = nosrv;
  argtab[ 4] = srv;
  argtab[ 5] = dbg;
  argtab[ 6] = command;
  argtab[ 7] = strict_npc;
  argtab[ 8] = profile;
  argtab[ 9] = mprofile;
  argtab[10] = load_file;
  argtab[11] = end;

  /* Parse */
  nerrors = arg_parse (argc, argv, argtab);

  /* Special case here is if help or version is specified, we ignore any other
     errors and just print the help or version information and then give up. */
  if (vercop->count > 0)
    {
      PRINTF ("OpenRISC 1000 Architectural Simulator, version %s\n",
	      PACKAGE_VERSION);

      arg_freetable (argtab, sizeof (argtab) / sizeof (argtab[0]));
      return 1;
    }

  if (help->count > 0)
    {
      printf ("Usage:\n  %s ", argv[0]);
      arg_print_syntax (stdout, argtab, "\n\n");
      arg_print_glossary (stdout, argtab, "  %-25s %s\n");

      arg_freetable (argtab, sizeof (argtab) / sizeof (argtab[0]));
      return 1;
    }

  /* Deal with any errors */
  if (0 != nerrors)
    {
      arg_print_errors (stderr, end, "or1ksim");
      printf ("\nUsage:\n  %s ", argv[0]);
      arg_print_syntaxv (stderr, argtab, "\n");

      arg_freetable (argtab, sizeof (argtab) / sizeof (argtab[0]));
      return 1;
    }

  /* Process config file next, so any other command args will override */
  if (0 == cfg_file->count)
    {
      fprintf (stderr,
	       "Warning: No config file given, default configuration used\n");
    }

  read_script_file (cfg_file->filename[0]);


  /* Remote debug server */
  if (nosrv->count > 0)
    {
      if (srv->count > 0)
	{
	  fprintf (stderr,
		   "%s: cannot specify --nosrv with --srv. Ignoring --nosrv\n",
		   argv[0]);
	}
      else
	{
	  config.debug.enabled = 0;
	  config.debug.gdb_enabled = 0;
	}
    }

  if (srv->count > 0)
    {
      config.debug.enabled = 1;
      config.debug.gdb_enabled = 1;
      config.debug.server_port = srv->ival[0];
    }

  /* Runtime debug messages */
  if (dbg->count > 0)
    {
      parse_dbchs (dbg->sval[0]);
    }

  /* Interactive operation */
  runtime.sim.iprompt = command->count;

  /* Request for strict NPC behavior (flush the pipeline on change) */
  config.sim.strict_npc = strict_npc->count;

  /* Profiling requests */
  config.sim.profile = profile->count;
  config.sim.mprofile = mprofile->count;

  /* Executable file */
  if (load_file->count > 0)
    {
      runtime.sim.filename = strdup (load_file->filename[0]);
    }
  else
    {
      runtime.sim.filename = NULL;
    }

  arg_freetable (argtab, sizeof (argtab) / sizeof (argtab[0]));
  return 0;			/* Success */

}	/* parse_args() */


/*---------------------------------------------------------------------------*/
/*!Print the current configuration                                           */
/*---------------------------------------------------------------------------*/
void
print_config ()
{
  if (config.sim.verbose)
    {
      char temp[20];
      PRINTF ("Verbose on, ");
      if (config.sim.debug)
	PRINTF ("simdebug on, ");
      else
	PRINTF ("simdebug off, ");
      if (runtime.sim.iprompt)
	PRINTF ("interactive prompt on\n");
      else
	PRINTF ("interactive prompt off\n");

      PRINTF ("Machine initialization...\n");
      generate_time_pretty (temp, config.sim.clkcycle_ps);
      PRINTF ("Clock cycle: %s\n", temp);
      if (cpu_state.sprs[SPR_UPR] & SPR_UPR_DCP)
	PRINTF ("Data cache present.\n");
      else
	PRINTF ("No data cache.\n");
      if (cpu_state.sprs[SPR_UPR] & SPR_UPR_ICP)
	PRINTF ("Insn cache tag present.\n");
      else
	PRINTF ("No instruction cache.\n");
      if (config.bpb.enabled)
	PRINTF ("BPB simulation on.\n");
      else
	PRINTF ("BPB simulation off.\n");
      if (config.bpb.btic)
	PRINTF ("BTIC simulation on.\n");
      else
	PRINTF ("BTIC simulation off.\n");
    }
}

struct config_param
{
  char *name;
  enum param_t type;
  void (*func) (union param_val, void *dat);
  struct config_param *next;
};

void
base_include (union param_val val, void *dat)
{
  read_script_file (val.str_val);
  cur_section = NULL;
}

/*---------------------------------------------[ Simulator configuration ]---*/


void
sim_verbose (union param_val val, void *dat)
{
  config.sim.verbose = val.int_val;
}


/*---------------------------------------------------------------------------*/
/*!Set the simulator debug message level

   Value must be in the range 0 (no messages) to 9. Values outside this range
   are converted to the nearer end of the range with a warning.

   @param[in] val  The value to use
   @param[in] dat  The config data structure (not used here)                 */
/*---------------------------------------------------------------------------*/
void
sim_debug (union param_val  val,
	   void            *dat)
{
  if (val.int_val < 0)
    {
      fprintf (stderr,
	       "Warning: Config debug value negative: 0 substituted\n");
      config.sim.debug = 0;
    }
  else if (val.int_val > 9)
    {
      fprintf (stderr,
	       "Warning: Config debug value too large: 9 substituted\n");
      config.sim.debug = 9;
    }
  else
    {
      config.sim.debug = val.int_val;
    }
}	/* sim_debug() */


void
sim_profile (union param_val val, void *dat)
{
  config.sim.profile = val.int_val;
}

void
sim_prof_fn (union param_val val, void *dat)
{
  if (NULL != config.sim.prof_fn)
    {
      free (config.sim.prof_fn);
    }

  config.sim.prof_fn = strdup(val.str_val);
}

void
sim_mprofile (union param_val val, void *dat)
{
  config.sim.mprofile = val.int_val;
}

void
sim_mprof_fn (union param_val val, void *dat)
{
  if (NULL != config.sim.mprof_fn)
    {
      free (config.sim.mprof_fn);
    }

  config.sim.mprof_fn = strdup (val.str_val);
}

void
sim_history (union param_val val, void *dat)
{
  config.sim.history = val.int_val;
}

void
sim_exe_log (union param_val val, void *dat)
{
  config.sim.exe_log = val.int_val;
}

/*---------------------------------------------------------------------------*/
/*!Set the execution log type

   Value must be one of default, hardware, simple or software. Invalid values
   are ignored with a warning.

   @param[in] val  The value to use
   @param[in] dat  The config data structure (not used here)                 */
/*---------------------------------------------------------------------------*/
void
sim_exe_log_type (union param_val  val,
		  void            *dat)
{
  if (strcasecmp (val.str_val, "default") == 0)
    {
      config.sim.exe_log_type = EXE_LOG_HARDWARE;
    }
  else if (strcasecmp (val.str_val, "hardware") == 0)
    {
      config.sim.exe_log_type = EXE_LOG_HARDWARE;
    }
  else if (strcasecmp (val.str_val, "simple") == 0)
    {
      config.sim.exe_log_type = EXE_LOG_SIMPLE;
    }
  else if (strcasecmp (val.str_val, "software") == 0)
    {
      config.sim.exe_log_type = EXE_LOG_SOFTWARE;
    }
  else
    {
      fprintf (stderr, "Warning: Execution log type %s invalid. Ignored",
	       val.str_val);
    }
}	/* sim_exe_log_type() */


void
sim_exe_log_start (union param_val val, void *dat)
{
  config.sim.exe_log_start = val.longlong_val;
}

void
sim_exe_log_end (union param_val val, void *dat)
{
  config.sim.exe_log_end = val.longlong_val;
}

void
sim_exe_log_marker (union param_val val, void *dat)
{
  config.sim.exe_log_marker = val.int_val;
}

void
sim_exe_log_fn (union param_val val, void *dat)
{
  if (NULL != config.sim.exe_log_fn)
    {
      free (config.sim.exe_log_fn);
    }

  config.sim.exe_log_fn = strdup (val.str_val);
}

/*---------------------------------------------------------------------------*/
/*!Set the clock cycle time.

   Value must be an integer followed by one of ps, ns, us or ms.

   If a valid time is not presented, the value is unchanged.

   @param[in] val  The value to use
   @param[in] dat  The config data structure (not used here)                 */
/*---------------------------------------------------------------------------*/
void
sim_clkcycle (union param_val  val,
	      void            *dat)
{
  int len = strlen (val.str_val);
  int pos = len - 1;
  long time;
  if ((len < 2) || (val.str_val[pos--] != 's'))
    {
      fprintf (stderr, "Warning: Clock cycle time %s invalid: ignored\n",
	       val.str_val);
      return;
    }

  switch (val.str_val[pos--])
    {
    case 'p':
      time = 1;
      break;
    case 'n':
      time = 1000;
      break;
    case 'u':
      time = 1000000;
      break;
    case 'm':
      time = 1000000000;
      break;
    default:
      fprintf (stderr, "Warning: Clock cycle time %s invalid: ignored\n",
	       val.str_val);
      return;
    }

  val.str_val[pos + 1] = 0;
  time = time * atol (val.str_val);

  if (0 == time)
    {
      fprintf (stderr, "Warning: Clock cycle time of zero invalid: ignored\n");
      return;
    }

  config.sim.clkcycle_ps = time;

}	/* sim_clkcycle() */


/*---------------------------------------------------------------------------*/
/*!Register the functions to handle a section sim

   This section does not allocate dynamically a data structure holding its
   config information. It's all in the global config.sim data
   structure. Therefore it does not need a start and end function to
   initialize default values (although it might be clearer to do so). The
   default values are set in init_defconfig().

   New preferred parameter names are introduced (_file for filenames), but
   the legacy names (_fn) are also present for backwards compatibility       */
/*---------------------------------------------------------------------------*/
static void
reg_sim_sec ()
{
  struct config_section *sec = reg_config_sec ("sim", NULL, NULL);

  reg_config_param (sec, "verbose",        paramt_int,      sim_verbose);
  reg_config_param (sec, "debug",          paramt_int,      sim_debug);
  reg_config_param (sec, "profile",        paramt_int,      sim_profile);
  reg_config_param (sec, "prof_file",      paramt_str,      sim_prof_fn);
  reg_config_param (sec, "prof_fn",        paramt_str,      sim_prof_fn);
  reg_config_param (sec, "mprofile",       paramt_int,      sim_mprofile);
  reg_config_param (sec, "mprof_file",     paramt_str,      sim_mprof_fn);
  reg_config_param (sec, "mprof_fn",       paramt_str,      sim_mprof_fn);
  reg_config_param (sec, "history",        paramt_int,      sim_history);
  reg_config_param (sec, "exe_log",        paramt_int,      sim_exe_log);
  reg_config_param (sec, "exe_log_type",   paramt_word,     sim_exe_log_type);
  reg_config_param (sec, "exe_log_start",  paramt_longlong, sim_exe_log_start);
  reg_config_param (sec, "exe_log_end",    paramt_longlong, sim_exe_log_end);
  reg_config_param (sec, "exe_log_marker", paramt_int,      sim_exe_log_marker);
  reg_config_param (sec, "exe_log_file",   paramt_str,      sim_exe_log_fn);
  reg_config_param (sec, "exe_log_fn",     paramt_str,      sim_exe_log_fn);
  reg_config_param (sec, "clkcycle",       paramt_word,     sim_clkcycle);

}	/* reg_sim_sec() */


void
reg_config_secs (void)
{
  reg_config_param (reg_config_sec ("base", NULL, NULL), "include",
		    paramt_str, base_include);

  reg_generic_sec ();		/* JPB */
  reg_sim_sec ();
  reg_cpu_sec ();
  reg_pic_sec ();
  reg_memory_sec ();
  reg_mc_sec ();
  reg_uart_sec ();
  reg_dma_sec ();
  reg_debug_sec ();
  reg_vapi_sec ();
  reg_ethernet_sec ();
  reg_immu_sec ();
  reg_dmmu_sec ();
  reg_ic_sec ();
  reg_dc_sec ();
  reg_gpio_sec ();
  reg_bpb_sec ();
  reg_pm_sec ();
  reg_vga_sec ();
  reg_fb_sec ();
  reg_kbd_sec ();
  reg_ata_sec ();
  reg_cuc_sec ();
}

void
reg_config_param (struct config_section *sec, const char *param,
		  enum param_t type,
		  void (*param_cb) (union param_val, void *))
{
  struct config_param *new = malloc (sizeof (struct config_param));

  if (!new)
    {
      fprintf (stderr, "Out-of-memory\n");
      exit (1);
    }

  if (!(new->name = strdup (param)))
    {
      fprintf (stderr, "Out-of-memory\n");
      exit (1);
    }

  new->func = param_cb;
  new->type = type;

  new->next = sec->params;
  sec->params = new;
}

struct config_section *
reg_config_sec (const char *section,
		void *(*sec_start) (void), void (*sec_end) (void *))
{
  struct config_section *new = malloc (sizeof (struct config_section));

  if (!new)
    {
      fprintf (stderr, "Out-of-memory\n");
      exit (1);
    }

  if (!(new->name = strdup (section)))
    {
      fprintf (stderr, "Out-of-memory\n");
      exit (1);
    }

  new->next = sections;
  new->sec_start = sec_start;
  new->sec_end = sec_end;
  new->params = NULL;

  sections = new;

  return new;
}

static void
switch_param (char *param, struct config_param *cur_param)
{
  char *end_p;
  union param_val val;

  /* Skip over an = sign if it exists */
  if (*param == '=')
    {
      param++;
      while (*param && isspace (*param))
	param++;
    }

  switch (cur_param->type)
    {
    case paramt_int:
      val.int_val = strtol (param, NULL, 0);
      break;
    case paramt_longlong:
      val.longlong_val = strtoll (param, NULL, 0);
      break;
    case paramt_addr:
      val.addr_val = strtoul (param, NULL, 0);
      break;
    case paramt_str:
      if (*param != '"')
	{
	  fprintf (stderr,
		   "Warning: String value for parameter expected: ignored\n");
	  return;
	}

      param++;
      end_p = param;
      while (*end_p && (*end_p != '"'))
	end_p++;
      *end_p = '\0';
      val.str_val = param;
      break;
    case paramt_word:
      end_p = param;
      while (*end_p && !isspace (*end_p))
	end_p++;
      *end_p = '\0';
      val.str_val = param;
      break;
    case paramt_none:
      break;
    }

  cur_param->func (val, cur_section->dat);
}

/* Read environment from a script file. Does not fail - assumes default configuration instead.
   The syntax of script file is:
   param = value
   section x
     data
     param = value
   end
   
   Example:
   section mc
     memory_table_file = sim.mem
     enable = 1
     POC = 0x47892344
   end
   
 */

static void
read_script_file (const char *filename)
{
  FILE *f;
  char *home = getenv ("HOME");
  char ctmp[STR_SIZE];
  int local = 1;
  cur_section = NULL;

  sprintf (ctmp, "%s/.or1k/%s", home, filename);
  if ((f = fopen (filename, "rt")) || (home && (f = fopen (ctmp, "rt"))))
    {
      if (config.sim.verbose)
	PRINTF ("Reading script file from '%s'...\n",
		local ? filename : ctmp);

      while (!feof (f))
	{
	  char param[STR_SIZE];
	  if (fscanf (f, "%s ", param) != 1)
	    break;
	  /* Is this a section? */
	  if (strcmp (param, "section") == 0)
	    {
	      struct config_section *cur;
	      cur_section = NULL;
	      if (fscanf (f, "%s\n", param) != 1)
		{
		  fprintf (stderr, "%s: ERROR: Section name required.\n",
			   local ? filename : ctmp);
		  exit (1);
		}
	      for (cur = sections; cur; cur = cur->next)
		if (strcmp (cur->name, param) == 0)
		  {
		    cur_section = cur;
		    break;
		  }
	      if (!cur)
		{
		  fprintf (stderr,
			   "Warning: Unknown config section: %s; ignoring.\n",
			   param);
		  /* just skip section */
		  while (fscanf (f, "%s\n", param) == 1
			 && strcmp (param, "end"));
		}
	      else
		{
		  cur->dat = NULL;
		  if (cur->sec_start)
		    cur->dat = cur->sec_start ();
		}
	    }
	  else if (strcmp (param, "end") == 0)
	    {
	      if (cur_section->sec_end)
		cur_section->sec_end (cur_section->dat);
	      cur_section = NULL;
	    }
	  else if (strncmp (param, "/*", 2) == 0)
	    {
	      char c0 = 0, c1 = 0;
	      while (c0 != '*' || c1 != '/')
		{
		  c0 = c1;
		  c1 = fgetc (f);
		  if (feof (f))
		    {
		      fprintf (stderr, "%s: ERROR: Comment reached EOF.\n",
			       local ? filename : ctmp);
		      exit (1);
		    }
		}
	    }
	  else
	    {
	      struct config_param *cur_param;
	      char *cur_p;
	      for (cur_param = cur_section->params; cur_param;
		   cur_param = cur_param->next)
		if (strcmp (cur_param->name, param) == 0)
		  {
		    break;
		  }
	      if (!cur_param)
		{
		  fprintf (stderr, "Warning: Invalid parameter: %s; ignored\n",
			   param);
		  while (fgetc (f) != '\n' || feof (f));
		  continue;
		}

	      if (cur_param->type == paramt_none)
		continue;

	      /* Parse parameter value */
	      cur_p = fgets (param, STR_SIZE, f);

	      while (*cur_p && isspace (*cur_p))
		cur_p++;

	      switch_param (cur_p, cur_param);
	    }
	}
      fclose (f);
    }
  else if (config.sim.verbose)
    fprintf (stderr,
	     "Warning: Cannot read script file from '%s' of '%s'.\n",
	     filename, ctmp);
}

/* Utility for execution of set sim command.  */
static int
set_config (int argc, char **argv)
{
  struct config_section *cur;
  struct config_param *cur_param;

  if (argc < 2)
    return 1;

  PRINTF ("sec:%s\n", argv[1]);
  cur_section = NULL;
  for (cur = sections; cur; cur = cur->next)
    if (strcmp (cur->name, argv[1]) == 0)
      {
	cur_section = cur;
	break;
      }

  if (!cur_section)
    return 1;

  if (argc < 3)
    return 2;

  PRINTF ("item:%s\n", argv[2]);
  {
    for (cur_param = cur->params; cur_param; cur_param = cur_param->next)
      if (strcmp (cur_param->name, argv[2]) == 0)
	{
	  break;
	}
    if (!cur_param)
      return 2;

    /* Parse parameter value */
    if (cur_param->type)
      {
	if (argc < 4)
	  return 3;
	PRINTF ("params:%s\n", argv[3]);
      }

    switch_param (argv[3], cur_param);
  }
  return 0;
}

/* Executes set sim command, displays error.  */
void
set_config_command (int argc, char **argv)
{
  struct config_section *cur;
  struct config_param *cur_param;

  switch (set_config (argc, argv))
    {
    case 1:
      PRINTF
	("Invalid or missing section name.  One of valid sections must be specified:\n");
      for (cur = sections; cur; cur = cur->next)
	PRINTF ("%s ", cur->name);
      PRINTF ("\n");
      break;
    case 2:
      PRINTF
	("Invalid or missing item name.  One of valid items must be specified:\n");
      for (cur_param = cur_section->params; cur_param;
	   cur_param = cur_param->next)
	PRINTF ("%s ", cur_param->name);
      PRINTF ("\n");
      break;
    case 3:
      PRINTF ("Invalid parameters specified.\n");
      break;
    }
}
