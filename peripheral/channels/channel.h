/* channel.h -- Definition of types and structures for 
   peripheral to communicate with host.  Addapted from UML.
   
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


#ifndef CHANNEL__H
#define CHANNEL__H

/*! A data structure representing all the functions required on a channel */
struct channel_ops
{
  void *(*init) (const char *);
  int   (*open) (void *);
  void  (*close) (void *);
  int   (*read) (void *, char *, int);
  int   (*write) (void *, const char *, int);
  void  (*free) (void *);
  int   (*isok) (void *);
  char *(*status) (void *);
};

/*! A data structure representing a channel. Its operations and data */
struct channel
{
  const struct channel_ops *ops;
  void *data;
};


/* Function prototypes for external use */
extern struct channel *channel_init (const char *descriptor);
extern int             channel_open (struct channel *channel);
extern int             channel_read (struct channel *channel,
				     char           *buffer,
				     int             size);
extern int             channel_write (struct channel *channel,
				      const char     *buffer,
				      int             size);
extern void            channel_close (struct channel *channel);

#endif	/* CHANNEL__H */
