/* task.c.  Microkernel task handler for Or1ksim
	
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

#include "support.h"
#include "uos.h"
#include "ipc.h"
#include "int.h"

extern struct tcb tasks[MAX_TASKS+1];

int task(int id)
{
	struct _msg {
		char		id;
		unsigned long	count;
	} msg;

	printf("Task %d started\n", id);
	
	if(id == 1) {
		msg.id = 1;
		msg.count = 0;
		uos_msgsnd(2, (char *)&msg, sizeof(msg));
	}
	
	for(;;) {
		uos_msgrcv(0, (char *)&msg, sizeof(msg));

		printf("Task %d: Got massage from task %d: 0x%.8x. "
		       "Sending message to task %d: 0x%.8x \n", id, msg.id,
		       (int)msg.count, (id == 3 ? 1 : (id + 1)),
		       (int)(msg.count  + 1));
		msg.id = id;

		if((id == 1) && (msg.count > 15)) {
		  report(msg.count + 0xdeadde9c);
		  exit(0);
			

			msg.count += 1;
			uos_msgsnd((id == 3 ? 1 : (id + 1)), (char *)&msg, sizeof(msg));
		}
	}
}

/* Called by kernel_init to collect all tasks entries. */
void tasks_entries()
{
	tasks[1].regs.pc = (unsigned long)task;
	tasks[2].regs.pc = (unsigned long)task;
	tasks[3].regs.pc = (unsigned long)task;
}

