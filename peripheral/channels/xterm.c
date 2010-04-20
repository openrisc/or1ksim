/* xterm.c -- Definition of functions and structures for 
   peripheral to communicate with host through an xterm.
   Inspired from SWI-Prolog by Jan Wielemaker (GPL too)
   even if there is really few in common.
   
   Copyright (C) 2002 Richard Prescott <rip@step.polymtl.ca>
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


/* Autoconf and/or portability configuration */
#include "config.h"
#include "port.h"

/* System includes */
#if defined(HAVE_GRANTPT) && defined(HAVE_UNLOCKPT) && defined(HAVE_PTSNAME)
#endif

#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <termios.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

#if HAVE_SYS_STROPTS_H
#include <sys/stropts.h>	/* I_PUSH ioctl */
#endif

#if HAVE_BASENAME
#include <libgen.h>		/* basename() */
#endif

/* Package includes */
#include "channel.h"
#include "generic.h"
#include "fd.h"

/*! Data structure to represent the connection to the xterm */
struct xterm_channel
{
  struct fd_channel fds;
  int pid;
  char **argv;
};

/* Forward declaration of static functions */
static void  xterm_close (void *data);
static void *xterm_init (const char *input);
static int   xterm_open (void *data);

/*! Global data structure with the xterm interface functions */
struct channel_ops  xterm_channel_ops = {
	.init  = xterm_init,
	.open  = xterm_open,
	.close = xterm_close,
	.read  = fd_read,
	.write = fd_write,
	.free  = generic_free,
};


#if !(HAVE_BASENAME)
static char *
basename (const char *filename)
{
  char *p = strrchr (filename, '/');

  return p ? p + 1 : (char *) filename;
}
#endif /* HAVE_BASENAME */

static void
xterm_close (void *data)
{
  struct xterm_channel *xt = data;

  if (!xt)
    return;

  if (xt->fds.fdin != -1)
    close (xt->fds.fdin);

  if (xt->pid != -1)
    {
      kill (xt->pid, SIGKILL);
      waitpid (xt->pid, NULL, 0);
    }

  if (xt->argv)
    free (xt->argv);

  xt->fds.fdin = -1;
  xt->fds.fdout = -1;
  xt->pid = -1;
  xt->argv = NULL;

}

#if defined(HAVE_ON_EXIT)
static void
xterm_exit (int i, void *data)
{
  xterm_close (data);
}
#endif

#define MAX_XTERM_ARGS 100
static void *
xterm_init (const char *input)
{
  struct xterm_channel *retval = malloc (sizeof (struct xterm_channel));
  if (retval)
    {
      int i;
      char *arglist;

      retval->fds.fdin = -1;
      retval->fds.fdout = -1;
      retval->pid = -1;

#if defined(HAVE_ON_EXIT)
      /* reset cause exit(1), leaving an xterm opened */
      on_exit (xterm_exit, retval);
#endif

      i = 2;
      arglist = (char *) input;
      retval->argv = malloc (sizeof (char *) * MAX_XTERM_ARGS);
      if (!retval->argv)
	{
	  free (retval);
	  return NULL;
	}
      /* Assume xterm arguments are separated by whitespace */
      while ((retval->argv[i++] = strtok (arglist, " \t\n")))
	{
	  arglist = NULL;
	  if (i == MAX_XTERM_ARGS - 1)
	    {
	      free (retval);
	      return NULL;
	    }
	}

    }
  return (void *) retval;
}



static int
xterm_open (void *data)
{
#if defined(HAVE_GRANTPT) && defined(HAVE_UNLOCKPT) && defined(HAVE_PTSNAME)
  struct xterm_channel *xt = data;
  int master, retval;
  char *slavename;
  struct termios termio;
  char arg[64], *fin;

  if (!data)
    {
      errno = ENODEV;
      return -1;
    }

  master = open ("/dev/ptmx", O_RDWR);

  if (master < 0)
    return -1;

  grantpt (master);
  unlockpt (master);
  slavename = (char *) ptsname (master);

  if (!slavename)
    {
      errno = ENOTTY;
      goto closemastererror;
    }

  xt->fds.fdout = xt->fds.fdin = open (slavename, O_RDWR);
  if (xt->fds.fdout < 0)
    goto closemastererror;

#if HAVE_DECL_I_PUSH
  /* These modules must be pushed onto the stream for some non-Linux and
     non-Cygwin operating systems. */
  retval = ioctl (xt->fds.fdin, I_PUSH, "ptem");
  if (retval < 0)
    goto closeslaveerror;

  retval = ioctl (xt->fds.fdin, I_PUSH, "ldterm");
  if (retval < 0)
    goto closeslaveerror;
#endif

  retval = tcgetattr (xt->fds.fdin, &termio);
  if (retval < 0)
    goto closeslaveerror;
  termio.c_lflag &= ~ECHO;
  retval = tcsetattr (xt->fds.fdin, TCSADRAIN, &termio);
  if (retval < 0)
    goto closeslaveerror;

  xt->pid = fork ();

  if (xt->pid == -1)
    goto closeslaveerror;

  if (xt->pid == 0)
    {
      /* Ctrl-C on sim still kill the xterm, grrr */
      signal (SIGINT, SIG_IGN);

      fin = slavename + strlen (slavename) - 2;
      if (strchr (fin, '/'))
	{
	  sprintf (arg, "-S%s/%d", basename (slavename), master);
	}
      else
	{
	  sprintf (arg, "-S%c%c%d", fin[0], fin[1], master);
	}
      xt->argv[0] = "xterm";
      xt->argv[1] = arg;
      execvp ("xterm", xt->argv);
      if (write (master, "\n", 1) < 0)	/* Don't ignore result */
	{
	  printf ("ERROR: xterm: write failed\n");
	}
      exit (1);
    }

  do
    retval = read (xt->fds.fdin, &arg, 1);
  while (retval >= 0 && arg[0] != '\n');
  if (retval < 0)
    goto closeslaveerror;

  termio.c_lflag |= ECHO;
  retval = tcsetattr (xt->fds.fdin, TCSADRAIN, &termio);

  if (retval < 0)
    goto closeslaveerror;

  return 0;

closeslaveerror:
  close (xt->fds.fdin);

closemastererror:
  close (master);
  xt->pid = xt->fds.fdin = xt->fds.fdout = -1;
  return -1;

#else
  /* I don't see how this stuff should be working on a system that doesn't know
     grantpt(), unlockpt(), ptsname(). Mac OS X also does not have /dev/ptmx.
     -hpanther
   */
  return -1;
#endif
}
