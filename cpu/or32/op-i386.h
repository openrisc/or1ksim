/* op-i386.h -- i386 specific support routines for micro operations

   Copyright (C) 2005 György `nog' Jeney, nog@sdf.lonestar.org
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


#ifndef OP_I386__H
#define OP_I386__H

#define OP_JUMP(x) asm("jmp *%0" : : "rm" (x))

#define FORCE_RET asm volatile ("")

/* Does a function call (with no arguments) makeing sure that gcc doesn't peddle
 * the stack. (FIXME: Is this safe??) */
#define SPEEDY_CALL(func) asm("call "#func"\n")

/* Return out of the recompiled code */
asm ("	.align 2\n"
     "	.p2align 4,,15\n"
     ".globl op_do_jump\n"
     "	.type	op_do_jump,@function\n"
     "op_do_jump:\n"
     "	ret\n" "	ret\n" "1:\n" "	.size	op_do_jump,1b-op_do_jump\n");

#endif /* OP_I386__H */
