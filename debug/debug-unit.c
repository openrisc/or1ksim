/* debug_unit.c -- Simulation of Or1k debug unit

   Copyright (C) 2001 Chris Ziomkowski, chris@asics.ws
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

/* This is an architectural level simulation of the Or1k debug unit as
   described in OpenRISC 1000 System Architecture Manual, v. 0.1 on 22 April,
   2001. This unit is described in Section 13.

   Every attempt has been made to be as accurate as possible with respect to
   the registers and the behavior. There are no known limitations at this
   time.

   Note in particular that there is an alternative (smaller) debug unit on the
   OpenCores website, designed by Igor Mohor. At present this interface is NOT
   supported here. */

/* Autoconf and/or portability configuration */
#include "config.h"
#include "port.h"

/* System includes */
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

/* Package includes */
#include "arch.h"
#include "debug-unit.h"
#include "sim-config.h"
#include "except.h"
#include "abstract.h"
#include "parse.h"
#include "gdb.h"
#include "except.h"
#include "opcode/or32.h"
#include "spr-defs.h"
#include "execute.h"
#include "sprs.h"
#include "toplevel-support.h"
#include "rsp-server.h"


/*! The fields for the RISCOP register in the development interface scan chain
    (JTAG_CHAIN_DEVELOPMENT). */
#define RISCOP_STALL  0x00000001	/*!< Stall processor */
#define RISCOP_RESET  0x00000002	/*!< Reset processor (clears stall) */

/*! The various addresses in the development interface scan chain
    (JTAG_CHAIN_DEVELOPMENT). Only documents the ones we actually have*/
enum development_interface_address_space
{
  DEVELOPINT_RISCOP  =  4,
  DEVELOPINT_MAX     = 27,
};

/*! Data structure holding debug registers and their bits */
unsigned long  development[DEVELOPINT_MAX + 1];

/*! The current scan chain being accessed */
static enum debug_scan_chain_ids current_scan_chain = JTAG_CHAIN_GLOBAL;

/*! External STALL signal to debug interface */
static int in_reset = 0;

/*! Forward declaration of static functions */
static int calculate_watchpoints (enum debug_unit_action action,
				  unsigned long          udata);
static int get_devint_reg (unsigned int   addr,
			   unsigned long *data);
static int set_devint_reg (unsigned int   addr,
			   unsigned long  data);
static int debug_set_mem (oraddr_t address, uorreg_t data);
static int debug_get_mem (oraddr_t address, uorreg_t * data);



/*---------------------------------------------------------------------------*/
/*!Reset the debug unit

   Clear all development inteface registers                                  */
/*---------------------------------------------------------------------------*/
void
du_reset ()
{
  int  i;
  
  for (i = 0; i <= DEVELOPINT_MAX; i++)
    {
      development[i] = 0;
    }

  set_stall_state (0);

}	/* du_reset () */


/*---------------------------------------------------------------------------*/
/*!Set the stall state of the processor

   @param[in] state  If non-zero stall the processor.                        */
/*---------------------------------------------------------------------------*/
void
set_stall_state (int state)
{
#if DYNAMIC_EXECUTION
  if (state)
    {
      PRINTF("FIXME: Emulating a stalled cpu not implemented "
	     "(in the dynamic execution model)\n");
    }
#endif

  development[DEVELOPINT_RISCOP] &= ~RISCOP_STALL;
  development[DEVELOPINT_RISCOP] |= state ? RISCOP_STALL : 0;

  runtime.cpu.stalled             = state;

  /* If we unstall, any changed NPC becomes valid again and we cannot be
     halted. */

  if (!runtime.cpu.stalled)
    {
      cpu_state.npc_not_valid = 0;
      runtime.cpu.halted      = 0;
    }
}	/* set_stall_state () */


/*---------------------------------------------------------------------------*/
/*!Check for a breakpoint on this action

   @note This does not include single-stepping - that will be picked up in the
   main loop AFTER the instruction has executed.

   @param[in] action  The action to be checked
   @param[in] udata   The data to compare against (for some actions)

   @return  Non-zero if there was a breakpoint, 0 otherwise.                 */
