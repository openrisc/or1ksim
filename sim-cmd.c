/* sim-cmd.c -- Simulator command parsing

   Copyright (C) 1999 Damjan Lampret, lampret@opencores.org
   Copyright (C) 2005 György `nog' Jeney, nog@sdf.lonestar.org
   Copyright (C) 2008 Embecosm Limited
   Copyright (C) 2011 Giuseppe Scrivano <gscrivano@gnu.org>

   Contributor Jeremy Bennett <jeremy.bennett@embecosm.com>
   contributor Giuseppe Scrivano <gscrivano@gnu.org>

   This file is part of Or1ksim, the OpenRISC 1000 Architectural Simulator.

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the Free
   Software Foundation; either version 3 of the License, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
   more details.

   You should have received a copy of the GNU General Public License along
   with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* This program is commented throughout in a fashion suitable for processing
   with Doxygen. */


/* Autoconf and/or portability configuration */
#include "config.h"

/* System includes */
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#ifdef HAVE_LIBREADLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif /* HAVE_LIBREADLINE */

/* Package includes */
#include "sim-cmd.h"
#include "sim-config.h"
#include "execute.h"
#include "labels.h"
#include "sched.h"
#include "toplevel-support.h"
#include "dumpverilog.h"
#include "profiler.h"
#include "mprofiler.h"
#include "trace.h"
#include "debug-unit.h"
#include "stats.h"
#include "sprs.h"
#include "dcache-model.h"
#include "branch-predict.h"
#include "debug.h"
#include "cuc.h"
#include "rsp-server.h"


/* The number of instructions to execute before droping into interactive mode */
static long long to_insn_num;

struct sim_stat
{
  void (*stat_func) (void *dat);
  void *dat;
  struct sim_stat *next;
};

static struct sim_stat *sim_stats = NULL;

/* Registers a status printing callback */
void
reg_sim_stat (void (*stat_func) (void *dat), void *dat)
{
  struct sim_stat *new = malloc (sizeof (struct sim_stat));

  if (!new)
    {
      fprintf (stderr, "reg_sim_stat: Out-of-memory\n");
      exit (1);
    }

  new->stat_func = stat_func;
  new->dat = dat;
  new->next = sim_stats;
  sim_stats = new;
}

/* Scheduler job that drops us back into interactive mode after the next
 * instruction has executed */
void
reenter_int (void *dat)
{
  if (!runtime.sim.hush)
    dumpreg ();
  handle_sim_command ();
}

static int
sim_cmd_quit (int argc, char **argv)	/* quit */
{
  PRINTF ("\n");
  sim_done ();
  return 0;
}

static int
sim_cmd_help (int argc, char **argv)	/* help */
{
  PRINTF ("q                              - quit simulator\n");
  PRINTF ("r                              - display all registers\n");
  PRINTF ("t                              - execute next instruction\n");
  PRINTF
    ("run <instructions> [<hush>]    - execute <instruction> instructions, no reg\n");
  PRINTF ("                                 dump if hush\n");
  PRINTF
    ("pr <r> <value>                 - patch register <r> with <value>\n");
  PRINTF
    ("dm <fromaddr> [<toaddr>]       - display memory from <fromaddr> to <toaddr>\n");
  PRINTF ("de <fromaddr> [<toaddr>]       - debug insn memory\n");
  PRINTF
    ("pm <addr> <value>              - patch memory location <addr> with <value>\n");
  PRINTF
    ("pc <value>                     - patch PC register with <value>\n");
  PRINTF ("cm <fromaddr> <toaddr> <size>  - copy memory\n");
  PRINTF
    ("break <addr>                   - toggle breakpoint at address <addr>\n");
  PRINTF ("breaks                         - print all set breakpoints\n");
  PRINTF ("reset                          - simulator reset\n");
  PRINTF ("hist                           - execution history\n");
  PRINTF
    ("stall                          - stalls the processor and gives control to\n");
  PRINTF ("                                 the debugger\n");
  PRINTF ("unstall                        - unstalls the processor\n");
  PRINTF
    ("stats <num|clear>              - execution statistics num or clear it.\n");
  PRINTF
    ("info                           - configuration info (caches etc.)\n");
  PRINTF
    ("dv <fromaddr> [<toaddr>] [<m>] - dumps memory as verilog module <m>\n");
  PRINTF ("                                 (use redirect to capture)\n");
  PRINTF
    ("dh <fromaddr> [<toaddr>]       - dumps memory as hex code (use redirect)\n");
  PRINTF ("setdbch                        - toggles debug channels on/off\n");
  PRINTF
    ("set <section> <item> = <param> - set configuration.  See sim.cfg for more\n");
  PRINTF ("                                 information.\n");
  PRINTF ("debug                          - toggles simulator debug mode\n");
  PRINTF
    ("cuc                            - enters Custom Unit Compiler command prompt\n");
  PRINTF
    ("help                           - available commands (this list)\n");
  PRINTF
    ("<cmd> > <filename>             - redirect simulator stdout to <filename>\n");
  PRINTF ("                                 (and not emulated PRINTF)\n\n");
  (void)main_mprofiler (0, NULL, 1);
  PRINTF ("\n");
  (void)main_profiler (0, NULL, 1);
  return 0;
}

