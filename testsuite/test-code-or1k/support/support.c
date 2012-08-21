/* support.c -- Support code for Or1ksim testing.

   Copyright (C) 1999 Damjan Lampret, lampret@opencores.org
   Copyright (C) 2008, 2010 Embecosm Limited
  
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
   with this program.  If not, see <http://www.gnu.org/licenses/>. */

/* ----------------------------------------------------------------------------
   This code is commented throughout for use with Doxygen.
   --------------------------------------------------------------------------*/

/* Support */

#include "spr-defs.h"
#include "support.h"
#include "int.h"


/* Forward declarations of interrupt handlers */
static void excpt_dummy();
extern void int_main();

/* Exception handlers. All are dummy except the interrupt handler */
unsigned long excpt_buserr   = (unsigned long) excpt_dummy;
unsigned long excpt_dpfault  = (unsigned long) excpt_dummy;
unsigned long excpt_ipfault  = (unsigned long) excpt_dummy;
unsigned long excpt_tick     = (unsigned long) excpt_dummy;
unsigned long excpt_align    = (unsigned long) excpt_dummy;
unsigned long excpt_illinsn  = (unsigned long) excpt_dummy;
unsigned long excpt_int      = (unsigned long) int_main;
unsigned long excpt_dtlbmiss = (unsigned long) excpt_dummy;
unsigned long excpt_itlbmiss = (unsigned long) excpt_dummy;
unsigned long excpt_range    = (unsigned long) excpt_dummy;
unsigned long excpt_syscall  = (unsigned long) excpt_dummy;
unsigned long excpt_break    = (unsigned long) excpt_dummy;
unsigned long excpt_trap     = (unsigned long) excpt_dummy;


/* --------------------------------------------------------------------------*/
/*!Is a character a decimal digit?

   @param[in] c  The character to test

   @return  1 (TRUE) if the character is a decimal digit, 0 (FALSE)
            otherwise                                                        */
/* --------------------------------------------------------------------------*/
static int
is_digit (char  c)
{
  return ('0' <= c) && (c <= '9');

}	/* is_digit () */


/* --------------------------------------------------------------------------*/
/*!Print a char in a width

   The char is always right justified.

   @param[in] c      The character to print
   @param[in] width  The width to print in 

   @return  The number of characters actually printed (always width)         */
/* --------------------------------------------------------------------------*/
static int
printf_char (char  c,
	     int   width)
{
  int  i;

  /* Spacing */
  for (i = 1; i < width; i++)
    {
      putchar (' ');
    }

  /* The char */
  putchar (c);

  return  width;

}	/* printf_char () */


/* --------------------------------------------------------------------------*/
/*!Convert a digit to a char

   We don't worry about the base. If the value supplied is over 10, we assume
   its a letter.

   @param[in] d  The digit to convert.

   @return  The character representation.                                    */
/* --------------------------------------------------------------------------*/
static char
dig2char (int  d)
{
  return (d < 10) ? '0' + d : 'a' + d - 10;

}	/* dit2char () */


/* --------------------------------------------------------------------------*/
/*!Print a number to a base to a string

   The number is unsigned, left justified and null terminated

   @param[in] uval  The value to print
   @param[in] buf   The buffer to print in
   @param[in] base  The base to use.

   @return  the length of the string created.                                */
/* --------------------------------------------------------------------------*/
static int
print_base (long unsigned int  uval,
	    char               buf[],
	    unsigned int       base)
{
  /* Initially print backwards. Always have at least a zero. */
  int  i = 0;

  do
    {
      buf[i] = dig2char (uval % base);
      uval = uval / base;
      i++;
    }
  while (0 != uval);

  buf[i] = 0;			/* End of string */

  int  len = i;			/* Length of the string */

  /* Reverse the string */
  for (i = 0; i < (len / 2); i++)
    {
      char  c          = buf[i];
      buf[i]           = buf[len - i - 1];
      buf[len - i - 1] = c;
    }

  return  len;

}	/* print_base () */


