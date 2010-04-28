/* kbdtest.c. Test of Or1ksim keyboard

   Copyright (C) 1999-2006 OpenCores
   Copyright (C) 2010 Embecosm Limited

   Contributors various OpenCores participants
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

#include "support.h"
#include "spr-defs.h"
#include "board.h"


/*! Maximum number of characters to read before giving up. */
#define MAX_CHARS_TO_READ  25

/*! A character used to indicate shift */
#define SHIFT_CHAR 0xfe

/*! An invalid character */
#define INVALID_CHAR 0xff

/*! Enumeration of state machine for processing scan codes */
enum  scan_state {
  STATE_START,
  STATE_SHIFT_MADE,
  STATE_SHIFT_CHAR_MADE,
  STATE_SHIFT_CHAR_BROKEN,
  STATE_CHAR_MADE
};

/*! Shared volatile flag counting how many chars have been done. */
static volatile int num_chars_done;


/* --------------------------------------------------------------------------*/
/*!Report failure

  Terminate execution.

  @param[in] msg  Description of test which failed.                          */
/* --------------------------------------------------------------------------*/
void fail (char *msg)
{
  printf ("Test failed: %s\n", msg);
  report (0xeeeeeeee);
  exit (1);

}	/* fail () */


/* --------------------------------------------------------------------------*/
/*!Print an escaped character
   Non-printable characters use their name:
   '\n' - ENTER
   ' '  - SPACE
   '\t' - TAB

   Any char > 127 (0x7f) is an error.

   Any other char less than 0x20 appears as ^<char>.                         */
/* --------------------------------------------------------------------------*/
static void
print_escaped_char (unsigned char  c)
{
  switch (c)
    {
    case '\n': printf ("Received: ENTER.\n"); return;
    case ' ':  printf ("Received: SPACE.\n"); return;
    case '\t': printf ("Received: TAB.\n");   return;

    default:
      if (c > 0x7f)
	{
	  printf ("\nWarning: received erroneous character 0x%02x.\n",
		  (unsigned int) c);
	}
      else if (c < 0x20)
	{
	  printf ("Received '^%c'.\n", '@' + c);
	}
      else
	{
	  printf ("Received '%c'.\n", c);
	}
    }
}	/* print_escaped_char () */


/* --------------------------------------------------------------------------*/
/*!Utility to read a 32 bit memory mapped register

   @param[in] addr  The address to read from

   @return  The result of the read                                           */
/* --------------------------------------------------------------------------*/
static unsigned long
getreg (unsigned long addr)
{
  return *((volatile unsigned char *)addr);

}	/* getreg () */


/* --------------------------------------------------------------------------*/
/*!Keyboard interrupt handler

   Receives codes in the keyboard base register. Reports the first specified
   number of them.

   @todo We only deal with shift keys here. Control, Alt and other funnies are
         left for the future. We also don't do extended sequences.

   If there is a zero in the register that is interpreted as meaning "No data
   more available" and the interrupt routine completes.

   Map the codes back to their characters. Fail on any bad chars.            */