static int
sim_cmd_trace (int argc, char **argv)	/* trace */
{
  runtime.sim.hush = 0;
  sched_next_insn (reenter_int, NULL);
  return 1;
}

static int
sim_cmd_dm (int argc, char **argv)	/* dump memory */
{
  static oraddr_t from = 0, to = 0;

  if (argc >= 2)
    {
      if (argv[1][0] == '_')
	from = eval_label (argv[1]);
      else
	from = strtoul (argv[1], NULL, 0);
      to = from + 0x40;
    }
  if (argc >= 3)
    to = strtoul (argv[2], NULL, 0);
  dump_memory (from, to);
  PRINTF ("\n");
  return 0;
}

static int
sim_cmd_dv (int argc, char **argv)	/* dump memory as verilog */
{
  static oraddr_t from = 0, to = 0;

  if (argc >= 2)
    {
      if (argv[1][0] == '_')
	from = eval_label (argv[1]);
      else
	from = strtoul (argv[1], NULL, 0);
      to = from + 0x40;
    }
  if (argc >= 3)
    to = strtoul (argv[2], NULL, 0);

  if (argc < 4)
    dumpverilog ("or1k_mem", from, to);
  else
    dumpverilog (argv[3], from, to);

  PRINTF ("\n");
  return 0;
}

static int
sim_cmd_dh (int argc, char **argv)	/* dump memory as hex */
{
  static oraddr_t from = 0, to = 0;

  if (argc >= 2)
    {
      if (argv[1][0] == '_')
	from = eval_label (argv[1]);
      else
	from = strtoul (argv[1], NULL, 0);
      to = from + 0x40;
    }
  if (argc >= 3)
    to = strtoul (argv[2], NULL, 0);

  dumphex (from, to);
  PRINTF ("\n");
  return 0;
}

static int
sim_cmd_pm (int argc, char **argv)	/* patch memory */
{
  static oraddr_t addr = 0;
  int breakpoint = 0;

  if (argc != 3)
    {
      PRINTF ("pm <address> <value>\n");
      return 0;
    }

  if (argc >= 2)
    {
      if (argv[1][0] == '_')
	addr = eval_label (argv[1]);
      else
	addr = strtoul (argv[1], NULL, 0);
    }
  set_mem32 (addr, strtoul (argv[2], NULL, 0), &breakpoint);
  return 0;
}

static int
sim_cmd_cm (int argc, char **argv)	/* copy memory 2004-01-20 hpanther */
{
  static oraddr_t from = 0, to = 0;
  static unsigned int size = 0;
  int i;

  if (argc >= 2)
    {
      if (argv[1][0] == '_')
	from = eval_label (argv[1]);
      else
	from = strtoul (argv[1], NULL, 0);
    }

  if (argc >= 3)
    {
      if (argv[2][0] == '_')
	to = eval_label (argv[2]);
      else
	to = strtoul (argv[2], NULL, 0);
    }

  if (argc >= 4)
    {
      if (argv[3][0] == '_')
	size = eval_label (argv[3]);
      else
	size = strtoul (argv[3], NULL, 0);
    }

  for (i = 0; i < size; i += 4)
    set_direct32 (to + i, eval_direct32 (from + i, 0, 0), 0, 0);
  return 0;
}

static int
sim_cmd_pr (int argc, char **argv)	/* patch regs */
{
  if (argc != 3)
    {
      PRINTF ("pr <register> <value>\n");
      return 0;
    }
  setsim_reg (strtoul (argv[1], NULL, 0), strtoul (argv[2], NULL, 0));
  return 0;
}

static int
sim_cmd_pc (int argc, char **argv)	/* patch PC */
{
  if (argc != 2)
    {
      PRINTF ("pc <value>\n");
      return 0;
    }

  cpu_state.pc = strtoul (argv[1], NULL, 0);
  pcnext = cpu_state.pc + 4;
  return 0;
}

