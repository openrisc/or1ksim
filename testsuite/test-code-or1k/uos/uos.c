/* uos.c.  Microkernel for Or1ksim
	
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
#include "spr-defs.h"
#include "uos.h"
#include "ipc.h"
#include "int.h"

/* External functions prototypes */
int tick_init(unsigned long period, void (* inf)(void));

/* Pointers to contexts used by except_or32.S routines */
unsigned long *task_context;
unsigned long *kernel_context;

/* TCBs for all tasks in the system */
struct tcb tasks[MAX_TASKS+1];

/* Stacks for the tasks (stacks[0] is kernel stack) */
unsigned char stacks[MAX_TASKS+1][STACK_SIZE];

/* MCBs for IPC messages */
struct mcb msgs[MAX_MSGS];

/* Pointer to linked list of free MCBs. */
struct mcb *free_mcbs;

/* TID of the current user task */
tid_t curtask = 0;

/* Statistics */
int kernel_sched_cnt = 0;
int kernel_syscall_cnt = 0;

/* Timestamp via or1ksim (CPU cycle number). */
unsigned long timestamp()
{
	register unsigned long cycles asm("r3");
	asm("l.sys 201");
	return cycles;
}

/* Standard function for filling memory with a constant byte. */
void *memset(void *dst, int c, size_t size)
{
	char *tmp = dst;
	
	for(;tmp && (tmp < (char *)dst + size); tmp++)
		*(char *)tmp = (char)c;
	
	return dst;
}

/* Traverse linked list of MCBs and show individual messages. */
void kernel_show_mcbs(struct mcb *mcb)
{
	for(;mcb; mcb = mcb->next) {
		printf("MCB len=%u origintask=%u ", mcb->length, mcb->origin);
		printf("msg:%s\n", mcb->msg);
	}
}

/* Show all contexts. */
void kernel_show_contexts()
{
	int i;
	tid_t t;
	
	for(t = 1; t <= MAX_TASKS; t++) {
		printf("\ntask TID=%d: PC=0x%x ", t, (unsigned)tasks[t].regs.pc & ~0x3);
		printf("SP(r1)=0x%x ", (unsigned)tasks[t].regs.sp);
		printf("SR[IEE]=%d\n", (unsigned)tasks[t].regs.sr & SPR_SR_IEE);
		printf("SR[TEE]=%d\n", (unsigned)tasks[t].regs.sr & SPR_SR_TEE);
		printf("SR[SM]=%d\n", (unsigned)tasks[t].regs.sr & SPR_SR_SM);
		for(i = 1; i < GPRS; i++) {
			if (i % 4 == 0)
				printf("\n");
			printf("r%d=0x%.8x ", i, (unsigned)tasks[t].regs.gprs[i]);
		}
		printf("\n");
		kernel_show_mcbs(tasks[t].waiting_msgs);
	}
	printf("\n");
}

/* Simple round-robin scheduler that directly calls dispatcher. It is
   called by low level external interrupt exception handler or by
   kernel_syscall if KERNEL_SYSCALL_SCHED is defined. */
void kernel_sched()
{
	if ((++curtask > MAX_TASKS) || !(tasks[curtask].regs.pc & ~0x3))
		curtask = 1;
	task_context = (unsigned long *)&tasks[curtask].regs;

#if KERNEL_OUTPUT
	printf("kernel_sched(): entry number %d, ", ++kernel_sched_cnt);
	printf("dispatching task TID=%d, time %u cycles", curtask, timestamp());

	kernel_show_contexts();
#endif
		
	dispatch();
}

/* System call uos_msgsnd. */
void
uos_msgsnd(tid_t desttask, char *buf, int len)
{
	asm("l.sys 1");
	asm("l.nop");				
}

/* System call uos_msgrcv. */
void
uos_msgrcv(tid_t origintask, char *buf, int len)
{
	asm("l.sys 2");
	asm("l.nop");
}

/* Handles system call uos_msgsnd. */
void kernel_msgsnd(tid_t tid)
{
	struct mcb *mcb;
	struct mcb **dstmq;
	struct tcb *task;
	
	task = &tasks[tid];
	
	/* Sanity checks. */
	
	/* Does destination task exist? */
	if (!task->regs.gprs[1] || (task->regs.gprs[1] > MAX_TASKS)) {
	task->regs.gprs[9] = IPC_ENOTASK;
		return;
	}

	/* Are there any free MCBs? */
	if (!free_mcbs) {
		task->regs.gprs[9] = IPC_EOUTOFMCBS;
		return;
	}

	/* Is message too big to fit into MCB's message buffer? */
	if (task->regs.gprs[3] > MAX_MSGLEN) {
		task->regs.gprs[9] = IPC_ETOOBIG;
		return;
	}
	
	/* OK, send the message. */
	
	/* First, allocate MCB. */
	mcb = free_mcbs;
	free_mcbs = mcb->next;
	
	/* Second, copy message to the MCB. */
	memcpy(mcb->msg, (void *)task->regs.gprs[2], task->regs.gprs[3]);
	mcb->origin = tid;
	mcb->length = task->regs.gprs[3];
	mcb->next = NULL;
	
	/* Insert MCB into destination task's message queue at
	   the end. */
	dstmq = &tasks[task->regs.gprs[1]].waiting_msgs;
	for(;*dstmq;)
		dstmq = &((*dstmq)->next);
	*dstmq = mcb;
	
	task->regs.gprs[9] = IPC_NOERR;
	return;	
}

