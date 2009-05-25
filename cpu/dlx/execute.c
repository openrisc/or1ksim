/* execute.c -- DLX dependent simulation
   Copyright (C) 1999 Damjan Lampret, lampret@opencores.org

This file is part of OpenRISC 1000 Architectural Simulator.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA. */

/* Most of the DLX simulation is done in this file. */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "arch.h"

#include "branch_predict.h"
#include "abstract.h"
#include "parse.h"
#include "trace.h"
#include "execute.h"
#include "stats.h"

/* General purpose registers. */
machword reg[MAX_GPRS];

/* Instruction queue */
struct iqueue_entry iqueue[20];

/* Benchmark multi issue execution */
int multissue[20];
int supercycles;

/* Load and store stalls */
int loadcycles, storecycles;

/* Result forwarding stall cycles */
int forwardingcycles;

/* Completition queue */
struct icomplet_entry icomplet[20];

/* Program counter */
unsigned long pc;

/* Temporary program counter */
unsigned long pctemp;

/* Cycles counts fetch stages */
int cycles;

/* Implementation specific.
   Get an actual value of a specific register. */

machword eval_reg(char *regstr)
{
	int regno;
	
	regno = atoi(regstr + 1);

	if (regno < MAX_GPRS)
		return reg[regno];
	else {
		PRINTF("\nABORT: read out of registers\n");
		cont_run = 0;
		return 0;
	}
}

/* Implementation specific.
   Set a specific register with value. */

void set_reg32(char *regstr, unsigned long value)
{
	int regno;
	
	regno = atoi(regstr + 1);

	if (regno == 0)		/* gpr0 is always zero */
		value = 0;
		
	if (regno < MAX_GPRS)
		reg[regno] = value;
	else {
		PRINTF("\nABORT: write out of registers\n");
		cont_run = 0;
	}

	return;
}

/* Does srcoperand depend on computation of dstoperand? Return
   non-zero if yes.

 Cycle t		 Cycle t+1
dst: irrelevant		src: immediate			always 0
dst: reg1 direct	src: reg2 direct		0 if reg1 != reg2
dst: reg1 disp		src: reg2 direct		always 0
dst: reg1 direct	src: reg2 disp			0 if reg1 != reg2
dst: reg1 disp		src: reg2 disp 			always 1 (store must
							finish before load)
*/

int depend_operands(char *dstoperand, char *srcoperand)
{
	char dst[OPERANDNAME_LEN];
	char src[OPERANDNAME_LEN];
	
	if (!srcoperand)
		return 0;

	if (!dstoperand)
		return 0;
		
	strcpy(dst, dstoperand);
	strcpy(src, srcoperand);

	if (*src == '#')		/* immediate */
		return 0;
	else
	if (!strstr(src, "(")) 
		if (*src == 'r') {	/* src: reg direct */
			if (!strstr(dst, "("))
				if (*dst == 'r')
					if (strcmp(dst, src) == 0)
						return 1; /* dst: reg direct */
					else
						return 0; /* dst: reg direct */
				else
					return 0; /* dst: addr */
			else
				return 0; /* dst: reg disp */
		} else
			return 0;	/* src: addr */
	else {				/* src: register disp */
		char *regstr;
		
		regstr = strstr(src, "(r") + 1;	/* regstr == "rXX)" */
		*strstr(regstr, ")") = '\0';		/* regstr == "rXX" */

		if (!strstr(dst, "("))
			if (*dst == 'r')
				if (strcmp(dst, regstr) == 0)
					return 1; /* dst: reg direct */
				else
					return 0; /* dst: reg direct */
			else
				return 0; /* dst: addr */
		else
			return 1; /* dst: reg disp */
	}	

	return 0;
}

/* Implementation specific.
   Get an actual value represented by operand (register direct, register
   indirect (with displacement), immediate etc.).
   
   #n		- immediate n
   rXX		- register direct
   XX		- relative or absolute address (labels)
   n(XX) 	- register indirect (with displacement) */
   
