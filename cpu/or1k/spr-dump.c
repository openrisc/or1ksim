/* spr_dump.c -- Dump given spr in human readable form

   Copyright (C) 2005 György `nog' Jeney, nog@sdf.lonestar.org
   Copyright (C) 2008 Embecosm Limited
  
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
   with this program.  If not, see <http://www.gnu.org/licenses/>. */

/* This program is commented throughout in a fashion suitable for processing
   with Doxygen. */


/* Autoconf and/or portability configuration */
#include "config.h"
#include "port.h"

/* System includes */
#include <stdio.h>

/* Package includes */
#include "arch.h"
#include "spr-defs.h"
#include "debug.h"
#include "misc.h"


struct spr_bit_def {
  const char *name;
  uorreg_t mask;
};

struct spr_def {
  uint16_t from_spr;
  uint16_t to_spr;
  const char *name;
  const struct spr_bit_def *bits;
};

/* One value spr */
static const struct spr_bit_def spr_one_val[] = {
 { "", 0xffffffff },
 { NULL, 0 } };

/*---------------------------------------------------------[ System group ]---*/
/* Supervisor register */
static const struct spr_bit_def spr_sr[] = {
 { "SR_SM",    SPR_SR_SM    },
 { "SR_TEE",   SPR_SR_TEE   },
 { "SR_IEE",   SPR_SR_IEE   },
 { "SR_DCE",   SPR_SR_DCE   },
 { "SR_ICE",   SPR_SR_ICE   },
 { "SR_DME",   SPR_SR_DME   },
 { "SR_IME",   SPR_SR_IME   },
 { "SR_LEE",   SPR_SR_IME   },
 { "SR_CE",    SPR_SR_CE    },
 { "SR_F",     SPR_SR_F     },
 { "SR_CY",    SPR_SR_CY    },
 { "SR_OV",    SPR_SR_OV    },
 { "SR_OVE",   SPR_SR_OVE   },
 { "SR_DSX",   SPR_SR_DSX   },
 { "SR_EPH",   SPR_SR_EPH   },
 { "SR_FO",    SPR_SR_FO    },
 { "SR_SUMRA", SPR_SR_SUMRA },
 { "SR_RES",   SPR_SR_RES   },
 { "SR_CID",   SPR_SR_CID   },
 { NULL,       0            } };

/* Version register */
static const struct spr_bit_def spr_vr[] = {
 { "VR_VER", SPR_VR_VER },
 { "VR_REV", SPR_VR_REV },
 { NULL,     0          } };

/* upr register */
static const struct spr_bit_def spr_upr[] = {
 { "UPR_UP",   SPR_UPR_UP   },
 { "UPR_DCP",  SPR_UPR_DCP  },
 { "UPR_ICP",  SPR_UPR_ICP  },
 { "UPR_DMP",  SPR_UPR_DMP  },
 { "UPR_IMP",  SPR_UPR_IMP  },
 { "UPR_MP",   SPR_UPR_MP   },
 { "UPR_DUP",  SPR_UPR_DUP  },
 { "UPR_PCUP", SPR_UPR_PCUP },
 { "UPR_PMP",  SPR_UPR_PMP  },
 { "UPR_PICP", SPR_UPR_PICP },
 { "UPR_TTP",  SPR_UPR_TTP  },
 { "UPR_RES",  SPR_UPR_RES  },
 { "UPR_CUP",  SPR_UPR_CUP  },
 { NULL,       0            } };

/* CPUCFGR register */
static const struct spr_bit_def spr_cpucfgr[] = {
 { "CPUCFGR_OB32S", SPR_CPUCFGR_OB32S },
 { "CPUCFGR_OB64S", SPR_CPUCFGR_OB64S },
 { "CPUCFGR_OF32S", SPR_CPUCFGR_OF32S },
 { "CPUCFGR_OF64S", SPR_CPUCFGR_OF64S },
 { "CPUCFGR_OV64S", SPR_CPUCFGR_OV64S },
 { NULL,            0                 } };

