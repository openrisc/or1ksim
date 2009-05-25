/* fd.h -- Declaration of functions for peripheral to 
 * communicate with host through file descriptors
   
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


#ifndef FD__H
#define FD__H

/*! Data structure to represent a channel through file descriptors */
struct fd_channel
{
  int fdin;
  int fdout;
};

/* Global data structures */
extern struct channel_ops  fd_channel_ops;

/* Function prototypes for external use */
extern int fd_read (void *data,
		    char *buffer,
		    int   size);
extern int fd_write (void       *data,
		     const char *buffer,
		     int         size);

#endif	/* FD__H */

