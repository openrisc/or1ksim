/* acv-gpio.c. GPIO test for Or1ksim

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

/* GPIO test */

#include "spr-defs.h"
#include "support.h"
#include "int.h"

/* Relative Register Addresses */
#define RGPIO_IN        0x00
#define RGPIO_OUT       0x04
#define RGPIO_OE        0x08
#define RGPIO_INTE      0x0C
#define RGPIO_PTRIG     0x10
#define RGPIO_AUX       0x14
#define RGPIO_CTRL      0x18
#define RGPIO_INTS      0x1C

/* Fields inside RGPIO_CTRL */
#define RGPIO_CTRL_ECLK      0x00000001
#define RGPIO_CTRL_NEC       0x00000002
#define RGPIO_CTRL_INTE      0x00000004
#define RGPIO_CTRL_INTS      0x00000008

#define GPIO_BASE 0xB0000000LU

#define GPIO_INT_LINE 23 /* To which interrupt is GPIO connected */

typedef volatile unsigned long *GPIO_REG;

GPIO_REG rgpio_in = (unsigned long *)(GPIO_BASE + RGPIO_IN),
  rgpio_out = (unsigned long *)(GPIO_BASE + RGPIO_OUT),
  rgpio_oe = (unsigned long *)(GPIO_BASE + RGPIO_OE),
  rgpio_inte = (unsigned long *)(GPIO_BASE + RGPIO_INTE),
  rgpio_ptrig = (unsigned long *)(GPIO_BASE + RGPIO_PTRIG),
  rgpio_aux = (unsigned long *)(GPIO_BASE + RGPIO_AUX),
  rgpio_ctrl = (unsigned long *)(GPIO_BASE + RGPIO_CTRL),
  rgpio_ints = (unsigned long *)(GPIO_BASE + RGPIO_INTS);

/* fails if x is false */
#define ASSERT(x) ((x)?1: fail (__FILE__, __LINE__))

static void fail (char *file, int line)
{
  printf( "Test failed in %s:%i\n", file, line );
  report( 0xeeeeeeee );
  exit( 1 );
}


static void wait_input( unsigned long value )
{
  unsigned long first = *rgpio_in;
  if ( first == value )
    return;
  while ( 1 ) {
    unsigned long curr = *rgpio_in;
    if ( curr == value )
      return;
    if ( curr != first ) {
      printf( "While waiting for 0x%08lX, input changed from 0x%08lX to 0x%08lX\n", value, first, curr );
      ASSERT( 0 );
    }
  }
}

static volatile unsigned int_count = 0;


static void gpio_interrupt( void *arg )
{
  ++ int_count;
}

static void init_interrupts( void )
{
  int_init();
  int_add( GPIO_INT_LINE, gpio_interrupt, 0);
  
  /* Enable interrupts */
  mtspr( SPR_SR, mfspr(SPR_SR) | SPR_SR_IEE );
}



static void test_registers( void )
{
  printf( "Testing initial values of all registers\n" );
  ASSERT( *rgpio_oe == 0 );
  ASSERT( *rgpio_inte == 0 );
  ASSERT( *rgpio_ptrig == 0 );
  ASSERT( *rgpio_ctrl == 0 );
  ASSERT( *rgpio_ints == 0 );

  printf( "Verifying that RGPIO_IN is read-only\n" );
  {
    unsigned long value = *rgpio_in;
    unsigned i;
    *rgpio_in = ~value;
    ASSERT( *rgpio_in == value );

    for ( i = 0; i < 32; ++ i ) {
      *rgpio_in = 1LU << i;
      ASSERT( *rgpio_in == value );
    }
  }
}


static void test_simple_io( void )
{
  unsigned i;
  unsigned long oe;

  printf( "Testing simple I/O\n" );
  for ( i = 1, oe = 1; i < 31; ++ i, oe = (oe << 1) | 1 ) {
    *rgpio_oe = oe;
    
    *rgpio_out = 0xFFFFFFFF;
    wait_input( 0xFFFFFFFF );
    
    *rgpio_out = 0x00000000;
    wait_input( 0x00000000 );
  }
}


static void clear_interrupt_status( void )
{
  *rgpio_ctrl &= ~RGPIO_CTRL_INTS;
  *rgpio_ints = 0;
}

static void assert_good_interrupt( unsigned expected_count, unsigned long expected_mask )
{
  ASSERT( int_count == expected_count );
  ASSERT( (*rgpio_ctrl & RGPIO_CTRL_INTS) == RGPIO_CTRL_INTS );
  ASSERT( (*rgpio_in & ~*rgpio_oe) == expected_mask );
  ASSERT( (*rgpio_ints & ~*rgpio_oe) == expected_mask );
}

static void test_interrupts( void )
{
  unsigned i;
  
  printf( "Testing interrupts\n" );
  
  *rgpio_oe = 0x80000000;
  int_count = 0;
  *rgpio_inte = 0x7fffffff;
  *rgpio_ptrig = 0x7fffffff;
  *rgpio_ctrl = RGPIO_CTRL_INTE;
  
  *rgpio_out = 0x80000000;
  for ( i = 0; i < 31; ++ i ) {
    /* Wait for interrupt */
    while ( int_count <= i );
    assert_good_interrupt( i + 1, 1LU << i );
    clear_interrupt_status();
    *rgpio_out = (i % 2) ? 0x80000000 : 0;
  }

  /* Return things to normal */
  *rgpio_ctrl = 0;
}

static void test_external_clock( void )
{
  unsigned i;
  printf( "Testing external clock\n" );

  *rgpio_oe = 0x80000000;
  *rgpio_inte = 0x7fffffff;
  *rgpio_ptrig = 0x7fffffff;
  
  /* Test positive edge */
  int_count = 0;
  *rgpio_ctrl = RGPIO_CTRL_INTE;
  *rgpio_out = 0x80000000;
  for ( i = 0; i < 31; ++ i ) {
    while ( int_count <= i );
    assert_good_interrupt( i + 1, 1LU << i );
    clear_interrupt_status();
    *rgpio_out = (i % 2) ? 0x80000000 : 0;
  }

  /* Test negative edge */
  int_count = 0;
  *rgpio_ctrl = RGPIO_CTRL_INTE | RGPIO_CTRL_NEC;
  *rgpio_out = 0x80000000;
  for ( i = 0; i < 31; ++ i ) {
    while ( int_count <= i );
    assert_good_interrupt( i + 1, 1LU << i );
    clear_interrupt_status();
    *rgpio_out = (i % 2) ? 0x80000000 : 0;
  }

  /* Return things to normal */
  *rgpio_ctrl = 0;
}


static void endshake( void )
{
  printf( "Finishing simulation\n" );
  *rgpio_oe = 0xffff0000;
  *rgpio_out = 0x12340000;
  wait_input( 0x12345678 );
  *rgpio_oe = 0xffffffff;
  *rgpio_out = 0xDeadDead;
}


int main()
{
  printf( "Starting GPIO test\n" );

  init_interrupts();
  
  test_registers();
  test_simple_io();
  test_interrupts();
  test_external_clock();
  endshake();

  printf( "Ending GPIO test\n" );
  
  report (0xdeaddead);
  return 0;
}


