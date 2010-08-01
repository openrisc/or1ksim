/* parse.c -- Architecture independent load

   Copyright (C) 1999 Damjan Lampret, lampret@opencores.org
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

/* Package includes */
#include "parse.h"
#include "sim-config.h"
#include "debug.h"
#include "abstract.h"
#include "opcode/or32.h"
#include "coff.h"
#include "elf.h"
#include "labels.h"


DECLARE_DEBUG_CHANNEL (coff)

#define MEMORY_LEN  0x100000000ULL

/*!Whether to do immediate statistics. This seems to be for local debugging
   of parse.c */
#define IMM_STATS 0

/*!Unused mem memory marker. It is used when allocating program and data
   memory during parsing */
static unsigned int  freemem;

/*!Translation table provided by microkernel. Only used if simulating
   microkernel. */
static oraddr_t  transl_table;

/*!Used to signal whether during loading of programs a translation fault
   occured. */
static uint32_t  transl_error;

#if IMM_STATS
static int       bcnt[33][3] = { 0 };
static int       bsum[3]     = { 0 };
static uint32_t  movhi       = 0;
#endif  /* IMM_STATS */

/*---------------------------------------------------------------------------*/
/*!Copy a string with null termination

   This function is very similar to strncpy, except it null terminates the
   string. A global function also used by the CUC.

   @param[in] dst  The destination string
   @param[in] src  The source string
   @param[in] n    Number of chars to copy EXCLUDING the null terminator
                   (i.e. dst had better have room for n+1 chars)

   @return  A pointer to dst                                                 */
/*---------------------------------------------------------------------------*/
char *
strstrip (char       *dst,
	  const char *src,
	  int         n)
{
  strncpy (dst, src, n);
  *(dst + n) = '\0';

  return  dst;

}	/* strstrip () */

/*---------------------------------------------------------------------------*/
/*!Translate logical to physical addresses for the loader

   Used only by the simulator loader to translate logical addresses into
   physical.  If loadcode() is called with valid @c virtphy_transl pointer to
   a table of translations then translate() performs translation otherwise
   physical address is equal to logical.

   @param[in] laddr       Logical address
   @param[in] breakpoint  Unused

   @return  The physical address                                             */
/*---------------------------------------------------------------------------*/
static oraddr_t
translate (oraddr_t  laddr,
	   int      *breakpoint)
{
  int i;

  /* No translation (i.e. when loading kernel into simulator) */
  if (transl_table == 0)
    {
      return laddr;
    }

  /* Try to find our translation in the table. */
  for (i = 0; i < (MEMORY_LEN / PAGE_SIZE) * 16; i += 16)
    {
      if ((laddr & ~(PAGE_SIZE - 1)) == eval_direct32 (transl_table + i, 0, 0))
	{
	  /* Page modified */
	  set_direct32 (transl_table + i + 8, -2, 0, 0);
	  PRINTFQ ("found paddr=%" PRIx32 "\n",
		  eval_direct32 (transl_table + i + 4, 0, 0) |
		  (laddr & (PAGE_SIZE - 1)));
	  return  (oraddr_t) eval_direct32 (transl_table + i + 4, 0, 0) |
	          (laddr & (oraddr_t) (PAGE_SIZE - 1));
	}
    }

  /* Allocate new phy page for us. */
  for (i = 0; i < (MEMORY_LEN / PAGE_SIZE) * 16; i += 16)
    {
      if (eval_direct32 (transl_table + i + 8, 0, 0) == 0)
	{
	  /* VPN */
	  set_direct32 (transl_table + i, laddr & ~(PAGE_SIZE - 1), 0, 0);
	  /* PPN */
	  set_direct32 (transl_table + i + 4, (i / 16) * PAGE_SIZE, 0, 0);
	  /* Page modified */
	  set_direct32 (transl_table + i + 8, -2, 0, 0);
	  PRINTFQ ("newly allocated ppn=%" PRIx32 "\n",
		  eval_direct32 (transl_table + i + 4, 0, 0));
	  PRINTFQ ("newly allocated .ppn=%" PRIxADDR "\n", transl_table + i + 4);
	  PRINTFQ ("newly allocated ofs=%" PRIxADDR "\n",
		  (laddr & (PAGE_SIZE - 1)));
	  PRINTFQ ("newly allocated paddr=%" PRIx32 "\n",
		  eval_direct32 (transl_table + i + 4, 0,
				 0) | (laddr & (PAGE_SIZE - 1)));
	  return  (oraddr_t) eval_direct32 (transl_table + i + 4, 0, 0) |
	          (laddr & (oraddr_t) (PAGE_SIZE - 1));
	}
    }

  /* If we come this far then all phy memory is used and we can't find our
     page nor allocate new page. */
  transl_error = 1;
  PRINTFQ ("can't translate %" PRIxADDR "\n", laddr);
  exit (1);

  return  -1;

}	/* translate() */

