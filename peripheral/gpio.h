/* gpio.h -- Definition of types and structures for the GPIO code

   Copyright (C) 2001 Erez Volk, erez@mailandnews.comopencores.org
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


#ifndef GPIO__H
#define GPIO__H


/* Constants also used by testbench */

/* Address space required by one GPIO */
#define GPIO_ADDR_SPACE      0x20

/* Relative Register Addresses */
#define RGPIO_IN             0x00
#define RGPIO_OUT            0x04
#define RGPIO_OE             0x08
#define RGPIO_INTE           0x0C
#define RGPIO_PTRIG          0x10
#define RGPIO_AUX            0x14
#define RGPIO_CTRL           0x18
#define RGPIO_INTS           0x1C

/* Fields inside RGPIO_CTRL */
#define RGPIO_CTRL_ECLK      0x00000001
#define RGPIO_CTRL_NEC       0x00000002
#define RGPIO_CTRL_INTE      0x00000004
#define RGPIO_CTRL_INTS      0x00000008


/* Function prototypes for external use */
extern void  reg_gpio_sec ();

#endif  /* GPIO__H */
