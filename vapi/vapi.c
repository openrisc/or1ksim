/* vapi.c -- Verification API Interface

   Copyright (C) 2001, Marko Mlinar, markom@opencores.org
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
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/tcp.h>

/* Package includes */
#include "sim-config.h"
#include "vapi.h"


static unsigned int serverIP = 0;

static unsigned int server_fd = 0;
static unsigned int nhandlers = 0;

static int tcp_level = 0;

static struct vapi_handler
{
  int fd;
  unsigned long base_id, num_ids;
  void (*read_func) (unsigned long, unsigned long, void *);
  void *priv_dat;
  struct vapi_handler *next;
  int temp;
} *vapi_handler = NULL;

/* Structure for polling, it is cached, that it doesn't have to be rebuilt each time */
static struct pollfd *fds = NULL;
static int nfds = 0;

/* Rebuilds the fds structures; see fds.  */
void
rebuild_fds ()
{
  struct vapi_handler *t;
  if (fds)
    free (fds);
  fds = (struct pollfd *) malloc (sizeof (struct pollfd) * (nhandlers + 1));
  if (!fds)
    {
      fprintf (stderr, "FATAL: Out of memory.\n");
      exit (1);
    }

  nfds = 0;
  fds[nfds].fd = server_fd;
  fds[nfds].events = POLLIN;
  fds[nfds++].revents = 0;

  for (t = vapi_handler; t; t = t->next)
    {
      if (t->fd)
	{
	  t->temp = nfds;
	  fds[nfds].fd = t->fd;
	  fds[nfds].events = POLLIN;
	  fds[nfds++].revents = 0;
	}
      else
	t->temp = -1;
    }
}

/* Determines whether a certain handler handles an ID */
static int
handler_fits_id (const struct vapi_handler *t, unsigned long id)
{
  return ((id >= t->base_id) && (id < t->base_id + t->num_ids));
}

/* Finds a handler with given ID, return it, NULL if not found.  */
static struct vapi_handler *
find_handler (unsigned long id)
{
  struct vapi_handler *t = vapi_handler;
  while (t && !handler_fits_id (t, id))
    t = t->next;
  return t;
}

/* Adds a handler with given id and returns it.  */
static struct vapi_handler *
add_handler (unsigned long base_id, unsigned long num_ids)
{
  struct vapi_handler **t = &vapi_handler;
  struct vapi_handler *tt;
  while ((*t))
    t = &(*t)->next;
  tt = (struct vapi_handler *) malloc (sizeof (struct vapi_handler));
  tt->next = NULL;
  tt->base_id = base_id;
  tt->num_ids = num_ids;
  tt->read_func = NULL;
  tt->priv_dat = NULL;
  tt->fd = 0;
  (*t) = tt;
  free (fds);
  fds = NULL;
  nhandlers++;
  rebuild_fds ();
  return tt;
}

void
vapi_write_log_file (VAPI_COMMAND command, unsigned long devid,
		     unsigned long data)
{
  if (!runtime.vapi.vapi_file)
    return;
  if (!config.vapi.hide_device_id && devid <= VAPI_MAX_DEVID)
    fprintf (runtime.vapi.vapi_file, "%04lx", devid);
  fprintf (runtime.vapi.vapi_file, "%1x%08lx\n", command, data);
}

static int
vapi_write_stream (int fd, void *buf, int len)
{
  int n;
  char *w_buf = (char *) buf;
  struct pollfd block;

  while (len)
    {
      if ((n = write (fd, w_buf, len)) < 0)
	{
	  switch (errno)
	    {
	    case EWOULDBLOCK:	/* or EAGAIN */
	      /* We've been called on a descriptor marked
	         for nonblocking I/O. We better simulate
	         blocking behavior. */
	      block.fd = fd;
	      block.events = POLLOUT;
	      block.revents = 0;
	      poll (&block, 1, -1);
	      continue;
	    case EINTR:
	      continue;
	    case EPIPE:
	      close (fd);
	      fd = 0;
	      return -1;
	    default:
	      return -1;
	    }
	}
      else
	{
	  len -= n;
	  w_buf += n;
	}
    }
  return 0;
}

