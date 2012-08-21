/* or1ksim.h -- Simulator library header file

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


/* Header file definining the interface to the Or1ksim library. */


#ifndef OR1KSIM__H
#define OR1KSIM__H


/* The bus width */

/* #define BUSWIDTH  32 */

/* The return codes */

enum  or1ksim_rc {
  OR1KSIM_RC_OK = 0,		/* No error */

  OR1KSIM_RC_BADINIT,		/* Couldn't initialize */
  OR1KSIM_RC_BRKPT,		/* Hit a breakpoint */
  OR1KSIM_RC_HALTED		/* Hit NOP_EXIT */
};

/* The interface methods */

#ifdef __cplusplus
extern "C" {
#endif

int  or1ksim_init (int         argc,
		   char       *argv[],
		   void       *class_ptr,
		   int       (*upr) (void              *class_ptr,
				     unsigned long int  addr,
				     unsigned char      mask[],
				     unsigned char      rdata[],
				     int                data_len),
		   int       (*upw) (void              *class_ptr,
				     unsigned long int  addr,
				     unsigned char      mask[],
				     unsigned char      wdata[],
				     int                data_len));

int  or1ksim_run (double  duration);

void  or1ksim_reset_duration (double duration);

void  or1ksim_set_time_point ();

double  or1ksim_get_time_period ();

int  or1ksim_is_le ();

unsigned long int  or1ksim_clock_rate();

/* Interrupt handling interface */
void  or1ksim_interrupt (int  i);

void or1ksim_interrupt_set (int  i);

void  or1ksim_interrupt_clear (int  i);

/* JTAG interface */
double  or1ksim_jtag_reset ();

double  or1ksim_jtag_shift_ir (unsigned char *jreg,
			       int            num_bits);

double  or1ksim_jtag_shift_dr (unsigned char *jreg,
			       int            num_bits);

/* Access to simulator state */
int  or1ksim_read_mem (unsigned long int  addr,
		       unsigned char     *buf,
		       int                len);

int  or1ksim_write_mem (unsigned long int    addr,
			const unsigned char *buf,
			int                  len);

int  or1ksim_read_spr (int                 sprnum,
		       unsigned long int  *sprval_ptr);

int  or1ksim_write_spr (int                sprnum,
			unsigned long int  sprval);
    
int  or1ksim_read_reg (int                regnum,
		       unsigned long int *regval_ptr);

int  or1ksim_write_reg (int                regnum,
			unsigned long int  regval);

void  or1ksim_set_stall_state (int  state);

#ifdef __cplusplus
}
#endif


#endif	/* OR1KSIM__H */