/*---------------------------------------------------------------------------*/
int
check_debug_unit (enum debug_unit_action  action,
		  unsigned long           udata)
{
  /* Do not stop if we have debug module disabled or during reset */
  if (!config.debug.enabled || in_reset)
    {
      return 0;
    }

  /* is any watchpoint enabled to generate a break or count? If not, ignore */
  if (cpu_state.sprs[SPR_DMR2] & (SPR_DMR2_WGB | SPR_DMR2_AWTC))
    {
      return  calculate_watchpoints (action, udata);
    }

  return 0;			/* No breakpoint */

}	/* check_debug_unit () */


/*---------------------------------------------------------------------------*/
/*!Check whether we should stall the RISC or cause an exception.

   Rewritten by JPB for current architecture.

   @param[in] action  The action to be checked
   @param[in] udata   The data to compare against (for some actions)

   @return  Non-zero if this should generate a breakpoint                    */
/*---------------------------------------------------------------------------*/
static int
calculate_watchpoints (enum debug_unit_action  action,
		       unsigned long           udata)
{
  int  i;
  int  match_found      = 0;		/* Flag if we found any matchpoint */
  int  breakpoint_found;		/* Flag if we found any breakpoint */

  /* Debug registers */
  unsigned long  dmr1;
  unsigned long  dmr2;

  /* Debug bit fields */
  unsigned char  counter0_enabled;
  unsigned char  counter1_enabled;
  unsigned char  counter0_matched;
  unsigned char  counter1_matched;

  unsigned char  mp[MAX_MATCHPOINTS];	/* Which matchpoints matched */
  unsigned char  wp[MAX_WATCHPOINTS];	/* Which watchpoints matched */

  memset (mp, 0, sizeof (mp));
  memset (wp, 0, sizeof (wp));

  /* First find the matchpoints */

  for (i = 0; i < MAX_MATCHPOINTS; i++)
    {
      unsigned long  dcr    = cpu_state.sprs[SPR_DCR (i)];
      unsigned char  dcr_dp = dcr & SPR_DCR_DP;
      unsigned char  dcr_cc;
      unsigned char  dcr_sc;
      unsigned char  dcr_ct;
      int            match_so_far;

      if (SPR_DCR_DP != dcr_dp)
	{
	  continue;
	}

      dcr_ct       = dcr & SPR_DCR_CT;
      match_so_far = 0;

      switch (dcr_ct)
	{
	case SPR_DCR_CT_IFEA:
	  match_so_far = (DebugInstructionFetch == action);
	  break;

	case SPR_DCR_CT_LEA:
	  match_so_far = (DebugLoadAddress == action);
	  break;

	case SPR_DCR_CT_SEA:
	  match_so_far = (DebugStoreAddress == action);
	  break;

	case SPR_DCR_CT_LD:
	  match_so_far = (DebugLoadData == action);
	  break;

	case SPR_DCR_CT_SD:
	  match_so_far = (DebugStoreData == action);
	  break;

	case SPR_DCR_CT_LSEA:
	  match_so_far = (DebugLoadAddress == action) ||
	    (DebugStoreAddress == action);
	  break;

	case SPR_DCR_CT_LSD:
	  match_so_far = (DebugLoadData == action) ||
	    (DebugStoreData == action);
	  break;

	default:
	  break;
	}

      if (!match_so_far)
	{
	  continue;			/* Skip to the end of the loop */
	}

      dcr_sc = dcr & SPR_DCR_SC;
      dcr_cc = dcr & SPR_DCR_CC;

      /* Perform signed comparison?  */
      if (SPR_DCR_SC == dcr_sc)
	{
	  long int sop1 = udata;
	  long int sop2 = cpu_state.sprs[SPR_DVR (i)];

	  switch (dcr & SPR_DCR_CC)
	    {
	    case SPR_DCR_CC_MASKED:
	      mp[i] = sop1 & sop2;
	      break;

	    case SPR_DCR_CC_EQUAL:
	      mp[i] = sop1 == sop2;
	      break;

	    case SPR_DCR_CC_NEQUAL:
	      mp[i] = sop1 != sop2;
	      break;

	    case SPR_DCR_CC_LESS:
	      mp[i] = sop1 < sop2;
	      break;

	    case SPR_DCR_CC_LESSE:
	      mp[i] = sop1 <= sop2;
	      break;

	    case SPR_DCR_CC_GREAT:
	      mp[i] = sop1 > sop2;
	      break;

	    case SPR_DCR_CC_GREATE:
	      mp[i] = sop1 >= sop2;
	      break;

	    default:
	      break;
	    }
	}
      else
	{
	  unsigned long int op1 = udata;
	  unsigned long int op2 = cpu_state.sprs[SPR_DVR (i)];

	  switch (dcr & SPR_DCR_CC)
	    {
	    case SPR_DCR_CC_MASKED:
	      mp[i] = op1 & op2;
	      break;

	    case SPR_DCR_CC_EQUAL:
	      mp[i] = op1 == op2;
	      break;

	    case SPR_DCR_CC_NEQUAL:
	      mp[i] = op1 != op2;
	      break;

	    case SPR_DCR_CC_LESS:
	      mp[i] = op1 < op2;
	      break;

	    case SPR_DCR_CC_LESSE:
	      mp[i] = op1 <= op2;
	      break;

	    case SPR_DCR_CC_GREAT:
	      mp[i] = op1 > op2;
	      break;

	    case SPR_DCR_CC_GREATE:
	      mp[i] = op1 >= op2;
	      break;

	    default:
	      break;
	    }
	}

      if (mp[i])
	{
	  match_found = 1;	/* A match was found */
	}
    }

  /* If no match was found, give up here, since none of the watchpoints will
     change. */

  if (!match_found)
    {
      return 0;
    }

  /* Compute the non-counting watchpoints. Done by slog, since each one is
     different. The counting watchpoints will be done AFTER the counts have
     been incremented. Done in order, so the chaining works correctly. This
     code expects the number of matchpoints to be 8. As a precaution, that is
     asserted here.

     IMPORTANT.....

     The architecture manual appears to be wrong, in suggesting that
     watchpoint 4 chains with external watchpoint in the same way as
     watchpoint 0. The Verilog source code suggests it chains with watchpoint
     3. */

  assert (MAX_MATCHPOINTS == 8);

  dmr1 = cpu_state.sprs[SPR_DMR1];

  switch (dmr1 & SPR_DMR1_CW0)
    {
    case 0:
      wp[0] = mp[0];
      break;

    case SPR_DMR1_CW0_AND:
      printf ("External watchpoint not supported\n");
      break;

    case SPR_DMR1_CW0_OR:
      printf ("External watchpoint not supported\n");
      break;

    case SPR_DMR1_CW0:
      printf ("SPR DMR1_CW0=11 reserved\n");
      break;
    }

  switch (dmr1 & SPR_DMR1_CW1)
    {
    case 0:
      wp[1] = mp[1];
      break;

    case SPR_DMR1_CW1_AND:
      wp[1] = mp[1] && wp[0];
      break;

    case SPR_DMR1_CW1_OR:
      wp[1] = mp[1] || wp[0];
      break;

    case SPR_DMR1_CW1:
      printf ("SPR DMR1_CW1=11 reserved\n");
      break;
    }

  switch (dmr1 & SPR_DMR1_CW2)
    {
    case 0:
      wp[2] = mp[2];
      break;

    case SPR_DMR1_CW2_AND:
      wp[2] = mp[2] && wp[1];
      break;

    case SPR_DMR1_CW2_OR:
      wp[2] = mp[2] || wp[1];
      break;

    case SPR_DMR1_CW2:
      printf ("SPR DMR1_CW2=11 reserved\n");
      break;
    }

  switch (dmr1 & SPR_DMR1_CW3)
    {
    case 0:
      wp[3] = mp[3];
      break;

    case SPR_DMR1_CW3_AND:
      wp[3] = mp[3] && wp[2];
      break;

    case SPR_DMR1_CW3_OR:
      wp[3] = mp[3] || wp[2];
      break;

    case SPR_DMR1_CW3:
      printf ("SPR DMR1_CW3=11 reserved\n");
      break;
    }

  switch (dmr1 & SPR_DMR1_CW4)
    {
    case 0:
      wp[4] = mp[4];
      break;

    case SPR_DMR1_CW4_AND:
      wp[4] = mp[4] && wp[3];
      break;

    case SPR_DMR1_CW4_OR:
      wp[4] = mp[4] || wp[3];
      break;

    case SPR_DMR1_CW4:
      printf ("SPR DMR1_CW4=11 reserved\n");
      break;
    }

  switch (dmr1 & SPR_DMR1_CW5)
    {
    case 0:
      wp[5] = mp[5];
      break;

    case SPR_DMR1_CW5_AND:
      wp[5] = mp[5] && wp[4];
      break;

    case SPR_DMR1_CW5_OR:
      wp[5] = mp[5] || wp[4];
      break;

    case SPR_DMR1_CW5:
      printf ("SPR DMR1_CW5=11 reserved\n");
      break;
    }

  switch (dmr1 & SPR_DMR1_CW6)
    {
    case 0:
      wp[6] = mp[6];
      break;

    case SPR_DMR1_CW6_AND:
      wp[6] = mp[6] && wp[5];
      break;

    case SPR_DMR1_CW6_OR:
      wp[6] = mp[6] || wp[5];
      break;

    case SPR_DMR1_CW6:
      printf ("SPR DMR1_CW6=11 reserved\n");
      break;
    }

  switch (dmr1 & SPR_DMR1_CW7)
    {
    case 0:
      wp[7] = mp[7];
      break;

    case SPR_DMR1_CW7_AND:
      wp[7] = mp[7] && wp[6];
      break;

    case SPR_DMR1_CW7_OR:
      wp[7] = mp[7] || wp[6];
      break;

    case SPR_DMR1_CW7:
      printf ("SPR DMR1_CW7=11 reserved\n");
      break;
    }

  /* Increment counters. Note the potential ambiguity, if the last two
     watchpoints, which depend on the counters, also increment the
     counters. Since they cannot yet be set, they are not tested here. */

  dmr2             = cpu_state.sprs[SPR_DMR2];

  counter0_enabled = SPR_DMR2_WCE0 == (dmr2 & SPR_DMR2_WCE0);
  counter1_enabled = SPR_DMR2_WCE1 == (dmr2 & SPR_DMR2_WCE1);

  if (counter0_enabled || counter1_enabled)
    {
      short int  counter0 = cpu_state.sprs[SPR_DWCR0] & SPR_DWCR_COUNT;
      short int  counter1 = cpu_state.sprs[SPR_DWCR1] & SPR_DWCR_COUNT;

      for (i = 0; i < MAX_WATCHPOINTS - 2; i++)
	{
	  int  use_counter_0 = (dmr2 >> (SPR_DMR2_AWTC_OFF + i) & 1) != 1;

	  if (use_counter_0)
	    {
	      if (counter0_enabled && wp[i])
		{
		  counter0++;
		}
	    }
	  else
	    {
	      if (counter1_enabled && wp[i])
		{
		  counter1++;
		}
	    }
	}

      cpu_state.sprs[SPR_DWCR0] &= ~SPR_DWCR_COUNT;
      cpu_state.sprs[SPR_DWCR0] |= counter0;
      cpu_state.sprs[SPR_DWCR1] &= ~SPR_DWCR_COUNT;
      cpu_state.sprs[SPR_DWCR1] |= counter1;
    }

  /* Sort out the last two matchpoints, which depend on counters

     IMPORTANT.....

     The architecture manual appears to be wrong, in suggesting that
     watchpoint 8 chains with watchpoint 3 and watchpoint 9 chains with
     watchpoint 7. The Verilog source code suggests watchpoint 8 chains with
     watchpoint 7 and watchpoint 9 chains with watchpoint 8. */

  counter0_matched =
    ((cpu_state.sprs[SPR_DWCR0] & SPR_DWCR_COUNT) ==
     ((cpu_state.sprs[SPR_DWCR0] & SPR_DWCR_MATCH) >> SPR_DWCR_MATCH_OFF));
  counter1_matched =
    ((cpu_state.sprs[SPR_DWCR1] & SPR_DWCR_COUNT) ==
     ((cpu_state.sprs[SPR_DWCR1] & SPR_DWCR_MATCH) >> SPR_DWCR_MATCH_OFF));

  switch (dmr1 & SPR_DMR1_CW8)
    {
    case 0:
      wp[8] = counter0_matched;
      break;

    case SPR_DMR1_CW8_AND:
      wp[8] = counter0_matched && wp[7];
      break;

    case SPR_DMR1_CW8_OR:
      wp[8] = counter0_matched || wp[7];
      break;

    case SPR_DMR1_CW8:
      printf ("SPR DMR1_CW8=11 reserved\n");
      break;
    }

  switch (dmr1 & SPR_DMR1_CW9)
    {
    case 0:
      wp[9] = counter1_matched;
      break;

    case SPR_DMR1_CW9_AND:
      wp[9] = counter1_matched && wp[8];
      break;

    case SPR_DMR1_CW9_OR:
      wp[9] = counter1_matched || wp[8];
      break;

    case SPR_DMR1_CW9:
      printf ("SPR DMR1_CW9=11 reserved\n");
      break;
    }

  /* Now work out which watchpoints (if any) have caused a breakpoint and
     update the breakpoint status bits */

  breakpoint_found = 0;

  for (i = 0; i < MAX_WATCHPOINTS; i++)
    {
      if (1 == (dmr2 >> (SPR_DMR2_WGB_OFF + i) & 1))
	{
	  if (wp[i])
	    {
	      dmr2             |= 1 << (SPR_DMR2_WBS_OFF + i);
	      breakpoint_found  = 1;
	    }
	}
    }

  cpu_state.sprs[SPR_DMR2] = dmr2;

  return breakpoint_found;

}	/* calculate_watchpoints () */


