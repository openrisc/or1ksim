/* cfi_memory.c -- CFI Flash memory model

   Copyright (C) 2014 Franck Jullien, franck.jullien@gmail.com

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

/* Autoconf and/or portability configuration */
#include "config.h"
#include "port.h"

/* System includes */
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>

/* Package includes */
#include "arch.h"
#include "sim-config.h"
#include "abstract.h"
#include "mc.h"
#include "toplevel-support.h"

#define XFER_READ8	0
#define XFER_READ16	1
#define XFER_READ32	2
#define XFER_WRITE8	3
#define XFER_WRITE16	4

#define FLASH_OFFSET_MANUFACTURER_ID	0x00
#define FLASH_OFFSET_DEVICE_ID		0x01
#define FLASH_OFFSET_DEVICE_ID2		0x0E
#define FLASH_OFFSET_DEVICE_ID3		0x0F
#define FLASH_OFFSET_CFI		0x55
#define FLASH_OFFSET_CFI_ALT		0x555
#define FLASH_OFFSET_CFI_RESP		0x10
#define FLASH_OFFSET_PRIMARY_VENDOR	0x13
#define FLASH_OFFSET_EXT_QUERY_T_P_ADDR	0x15
#define FLASH_OFFSET_WTOUT		0x1F
#define FLASH_OFFSET_WBTOUT		0x20
#define FLASH_OFFSET_ETOUT		0x21
#define FLASH_OFFSET_CETOUT		0x22
#define FLASH_OFFSET_WMAX_TOUT		0x23
#define FLASH_OFFSET_WBMAX_TOUT		0x24
#define FLASH_OFFSET_EMAX_TOUT		0x25
#define FLASH_OFFSET_CEMAX_TOUT		0x26
#define FLASH_OFFSET_SIZE		0x27
#define FLASH_OFFSET_INTERFACE		0x28
#define FLASH_OFFSET_BUFFER_SIZE	0x2A
#define FLASH_OFFSET_NUM_ERASE_REGIONS	0x2C
#define FLASH_OFFSET_ERASE_REGIONS	0x2D
#define FLASH_OFFSET_PROTECT		0x02
#define FLASH_OFFSET_USER_PROTECTION	0x85
#define FLASH_OFFSET_INTEL_PROTECTION	0x81

#define FLASH_CMD_CFI			0x98
#define FLASH_CMD_READ_ID		0x90
#define FLASH_CMD_RESET			0xff
#define FLASH_CMD_BLOCK_ERASE		0x20
#define FLASH_CMD_ERASE_CONFIRM		0xD0
#define FLASH_CMD_WRITE			0x40
#define FLASH_CMD_PROTECT		0x60
#define FLASH_CMD_PROTECT_SET		0x01
#define FLASH_CMD_PROTECT_CLEAR		0xD0
#define FLASH_CMD_CLEAR_STATUS		0x50
#define FLASH_CMD_WRITE_TO_BUFFER	0xE8
#define FLASH_CMD_WRITE_BUFFER_CONFIRM	0xD0

#define AMD_CMD_RESET			0xF0
#define AMD_CMD_WRITE			0xA0
#define AMD_CMD_ERASE_START		0x80
#define AMD_CMD_ERASE_SECTOR		0x30
#define AMD_CMD_UNLOCK_START		0xAA
#define AMD_CMD_UNLOCK_ACK		0x55
#define AMD_CMD_WRITE_TO_BUFFER		0x25
#define AMD_CMD_WRITE_BUFFER_CONFIRM	0x29

#define AMD_STATUS_TOGGLE		0x40
#define AMD_STATUS_ERROR		0x20

#define ADDR_UNLOCK1			0x555
#define ADDR_UNLOCK2			0x2AA

#define DEVICE_SIZE			0x17
#define ERASE_BLK_INFO			0x1D

#define DEVICE_SIZE_128Mb		0x18
#define DEVICE_SIZE_256Mb		0x19
#define DEVICE_SIZE_512Mb		0x1A
#define DEVICE_SIZE_1Gb			0x1B

uint16_t erase_blk_region_1Gb[] = {
  0xFF, 0x03, 0x00, 0x02
};

uint16_t erase_blk_region_512Mb[] = {
  0xFF, 0x01, 0x00, 0x02
};

uint16_t erase_blk_region_256Mb[] = {
  0xFF, 0x00, 0x00, 0x02
};

uint16_t erase_blk_region_128Mb[] = {
  0x7F, 0x00, 0x00, 0x02
};

