/* tty.c -- Definition of functions for peripheral to 
 * communicate with host via a tty.
   
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
#include <unistd.h>
#include <termios.h>
#include <stdio.h>
#include <fcntl.h>

/* Package includes */
#include "channel.h"
#include "generic.h"
#include "fd.h"

/* Default parameters if not specified in config file */
#define DEFAULT_BAUD        B19200
#define DEFAULT_TTY_DEVICE  "/dev/ttyS0"

/*! Data structure representing a TTY channel */
struct tty_channel
{
  struct fd_channel fds;
};

/*! Table of Baud rates */
static const struct
{
  char *name;
  int value;
} baud_table[] =
{
  {
  "50", B50},
  {
  "2400", B2400},
  {
  "4800", B4800},
  {
  "9600", B9600},
  {
  "19200", B19200},
  {
  "38400", B38400},
  {
  "115200", B115200},
  {
  "230400", B230400},
  {
  0, 0}
};

/* Forward declaration of static functions */
static void *tty_init (const char *input);
static int   tty_open (void *data);

/*! Global data structure representing the operations on a TTY channel */
struct channel_ops tty_channel_ops = {
	.init  = tty_init,
	.open  = tty_open,
	.close = generic_close,
	.read  = fd_read,
	.write = fd_write,
	.free  = generic_free,
};


/* Convert baud rate string to termio baud rate constant */
static int
parse_baud (char *baud_string)
{
  int i;
  for (i = 0; baud_table[i].name; i++)
    {
      if (!strcmp (baud_table[i].name, baud_string))
	return baud_table[i].value;
    }

  fprintf (stderr, "Error: unknown baud rate: %s\n", baud_string);
  fprintf (stderr, "       Known baud rates: ");

  for (i = 0; baud_table[i].name; i++)
    {
      fprintf (stderr, "%s%s", baud_table[i].name,
	       baud_table[i + 1].name ? ", " : "\n");
    }
  return B0;
}

static void *
tty_init (const char *input)
{
  int fd = 0, baud;
  char *param_name, *param_value, *device;
  struct termios options;
  struct tty_channel *channel;

  channel = (struct tty_channel *) malloc (sizeof (struct tty_channel));
  if (!channel)
    return NULL;

  /* Make a copy of config string, because we're about to mutate it */
  input = strdup (input);
  if (!input)
    goto error;

  baud = DEFAULT_BAUD;
  device = DEFAULT_TTY_DEVICE;

  /* Parse command-line parameters
     Command line looks like name1=value1,name2,name3=value3,... */
  while ((param_name = strtok ((char *) input, ",")))
    {

      input = NULL;

      /* Parse a parameter's name and value */
      param_value = strchr (param_name, '=');
      if (param_value != NULL)
	{
	  *param_value = '\0';
	  param_value++;	/* Advance past '=' character */
	}

      if (!strcmp (param_name, "baud") && param_value)
	{
	  baud = parse_baud (param_value);
	  if (baud == B0)
	    {
	      goto error;
	    }
	}
      else if (!strcmp (param_name, "device"))
	{
	  device = param_value;
	}
      else
	{
	  fprintf (stderr, "error: unknown tty channel parameter \"%s\"\n",
		   param_name);
	  goto error;
	}
    }

  fd = open (device, O_RDWR);
  if (fd < 0)
    goto error;

  /* Get the current options for the port... */
  if (tcgetattr (fd, &options) < 0)
    goto error;

  /* Set the serial baud rate */
  cfsetispeed (&options, baud);
  cfsetospeed (&options, baud);

  /* Enable the receiver and set local mode... */

  /* cfmakeraw(&options);
   * 
   * cygwin lacks cfmakeraw(), just do it explicitly
   */
  options.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP
		       | INLCR | IGNCR | ICRNL | IXON);
  options.c_oflag &= ~OPOST;
  options.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
  options.c_cflag &= ~(CSIZE | PARENB);
  options.c_cflag |= CS8;

  options.c_cflag |= (CLOCAL | CREAD);


  /* Set the new options for the port... */
  if (tcsetattr (fd, TCSANOW, &options) < 0)
    goto error;

  channel->fds.fdin = channel->fds.fdout = fd;
  free ((void *) input);
  return channel;

error:
  if (fd > 0)
    close (fd);
  free (channel);
  if (input)
    free ((void *) input);
  return NULL;
}

static int
tty_open (void *data)
{
  return 0;
}