/*---------------------------------------------------------------------------*/
/*!Get a JTAG register

   Action depends on which scan chain is currently active.

   @param[in]  address  Address on the scan chain
   @param[out] data     Where to put the result of the read

   @return  An error code (including ERR_NONE) if there is no error          */
/*---------------------------------------------------------------------------*/
int
debug_get_register (oraddr_t  address,
		    uorreg_t *data)
{
  int  err = ERR_NONE;

  switch (current_scan_chain)
    {
    case JTAG_CHAIN_DEBUG_UNIT:
      *data = mfspr (address);
      break;

    case JTAG_CHAIN_TRACE:
      err = JTAG_PROXY_INVALID_CHAIN;	/* Not yet implemented */
      break;

    case JTAG_CHAIN_DEVELOPMENT:
      err = get_devint_reg (address, (unsigned long *)data);
      break;

    case JTAG_CHAIN_WISHBONE:
      err = debug_get_mem (address, data);
      break;

    default:
      err = JTAG_PROXY_INVALID_CHAIN;
    }

  return  err;

}	/* debug_get_register () */


/*---------------------------------------------------------------------------*/
/*!Set a JTAG register

   Action depends on which scan chain is currently active.

   @param[in]  address  Address on the scan chain
   @param[out] data     Data to set

   @return  An error code (including ERR_NONE) if there is no error          */