static int
vapi_read_stream (int fd, void *buf, int len)
{
  int n;
  char *r_buf = (char *) buf;
  struct pollfd block;

  while (len)
    {
      if ((n = read (fd, r_buf, len)) < 0)
	{
	  switch (errno)
	    {
	    case EWOULDBLOCK:	/* or EAGAIN */
	      /* We've been called on a descriptor marked
	         for nonblocking I/O. We better simulate
	         blocking behavior. */
	      block.fd = fd;
	      block.events = POLLIN;
	      block.revents = 0;
	      poll (&block, 1, -1);
	      continue;
	    case EINTR:
	      continue;
	    default:
	      return -1;
	    }
	}
      else if (n == 0)
	{
	  close (fd);
	  fd = 0;
	  return -1;
	}
      else
	{
	  len -= n;
	  r_buf += n;
	}
    }
  return 0;
}

/* Added by CZ 24/05/01 */
int
get_server_socket (const char *name, const char *proto, int port)
{
  struct servent *service;
  struct protoent *protocol;
  struct sockaddr_in sa;
  struct hostent *hp;
  int sockfd;
  socklen_t len;
  char myname[256];
  int flags;
  char sTemp[256];

  /* First, get the protocol number of TCP */
  if (!(protocol = getprotobyname (proto)))
    {
      sprintf (sTemp, "Unable to load protocol \"%s\"", proto);
      perror (sTemp);
      return 0;
    }
  tcp_level = protocol->p_proto;	/* Save for later */

  /* If we weren't passed a non standard port, get the port
     from the services directory. */
  if (!port)
    {
      if ((service = getservbyname (name, protocol->p_name)))
	port = ntohs (service->s_port);
    }

  /* Create the socket using the TCP protocol */
  if ((sockfd = socket (PF_INET, SOCK_STREAM, protocol->p_proto)) < 0)
    {
      perror ("Unable to create socket");
      return 0;
    }

  flags = 1;
  if (setsockopt
      (sockfd, SOL_SOCKET, SO_REUSEADDR, (const char *) &flags,
       sizeof (int)) < 0)
    {
      sprintf (sTemp, "Can not set SO_REUSEADDR option on socket %d", sockfd);
      perror (sTemp);
      close (sockfd);
      return 0;
    }

  /* The server should also be non blocking. Get the current flags. */
  if (fcntl (sockfd, F_GETFL, &flags) < 0)
    {
      sprintf (sTemp, "Unable to get flags for socket %d", sockfd);
      perror (sTemp);
      close (sockfd);
      return 0;
    }

  /* Set the nonblocking flag */
  if (fcntl (sockfd, F_SETFL, flags | O_NONBLOCK) < 0)
    {
      sprintf (sTemp, "Unable to set flags for socket %d to value 0x%08x",
	       sockfd, flags | O_NONBLOCK);
      perror (sTemp);
      close (sockfd);
      return 0;
    }

  /* Find out what our address is */
  memset (&sa, 0, sizeof (struct sockaddr_in));
  gethostname (myname, sizeof (myname));
  if (!(hp = gethostbyname (myname)))
    {
      perror ("Unable to read hostname");
      close (sockfd);
      return 0;
    }

  /* Bind our socket to the appropriate address */
  sa.sin_family = hp->h_addrtype;
  sa.sin_port = htons (port);
  if (bind (sockfd, (struct sockaddr *) &sa, sizeof (struct sockaddr_in)) < 0)
    {
      sprintf (sTemp, "Unable to bind socket %d to port %d", sockfd, port);
      perror (sTemp);
      close (sockfd);
      return 0;
    }
  serverIP = sa.sin_addr.s_addr;
  len = sizeof (struct sockaddr_in);
  if (getsockname (sockfd, (struct sockaddr *) &sa, &len) < 0)
    {
      sprintf (sTemp, "Unable to get socket information for socket %d",
	       sockfd);
      perror (sTemp);
      close (sockfd);
      return 0;
    }
  runtime.vapi.server_port = ntohs (sa.sin_port);

  /* Set the backlog to 1 connections */
  if (listen (sockfd, 1) < 0)
    {
      sprintf (sTemp, "Unable to set backlog on socket %d to %d", sockfd, 1);
      perror (sTemp);
      close (sockfd);
      return 0;
    }

  return sockfd;
}