static int
sim_cmd_breaks (int argc, char **argv)	/* print breakpoints */
{
  print_breakpoints ();
  return 0;
}

static int
sim_cmd_break (int argc, char **argv)	/* set/clear breakpoint */
{
  char *p;
  oraddr_t addr;
  struct label_entry *l;

  if (argc != 2)
    {
      PRINTF ("break <label or address>\n");
      return 0;
    }

  addr = strtoul (argv[1], &p, 0);
  if (*p)
    {
      l = find_label (argv[1]);
      if (l)
	set_insnbrkpoint (l->addr);
      else
	PRINTF ("Label `%s' does not exist\n", argv[1]);
    }
  else
    set_insnbrkpoint (addr);
  return 0;
}

static int
sim_cmd_r (int argc, char **argv)	/* dump regs */
{
  dumpreg ();
  return 0;
}

static int
sim_cmd_de (int argc, char **argv)	/* disassemble */
{
  static oraddr_t from = 0, to = 0;

  if (argc >= 2)
    {
      if (argv[1][0] == '_')
	from = eval_label (argv[1]);
      else
	from = strtoul (argv[1], NULL, 0);
      to = from + 0x40;
    }

  if (argc >= 3)
    to = strtoul (argv[2], NULL, 0);

  disassemble_memory (from, to, 1);
  PRINTF ("\n");
  return 0;
}

static int
sim_cmd_reset (int argc, char **argv)	/* reset simulator */
{
  sim_reset ();
  return 0;
}

static int
sim_cmd_hist (int argc, char **argv)	/* dump history */
{
  int i;
  struct hist_exec *cur;
  if (!config.sim.history)
    {
      PRINTF ("Simulation history disabled.\n");
      return 0;
    }
  for (i = HISTEXEC_LEN, cur = hist_exec_tail->next; i; i--, cur = cur->next)
    disassemble_memory (cur->addr, cur->addr + 4, 1);
  PRINTF ("\n");
  return 0;
}

/* Called when it is suspisous that runtime.sim.instructions has reached
 * to_insn_num */
void
check_insn_exec (void *dat)
{
  if (runtime.cpu.instructions < to_insn_num)
    {
      /* Instruction count has not yet been reached, reschedule */
      long long int delta = to_insn_num - runtime.cpu.instructions;
      SCHED_ADD (check_insn_exec, NULL,
		 (delta > INT32_MAX) ? INT32_MAX : delta);
      return;
    }
  handle_sim_command ();
}

void
print_insn_exec (void *dat)
{
  dumpreg ();
  if (runtime.cpu.instructions < to_insn_num)
    {
      /* Instruction count has not yet been reached, reschedule */
      sched_next_insn (print_insn_exec, NULL);
      return;
    }
  handle_sim_command ();
}

static int
sim_cmd_run (int argc, char **argv)	/* run */
{
  runtime.sim.hush = 0;
  if (argc >= 3)
    {
      if (!strcmp (argv[2], "hush"))
	runtime.sim.hush = 1;
    }

  if (argc >= 2)
    {
      if ((to_insn_num = strtoll (argv[1], NULL, 0)) != -1)
	{
	  if (runtime.sim.hush)
	    {
	      /* Schedule a job to run in to_insn_num cycles time since an instruction
	       * may execute in only 1 cycle.  check_insn_exec will check if the right
	       * number of instructions have been executed.  If not it will
	       * reschedule.  */
	      SCHED_ADD (check_insn_exec, NULL,
			 (to_insn_num > INT32_MAX) ? INT32_MAX : to_insn_num);
	    }
	  else
	    {
	      /* The user wants to see the execution dumps.  Schedule a task to show
	       * it to him after each cycle */
	      sched_next_insn (print_insn_exec, NULL);
	    }
	  to_insn_num += runtime.cpu.instructions;
	}
      else
	{
	  if (!runtime.sim.hush)
	    sched_next_insn (print_insn_exec, NULL);
	}
    }
  else
    /* Run 0 instructions */
    return 0;

  return 1;
}

static int
sim_cmd_stall (int argc, char **argv)	/* Added by CZ 210801 */
{
  set_stall_state (1);
  runtime.sim.iprompt = 0;
  runtime.sim.hush = 1;
  return 1;
}

static int
sim_cmd_unstall (int argc, char **argv)	/* Added by CZ 210801 */
{
  set_stall_state (0);
  return 0;
}

