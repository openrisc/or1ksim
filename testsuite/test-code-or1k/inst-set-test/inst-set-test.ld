/* inst-set.ld. Linker script for Or1ksim instruction set test programs

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

/* ----------------------------------------------------------------------------
   This is a much simplified linker script for use with instruction set test
   programs. In particular they do not use the memory controller.
   --------------------------------------------------------------------------*/

MEMORY
        {
	ram : ORIGIN = 0x00000000, LENGTH = 0x00200000
        }

/* Force _reset to be linked in wherever it is (library or source) */
EXTERN(_reset)

SECTIONS
{
	/* Section .boot-text guarantees that the code for exception
	   handling is placed first. */
	.text :
        {
	*(.boot-text)
        *(.text)
        *(.rodata)
        } > ram

	.data :
	{
	*(.data)
        } > ram

	.bss :
	{
	*(.bss)
        } > ram

	.stack ALIGN (0x10) (NOLOAD) :
	{
        *(.stack)
        } > ram
}

ENTRY (_reset)