/* dmmucfgr register */
static const struct spr_bit_def spr_dmmucfgr[] = {
 { "DMMUCFGR_NTW", SPR_DMMUCFGR_NTW },
 { "DMMUCFGR_NTS", SPR_DMMUCFGR_NTS },
 { "DMMUCFGR_NAE", SPR_DMMUCFGR_NAE },
 { "DMMUCFGR_CRI", SPR_DMMUCFGR_CRI },
 { "DMMUCFGR_PRI", SPR_DMMUCFGR_PRI },
 { "DMMUCFGR_TEIRI", SPR_DMMUCFGR_TEIRI },
 { "DMMUCFGR_HTR", SPR_DMMUCFGR_HTR },
 { NULL, 0 } };

/* immucfgr register */
static const struct spr_bit_def spr_immucfgr[] = {
 { "IMMUCFGR_NTW", SPR_IMMUCFGR_NTW },
 { "IMMUCFGR_NTS", SPR_IMMUCFGR_NTS },
 { "IMMUCFGR_NAE", SPR_IMMUCFGR_NAE },
 { "IMMUCFGR_CRI", SPR_IMMUCFGR_CRI },
 { "IMMUCFGR_PRI", SPR_IMMUCFGR_PRI },
 { "IMMUCFGR_TEIRI", SPR_IMMUCFGR_TEIRI },
 { "IMMUCFGR_HTR", SPR_IMMUCFGR_HTR },
 { NULL, 0 } };

/* dccfgr register */
static const struct spr_bit_def spr_dccfgr[] = {
 { "DCCFGR_NCW", SPR_DCCFGR_NCW },
 { "DCCFGR_NCS", SPR_DCCFGR_NCS },
 { "DCCFGR_CBS", SPR_DCCFGR_CBS },
 { "DCCFGR_CWS", SPR_DCCFGR_CWS },
 { "DCCFGR_CCRI", SPR_DCCFGR_CCRI },
 { "DCCFGR_CBIRI", SPR_DCCFGR_CBIRI },
 { "DCCFGR_CBPRI", SPR_DCCFGR_CBPRI },
 { "DCCFGR_CBLRI", SPR_DCCFGR_CBLRI },
 { "DCCFGR_CBFRI", SPR_DCCFGR_CBFRI },
 { "DCCFGR_CBWBRI", SPR_DCCFGR_CBWBRI },
 { NULL, 0 } };

/* iccfgr register */
static const struct spr_bit_def spr_iccfgr[] = {
 { "ICCFGR_NCW", SPR_ICCFGR_NCW },
 { "ICCFGR_NCS", SPR_ICCFGR_NCS },
 { "ICCFGR_CBS", SPR_ICCFGR_CBS },
 { "ICCFGR_CCRI", SPR_ICCFGR_CCRI },
 { "ICCFGR_CBIRI", SPR_ICCFGR_CBIRI },
 { "ICCFGR_CBPRI", SPR_ICCFGR_CBPRI },
 { "ICCFGR_CBLRI", SPR_ICCFGR_CBLRI },
 { NULL, 0 } };

/* DCFGR register */
static const struct spr_bit_def spr_dcfgr[] = {
 { "DCFGR_NDP",  SPR_DCFGR_NDP  },
 { "DCFGR_WPCI", SPR_DCFGR_WPCI },
 { NULL,         0              } };

/* System group */
static const struct spr_def spr_sys_group[] = {
 /* 000-000 */ { 0x000, 0x000, "SPR_VR",       spr_vr       },
 /* 001-001 */ { 0x001, 0x001, "SPR_UPR",      spr_upr      },
 /* 002-002 */ { 0x002, 0x002, "SPR_CPUCFGR",  spr_cpucfgr  },	/* JPB */
 /* 003-003 */ { 0x003, 0x003, "SPR_DMMUCFGR", spr_dmmucfgr },
 /* 004-004 */ { 0x004, 0x004, "SPR_IMMUCFGR", spr_immucfgr },
 /* 005-005 */ { 0x005, 0x005, "SPR_DCCFGR",   spr_dccfgr   },
 /* 006-006 */ { 0x006, 0x006, "SPR_ICCFGR",   spr_iccfgr   },
 /* 007-007 */ { 0x007, 0x007, "SPR_DCFGR",    spr_dcfgr    },	/* JPB */
 /* 010-010 */ { 0x010, 0x010, "SPR_NPC",      spr_one_val  },
 /* 011-011 */ { 0x011, 0x011, "SPR_SR",       spr_sr       },
 /* 012-012 */ { 0x012, 0x012, "SPR_PPC",      spr_one_val  },
 /* 020-02f */ { 0x020, 0x02f, "SPR_EPCR(%i)", spr_one_val  },
 /* 030-03f */ { 0x030, 0x03f, "SPR_EEAR(%i)", spr_one_val  },
 /* 040-04f */ { 0x040, 0x04f, "SPR_ESR(%i)",  spr_sr       },
 /* 400-41f */ { 0x400, 0x41f, "GPR(%i)",      spr_one_val  },
 /* ------- */ { -1,    -1,    NULL,           NULL         } };