/*---------------------------------------------------------------------------*/
int
debug_set_register (oraddr_t  address,
		    uorreg_t  data)
{
  int  err = ERR_NONE;

  switch (current_scan_chain)
    {
    case JTAG_CHAIN_DEBUG_UNIT:
      mtspr (address, data);
      break;

    case JTAG_CHAIN_TRACE:
      err = JTAG_PROXY_ACCESS_EXCEPTION;	/* Not yet implemented */
      break;

    case JTAG_CHAIN_DEVELOPMENT:
      err = set_devint_reg (address, data);
      break;

    case JTAG_CHAIN_WISHBONE:
      err = debug_set_mem (address, data);
      break;

    default:
      err = JTAG_PROXY_INVALID_CHAIN;
    }

  return err;

}	/* debug_set_register () */


/*---------------------------------------------------------------------------*/
/*!Set the JTAG chain

   Only permit chains we support. Currently TRACE is not implemented.

   @param[in] chain  Chain to be set as current

   @return  An error code (including ERR_NONE) if there is no error          */
/*---------------------------------------------------------------------------*/
int
debug_set_chain (enum debug_scan_chain_ids  chain)
{
  switch (chain)
    {
    case JTAG_CHAIN_DEBUG_UNIT:
    case JTAG_CHAIN_DEVELOPMENT:
    case JTAG_CHAIN_WISHBONE:
      current_scan_chain = chain;
      break;

    case JTAG_CHAIN_TRACE:
      return  JTAG_PROXY_INVALID_CHAIN;	/* Not yet implemented */

    default:			
      return  JTAG_PROXY_INVALID_CHAIN;	/* All other chains not implemented */
    }

  return  ERR_NONE;

}	/* debug_set_chain() */


