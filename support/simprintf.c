/* simprintf.c -- Simulator printf implementation

   Copyright (C) 1999 Damjan Lampret, lampret@opencores.org
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

/* Debugger LIBC functions. Working, but VERY, VERY ugly written.  I wrote
   following code when basic simulator started to work and I was desperate to
   use some PRINTFs in my debugged code. And it was also used to get some
   output from Dhrystone MIPS benchmark. */


/* Autoconf and/or portability configuration */
#include "config.h"
#include "port.h"

/* System includes */
#include <stdlib.h>

/* Package includes */
#include "sim-config.h"
#include "arch.h"
#include "debug.h"
#include "abstract.h"
#include "execute.h"


/* Should args be passed on stack for simprintf 
 *
 * FIXME: do not enable this since it causes problems 
 *        in some cases (an example beeing cbasic test
 *        from orp testbench). the problems is in
 *
 *        or1k/support/simprintf.c
 * 
 *        #if STACK_ARGS
 *                arg = eval_mem32(argaddr,&breakpoint);
 *                argaddr += 4;
 *        #else
 *                sprintf(regstr, "r%u", ++argaddr);
 *                arg = evalsim_reg(atoi(regstr));
 *        #endif
 *
 *        the access to memory should be without any 
 *        checks (ie not like or32 application accessed it)
 * 
 */
#define STACK_ARGS 0

/* Length of PRINTF format string */
#define FMTLEN 2000

char fmtstr[FMTLEN];

static char *
simgetstr (oraddr_t stackaddr, unsigned long regparam)
{
  oraddr_t fmtaddr;
  int i;

  fmtaddr = regparam;

  i = 0;
  while (eval_direct8 (fmtaddr, 1, 0) != '\0')
    {
      fmtstr[i++] = eval_direct8 (fmtaddr, 1, 0);
      fmtaddr++;
      if (i == FMTLEN - 1)
	break;
    }
  fmtstr[i] = '\0';

  return fmtstr;
}

void
simprintf (oraddr_t stackaddr, unsigned long regparam)
{
  uint32_t arg;
  oraddr_t argaddr;
  char *fmtstrend;
  char *fmtstrpart = fmtstr;
  int tee_exe_log;

  simgetstr (stackaddr, regparam);

#if STACK_ARGS
  argaddr = stackaddr;
#else
  argaddr = 3;
#endif
  tee_exe_log = (config.sim.exe_log
		 && (config.sim.exe_log_type == EXE_LOG_SOFTWARE
		     || config.sim.exe_log_type == EXE_LOG_SIMPLE)
		 && config.sim.exe_log_start <= runtime.cpu.instructions
		 && (config.sim.exe_log_end <= 0
		     || runtime.cpu.instructions <= config.sim.exe_log_end));

  if (tee_exe_log)
    fprintf (runtime.sim.fexe_log, "SIMPRINTF: ");
  while (strlen (fmtstrpart))
    {
      if ((fmtstrend = strstr (fmtstrpart + 1, "%")))
	*fmtstrend = '\0';
      if (strstr (fmtstrpart, "%"))
	{
	  char *tmp;
	  int string = 0;
#if STACK_ARGS
	  arg = eval_direct32 (argaddr, 1, 0);
	  argaddr += 4;
#else
	  {
	    /* JPB. I can't see how the original code ever worked. It does trash
	       the file pointer by overwriting the end of regstr. In any case
	       why create a string, only to turn it back into an integer! */

	    /* orig:  char regstr[5]; */
	    /* orig:  */
	    /* orig:  sprintf(regstr, "r%"PRIxADDR, ++argaddr); */
	    /* orig:  arg = evalsim_reg(atoi(regstr)); */

	    arg = evalsim_reg (++argaddr);
	  }
#endif
	  tmp = fmtstrpart;
	  if (*tmp == '%')
	    {
	      tmp++;
	      while (*tmp == '-' || (*tmp >= '0' && *tmp <= '9'))
		tmp++;
	      if (*tmp == 's')
		string = 1;
	    }
	  if (string)
	    {
	      int len = 0;
	      char *str;
	      for (; eval_direct8 (arg++, 1, 0); len++);
	      len++;		/* for null char */
	      arg -= len;
	      str = (char *) malloc (len);
	      len = 0;
	      for (; eval_direct8 (arg, 1, 0); len++)
		*(str + len) = eval_direct8 (arg++, 1, 0);
	      *(str + len) = eval_direct8 (arg, 1, 0);	/* null ch */
	      printf (fmtstrpart, str);
	      if (tee_exe_log)
		fprintf (runtime.sim.fexe_log, fmtstrpart, str);
	      free (str);
	    }
	  else
	    {
	      printf (fmtstrpart, arg);
	      if (tee_exe_log)
		fprintf (runtime.sim.fexe_log, fmtstrpart, arg);
	    }
	}
      else
	{
	  printf (fmtstrpart);
	  if (tee_exe_log)
	    fprintf (runtime.sim.fexe_log, fmtstrpart);
	}
      if (!fmtstrend)
	break;
      fmtstrpart = fmtstrend;
      *fmtstrpart = '%';
    }
}