/* --------------------------------------------------------------------------*/
/*!Print a character multiple times

   @param[in] c    The char to print
   @param[in] num  Number of times to print                                 */
/* --------------------------------------------------------------------------*/
static void
print_multichar (char  c,
		 int   num)
{
  for (; num > 0; num--)
    {
      putchar (c);
    }
}	/* print_multichar () */


/* --------------------------------------------------------------------------*/
/*!Print a string

   @param[in] str  The string to print                                       */
/* --------------------------------------------------------------------------*/
static void
print_str (char  str [])
{
  int  i;

  for (i = 0; 0 != str[i]; i++)
    {
      putchar (str[i]);
    }
}	/* print_str () */


/* --------------------------------------------------------------------------*/
/*!Return the length of a string

   @param[in] str    The string whose length is wanted

   @return  The length of the string                                         */
/* --------------------------------------------------------------------------*/
static int
strlen (char  str [])
{
  int  i;

  for (i = 0; str[i] != 0; i++)
    {
    }

  return  i;

}	/* strlen () */


/* --------------------------------------------------------------------------*/
/*!Print a string in a width

   @param[in] str    The string to print
   @param[in] width  The width to print it in (at least)

   @return  The number of chars printed                                      */
/* --------------------------------------------------------------------------*/
static int
printf_str (char  str [],
	    int   width)
{
  int len = strlen (str);

  if (width > len)
    {
      print_multichar (' ', width - len);
    }

  print_str (str);

  return  (width > len) ? width : len;

}	/* printf_str () */


/* --------------------------------------------------------------------------*/
/*!Print a decimal in a width

   The number is always right justified and signed.

   @param[in] val              The value to print
   @param[in] width            The width to print in (at least)
   @param[in] leading_zeros_p  1 (TRUE) if we should print leading zeros,
                               0 (FALSE) otherwise

   @return  The number of chars printed                                      */
/* --------------------------------------------------------------------------*/
static int
printf_decimal (long int  val,
	        int       width,
		int       leading_zeros_p)
{
  int  is_signed_p = 0;

  /* Note if we need a sign */
  if (val < 0)
    {
      val         = -val;
      is_signed_p = 1;
    }

  /* Array to store the number in. We know the max for 32 bits is 10
     digits. Allow for end of string marker */
  char  num_array[11];

  int  num_width = print_base ((unsigned long int) val, num_array, 10);

  /* Now print out the number. */
  num_width += is_signed_p ? 1 : 0;

  if (num_width < width)
    {
      if (leading_zeros_p)
	{
	  if (is_signed_p)
	    {
	      putchar ('-');
	    }

	  print_multichar ('0', width - num_width);
	}
      else
	{
	  print_multichar (' ', width - num_width);

	  if (is_signed_p)
	    {
	      putchar ('-');
	    }
	}
    }
  else
    {
      if (is_signed_p)
	{
	  putchar ('-');
	}
    }

  print_str (num_array);

  return  width > num_width ? width : num_width;

}	/* printf_decimal () */


/* --------------------------------------------------------------------------*/
/*!Print a unsigned to a base in a width

   The number is always right justified and unsigned.

   @param[in] val              The value to print
   @param[in] width            The width to print in (at least)
   @param[in] leading_zeros_p  1 (TRUE) if we should print leading zeros,
                               0 (FALSE) otherwise
   @param[in] base             Base to use when printing

   @return  The number of chars printed                                      */
/* --------------------------------------------------------------------------*/
static int
printf_unsigned_base (unsigned long int  val,
		      int                width,
		      int                leading_zeros_p,
		      unsigned int       base)
{
  int  is_signed_p = 0;

  /* Note if we need a sign */
  if (val < 0)
    {
      val         = -val;
      is_signed_p = 1;
    }

  /* Array to store the number in. We know the max for 32 bits of octal is 11
     digits. Allow for end of string marker */
  char  num_array[12];

  int  num_width = print_base (val, num_array, base);

  /* Now print out the number. */
  num_width += is_signed_p ? 1 : 0;

  if (num_width < width)
    {
      print_multichar (leading_zeros_p ? '0' : ' ', width - num_width);
    }

  print_str (num_array);

  return  width > num_width ? width : num_width;

}	/* printf_unsigned_base () */