machword eval_operand(char *srcoperand,int* breakpoint)
{
	char operand[OPERANDNAME_LEN];
	
	strcpy(operand, srcoperand);

	if (*operand == '#')		/* immediate */
		return strtoul(&operand[1], NULL, 0);
	else
	if (!strstr(operand, "(")) 	/* not indirect but ...*/
		if (*operand == 'r') 	/* ... register direct */
			return eval_reg(operand);
		
		else 			/* ... rel. or abs. address */
			return eval_label(operand);
	else {				/* register indirect */
		int disp;		/* with possible displacement */
		char *regstr;
		unsigned int memaddr;
		
		disp = atoi(operand);	/* operand == "nn(rXX)" */
		regstr = strstr(operand, "(r") + 1;	/* regstr == "rXX)" */
		*strstr(regstr, ")") = '\0';		/* regstr == "rXX" */
		memaddr = eval_reg(regstr) + disp;
		
		return eval_mem32(memaddr,breakpoint);
	}
	
	return 0;
}

/* Implementation specific.
   Set destination operand (register direct, register indirect
   (with displacement) with value. */
   
void set_operand(char *dstoperand, unsigned long value,int* breakpoint)
{
	char operand[OPERANDNAME_LEN];
	
	strcpy(operand, dstoperand);

	if (*operand == '#')		/* immediate */
		PRINTF("INTERNAL ERROR: Can't set immediate operand.\n");
	else
	if (!strstr(operand, "(")) 	/* not indirect but ...*/
		if (*operand == 'r') 	/* ... register direct */
			set_reg32(operand, value);
		else 			/* ... rel. or abs. address */
			PRINTF("INTERNAL ERROR: Can't set addr operand.\n");
	else {				/* register indirect */
		int disp;		/* with possible displacement */
		char *regstr;
		unsigned int memaddr;
		
		disp = atoi(operand);	/* operand == "nn(rXX)" */
		regstr = strstr(operand, "(r") + 1;	/* regstr == "rXX)" */
		*strstr(regstr, ")") = '\0';		/* regstr == "rXX" */
		memaddr = eval_reg(regstr) + disp;
		
		set_mem32(memaddr, value, breakpoint);
	}
	
	return;
}

void reset()
{
	cycles = 0;
	supercycles = 0;
	loadcycles = 0;
	storecycles = 0;
	forwardingcycles = 0;
	memset(reg, 0, sizeof(reg));
	memset(iqueue, 0, sizeof(iqueue));
	memset(icomplet, 0, sizeof(icomplet));
	pctemp = eval_label("_main");
	pc = pctemp;
	set_reg32(STACK_REG , MEMORY_LEN - STACK_SIZE);
}

void fetch()
{
	/* Cycles after reset. */
	cycles++;

	/* Simulate instruction cache */
	ic_simulate(pc);
	
	/* Fetch instruction. */
	strcpy(iqueue[0].insn, mem[pc].insn->insn);
	strcpy(iqueue[0].op1, mem[pc].insn->op1);
	strcpy(iqueue[0].op2, mem[pc].insn->op2);
	strcpy(iqueue[0].op3, mem[pc].insn->op3);
	iqueue[0].insn_addr = pc;
	iqueue[0].dependdst = NULL;
	iqueue[0].dependsrc1 = NULL;
	iqueue[0].dependsrc2 = NULL;

	/* Increment program counter. */
	pc = pctemp;
	pctemp += 4;
	
	/* Check for breakpoint. */
	if (mem[pc].brk)
		cont_run = 0;	/* Breakpoint set. */
		
	return;
}

