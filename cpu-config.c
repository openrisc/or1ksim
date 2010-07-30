/* cpu-config.c -- CPU configuration

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

/* Broken out from sim-config.c */


/* Autoconf and/or portability configuration */
#include "config.h"

/* System includes */
#include <stdio.h>

/* Package includes */
#include "cpu-config.h"
#include "sim-config.h"
#include "spr-defs.h"
#include "execute.h"


#define WARNING(s) fprintf (stderr, "Warning: config.cpu: %s\n", (s))

/*---------------------------------------------------------------------------*/
/*!Set the CPU version

   Value must be an 8-bit integer. Larger values are truncated with a warning.

   @param[in] val  The value to use
   @param[in] dat  The config data structure (not used here)                 */
/*---------------------------------------------------------------------------*/
static void
cpu_ver (union param_val  val,
	 void            *dat)
{
  if (val.int_val > 0xff)
    {
      WARNING ("CPU version > 8 bits truncated\n");
    }

  cpu_state.sprs[SPR_VR] &= ~SPR_VR_VER;
  cpu_state.sprs[SPR_VR] |= (val.int_val & 0xff) << SPR_VR_VER_OFF;

}	/* cpu_ver() */


/*---------------------------------------------------------------------------*/
/*!Set the CPU configuration

   Value must be an 8-bit integer. Larger values are truncated with a warning.

   @param[in] val  The value to use
   @param[in] dat  The config data structure (not used here)                 */
/*---------------------------------------------------------------------------*/
static void
cpu_cfg (union param_val  val,
	 void            *dat)
{
  if (val.int_val > 0xff)
    {
      WARNING ("CPU configuration > 8 bits truncated\n");
    }

  cpu_state.sprs[SPR_VR] &= ~SPR_VR_CFG;
  cpu_state.sprs[SPR_VR] |= (val.int_val & 0xff) << SPR_VR_CFG_OFF;

}	/* cpu_cfg() */


/*---------------------------------------------------------------------------*/
/*!Set the CPU revision

   Value must be an 6-bit integer. Larger values are truncated with a warning.


   @param[in] val  The value to use
   @param[in] dat  The config data structure (not used here)                 */
/*---------------------------------------------------------------------------*/
static void
cpu_rev (union param_val  val,
	 void            *dat)
{
  if (val.int_val > 0x3f)
    {
      WARNING ("CPU revision > 6 bits truncated\n");
    }

  cpu_state.sprs[SPR_VR] &= ~SPR_VR_REV_OFF ;
  cpu_state.sprs[SPR_VR] |= (val.int_val & 0x3f) << SPR_VR_REV_OFF ;

}	/* cpu_rev() */


static void
cpu_upr (union param_val val, void *dat)
{
  cpu_state.sprs[SPR_UPR] = val.int_val;
}

/*---------------------------------------------------------------------------*/
/*!Set the CPU configuration

   Value must be just the OB32S instruction set bit. Nothing else is currently
   supported. If other values are specified, they will be set, but with a
   warning.


   @param[in] val  The value to use
   @param[in] dat  The config data structure (not used here)                 */
/*---------------------------------------------------------------------------*/
static void
cpu_cfgr (union param_val  val,
	  void            *dat)
{
  if (SPR_CPUCFGR_OB32S != val.int_val)
    {
      WARNING ("CPU configuration: only OB32S currently supported\n");
    }

  cpu_state.sprs[SPR_CPUCFGR] = val.int_val;

}	/* cpu_cfgr() */


/*---------------------------------------------------------------------------*/
/*!Set the CPU supervision register

   Only the lowest 17 bits may be set. The top 4 bits are for context ID's
   (not currently supported), the rest are reserved and should not be set.

   If such values are specified, the value will be set (it has no effect), but
   with a warning.

   @param[in] val  The value to use
   @param[in] dat  The config data structure (not used here)                 */
/*---------------------------------------------------------------------------*/
static void
cpu_sr (union param_val  val,
	void            *dat)
{
  if (0 != (val.int_val & 0xf0000000))
    {
      WARNING ("Supervision Register ContextID not supported: ignored\n");
    }
  else if (val.int_val > 0x1ffff)
    {
      WARNING ("Supervision Register reserved bits set: ignored\n");
    }

  cpu_state.sprs[SPR_SR] = val.int_val;

}	/* cpu_sr() */


static void
cpu_hazards (union param_val val, void *dat)
{
  config.cpu.hazards = val.int_val;
}

static void
cpu_superscalar (union param_val val, void *dat)
{
  config.cpu.superscalar = val.int_val;
}

static void
cpu_dependstats (union param_val val, void *dat)
{
  config.cpu.dependstats = val.int_val;
}

static void
cpu_sbuf_len (union param_val val, void *dat)
{
  if (val.int_val >= MAX_SBUF_LEN)
    {
      config.cpu.sbuf_len = MAX_SBUF_LEN - 1;
      WARNING ("sbuf_len too large; truncated.");
    }
  else if (val.int_val < 0)
    {
      config.cpu.sbuf_len = 0;
      WARNING ("sbuf_len negative; disabled.");
    }
  else
    config.cpu.sbuf_len = val.int_val;
}

static void
cpu_hardfloat (union param_val val, void *dat)
{
  config.cpu.hardfloat = val.int_val;
}
/*---------------------------------------------------------------------------*/
/*!Register the functions to handle a section cpu

   This section does not allocate dynamically a data structure holding its
   config information. It's all in the global config.sim data
   structure. Therefore it does not need a start and end function to
   initialize default values (although it might be clearer to do so). The
   default values are set in init_defconfig().                               */
/*---------------------------------------------------------------------------*/
void
reg_cpu_sec ()
{
  struct config_section *sec = reg_config_sec ("cpu", NULL, NULL);

  reg_config_param (sec, "ver",         PARAMT_INT, cpu_ver);
  reg_config_param (sec, "cfg",         PARAMT_INT, cpu_cfg);
  reg_config_param (sec, "rev",         PARAMT_INT, cpu_rev);
  reg_config_param (sec, "upr",         PARAMT_INT, cpu_upr);
  reg_config_param (sec, "cfgr",        PARAMT_INT, cpu_cfgr);
  reg_config_param (sec, "sr",          PARAMT_INT, cpu_sr);
  reg_config_param (sec, "superscalar", PARAMT_INT, cpu_superscalar);
  reg_config_param (sec, "hazards",     PARAMT_INT, cpu_hazards);
  reg_config_param (sec, "dependstats", PARAMT_INT, cpu_dependstats);
  reg_config_param (sec, "sbuf_len",    PARAMT_INT, cpu_sbuf_len);
  reg_config_param (sec, "hardfloat",   PARAMT_INT, cpu_hardfloat);

}	/* reg_cpu_sec() */
