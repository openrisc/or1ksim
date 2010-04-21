/* mprofiler.c -- memory profiling utility

   Copyright (C) 2002 Marko Mlinar, markom@opencores.org
   Copyright (C) 1999 Damjan Lampret, lampret@opencores.org
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


/* Autoconf and/or portability configuration */
#include "config.h"
#include "port.h"

/* System includes */
#include <stdlib.h>
#include <regex.h>

/* Package includes */
#include "mprofiler.h"
#include "sim-config.h"
#include "argtable2.h"
#include "support/profile.h"

/* output modes */
#define MODE_DETAIL     0
#define MODE_PRETTY     1
#define MODE_ACCESS     2
#define MODE_WIDTH      3

/* Input buffer size */
#define BUF_SIZE        256

/* HASH */
#define HASH_SIZE       0x10000
#define HASH_FUNC(x)    ((x) & 0xffff)

/*! Hash table data structure */
static struct memory_hash
{
  struct memory_hash *next;
  oraddr_t            addr;
  unsigned long       cnt[3];		/* Various counters */
} *hash[HASH_SIZE];

/*! Groups size -- how much addresses should be joined together */
static int  group_bits = 2;

/*! Start address */
static oraddr_t  start_addr = 0;

/*! End address */
static oraddr_t  end_addr = 0xffffffff;

/* File to read from */
static FILE *fprof = 0;

static void
hash_add (oraddr_t addr, int index)
{
  struct memory_hash *h = hash[HASH_FUNC (addr)];
  while (h && h->addr != addr)
    h = h->next;

  if (!h)
    {
      h = (struct memory_hash *) malloc (sizeof (struct memory_hash));
      h->next = hash[HASH_FUNC (addr)];
      hash[HASH_FUNC (addr)] = h;
      h->addr = addr;
      h->cnt[0] = h->cnt[1] = h->cnt[2] = 0;
    }
  h->cnt[index]++;
}


static unsigned long
hash_get (oraddr_t addr, int index)
{
  struct memory_hash *h = hash[HASH_FUNC (addr)];
  while (h && h->addr != addr)
    h = h->next;

  if (!h)
    return 0;
  return h->cnt[index];
}

static void
init ()
{
  int i;
  for (i = 0; i < HASH_SIZE; i++)
    hash[i] = NULL;
}

static void
read_file (FILE * f, int mode)
{
  struct mprofentry_struct buf[BUF_SIZE];
  int num_read;
  do
    {
      int i;
      num_read = fread (buf, sizeof (struct mprofentry_struct), BUF_SIZE, f);
      for (i = 0; i < num_read; i++)
	if (buf[i].addr >= start_addr && buf[i].addr <= end_addr)
	  {
	    int index;
	    unsigned t = buf[i].type;
	    if (t > 64)
	      {
		PRINTF ("!");
		t = 0;
	      }
	    if (mode == MODE_WIDTH)
	      t >>= 3;
	    else
	      t &= 0x7;

	    switch (t)
	      {
	      case 1:
		index = 0;
		break;
	      case 2:
		index = 1;
		break;
	      case 4:
		index = 2;
		break;
	      default:
		index = 0;
		PRINTF ("!!!!");
		break;
	      }
	    hash_add (buf[i].addr >> group_bits, index);
	  }
    }
  while (num_read > 0);
}

static int
nbits (unsigned long a)
{
  int cnt = 0;
  int b = a;
  if (!a)
    return 0;

  while (a)
    a >>= 1, cnt++;
  if (cnt > 1 && ((b >> (cnt - 2)) & 1))
    cnt = cnt * 2 + 1;
  else
    cnt *= 2;

  return cnt - 1;
}

