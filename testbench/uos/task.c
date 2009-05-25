/* This file is part of test microkernel for OpenRISC 1000. */
/* (C) 2000 Damjan Lampret, lampret@opencores.org */

#include "support.h"
#include "uos.h"
#include "ipc.h"
#include "int.h"

extern struct tcb tasks[MAX_TASKS+1];

int task(int id)
{
	int rc;
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
		rc = uos_msgrcv(0, (char *)&msg, sizeof(msg));

		if(rc != 0) {
			printf("Task %d: Waiting for massage\n", id);
		} else {
			printf("Task %d: Got massage from task %d: 0x%.8x. Sending message to task %d: 0x%.8x \n", id, msg.id, (int)msg.count, (id == 3 ? 1 : (id + 1)), (int)(msg.count  + 1));
			msg.id = id;

			if((id == 1) && (msg.count > 15)) {
				report(msg.count + 0xdeadde9c);
				exit(0);
			}

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