/*---------------------------------------------------------------------------*/
/*!Get a development interface register

   No side effects on get - just return the register

   @param[in]  address  The register to get
   @param[out] data     Where to put the result

   @return  An error code (including ERR_NONE) if there is no error          */
/*---------------------------------------------------------------------------*/
static int
get_devint_reg (enum development_interface_address_space  address,
		unsigned long                            *data)
{
  int  err = ERR_NONE;

  if (address <= DEVELOPINT_MAX)
    {
      *data = development[address];
    }
  else
    {
      err = JTAG_PROXY_INVALID_ADDRESS;
    }

  return  err;

}	/* get_devint_reg () */


/*---------------------------------------------------------------------------*/
/*!Set a development interface register

   Sets the value of the corresponding register. Only RISC_OP has any
   side-effects. The others just store the value, so it can be read back.

   @param[in] address  The register to set
   @param[in] data     The data to set

   @return  An error code (including ERR_NONE) if there is no error          */
/*---------------------------------------------------------------------------*/
static int
set_devint_reg (enum development_interface_address_space  address,
		unsigned long                             data)
{
  int err =  ERR_NONE;

  if (DEVELOPINT_RISCOP == address)
    {
      int old_value = (development[DEVELOPINT_RISCOP] & RISCOP_RESET) != 0;

      development[DEVELOPINT_RISCOP] = data;
      in_reset                       = ((data & RISCOP_RESET) != 0);

      /* Reset the cpu on the negative edge of RESET */
      if (old_value && !in_reset)
	{
	  sim_reset ();		/* Reset all units */
	}

      set_stall_state ((development[DEVELOPINT_RISCOP] & RISCOP_STALL) != 0);
    }
  else if (address <= DEVELOPINT_MAX)
    {
      development[address] = data;
    }
  else
    {
      err = JTAG_PROXY_INVALID_ADDRESS;
    }

  return  err;

}	/* set_devint_reg() */