uint32_t state_machine_process (int xfer_type, int addr, int data_in,
				void *dat);

uint16_t cfi_table[] = {
  'Q', 'R', 'Y',		/* Query Unique ASCII string 'QRY' */
  0x02, 0x00,			/* Primary OEM Command Set */
  0x40, 0x00,			/* Address for Primary Extended Table */
  0x00, 0x00,			/* Alternate OEM Command Set (00h = none exists) */
  0x00, 0x00,			/* Address for Alternate OEM Extended Table (00h = none exists) */
  0x27,				/* VCC Min. (write/erase) D7-D4: volt, D3-D0: 100 mV */
  0x36,				/* VCC Max. (write/erase) D7-D4: volt, D3-D0: 100 mV */
  0x00,				/* VPP Min. voltage (00h = no VPP pin present) */
  0x00,				/* VPP Max. voltage (00h = no VPP pin present) */
  0x06,				/* Typical timeout per single byte/word write 2N -s */
  0x06,				/* Typical timeout for buffer write 2N -s (00h = not supported) */
  0x09,				/* Typical timeout per individual block erase 2N ms */
  0x13,				/* Typical timeout for full chip erase 2N ms (00h = not supported) */
  0x03,				/* Max. timeout for byte/word write 2N times typical */
  0x05,				/* Max. timeout for buffer write 2N times typical */
  0x03,				/* Max. timeout per individual block erase 2N times typical */
  0x02,				/* Max. timeout for full chip erase 2N times typical (00h = not supported) */
  0x1B,				/* Device Size = 2N byte, 1B = 1 Gb, 1A= 512 Mb, 19 = 256 Mb, 18 = 128 Mb */
  0x02, 0x00,			/* Flash Device Interface description (refer to CFI publication 100) */
  0x06, 0x00,			/* Max. number of byte in multi-byte write = 2N (00h = not supported) */
  0x01,				/* Number of Erase Block Regions within device (01h = uniform device, 02h = boot device) */
  0xff, 0x03,			/* Erase Block Region 1 Information (refer to the CFI specification or CFI publication 100) */
  0x00, 0x02,			/* 00FFh, 0003h, 0000h, 0002h = 1 Gb */
  0x00, 0x00, 0x00, 0x00,	/* Erase Block Region 2 Information (refer to CFI publication 100) */
  0x00, 0x00, 0x00, 0x00,	/* Erase Block Region 3 Information (refer to CFI publication 100) */
  0x00, 0x00, 0x00, 0x00,	/* Erase Block Region 4 Information (refer to CFI publication 100) */
  0x00, 0x00, 0x00,		/* Unused */
  'P', 'R', 'I',		/* Query-unique ASCII string 'PRI' */
  '1',				/* Major version number, ASCII */
  '3',				/* Minor version number, ASCII */
  0x14,				/* Address Sensitive Unlock (Bits 1-0) 0 = Required, 1 = Not Required */
  /* Process Technology (Bits 7-2) 0101b = 90 nm MirrorBit */
  0x02,				/* Erase Suspend, 0 = Not Supported, 1 = To Read Only, 2 = To Read & Write */
  0x01,				/* Sector Protect, 0 = Not Supported, X = Number of sectors in per group */
  0x00,				/* Sector Temporary Unprotect 00 = Not Supported, 01 = Supported */
  0x08,				/* Sector Protect/Unprotect scheme 0008h = Advanced Sector Protection */
  0x00,				/* Simultaneous Operation 00 = Not Supported, X = Number of Sectors */
  0x00,				/* Burst Mode Type 00 = Not Supported, 01 = Supported */
  0x02,				/* Page Mode Type 00 = Not Supported, 01 = 4 Word Page, 02 = 8 Word Page */
  0xb5,				/* ACC (Acceleration) Supply Minimum 00h = Not Supported, D7-D4: Volt, D3-D0: 100 mV */
  0xc5,				/* ACC (Acceleration) Supply Maximum 00h = Not Supported, D7-D4: Volt, D3-D0: 100 mV */
  0x04,				/* WP# Protection 04h = Uniform sectors bottom WP# protect */
  /*                05h = Uniform sectors top WP# protect */
  0x01				/* Program Suspend 00h = Not Supported, 01h = Supported */
};

uint16_t autoselect_array[] = {
  0x01,				/* Manufacturer ID = Spansion */
  0x7E,				/* Device ID, Word 1 */
  0x00, 0x19,			/* Secure Device Verify */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x21,	/* Device ID, Word 2 */
  0x01				/* Device ID, Word 3 */
};