/*-----------------------------------------------------------[ DMMU group ]---*/
/* Data MMU conrtol reg */
static const struct spr_bit_def spr_dmmucr[] = {
 { "DMMUCR_P2S", SPR_DMMUCR_P2S },
 { "DMMUCR_P1S", SPR_DMMUCR_P1S },
 { "DMMUCR_VADDR_WIDTH", SPR_DMMUCR_VADDR_WIDTH },
 { "DMMUCR_PADDR_WIDTH", SPR_DMMUCR_PADDR_WIDTH },
 { NULL, 0 } };

/* dtlbmr register */
static const struct spr_bit_def spr_dtlbmr[] = {
 { "DTLBMR_V", SPR_DTLBMR_V },
 { "DTLBMR_PL1", SPR_DTLBMR_PL1 },
 { "DTLBMR_CID", SPR_DTLBMR_CID },
 { "DTLBMR_LRU", SPR_DTLBMR_LRU },
 { "DTLBMR_VPN", SPR_DTLBMR_VPN },
 { NULL, 0 } };

/* dtlbtr register */
static const struct spr_bit_def spr_dtlbtr[] = {
 { "DTLBTR_CC", SPR_DTLBTR_CC },
 { "DTLBTR_CI", SPR_DTLBTR_CI },
 { "DTLBTR_WBC", SPR_DTLBTR_WBC },
 { "DTLBTR_WOM", SPR_DTLBTR_WOM },
 { "DTLBTR_A", SPR_DTLBTR_A },
 { "DTLBTR_D", SPR_DTLBTR_D },
 { "DTLBTR_URE", SPR_DTLBTR_URE },
 { "DTLBTR_UWE", SPR_DTLBTR_UWE },
 { "DTLBTR_SRE", SPR_DTLBTR_SRE },
 { "DTLBTR_SWE", SPR_DTLBTR_SWE },
 { "DTLBTR_PPN", SPR_DTLBTR_PPN },
 { NULL, 0 } };

/* DMMU group */
static const struct spr_def spr_dmmu_group[] = {
 /* 000-000 */ { 0, 0, "SPR_DMMUCR", spr_dmmucr },
 /* 200-27f */ { 0x200, 0x27f, "SPR_DTLBMR way 0 set %i", spr_dtlbmr },
 /* 280-2ff */ { 0x280, 0x2ff, "SPR_DTLBTR way 0 set %i", spr_dtlbtr },
 /* 300-37f */ { 0x300, 0x37f, "SPR_DTLBMR way 1 set %i", spr_dtlbmr },
 /* 380-3ff */ { 0x380, 0x3ff, "SPR_DTLBTR way 1 set %i", spr_dtlbtr },
 /* 400-47f */ { 0x400, 0x47f, "SPR_DTLBMR way 2 set %i", spr_dtlbmr },
 /* 480-4ff */ { 0x480, 0x4ff, "SPR_DTLBTR way 2 set %i", spr_dtlbtr },
 /* 500-57f */ { 0x500, 0x57f, "SPR_DTLBMR way 3 set %i", spr_dtlbmr },
 /* 580-5ff */ { 0x580, 0x5ff, "SPR_DTLBTR way 3 set %i", spr_dtlbtr },
 /* ------- */ { -1, -1, NULL, NULL } };

