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
#include "pcu.h"
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
#include "jtag.h"
#include "misc.h"
#include "argtable2.h"


/*! A structure used to represent possible parameters in a section. */
struct config_param
{
  char                 *name;				/* param name */
  enum param_t          type;				/* param type */
  void                (*func) (union param_val  val,	/* Setter function */
		  	       void            *dat);
  struct config_param  *next;				/* Next param in list */

};	/* struct config_param */


/*! Config file line count */
static int  line_number;

/*! The global configuration data structure */
struct config config;

/*! The global runtime status data structure */
struct runtime runtime;

/*! Master list of sections it is possible to configure. */
static struct config_section *section_master_list = NULL;


/* -------------------------------------------------------------------------- */
/*!Register a parameter for a section

   Add a new parameter to the list of parameters that may be set for a given
   section.

   @param[in] sec       The section containing the parameter.
   @param[in] param     Name of the parameter
   @param[in] type      Type of the parameter
   @param[in] param_cb  Call back function to set this parameter.             */
/* -------------------------------------------------------------------------- */
void
reg_config_param (struct config_section  *sec,
		  const char             *param,
		  enum param_t            type,
		  void                  (*param_cb) (union param_val,
						     void *))
{
  struct config_param *new = malloc (sizeof (struct config_param));

  if (!new)
    {
      fprintf (stderr, "ERROR: Out-of-memory allocating parameter: exiting,\n");
      exit (1);
    }

  if (!(new->name = strdup (param)))
    {
      fprintf (stderr, "ERROR: Out-of-memory allocating parameter name: "
	       "exiting,\n");
      exit (1);
    }

  /* Set up the parameter */
  new->func = param_cb;
  new->type = type;

  /* Insert on head of list */
  new->next = sec->params;
  sec->params = new;

}	/* reg_config_param () */


/* -------------------------------------------------------------------------- */
/*!Register a new section.

   Add a new section to the list of sections that may be found in a config
   file.

   @param[in] section    The name of the new section.
   @param[in] sec_start  Function to call at start of section configuration
                         (or NULL if none)
   @param[in] sec_end    Function to call at end of section configuration
                         (or NULL if none). Returns pointer to an arbitrary
                         data structure.

   @return  A pointer to the section data structure for the new section.      */
/* -------------------------------------------------------------------------- */
struct config_section *
reg_config_sec (const char   *section,
		void       *(*sec_start) (void),
		void        (*sec_end) (void *))
{
  struct config_section *new = malloc (sizeof (struct config_section));

  if (!new)
    {
      fprintf (stderr, "ERROR: Out-of-memory allocating section: exiting,\n");
      exit (1);
    }

  if (!(new->name = strdup (section)))
    {
      fprintf (stderr, "ERROR: Out-of-memory allocating section name: "
	       "exiting,\n");
      exit (1);
    }

  /* Set up the section */
  new->next      = section_master_list;
  new->sec_start = sec_start;
  new->sec_end   = sec_end;
  new->params    = NULL;

  /* Insert the section */
  section_master_list = new;

  return new;

}	/* reg_config_sec () */


/* -------------------------------------------------------------------------- */
/*!Look up a section

   Given a section name, return the data structure describing that section.

   @param[in] name  The section to look for.

   @return  A pointer to the section config data structure, or NULL if the
            section is not found                                              */
/* -------------------------------------------------------------------------- */
static struct config_section *
lookup_section (char *name)
{
  struct config_section *cur = NULL;

  for (cur = section_master_list; NULL != cur; cur = cur->next)
    {
      if (strcmp (cur->name, name) == 0)
	{
	  break;
	}
    }

  return  cur;

}	/* lookup_section () */


/* -------------------------------------------------------------------------- */
/*!Look up a parameter for a section

   Given a parameter name and a section data structure, return the data
   structure describing that parameter

   @param[in] name  The parameter to look for.
   @param[in] sec   The section containing the parameter.

   @return  A pointer to the parameter config data structure, or NULL if the
            parameter is not found                                            */
/* -------------------------------------------------------------------------- */
static struct config_param *
lookup_param (char                  *name,
	      struct config_section *sec)
{
  struct config_param *param = NULL;

  for (param = sec->params; NULL != param; param = param->next)
    {
      if (strcmp (param->name, name) == 0)
	{
	  break;
	}
    }

  return  param;

}	/* lookup_param () */


/* -------------------------------------------------------------------------- */
/*!Set default configuration parameters for fixed components

   These values are held in the global config variable. Parameter orders
   match the order in the corresponding section registration function and
   documentation.

   Also set some starting values for runtime elements.                        */