enum cfi_state
{
  UNKNOWN,
  READ,
  RESET,
  UNLOCK_START,
  GET_COMMAND,
  READ_ID,
  ERASE_START,
  ERASE_ACK,
  ERASE,
  DO_WRITE,
  AUTOSELECT_MAN_ID,
  AUTOSELECT_DEV_ID,
  AUTOSELECT_SEC_PROT_VERIFY,
  AUTOSELECT_DEV_PROT_VERIFY,
  CFI_QUERY,
  PROGRAM,
  WRITE_TO_BUFFER,
  PROGRAM_BUF_TO_FLASH,
  WRITE_TO_BUF_ABORT_RESET,
  UNLOCK_BYPASS_ENTER,
  UNLOCK_BYPASS_PROGRAM,
  UNLOCK_BYPASS_SECT_ERASE,
  UNLOCK_BYPASS_CHIP_ERASE,
  UNLOCK_BYPASS_RESET,
  CHIP_ERASE,
  SECTOR_ERASE,
  ERASE_PROG_SUSPEND,
  ERASE_PROG_RESUME,
  SECURED_SILICON_SEC_ENTRY,
  SECURED_SILICON_SEC_EXIT
};

enum cfi_state current_state;

struct mem_config
{
  oraddr_t baseaddr;		/* Start address of the memory */
  unsigned int size;		/* Memory size */
  char *name;			/* Memory type string */
  char *log;			/* Memory log filename */
  char *file;			/* Memory log filename */
  void *mem;			/* malloced memory for this memory */
  int enabled;

};

static uint32_t
simmem_read32 (oraddr_t addr, void *dat)
{
  return state_machine_process (XFER_READ32, addr, 0, dat);
}

static uint16_t
simmem_read16 (oraddr_t addr, void *dat)
{
  return (state_machine_process (XFER_READ16, addr, 0, dat) & 0xffff);
}

static uint8_t
simmem_read8 (oraddr_t addr, void *dat)
{
  return (state_machine_process (XFER_READ8, addr, 0, dat) & 0xff);
}

static void
simmem_write32 (oraddr_t addr, uint32_t value, void *dat)
{
  printf ("ERROR: simmem_write32 not implemented\n");
  exit (1);
}

static void
simmem_write16 (oraddr_t addr, uint16_t value, void *dat)
{
  state_machine_process (XFER_WRITE16, addr, value, dat);
}

static void
simmem_write8 (oraddr_t addr, uint8_t value, void *dat)
{
  state_machine_process (XFER_WRITE8, addr, value, dat);
}

static void
mem_reset (void *dat)
{
  int fd;
  struct mem_config *mem = dat;
  int offset = 0;
  uint32_t *cfi_mem = (uint32_t *) (mem->mem);
  uint32_t tmp;

  if (mem->file)
    {
      fd = open (mem->file, O_RDONLY);
      if (fd < 0)
	printf ("WARNING: Couldn't open %s\n", mem->file);
      else
	while ((read (fd, &tmp, 4)) > 0)
#ifdef OR32_BIG_ENDIAN
	  cfi_mem[offset++] = htonl (tmp);
#else
	  cfi_mem[offset++] = tmp;
#endif
      close (fd);
    }

  current_state = READ;
}

static void
cfi_memory_baseaddr (union param_val val, void *dat)
{
  struct mem_config *mem = dat;
  mem->baseaddr = val.addr_val;
}

static void
cfi_memory_enabled (union param_val val, void *dat)
{
  struct mem_config *mem = dat;
  mem->enabled = val.addr_val;
}

static void
cfi_memory_size (union param_val val, void *dat)
{
  struct mem_config *mem = dat;
  mem->size = val.int_val;

  if (val.int_val <= 0x1000000)
    {				/* 16MB */
      memcpy (&cfi_table[ERASE_BLK_INFO], erase_blk_region_128Mb, 8);
      cfi_table[DEVICE_SIZE] = DEVICE_SIZE_128Mb;
      mem->size = 0x1000000;
    }
  else if (val.int_val <= 0x2000000)
    {				/* 32MB */
      memcpy (&cfi_table[ERASE_BLK_INFO], erase_blk_region_256Mb, 8);
      cfi_table[DEVICE_SIZE] = DEVICE_SIZE_256Mb;
      mem->size = 0x2000000;
    }
  else if (val.int_val <= 0x4000000)
    {				/* 64MB */
      memcpy (&cfi_table[ERASE_BLK_INFO], erase_blk_region_512Mb, 8);
      cfi_table[DEVICE_SIZE] = DEVICE_SIZE_512Mb;
      mem->size = 0x4000000;
    }
  else
    {				/* 128MB */
      memcpy (&cfi_table[ERASE_BLK_INFO], erase_blk_region_1Gb, 8);
      cfi_table[DEVICE_SIZE] = DEVICE_SIZE_1Gb;
      mem->size = 0x8000000;
      if (val.int_val > 0x8000000)
	{
	  printf ("WARNING: Memory size limited to 128MB\n");
	}
    }
}