static void
server_request ()
{
  struct sockaddr_in sa;
  struct sockaddr *addr = (struct sockaddr *) &sa;
  socklen_t len = sizeof (struct sockaddr_in);
  int fd = accept (server_fd, addr, &len);
  int on_off = 0;		/* Turn off Nagel's algorithm on the socket */
  int flags;
  char sTemp[256];

  if (fd < 0)
    {
      /* This is valid, because a connection could have started,
         and then terminated due to a protocol error or user
         initiation before the accept could take place. */
      if (errno != EWOULDBLOCK && errno != EAGAIN)
	{
	  perror ("accept");
	  close (server_fd);
	  server_fd = 0;
	  runtime.vapi.enabled = 0;
	  serverIP = 0;
	}
      return;
    }

  if (fcntl (fd, F_GETFL, &flags) < 0)
    {
      sprintf (sTemp, "Unable to get flags for vapi socket %d", fd);
      perror (sTemp);
      close (fd);
      return;
    }

  if (fcntl (fd, F_SETFL, flags | O_NONBLOCK) < 0)
    {
      sprintf (sTemp,
	       "Unable to set flags for vapi socket %d to value 0x%08x", fd,
	       flags | O_NONBLOCK);
      perror (sTemp);
      close (fd);
      return;
    }

  if (setsockopt (fd, tcp_level, TCP_NODELAY, &on_off, sizeof (int)) < 0)
    {
      sprintf (sTemp,
	       "Unable to disable Nagel's algorithm for socket %d.\nsetsockopt",
	       fd);
      perror (sTemp);
      close (fd);
      return;
    }

  /* Install new handler */
  {
    unsigned long id;
    struct vapi_handler *t;
    if (vapi_read_stream (fd, &id, sizeof (id)))
      {
	perror ("Cannot get id");
	close (fd);
	return;
      }
    t = find_handler (id);
    if (t)
      {
	if (t->fd)
	  {
	    fprintf (stderr,
		     "WARNING: Test with id %lx already connected. Ignoring.\n",
		     id);
	    close (fd);
	    return;
	  }
	else
	  {
	    t->fd = fd;
	    rebuild_fds ();
	  }
      }
    else
      {
	fprintf (stderr,
		 "WARNING: Test with id %lx not registered. Ignoring.\n", id);
	close (fd);		/* kill the connection */
	return;
      }
    if (config.sim.verbose)
      PRINTF ("\nConnection with test (id %lx) established.\n", id);
  }
}

static int
write_packet (unsigned long id, unsigned long data)
{
  struct vapi_handler *t = find_handler (id);
  if (!t || !t->fd)
    return 1;
  id = htonl (id);
  if (vapi_write_stream (t->fd, &id, sizeof (id)) < 0)
    return 1;
  data = htonl (data);
  if (vapi_write_stream (t->fd, &data, sizeof (data)) < 0)
    return 1;
  return 0;
}

static int
read_packet (int fd, unsigned long *id, unsigned long *data)
{
  if (fd <= 0)
    return 1;
  if (vapi_read_stream (fd, id, sizeof (unsigned long)) < 0)
    return 1;
  *id = ntohl (*id);
  if (vapi_read_stream (fd, data, sizeof (unsigned long)) < 0)
    return 1;
  *data = ntohl (*data);
  return 0;
}

