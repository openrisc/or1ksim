/* vga.c -- Definition of types and structures for VGA/LCD

   Copyright (C) 2001 Marko Mlinar, markom@opencores.org
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


/* Autoconf and/or portability configuration */
#include "config.h"
#include "port.h"

/* System includes */
#include <stdlib.h>
#include <stdio.h>

/* Package includes */
#include "arch.h"
#include "sim-config.h"
#include "abstract.h"
#include "sched.h"
#include "toplevel-support.h"


#define VGA_CTRL        0x00	/* Control Register */
#define VGA_STAT        0x04	/* Status Register */
#define VGA_HTIM        0x08	/* Horizontal Timing Register */
#define VGA_VTIM        0x0c	/* Vertical Timing Register */
#define VGA_HVLEN       0x10	/* Horizontal and Vertical Length Register */
#define VGA_VBARA       0x14	/* Video Memory Base Address Register A */
#define VGA_VBARB       0x18	/* Video Memory Base Address Register B */
#define VGA_CLUTA       0x800
#define VGA_CLUTB       0xc00
#define VGA_MASK        0xfff
#define VGA_ADDR_SPACE  1024


/* List of implemented registers; all other are ignored.  */

/* Control Register */
#define VGA_CTRL_VEN    0x00000001	/* Video Enable */
#define VGA_CTRL_CD     0x00000300	/* Color Depth */
#define VGA_CTRL_PC     0x00000400	/* Pseudo Color */

/* Status Register */
/* Horiz. Timing Register */
/* Vert. Timing Register */
/* Horiz. and Vert. Length Register */

/* When this counter reaches config.vgas[].refresh_rate, a screenshot is taken and outputted */
struct vga_state
{
  int enabled;
  int pics;
  unsigned long ctrl, stat, htim, vtim;
  int vbindex;
  unsigned long vbar[2];
  unsigned hlen, vlen;
  int pindex;
  unsigned long palette[2][256];
  oraddr_t baseaddr;
  int refresh_rate;
  int irq;
  char *filename;
};


/* Write a register */
void
vga_write32 (oraddr_t addr, uint32_t value, void *dat)
{
  struct vga_state *vga = dat;

  switch (addr)
    {
    case VGA_CTRL:
      vga->ctrl = value;
      break;
    case VGA_STAT:
      vga->stat = value;
      break;
    case VGA_HTIM:
      vga->htim = value;
      break;
    case VGA_VTIM:
      vga->vtim = value;
      break;
    case VGA_HVLEN:
      vga->hlen = (value >> 16) + 2;
      vga->hlen = (value & 0xffff) + 2;
      break;
    case VGA_VBARA:
      vga->vbar[0] = value;
      break;
    case VGA_VBARB:
      vga->vbar[1] = value;
      break;
    default:
      if (addr >= VGA_CLUTA && addr < VGA_CLUTB)
	{
	  vga->palette[0][addr - VGA_CLUTA] = value & 0x00ffffff;
	}
      else if (addr >= VGA_CLUTB)
	{
	  vga->palette[1][addr - VGA_CLUTB] = value & 0x00ffffff;
	}
      else
	{
	  fprintf (stderr,
		   "vga_write32( 0x%" PRIxADDR ", 0x%08" PRIx32
		   " ): Out of range\n", addr + vga->baseaddr, value);
	  return;
	}
      break;
    }
}

/* Read a register */
uint32_t
vga_read32 (oraddr_t addr, void *dat)
{
  struct vga_state *vga = dat;

  switch (addr)
    {
    case VGA_CTRL:
      return vga->ctrl;
    case VGA_STAT:
      return vga->stat;
    case VGA_HTIM:
      return vga->htim;
    case VGA_VTIM:
      return vga->vtim;
    case VGA_HVLEN:
      return ((vga->hlen - 2) << 16) | (vga->vlen - 2);
    case VGA_VBARA:
      return vga->vbar[0];
    case VGA_VBARB:
      return vga->vbar[1];
    default:
      if (addr >= VGA_CLUTA && addr < VGA_CLUTB)
	{
	  return vga->palette[0][addr - VGA_CLUTA];
	}
      else if (addr >= VGA_CLUTB)
	{
	  return vga->palette[1][addr - VGA_CLUTB];
	}
      else
	{
	  fprintf (stderr, "vga_read32( 0x%" PRIxADDR " ): Out of range\n",
		   addr);
	  return 0;
	}
      break;
    }
  return 0;
}

