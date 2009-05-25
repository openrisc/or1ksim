/* channel.c -- Definition of types and structures for 
   peripherals to communicate with host.  Adapted from UML.
   
   Copyright (C) 2002 Richard Prescott <rip@step.polymtl.ca>
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
#include <stdio.h>
#include <errno.h>

/* Package includes */
#include "channel.h"
#include "fd.h"
#include "file.h"
#include "tcp.h"
#include "tty.h"
#include "xterm.h"

struct channel_factory
{
  const char *name;
  const struct channel_ops *ops;
  struct channel_factory *next;
};

static struct channel_factory preloaded[] = {
  {"fd",    &fd_channel_ops,    &preloaded[1]},
  {"file",  &file_channel_ops,  &preloaded[2]},
  {"xterm", &xterm_channel_ops, &preloaded[3]},
  {"tcp",   &tcp_channel_ops,   &preloaded[4]},
  {"tty",   &tty_channel_ops,   NULL}
};

static struct channel_factory *head = &preloaded[0];

/* Forward declaration of static functions */
static struct channel_factory *find_channel_factory (const char *name);

struct channel *
channel_init (const char *descriptor)
{
  struct channel *retval;
  struct channel_factory *current;
  char *args, *name;
  int count;

  if (!descriptor)
    {
      return NULL;
    }

  retval = (struct channel *) calloc (1, sizeof (struct channel));

  if (!retval)
    {
      perror (descriptor);
      exit (1);
    }

  args = strchr (descriptor, ':');

  if (args)
    {
      count = args - descriptor;
      args++;
    }
  else
    {
      count = strlen (descriptor);
    }

  name = (char *) strndup (descriptor, count);

  if (!name)
    {
      perror (name);
      exit (1);
    }

  current = find_channel_factory (name);

  if (!current)
    {
      errno = ENODEV;
      perror (descriptor);
      exit (1);
    }

  retval->ops = current->ops;

  free (name);

  if (!retval->ops)
    {
      errno = ENODEV;
      perror (descriptor);
      exit (1);
    }

  if (retval->ops->init)
    {
      retval->data = (retval->ops->init) (args);

      if (!retval->data)
	{
	  perror (descriptor);
	  exit (1);
	}
    }

  return retval;
}

int
channel_open (struct channel *channel)
{
  if (channel && channel->ops && channel->ops->open)
    {
      return (channel->ops->open) (channel->data);
    }
  errno = ENOSYS;
  return -1;
}

int
channel_read (struct channel *channel, char *buffer, int size)
{
  if (channel && channel->ops && channel->ops->read)
    {
      return (channel->ops->read) (channel->data, buffer, size);
    }
  errno = ENOSYS;
  return -1;
}

int
channel_write (struct channel *channel, const char *buffer, int size)
{
  if (channel && channel->ops && channel->ops->write)
    {
      return (channel->ops->write) (channel->data, buffer, size);
    }
  errno = ENOSYS;
  return -1;
}

void
channel_close (struct channel *channel)
{
  if (channel && channel->ops && channel->ops->close)
    {
      (channel->ops->close) (channel->data);
    }
}

static struct channel_factory *
find_channel_factory (const char *name)
{
  struct channel_factory *current = head;

  current = head;
  while (current && strcmp (current->name, name))
    {
      current = current->next;
    }

  return current;
}
