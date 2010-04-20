/* uos.h.  Microkernel header for Or1ksim
	
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

/* Length of the IPC message's useful data. */
#define MAX_MSGLEN 100

/* Number of user tasks in the system. */
#define MAX_TASKS 8

/* Number of IPC messages in the system. */
#define MAX_MSGS 16

/* Number of general purpose registers (not counting r0 and r1). */
#define GPRS 30

/* Size of kernel and user task stacks. Size for individual task. */
#define STACK_SIZE 2048 

/* Define this if you want kernel debug output. Note that you must
   assemble except_or32.S with KERNEL_OUTPUT defined in Makefile. This
   definition is only for main uos.c. */
#define KERNEL_OUTPUT 0

/* Define this if you want task switch at every system call. */
#define KERNEL_SYSCALL_SCHED 0

/* System tick timer period */
#define TICK_PERIOD 0x500

/* Task ID type (if we would have processes then we would call it PID) */
typedef int tid_t;

/* System call numbers */
#define IPC_MSGSND 1
#define IPC_MSGRCV 2

/* Message Control Block structure */
struct mcb {
	char msg[MAX_MSGLEN];		/* Message's data */
	int length;			/* Message's length */
	tid_t origin;			/* TID of message's origin task */
	struct mcb *next;		/* Next message in linked list */
};
	
/* Task Control Block structure */
struct tcb {
	struct regs {
		unsigned long pc;		/* Task's PC */
		unsigned long sp;		/* Task's stack (r1)*/
		unsigned long gprs[GPRS];	/* Task's GPRs r2-r15 */
		unsigned long sr;		/* Task's supervision register */
	} regs;
	struct mcb *waiting_msgs;	/* Waiting messages */
};

extern void dispatch();
/* Called by kernel_init to collect all tasks entries. */
extern void tasks_entries();
