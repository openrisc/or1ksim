/* cache.c. Cache test of Or1ksim

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

#include "support.h"
#include "spr-defs.h"


#define MEM_RAM 0x00100000

/* Linker script symbols */
extern unsigned int _ram_end;

unsigned int program_ram_end;

unsigned int ic_present;
unsigned int dc_present;

/* Number of IC sets (power of 2) */
unsigned int ic_sets;
#define IC_SETS ic_sets
unsigned int dc_sets;
#define DC_SETS dc_sets

/* Block size in bytes (1, 2, 4, 8, 16, 32 etc.) */
unsigned int ic_bs;
#define IC_BLOCK_SIZE ic_bs
unsigned int dc_bs;
#define DC_BLOCK_SIZE dc_bs
 
/* Number of IC ways (1, 2, 3 etc.). */
unsigned int ic_ways;
#define IC_WAYS ic_ways
unsigned int dc_ways;
#define DC_WAYS dc_ways
 
/* Cache size */
#define IC_SIZE (IC_WAYS*IC_SETS*IC_BLOCK_SIZE)
#define DC_SIZE (DC_WAYS*DC_SETS*DC_BLOCK_SIZE)

extern void ic_enable(void);
extern void ic_disable(void);
extern void dc_enable(void);
extern void dc_disable(void);
extern void dc_inv(void);
extern unsigned long ic_inv_test(void);
extern unsigned long dc_inv_test(unsigned long);

extern void (*jalr)(void);
extern void (*jr)(void);

/* Index on jump table */
unsigned long jump_indx;

/* Jump address table */
#define MAX_IC_WAYS 32
unsigned long jump_addr[15*MAX_IC_WAYS];

void dummy();

void jump_and_link(void) 
{
	asm("jalr:");
  asm("l.jr\tr9");
  asm("l.nop");
}

void jump(void)
{
	asm("jr:");
	/* Read and increment index */
	asm("l.lwz\t\tr3,0(r11)");
	asm("l.addi\t\tr3,r3,4");
	asm("l.sw\t\t0(r11),r3");
	/* Load next executin address from table */
	asm("l.lwz\t\tr3,0(r3)");
	/* Jump to that address */
	asm("l.jr\t\tr3") ;
	/* Report that we succeeded */
	asm("l.nop\t0");
}

void copy_jr(unsigned long add)
{
	memcpy((void *)add, (void *)&jr, 24);
}

void call(unsigned long add)
{
  asm("l.movhi\tr11,hi(jump_indx)" : :);
  asm("l.ori\tr11,r11,lo(jump_indx)" : :);
	asm("l.jalr\t\t%0" : : "r" (add) : "r11", "r9");
	asm("l.nop" : :);
}

/* Determine cache configuration from cache configuration registers */
void init_cache_config(void)
{

	unsigned long iccfgr, dccfgr;
	unsigned long upr;

	ic_present = dc_present = 0;
	
	upr = mfspr (SPR_UPR);

	if (!(upr & SPR_UPR_ICP))
	{
		printf("No instruction cache present. Skipping tests.\n");
	}
	else
	{
		iccfgr = mfspr (SPR_ICCFGR);
		
		/* Number of ways */
		ic_ways = (1 << (iccfgr & SPR_ICCFGR_NCW));
		
		/* Number of sets */
		ic_sets = 1 << ((iccfgr & SPR_ICCFGR_NCS) >> 
				SPR_ICCFGR_NCS_OFF);
		
		/* Block size */
		ic_bs = 16 << ((iccfgr & SPR_ICCFGR_CBS) >> SPR_ICCFGR_CBS_OFF);

		ic_present = 1;
	}

	if (!(upr & SPR_UPR_DCP))
	{
		printf("No data cache present. Skipping tests.\n");
	}
	else
	{
		dccfgr = mfspr (SPR_DCCFGR);
		
		/* Number of ways */
		dc_ways = (1 << (dccfgr & SPR_DCCFGR_NCW));
		
		/* Number of sets */
		dc_sets = 1 << ((dccfgr & SPR_DCCFGR_NCS) >> 
				SPR_DCCFGR_NCS_OFF);
		
		/* Block size */
		dc_bs = 16 << ((dccfgr & SPR_DCCFGR_CBS) >> SPR_DCCFGR_CBS_OFF);

		dc_present = 1;
	}
		
}

