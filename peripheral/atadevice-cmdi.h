/* atadevice-cmdi.h -- ATA Device command interpreter

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


#ifndef ATADEVICE_CMDI__H
#define ATADEVICE_CMDI__H

/* Package includes */
#include "atadevice.h"


/**********************************************************************/
/* Define default CHS translation parameters                          */
/* bytes per sector                                                   */
#define BYTES_PER_SECTOR 512


/**********************************************************************/
/* Define DMA and PIO cycle times                                     */
/* define the minimum Multiword DMA cycle time per word (in nsec)     */
#define MIN_MWDMA_CYCLE_TIME 100

/* define the manufacturer's recommended Multiword DMA cycle time     */
#define RECOMMENDED_MWDMA_CYCLE_TIME 100

/* define the minimum PIO cycle time per word (in nsec), no IORDY     */
#define MIN_PIO_CYCLE_TIME_NO_IORDY 100

/* define the minimum PIO cycle time per word (in nsec), with IORDY   */
#define MIN_PIO_CYCLE_TIME_IORDY 100


/**********************************************************************/
/* Define supported command sets                                      */
/* 1=command is supported                                             */
/* 0=command is not (yet) supported                                   */
#define SUPPORT_NOP_CMD                      0
#define SUPPORT_READ_BUFFER_CMD              0
#define SUPPORT_WRITE_BUFFER_CMD             0
#define SUPPORT_HOST_PROTECTED_AREA          0
#define SUPPORT_DEVICE_RESET_CMD             1
#define SUPPORT_SERVICE_INTERRUPT            0
#define SUPPORT_RELEASE_INTERRUPT            0
#define SUPPORT_LOOKAHEAD                    0
#define SUPPORT_WRITE_CACHE                  0
#define SUPPORT_POWER_MANAGEMENT             0
#define SUPPORT_REMOVABLE_MEDIA              0
#define SUPPORT_SECURITY_MODE                0
#define SUPPORT_SMART                        0
#define SUPPORT_SET_MAX                      0
#define SET_FEATURES_REQUIRED_AFTER_POWER_UP 0
#define SUPPORT_POWER_UP_IN_STANDBY_MODE     0
#define SUPPORT_REMOVABLE_MEDIA_NOTIFICATION 0
#define SUPPORT_APM                          0
#define SUPPORT_CFA                          0
#define SUPPORT_READ_WRITE_DMA_QUEUED        0
#define SUPPORT_DOWNLOAD_MICROCODE           0

/*
Queue depth defines the maximum queue depth supported by the device.
This includes all commands for which the command acceptance has occured
and command completion has not occured. This value is used for the DMA
READ/WRITE QUEUE command.

QUEUE_DEPTH = actual_queue_depth -1
*/
#define QUEUE_DEPTH 0


/*
   ----------------
   -- Prototypes --
   ----------------
*/
int  ata_device_execute_cmd(struct ata_device *device);

void ata_execute_device_diagnostics_cmd(struct ata_device *device);

#endif	/* ATADEVICE_CMDI__H */