void decode(struct iqueue_entry *cur)
{
  int breakpoint = 0;

	cur->dependdst = cur->op1;
	cur->dependsrc1 = cur->op2;	/* for calculating register */
	cur->dependsrc2 = cur->op3;	/* dependency */
	
	cur->func_unit = unknown;
	
	if (strcmp(cur->insn, "sw") == 0) {
		cur->func_unit = store;
		set_operand(cur->op1, eval_operand(cur->op2,&breakpoint),&breakpoint);
	} else
	if (strcmp(cur->insn, "sb") == 0) {
		cur->func_unit = store;
		set_operand(cur->op1, (eval_operand(cur->op2,&breakpoint) << 24) + (eval_operand(cur->op1,&breakpoint) & 0xffffff),&breakpoint);
	} else
	if (strcmp(cur->insn, "sh") == 0) {
		cur->func_unit = store;
		set_operand(cur->op1, (eval_operand(cur->op2,&breakpoint) << 16) + (eval_operand(cur->op1,&breakpoint) & 0xffff),&breakpoint);
	} else
	if (strcmp(cur->insn, "lw") == 0) {
		cur->func_unit = load;
		set_operand(cur->op1, eval_operand(cur->op2,&breakpoint),&breakpoint);
	} else
	if (strcmp(cur->insn, "lb") == 0) {
		signed char temp = (eval_operand(cur->op2,&breakpoint) >> 24);
		cur->func_unit = load;
		set_operand(cur->op1, temp,&breakpoint);
	} else
	if (strcmp(cur->insn, "lbu") == 0) {
		unsigned char temp = (eval_operand(cur->op2,&breakpoint) >> 24);
		cur->func_unit = load;
		set_operand(cur->op1, temp,&breakpoint);
	} else
	if (strcmp(cur->insn, "lh") == 0) {
		signed short temp = (eval_operand(cur->op2,&breakpoint) >> 16);
		cur->func_unit = load;
		set_operand(cur->op1, temp,&breakpoint);
	} else
	if (strcmp(cur->insn, "lhu") == 0) {
		unsigned short temp = (eval_operand(cur->op2,&breakpoint) >> 16);
		cur->func_unit = load;
		set_operand(cur->op1, temp,&breakpoint);
	} else
	if (strcmp(cur->insn, "lwi") == 0) {
		cur->func_unit = movimm;
		set_operand(cur->op1, eval_operand(cur->op2,&breakpoint),&breakpoint);
	} else
	if (strcmp(cur->insn, "lhi") == 0) {
		cur->func_unit = movimm;
		set_operand(cur->op1, eval_operand(cur->op2,&breakpoint) << 16,&breakpoint);
	} else
	if (strcmp(cur->insn, "and") == 0) {
		cur->func_unit = arith;
		set_operand(cur->op1, eval_operand(cur->op2,&breakpoint) & eval_operand(cur->op3,&breakpoint),&breakpoint);
	} else
	if (strcmp(cur->insn, "andi") == 0) {
		cur->func_unit = arith;
		set_operand(cur->op1, eval_operand(cur->op2,&breakpoint) & eval_operand(cur->op3,&breakpoint),&breakpoint);
	} else
	if (strcmp(cur->insn, "or") == 0) {
		cur->func_unit = arith;
		set_operand(cur->op1, eval_operand(cur->op2,&breakpoint) | eval_operand(cur->op3,&breakpoint),&breakpoint);
	} else
	if (strcmp(cur->insn, "ori") == 0) {
		cur->func_unit = arith;
		set_operand(cur->op1, eval_operand(cur->op2,&breakpoint) | eval_operand(cur->op3,&breakpoint),&breakpoint);
	} else
	if (strcmp(cur->insn, "xor") == 0) {
		cur->func_unit = arith;
		set_operand(cur->op1, eval_operand(cur->op2,&breakpoint) ^ eval_operand(cur->op3,&breakpoint),&breakpoint);
	} else
	if (strcmp(cur->insn, "add") == 0) {
		signed long temp3, temp2, temp1;
		signed char temp4;

		cur->func_unit = arith;
		temp3 = eval_operand(cur->op3,&breakpoint);
		temp2 = eval_operand(cur->op2,&breakpoint);
		temp1 = temp2 + temp3;
		set_operand(cur->op1, temp1,&breakpoint);
		
		temp4 = temp1;
		if (temp4 == temp1)
			mstats.byteadd++;
	} else
	if (strcmp(cur->insn, "addi") == 0) {
		signed long temp3, temp2, temp1;
		signed char temp4;
		
		cur->func_unit = arith;
		temp3 = eval_operand(cur->op3,&breakpoint);
		temp2 = eval_operand(cur->op2,&breakpoint);
		temp1 = temp2 + temp3;
		set_operand(cur->op1, temp1,&breakpoint);
		
		temp4 = temp1;
		if (temp4 == temp1)
			mstats.byteadd++;
	} else
	if (strcmp(cur->insn, "addui") == 0) {
		unsigned long temp3, temp2, temp1;
		unsigned char temp4;
		
		cur->func_unit = arith;
		temp3 = eval_operand(cur->op3,&breakpoint);
		temp2 = eval_operand(cur->op2,&breakpoint);
		temp1 = temp2 + temp3;
		set_operand(cur->op1, temp1,&breakpoint);
		
		temp4 = temp1;
		if (temp4 == temp1)
			mstats.byteadd++;
	} else
	if (strcmp(cur->insn, "sub") == 0) {
		signed long temp3, temp2, temp1;

		cur->func_unit = arith;
		temp3 = eval_operand(cur->op3,&breakpoint);
		temp2 = eval_operand(cur->op2,&breakpoint);
		temp1 = temp2 - temp3;
		set_operand(cur->op1, temp1,&breakpoint);
	} else
	if (strcmp(cur->insn, "subui") == 0) {
		unsigned long temp3, temp2, temp1;

		cur->func_unit = arith;
		temp3 = eval_operand(cur->op3,&breakpoint);
		temp2 = eval_operand(cur->op2,&breakpoint);
		temp1 = temp2 - temp3;
		set_operand(cur->op1, temp1,&breakpoint);
	} else
	if (strcmp(cur->insn, "subi") == 0) {
		signed long temp3, temp2, temp1;

		cur->func_unit = arith;
		temp3 = eval_operand(cur->op3,&breakpoint);
		temp2 = eval_operand(cur->op2,&breakpoint);
		temp1 = temp2 - temp3;
		set_operand(cur->op1, temp1,&breakpoint);
	} else
	if (strcmp(cur->insn, "mul") == 0) {
		signed long temp3, temp2, temp1;

		cur->func_unit = arith;
		temp3 = eval_operand(cur->op3,&breakpoint);
		temp2 = eval_operand(cur->op2,&breakpoint);
		temp1 = temp2 * temp3;
		set_operand(cur->op1, temp1,&breakpoint);
	} else
	if (strcmp(cur->insn, "div") == 0) {
		signed long temp3, temp2, temp1;

		cur->func_unit = arith;
		temp3 = eval_operand(cur->op3,&breakpoint);
		temp2 = eval_operand(cur->op2,&breakpoint);
		temp1 = temp2 / temp3;
		set_operand(cur->op1, temp1,&breakpoint);
	} else
	if (strcmp(cur->insn, "divu") == 0) {
		unsigned long temp3, temp2, temp1;

		cur->func_unit = arith;
		temp3 = eval_operand(cur->op3,&breakpoint);
		temp2 = eval_operand(cur->op2,&breakpoint);
		temp1 = temp2 / temp3;
		set_operand(cur->op1, temp1,&breakpoint);
	} else
	if (strcmp(cur->insn, "slli") == 0) {
		cur->func_unit = shift;
		set_operand(cur->op1, eval_operand(cur->op2,&breakpoint) << eval_operand(cur->op3,&breakpoint),&breakpoint);
	} else
	if (strcmp(cur->insn, "sll") == 0) {
		cur->func_unit = shift;
		set_operand(cur->op1, eval_operand(cur->op2,&breakpoint) << eval_operand(cur->op3,&breakpoint),&breakpoint);
	} else
	if (strcmp(cur->insn, "srl") == 0) {
		cur->func_unit = shift;
		set_operand(cur->op1, eval_operand(cur->op2,&breakpoint) >> eval_operand(cur->op3,&breakpoint),&breakpoint);
	} else
	if (strcmp(cur->insn, "srai") == 0) {
		cur->func_unit = shift;
		set_operand(cur->op1, (signed)eval_operand(cur->op2,&breakpoint) / (1 << eval_operand(cur->op3,&breakpoint)),&breakpoint);
	} else
	if (strcmp(cur->insn, "sra") == 0) {
		cur->func_unit = shift;
		set_operand(cur->op1, (signed)eval_operand(cur->op2,&breakpoint) / (1 << eval_operand(cur->op3,&breakpoint)),&breakpoint);
	} else
	if (strcmp(cur->insn, "jal") == 0) {
		cur->func_unit = jump;
		pctemp = eval_operand(cur->op1,&breakpoint);
                set_reg32(LINK_REG, pc + 4);
	} else
	if (strcmp(cur->insn, "jr") == 0) {
		cur->func_unit = jump;
		cur->dependsrc1 = cur->op1;
		pctemp = eval_operand(cur->op1,&breakpoint);
	} else
	if (strcmp(cur->insn, "j") == 0) {
		cur->func_unit = jump;
		pctemp = eval_operand(cur->op1,&breakpoint);
	} else
	if (strcmp(cur->insn, "nop") == 0) {
		cur->func_unit = nop;	
	} else
	if (strcmp(cur->insn, "beqz") == 0) {
		cur->func_unit = branch;
		cur->dependsrc1 = cur->op1;
		if (eval_operand(cur->op1,&breakpoint) == 0) {
			pctemp = eval_operand(cur->op2,&breakpoint);
			mstats.beqz.taken++;
			bpb_update(cur->insn_addr, 1);
			btic_update(pctemp);
		} else {
			mstats.beqz.nottaken++;
			bpb_update(cur->insn_addr, 0);
			btic_update(pc);
		}
	} else
	if (strcmp(cur->insn, "bnez") == 0) {
		cur->func_unit = branch;
		cur->dependsrc1 = cur->op1;
		if (eval_operand(cur->op1,&breakpoint) != 0) {
			pctemp = eval_operand(cur->op2,&breakpoint);
			mstats.bnez.taken++;
			bpb_update(cur->insn_addr, 1);
			btic_update(pctemp);
		} else {
			mstats.bnez.nottaken++;
			bpb_update(cur->insn_addr, 0);
			btic_update(pc);
		}
	} else
	if (strcmp(cur->insn, "seq") == 0) {
		cur->func_unit = compare;
		if (eval_operand(cur->op2,&breakpoint) == eval_operand(cur->op3,&breakpoint))
			set_operand(cur->op1, 1,&breakpoint);
		else
			set_operand(cur->op1, 0,&breakpoint);
	} else
	if (strcmp(cur->insn, "snei") == 0) {
		cur->func_unit = compare;
		if (eval_operand(cur->op2,&breakpoint) != eval_operand(cur->op3,&breakpoint))
			set_operand(cur->op1, 1,&breakpoint);
		else
			set_operand(cur->op1, 0,&breakpoint);
	} else
	if (strcmp(cur->insn, "sne") == 0) {
		cur->func_unit = compare;
		if (eval_operand(cur->op2,&breakpoint) != eval_operand(cur->op3,&breakpoint))
			set_operand(cur->op1, 1,&breakpoint);
		else
			set_operand(cur->op1, 0,&breakpoint);
	} else
	if (strcmp(cur->insn, "seqi") == 0) {
		cur->func_unit = compare;
		if (eval_operand(cur->op2,&breakpoint) == eval_operand(cur->op3,&breakpoint))
			set_operand(cur->op1, 1,&breakpoint);
		else
			set_operand(cur->op1, 0,&breakpoint);
	} else
	if (strcmp(cur->insn, "sgt") == 0) {
		cur->func_unit = compare;
		if ((signed)eval_operand(cur->op2,&breakpoint) >
		    (signed)eval_operand(cur->op3,&breakpoint))
			set_operand(cur->op1, 1,&breakpoint);
		else
			set_operand(cur->op1, 0,&breakpoint);
	} else
	if (strcmp(cur->insn, "sgtui") == 0) {
		cur->func_unit = compare;
		if ((unsigned)eval_operand(cur->op2,&breakpoint) >
		    (unsigned)eval_operand(cur->op3,&breakpoint))
			set_operand(cur->op1, 1,&breakpoint);
		else
			set_operand(cur->op1, 0,&breakpoint);
	} else
	if (strcmp(cur->insn, "sgeui") == 0) {
		cur->func_unit = compare;
		if ((unsigned)eval_operand(cur->op2,&breakpoint) >=
		    (unsigned)eval_operand(cur->op3,&breakpoint))
			set_operand(cur->op1, 1,&breakpoint);
		else
			set_operand(cur->op1, 0,&breakpoint);
	} else
	if (strcmp(cur->insn, "sgei") == 0) {
		cur->func_unit = compare;
		if ((signed)eval_operand(cur->op2,&breakpoint) >= (signed)eval_operand(cur->op3,&breakpoint))
			set_operand(cur->op1, 1,&breakpoint);
		else
			set_operand(cur->op1, 0,&breakpoint);
	} else
	if (strcmp(cur->insn, "sgti") == 0) {
		cur->func_unit = compare;
		if ((signed)eval_operand(cur->op2,&breakpoint) >
		    (signed)eval_operand(cur->op3,&breakpoint))
			set_operand(cur->op1, 1,&breakpoint);
		else
			set_operand(cur->op1, 0,&breakpoint);
	} else
	if (strcmp(cur->insn, "slt") == 0) {
		cur->func_unit = compare;
		if ((signed)eval_operand(cur->op2,&breakpoint) <
		    (signed)eval_operand(cur->op3,&breakpoint))
			set_operand(cur->op1, 1,&breakpoint);
		else
			set_operand(cur->op1, 0,&breakpoint);
	} else
	if (strcmp(cur->insn, "slti") == 0) {
		cur->func_unit = compare;
		if ((signed)eval_operand(cur->op2,&breakpoint) <
		    (signed)eval_operand(cur->op3,&breakpoint))
			set_operand(cur->op1, 1,&breakpoint);
		else
			set_operand(cur->op1, 0,&breakpoint);
	} else
	if (strcmp(cur->insn, "sle") == 0) {
		cur->func_unit = compare;
		if ((signed)eval_operand(cur->op2,&breakpoint) <=
		    (signed)eval_operand(cur->op3,&breakpoint))
			set_operand(cur->op1, 1,&breakpoint);
		else
			set_operand(cur->op1, 0,&breakpoint);
	} else
	if (strcmp(cur->insn, "slei") == 0) {
		cur->func_unit = compare;
		if ((signed)eval_operand(cur->op2,&breakpoint) <=
		    (signed)eval_operand(cur->op3,&breakpoint))
			set_operand(cur->op1, 1,&breakpoint);
		else
			set_operand(cur->op1, 0,&breakpoint);
	} else
	if (strcmp(cur->insn, "sleui") == 0) {
		cur->func_unit = compare;
		if ((unsigned)eval_operand(cur->op2,&breakpoint) <=
		    (unsigned)eval_operand(cur->op3,&breakpoint))
			set_operand(cur->op1, 1,&breakpoint);
		else
			set_operand(cur->op1, 0,&breakpoint);
	} else
	if (strcmp(cur->insn, "sleu") == 0) {
		cur->func_unit = compare;
		if ((unsigned)eval_operand(cur->op2,&breakpoint) <=
		    (unsigned)eval_operand(cur->op3,&breakpoint))
			set_operand(cur->op1, 1,&breakpoint);
		else
			set_operand(cur->op1, 0,&breakpoint);
	} else
	if (strcmp(cur->insn, "simrdtsc") == 0) {
		set_operand(cur->op1, cycles+loadcycles+storecycles+forwardingcycles,&breakpoint);
	} else
	if (strcmp(cur->insn, "simprintf") == 0) {
		unsigned long stackaddr;
		
		stackaddr = eval_reg(FRAME_REG);
		simprintf(stackaddr, 0);
		/* PRINTF("simprintf %x %x %x\n", stackaddr, fmtaddr, args); */
	} else {
		PRINTF("\nABORT: illegal opcode %s ", cur->insn);
		PRINTF("at %.8lx\n", cur->insn_addr);
		cont_run = 0;
	}

	/* Dynamic, dependency stats. */
	adddstats(icomplet[0].insn, iqueue[0].insn, 1, check_depend());

	/* Dynamic, functional units stats. */
	addfstats(icomplet[0].func_unit, iqueue[0].func_unit, 1, check_depend());

	/* Dynamic, single stats. */
	addsstats(iqueue[0].insn, 1, 0);

	if (cur->func_unit == store) 
		storecycles += 0;

	if (cur->func_unit == load) 
		loadcycles += 0;

	if (check_depend())
		forwardingcycles += 0;
		
	/* Pseudo multiple issue benchmark */
	if ((multissue[cur->func_unit] == 0) || (check_depend())) {
		int i;
		for (i = 0; i < 20; i++)
			multissue[i] = 9;
		supercycles++;
		multissue[arith] = 9;
		multissue[store] = 9;
		multissue[load] = 9;
	}
	multissue[cur->func_unit]--;
	
	return;
}