#if IMM_STATS
static int
bits (uint32_t val)
{
  int i = 1;
  if (!val)
    return 0;
  while (val != 0 && (int32_t) val != -1)
    {
      i++;
      val = (int32_t) val >> 1;
    }
  return i;
}

static void
check_insn (uint32_t insn)
{
  int insn_index = or1ksim_insn_decode (insn);
  struct insn_op_struct *opd = or1ksim_op_start[insn_index];
  uint32_t data = 0;
  int dis = 0;
  const char *name;
  if (!insn || insn_index < 0)
    return;
  name = or1ksim_insn_name (insn_index);
  if (strcmp (name, "l.nop") == 0 || strcmp (name, "l.sys") == 0)
    return;

  while (1)
    {
      uint32_t tmp = 0 unsigned int nbits = 0;
      while (1)
	{
	  tmp |=
	    ((insn >> (opd->type & OPTYPE_SHR)) & ((1 << opd->data) - 1)) <<
	    nbits;
	  nbits += opd->data;
	  if (opd->type & OPTYPE_OP)
	    break;
	  opd++;
	}

      /* Do we have to sign extend? */
      if (opd->type & OPTYPE_SIG)
	{
	  int sbit = (opd->type & OPTYPE_SBIT) >> OPTYPE_SBIT_SHR;
	  if (tmp & (1 << sbit))
	    tmp |= 0xFFFFFFFF << sbit;
	}
      if (opd->type & OPTYPE_DIS)
	{
	  /* We have to read register later.  */
	  data += tmp;
	  dis = 1;
	}
      else
	{
	  if (!(opd->type & OPTYPE_REG) || dis)
	    {
	      if (!dis)
		data = tmp;
	      if (strcmp (name, "l.movhi") == 0)
		{
		  movhi = data << 16;
		}
	      else
		{
		  data |= movhi;
		  //PRINTF ("%08x %s\n", data, name);
		  if (!(or1ksim_or32_opcodes[insn_index].flags & OR32_IF_DELAY))
		    {
		      bcnt[bits (data)][0]++;
		      bsum[0]++;
		    }
		  else
		    {
		      if (strcmp (name, "l.bf") == 0
			  || strcmp (name, "l.bnf") == 0)
			{
			  bcnt[bits (data)][1]++;
			  bsum[1]++;
			}
		      else
			{
			  bcnt[bits (data)][2]++;
			  bsum[2]++;
			}
		    }
		}
	    }
	  data = 0;
	  dis = 0;
	}
      if (opd->type & OPTYPE_LAST)
	{
	  return;
	}
      opd++;
    }
}
#endif

/*---------------------------------------------------------------------------*/
/*!Add an instruction to the program

  @note insn must be in big endian format

  @param[in] address     The address to use
  @param[in] insn        The instruction to add
  @param[in] breakpoint  Not used (it is passed to the translate() function,
                         which also does not use it.                         */