/* -------------------------------------------------------------------------- */
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
  config.sim.is_library          = 0;	/* Not library operation */
  config.sim.verbose             = 0;
  config.sim.debug               = 0;
  config.sim.profile             = 0;
  config.sim.prof_fn             = strdup ("sim.profile");
  config.sim.mprofile            = 0;
  config.sim.mprof_fn            = strdup ("sim.mprofile");
  config.sim.history             = 0;
  config.sim.exe_log             = 0;
  config.sim.exe_log_type        = EXE_LOG_HARDWARE;
  config.sim.exe_log_start       = 0;
  config.sim.exe_log_end         = 0;
  config.sim.exe_log_marker      = 0;
  config.sim.exe_log_fn          = strdup ("executed.log");
  config.sim.exe_bin_insn_log    = 0;
  config.sim.exe_bin_insn_log_fn = strdup ("exe-insn.bin");
  config.sim.clkcycle_ps         = 4000;	/* 4000 for 4ns (250MHz) */

  /* Debug */
  config.debug.jtagcycle_ps = 40000;	/* 40000 for 40ns (25MHz) */

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
  config.cpu.hardfloat        = 0;

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
  config.pic.use_nmi      = 1;

  if (config.pic.enabled)
    {
      cpu_state.sprs[SPR_UPR] |= SPR_UPR_PICP;
    }
  else
    {
      cpu_state.sprs[SPR_UPR] &= ~SPR_UPR_PICP;
    }

  /* Performance Counters Unit */
  config.pcu.enabled     = 0;

  /* Branch Prediction */
  config.bpb.enabled     = 0;
  config.bpb.btic        = 0;
  config.bpb.sbp_bnf_fwd = 0;
  config.bpb.sbp_bf_fwd  = 0;
  config.bpb.missdelay   = 0;
  config.bpb.hitdelay    = 0;
  
  /* Debug */
  config.debug.enabled     = 0;
  config.debug.rsp_enabled = 0;
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

  /* Debug */
  runtime.debug.instr           = JI_UNDEFINED;
  runtime.debug.mod_id          = JM_UNDEFINED;
  runtime.debug.write_defined_p = 0;		/* No WRITE_COMMAND yet */

  /* NPC state. Set to 1 when NPC is changed while the processor is stalled. */
  cpu_state.npc_not_valid = 0;

  /* VAPI */
  runtime.vapi.vapi_file = NULL;
  runtime.vapi.enabled   = 0;

}	/* init_defconfig() */


/* -------------------------------------------------------------------------- */
/*!Set a configuration parameter.

   We have a string representing the value, and a data structure representing
   the particular parameter. Break out the value and call the setter function
   to set its value in the section.

   The value text is guaranteed to have no leading or trailing whitespace.

   @param[in] cur_section  Spec of the section currently being configured.
   @param[in] param        Spec of the parameter we are setting.
   @param[in] val_text     The parameter value text			      */
/* -------------------------------------------------------------------------- */
static void
set_config_param (struct config_section *cur_section,
		  struct config_param   *param,
		  char                  *val_text)
{
  union param_val  val;

  /* Break out the different parameter types */
  switch (param->type)
    {
    case PARAMT_NONE:
      break;

    case PARAMT_INT:
      val.int_val = strtol (val_text, NULL, 0);
      break;

    case PARAMT_LONGLONG:
      val.longlong_val = strtoll (val_text, NULL, 0);
      break;

    case PARAMT_ADDR:
      val.addr_val = strtoul (val_text, NULL, 0);
      break;

    case PARAMT_WORD:
    case PARAMT_STR:
      /* Word and string are the same thing by now. */
      val.str_val = val_text;
      break;
    }

  /* Call the setter function */
  param->func (val, cur_section->dat);

}	/* set_config_param () */


/* -------------------------------------------------------------------------- */
/*!Scan the next word, skipping any preceding space.

   A word is anything that is not white space, except where the white space is
   within quotation marks. Return the number of newlines we have to skip.

   @param[in] f     The file handle to read from
   @param[in] word  A buffer in which to store the word.

   @return  The text of the next entity or NULL at end of file. Note strings
            have their quotation marks removed.                               */
