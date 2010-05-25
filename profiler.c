/* profiler.c -- profiling utility

   Copyright (C) 2001 Marko Mlinar, markom@opencores.org
   Copyright (C) 2008 Embecosm Limited

   Contributor Jeremy Bennett <jeremy.bennett@embecosm.com>

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

/* Command line utility, that displays profiling information, generated
   by or1ksim. (use profile command interactively, when running or1ksim, or
   separate psim command).  */

#define PROF_DEBUG 0

/* Autoconf and/or portability configuration */
#include "config.h"
#include "port.h"

/* Package includes */
#include "profiler.h"
#include "sim-config.h"
#include "argtable2.h"

/*! Maximum stack frames that can be profiled */
#define MAX_STACK  262144

/*! Data structure representing information about a stack frame */
struct stack_struct
{
  unsigned int  addr;	   /*!< Function address */
  unsigned int  cycles;    /*!< Cycles of func start; subfuncs added later */
  unsigned int  raddr;	   /*!< Return address */
  char          name[33];  /*!< Name of the function */
};

/*! Global: data about functions */
struct func_struct  prof_func[MAX_FUNCS];

/*! Global: total number of functions */
int  prof_nfuncs = 0;

/*! Global: current cycles */
int  prof_cycles = 0;

/*! Representation of the stack */
static struct stack_struct  stack[MAX_STACK];

/*! Current depth */
static int  nstack = 0;

/*! Max depth */
static int  maxstack = 0;

/*! Number of total calls */
static int  ntotcalls = 0;

/*! Number of covered calls */
static int  nfunccalls = 0;

/*! Whether we are in cumulative mode */
static int  cumulative = 0;

/*! Whether we should not report warnings */
static int  quiet = 0;

/*! File to read from */
static FILE *fprof = 0;


/*---------------------------------------------------------------------------*/
/*! Acquire data from profiler file

  @param[in] fprofname   Data file to analyse

  @return  0 on success, return code otherwise                               */
