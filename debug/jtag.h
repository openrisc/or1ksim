/* jtag.c -- JTAG modeling

   Copyright (C) 2010 Embecosm Limited

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


#ifndef JTAG__H
#define JTAG__H

/*! Enumeration of the JTAG instruction types */
enum  jtag_instr {
  JI_UNDEFINED      =  -1,		/* Won't fit in bitfield */
  JI_EXTEST         = 0x0,
  JI_SAMPLE_PRELOAD = 0x1,
  JI_IDCODE         = 0x2,
  JI_DEBUG          = 0x8,
  JI_MBIST          = 0x9,
  JI_BYPASS         = 0xf
};

/*! Enumeration of the JTAG module IDs */
enum  jtag_mod_id {
  JM_UNDEFINED =  -1,			/* Won't fit in bitfield */
  JM_WISHBONE  = 0x0,
  JM_CPU0      = 0x1,
  JM_CPU1      = 0x2
};

/*! Enumeration of the DEBUG command types */
enum  jtag_cmd {
  JCMD_UNDEFINED     = -1,		/* Won't fit in bitfield */
  JCMD_GO_COMMAND    = 0x0,
  JCMD_READ_COMMAND  = 0x1,
  JCMD_WRITE_COMMAND = 0x2,
  JCMD_READ_CONTROL  = 0x3,
  JCMD_WRITE_CONTROL = 0x4
};

/*! Enumeration of the access types for WRITE_COMMAND */
enum  jtag_acc_type {
  JAT_UNDEFINED  = -1,			/* Won't fit in bitfield */
  JAT_WRITE8     = 0,			/* WishBone only */
  JAT_WRITE16    = 1,			/* WishBone only */
  JAT_WRITE32    = 2,
  JAT_READ8      = 4,			/* WishBone only */
  JAT_READ16     = 5,			/* WishBone only */
  JAT_READ32     = 6
};

/*! Enumeration of the status field bits */
enum jtag_status {
  JS_OK             = 0x0,		/*!< No error */
  JS_CRC_IN_ERROR   = 0x1,		/*!< Supplied CRC error */
  JS_MODULE_MISSING = 0x2,		/*!< Non-existent module select */
  JS_WISHBONE_ERROR = 0x4,		/*!< Problem accessing Wishgone */
  JS_OVER_UNDERRUN  = 0x8		/*!< Over/under-run of data */
};

/*! Enumeration of the control bits */
enum jtag_control_bits {
  JCB_RESET = 51,			/*!< Reset the processor */
  JCB_STALL = 50,			/*!< Stall the processor */
};

/* Function prototypes for external use */
extern void  jtag_init ();
extern void  jtag_reset ();
extern void  jtag_shift_ir (unsigned char *jreg,
			    int            num_bits);
extern void  jtag_shift_dr (unsigned char *jreg,
			    int            num_bits);

#endif	/* JTAG__H */