int dc_test(void)
{
	int i;
	unsigned long base, add, ul;
	
	base = (((unsigned long)MEM_RAM / (IC_SETS*IC_BLOCK_SIZE)) * IC_SETS*IC_BLOCK_SIZE) + IC_SETS*IC_BLOCK_SIZE;

	dc_enable();
	
	/* Cache miss r */
	add = base;
	for(i = 0; i < DC_WAYS; i++) {
		ul = REG32(add);
		ul = REG32(add + DC_BLOCK_SIZE + 4);
		ul = REG32(add + 2*DC_BLOCK_SIZE + 8);
		ul = REG32(add + 3*DC_BLOCK_SIZE + 12);
		add += DC_SETS*DC_BLOCK_SIZE;
	}

	/* Cache hit w */
	add = base;
	for(i = 0; i < DC_WAYS; i++) { 
		REG32(add + 0) = 0x00000001;
		REG32(add + 4) = 0x00000000;
		REG32(add + 8) = 0x00000000;
		REG32(add + 12) = 0x00000000;
		REG32(add + DC_BLOCK_SIZE + 0) = 0x00000000;
		REG32(add + DC_BLOCK_SIZE + 4) = 0x00000002;
		REG32(add + DC_BLOCK_SIZE + 8) = 0x00000000;
		REG32(add + DC_BLOCK_SIZE + 12) = 0x00000000;
		REG32(add + 2*DC_BLOCK_SIZE + 0) = 0x00000000;
		REG32(add + 2*DC_BLOCK_SIZE + 4) = 0x00000000;
		REG32(add + 2*DC_BLOCK_SIZE + 8) = 0x00000003;
		REG32(add + 2*DC_BLOCK_SIZE + 12) = 0x00000000;
		REG32(add + 3*DC_BLOCK_SIZE + 0) = 0x00000000;
		REG32(add + 3*DC_BLOCK_SIZE + 4) = 0x00000000;
		REG32(add + 3*DC_BLOCK_SIZE + 8) = 0x00000000;
		REG32(add + 3*DC_BLOCK_SIZE + 12) = 0x00000004;
		add += DC_SETS*DC_BLOCK_SIZE;
	}

	/* Cache hit r/w */
	add = base;
	for(i = 0; i < DC_WAYS; i++) {
		REG8(add + DC_BLOCK_SIZE - 4) = REG8(add + 3);
		REG8(add + 2*DC_BLOCK_SIZE - 8) = REG8(add + DC_BLOCK_SIZE + 7);
		REG8(add + 3*DC_BLOCK_SIZE - 12) = REG8(add + 2*DC_BLOCK_SIZE + 11);
		REG8(add + 4*DC_BLOCK_SIZE - 16) = REG8(add + 3*DC_BLOCK_SIZE + 15);
		add += DC_SETS*DC_BLOCK_SIZE;
	}

	/* Cache hit/miss r/w */
	add = base;
	for(i = 0; i < DC_WAYS; i++) {
		REG16(add + (IC_SETS - 1)*IC_BLOCK_SIZE) = REG16(add + DC_BLOCK_SIZE - 4) + REG16(add + 2);
		REG16(add + (IC_SETS - 1)*IC_BLOCK_SIZE + 2) = REG16(add + DC_BLOCK_SIZE - 8) + REG16(add + DC_BLOCK_SIZE + 6);
		REG16(add + (IC_SETS - 1)*IC_BLOCK_SIZE + 4) = REG16(add + DC_BLOCK_SIZE - 12) + REG16(add + 2*DC_BLOCK_SIZE + 10);
		REG16(add + (IC_SETS - 1)*IC_BLOCK_SIZE + 6) = REG16(add+ DC_BLOCK_SIZE - 16) + REG16(add + 2*DC_BLOCK_SIZE + 14);
		add += DC_SETS*DC_BLOCK_SIZE;
	}

	/* Fill cache with unused data */
	add = base + DC_WAYS*DC_SETS*DC_BLOCK_SIZE;
	for(i = 0; i < DC_WAYS; i++) {
		ul = REG32(add);
		ul = REG32(add + DC_BLOCK_SIZE);
		ul = REG32(add + 2*DC_BLOCK_SIZE);
		ul = REG32(add + 3*DC_BLOCK_SIZE);
		add += DC_SETS*DC_BLOCK_SIZE;
	}

	/* Cache hit/miss r */
	ul = 0;
	add = base;
	for(i = 0; i < DC_WAYS; i++) {
                ul += 	REG16(add + (IC_SETS - 1)*IC_BLOCK_SIZE) + 
			REG16(add + DC_BLOCK_SIZE - 4) + 
			REG16(add + 2);
                ul += 	REG16(add + (IC_SETS - 1)*IC_BLOCK_SIZE + 2) +
			REG16(add + DC_BLOCK_SIZE - 8) +
			REG16(add + DC_BLOCK_SIZE + 6); 
                ul +=	REG16(add + (IC_SETS - 1)*IC_BLOCK_SIZE + 4) + 
			REG16(add + DC_BLOCK_SIZE - 12) + 
			REG16(add + 2*DC_BLOCK_SIZE + 10);
                ul +=	REG16(add + (IC_SETS - 1)*IC_BLOCK_SIZE + 6) + 
			REG16(add+ DC_BLOCK_SIZE - 16) + 
			REG16(add + 2*DC_BLOCK_SIZE + 14);
		add += DC_SETS*DC_BLOCK_SIZE;
        } 

	dc_disable();

	return ul;
}

