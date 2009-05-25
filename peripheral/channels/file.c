/* file.c -- Definition of functions and structures for 
   peripheral to communicate with host through files
   
   Copyright (C) 2002 Richard Prescott <rip@step.polymtl.ca>
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
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

/* Package includes */
#include "channel.h"
#include "fd.h"

/*! Data structure representing a channel to/from a file */
struct file_channel
{
  struct fd_channel  fds;
  char              *namein;
  char              *nameout;
};


/* Forward declarations of static routines */
static void *file_init (const char *args);
static int   file_open (void *data);
static void  file_close (void *data);
static void  file_free (void *data);

/*! Data structure with all the operations for communicating with a file
    channel */
struct channel_ops  file_channel_ops = {
	.init  = file_init,
	.open  = file_open,
	.close = file_close,
	.read  = fd_read,
	.write = fd_write,
	.free  = file_free,
};


static void *
file_init (const char *args)
{
  struct file_channel *retval;
  char *nameout;

  if (!args)
    {
      errno = EINVAL;
      return NULL;
    }

  retval = (struct file_channel *) calloc (1, sizeof (struct file_channel));

  if (!retval)
    {
      return NULL;
    }

  retval->fds.fdin = -1;
  retval->fds.fdout = -1;

  nameout = strchr (args, ',');

  if (nameout)
    {
      retval->namein = strndup (args, nameout - args);
      retval->nameout = strdup (nameout + 1);
    }
  else
    {
      retval->nameout = retval->namein = strdup (args);
    }

  return (void *) retval;
}

static int
file_open (void *data)
{
  struct file_channel *files = (struct file_channel *) data;

  if (!files)
    {
      errno = ENODEV;
      return -1;
    }

  if (files->namein == files->nameout)
    {
      /* if we have the same name in and out 
       * it cannot (logically) be a regular files.
       * so we wont create one
       */
      files->fds.fdin = files->fds.fdout = open (files->namein, O_RDWR);

      return files->fds.fdin < 0 ? -1 : 0;
    }


  files->fds.fdin = open (files->namein, O_RDONLY | O_CREAT, 0664);

  if (files->fds.fdin < 0)
    return -1;

  files->fds.fdout = open (files->nameout, O_WRONLY | O_CREAT, 0664);

  if (files->fds.fdout < 0)
    {
      close (files->fds.fdout);
      files->fds.fdout = -1;
      return -1;
    }

  return 0;
}

static void
file_close (void *data)
{
  struct file_channel *files = (struct file_channel *) data;

  if (files->fds.fdin != files->fds.fdout)
    close (files->fds.fdin);

  close (files->fds.fdout);

  files->fds.fdin = -1;
  files->fds.fdout = -1;
}

static void
file_free (void *data)
{
  struct file_channel *files = (struct file_channel *) data;

  if (files->namein != files->nameout)
    free (files->namein);

  free (files->nameout);

  free (files);
}