/* Handles system call uos_msgrcv. */
void kernel_msgrcv(tid_t tid)
{
	struct mcb *curmsg, **linkp;
	struct tcb *task;
	
	task = &tasks[tid];
	
	/* Sanity checks. */
	
	/* Does origin task exist? */
	if (task->regs.gprs[1] > MAX_TASKS) {
		task->regs.gprs[9] = IPC_ENOTASK;
		return;
	}

	/* Are there any messages waiting for reception? */
	if (!task->waiting_msgs) {
		task->regs.gprs[9] = IPC_ENOMSGS;
		return;
	}

	/* OK, receive the message. */
	
	/* Search waiting messages for one coming from origintask. If
	   origintask is zero then grab the first message. */
	curmsg = task->waiting_msgs;
	linkp = &task->waiting_msgs;
	for(;task->regs.gprs[1] && curmsg->next && curmsg->origin != task->regs.gprs[1];) {
		linkp = &curmsg->next;
		curmsg = curmsg->next;
	}

	/* Is receive buffer too small for receiving message? */
	if (task->regs.gprs[3] < curmsg->length) {
		task->regs.gprs[9] = IPC_ETOOBIG;
		return;
	}

	/* Now copy the message from the MCB. */
	memcpy((void *)task->regs.gprs[2], curmsg->msg, task->regs.gprs[3]);
	
	/* Remove MCB from task's waiting queue and place it
	   back into free MCBs queue. */
	*linkp = curmsg->next;
	curmsg->next = free_mcbs;
	free_mcbs = curmsg;
	
	task->regs.gprs[9] = IPC_NOERR;
	return;	
}

/* Handles all uOS system calls. It is called by low level system call
   exception handler. */
void kernel_syscall()
{
	unsigned short syscall_num;

#if KERNEL_OUTPUT
	printf("kernel_syscall(): entry number %d, ", ++kernel_syscall_cnt);
	printf("current TID=%d, time %u cycles", curtask, timestamp());
	
	kernel_show_contexts();
#endif	
	syscall_num = *(unsigned short *)((tasks[curtask].regs.pc & ~0x3) - 6);
	
	switch(syscall_num) {
		case IPC_MSGSND:
			kernel_msgsnd(curtask);
			break;
		case IPC_MSGRCV:
			kernel_msgrcv(curtask);
			break;
		default:
			printf("kernel_syscall(): unknown syscall (%u)\n", syscall_num);
	}

#if KERNEL_SYSCALL_SCHED
	kernel_sched();
#endif
	dispatch();
}

/* Called by reset exception handler to initialize the kernel and start
   rolling first task. */
void
kernel_init()
{
	tid_t t;
	int i;
	
	printf("Initializing kernel:\n");

	printf("  Clearing kernel structures...\n");
	memset(tasks, 0, sizeof(tasks));
	memset(stacks, 0, sizeof(stacks));
	memset(msgs, 0, sizeof(msgs));
	
	printf("  Initializing MCBs... %d MCB(s)\n", MAX_MSGS);
	for(i = 0; i < (MAX_MSGS - 1); i++)
		msgs[i].next = &msgs[i+1];
	free_mcbs = &msgs[0];
	
	printf("  Initializing TCBs... %d user task(s)\n", MAX_TASKS);

	tasks_entries();

	for(t = 0; t <= MAX_TASKS; t++) {
		tasks[t].regs.sp = (unsigned long)stacks[t] + STACK_SIZE - 4;
		/* Disable EXR for kernel context */
		tasks[t].regs.sr |= (t == 0 ? SPR_SR_SM : SPR_SR_TEE | SPR_SR_IEE);
		tasks[t].regs.gprs[1] = t; 
	}

	/* First task runs in seprvisor mode */
	tasks[1].regs.sr |= SPR_SR_SM;

	/* TID=0 is reserved for kernel use */
	kernel_context = (unsigned long *)&tasks[0].regs;

	/* First task to be scheduled is task TID=1 */
	task_context = (unsigned long *)&tasks[1].regs;

	/* Initialize initrrupt controller */
	int_init();

	printf("  Exceptions will be enabled when first task is dispatched.\n");
	printf("Kernel initalized. Starting first user task.\n");

#if KERNEL_SYSCALL_SCHED
	kernel_sched();		/* Lets schedule and dispatch our first task */
#else
	tick_init(TICK_PERIOD, kernel_sched);
	kernel_sched();		/* Lets schedule and dispatch our first task */
#endif	
	/* ... */		/* We never get here */
}

int main ()
{
  kernel_init();
  return 0;
}
