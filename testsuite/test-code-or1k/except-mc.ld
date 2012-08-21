/* except-mc.ld. Linker script for Or1ksim memory controller test programs

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
        except : ORIGIN = 0x00000000, LENGTH = 0x00002000
        flash  : ORIGIN = 0xf0000000, LENGTH = 0x00200000
        ram    : ORIGIN = 0x00002000, LENGTH = 0x001fe000
        }

ENTRY (_reset_vector)
 
SECTIONS
{
	.except :
        {
        *(.except)
         _src_beg = .;
        } > except

      	.text :
        AT ( ADDR (.except) + SIZEOF (.except) )
        {
        _dst_beg = .;
        *(.text)
        *(.rodata)
	*(.rodata.*)
        } > ram

      	.data :
        AT ( ADDR (.except) + SIZEOF (.except) + SIZEOF (.text))
        {
        *(.data)
        *(.data.rel)
        *(.data.rel.local)
        _dst_end = .;
        } > ram

      	.bss :
        {
        *(.bss)
        } > ram

      	.stack  ALIGN(0x10) (NOLOAD):
        {
        *(.stack)
        _ram_end = .;
        } > ram
} 
