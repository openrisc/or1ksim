/* int-test.ld. Linker script for Or1ksim interrupt controller test program

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
MEMORY
        {
        flash  : ORIGIN = 0xf0000000, LENGTH = 0x00200000
        ram    : ORIGIN = 0x00000500, LENGTH = 0x001fb000
        }
 
SECTIONS
{
      .reset :
        {
        *(.reset)
         _src_beg = .;
        } > flash
      .text : 
        AT ( ADDR (.reset) + SIZEOF (.reset) )
        {
        _dst_beg = .;
        *(.text)
        } > ram
      .data :
        AT ( ADDR (.reset) + SIZEOF (.reset) + SIZEOF (.text) )
        {
        *(.data)
        *(.rodata)
        _dst_end = .;
        } > ram
      .bss :
        {
        *(.bss)
        } > ram
}