/*---------------------------------------------------------------------------*/
int
prof_acquire (const char *fprofname)
{
  int line = 0;
  int reopened = 0;

  if (runtime.sim.fprof)
    {
      fprof = runtime.sim.fprof;
      reopened = 1;
      if (PROF_DEBUG) printf("reopened=1\n");
      rewind (fprof);
    }
  else
    fprof = fopen (fprofname, "rt");

  if (!fprof)
    {
      fprintf (stderr, "Cannot open profile file: %s\n", fprofname);
      return 1;
    }
  int ctr =0;
  while (1)
    {
      if (PROF_DEBUG) printf("%d ",ctr++);
      char dir = fgetc (fprof);
      line++;
      if (dir == '+')
	{
	  if (fscanf
	      (fprof, "%08X %08X %08X %s\n", &stack[nstack].cycles,
	       &stack[nstack].raddr, &stack[nstack].addr,
	       &stack[nstack].name[0]) != 4)
	    fprintf (stderr, "Error reading line #%i\n", line);
	  else
	    {
	      prof_cycles = stack[nstack].cycles;
	      nstack++;
	      if (PROF_DEBUG) printf("+ 0x%.8x nstack %d\n",stack[nstack-1].raddr, nstack);
	      if (nstack > maxstack)
		maxstack = nstack;
	    }
	  ntotcalls++;
	}
      else if (dir == '-')
	{
	  struct stack_struct s;
	  if (fscanf (fprof, "%08X %08X\n", &s.cycles, &s.raddr) != 2)
	    fprintf (stderr, "Error reading line #%i\n", line);
	  else
	    {
	      int i;
	      prof_cycles = s.cycles;
	      if (PROF_DEBUG) printf("- 0x%.8x nstack %d\n",s.raddr ,nstack);
	      for (i = nstack - 1; i >= 0; i--)
		if (stack[i].raddr == s.raddr)
		  break;
	      if (i >= 0)
		{
		  /* pop everything above current from stack,
		     if more than one, something went wrong */
		  while (nstack > i)
		    {
		      int j;
		      long time;
		      nstack--;
		      time = s.cycles - stack[nstack].cycles;
		      if (!quiet && time < 0)
			{
			  fprintf (stderr,
				   "WARNING: Negative time at %s (return addr = %08X).\n",
				   stack[i].name, stack[i].raddr);
			  time = 0;
			}

		      /* Whether in normal mode, we must substract called function from execution time.  */
		      if (!cumulative)
			for (j = 0; j < nstack; j++)
			  stack[j].cycles += time;

		      if (!quiet && i != nstack)
			fprintf (stderr,
				 "WARNING: Missaligned return call for %s (%08X) (found %s @ %08X), closing.\n",
				 stack[nstack].name, stack[nstack].raddr,
				 stack[i].name, stack[i].raddr);

		      for (j = 0; j < prof_nfuncs; j++)
			if (stack[nstack].addr == prof_func[j].addr)
			  {	/* function exists, append. */
			    prof_func[j].cum_cycles += time;
			    prof_func[j].calls++;
			    nfunccalls++;
			    break;
			  }
		      if (j >= prof_nfuncs)
			{	/* function does not yet exist, create new. */
			  prof_func[prof_nfuncs].cum_cycles = time;
			  prof_func[prof_nfuncs].calls = 1;
			  nfunccalls++;
			  prof_func[prof_nfuncs].addr = stack[nstack].addr;
			  strcpy (prof_func[prof_nfuncs].name,
				  stack[nstack].name);
			  prof_nfuncs++;
			}
		    }
		}
	      else if (!quiet)
		fprintf (stderr,
			 "WARNING: Cannot find return call for (%08X), ignoring.\n",
			 s.raddr);
	    }
	}
      else
	break;
    }

  /* If we have reopened the file, we need to add end of "[outside functions]" */
  if (reopened)
    {      
      prof_cycles = runtime.sim.cycles;
      /* pop everything above current from stack,
         if more than one, something went wrong */
      while (nstack > 0)
	{
	  int j;
	  long time;
	  nstack--;
	  time = runtime.sim.cycles - stack[nstack].cycles;
	  /* Whether in normal mode, we must substract called function from execution time.  */
	  if (!cumulative)
	    for (j = 0; j < nstack; j++)
	      stack[j].cycles += time;

	  for (j = 0; j < prof_nfuncs; j++)
	    if (stack[nstack].addr == prof_func[j].addr)
	      {			/* function exists, append. */
		prof_func[j].cum_cycles += time;
		prof_func[j].calls++;
		nfunccalls++;
		break;
	      }
	  if (j >= prof_nfuncs)
	    {			/* function does not yet exist, create new. */
	      prof_func[prof_nfuncs].cum_cycles = time;
	      prof_func[prof_nfuncs].calls = 1;
	      nfunccalls++;
	      prof_func[prof_nfuncs].addr = stack[nstack].addr;
	      strcpy (prof_func[prof_nfuncs].name, stack[nstack].name);
	      prof_nfuncs++;
	    }
	}
    }
  else
    fclose (fprof);
  return 0;
}

/* Print out profiling data */
static void
prof_print ()
{
  int i, j;
  if (cumulative)
    PRINTF ("CUMULATIVE TIMES\n");
  PRINTF
    ("---------------------------------------------------------------------------\n");
  PRINTF
    ("|function name            |addr    |# calls |avg cycles  |total cyles     |\n");
  PRINTF
    ("|-------------------------+--------+--------+------------+----------------|\n");
  for (j = 0; j < prof_nfuncs; j++)
    {
      int bestcyc = 0, besti = 0;
      for (i = 0; i < prof_nfuncs; i++)
	if (prof_func[i].cum_cycles > bestcyc)
	  {
	    bestcyc = prof_func[i].cum_cycles;
	    besti = i;
	  }
      i = besti;
      PRINTF ("| %-24s|%08X|%8li|%12.1f|%11li,%3.0f%%|\n",
	      prof_func[i].name, prof_func[i].addr, prof_func[i].calls,
	      ((double) prof_func[i].cum_cycles / prof_func[i].calls),
	      prof_func[i].cum_cycles,
	      (100. * prof_func[i].cum_cycles / prof_cycles));
      prof_func[i].cum_cycles = -1;
    }
  PRINTF
    ("---------------------------------------------------------------------------\n");
  PRINTF ("Total %i functions, %i cycles.\n", prof_nfuncs, prof_cycles);
  PRINTF ("Total function calls %i/%i (max depth %i).\n", nfunccalls,
	  ntotcalls, maxstack);
}