static void
cfi_memory_name (union param_val val, void *dat)
{
  struct mem_config *mem = dat;

  if (NULL != mem->name)
    {
      free (mem->name);
    }

  mem->name = strdup (val.str_val);
}

static void
cfi_memory_file (union param_val val, void *dat)
{
  struct mem_config *mem = dat;
  mem->file = strdup (val.str_val);
}

static void *
cfi_memory_sec_start ()
{
  struct mem_config *mem = malloc (sizeof (struct mem_config));

  if (!mem)
    {
      fprintf (stderr, "Memory Peripheral: Run out of memory\n");
      exit (-1);
    }

  mem->enabled = 1;
  mem->baseaddr = 0;
  mem->size = 1024;
  mem->name = strdup ("anonymous memory block");
  mem->log = NULL;
  mem->file = NULL;

  return mem;

}

static void
cfi_memory_sec_end (void *dat)
{
  struct mem_config *mem = dat;
  struct dev_memarea *mema;

  struct mem_ops ops;

  if (!mem->size || !mem->enabled)
    {
      free (dat);
      return;
    }

  /* Round up to the next 32-bit boundry */
  if (mem->size & 3)
    {
      mem->size &= ~3;
      mem->size += 4;
    }

  if (!(mem->mem = malloc (mem->size)))
    {
      fprintf (stderr,
	       "Unable to allocate memory at %" PRIxADDR ", length %i\n",
	       mem->baseaddr, mem->size);
      exit (-1);
    }

  memset(mem->mem, 0xff, mem->size);

  ops.readfunc32 = simmem_read32;
  ops.readfunc16 = simmem_read16;
  ops.readfunc8 = simmem_read8;

  ops.writefunc32 = simmem_write32;
  ops.writefunc16 = simmem_write16;
  ops.writefunc8 = simmem_write8;

  ops.writeprog8 = simmem_write8;
  ops.writeprog32 = simmem_write32;
  ops.writeprog8_dat = mem->mem;
  ops.writeprog32_dat = mem->mem;

  ops.read_dat32 = mem->mem;
  ops.read_dat16 = mem->mem;
  ops.read_dat8 = mem->mem;

  ops.write_dat32 = mem->mem;
  ops.write_dat16 = mem->mem;
  ops.write_dat8 = mem->mem;

  ops.log = mem->log;

  mema = reg_mem_area (mem->baseaddr, mem->size, 0, &ops);

  /* Set valid */
  /* FIXME: Should this be done during reset? */
  set_mem_valid (mema, 1);

  mc_reg_mem_area (mema, 0, 0);

  reg_sim_reset (mem_reset, dat);

  current_state = UNKNOWN;
}

void
reg_cfi_memory_sec (void)
{
  struct config_section *sec =
    reg_config_sec ("cfi_flash", cfi_memory_sec_start, cfi_memory_sec_end);

  reg_config_param (sec, "enabled", PARAMT_ADDR, cfi_memory_enabled);
  reg_config_param (sec, "baseaddr", PARAMT_ADDR, cfi_memory_baseaddr);
  reg_config_param (sec, "size", PARAMT_INT, cfi_memory_size);
  reg_config_param (sec, "name", PARAMT_STR, cfi_memory_name);
  reg_config_param (sec, "file", PARAMT_STR, cfi_memory_file);
}

