/* fields.h -- Some macros to help with bit field definitions

   Copyright (C) 2001 by Erez Volk, erez@opencores.org
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


#ifndef FIELDS__H
#define FIELDS__H


/* Macros to get/set a field in a register
 * Example:
 *  unsigned long done, priority, channel_csr;
 *  
 *  priority = GET_FIELD( channel_csr, DMA_CH_CSR, PRIORITY );
 *  SET_FIELD( channel_csr, DMA_CH_CSR, PRIORITY, priority );
 *  
 *  done = TEST_FLAG( channel_csr, DMA_CH_CSR, DONE );
 *  SET_FLAG( channel_csr, DMA_CH_CSR, DONE );
 *  CLEAR_FLAG( channel_csr, DMA_CH_CSR, DONE );
 *  ASSIGN_FLAG( channel_csr, DMA_CH_CSR, done );
 *
 * For each field, we then define e.g.
 * #define DMA_CH_CSR_PRIORITY_OFFSET  13
 * #define DMA_CH_CSR_PRIORITY_WIDTH   3 // not needed for flags, which always have width = 1
 */

#define FLAG_SHIFT(reg_name,flag_name)  (reg_name##_##flag_name##_OFFSET)
#define FLAG_MASK(reg_name,flag_name)   (1LU << reg_name##_##flag_name##_OFFSET)

#define TEST_FLAG(reg_value,reg_name,flag_name) (((reg_value ) >> reg_name##_##flag_name##_OFFSET) & 1LU)
#define SET_FLAG(reg_value,reg_name,flag_name) { (reg_value) |= 1LU << reg_name##_##flag_name##_OFFSET; }
#define CLEAR_FLAG(reg_value,reg_name,flag_name) { (reg_value) &= ~(1LU << reg_name##_##flag_name##_OFFSET); }
#define ASSIGN_FLAG(reg_value,reg_name,flag_name,flag_value) { \
    (reg_value) = flag_value ? ((reg_value) | (1LU << reg_name##_##flag_name##_OFFSET)) : ((reg_value) & ~(1LU << reg_name##_##flag_name##_OFFSET)); }

#define FIELD_SHIFT(reg_name,field_name) (reg_name##_##field_name##_OFFSET)
#define FIELD_MASK(reg_name,field_name) ((~(~0LU << reg_name##_##field_name##_WIDTH)) << reg_name##_##field_name##_OFFSET)

#define GET_FIELD(reg_value,reg_name,field_name) (((reg_value) >> reg_name##_##field_name##_OFFSET) & (~(~0LU << reg_name##_##field_name##_WIDTH)))
#define SET_FIELD(reg_value,reg_name,field_name,field_value) { \
    (reg_value) = ((reg_value) & ~((~(~0LU << reg_name##_##field_name##_WIDTH)) << reg_name##_##field_name##_OFFSET)) | ((field_value) << reg_name##_##field_name##_OFFSET); }

#endif /* FIELDS__H */