/* Set options */
void
prof_set (int _quiet, int _cumulative)
{
  quiet = _quiet;
  cumulative = _cumulative;
}

/*---------------------------------------------------------------------------*/
/*! Parse the arguments for the profiling utility

    Updated by Jeremy Bennett to use argtable2. Also has an option just to
    print help, for use with the CLI.

    @param[in] argc       Number of command args
    @param[in] argv       Vector of the command args
    @param[in] just_help  If 1 (true), ignore argc & argv and just print out
                          the help message without parsing args

    @return  0 on success, 1 on failure                                      */
/*---------------------------------------------------------------------------*/
int
main_profiler (int argc, char *argv[], int just_help)
{
  struct arg_lit *vercop;
  struct arg_lit *help;
  struct arg_lit *cum_arg;
  struct arg_lit *quiet_arg;
  struct arg_file *gen_file;
  struct arg_end *end;

  void *argtab[6];
  int nerrors;

  /* Specify each argument, with fallback values */
  vercop = arg_lit0 ("v", "version", "version and copyright notice");
  help = arg_lit0 ("h", "help", "print this help message");
  cum_arg = arg_lit0 ("c", "cumulative",
		      "cumulative sum of cycles in functions");
  quiet_arg = arg_lit0 ("q", "quiet", "suppress messages");
  gen_file = arg_file0 ("g", "generate", "<file>",
			"data file to analyse (default " "sim.profile)");
  gen_file->filename[0] = "sim.profile";
  end = arg_end (20);

  /* Set up the argument table */
  argtab[0] = vercop;
  argtab[1] = help;
  argtab[2] = cum_arg;
  argtab[3] = quiet_arg;
  argtab[4] = gen_file;
  argtab[5] = end;

  /* If we are just asked for a help message, then we don't parse the
     args. This is used to implement the help function from the CLI. */
  if (just_help)
    {
      printf ("profile");
      arg_print_syntax (stdout, argtab, "\n");
      arg_print_glossary (stdout, argtab, "  %-25s %s\n");

      arg_freetable (argtab, sizeof (argtab) / sizeof (argtab[0]));
      return 0;
    }

  /* Parse */
  nerrors = arg_parse (argc, argv, argtab);

  /* Special case here is if help or version is specified, we ignore any other
     errors and just print the help or version information and then give up. */
  if (vercop->count > 0)
    {
      PRINTF ("OpenRISC 1000 Profiling Utility, version %s\n",
	      PACKAGE_VERSION);

      arg_freetable (argtab, sizeof (argtab) / sizeof (argtab[0]));
      return 0;
    }

  if (help->count > 0)
    {
      printf ("Usage: %s ", argv[0]);
      arg_print_syntax (stdout, argtab, "\n");
      arg_print_glossary (stdout, argtab, "  %-25s %s\n");

      arg_freetable (argtab, sizeof (argtab) / sizeof (argtab[0]));
      return 0;
    }

  /* Deal with any errors */
  if (0 != nerrors)
    {
      arg_print_errors (stderr, end, "profile");
      fprintf (stderr, "Usage: %s ", argv[0]);
      arg_print_syntaxv (stderr, argtab, "\n");

      arg_freetable (argtab, sizeof (argtab) / sizeof (argtab[0]));
      return 1;
    }

  /* Cumulative result wanted? */
  cumulative = cum_arg->count;

  /* Suppress messages? */
  quiet = quiet_arg->count;

  /* Get the profile from the file */
  prof_acquire (gen_file->filename[0]);

  /* Now we have all data acquired. Print out. */
  prof_print ();

  arg_freetable (argtab, sizeof (argtab) / sizeof (argtab[0]));
  return 0;

}				/* main_profiler() */