/* -------------------------------------------------------------------------- */
static char *
next_word (FILE *f,
	   char  word[])
{
  int  c;
  int  i;

  /* Skip the whitespace */
  do
    {
      c = fgetc (f);

      line_number += ('\n' == c) ? 1: 0;
    }
  while ((EOF != c) && isspace (c));

  /* Get the word> Special treatment if it is a string. */
  if ('"' == c)
    {
      /* We have a string. Skip the opening quote. */
      c = fgetc (f);

      for (i = 0; i < (STR_SIZE - 1); i++)
	{
	  if ('"' == c)
	    {
	      c = fgetc (f);		/* So ungetc works */
	      break;			/* End of the string */
	    }
	  else if ('\n' == c)
	    {
	      line_number++;
	    }
	  else if (EOF == c)
	    {
	      fprintf (stderr, "ERROR: EOF in middle of string: exiting.\n");
	      exit (1);
	    }

	  word[i] = c;
	  c = fgetc (f);
	}

      /* Skip the closing quote */
      c = fgetc (f);
    }
  else
    {
      /* We have a space delimited word */
      for (i = 0; i < (STR_SIZE - 1); i++)
	{
	  if ((EOF == c) || isspace (c))
	    {
	      break;			/* End of the word */
	    }

	  word[i] = c;
	  c = fgetc (f);
	}
    }

  word[i] = '\0';			/* Terminate the word */

  if ((STR_SIZE - 1) == i)
    {
      word[10]= '\0';
      fprintf (stderr,
	       "ERROR: Symbol beginning %s on line %d too long: exiting.\n",
	       word, line_number);
      exit (1);
    }

  ungetc (c, f);			/* Ready for next time */

  return  (0 == i) ? NULL : word;

}	/* next_word () */
	 

/* -------------------------------------------------------------------------- */
/*!Read the next lexeme from the a config file.

   At this stage we are just breaking things out into space delimited
   entities, stripping out comments.

   @param[in] f       The file handle to read from
   @param[in] lexeme  A buffer in which to store the lexeme.

   @return  The text of the next entity or NULL at end of file.               */
/* -------------------------------------------------------------------------- */
static char *
next_lexeme (FILE *f,
	     char  lexeme[])
{
  if (NULL == next_word (f, lexeme))
    {
      return  NULL;
    }

  /* Skip any comments */
  while (0 ==strncmp (lexeme, "/*", 2))
    {
      /* Look for the closing '*' and '/'. */
      char  c0 = '\0';
      char  c1 = '\0';

      while (('*' != c0) || ('/' != c1))
	{
	  c0 = c1;
	  c1 = fgetc (f);

	  line_number += ('\n' == c1) ? 1 : 0;

	  /* We assume if we hit EOF we have a serious problem and die. */
	  if (feof (f))
	    {
	      fprintf (stderr, "ERROR: Comment reached EOF.\n");
	      exit (1);
	    }
	}

      /* Get the next lexeme */
      if (NULL == next_word (f, lexeme))
	{
	  return  NULL;
	}
    }

  return  lexeme;

}	/* next_lexeme () */


/* -------------------------------------------------------------------------- */
/*!Read configuration from a script file.

   The syntax of script file is:

     [section x
       [param [=] value]+
     end]*
   
   Example:

     section mc
       enabled = 1
       POC     = 0x47892344
     end
   
   The config file is searched for first in the local directory, then in
   ${HOME}/.or1ksim, then (for backwards compatibility) in ${HOME}/.or1ksim.

   If the file is not found, then a rude message is printed. The system will
   just use default values.

   @param[in] filename  The configuration file to use.                        */