/*---------------------------------------------------------------------------*/
/*!Read from main bus

   @param[in]  address  Address to read from
   @param[out] data     Where to put the result

   @return  An error code (including ERR_NONE) if there is no error          */
/*---------------------------------------------------------------------------*/
static int
debug_get_mem (oraddr_t  address,
	       uorreg_t *data)
{
  int  err = ERR_NONE;

  if (!verify_memoryarea (address))
    {
    err = JTAG_PROXY_INVALID_ADDRESS;
    }
  else
    {
      *data = eval_direct32 (address, 0, 0);
    }

  return  err;

}	/* debug_get_mem () */


/*---------------------------------------------------------------------------*/
/*!Write to main bus

   @param[in]  address  Address to write to
   @param[out] data     Data to write

   @return  An error code (including ERR_NONE) if there is no error          */
/*---------------------------------------------------------------------------*/
static int
debug_set_mem (oraddr_t  address,
	       uint32_t  data)
{
  int  err = ERR_NONE;

  if (!verify_memoryarea (address))
    {
      err = JTAG_PROXY_INVALID_ADDRESS;
    }
  else
    {
      // circumvent the read-only check usually done for mem accesses
      // data is in host order, because that's what set_direct32 needs
      set_program32 (address, data);
    }

  return  err;

}	/* debug_set_mem () */


/*---------------------------------------------------------------------------*/
/*!See if an exception should be ignored

   @param[in] except  The exception to consider

   @return  Non-zero if the exception should be ignored                      */