/*---------------------------------------------------------------------------*/
static void
addprogram (oraddr_t  address,
	    uint32_t  insn,
	    int      *breakpoint)
{
  int vaddr = (!runtime.sim.filename) ? translate (address, breakpoint) :
                                        translate (freemem, breakpoint);

  /* We can't have set_program32 functions since it is not gauranteed that the
     section we're loading is aligned on a 4-byte boundry */
  set_program8 (vaddr, (insn >> 24) & 0xff);
  set_program8 (vaddr + 1, (insn >> 16) & 0xff);
  set_program8 (vaddr + 2, (insn >> 8) & 0xff);
  set_program8 (vaddr + 3, insn & 0xff);

#if IMM_STATS
  check_insn (insn);
#endif

  if (runtime.sim.filename)
    {
      freemem += or1ksim_insn_len (or1ksim_insn_decode (insn));
    }
}	/* addprogram () */


/*---------------------------------------------------------------------------*/
/*!Load big-endian COFF file

   @param[in] filename  File to load
   @param[in] sections  Number of sections in file                           */
/*---------------------------------------------------------------------------*/
static void
readfile_coff (char  *filename,
	       short  sections)
{
  FILE *inputfs;
  char inputbuf[4];
  uint32_t insn;
  int32_t sectsize;
  COFF_AOUTHDR coffaouthdr;
  struct COFF_scnhdr coffscnhdr;
  int len;
  int firstthree = 0;
  int breakpoint = 0;

  if (!(inputfs = fopen (filename, "r")))
    {
      perror ("readfile_coff");
      exit (1);
    }

  if (fseek (inputfs, sizeof (struct COFF_filehdr), SEEK_SET) == -1)
    {
      fclose (inputfs);
      perror ("readfile_coff");
      exit (1);
    }

  if (fread (&coffaouthdr, sizeof (coffaouthdr), 1, inputfs) != 1)
    {
      fclose (inputfs);
      perror ("readfile_coff");
      exit (1);
    }

  while (sections--)
    {
      uint32_t scnhdr_pos =
	sizeof (struct COFF_filehdr) + sizeof (coffaouthdr) +
	sizeof (struct COFF_scnhdr) * firstthree;
      if (fseek (inputfs, scnhdr_pos, SEEK_SET) == -1)
	{
	  fclose (inputfs);
	  perror ("readfile_coff");
	  exit (1);
	}
      if (fread (&coffscnhdr, sizeof (struct COFF_scnhdr), 1, inputfs) != 1)
	{
	  fclose (inputfs);
	  perror ("readfile_coff");
	  exit (1);
	}
      PRINTFQ ("Section: %s,", coffscnhdr.s_name);
      PRINTFQ (" paddr: 0x%.8lx,", COFF_LONG_H (coffscnhdr.s_paddr));
      PRINTFQ (" vaddr: 0x%.8lx,", COFF_LONG_H (coffscnhdr.s_vaddr));
      PRINTFQ (" size: 0x%.8lx,", COFF_LONG_H (coffscnhdr.s_size));
      PRINTFQ (" scnptr: 0x%.8lx\n", COFF_LONG_H (coffscnhdr.s_scnptr));

      sectsize = COFF_LONG_H (coffscnhdr.s_size);
      ++firstthree;

      /* loading section */
      freemem = COFF_LONG_H (coffscnhdr.s_paddr);
      if (fseek (inputfs, COFF_LONG_H (coffscnhdr.s_scnptr), SEEK_SET) == -1)
	{
	  fclose (inputfs);
	  perror ("readfile_coff");
	  exit (1);
	}
      while (sectsize > 0
	     && (len = fread (&inputbuf, sizeof (inputbuf), 1, inputfs)))
	{
	  insn = COFF_LONG_H (inputbuf);
	  len = or1ksim_insn_len (or1ksim_insn_decode (insn));
	  if (len == 2)
	    {
	      fseek (inputfs, -2, SEEK_CUR);
	    }

	  addprogram (freemem, insn, &breakpoint);
	  sectsize -= len;
	}
    }
  if (firstthree < 3)
    {
      PRINTFQ ("One or more missing sections. At least");
      PRINTFQ (" three sections expected (.text, .data, .bss).\n");
      exit (1);
    }
  if (firstthree > 3)
    {
      PRINTFQ ("Warning: one or more extra sections. These");
      PRINTFQ (" sections were handled as .data sections.\n");
    }

  fclose (inputfs);
  PRINTFQ ("Finished loading COFF.\n");
  return;

}	/* readfile_coff () */