static void
vapi_request (struct vapi_handler *t)
{
  unsigned long id, data;

  if (read_packet (t->fd, &id, &data))
    {
      if (t->fd > 0)
	{
	  perror ("vapi read");
	  close (t->fd);
	  t->fd = 0;
	  rebuild_fds ();
	}
      return;
    }

  vapi_write_log_file (VAPI_COMMAND_REQUEST, id, data);

  /* This packet may be for another handler */
  if (!handler_fits_id (t, id))
    t = find_handler (id);
  if (!t || !t->read_func)
    fprintf (stderr,
	     "WARNING: Received packet for undefined id %08lx, data %08lx\n",
	     id, data);
  else
    t->read_func (id, data, t->priv_dat);
}

void
vapi_check ()
{
  struct vapi_handler *t;

  if (!server_fd || !fds)
    {
      fprintf (stderr, "FATAL: Unable to maintain VAPI server.\n");
      exit (1);
    }

  /* Handle everything in queue. */
  while (1)
    {
      switch (poll (fds, nfds, 0))
	{
	case -1:
	  if (errno == EINTR)
	    continue;
	  perror ("poll");
	  if (server_fd)
	    close (server_fd);
	  runtime.vapi.enabled = 0;
	  serverIP = 0;
	  return;
	case 0:		/* Nothing interesting going on */
	  return;
	default:
	  /* Handle the vapi ports first. */
	  for (t = vapi_handler; t; t = t->next)
	    if (t->temp >= 0 && fds[t->temp].revents)
	      vapi_request (t);

	  if (fds[0].revents)
	    {
	      if (fds[0].revents & POLLIN)
		server_request ();
	      else
		{		/* Error Occurred */
		  fprintf (stderr,
			   "Received flags 0x%08x on server. Shutting down.\n",
			   fds[0].revents);
		  if (server_fd)
		    close (server_fd);
		  server_fd = 0;
		  runtime.vapi.enabled = 0;
		  serverIP = 0;
		}
	    }
	  break;
	}			/* End of switch statement */
    }				/* End of while statement */
}

/* Inits the VAPI, according to sim-config */
int
vapi_init ()
{
  nhandlers = 0;
  vapi_handler = NULL;
  if (!runtime.vapi.enabled)
    return 0;			/* Nothing to do */

  runtime.vapi.server_port = config.vapi.server_port;
  if (!runtime.vapi.server_port)
    {
      fprintf (stderr, "WARNING: server_port = 0, shutting down VAPI\n");
      runtime.vapi.enabled = 0;
      return 1;
    }
  if ((server_fd =
       get_server_socket ("or1ksim", "tcp", runtime.vapi.server_port)))
    PRINTF ("VAPI Server started on port %d\n", runtime.vapi.server_port);
  else
    {
      perror ("Connection");
      return 1;
    }

  rebuild_fds ();

  if ((runtime.vapi.vapi_file = fopen (config.vapi.vapi_fn, "wt+")) == NULL)
    fprintf (stderr, "WARNING: cannot open VAPI log file\n");

  return 0;
}

/* Closes the VAPI */
void
vapi_done ()
{
  int i;
  struct vapi_handler *t = vapi_handler;

  for (i = 0; i < nfds; i++)
    if (fds[i].fd)
      close (fds[i].fd);
  server_fd = 0;
  runtime.vapi.enabled = 0;
  serverIP = 0;
  free (fds);
  fds = 0;
  if (runtime.vapi.vapi_file)
    {
      /* Mark end of simulation */
      vapi_write_log_file (VAPI_COMMAND_END, 0, 0);
      fclose (runtime.vapi.vapi_file);
    }

  while (vapi_handler)
    {
      t = vapi_handler;
      vapi_handler = vapi_handler->next;
      free (t);
    }
}

/* Installs a vapi handler for one VAPI id */
void
vapi_install_handler (unsigned long id,
		      void (*read_func) (unsigned long, unsigned long,
					 void *), void *dat)
{
  vapi_install_multi_handler (id, 1, read_func, dat);
}