/*---------------------------------------------------------------------------*/
int
debug_ignore_exception (unsigned long  except)
{
  int           result = 0;
  unsigned long dsr    = cpu_state.sprs[SPR_DSR];

  switch (except)
    {
    case EXCEPT_RESET:    result = (dsr & SPR_DSR_RSTE);  break;
    case EXCEPT_BUSERR:   result = (dsr & SPR_DSR_BUSEE); break;
    case EXCEPT_DPF:      result = (dsr & SPR_DSR_DPFE);  break;
    case EXCEPT_IPF:      result = (dsr & SPR_DSR_IPFE);  break;
    case EXCEPT_TICK:     result = (dsr & SPR_DSR_TTE);   break;
    case EXCEPT_ALIGN:    result = (dsr & SPR_DSR_AE);    break;
    case EXCEPT_ILLEGAL:  result = (dsr & SPR_DSR_IIE);   break;
    case EXCEPT_INT:      result = (dsr & SPR_DSR_IE);    break;
    case EXCEPT_DTLBMISS: result = (dsr & SPR_DSR_DME);   break;
    case EXCEPT_ITLBMISS: result = (dsr & SPR_DSR_IME);   break;
    case EXCEPT_RANGE:    result = (dsr & SPR_DSR_RE);    break;
    case EXCEPT_SYSCALL:  result = (dsr & SPR_DSR_SCE);   break;
    case EXCEPT_TRAP:     result = (dsr & SPR_DSR_TE);    break;

    default:                                              break;
    }

  cpu_state.sprs[SPR_DRR] |= result;
  set_stall_state (result != 0);

  /* Notify RSP if enabled. TODO: Should we actually notify ALL exceptions,
     not just those maked in the DSR? */

  if (config.debug.rsp_enabled && (0 != result))
    {
      rsp_exception (except);
    }

  return  (result != 0);

}	/* debug_ignore_exception () */


/*---------------------------------------------------------------------------*/
/*!Enable or disable the debug unit

   Set the corresponding field in the UPR

   @param[in] val  The value to use
   @param[in] dat  The config data structure (not used here)                 */
/*---------------------------------------------------------------------------*/
static void
debug_enabled (union param_val val, void *dat)
{
  if (val.int_val)
    {
      cpu_state.sprs[SPR_UPR] |= SPR_UPR_DUP;
    }
  else
    {
      cpu_state.sprs[SPR_UPR] &= ~SPR_UPR_DUP;
    }

  config.debug.enabled = val.int_val;

}	/* debug_enabled() */


/*---------------------------------------------------------------------------*/
/*!Enable or disable Remote Serial Protocol GDB interface to the debug unit

   This is the now the only interface.

   @param[in] val  The value to use
   @param[in] dat  The config data structure (not used here)                 */
/*---------------------------------------------------------------------------*/
static void
debug_rsp_enabled (union param_val val, void *dat)
{
  config.debug.rsp_enabled = val.int_val;

}	/* debug_rsp_enabled () */


/*---------------------------------------------------------------------------*/
/*!Set the Remote Serial Protocol GDB server port

   This is for use with the RSP, which is now the preferred interface.  Ensure
   the value chosen is valid. Note that 0 is permitted, meaning the connection
   should be to the "or1ksim-rsp" service, rather than a port.

   @param[in] val  The value to use
   @param[in] dat  The config data structure (not used here)                 */
/*---------------------------------------------------------------------------*/
static void
debug_rsp_port (union param_val val, void *dat)
{
  if ((val.int_val < 0) || (val.int_val > 65535))
    {
      fprintf (stderr, "Warning: invalid RSP GDB port specified: ignored\n");
    }
  else
    {
      config.debug.rsp_port = val.int_val;
    }
}	/* debug_rsp_port() */


/*---------------------------------------------------------------------------*/
/*!Set the VAPI ID for the debug unit

   @param[in] val  The value to use
   @param[in] dat  The config data structure (not used here)                 */
/*---------------------------------------------------------------------------*/
static void
debug_vapi_id (union param_val val, void *dat)
{
  config.debug.vapi_id = val.int_val;

}	/* debug_vapi_id () */


/*---------------------------------------------------------------------------*/
/*!Register the configuration functions for the debug unit                   */
/*---------------------------------------------------------------------------*/
void
reg_debug_sec ()
{
  struct config_section *sec = reg_config_sec ("debug", NULL, NULL);

  reg_config_param (sec, "enabled",     PARAMT_INT, debug_enabled);
  reg_config_param (sec, "rsp_enabled", PARAMT_INT, debug_rsp_enabled);
  reg_config_param (sec, "rsp_port",    PARAMT_INT, debug_rsp_port);
  reg_config_param (sec, "vapi_id",     PARAMT_INT, debug_vapi_id);

}	/* reg_debug_sec () */