static void
printout (int mode)
{
  oraddr_t addr = start_addr & ~((1 << group_bits) - 1);
  PRINTF ("start = %" PRIxADDR " (%" PRIxADDR "); end = %" PRIxADDR
	  "; group_bits = %08x\n", start_addr, addr, end_addr,
	  (1 << group_bits) - 1);
  for (; addr <= end_addr; addr += (1 << group_bits))
    {
      int i;
      unsigned long a = hash_get (addr >> group_bits, 0);
      unsigned long b = hash_get (addr >> group_bits, 1);
      unsigned long c = hash_get (addr >> group_bits, 2);
      PRINTF ("%" PRIxADDR ":", addr);
      switch (mode)
	{
	case MODE_DETAIL:
	  if (a)
	    PRINTF (" %10li R", a);
	  else
	    PRINTF ("            R");
	  if (b)
	    PRINTF (" %10li W", b);
	  else
	    PRINTF ("            W");
	  if (c)
	    PRINTF (" %10li F", c);
	  else
	    PRINTF ("            F");
	  break;
	case MODE_ACCESS:
	  PRINTF (" %10li", a + b + c);
	  break;
	case MODE_PRETTY:
	  PRINTF (" %10li ", a + b + c);
	  for (i = 0; i < nbits (a + b + c); i++)
	    PRINTF ("#");
	  break;
	case MODE_WIDTH:
	  if (a)
	    PRINTF (" %10li B", a);
	  else
	    PRINTF ("            B");
	  if (b)
	    PRINTF (" %10li H", b);
	  else
	    PRINTF ("            H");
	  if (c)
	    PRINTF (" %10li W", c);
	  else
	    PRINTF ("            W");
	  break;
	}
      PRINTF ("\n");
      if (addr >= addr + (1 << group_bits))
	break;			/* Overflow? */
    }
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
main_mprofiler (int argc, char *argv[], int just_help)
{
  struct arg_lit *vercop;
  struct arg_lit *help;
  struct arg_rex *mode_arg;
  struct arg_int *group;
  struct arg_file *prof_file;
  struct arg_int *from;
  struct arg_int *to;
  struct arg_end *end;

  void *argtab[8];
  int nerrors;

  int mode = MODE_DETAIL;

  /* Specify each argument, with fallback values.

     Bug 1710 (Vinay Patil) fixed. mode should allow REG_EXTENDED. */
  vercop = arg_lit0 ("v", "version", "version and copyright notice");
  help = arg_lit0 ("h", "help", "print this help message");
  mode_arg = arg_rex0 ("m", "mode",
		       "(detailed|d)|(pretty|p)|(access|a)|"
		       "(width|w)", "<m>", REG_ICASE | REG_EXTENDED,
		       "Output mode (detailed, pretty, access " "or width)");
  mode_arg->sval[0] = "detailed";
  group = arg_int0 ("g", "group", "<n>",
		    "group 2^n bits successive addresses " "together");
  group->ival[0] = 0;
  prof_file = arg_file0 ("f", "filename", "<file>",
			 "data file to analyse (default " "sim.mprofile)");
  prof_file->filename[0] = "sim.mprofile";
  from = arg_int1 (NULL, NULL, "<from>", "start address");
  to = arg_int1 (NULL, NULL, "<to>", "end address");
  end = arg_end (20);

  /* Set up the argument table */
  argtab[0] = vercop;
  argtab[1] = help;
  argtab[2] = mode_arg;
  argtab[3] = group;
  argtab[4] = prof_file;
  argtab[5] = from;
  argtab[6] = to;
  argtab[7] = end;

  /* If we are just asked for a help message, then we don't parse the
     args. This is used to implement the help function from the CLI. */
  if (just_help)
    {
      printf ("mprofile");
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
      PRINTF ("OpenRISC 1000 Memory Profiling Utility, version %s\n",
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
      arg_print_errors (stderr, end, "mprofile");
      fprintf (stderr, "Usage: %s ", argv[0]);
      arg_print_syntaxv (stderr, argtab, "\n");

      arg_freetable (argtab, sizeof (argtab) / sizeof (argtab[0]));
      return 1;
    }

  /* If version or help is requested, that is all that is printed out
     Sort out the mode.

     Bug 1710 fixed (Vinay Patil). Modes now set correctly (were all set to
     detail). */
  if ((0 == strcmp (mode_arg->sval[0], "detail")) ||
      (0 == strcmp (mode_arg->sval[0], "d")))
    {
      mode = MODE_DETAIL;
    }
  else if ((0 == strcmp (mode_arg->sval[0], "pretty")) ||
	   (0 == strcmp (mode_arg->sval[0], "p")))
    {
      mode = MODE_PRETTY;
    }
  else if ((0 == strcmp (mode_arg->sval[0], "access")) ||
	   (0 == strcmp (mode_arg->sval[0], "a")))
    {
      mode = MODE_ACCESS;
    }
  else if ((0 == strcmp (mode_arg->sval[0], "width")) ||
	   (0 == strcmp (mode_arg->sval[0], "w")))
    {
      mode = MODE_WIDTH;
    }
  else
    {
      fprintf (stderr, "Impossible mode: %s\n", mode_arg->sval[0]);
    }

  /* Group bits */
  group_bits = group->ival[0];

  /* Start and end addresses */
  start_addr = from->ival[0];
  end_addr = to->ival[0];

  /* Get the profile */
  fprof = fopen (prof_file->filename[0], "rm");

  if (!fprof)
    {
      fprintf (stderr, "Cannot open profile file: %s\n",
	       prof_file->filename[0]);

      arg_freetable (argtab, sizeof (argtab) / sizeof (argtab[0]));
      return 1;
    }

  /* Finished with the args */
  arg_freetable (argtab, sizeof (argtab) / sizeof (argtab[0]));

  init ();
  read_file (fprof, mode);
  fclose (fprof);
  printout (mode);
  return 0;

}	/* main_mprofiler () */