/*-----------------------------------------------------------[ IMMU group ]---*/
/* Instruction MMU conrtol reg */
static const struct spr_bit_def spr_immucr[] = {
 { "IMMUCR_P2S", SPR_IMMUCR_P2S },
 { "IMMUCR_P1S", SPR_IMMUCR_P1S },
 { "IMMUCR_VADDR_WIDTH", SPR_IMMUCR_VADDR_WIDTH },
 { "IMMUCR_PADDR_WIDTH", SPR_IMMUCR_PADDR_WIDTH },
 { NULL, 0 } };

/* itlbmr register */
static const struct spr_bit_def spr_itlbmr[] = {
 { "ITLBMR_V", SPR_ITLBMR_V },
 { "ITLBMR_PL1", SPR_ITLBMR_PL1 },
 { "ITLBMR_CID", SPR_ITLBMR_CID },
 { "ITLBMR_LRU", SPR_ITLBMR_LRU },
 { "ITLBMR_VPN", SPR_ITLBMR_VPN },
 { NULL, 0 } };

/* itlbtr register */
static const struct spr_bit_def spr_itlbtr[] = {
 { "ITLBTR_CC", SPR_ITLBTR_CC },
 { "ITLBTR_CI", SPR_ITLBTR_CI },
 { "ITLBTR_WBC", SPR_ITLBTR_WBC },
 { "ITLBTR_WOM", SPR_ITLBTR_WOM },
 { "ITLBTR_A", SPR_ITLBTR_A },
 { "ITLBTR_D", SPR_ITLBTR_D },
 { "ITLBTR_URE", SPR_ITLBTR_SXE },
 { "ITLBTR_UWE", SPR_ITLBTR_UXE },
 { "ITLBTR_PPN", SPR_ITLBTR_PPN },
 { NULL, 0 } };

/* IMMU group */
static const struct spr_def spr_immu_group[] = {
 /* 000-000 */ { 0, 0, "SPR_IMMUCR", spr_immucr },
 /* 200-27f */ { 0x200, 0x27f, "SPR_ITLBMR way 0 set %i", spr_itlbmr },
 /* 280-2ff */ { 0x280, 0x2ff, "SPR_ITLBTR way 0 set %i", spr_itlbtr },
 /* 300-37f */ { 0x300, 0x37f, "SPR_ITLBMR way 1 set %i", spr_itlbmr },
 /* 380-3ff */ { 0x380, 0x3ff, "SPR_ITLBTR way 1 set %i", spr_itlbtr },
 /* 400-47f */ { 0x400, 0x47f, "SPR_ITLBMR way 2 set %i", spr_itlbmr },
 /* 480-4ff */ { 0x480, 0x4ff, "SPR_ITLBTR way 2 set %i", spr_itlbtr },
 /* 500-57f */ { 0x500, 0x57f, "SPR_ITLBMR way 3 set %i", spr_itlbmr },
 /* 580-5ff */ { 0x580, 0x5ff, "SPR_ITLBTR way 3 set %i", spr_itlbtr },
 /* ------- */ { -1, -1, NULL, NULL } };

/*-----------------------------------------------------[ Data cache group ]---*/
static const struct spr_bit_def spr_dccr[] = {
 { "DCCR_EW", SPR_DCCR_EW },
 { NULL, 0 } };

static const struct spr_def spr_dc_group[] = {
 /* 000-000 */ { 0x000, 0x000, "SPR_DCCR", spr_dccr },
 /* 001-001 */ { 0x001, 0x001, "SPR_DCBPR", spr_one_val },
 /* 002-002 */ { 0x002, 0x002, "SPR_DCBFR", spr_one_val },
 /* 003-003 */ { 0x003, 0x003, "SPR_DCBIR", spr_one_val },
 /* 004-004 */ { 0x004, 0x004, "SPR_DCBWR", spr_one_val },
 /* 005-005 */ { 0x005, 0x005, "SPR_DCBLR", spr_one_val },
 /* 200-3ff */ { 0x200, 0x3ff, "SPR_DCR way 0 set %i", spr_one_val },
 /* 400-5ff */ { 0x400, 0x5ff, "SPR_DCR way 1 set %i", spr_one_val },
 /* 600-7ff */ { 0x600, 0x7ff, "SPR_DCR way 2 set %i", spr_one_val },
 /* 800-9ff */ { 0x800, 0x9ff, "SPR_DCR way 3 set %i", spr_one_val },
 /* ------- */ { -1, -1, NULL, NULL } };