/* This code will only work on little endian machines */
#ifdef __BIG_ENDIAN__
#warning Image dump not supported on big endian machines

static int
vga_dump_image (char *filename, struct vga_start *vga)
{
  return 1;
}

#else

typedef struct
{
  unsigned short int type;	/* Magic identifier            */
  unsigned int size;		/* File size in bytes          */
  unsigned short int reserved1, reserved2;
  unsigned int offset;		/* Offset to image data, bytes */
} BMP_HEADER;

typedef struct
{
  unsigned int size;		/* Header size in bytes      */
  int width, height;		/* Width and height of image */
  unsigned short int planes;	/* Number of colour planes   */
  unsigned short int bits;	/* Bits per pixel            */
  unsigned int compression;	/* Compression type          */
  unsigned int imagesize;	/* Image size in bytes       */
  int xresolution, yresolution;	/* Pixels per meter          */
  unsigned int ncolours;	/* Number of colours         */
  unsigned int importantcolours;	/* Important colours         */
} INFOHEADER;


/* Dumps a bmp file, based on current image */
static int
vga_dump_image (char *filename, struct vga_state *vga)
{
  int sx = vga->hlen;
  int sy = vga->vlen;
  int i, x = 0, y = 0;
  int pc = vga->ctrl & VGA_CTRL_PC;
  int rbpp = vga->ctrl & VGA_CTRL_CD;
  int bpp = rbpp >> 8;

  BMP_HEADER bh;
  INFOHEADER ih;
  FILE *fo;

  if (!sx || !sy)
    return 1;

  /* 16bpp and 32 bpp will be converted to 24bpp */
  if (bpp == 1 || bpp == 3)
    bpp = 2;

  bh.type = 19778;		/* BM */
  bh.size =
    sizeof (BMP_HEADER) + sizeof (INFOHEADER) + sx * sy * (bpp * 4 + 4) +
    (pc ? 1024 : 0);
  bh.reserved1 = bh.reserved2 = 0;
  bh.offset = sizeof (BMP_HEADER) + sizeof (INFOHEADER) + (pc ? 1024 : 0);

  ih.size = sizeof (INFOHEADER);
  ih.width = sx;
  ih.height = sy;
  ih.planes = 1;
  ih.bits = bpp * 4 + 4;
  ih.compression = 0;
  ih.imagesize = x * y * (bpp * 4 + 4);
  ih.xresolution = ih.yresolution = 0;
  ih.ncolours = 0;		/* should be generated */
  ih.importantcolours = 0;	/* all are important */

  fo = fopen (filename, "wb+");
  if (!fwrite (&bh, sizeof (BMP_HEADER), 1, fo))
    return 1;
  if (!fwrite (&ih, sizeof (INFOHEADER), 1, fo))
    return 1;

  if (pc)
    {				/* Write palette? */
      for (i = 0; i < 256; i++)
	{
	  unsigned long val, d;
	  d = vga->palette[vga->pindex][i];
	  val = (d >> 0) & 0xff;	/* Blue */
	  val |= (d >> 8) & 0xff;	/* Green */
	  val |= (d >> 16) & 0xff;	/* Red */
	  if (!fwrite (&val, sizeof (val), 1, fo))
	    return 1;
	}
    }

  /* Data is stored upside down */
  for (y = sy - 1; y >= 0; y--)
    {
      int align = 4 - ((bpp + 1) * sx) % 4;
      int zero = 0;
      for (x = 0; x < sx; x++)
	{
	  unsigned long pixel =
	    eval_direct32 (vga->vbar[vga->vbindex] + (y * sx + x) * (bpp + 1),
			   0, 0);
	  if (!fwrite (&pixel, sizeof (pixel), 1, fo))
	    return 1;
	}
      if (!fwrite (&zero, align, 1, fo))
	return 1;
    }

  fclose (fo);
  return 0;
}
#endif /* !__BIG_ENDIAN__ */

void
vga_job (void *dat)
{
  struct vga_state *vga = dat;
  /* dump the image? */
  char temp[STR_SIZE];
  sprintf (temp, "%s%04i.bmp", vga->filename, vga->pics++);
  vga_dump_image (temp, vga);

  SCHED_ADD (vga_job, dat, vga->refresh_rate);
}