/* Load symbols from big-endian COFF file. */

static void
readsyms_coff (char *filename, uint32_t symptr, uint32_t syms)
{
  FILE *inputfs;
  struct COFF_syment coffsymhdr;
  int count = 0;
  uint32_t nsyms = syms;
  if (!(inputfs = fopen (filename, "r")))
    {
      perror ("readsyms_coff");
      exit (1);
    }

  if (fseek (inputfs, symptr, SEEK_SET) == -1)
    {
      fclose (inputfs);
      perror ("readsyms_coff");
      exit (1);
    }

  while (syms--)
    {
      int i, n;
      if (fread (&coffsymhdr, COFF_SYMESZ, 1, inputfs) != 1)
	{
	  fclose (inputfs);
	  perror ("readsyms_coff");
	  exit (1);
	}

      n = (unsigned char) coffsymhdr.e_numaux[0];

      /* check whether this symbol belongs to a section and is external
	 symbol; ignore all others */
      if (COFF_SHORT_H (coffsymhdr.e_scnum) >= 0
	  && coffsymhdr.e_sclass[0] == C_EXT)
	{
	  uint32_t *ref = ((uint32_t *) coffsymhdr.e.e.e_zeroes);
	  if (*ref)
	    {
	      if (strlen (coffsymhdr.e.e_name)
		  && strlen (coffsymhdr.e.e_name) < 9)
		add_label (COFF_LONG_H (coffsymhdr.e_value),
			   coffsymhdr.e.e_name);
	    }
	  else
	    {
	      uint32_t fpos = ftell (inputfs);

	      if (fseek
		  (inputfs,
		   symptr + nsyms * COFF_SYMESZ +
		   COFF_LONG_H (coffsymhdr.e.e.e_offset), SEEK_SET) == 0)
		{
		  char tmp[33], *s = &tmp[0];
		  while (s != &tmp[32])
		    if ((*(s++) = fgetc (inputfs)) == 0)
		      break;
		  tmp[32] = 0;
		  add_label (COFF_LONG_H (coffsymhdr.e_value), &tmp[0]);
		}
	      fseek (inputfs, fpos, SEEK_SET);
	    }
	}

      for (i = 0; i < n; i++)
	if (fread (&coffsymhdr, COFF_SYMESZ, 1, inputfs) != 1)
	  {
	    fclose (inputfs);
	    perror ("readsyms_coff3");
	    exit (1);
	  }
      syms -= n;
      count += n;
    }

  fclose (inputfs);
  PRINTFQ ("Finished loading symbols.\n");
  return;
}