/*----------------------------------------------[ Instruction cache group ]---*/
static const struct spr_bit_def spr_iccr[] = {
 { "ICCR_EW", SPR_ICCR_EW },
 { NULL, 0 } };

static const struct spr_def spr_ic_group[] = {
 /* 000-000 */ { 0x000, 0x000, "SPR_ICCR", spr_iccr },
 /* 001-001 */ { 0x001, 0x001, "SPR_ICBPR", spr_one_val },
 /* 002-002 */ { 0x002, 0x002, "SPR_ICBFR", spr_one_val },
 /* 003-003 */ { 0x003, 0x003, "SPR_ICBIR", spr_one_val },
 /* 200-3ff */ { 0x200, 0x3ff, "SPR_ICR way 0 set %i", spr_one_val },
 /* 400-5ff */ { 0x400, 0x5ff, "SPR_ICR way 1 set %i", spr_one_val },
 /* 600-7ff */ { 0x600, 0x7ff, "SPR_ICR way 2 set %i", spr_one_val },
 /* 800-9ff */ { 0x800, 0x9ff, "SPR_ICR way 3 set %i", spr_one_val },
 /* ------- */ { -1, -1, NULL, NULL } };

/*------------------------------------------------------------[ MAC group ]---*/
static const struct spr_def spr_mac_group[] = {
 /* 1-1 */ { 0x1, 0x1, "SPR_MACLO", spr_one_val },
 /* 2-2 */ { 0x2, 0x2, "SPR_MACHI", spr_one_val },
 /* ------- */ { -1, -1, NULL, NULL } };

/*----------------------------------------------------------[ Debug group ]---*/
/* dmr1 register */
static const struct spr_bit_def spr_dmr1[] = {
 { "DMR1_CW0",  SPR_DMR1_CW0  },
 { "DMR1_CW1",  SPR_DMR1_CW1  },
 { "DMR1_CW2",  SPR_DMR1_CW2  },
 { "DMR1_CW3",  SPR_DMR1_CW3  },
 { "DMR1_CW4",  SPR_DMR1_CW4  },
 { "DMR1_CW5",  SPR_DMR1_CW5  },
 { "DMR1_CW6",  SPR_DMR1_CW6  },
 { "DMR1_CW7",  SPR_DMR1_CW7  },
 { "DMR1_CW8",  SPR_DMR1_CW8  },
 { "DMR1_CW9",  SPR_DMR1_CW9  },
 { "DMR1_RES1", SPR_DMR1_RES1 },
 { "DMR1_ST",   SPR_DMR1_ST   },
 { "DMR1_BT",   SPR_DMR1_BT   },
 { "DMR1_RES2", SPR_DMR1_RES2 },
 { NULL,        0             } };

/* dmr2 register */
static const struct spr_bit_def spr_dmr2[] = {
 { "DMR2_WCE0", SPR_DMR2_WCE0 },
 { "DMR2_WCE1", SPR_DMR2_WCE1 },
 { "DMR2_AWTC", SPR_DMR2_AWTC },
 { "DMR2_WGB",  SPR_DMR2_WGB  },
 { "DMR2_WBS",  SPR_DMR2_WBS  },	/* JPB */
 { NULL,        0             } };

/* dwcr register */
static const struct spr_bit_def spr_dwcr[] = {
 { "DWCR_COUNT", SPR_DWCR_COUNT },
 { "DWCR_MATCH", SPR_DWCR_MATCH },
 { NULL, 0 } };

/* dsr register */
static const struct spr_bit_def spr_dsr[] = {
 { "DSR_RSTE", SPR_DSR_RSTE },
 { "DSR_BUSE", SPR_DSR_BUSEE },
 { "DSR_DPFE", SPR_DSR_DPFE },
 { "DSR_IPFE", SPR_DSR_IPFE },
 { "DSR_TTE", SPR_DSR_TTE },
 { "DSR_AE", SPR_DSR_AE },
 { "DSR_IIE", SPR_DSR_IIE },
 { "DSR_IE", SPR_DSR_IE },
 { "DSR_DME", SPR_DSR_DME },
 { "DSR_IME", SPR_DSR_IME },
 { "DSR_RE", SPR_DSR_RE },
 { "DSR_SCE", SPR_DSR_SCE },
 { "DSR_FPE", SPR_DSR_FPE },
 { "DSR_TE", SPR_DSR_TE },
 { NULL, 0 } };