/* --------------------------------------------------------------------------*/
/*!Dummy exception handler

   Used for most exceptions as the default hander which does nothing.        */
/* --------------------------------------------------------------------------*/
static void
excpt_dummy()
{
}	/* excpt_dummy ()*/


/*! Function to be called at entry point - not defined here. */
extern int main  ();

/* --------------------------------------------------------------------------*/
/*!Start function

   Called by reset exception handler.                                        */
/* --------------------------------------------------------------------------*/
void
reset ()
{
  exit (main ());  

}	/* reset () */

/* --------------------------------------------------------------------------*/
/*!Exit function

   Return value by making a syscall

   @param[in] rc  Return code                                                */
/* --------------------------------------------------------------------------*/
void
exit (int rc)
{
  __asm__ __volatile__ ("l.add r3,r0,%0\n\t"
                        "l.nop %1": : "r" (rc), "K" (NOP_EXIT));

  /* Header declares function as __noreturn, so ensure that is so. */
  while (1)
    {
    }
}	/* exit () */


/* --------------------------------------------------------------------------*/
/*!Activate printing a character in the simulator

   @param[in] c  The character to print

   @return  The char printed cast to an int                                  */
/* --------------------------------------------------------------------------*/
int
putchar (int  c)
{
  __asm__ __volatile__ ("l.addi\tr3,%0,0\n\t"
                        "l.nop %1": : "r" (c), "K" (NOP_PUTC));

  return  c;

}	/* putchar () */


/* --------------------------------------------------------------------------*/
/*!Print a string

   We need to define this, since the compiler will replace calls to printf
   using just constant strings with trailing newlines with calls to puts
   without the newline.

   @param[in] str  The string to print (without a newline)

   @return  The char printed cast to an int                                  */
/* --------------------------------------------------------------------------*/
int
puts (const char *str)
{
  return  printf ("%s\n", str);

}	/* puts () */


/* --------------------------------------------------------------------------*/
/*!Activate printf support in simulator

   @note This doesn't actually work, so we implement the basics of printf by
         steam, calling useful subsidiary functions based on putchar (), which
         does work.

   @param[in] fmt  The format string
   @param[in] ...  The variable arguments if any

   @return  The number of characters printed                                 */
/* --------------------------------------------------------------------------*/
int
printf(const char *fmt,
	  ...)
{
  int  num_chars = 0;			/* How many chars printed */

  va_list args;
  va_start (args, fmt);

  int  i;				/* Index into the string */

  for (i = 0; fmt[i] != 0; i++)
    {
      if ('%' == fmt[i])
	{
	  int  width;
	  int  leading_zeros_p;

	  /* Decode the field */
	  i++;

	  /* Ignore periods */
	  if ('.' == fmt[i])
		  i++;

	  /* Are leading zeros requested? */
	  if ('0' == fmt[i])
	    {
	      leading_zeros_p = 1;
	      i++;
	    }
	  else
	    {
	      leading_zeros_p = 0;
	    }

	  /* Is there a width specification? */
	  width = 0;

	  while (is_digit (fmt[i]))
	    {
	      width = width * 10 + fmt[i] - '0';
	      i++;
	    }

	  /* We just ignore any "l" specification. We do everything as
	     32-bit. */
	  i += ('l' == fmt[i]) ? 1 : 0;
	      
	  /* Deal with each field according to the type indicactor */
	  char              ch;
	  char             *str;
	  long int          val;
	  unsigned long int uval; 

	  /* There is a bug in GCC for OR1K, which can't handle two many
	     cases. For now we split this into two disjoint case statements. */
	  switch (fmt[i])
	    {
	    case 'c':
	      ch = va_arg (args, int);
	      num_chars += printf_char (ch, width);
	      break;

	    case 'o':
	      uval = va_arg (args, unsigned long int);
	      num_chars +=printf_unsigned_base (uval, width, leading_zeros_p,
					       8);
	      break;

	    case 's':
	      str = va_arg (args, char *);
	      num_chars += printf_str (str, width);
	      break;

	    case 'x':
	      uval = va_arg (args, unsigned long int);
	      num_chars += printf_unsigned_base (uval, width, leading_zeros_p,
						16);
	      break;

	    default:
	      /* Default is to do nothing silently */
	      break;
	    }

	  switch (fmt[i])
	    {
	    case'd': case 'i':
	      val = va_arg (args, long int);
	      num_chars += printf_decimal (val, width, leading_zeros_p);
	      break;
	    }
	}
      else
	{
	  putchar (fmt[i]);
	  num_chars++;
	}
    }

  va_end (args);

  return  num_chars;

}	/* printf () */


