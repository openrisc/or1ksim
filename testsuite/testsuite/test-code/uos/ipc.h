/* ipc.h.  Microkernel IPC header for Or1ksim
	
   Copyright (C) 2000 Damjan Lampret
   Copyright (C) 2010 Embecosm Limited
   
   Contributor Damjan Lampret <lampret@opencores.org>
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
   with this program.  If not, see <http:  www.gnu.org/licenses/>.  */

/* ----------------------------------------------------------------------------
   This code is commented throughout for use with Doxygen.
   --------------------------------------------------------------------------*/

/* This file is part of test microkernel for OpenRISC 1000. */

/* Operation completed sucessfully. */
#define IPC_NOERR 0

/* Can't send any messages due to lack of free MCBs. */
#define IPC_EOUTOFMCBS 1

/* Sending message: No such destination task.
   Receiving message: No such origin task. */
#define IPC_ENOTASK 2

/* Message to big to be sent or receive buffer is to small for
   receiving message. If receiving then try again with bigger buffer. */
#define IPC_ETOOBIG 3

/* No messages waiting to be received. */
#define IPC_ENOMSGS 4

/* Send message in buffer buf of size len to task desttask. */
void uos_msgsnd(tid_t desttask, char *buf, int len);

/* Receive message of max size len from task origintask and put it
   into buffer buf. If origintask is zero then get the first message
   from the message queue. */
void uos_msgrcv(tid_t origintask, char *buf, int len);