static int
sim_cmd_stats (int argc, char **argv)	/* stats */
{
  if (argc != 2)
    {
      PRINTF ("stats <stat no. or `clear'>\n");
      return 0;
    }

  if (strcmp (argv[1], "clear") == 0)
    {
      initstats ();
      PRINTF ("Cleared.\n");
    }
  else
    {
      printstats (strtoul (argv[1], NULL, 0));
    }
  return 0;
}

static int
sim_cmd_info (int argc, char **argv)	/* configuration info */
{
  struct sim_stat *cur_stat = sim_stats;

  /* Display info about various modules */
  sprs_status ();
  PRINTF ("\n");
  memory_table_status ();
  if (config.dc.enabled)
    dc_info ();

  if (config.bpb.enabled)
    bpb_info ();
  if (config.bpb.btic)
    btic_info ();

  while (cur_stat)
    {
      cur_stat->stat_func (cur_stat->dat);
      cur_stat = cur_stat->next;
    }

  return 0;
}

static int
sim_cmd_setdbch (int argc, char **argv)	/* Toggle debug channel on/off */
{
  if (argc != 2)
    {
      PRINTF ("setdbch <channel>\n");
      return 0;
    }
  parse_dbchs (argv[1]);
  return 0;
}

static int
sim_cmd_debug (int argc, char **argv)	/* debug mode */
{
  config.sim.debug ^= 1;
  return 0;
}

static int
sim_cmd_profile (int argc, char **argv)	/* run profiler utility */
{
  return  main_profiler (argc, argv, 0);
}

static int
sim_cmd_mprofile (int argc, char **argv)	/* run mprofiler utility */
{
  return  main_mprofiler (argc, argv, 0);
}

static int
sim_cmd_cuc (int argc, char **argv)	/* run Custom Unit Compiler */
{
  main_cuc (runtime.sim.filename);
  return 0;
}

static int
sim_cmd_set (int argc, char **argv)	/* configuration info */
{
  set_config_command (argc, argv);
  return 0;
}

static char *
strip_space (char *str)
{
  while (isblank ((int)*str) && *str)
    str++;
  return str;
}

struct sim_command
{
  const char *name;
  int (*cmd_handle) (int argc, char **argv);
};

static const struct sim_command sim_commands[] = {
  {"q", sim_cmd_quit},
  {"help", sim_cmd_help},
  {"t", sim_cmd_trace},
  {"dm", sim_cmd_dm},
  {"dv", sim_cmd_dv},
  {"dh", sim_cmd_dh},
  {"pm", sim_cmd_pm},
  {"cm", sim_cmd_cm},
  {"pr", sim_cmd_pr},
  {"pc", sim_cmd_pc},
  {"breaks", sim_cmd_breaks},
  {"break", sim_cmd_break},
  {"r", sim_cmd_r},
  {"de", sim_cmd_de},
  {"reset", sim_cmd_reset},
  {"hist", sim_cmd_hist},
  {"stall", sim_cmd_stall},
  {"unstall", sim_cmd_unstall},
  {"stats", sim_cmd_stats},
  {"info", sim_cmd_info},
  {"run", sim_cmd_run},
  {"setdbch", sim_cmd_setdbch},
  {"debug", sim_cmd_debug},
  {"profile", sim_cmd_profile},
  {"mprofile", sim_cmd_mprofile},
  {"cuc", sim_cmd_cuc},
  {"set", sim_cmd_set},
  {NULL, NULL}
};

#ifdef HAVE_LIBREADLINE
static void initialize_readline (void);
#endif

