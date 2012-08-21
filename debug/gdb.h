/* config.h -- Simulator configuration header file

   Copyright (C) 2001 Chris Ziomkowski, chris@asics.ws
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

/* The data in here is copied from or1k-jtag.h in the GDB 6.8 port for
   OpenRISC 1000 */


#ifndef GDB__H
#define GDB__H

/*! Error codes for the OpenRISC 1000 JTAG debugging protocol */
enum or1k_jtag_errors  /* modified <chris@asics.ws> CZ 24/05/01 */
{
  /* Codes > 0 are for system errors */

  ERR_NONE                           =   0,
  ERR_CRC                            =  -1,
  ERR_MEM                            =  -2,
  JTAG_PROXY_INVALID_COMMAND         =  -3,
  JTAG_PROXY_SERVER_TERMINATED       =  -4,
  JTAG_PROXY_NO_CONNECTION           =  -5,
  JTAG_PROXY_PROTOCOL_ERROR          =  -6,
  JTAG_PROXY_COMMAND_NOT_IMPLEMENTED =  -7,
  JTAG_PROXY_INVALID_CHAIN           =  -8,
  JTAG_PROXY_INVALID_ADDRESS         =  -9,
  JTAG_PROXY_ACCESS_EXCEPTION        = -10,	/* Write to ROM */
  JTAG_PROXY_INVALID_LENGTH          = -11,
  JTAG_PROXY_OUT_OF_MEMORY           = -12
};

/*! The OR1K JTAG proxy protocol commands. */
enum or1k_jtag_proxy_protocol_commands {
  OR1K_JTAG_COMMAND_READ        = 1,
  OR1K_JTAG_COMMAND_WRITE       = 2,
  OR1K_JTAG_COMMAND_READ_BLOCK  = 3,
  OR1K_JTAG_COMMAND_WRITE_BLOCK = 4,
  OR1K_JTAG_COMMAND_CHAIN       = 5,
};

/* Each transmit structure must begin with an integer which specifies the type
   of command. Information after this is variable. Make sure to have all
   information aligned properly. If we stick with 32 bit integers, it should
   be portable onto every platform. These structures will be transmitted
   across the network in network byte order. */

struct jtr_read_message {
  uint32_t  command;
  uint32_t  length;
  uint32_t  address;
};

struct jtr_write_message {
  uint32_t  command;
  uint32_t  length;
  uint32_t  address;
  uint32_t  data_h;
  uint32_t  data_l;
};

struct jtr_read_block_message {
  uint32_t  command;
  uint32_t  length;
  uint32_t  address;
  int32_t   num_regs;
};

struct jtr_write_block_message {
  uint32_t  command;
  uint32_t  length;
  uint32_t  address;
  int32_t   num_regs;
  uint32_t  data[1];
};

struct jtr_chain_message {
  uint32_t  command;
  uint32_t  length;
  uint32_t  chain;
};

/* The responses are messages specific, however convention states the first
   word should be an error code. Again, sticking with 32 bit integers should
   provide maximum portability. */

struct jtr_failure_response {
  int32_t  status;
};

struct jtr_read_response {
  int32_t   status;
  uint32_t  data_h;
  uint32_t  data_l;
};
  
struct jtr_write_response {
  int32_t  status;
};

struct jtr_read_block_response {
  int32_t   status;
  int32_t   num_regs;
  uint32_t  data[1];
};

struct jtr_write_block_response {
  int32_t  status;
};

struct jtr_chain_response {
  int32_t  status;
};


#endif	/* GDB__H */