static void
readfile_elf (char *filename)
{

  FILE *inputfs;
  struct elf32_hdr elfhdr;
  struct elf32_phdr *elf_phdata = NULL;
  struct elf32_shdr *elf_spnt, *elf_shdata;
  struct elf32_sym *sym_tbl = (struct elf32_sym *) 0;
  uint32_t syms = 0;
  char *str_tbl = (char *) 0;
  char *s_str = (char *) 0;
  int breakpoint = 0;
  uint32_t inputbuf;
  uint32_t padd;
  uint32_t insn;
  int i, j, sectsize, len;

  if (!(inputfs = fopen (filename, "r")))
    {
      perror ("readfile_elf");
      exit (1);
    }

  if (fread (&elfhdr, sizeof (elfhdr), 1, inputfs) != 1)
    {
      perror ("readfile_elf");
      exit (1);
    }

  if ((elf_shdata =
       (struct elf32_shdr *) malloc (ELF_SHORT_H (elfhdr.e_shentsize) *
				     ELF_SHORT_H (elfhdr.e_shnum))) == NULL)
    {
      perror ("readfile_elf");
      exit (1);
    }

  if (fseek (inputfs, ELF_LONG_H (elfhdr.e_shoff), SEEK_SET) != 0)
    {
      perror ("readfile_elf");
      exit (1);
    }

  if (fread
      (elf_shdata,
       ELF_SHORT_H (elfhdr.e_shentsize) * ELF_SHORT_H (elfhdr.e_shnum), 1,
       inputfs) != 1)
    {
      perror ("readfile_elf");
      exit (1);
    }

  if (ELF_LONG_H (elfhdr.e_phoff))
    {
      if ((elf_phdata =
	   (struct elf32_phdr *) malloc (ELF_SHORT_H (elfhdr.e_phnum) *
					 ELF_SHORT_H (elfhdr.e_phentsize))) ==
	  NULL)
	{
	  perror ("readfile_elf");
	  exit (1);
	}

      if (fseek (inputfs, ELF_LONG_H (elfhdr.e_phoff), SEEK_SET) != 0)
	{
	  perror ("readfile_elf");
	  exit (1);
	}

      if (fread
	  (elf_phdata,
	   ELF_SHORT_H (elfhdr.e_phnum) * ELF_SHORT_H (elfhdr.e_phentsize),
	   1, inputfs) != 1)
	{
	  perror ("readfile_elf");
	  exit (1);
	}
    }

  for (i = 0, elf_spnt = elf_shdata; i < ELF_SHORT_H (elfhdr.e_shnum);
       i++, elf_spnt++)
    {

      if (ELF_LONG_H (elf_spnt->sh_type) == SHT_STRTAB)
	{
	  if (NULL != str_tbl)
	    {
	      free (str_tbl);
	    }

	  if ((str_tbl =
	       (char *) malloc (ELF_LONG_H (elf_spnt->sh_size))) == NULL)
	    {
	      perror ("readfile_elf");
	      exit (1);
	    }

	  if (fseek (inputfs, ELF_LONG_H (elf_spnt->sh_offset), SEEK_SET) !=
	      0)
	    {
	      perror ("readfile_elf");
	      exit (1);
	    }

	  if (fread (str_tbl, ELF_LONG_H (elf_spnt->sh_size), 1, inputfs) !=
	      1)
	    {
	      perror ("readfile_elf");
	      exit (1);
	    }
	}
      else if (ELF_LONG_H (elf_spnt->sh_type) == SHT_SYMTAB)
	{

	  if (NULL != sym_tbl)
	    {
	      free (sym_tbl);
	    }

	  if ((sym_tbl =
	       (struct elf32_sym *) malloc (ELF_LONG_H (elf_spnt->sh_size)))
	      == NULL)
	    {
	      perror ("readfile_elf");
	      exit (1);
	    }

	  if (fseek (inputfs, ELF_LONG_H (elf_spnt->sh_offset), SEEK_SET) !=
	      0)
	    {
	      perror ("readfile_elf");
	      exit (1);
	    }

	  if (fread (sym_tbl, ELF_LONG_H (elf_spnt->sh_size), 1, inputfs) !=
	      1)
	    {
	      perror ("readfile_elf");
	      exit (1);
	    }

	  syms =
	    ELF_LONG_H (elf_spnt->sh_size) /
	    ELF_LONG_H (elf_spnt->sh_entsize);
	}
    }

  if (ELF_SHORT_H (elfhdr.e_shstrndx) != SHN_UNDEF)
    {
      elf_spnt = &elf_shdata[ELF_SHORT_H (elfhdr.e_shstrndx)];

      if ((s_str = (char *) malloc (ELF_LONG_H (elf_spnt->sh_size))) == NULL)
	{
	  perror ("readfile_elf");
	  exit (1);
	}

      if (fseek (inputfs, ELF_LONG_H (elf_spnt->sh_offset), SEEK_SET) != 0)
	{
	  perror ("readfile_elf");
	  exit (1);
	}

      if (fread (s_str, ELF_LONG_H (elf_spnt->sh_size), 1, inputfs) != 1)
	{
	  perror ("readfile_elf");
	  exit (1);
	}
    }


  for (i = 0, elf_spnt = elf_shdata; i < ELF_SHORT_H (elfhdr.e_shnum);
       i++, elf_spnt++)
    {

      if ((ELF_LONG_H (elf_spnt->sh_type) & SHT_PROGBITS)
	  && (ELF_LONG_H (elf_spnt->sh_flags) & SHF_ALLOC))
	{

	  padd = ELF_LONG_H (elf_spnt->sh_addr);
	  for (j = 0; j < ELF_SHORT_H (elfhdr.e_phnum); j++)
	    {
	      if (ELF_LONG_H (elf_phdata[j].p_offset) &&
		  ELF_LONG_H (elf_phdata[j].p_offset) <=
		  ELF_LONG_H (elf_spnt->sh_offset)
		  && (ELF_LONG_H (elf_phdata[j].p_offset) +
		      ELF_LONG_H (elf_phdata[j].p_memsz)) >
		  ELF_LONG_H (elf_spnt->sh_offset))
		padd =
		  ELF_LONG_H (elf_phdata[j].p_paddr) +
		  ELF_LONG_H (elf_spnt->sh_offset) -
		  ELF_LONG_H (elf_phdata[j].p_offset);
	    }



	  if (ELF_LONG_H (elf_spnt->sh_name) && s_str)
	    {
	      PRINTFQ ("Section: %s,", &s_str[ELF_LONG_H (elf_spnt->sh_name)]);
	    }
	  else
	    {
	      PRINTFQ ("Section: noname,");
	    }

	  PRINTFQ (" vaddr: 0x%.8lx,", ELF_LONG_H (elf_spnt->sh_addr));
	  PRINTFQ (" paddr: 0x%" PRIx32, padd);
	  PRINTFQ (" offset: 0x%.8lx,", ELF_LONG_H (elf_spnt->sh_offset));
	  PRINTFQ (" size: 0x%.8lx\n", ELF_LONG_H (elf_spnt->sh_size));

	  freemem = padd;
	  sectsize = ELF_LONG_H (elf_spnt->sh_size);

	  if (fseek (inputfs, ELF_LONG_H (elf_spnt->sh_offset), SEEK_SET) !=
	      0)
	    {
	      perror ("readfile_elf");
	      free (elf_phdata);
	      exit (1);
	    }

	  while (sectsize > 0
		 && (len = fread (&inputbuf, sizeof (inputbuf), 1, inputfs)))
	    {
	      insn = ELF_LONG_H (inputbuf);
	      addprogram (freemem, insn, &breakpoint);
	      sectsize -= 4;
	    }
	}
    }

  if (str_tbl)
    {
      i = 0;
      while (syms--)
	{
	  if (sym_tbl[i].st_name && sym_tbl[i].st_info
	      && ELF_SHORT_H (sym_tbl[i].st_shndx) < 0x8000)
	    {
	      add_label (ELF_LONG_H (sym_tbl[i].st_value),
			 &str_tbl[ELF_LONG_H (sym_tbl[i].st_name)]);
	    }
	  i++;
	}
    }

  if (NULL != str_tbl)
    {
      free (str_tbl);
    }

  if (NULL != sym_tbl)
    {
      free (sym_tbl);
    }

  free (s_str);
  free (elf_phdata);
  free (elf_shdata);

}