/* --------------------------------------------------------------------------*/
static void
interrupt_handler ()
{
  /* String mapping 256 make codes to chars. 0xff means "invalid", 0xfe means
     "shift key". */
  static const unsigned char *make_codes = (unsigned char *)
    "\xff\xff" "1234567890-=\xff\t"
    "qwertyuiop[]\n\xff" "as"
    "dfghjkl;'`\xfe\\zxcv"
    "bnm,./\xfe\xff\xff \xff\xff\xff\xff\xff\xff"
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff";

  /* String mapping 256 make codes to chars in a shift context. 0xff means
     "invalid".  */
  static const unsigned char *make_shift_codes = (unsigned char *)
    "\xff\xff!@#$%^&*()_+\xff\t"
    "QWERTYUIOP{}\n\xff" "AS"
    "DFGHJKL:\"~\xff|ZXCV"
    "BNM<>?\xff\xff\xff \xff\xff\xff\xff\xff\xff"
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff";

  /* The break codes expected to correspond to a make code. 00 indicates an
     invalid code (should never be requested) */
  static const unsigned char *break_codes = (unsigned char *)
    "\x00\x81\x82\x83\x84\x85\x86\x87\x88\x89\x8a\x8b\x8c\x8d\x00\x8f"
    "\x90\x91\x92\x93\x94\x95\x96\x97\x98\x99\x9a\x9b\x9c\x00\x9e\x9f"
    "\xa0\xa1\xa2\xa3\xa4\xa5\xa6\xa7\xa8\xa9\xaa\xab\xac\xad\xae\xaf"
    "\xb0\xb1\xb2\xb3\xb4\xb5\xb6\x00\x00\xb9\x00\x00\x00\x00\x00\x00"
    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00";

  /* State machine */
  static enum scan_state  state = STATE_START;

  /* Stuff to remember between states */
  static char shift_scan_made;
  static char char_scan_made;

  /* Get all the chars currently available */
  while (1)
    {
      unsigned char  x = getreg (KBD_BASE_ADD);	/* Scan code */
      unsigned char  c;

      /* Give up if nothing more in the buffer */
      if (0 == x)
	{
	  mtspr(SPR_PICSR, 0);		/* Mark interrupt complete */
	  return;
	}

      /* Handle the scan code */
      switch (state)
	{
	case STATE_START:

	  c = make_codes[(int) x];

	  if (SHIFT_CHAR == c)
	    {
	      /* We had a shift key pressed */
	      state           = STATE_SHIFT_MADE;
	      shift_scan_made = x;
	    }
	  else
	    {
	      /* We had another key pressed. If it's valid record it,
		 otherwise blow up. */
	      state = STATE_CHAR_MADE;

	      if (INVALID_CHAR == c)
		{
		  fail ("Unrecognized make scan code");
		}
	      else
		{
		  print_escaped_char (c);
		  char_scan_made = x;
		}
	    }

	  break;

	case STATE_SHIFT_MADE:

	  c     = make_shift_codes[(int) x];
	  state = STATE_SHIFT_CHAR_MADE;

	  /* We had a key pressed after SHIFT. If it's valid, record it,
	     otherwise blow up. */
	  if (INVALID_CHAR == c)
	    {
	      fail ("Unrecognized make scan code");
	    }
	  else
	    {
	      print_escaped_char (c);
	      char_scan_made = x;
	    }

	  break;

	case STATE_SHIFT_CHAR_MADE:

	  /* Check the break matches the char scan made */
	  if (x == break_codes[(int) char_scan_made])
	    {
	      state = STATE_SHIFT_CHAR_BROKEN;
	    }
	  else
	    {
	      fail ("Unrecognized break scan code");
	    }

	  break;

	case STATE_SHIFT_CHAR_BROKEN:

	  /* Check the break matches the shift scan made */
	  if (x == break_codes[(int) shift_scan_made])
	    {
	      state = STATE_START;
	      num_chars_done++;
	    }
	  else
	    {
	      fail ("Unrecognized break scan code");
	    }

	  break;

	case STATE_CHAR_MADE:

	  /* Check the break matches the char scan made */
	  if (x == break_codes[(int) char_scan_made])
	    {
	      state = STATE_START;
	      num_chars_done++;
	    }
	  else
	    {
	      fail ("Unrecognized break scan code");
	    }

	  break;

	default:
	  fail ("*** ABORT ***: Unknown scan state");
	}
    }
}	/* interrupt_handler () */


/* --------------------------------------------------------------------------*/
/*!Simple keyboard test main program

   This appears to use the original XT scan code set, while the keyboard
   handler in Or1ksim generates codes appropriate to a US keyboard layout.

   Set up the interrupt handler and wait until the shared variable shows it
   has done enough chars

   @return  The return code from the program (always zero).                  */
/* --------------------------------------------------------------------------*/
int
main ()
{
  printf ("Reading from keyboard.\n");

  /* Use our low priority interrupt handler. Clear the shared completed flag. */
  excpt_int      = (unsigned long)interrupt_handler;
  
  /* Enable interrupts */
  printf ("Enabling interrupts.\n");

  mtspr (SPR_SR, mfspr(SPR_SR) | SPR_SR_IEE);
  mtspr (SPR_PICMR, mfspr(SPR_PICMR) | (0x00000001L << KBD_IRQ));

  /* Wait until the interrupt handler is finished. */
  while (num_chars_done < MAX_CHARS_TO_READ)
    {
    }

  /* Usual end report */
  report (0xdeaddead);
  return 0;

}	/* main () */