/* --------------------------------------------------------------------------*/
/*!Report a 32-bit value

   Uses the built-in simulator functionality.

   @param[in] value  Value to report.                                        */
/* --------------------------------------------------------------------------*/
void
report (unsigned long int  value)
{
  __asm__ __volatile__ ("l.addi\tr3,%0,0\n\t"
                        "l.nop %1": : "r" (value), "K" (NOP_REPORT));

}	/* report () */


/* --------------------------------------------------------------------------*/
/*!Mutliply two 32-bit values to give a 64-bit result
   
   This is not supported by the OR1K GCC.

   Use the identity

   (ax + b).(cx + d) = ac.x^2 + (ad + bc).x + bd

   x = 2^16. None of this should overflow.

   @param[in] op1  First operand
   @param[in] op2  Second operand

   @return  The result                                                       */
/* --------------------------------------------------------------------------*/
static unsigned long long int
l_mulu (unsigned long int  op1,
	unsigned long int  op2)
{
  unsigned long int a, b, c, d;

  a = op1 >> 16;
  b = op1 & 0xffff;
  c = op2 >> 16;
  d = op2 & 0xffff;

  /* Add in the terms */
  unsigned long long int  res;

  /* printf ("a = 0x%08lx, b = 0x%08lx, c = 0x%08lx, d = 0x%08lx\n", a, b, c, d); */
  res  =  (unsigned long long int) (a * c) << 32;
  /* printf ("  interim res = 0x%08lx%08lx\n", (unsigned long int) (res >> 32), */
  /* 	  (unsigned long int) res); */
  res += ((unsigned long long int) (a * d) << 16);
  /* printf ("  interim res = 0x%08lx%08lx\n", (unsigned long int) (res >> 32), */
  /* 	  (unsigned long int) res); */
  res += ((unsigned long long int) (b * c) << 16);
  /* printf ("  interim res = 0x%08lx%08lx\n", (unsigned long int) (res >> 32), */
  /* 	  (unsigned long int) res); */
  res +=  (unsigned long long int) (b * d);
  /* printf ("  interim res = 0x%08lx%08lx\n", (unsigned long int) (res >> 32), */
  /* 	  (unsigned long int) res); */

  /* printf ("0x%08lx * 0x%08lx = 0x%08lx%08lx\n", op1, op2, */
  /* 	  (unsigned long int) (res >> 32), (unsigned long int) res); */

  return  res;

}	/* l_mulu () */

  
/* --------------------------------------------------------------------------*/
/*!Mutliply two 64-bit values
   
   This is not supported by the OR1K GCC.

   Use the identity

   (ax + b).(cx + d) = ac.x^2 + (ad + bc).x + bd

   x = 2^32. We can discard the first term (overflow), though since this is
   for testing we'll print a message.

   The second term may overflow, so we compute the coefficient to 64-bit to
   see if we have overflowed.

   The final term may overflow, so we also compute this to 64-bit, so we can
   add the top 64-bits in.

   @param[in] op1  First operand
   @param[in] op2  Second operand

   @return  The result                                                       */