/* drr register */
static const struct spr_bit_def spr_drr[] = {
 { "DRR_RSTE", SPR_DRR_RSTE },
 { "DRR_BUSE", SPR_DRR_BUSEE },
 { "DRR_DPFE", SPR_DRR_DPFE },
 { "DRR_IPFE", SPR_DRR_IPFE },
 { "DRR_TTE", SPR_DRR_TTE },
 { "DRR_AE", SPR_DRR_AE },
 { "DRR_IIE", SPR_DRR_IIE },
 { "DRR_IE", SPR_DRR_IE },
 { "DRR_DME", SPR_DRR_DME },
 { "DRR_IME", SPR_DRR_IME },
 { "DRR_RE", SPR_DRR_RE },
 { "DRR_SCE", SPR_DRR_SCE },
 { "DRR_TE", SPR_DRR_TE },
 { NULL, 0 } };

/* Debug group */
static const struct spr_def spr_d_group[] = {
#if 0
 /* 00-07 */ { 0x00, 0x07, "SPR_DVR(%i)", spr_dvr },
 /* 08-0f */ { 0x08, 0x0f, "SPR_DCR(%i)", spr_dcr },
#endif
 /* 10-10 */ { 0x10, 0x10, "SPR_DMR1", spr_dmr1 },
 /* 11-11 */ { 0x11, 0x11, "SPR_DMR2", spr_dmr2 },
 /* 12-12 */ { 0x12, 0x12, "SPR_DWCR0", spr_dwcr },
 /* 13-13 */ { 0x13, 0x13, "SPR_DWCR1", spr_dwcr },
 /* 14-14 */ { 0x14, 0x14, "SPR_DSR", spr_dsr },
 /* 15-15 */ { 0x15, 0x15, "SPR_DRR", spr_drr },
 /* ----- */ { -1, -1, NULL, NULL } };

/*-------------------------------------------[ Performance counters group ]---*/
static const struct spr_bit_def spr_pcmr[] = {
 { "PCMR_CP", SPR_PCMR_CP },
 { "PCMR_UMRA", SPR_PCMR_UMRA },
 { "PCMR_CISM", SPR_PCMR_CISM },
 { "PCMR_CIUM", SPR_PCMR_CIUM },
 { "PCMR_LA", SPR_PCMR_LA },
 { "PCMR_SA", SPR_PCMR_SA },
 { "PCMR_IF", SPR_PCMR_IF },
 { "PCMR_DCM", SPR_PCMR_DCM },
 { "PCMR_ICM", SPR_PCMR_ICM },
 { "PCMR_IFS", SPR_PCMR_IFS },
 { "PCMR_LSUS", SPR_PCMR_LSUS },
 { "PCMR_BS", SPR_PCMR_BS },
 { "PCMR_DTLBM", SPR_PCMR_DTLBM },
 { "PCMR_ITLBM", SPR_PCMR_ITLBM },
 { "PCMR_DDS", SPR_PCMR_DDS },
 { "PCMR_WPE", SPR_PCMR_WPE },
 { NULL, 0 } };

static const struct spr_def spr_pc_group[] = {
 /* 00-07 */ { 0x00, 0x07, "PCCR", spr_one_val },
 /* 08-0f */ { 0x08, 0x0f, "PCMR", spr_pcmr },
 /* ----- */ { -1, -1, NULL, NULL } };

/*------------------------------------------------[ Power managment group ]---*/
static const struct spr_bit_def spr_pmr[] = {
 { "PMR_SDF", SPR_PMR_SDF },
 { "PMR_DME", SPR_PMR_DME },
 { "PMR_SME", SPR_PMR_SME },
 { "PMR_DCGE", SPR_PMR_DCGE },
 { "PMR_SUME", SPR_PMR_SUME },
 { NULL, 0 } };

static const struct spr_def spr_pm_group[] = {
 /* 0-0 */ { 0x0, 0x0, "SPR_PMR", spr_pmr },
 /* --- */ { -1, -1, NULL, NULL } };