/* -------------------------------------------------------------------------- */
static void
read_script_file (const char *filename)
{
  FILE *f;
  char *home = getenv ("HOME");
  char  ctmp1[STR_SIZE];
  char  ctmp2[STR_SIZE];
  char *dir;

  /* Attempt to open the config file. If we fail, give up with a rude
     message. */
  sprintf (ctmp1, "%s/.or1ksim/%s", home, filename);
  sprintf (ctmp2, "%s/.or1k/%s", home, filename);

  if (NULL != (f = fopen (filename, "r")))
    {
      dir = ".";
    }
  else if (home && (NULL != (f = fopen (ctmp1, "r"))))
    {
      dir = ctmp1;
    }
  else if (home && (NULL != (f = fopen (ctmp2, "r"))))
    {
      dir = ctmp2;
    }
  else
    {
      fprintf (stderr, "Warning: Failed to open script file \"%s\". Ignored.\n",
	       filename);
      return;
    }

  /* Log the config file we have just opened if required. */
  if (config.sim.verbose)
    {
      PRINTF ("Reading script file from \"%s/%s\"...\n", dir, filename);
    }

  /* Process the config file. */
  char lexeme[STR_SIZE];		/* Next entity from the input */
  int  in_section_p = 0;		/* Are we processing a section */

  struct config_section *cur_section = NULL;	/* Section being processed */

  line_number = 1;

  while (NULL != next_lexeme (f, lexeme))
    {
      /* Get the next symbol. Four possibilities.

	 1. It's "section". Only if we are not currently in a section. Process
	    the start of a section.

	 2. It's "end". Only if we are currently in a section. Process the end
	    of a section.

	 3. Anything else while we are in a section. Assume it is a parameter
	    for the current section.

	 4. Anything else. An error.
      */
      if (!in_section_p && (0 == strcmp (lexeme, "section")))
	{
	  /* We have the start of a section */
	  if (NULL == next_lexeme (f, lexeme))
	    {
	      fprintf (stderr, "ERROR: %s/%s: Section name required at line "
		       "%d. Exiting\n", dir, filename, line_number);
	      exit (1);
	    }

	  cur_section = lookup_section (lexeme);

	  if (NULL != cur_section)
	    {
	      /* Valid section, run its startup code, with any data saved. */
	      cur_section->dat = NULL;

	      if (cur_section->sec_start)
		{
		  cur_section->dat = cur_section->sec_start ();
		}

	      in_section_p = 1;		/* Now in a section */
	    }
	  else
	    {
	      /* Skip an unrecognized section with a warning. */
	      fprintf (stderr, "Warning: %s/%s: Unrecognized section: %s at "
		       "line %d: ignoring.\n", dir, filename, lexeme,
		       line_number);

	      /* just skip section */
	      while (NULL != next_lexeme (f, lexeme))
		{
		  if (strcmp (lexeme, "end"))
		    {
		      break;
		    }
		}
	    }
	}
      else if (in_section_p && strcmp (lexeme, "end") == 0)
	{
	  /* End of section. Run the end of section code */
	  if (cur_section->sec_end)
	    {
	      cur_section->sec_end (cur_section->dat);
	    }

	  in_section_p = 0;		/* Not in a section any more */
	}
      else if (in_section_p)
	{
	  /* We're in a section, so this must be a parameter. */
	  struct config_param *param;
	  char                *param_val;

	  param = lookup_param (lexeme, cur_section);

	  /* If we didn't recognize, then warn and skip to end of line/file) */
	  if (NULL == param)
	    {
	      fprintf (stderr, "Warning: %s/%s: Unrecognized parameter: %s at "
		       "line %d; ignored.\n", dir, filename, lexeme,
		       line_number);

	      /* Skip to end of line */
	      while (( '\n' != fgetc (f)) || feof (f))
		;

	      line_number++;
	      continue;			/* Start looking again */
	    }
	  
	  /* Get the argument if one is expected. */
	  if (PARAMT_NONE != param->type)
	    {
	      param_val = next_lexeme (f, lexeme);

	      if (NULL == param_val)
		{
		  fprintf (stderr, "Warning: %s/%s: No argument to parameter "
			   "%s at line %d; ignored.\n", dir, filename,
			   param->name, line_number);

		  /* Skip to end of line */
		  while (( '\n' != fgetc (f)) || feof (f))
		    ;

		  line_number++;
		  continue;			/* Start looking again */
		}
	      
	      /* We allow an optional '=' */
	      if (0 == strcmp (lexeme, "="))
		{
		  param_val = next_lexeme (f, lexeme);

		  if (NULL == param_val)
		    {
		      fprintf (stderr, "Warning: %s/%s: No argument to "
			       "parameter %s at line %d; ignored.\n", dir,
			       filename, param->name, line_number);

		      /* Skip to end of line */
		      while (( '\n' != fgetc (f)) || feof (f))
			;

		      line_number++;
		      continue;			/* Start looking again */
		    }
		}
	    }
	  else
	    {
	      /* No argument */
	      param_val = NULL;
	    }

	  /* Apply the parameter */
	  set_config_param (cur_section, param, param_val);
	}
      else
	{
	  /* We're not in a section, so we don't know what we have */
	  fprintf (stderr, "Warning: %s/%s: Unrecognized config file contents "
		   " at line %d: ignored.\n", dir, filename, line_number);

	  /* Skip to end of line */
	  while (( '\n' != fgetc (f)) || feof (f))
	    ;

	  line_number++;
	  continue;			/* Start looking again */
	}
    }

  fclose (f);		/* All done */

}	/* read_script_file () */