/* Reset all VGAs */
void
vga_reset (void *dat)
{
  struct vga_state *vga = dat;

  int i;

  /* Init palette */
  for (i = 0; i < 256; i++)
    vga->palette[0][i] = vga->palette[1][i] = 0;

  vga->ctrl = vga->stat = vga->htim = vga->vtim = 0;
  vga->hlen = vga->vlen = 0;
  vga->vbar[0] = vga->vbar[1] = 0;

  /* Init screen dumping machine */
  vga->pics = 0;

  vga->pindex = 0;
  vga->vbindex = 0;

  SCHED_ADD (vga_job, dat, vga->refresh_rate);
}

/*----------------------------------------------------[ VGA Configuration ]---*/
static void
vga_enabled (union param_val val, void *dat)
{
  struct vga_state *vga = dat;
  vga->enabled = val.int_val;
}

static void
vga_baseaddr (union param_val val, void *dat)
{
  struct vga_state *vga = dat;
  vga->baseaddr = val.addr_val;
}

static void
vga_irq (union param_val val, void *dat)
{
  struct vga_state *vga = dat;
  vga->irq = val.int_val;
}

static void
vga_refresh_rate (union param_val val, void *dat)
{
  struct vga_state *vga = dat;
  vga->refresh_rate = val.int_val;
}

/*---------------------------------------------------------------------------*/
/*!Set the VGA output file

   Free any previously allocated value.

   @param[in] val  The value to use
   @param[in] dat  The config data structure                                 */
/*---------------------------------------------------------------------------*/
static void
vga_filename (union param_val  val,
	      void            *dat)
{
  struct vga_state *vga = dat;

  if (NULL != vga->filename)
    {
      free (vga->filename);
      vga->filename = NULL;
    }

  if (NULL == (vga->filename = strdup (val.str_val)))
    {
      fprintf (stderr, "ERROR: Out of memory\n");
      exit (-1);
    }

}	/* vga_filename() */


/*---------------------------------------------------------------------------*/
/*!Initialize a new VGA configuration

   ALL parameters are set explicitly to default values.                      */
/*---------------------------------------------------------------------------*/
static void *
vga_sec_start ()
{
  struct vga_state *new = malloc (sizeof (struct vga_state));

  if (!new)
    {
      fprintf (stderr, "Peripheral VGA: Run out of memory\n");
      exit (-1);
    }

  new->enabled      = 1;
  new->baseaddr     = 0;
  new->irq          = 0;
  new->refresh_rate = 1000000000000ULL / 50ULL / config.sim.clkcycle_ps;
  new->filename     = strdup ("vga_out");

  return new;

}	/* vga_sec_start() */


static void
vga_sec_end (void *dat)
{
  struct vga_state *vga = dat;
  struct mem_ops ops;
  if (!vga->enabled)
    {
      free (vga->filename);
      free (vga);
      return;
    }

  memset (&ops, 0, sizeof (struct mem_ops));
  ops.readfunc32 = vga_read32;
  ops.writefunc32 = vga_write32;
  ops.write_dat32 = dat;
  ops.read_dat32 = dat;
  /* FIXME: Correct delay? */
  ops.delayr = 2;
  ops.delayw = 2;
  reg_mem_area (vga->baseaddr, VGA_ADDR_SPACE, 0, &ops);
  reg_sim_reset (vga_reset, dat);
}


/*---------------------------------------------------------------------------*/
/*!Create a new VGA configuration

   ALL parameters are set explicitly to default values. Alternative naming for
   file parameter supported.                                                 */
/*---------------------------------------------------------------------------*/
void
reg_vga_sec ()
{
  struct config_section *sec =
    reg_config_sec ("vga", vga_sec_start, vga_sec_end);

  reg_config_param (sec, "baseaddr",     PARAMT_ADDR, vga_baseaddr);
  reg_config_param (sec, "enabled",      PARAMT_INT, vga_enabled);
  reg_config_param (sec, "irq",          PARAMT_INT, vga_irq);
  reg_config_param (sec, "refresh_rate", PARAMT_INT, vga_refresh_rate);
  reg_config_param (sec, "txfile",       PARAMT_STR, vga_filename);
  reg_config_param (sec, "filename",     PARAMT_STR, vga_filename);

}	/* reg_vga_sec() */