/*------------------------------------------------------------[ PIC group ]---*/
/* picmr register */
static const struct spr_bit_def spr_picmr[] = {
 { "PICMR_IUM", SPR_PICMR_IUM },
 { NULL, 0 } };

static const struct spr_def spr_pic_group[] = {
 /* 0-0 */ { 0, 0, "PICMR", spr_picmr },
 /* 2-2 */ { 2, 2, "PICSR", spr_one_val },
 /* --- */ { -1, -1, NULL, NULL } };

/*-----------------------------------------------------[ Tick timer group ]---*/
static const struct spr_bit_def spr_ttmr[] = {
 { "TTMR_TP", SPR_TTMR_TP },
 { "TTMR_IP", SPR_TTMR_IP },
 { "TTMR_IE", SPR_TTMR_IE },
 { "TTMR_M", SPR_TTMR_M },
 { NULL, 0 } };

static const struct spr_def spr_tt_group[] = {
 /* 0-0 */ { 0, 0, "TTMR", spr_ttmr },
 /* 1-1 */ { 0, 0, "TTCR", spr_one_val },
 /* --- */ { -1, -1, NULL, NULL } };

/* Spr groups */
static const struct spr_def *spr_groups[] = {
 /* 00 */ spr_sys_group,
 /* 01 */ spr_dmmu_group,
 /* 02 */ spr_immu_group,
 /* 03 */ spr_dc_group,
 /* 04 */ spr_ic_group,
 /* 05 */ spr_mac_group,
 /* 06 */ spr_d_group,
 /* 07 */ spr_pc_group,
 /* 08 */ spr_pm_group,
 /* 09 */ spr_pic_group,
 /* 0a */ spr_tt_group };

/* Should be long enough for everything */
static char ret_spr[1000];

/* Dumps the given spr in nice, human readable form */
char *dump_spr(uint16_t spr, uorreg_t spr_val)
{
  const struct spr_def *spr_def;
  uint16_t spr_in_group = spr & (MAX_SPRS_PER_GRP - 1);
  uint16_t spr_group = spr >> MAX_SPRS_PER_GRP_BITS;
  const struct spr_bit_def *spr_bit_def;

  int first_bit = 1;
  int i;
  uorreg_t tmp;

  spr_def = spr_groups[spr_group];

  /* Find the given spr */
  for(; spr_def->from_spr != 0xffff; spr_def++) {
    if((spr_def->from_spr <= spr_in_group) && (spr_def->to_spr >= spr_in_group))
      break;
  }

  if(spr_def->from_spr == 0xffff) {
    sprintf(ret_spr, "Spr %"PRIx16", group %"PRIx16" = 0x%"PRIxREG"\n",
            spr_in_group, spr_group, spr_val);
    return ret_spr;
  }

  /* Decode the spr bits and show them in a pretty format */
  if(spr_def->from_spr == spr_def->to_spr)
    sprintf(ret_spr, "%s", spr_def->name);
  else
    sprintf(ret_spr, spr_def->name, spr_in_group - spr_def->from_spr);
  strcat(ret_spr, " = ");

  /* First get all the single-bit bit fields and dump them */
  for(spr_bit_def = spr_def->bits; spr_bit_def->name; spr_bit_def++) {
    if(!is_power2(spr_bit_def->mask))
      continue;

    if(!(spr_bit_def->mask & spr_val))
      continue;

    if(!first_bit)
      strcat(ret_spr, " | ");
    else
      first_bit = 0;
    strcat(ret_spr, spr_bit_def->name);
  }

  /* Now dump all the multi-bit bit fields */
  for(spr_bit_def = spr_def->bits; spr_bit_def->name; spr_bit_def++) {
    if(is_power2(spr_bit_def->mask))
      continue;
    if(!first_bit)
      strcat(ret_spr, " | ");
    else
      first_bit = 0;
    for(tmp = spr_bit_def->mask, i = 0; !(tmp & 1); i++)
      tmp >>= 1;

    sprintf(ret_spr, "%s%s = %" PRIxREG, ret_spr, spr_bit_def->name,
            (spr_val >> i) & tmp);
  }
  return ret_spr;
}

