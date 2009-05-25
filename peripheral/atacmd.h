/* atacommon.h -- ATA Host code simulation. Common defines for ATA Host and
   ATA Device

   Copyright (C) 2002 Richard Herveille, rherveille@opencores.org
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

/* Definitions for the Opencores ATA Controller Core */

#ifndef ATACMD__H
#define ATACMD__H

/* ----------------------------                                       */
/* ----- ATA commands     -----                                       */
/* ----------------------------                                       */
#define CFA_ERASE_SECTORS                0xC0
#define CFA_REQUEST_EXTENDED_ERROR_CODE  0x03
#define CFA_TRANSLATE_SECTOR             0x87
#define CFA_WRITE_MULTIPLE_WITHOUT_ERASE 0xCD
#define CFA_WRITE_SECTORS_WITHOUT_ERASE  0x38
#define CHECK_POWER_MODE                 0xE5
#define DEVICE_RESET                     0x08
#define DOWNLOAD_MICROCODE               0x92
#define EXECUTE_DEVICE_DIAGNOSTICS       0x90
#define FLUSH_CACHE                      0xE7
#define GET_MEDIA_STATUS                 0xDA
#define IDENTIFY_DEVICE                  0xEC
#define IDENTIFY_PACKET_DEVICE           0xA1
#define IDLE                             0xE3
#define IDLE_IMMEDIATE                   0xE1
#define INITIALIZE_DEVICE_PARAMETERS     0x91
#define MEDIA_EJECT                      0xED
#define MEDIA_LOCK                       0xDE
#define MEDIA_UNLOCK                     0xDF
#define NOP                              0x00
#define PACKET                           0xA0
#define READ_BUFFER                      0xE4
#define READ_DMA                         0xC8
#define READ_DMA_QUEUED                  0xC7
#define READ_MULTIPLE                    0xC4
#define READ_NATIVE_MAX_ADDRESS          0xF8
#define READ_SECTOR                      0x20
#define READ_SECTORS                     0x20
#define READ_VERIFY_SECTOR               0x40
#define READ_VERIFY_SECTORS              0x40
#define SECURITY_DISABLE_PASSWORD        0xF6
#define SECURITY_ERASE_PREPARE           0xF3
#define SECURITY_ERASE_UNIT              0xF4
#define SECURITY_FREEZE_LOCK             0xF5
#define SECURITY_SET_PASSWORD            0xF1
#define SECURITY_UNLOCK                  0xF2
#define SEEK                             0x70
#define SERVICE                          0xA2
#define SET_FEATURES                     0xEF
#define SET_MAX                          0xF9
#define SET_MULTIPLE_MODE                0xC6
#define SLEEP                            0xE6
#define SMART                            0xB0
#define STANDBY                          0xE2
#define STANDBY_IMMEDIATE                0xE0
#define WRITE_BUFFER                     0xE8
#define WRITE_DMA                        0xCA
#define WRITE_DMA_QUEUED                 0xCC
#define WRITE_MULTIPLE                   0xC5
#define WRITE_SECTOR                     0x30
#define WRITE_SECTORS                    0x30


/* SET_FEATURES has a number of sub-commands (in Features Register)   */
#define CFA_ENABLE_8BIT_PIO_TRANSFER_MODE       0x01
#define ENABLE_WRITE_CACHE                      0x02
#define SET_TRANSFER_MODE_SECTOR_COUNT_REG      0x03
#define ENABLE_ADVANCED_POWER_MANAGEMENT        0x05
#define ENABLE_POWERUP_IN_STANDBY_FEATURE_SET   0x06
#define POWERUP_IN_STANDBY_FEATURE_SET_SPINUP   0x07
#define CFA_ENABLE_POWER_MODE1                  0x0A
#define DISABLE_MEDIA_STATUS_NOTIFICATION       0x31
#define DISABLE_READ_LOOKAHEAD                  0x55
#define ENABLE_RELEASE_INTERRUPT                0x5D
#define ENABLE_SERVICE_INTERRUPT                0x5E
#define DISABLE_REVERTING_TO_POWERON_DEFAULTS   0x66
#define CFA_DISABLE_8BIT_PIO_TRANSFER_MODE      0x81
#define DISABLE_WRITE_CACHE                     0x82
#define DISABLE_ADVANCED_POWER_MANAGEMENT       0x85
#define DISABLE_POWERUP_IN_STANDBY_FEATURE_SET  0x86
#define CFA_DISABLE_POWER_MODE1                 0x8A
#define ENABLE_MEDIA_STATUS_NOTIFICATION        0x95
#define ENABLE_READ_LOOKAHEAD_FEATURE           0xAA
#define ENABLE_REVERTING_TO_POWERON_DEFAULTS    0xCC
#define DISABLE_RELEASE_INTERRUPT               0xDD
#define DISABLE_SERVICE_INTERRUPT               0xDE

/* SET_MAX has a number of sub-commands (in Features Register)        */
#define SET_MAX_ADDRESS                         0x00
#define SET_MAX_SET_PASSWORD                    0x01
#define SET_MAX_LOCK                            0x02
#define SET_MAX_UNLOCK                          0x03
#define SET_MAX_FREEZE_LOCK                     0x04

/* SET_MAX has a number of sub-commands (in Features Register)        */
#define SMART_READ_DATA                         0xD0
#define SMART_ATTRIBITE_AUTOSAVE                0xD1
#define SMART_SAVE_ATTRIBUTE_VALUES             0xD3
#define SMART_EXECUTE_OFFLINE_IMMEDIATE         0xD4
#define SMART_READ_LOG                          0xD5
#define SMART_WRITE_LOG                         0xD6
#define SMART_ENABLE_OPERATIONS                 0xD8
#define SMART_DISABLE_OPERATIONS                0xD9
#define SMART_RETURN_STATUS                     0xDA

#endif	/* ATACMD__H */