/*---------------------------------------------------------------------------*/
/*!Allocate a memory block

   We can request a block of memory be allocated from the command line, rather
   than in a configuration file. It will be allocated, starting at address
   zero.

   The memory size may be presented in any format recognized by strtol,
   optionally followed by "G" or "g", "M" or "m" or "K" or "k" for giga, mega
   and kilo bytes respectively, and leading to mutliplcation by 2^30, 2^20 and
   2^10 respectively.

   This is intended for simple use of the simulator in tool chain
   verification, where the detailed memory behavior does not matter.

   @param[in] size  Memory size.                                             */
/*---------------------------------------------------------------------------*/
static void
alloc_memory_block (const char *size)
{
  /* Sort out the memory size. */
  unsigned long int  multiplier;
  int                last_ch = strlen (size) - 1;

  switch (size[last_ch])
    {
    case 'G': case 'g': multiplier = 0x40000000UL; break;
    case 'M': case 'm': multiplier =   0x100000UL; break;
    case 'K': case 'k': multiplier =      0x400UL; break;
    default:            multiplier =        0x1UL; break;
    }

  unsigned long int  mem_size = strtoul (size, NULL, 0) * multiplier;

  if (0 == mem_size)
    {
      fprintf (stderr, "Warning: Memory size %s not recognized: ignored.\n",
	       size);
      return;
    }

  if (mem_size > 0xffffffff)
    {
      fprintf (stderr, "Warning: Memory size %s too large: ignored.\n",
	       size);
      return;
    }

  /* Turn the memory size back into a decimal string and allocate it */
  char str_size[11];
  sprintf (str_size, "%lu\n", mem_size);

  struct config_section *sec = lookup_section ("memory");

  sec->dat = sec->sec_start ();

  set_config_param (sec, lookup_param ("name", sec),     "Default RAM");
  set_config_param (sec, lookup_param ("type", sec),     "unknown");
  set_config_param (sec, lookup_param ("baseaddr", sec), "0");
  set_config_param (sec, lookup_param ("size", sec),     str_size);

  sec->sec_end (sec->dat);

  if (config.sim.verbose)
    {
      PRINTF ("Memory block of 0x%lx bytes allocated\n", mem_size);
    }  
}	/* alloc_memory_block () */


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
  struct arg_lit  *vercop;
  struct arg_lit  *help;
  struct arg_file *cfg_file;
  struct arg_lit  *nosrv;
  struct arg_int  *srv;
  struct arg_str  *mem;
  struct arg_str  *dbg;
  struct arg_lit  *command;
  struct arg_lit  *quiet;
  struct arg_lit  *verbose;
  struct arg_lit  *trace;
  struct arg_lit  *trace_phy;
  struct arg_lit  *trace_virt;
  struct arg_lit  *report_mem_errs;
  struct arg_lit  *strict_npc;
  struct arg_lit  *profile;
  struct arg_lit  *mprofile;
  struct arg_file *load_file;
  struct arg_end  *end;

  /* Specify each argument, with fall back values */
  vercop = arg_lit0 ("v", "version", "version and copyright notice");
  help = arg_lit0 ("h", "help", "print this help message");
  cfg_file = arg_file0 ("f", "file", "<file>",
			"config file (default \"sim.cfg\")");
  cfg_file->filename[0] = "sim.cfg";
  nosrv = arg_lit0 (NULL, "nosrv", "do not launch debug server");
  srv = arg_int0 (NULL, "srv", "<n>",
		  "launch debug server on port (default random)");
  srv->ival[0] = random () % (65536 - 49152) + 49152;
  srv->hdr.flag |= ARG_HASOPTVALUE;
  mem = arg_str0 ("m", "memory", "<n>", "add memory block of <n> bytes");
  dbg = arg_str0 ("d", "debug-config", "<str>", "Debug config string");
  command = arg_lit0 ("i", "interactive", "launch interactive prompt");
  quiet = arg_lit0 ("q", "quiet", "minimal message output");
  verbose = arg_lit0 ("V", "verbose", "verbose message output");
  trace = arg_lit0 ("t", "trace", "trace each instruction");
  trace_phy = arg_lit0 (NULL, "trace-physical",
			"show physical instruction address when tracing");
  trace_virt = arg_lit0 (NULL, "trace-virtual",
			"show virtual instruction address when tracing");
  report_mem_errs = arg_lit0 (NULL, "report-memory-errors",
			      "Report out of memory accesses");
  strict_npc = arg_lit0 (NULL, "strict-npc", "setting NPC flushes pipeline");
  profile = arg_lit0 (NULL, "enable-profile", "enable profiling");
  mprofile = arg_lit0 (NULL, "enable-mprofile", "enable memory profiling");
  load_file = arg_file0 (NULL, NULL, "<file>", "OR32 executable");
  end = arg_end (20);

  /* The argument table */
  void *argtab[] = {
    vercop,
    help,
    cfg_file,
    nosrv,
    srv,
    mem,
    dbg,
    command,
    quiet,
    verbose,
    trace,
    trace_phy,
    trace_virt,
    report_mem_errs,
    strict_npc,
    profile,
    mprofile,
    load_file,
    end };

  /* Parse */
  int  nerrors = arg_parse (argc, argv, argtab);

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

  /* Request for quiet running */
  config.sim.quiet = quiet->count;

  /* Request for verbose running. Not sensible to use with quiet as well. */
  if (quiet->count && verbose->count)
    {
      fprintf (stderr, "Warning: Cannot specify both --verbose and --quiet: "
	       "--vervose ignored.\n");
      config.sim.verbose = 0;
    }
  else
    {
      config.sim.verbose = verbose->count;
    }

  /* Request for tracing. We may ask for instructions to be recorded with
     either the physical or virtual address. */
  runtime.sim.hush       = trace->count ? 0 : 1;
  runtime.sim.trace_phy  = trace_phy->count  ? 1 : 0;
  runtime.sim.trace_virt = trace_virt->count ? 1 : 0;

  /* Ensure we have a least one address type in use. */
  if (!runtime.sim.trace_phy && !runtime.sim.trace_virt)
    {
      runtime.sim.trace_virt = 1;
    }

  /* Request for memory errors */
  config.sim.report_mem_errs = report_mem_errs->count;

  /* Process config file next (if given), so any other command args will
     override */
  if (0 == cfg_file->count)
    {
      if (config.sim.verbose)
	{
	  fprintf (stderr, "Default configuration used\n");
	}
    }
  else
    {
      read_script_file (cfg_file->filename[0]);
    }

  /* Allocate a memory block */
  if (mem->count > 0)
    {
      alloc_memory_block (mem->sval[0]);
    }

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
	  config.debug.rsp_enabled = 0;
	}
    }

  if (srv->count > 0)
    {
      config.debug.enabled     = 1;
      config.debug.rsp_enabled = 1;
      config.debug.rsp_port    = srv->ival[0];
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

/* Simulator configuration */


/*---------------------------------------------------------------------------*/
/*!Turn on verbose messages.

   @param[in] val  Non-zero (TRUE) to turn on verbose messages, zero (FALSE)
                   otherwise.
   @param[in] dat  The config data structure (not used here)                 */
/*---------------------------------------------------------------------------*/
static void
sim_verbose (union param_val  val,
	     void            *dat)
{
  config.sim.verbose = val.int_val;

}	/* sim_verbose () */


/*---------------------------------------------------------------------------*/
/*!Set the simulator debug message level

   Value must be in the range 0 (no messages) to 9. Values outside this range
   are converted to the nearer end of the range with a warning.

   @param[in] val  The value to use
   @param[in] dat  The config data structure (not used here)                 */
/*---------------------------------------------------------------------------*/
static void
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


/*---------------------------------------------------------------------------*/
/*!Turn on profiling

   @param[in] val  Non-zero (TRUE) to turn on profiling, zero (FALSE) otherwise.
   @param[in] dat  The config data structure (not used here)                 */
/*---------------------------------------------------------------------------*/
static void
sim_profile (union param_val  val,
	     void            *dat)
{
  config.sim.profile = val.int_val;

}	/* sim_profile () */


/*---------------------------------------------------------------------------*/
/*!Specify the profiling file name.

   @param[in] val  The profiling file name
   @param[in] dat  The config data structure (not used here)                 */
/*---------------------------------------------------------------------------*/
static void
sim_prof_fn (union param_val  val,
	     void            *dat)
{
  if (NULL != config.sim.prof_fn)
    {
      free (config.sim.prof_fn);
    }

  config.sim.prof_fn = strdup(val.str_val);

}	/* sim_prof_fn () */


/*---------------------------------------------------------------------------*/
/*!Turn on memory profiling

   @param[in] val  Non-zero (TRUE) to turn on memory profiling, zero (FALSE)
                   otherwise.
   @param[in] dat  The config data structure (not used here)                 */
/*---------------------------------------------------------------------------*/
static void
sim_mprofile (union param_val  val,
	      void            *dat)
{
  config.sim.mprofile = val.int_val;

}	/* sim_mprofile () */


/*---------------------------------------------------------------------------*/
/*!Specify the memory profiling file name.

   @param[in] val  The memory profiling file name
   @param[in] dat  The config data structure (not used here)                 */
/*---------------------------------------------------------------------------*/
static void
sim_mprof_fn (union param_val val, void *dat)
{
  if (NULL != config.sim.mprof_fn)
    {
      free (config.sim.mprof_fn);
    }

  config.sim.mprof_fn = strdup (val.str_val);

}	/* sim_mprof_fn () */


/*---------------------------------------------------------------------------*/
/*!Turn on execution tracking.

   @param[in] val  Non-zero (TRUE) to turn on tracking, zero (FALSE) otherwise.
   @param[in] dat  The config data structure (not used here)                 */
/*---------------------------------------------------------------------------*/
static void
sim_history (union param_val  val,
	     void            *dat)
{
  config.sim.history = val.int_val;
}


/*---------------------------------------------------------------------------*/
/*!Record an execution log

   @param[in] val  Non-zero (TRUE) to turn on logging, zero (FALSE) otherwise.
   @param[in] dat  The config data structure (not used here)                 */
/*---------------------------------------------------------------------------*/
static void
sim_exe_log (union param_val  val,
	     void            *dat)
{
  config.sim.exe_log = val.int_val;

}	/* sim_exe_log () */

/*---------------------------------------------------------------------------*/
/*!Set the execution log type

   Value must be one of default, hardware, simple or software. Invalid values
   are ignored with a warning.

   @param[in] val  The value to use
   @param[in] dat  The config data structure (not used here)                 */
/*---------------------------------------------------------------------------*/
static void
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


/*---------------------------------------------------------------------------*/
/*!Set the execution log start address

   Address at which to start logging.

   @param[in] val  The value to use
   @param[in] dat  The config data structure (not used here)                 */
/*---------------------------------------------------------------------------*/
static void
sim_exe_log_start (union param_val  val,
		   void            *dat)
{
  config.sim.exe_log_start = val.longlong_val;

}	/* sim_exe_log_start () */


/*---------------------------------------------------------------------------*/
/*!Set the execution log end address

   Address at which to end logging.

   @param[in] val  The value to use
   @param[in] dat  The config data structure (not used here)                 */
/*---------------------------------------------------------------------------*/
static void
sim_exe_log_end (union param_val  val,
		 void            *dat)
{
  config.sim.exe_log_end = val.longlong_val;

}	/* sim_exe_log_end () */


/*---------------------------------------------------------------------------*/
/*!Specify number of instruction between printing horizontal markers

   Control of log format

   @param[in] val  The value to use
   @param[in] dat  The config data structure (not used here)                 */
/*---------------------------------------------------------------------------*/
static void
sim_exe_log_marker (union param_val  val,
		    void            *dat)
{
  config.sim.exe_log_marker = val.int_val;

}	/* sim_exe_log_marker () */


/*---------------------------------------------------------------------------*/
/*!Specify the execution log file name.

   @param[in] val  The execution log file name
   @param[in] dat  The config data structure (not used here)                 */
/*---------------------------------------------------------------------------*/
static void
sim_exe_log_fn (union param_val val, void *dat)
{
  if (NULL != config.sim.exe_log_fn)
    {
      free (config.sim.exe_log_fn);
    }

  config.sim.exe_log_fn = strdup (val.str_val);

}	/* sim_exe_log_fn () */


/*---------------------------------------------------------------------------*/
/*!Turn on binary instruction logging

   @param[in] val  Non-zero (TRUE) to turn on logging, zero (FALSE) otherwise.
   @param[in] dat  The config data structure (not used here)                 */
/*---------------------------------------------------------------------------*/
static void
sim_exe_bin_insn_log (union param_val val, void *dat)
{
  config.sim.exe_bin_insn_log = val.int_val;

}	/* sim_exe_bin_insn_log () */


/*---------------------------------------------------------------------------*/
/*!Specify the binary instruction log file name.

   @param[in] val  The binary instruction log file name
   @param[in] dat  The config data structure (not used here)                 */
/*---------------------------------------------------------------------------*/
static void
sim_exe_bin_insn_log_fn (union param_val val, void *dat)
{
  if (NULL != config.sim.exe_bin_insn_log_fn)
    {
      free (config.sim.exe_bin_insn_log_fn);
    }

  config.sim.exe_bin_insn_log_fn = strdup (val.str_val);

}	/* sim_exe_bin_insn_log_fn () */


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

  reg_config_param (sec, "verbose",        PARAMT_INT,      sim_verbose);
  reg_config_param (sec, "debug",          PARAMT_INT,      sim_debug);
  reg_config_param (sec, "profile",        PARAMT_INT,      sim_profile);
  reg_config_param (sec, "prof_file",      PARAMT_STR,      sim_prof_fn);
  reg_config_param (sec, "prof_fn",        PARAMT_STR,      sim_prof_fn);
  reg_config_param (sec, "mprofile",       PARAMT_INT,      sim_mprofile);
  reg_config_param (sec, "mprof_file",     PARAMT_STR,      sim_mprof_fn);
  reg_config_param (sec, "mprof_fn",       PARAMT_STR,      sim_mprof_fn);
  reg_config_param (sec, "history",        PARAMT_INT,      sim_history);
  reg_config_param (sec, "exe_log",        PARAMT_INT,      sim_exe_log);
  reg_config_param (sec, "exe_log_type",   PARAMT_WORD,     sim_exe_log_type);
  reg_config_param (sec, "exe_log_start",  PARAMT_LONGLONG, sim_exe_log_start);
  reg_config_param (sec, "exe_log_end",    PARAMT_LONGLONG, sim_exe_log_end);
  reg_config_param (sec, "exe_log_marker", PARAMT_INT,      sim_exe_log_marker);
  reg_config_param (sec, "exe_log_file",   PARAMT_STR,      sim_exe_log_fn);
  reg_config_param (sec, "exe_log_fn",     PARAMT_STR,      sim_exe_log_fn);

  reg_config_param (sec, "exe_bin_insn_log",      PARAMT_INT,
		    sim_exe_bin_insn_log);
  reg_config_param (sec, "exe_bin_insn_log_fn",   PARAMT_STR,
		    sim_exe_bin_insn_log_fn);
  reg_config_param (sec, "exe_bin_insn_log_file", PARAMT_STR,
		    sim_exe_bin_insn_log_fn);

  reg_config_param (sec, "clkcycle",       PARAMT_WORD,     sim_clkcycle);

}	/* reg_sim_sec() */