int ic_test(void)
{
        int i;
        unsigned long base, addr;
 
        base = (((unsigned int)program_ram_end / (IC_SETS*IC_BLOCK_SIZE)) * 
		IC_SETS*IC_BLOCK_SIZE) + IC_SETS*IC_BLOCK_SIZE;
	//printf("ic_test\n");
	//printf("Test program from base at 0x%08x\n",(unsigned int)base);
        /* Copy jr to various location */
        addr = base;
        for(i = 0; i < IC_WAYS; i++) {
                copy_jr(addr);
                copy_jr(addr + 2*IC_BLOCK_SIZE + 4);
                copy_jr(addr + 4*IC_BLOCK_SIZE + 8);
                copy_jr(addr + 6*IC_BLOCK_SIZE + 12);
 
                copy_jr(addr + (IC_SETS - 2)*IC_BLOCK_SIZE + 0);
                copy_jr(addr + (IC_SETS - 4)*IC_BLOCK_SIZE + 4);
                copy_jr(addr + (IC_SETS - 6)*IC_BLOCK_SIZE + 8);
                copy_jr(addr + (IC_SETS - 8)*IC_BLOCK_SIZE + 12);
                addr += IC_SETS*IC_BLOCK_SIZE;
        }
 
        /* Load execution table which starts at address 4 
	   (at address 0 is table index) */
        addr = base;
        for(i = 0; i < IC_WAYS; i++) {
                /* Cache miss */
                jump_addr[15*i + 0] = addr + 2*IC_BLOCK_SIZE + 4;
                jump_addr[15*i + 1] = addr + 4*IC_BLOCK_SIZE + 8;
                jump_addr[15*i + 2] = addr + 6*IC_BLOCK_SIZE + 12;
                /* Cache hit/miss */
                jump_addr[15*i + 3] = addr;
                jump_addr[15*i + 4] = addr + (IC_SETS - 2)*IC_BLOCK_SIZE + 0;
                jump_addr[15*i + 5] = addr + 2*IC_BLOCK_SIZE + 4;
                jump_addr[15*i + 6] = addr + (IC_SETS - 4)*IC_BLOCK_SIZE + 4;
                jump_addr[15*i + 7] = addr + 4*IC_BLOCK_SIZE + 8;
                jump_addr[15*i + 8] = addr + (IC_SETS - 6)*IC_BLOCK_SIZE + 8;
                jump_addr[15*i + 9] = addr + 6*IC_BLOCK_SIZE + 12;
                jump_addr[15*i + 10] = addr + (IC_SETS - 8)*IC_BLOCK_SIZE + 12;
                /* Cache hit */
                jump_addr[15*i + 11] = addr + (IC_SETS - 2)*IC_BLOCK_SIZE + 0;
                jump_addr[15*i + 12] = addr + (IC_SETS - 4)*IC_BLOCK_SIZE + 4;
                jump_addr[15*i + 13] = addr + (IC_SETS - 6)*IC_BLOCK_SIZE + 8;
                jump_addr[15*i + 14] = addr + (IC_SETS - 8)*IC_BLOCK_SIZE + 12;

                addr += IC_SETS*IC_BLOCK_SIZE;
        }
 
        /* Go home */
	/* Warning - if using all 32 sets, the SET_MAX define above will need
	 to be incremented*/
        jump_addr[15*i] = (unsigned long)&jalr;
 
        /* Initilalize table index */
        jump_indx = (unsigned long)&jump_addr[0];
 
        ic_enable();

        /* Go */
        call(base);
 
        ic_disable();
 
        return 0;
}

/* Each of the 5 reports should be 0xdeaddead if the code is working
   correctly. */
int main(void)
{
	unsigned long rc, ret = 0;

	program_ram_end = (unsigned int)&_ram_end;
	program_ram_end += 4;


	/* Read UPR and configuration registers, extract cache settings */
	init_cache_config();

	if (dc_present)
	{
		report(0);
		
		rc = dc_test();
		
		ret += rc;

		report(rc + 0xdeaddca1);
		
		report(1);
		
		rc = dc_inv_test(MEM_RAM);
		
		ret += rc;
		
		report(rc + 0x9e8daa91);
	}

	if (ic_present)
	{
		report(2);
		
		rc = ic_test();
		
		ret += rc;
		
		report(rc + 0xdeaddead);

		report(3);
		
		ic_enable();
		
		report(4);
	
		rc = ic_inv_test();
		
		ret += rc;
		
		report(rc + 0xdeadde8f);
		
		report(ret + 0x9e8da867);
	}
	
	exit(0);

	return 0;

}

/* just for size calculation */
void dummy() 
{
}