/* Identify file type and call appropriate readfile_X routine. It only
handles orX-coff-big executables at the moment. */

static void
identifyfile (char *filename)
{
  FILE *inputfs;
  struct COFF_filehdr coffhdr;
  struct elf32_hdr elfhdr;

  if (!(inputfs = fopen (filename, "r")))
    {
      perror (filename);
      fflush (stdout);
      fflush (stderr);
      exit (1);
    }

  if (fread (&coffhdr, sizeof (coffhdr), 1, inputfs) == 1)
    {
      if (COFF_SHORT_H (coffhdr.f_magic) == 0x17a)
	{
	  uint32_t opthdr_size;
	  PRINTFQ ("COFF magic: 0x%.4x\n", COFF_SHORT_H (coffhdr.f_magic));
	  PRINTFQ ("COFF flags: 0x%.4x\n", COFF_SHORT_H (coffhdr.f_flags));
	  PRINTFQ ("COFF symptr: 0x%.8lx\n", COFF_LONG_H (coffhdr.f_symptr));
	  if ((COFF_SHORT_H (coffhdr.f_flags) & COFF_F_EXEC) != COFF_F_EXEC)
	    {
	      PRINTFQ ("This COFF is not an executable.\n");
	      exit (1);
	    }
	  opthdr_size = COFF_SHORT_H (coffhdr.f_opthdr);
	  if (opthdr_size != sizeof (COFF_AOUTHDR))
	    {
	      PRINTFQ ("COFF optional header is missing or not recognized.\n");
	      PRINTFQ ("COFF f_opthdr: 0x%" PRIx32 "\n", opthdr_size);
	      exit (1);
	    }
	  fclose (inputfs);
	  readfile_coff (filename, COFF_SHORT_H (coffhdr.f_nscns));
	  readsyms_coff (filename, COFF_LONG_H (coffhdr.f_symptr),
			 COFF_LONG_H (coffhdr.f_nsyms));
	  return;
	}
      else
	{
	  PRINTFQ ("Not COFF file format\n");
	  fseek (inputfs, 0, SEEK_SET);
	}
    }
  if (fread (&elfhdr, sizeof (elfhdr), 1, inputfs) == 1)
    {
      if (elfhdr.e_ident[0] == 0x7f && elfhdr.e_ident[1] == 0x45
	  && elfhdr.e_ident[2] == 0x4c && elfhdr.e_ident[3] == 0x46)
	{
	  PRINTFQ ("ELF type: 0x%.4x\n", ELF_SHORT_H (elfhdr.e_type));
	  PRINTFQ ("ELF machine: 0x%.4x\n", ELF_SHORT_H (elfhdr.e_machine));
	  PRINTFQ ("ELF version: 0x%.8lx\n", ELF_LONG_H (elfhdr.e_version));
	  PRINTFQ ("ELF sec = %d\n", ELF_SHORT_H (elfhdr.e_shnum));
	  if (ELF_SHORT_H (elfhdr.e_type) != ET_EXEC)
	    {
	      PRINTFQ ("This ELF is not an executable.\n");
	      exit (1);
	    }
	  fclose (inputfs);
	  readfile_elf (filename);
	  return;
	}
      else
	{
	  PRINTFQ ("Not ELF file format.\n");
	  fseek (inputfs, 0, SEEK_SET);
	}
    }

  perror ("identifyfile2");
  fclose (inputfs);

  return;
}


