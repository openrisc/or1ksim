/* This file is part of test microkernel for OpenRISC 1000. */
/* (C) 2000 Damjan Lampret, lampret@opencores.org */

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
int uos_msgsnd(tid_t desttask, char *buf, int len);

/* Receive message of max size len from task origintask and put it
   into buffer buf. If origintask is zero then get the first message
   from the message queue. */
int uos_msgrcv(tid_t origintask, char *buf, int len);