/* --------------------------------------------------------------------------*/
static unsigned long long int
ll_mulu (unsigned long long int  op1,
	 unsigned long long int  op2)
{
  unsigned long int       a, b, c, d;
  unsigned long long int  tmp, res;

  a = op1 >> 32;
  b = op1 & 0xffffffff;
  c = op2 >> 32;
  d = op2 & 0xffffffff;

  if ((a > 0) && (c > 0))
    {
      printf ("ll_mulu overflows\n");
    }

  /* Compute and test the second term */
  tmp = l_mulu (a, d);

  if (tmp >= 0x100000000ULL)
    {
      printf ("ll_mulu overflows\n");
    }

  res = tmp << 32;

  tmp = l_mulu (b, c);

  if (tmp >= 0x100000000ULL)
    {
      printf ("ll_mulu overflows\n");
    }

  res += tmp << 32;

  /* Compute the third term. Although the term can't overflow, it could
     overflow the result. So just check our answer is larger when the final
     term is added in. */
  tmp = res;

  res += l_mulu (b, d);

  if (res < tmp)
    {
      printf ("ll_mulu overflows\n");
    }

  /* printf ("0x%08lx%08lx * 0x%08lx%08lx = 0x%08lx%08lx\n", a, b, c, d, */
  /* 	  (unsigned long int) (res >> 32), (unsigned long int) res); */

  return  res;

}	/* ll_mulu () */
  
/* --------------------------------------------------------------------------*/
/*!Divide a 64-bit value by a 32 bit value
   
   Until I can get hold of a copy of Knuth volume 2 to check the algorithm,
   this is a bitwise version.

   @param[in] op1  First operand
   @param[in] op2  Second operand

   @return  The result                                                       */
/* --------------------------------------------------------------------------*/
static unsigned long long int
ll_divu (unsigned long long int  dividend,
	 unsigned long int       divisor)
{
  unsigned long long int  t, num_bits;
  unsigned long long int  q, bit, d;
  int            i;

  if (divisor == 0)
    {
      printf ("ERROR: Invalid division by zero\n");
      return  0;
    }

  if (divisor > dividend)
    {
      return  0;
    }

  if (divisor == dividend)
    {
      return  1ULL;
    }

  /* printf ("0x%08x%08x / 0x%08x = ", (unsigned int) (dividend >> 32), */
  /* 	  (unsigned int) (dividend & 0xffffffff), (unsigned int) divisor); */

  num_bits = 64;

  unsigned long long int  remainder = 0;
  unsigned long long int  quotient  = 0;

  while (remainder < divisor)
    {
      bit            = (dividend & 0x8000000000000000ULL) >> 63;
      remainder = (remainder << 1) | bit;
      d              = dividend;
      dividend       = dividend << 1;
      num_bits--;
    }

  /* The loop, above, always goes one iteration too far.  To avoid inserting
     an "if" statement inside the loop the last iteration is simply
     reversed. */
  dividend  = d;
  remainder = remainder >> 1;
  num_bits++;

  for (i = 0; i < num_bits; i++)
    {
      bit            = (dividend & 0x8000000000000000ULL) >> 63;
      remainder = (remainder << 1) | bit;
      t              = remainder - divisor;
      q              = !((t & 0x8000000000000000ULL) >> 63);
      dividend       = dividend << 1;
      quotient  = (quotient << 1) | q;

      if (q)
	{
	  remainder = t;
	}
    }

  /* printf ("0x%08x%08x\n", (unsigned int) (quotient >> 32), */
  /* 	  (unsigned int) (quotient & 0xffffffff)); */

  return  quotient;

}	/* ll_divu () */
  