/* Installs a vapi handler for many VAPI id */
void
vapi_install_multi_handler (unsigned long base_id, unsigned long num_ids,
			    void (*read_func) (unsigned long, unsigned long,
					       void *), void *dat)
{
  struct vapi_handler *tt;

  if (read_func == NULL)
    {
      struct vapi_handler **t = &vapi_handler;
      while ((*t) && !handler_fits_id (*t, base_id))
	t = &(*t)->next;
      if (!t)
	{
	  fprintf (stderr, "Cannot uninstall VAPI read handler from id %lx\n",
		   base_id);
	  exit (1);
	}
      tt = *t;
      (*t) = (*t)->next;
      free (tt);
      nhandlers--;
    }
  else
    {
      tt = find_handler (base_id);
      if (!tt)
	{
	  tt = add_handler (base_id, num_ids);
	  tt->read_func = read_func;
	  tt->priv_dat = dat;
	}
      else
	{
	  tt->read_func = read_func;
	  tt->priv_dat = dat;
	  rebuild_fds ();
	}
    }
}

/* Returns number of unconnected handles.  */
int
vapi_num_unconnected (int printout)
{
  struct vapi_handler *t = vapi_handler;
  int numu = 0;
  for (; t; t = t->next)
    {
      if (!t->fd)
	{
	  numu++;
	  if (printout)
	    {
	      if (t->num_ids == 1)
		PRINTF (" 0x%lx", t->base_id);
	      else
		PRINTF (" 0x%lx..0x%lx", t->base_id,
			t->base_id + t->num_ids - 1);
	    }
	}
    }
  return numu;
}

/* Sends a packet to specified test */
void
vapi_send (unsigned long id, unsigned long data)
{
  vapi_write_log_file (VAPI_COMMAND_SEND, id, data);
  write_packet (id, data);
}

/*
int main ()
{
  runtime.vapi.enabled = 1;
  config.vapi.server_port = 9999;
  vapi_init ();
  while (1) {
    vapi_check();
    usleep(1);
  }
  vapi_done ();
}*/

/*---------------------------------------------------[ VAPI configuration ]---*/

static void
vapi_enabled (union param_val val, void *dat)
{
  config.vapi.enabled = val.int_val;
}


/*---------------------------------------------------------------------------*/
/*!Set the VAPI server port

   Ensure the value chosen is valid

   @param[in] val  The value to use
   @param[in] dat  The config data structure (not used here)                 */
/*---------------------------------------------------------------------------*/
static void
vapi_server_port (union param_val val, void *dat)
{
  if ((val.int_val < 1) || (val.int_val > 65535))
    {
      fprintf (stderr, "Warning: invalid VAPI port specified: ignored\n");
    }
  else
    {
      config.vapi.server_port = val.int_val;
    }
}	/* vapi_server_port() */


static void
vapi_log_enabled (union param_val val, void *dat)
{
  config.vapi.log_enabled = val.int_val;
}

static void
vapi_hide_device_id (union param_val val, void *dat)
{
  config.vapi.hide_device_id = val.int_val;
}


/*---------------------------------------------------------------------------*/
/*!Set the log file

   Free any existing string.

   @param[in] val  The value to use
   @param[in] dat  The config data structure (not used here)                 */
/*---------------------------------------------------------------------------*/
static void
vapi_log_fn (union param_val  val,
	     void            *dat)
{
  if (NULL != config.vapi.vapi_fn)
    {
      free (config.vapi.vapi_fn);
    }

  config.vapi.vapi_fn = strdup (val.str_val);

}	/* vapi_log_fn() */


void
reg_vapi_sec (void)
{
  struct config_section *sec = reg_config_sec ("VAPI", NULL, NULL);

  reg_config_param (sec, "enabled",        PARAMT_INT, vapi_enabled);
  reg_config_param (sec, "server_port",    PARAMT_INT, vapi_server_port);
  reg_config_param (sec, "log_enabled",    PARAMT_INT, vapi_log_enabled);
  reg_config_param (sec, "hide_device_id", PARAMT_INT, vapi_hide_device_id);
  reg_config_param (sec, "vapi_log_file",  PARAMT_STR, vapi_log_fn);
  reg_config_param (sec, "vapi_log_fn",    PARAMT_STR, vapi_log_fn);
}
