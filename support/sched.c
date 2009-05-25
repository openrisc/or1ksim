/* sched.c -- Abstract entities, handling job scheduling

   Copyright (C) 2001 Marko Mlinar, markom@opencores.org
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
#include <stdio.h>
#include <limits.h>

/* Package includes */
#include "sched.h"
#include "debug.h"
#include "sim-config.h"


DECLARE_DEBUG_CHANNEL(sched_jobs);

#define SCHED_HEAP_SIZE 128
#define SCHED_TIME_MAX  INT32_MAX

/* FIXME: Scheduler should continue from previous cycles not current ones */

struct scheduler_struct scheduler;

/* Dummy function, representing a guard, which protects heap from
   emptying */
void sched_guard (void *dat)
{
  if(scheduler.job_queue)
    SCHED_ADD(sched_guard, dat, SCHED_TIME_MAX);
  else {
    scheduler.job_queue = scheduler.free_job_queue;
    scheduler.free_job_queue = scheduler.free_job_queue->next;
    scheduler.job_queue->next = NULL;
    scheduler.job_queue->func = sched_guard;
    scheduler.job_queue->time = SCHED_TIME_MAX;
  }
}

void sched_reset(void)
{
  struct sched_entry *cur, *next;

  for(cur = scheduler.job_queue; cur; cur = next) {
    next = cur->next;
    cur->next = scheduler.free_job_queue;
    scheduler.free_job_queue = cur;
  }
  scheduler.job_queue = NULL;
  sched_guard(NULL);
}

void sched_init(void)
{
  int i;
  struct sched_entry *new;

  scheduler.free_job_queue = NULL;

  for(i = 0; i < SCHED_HEAP_SIZE; i++) {
    if(!(new = malloc(sizeof(struct sched_entry)))) {
      fprintf(stderr, "Out-of-memory while allocateing scheduler queue\n");
      exit(1);
    }
    new->next = scheduler.free_job_queue;
    scheduler.free_job_queue = new;
  }
  scheduler.job_queue = NULL;
  sched_guard(NULL);
}

/* Executes jobs in time queue */ 
void do_scheduler(void)
{
  struct sched_entry *tmp;

  /* Execute all jobs till now */
  do {  
    tmp = scheduler.job_queue;
    scheduler.job_queue = tmp->next;
    tmp->next = scheduler.free_job_queue;
    scheduler.free_job_queue = tmp;

    scheduler.job_queue->time += tmp->time;

    tmp->func (tmp->param);
  } while(scheduler.job_queue->time <= 0);
}

/* Adds new job to the queue */
void sched_add(void (*job_func)(void *), void *job_param, int32_t job_time,
               const char *func)
{
  struct sched_entry *cur, *prev, *new_job;
  int32_t alltime;

  cur = scheduler.job_queue;
  prev = NULL;
  alltime = cur->time;
  while(cur && (alltime < job_time)) {
    prev = cur;
    cur = cur->next;
    if(cur)
      alltime += cur->time;
  }

  new_job = scheduler.free_job_queue;
  scheduler.free_job_queue = new_job->next;
  new_job->next = cur;

  new_job->func = job_func;
  new_job->param = job_param;

  if(prev) {
    new_job->time = job_time - (alltime - (cur ? cur->time : 0));
    prev->next = new_job;
  } else {
    scheduler.job_queue = new_job;
    new_job->time = job_time >= 0 ? job_time : cur->time;
  }

  if(cur)
    cur->time -= new_job->time;

}

/* Returns a job with specified function and param, NULL if not found */
void sched_find_remove(void (*job_func)(void *), void *dat)
{
  struct sched_entry *cur;
  struct sched_entry *prev = NULL;

  for (cur = scheduler.job_queue; cur; prev = cur, cur = cur->next) {
    if ((cur->func == job_func) && (cur->param == dat)) {
      if(cur->next)
        cur->next->time += cur->time;

      if(prev)
        prev->next = cur->next;
      else
        scheduler.job_queue = cur->next;
      cur->next = scheduler.free_job_queue;
      scheduler.free_job_queue = cur;
      break;
    }
  }
}

/* Schedules the next job so that it will run after the next instruction */
void sched_next_insn(void (*func)(void *), void *dat)
{
  int32_t cycles = 1;
  struct sched_entry *cur = scheduler.job_queue;

  /* The cycles count of the jobs may go into negatives.  If this happens, func
   * will get called before the next instruction has executed. */
  while(cur && (cur->time < 0)) {
    cycles -= cur->time;
    cur = cur->next;
  }

  SCHED_ADD(func, dat, cycles);
}