void
handle_sim_command (void)
{
  char *redirstr;
  int argc;
  char *argv[5];
  char *cur_arg;
  const struct sim_command *cur_cmd;
#ifdef HAVE_LIBREADLINE
  static char *prev_str = NULL;
#else
  char b2[500];
  static char prev_str[500] = { 0 };
#endif

  runtime.sim.iprompt_run = 1;

  /* Make sure that check_insn_exec is not left hanging in the scheduler (and
   * breaking the sim when the user doesn't want it to break). */
  SCHED_FIND_REMOVE (check_insn_exec, NULL);
  SCHED_FIND_REMOVE (print_insn_exec, NULL);

#ifdef HAVE_LIBREADLINE
  initialize_readline ();	/* Bind our completer. */
#endif

  for (;;)
    {
#ifdef HAVE_LIBREADLINE
      cur_arg = readline ("(sim) ");

      if (!cur_arg)
	{
	  sim_done ();
	}
#else
      PRINTF ("(sim) ");

      cur_arg = fgets (b2, sizeof (b2), stdin);

      if (!cur_arg)
	sim_done ();

      if (!*cur_arg)
	{
	  usleep (1000);
	  continue;
	}
#endif

#ifdef HAVE_LIBREADLINE
      if (!*cur_arg)
	{
	  if (prev_str)
	    {
	      free (cur_arg);
	      cur_arg = prev_str;
	    }
	}
      else
	{
	  prev_str = cur_arg;
	  add_history (cur_arg);
	}
#else
      cur_arg = strip_space (cur_arg);
      if (*cur_arg == '\n')
	strcpy (cur_arg, prev_str);
      else
	strcpy (prev_str, cur_arg);
#endif

      if ((redirstr = strchr (cur_arg, '>')))
	{
	  redirstr = strip_space (++redirstr);

	  while (!isspace ((int)*redirstr) && *redirstr)
	    redirstr++;
	  *redirstr = '\0';

	  redirstr = strchr (cur_arg, '>');
	  *redirstr = '\0';

	  redirstr = strip_space (++redirstr);
	  runtime.sim.fout = fopen (redirstr, "w+");
	  if (!runtime.sim.fout)
	    runtime.sim.fout = stdout;
	}

      if (*cur_arg)
	{
	  argc = 0;
	  while (*cur_arg)
	    {
	      argv[argc] = cur_arg;
	      argc++;
	      while (!isspace ((int)*cur_arg) && *cur_arg)
		cur_arg++;
	      if (*cur_arg)
		{
		  *cur_arg = '\0';
		  cur_arg = strip_space (cur_arg + 1);
		}
	      else
		*cur_arg = '\0';
	      if (argc == 5)
		{
		  fprintf (stderr,
			   "Too many arguments given to command `%s'\n",
			   argv[0]);
		  break;
		}
	    }

	  for (cur_cmd = sim_commands; cur_cmd->name; cur_cmd++)
	    {
	      if (!strcmp (cur_cmd->name, argv[0]))
		{
		  if (cur_cmd->cmd_handle (argc, argv))
		    {
		      runtime.sim.iprompt = 0;
		      runtime.sim.iprompt_run = 0;
		      return;
		    }
		  break;
		}
	    }

	  if (!cur_cmd->name)
	    PRINTF ("%s: Unknown command.\n", argv[0]);
	}

      if (redirstr)
	{
	  redirstr = NULL;
	  fclose (runtime.sim.fout);
	  runtime.sim.fout = stdout;
	}

    }
}

#ifdef HAVE_LIBREADLINE

char *command_generator ();
char **sim_completion ();

/* Tell the GNU readline library how to complete.  We want to try to complete
   on command names if this is the first word in the line, or on filenames
   if not. */
static void
initialize_readline (void)
{
  /* Allow conditional parsing of the ~/.inputrc file. */
  rl_readline_name = "or1ksim";

  /* Tell the completer that we want a crack first. */
  rl_attempted_completion_function = sim_completion;

}

/* Attempt to complete on the contents of TEXT.  START and END bound the
   region of rl_line_buffer that contains the word to complete.  TEXT is
   the word to complete.  We can use the entire contents of rl_line_buffer
   in case we want to do some simple parsing.  Return the array of matches,
   or NULL if there aren't any. */
/* FIXME: Handle arguments to the `set' command */
char **
sim_completion (char *text, int start, int end)
{
  char **matches;

  matches = NULL;

  /* If this word is at the start of the line, then it is a command
     to complete.  Otherwise it is the name of a file in the current
     directory. */
  if (!start)
    matches = rl_completion_matches (text, command_generator);

  return matches;
}

/* Generator function for command completion.  STATE lets us know whether
   to start from scratch; without any state (i.e. STATE == 0), then we
   start at the top of the list. */
char *
command_generator (char *text, int state)
{
  static int list_index, len;
  const char *name;

  /* If this is a new word to complete, initialize now.  This includes
     saving the length of TEXT for efficiency, and initializing the index
     variable to 0. */
  if (!state)
    {
      list_index = 0;
      len = strlen (text);
    }

  /* Return the next name which partially matches from the command list. */
  while ((name = sim_commands[list_index].name))
    {
      list_index++;

      if (strncmp (name, text, len) == 0)
	return strdup (name);
    }

  /* If no names matched, then return NULL. */
  return NULL;
}

/* Repeats the last command.  */
char *
repeat_last_command ()
{
  int offset = where_history ();
  HIST_ENTRY *hist;

  if ((hist = history_get (offset)))
    return strdup (hist->line);
  return 0;
}

#endif