/*---------------------------------------------------------------------------*/
/*!Register all the possible sections which we support.

   Each section type provides a registration function. This returns a struct
   config_section, which in turn contains a list of config_params.           */
/*---------------------------------------------------------------------------*/
void
reg_config_secs ()
{
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
  reg_pcu_sec ();
  reg_vga_sec ();
  reg_fb_sec ();
  reg_kbd_sec ();
  reg_ata_sec ();
  reg_cuc_sec ();
}

/* Utility for execution of set sim command.  */
static int
set_config (int argc, char **argv)
{
  struct config_section *cur_section;
  struct config_param *param;

  if (argc < 2)
    return 1;

  PRINTF ("sec:%s\n", argv[1]);
  cur_section = lookup_section (argv[1]);

  if (NULL == cur_section)
    return 1;

  if (argc < 3)
    return 2;

  PRINTF ("item:%s\n", argv[2]);
  {
    for (param = cur_section->params; param; param = param->next)
      if (strcmp (param->name, argv[2]) == 0)
	{
	  break;
	}
    if (!param)
      return 2;

    /* Parse parameter value */
    if (param->type)
      {
	if (argc < 4)
	  return 3;
	PRINTF ("params:%s\n", argv[3]);
      }

    set_config_param (cur_section, param, argv[3]);
  }
  return 0;
}

/* Executes set sim command, displays error.  */
void
set_config_command (int argc, char **argv)
{
  struct config_section *cur;
  struct config_param *param;

  switch (set_config (argc, argv))
    {
    case 1:
      PRINTF
	("Invalid or missing section name.  One of valid sections must be specified:\n");
      for (cur = section_master_list; cur; cur = cur->next)
	PRINTF ("%s ", cur->name);
      PRINTF ("\n");
      break;
    case 2:
      cur = lookup_section (argv[1]);
      PRINTF
	("Invalid or missing item name.  One of valid items must be specified:\n");
      for (param = cur->params; param;
	   param = param->next)
	PRINTF ("%s ", param->name);
      PRINTF ("\n");
      break;
    case 3:
      PRINTF ("Invalid parameters specified.\n");
      break;
    }
}