/*---------------------------------------------------------------------------*/
/*!Load file to memory

   Loads file to memory starting at address startaddr and returns freemem.

   @param[in] filename        File to load
   @param[in] startaddr       Start address at which to load
   @param[in] virtphy_transl  Virtual to physical transation table if
                              required. Only used for microkernel simulation,
			      and not used in Ork1sim at present (set to NULL)

   @return  zero on success, negative on failure.                            */
/*---------------------------------------------------------------------------*/
uint32_t
loadcode (char *filename, oraddr_t startaddr, oraddr_t virtphy_transl)
{
  int breakpoint = 0;

  transl_error = 0;
  transl_table = virtphy_transl;
  freemem      = startaddr;
  PRINTFQ ("loadcode: filename %s  startaddr=%" PRIxADDR "  virtphy_transl=%"
	  PRIxADDR "\n", filename, startaddr, virtphy_transl);
  identifyfile (filename);

#if IMM_STATS
  {
    int i = 0, a = 0, b = 0, c = 0;
    PRINTFQ ("index:arith/branch/jump\n");
    for (i = 0; i < 33; i++)
      PRINTFQ ("%2i:\t%3.0f%% / %3.0f%%/ %3.0f%%\t%5i / %5i / %5i\n", i,
	      100. * (a += bcnt[i][0]) / bsum[0], 100. * (b +=
							  bcnt[i][1]) /
	      bsum[1], 100. * (c +=
			       bcnt[i][2]) / bsum[2], bcnt[i][0],
	      bcnt[i][1], bcnt[i][2]);
    PRINTFQ ("\nsum %i %i %i\n", bsum[0], bsum[1], bsum[2]);
  }
#endif

  if (transl_error)
    return -1;
  else
    return translate (freemem, &breakpoint);

}