/* --------------------------------------------------------------------------*/
/*!Read the simulator timer

   Uses the built-in simulator functionality to return the time in
   microseconds.

   @note  Beware that this timer can wrap around.

   @return  The time used since the simulator started.                       */
/* --------------------------------------------------------------------------*/
unsigned long int
read_timer()
{
  unsigned long int       cycles_lo;
  unsigned long int       cycles_hi;
  unsigned long int       cycle_ps;
  unsigned long long int  time_us;

  __asm__ __volatile__ ("l.nop\t\t%0"       : : "K" (NOP_GET_TICKS));
  __asm__ __volatile__ ("l.add\t\t%0,r0,r11": "=r" (cycles_lo) : );
  __asm__ __volatile__ ("l.add\t\t%0,r0,r12": "=r" (cycles_hi) : );
  __asm__ __volatile__ ("l.nop\t\t%0"       : : "K" (NOP_GET_PS));
  __asm__ __volatile__ ("l.add\t\t%0,r0,r11": "=r" (cycle_ps) : );
  
  unsigned long long int  cycles = ((unsigned long long int) cycles_hi << 32) |
                                   ((unsigned long long int) cycles_lo);

  /* This could overflow 64 bits, but if so then the result would overflow 32
     bits. */
  time_us = ll_mulu (cycles, (unsigned long long int) cycle_ps);
  time_us = ll_divu (time_us, 1000000UL);
  return  (unsigned long int) time_us;

}	/* read_timer () */


/* --------------------------------------------------------------------------*/
/*!Write a SPR

   @todo Surely the SPR should be a short int, since it is only 16-bits. Left
         as is for now due to large amount of user code that might need
         changing.

   @param[in] spr    The SPR to write
   @param[in] value  The value to write to the SPR                           */
/* --------------------------------------------------------------------------*/
void
mtspr (unsigned long int  spr,
       unsigned long int  value)
{	
  __asm__ __volatile__ ("l.mtspr\t\t%0,%1,0": : "r" (spr), "r" (value));

}	/* mtspr () */


/* --------------------------------------------------------------------------*/
/*!Read a SPR

   @todo Surely the SPR should be a short int, since it is only 16-bits. Left
         as is for now due to large amount of user code that might need
         changing.

   @param[in] spr    The SPR to write

   @return  The value read from the SPR                                      */
/* --------------------------------------------------------------------------*/
unsigned long int
mfspr (unsigned long spr)
{	
  unsigned long value;

  __asm__ __volatile__ ("l.mfspr\t\t%0,%1,0" : "=r" (value) : "r" (spr));

  return value;

}	/* mfspr () */


/* --------------------------------------------------------------------------*/
/*!Copy between regions of memory

   This should match the library version of memcpy

   @param[out] dstvoid  Pointer to the destination memory area
   @param[in]  srcvoid  Pointer to the source memory area
   @param[in]  length   Number of bytes to copy.                             */
/* --------------------------------------------------------------------------*/
void *
memcpy (void *__restrict          dstvoid,
	__const void *__restrict  srcvoid,
	size_t                    length)
{
  char *dst = dstvoid;
  const char *src = (const char *) srcvoid;

  while (length--)
    {
      *dst++ = *src++;
    }

  return dst;

}	/* memcpy () */

/* --------------------------------------------------------------------------*/
/*!Pseudo-random number generator

   This should return pseudo-random numbers, based on a Galois LFSR

   @return The next pseudo-random number                                     */
/* --------------------------------------------------------------------------*/
unsigned long int
rand ()
{
  static unsigned long int lfsr = 1;
  static int period = 0;
  /* taps: 32 31 29 1; characteristic polynomial: x^32 + x^31 + x^29 + x + 1 */
  lfsr = (lfsr >> 1) ^ (unsigned long int)((0 - (lfsr & 1u)) & 0xd0000001u); 
  ++period;
  return lfsr;
}