void execute()
{
	int i;
	
	/* Here comes real execution someday... */
	
	/* Instruction waits in completition buffer until retired. */
	strcpy(icomplet[0].insn, iqueue[0].insn);
	strcpy(icomplet[0].op1, iqueue[0].op1);
	strcpy(icomplet[0].op2, iqueue[0].op2);
	strcpy(icomplet[0].op3, iqueue[0].op3);
	icomplet[0].func_unit = iqueue[0].func_unit;	
	icomplet[0].insn_addr = iqueue[0].insn_addr;

	if (iqueue[0].dependdst == iqueue[0].op1)
		icomplet[0].dependdst = icomplet[0].op1;
	else
	if (iqueue[0].dependdst == iqueue[0].op2)
		icomplet[0].dependdst = icomplet[0].op2;
	else
	if (iqueue[0].dependdst == iqueue[0].op3)
		icomplet[0].dependdst = icomplet[0].op3;
	else
		icomplet[0].dependdst = NULL;

	if (iqueue[0].dependsrc1 == iqueue[0].op1)
		icomplet[0].dependsrc1 = icomplet[0].op1;
	else
	if (iqueue[0].dependsrc1 == iqueue[0].op2)
		icomplet[0].dependsrc1 = icomplet[0].op2;
	else
	if (iqueue[0].dependsrc1 == iqueue[0].op3)
		icomplet[0].dependsrc1 = icomplet[0].op3;
	else
		icomplet[0].dependsrc1 = NULL;

	if (iqueue[0].dependsrc2 == iqueue[0].op1)
		icomplet[0].dependsrc2 = icomplet[0].op1;
	else
	if (iqueue[0].dependsrc2 == iqueue[0].op2)
		icomplet[0].dependsrc2 = icomplet[0].op2;
	else
	if (iqueue[0].dependsrc2 == iqueue[0].op3)
		icomplet[0].dependsrc2 = icomplet[0].op3;
	else
		icomplet[0].dependsrc2 = NULL;
	
	/* History of execution */
	for (i = HISTEXEC_LEN - 1; i; i--)
		histexec[i] = histexec[i - 1];
	histexec[0] = icomplet[0].insn_addr;	/* add last insn */
	
	return;
}

void dumpreg()
{
	int i;
	
	PRINTF("\n\nIQ[0]:");
	dumpmemory(iqueue[0].insn_addr, iqueue[0].insn_addr + 4);
	PRINTF(" (just executed)\tCYCLES: %u \nSuperscalar CYCLES: %u\n", cycles, supercycles);
	PRINTF("Additional LOAD CYCLES: %u  STORE CYCLES: %u\n", loadcycles, storecycles);
	PRINTF("Additional RESULT FORWARDING CYCLES: %u\nPC:", forwardingcycles);
	dumpmemory(pc, pc + 4);
	PRINTF(" (next insn)");
	for(i = 0; i < MAX_GPRS; i++) {
		if (i % 4 == 0)
			PRINTF("\n");
		PRINTF("GPR%.2u: %.8lx  ", i, reg[i]);
	}
}