uint32_t
state_machine_process (int xfer_type, int addr, int data_in, void *dat)
{
  uint8_t *cfi_mem_8 = (uint8_t *) dat;
  uint16_t *cfi_mem_16 = (uint16_t *) dat;
  uint32_t *cfi_mem_32 = (uint32_t *) dat;

  uint8_t *cfi_data_8 = (uint8_t *) cfi_table;
  uint16_t *cfi_data_16 = (uint16_t *) cfi_table;

  uint8_t *id_data_8 = (uint8_t *) autoselect_array;
  uint16_t *id_data_16 = (uint16_t *) autoselect_array;

  switch (current_state)
    {

    case UNKNOWN:
      if (xfer_type == XFER_WRITE16 && (data_in == AMD_CMD_RESET))
	{
	  current_state = READ;
	  break;
	}
      break;

    case READ:
      if (xfer_type == XFER_WRITE16 && (addr >> 1) == FLASH_OFFSET_CFI
	  && (data_in == FLASH_CMD_CFI))
	{
	  current_state = CFI_QUERY;
	  break;
	}

      if (xfer_type == XFER_WRITE16 && (addr >> 1) == ADDR_UNLOCK1
	  && (data_in == AMD_CMD_UNLOCK_START))
	{
	  current_state = UNLOCK_START;
	  break;
	}

      if (xfer_type == XFER_READ8)
	{
#ifdef WORDS_BIGENDIAN
	  return cfi_mem_8[addr];
#else
	  return cfi_mem_8[((addr & ~3ul) | (3 - (addr & 3)))];
#endif
	  break;
	}

      if (xfer_type == XFER_READ16)
	{
#ifdef WORDS_BIGENDIAN
	  return cfi_mem_16[addr >> 1];
#else
	  return cfi_mem_16[(addr ^ 2) >> 1];
#endif
	  break;
	}

      if (xfer_type == XFER_READ32)
	{
	  return cfi_mem_32[addr >> 2];
	  break;
	}

      break;

    case CFI_QUERY:
      if (xfer_type == XFER_WRITE16 && (data_in == AMD_CMD_RESET))
	{
	  current_state = READ;
	  break;
	}

      if (xfer_type == XFER_READ16)
	{
	  return cfi_data_16[(addr >> 1) - 0x10];
	}

      if (xfer_type == XFER_READ8)
	{
	  return cfi_data_8[(addr >> 1) * 2 - 0x20];
	}

      break;

    case UNLOCK_START:
      if (xfer_type != XFER_WRITE16)
	break;

      if ((addr >> 1) == ADDR_UNLOCK2 && (data_in == AMD_CMD_UNLOCK_ACK))
	{
	  current_state = GET_COMMAND;
	  break;
	}

    case GET_COMMAND:
      if (xfer_type != XFER_WRITE16)
	break;

      if ((addr >> 1) == ADDR_UNLOCK1 && (data_in == FLASH_CMD_READ_ID))
	{
	  current_state = READ_ID;
	  break;
	}

      if ((addr >> 1) == ADDR_UNLOCK1 && (data_in == AMD_CMD_ERASE_START))
	{
	  current_state = ERASE_START;
	  break;
	}

      if ((addr >> 1) == ADDR_UNLOCK1 && (data_in == AMD_CMD_WRITE))
	{
	  current_state = DO_WRITE;
	  break;
	}

      break;

    case DO_WRITE:
      if (xfer_type != XFER_WRITE16)
	break;

#ifdef WORDS_BIGENDIAN
      cfi_mem_16[addr >> 1] = data_in;
#else
      cfi_mem_16[(addr ^ 2) >> 1] = data_in;
#endif
      current_state = READ;
      break;

    case ERASE_START:
      if (xfer_type != XFER_WRITE16)
	break;

      if ((addr >> 1) == ADDR_UNLOCK1 && (data_in == AMD_CMD_UNLOCK_START))
	{
	  current_state = ERASE_ACK;
	  break;
	}

      break;

    case ERASE_ACK:
      if (xfer_type != XFER_WRITE16)
	break;

      if ((addr >> 1) == ADDR_UNLOCK2 && (data_in == AMD_CMD_UNLOCK_ACK))
	{
	  current_state = ERASE;
	  break;
	}

      break;

    case ERASE:
      if (xfer_type != XFER_WRITE16)
	break;

      if ((addr >> 1) == ADDR_UNLOCK1 && (data_in == 0x10))
	{
	  current_state = READ;
	  break;
	}

      if (data_in == AMD_CMD_ERASE_SECTOR)
	{
	  memset (&cfi_mem_8[(addr >> 1)], 0xff, 64 * 1024);
	  current_state = READ;
	  break;
	}

      break;

    case READ_ID:
      if (xfer_type == XFER_WRITE16 && (data_in == AMD_CMD_RESET))
	{
	  current_state = READ;
	  break;
	}

      if (xfer_type == XFER_READ16)
	{
	  return (id_data_16[(addr >> 1)]);
	}

      if (xfer_type == XFER_READ8)
	{
	  return id_data_8[(addr >> 1)];
	}

      break;

    default:
      printf ("Unknown command %02X\n", data_in);
    }

  return 0;
}
