/* sim-config.h -- Simulator configuration header file

   Copyright (C) 1999 Damjan Lampret, lampret@opencores.org
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


#ifndef SIM_CONFIG_H
#define SIM_CONFIG_H

/* System includes */
#include <stdio.h>

/* Package includes */
#include "arch.h"

/* Simulator configuration macros. Eventually this one will be a lot bigger. */

#define MAX_SBUF_LEN     256	/* Max. length of store buffer */

#define EXE_LOG_HARDWARE   0	/* Print out RTL states */
#define EXE_LOG_SIMPLE     1	/* Executed log prints out dissasembly */
#define EXE_LOG_SOFTWARE   2	/* Simple with some register output */

#define STR_SIZE         256

/* Number of cycles between checks to runtime.sim.iprompt */
#define CHECK_INT_TIME 100000

/* Print to the defined output stream */
#define PRINTF(x...) fprintf (runtime.sim.fout, x)

/* Print only if we are not running silently */
#define PRINTFQ(x...) if (!config.sim.quiet) { fprintf (runtime.sim.fout, x); }

/*! Data structure for configuration data */
struct config
{
  struct
  {				/* External linkage for SystemC */
    void  *class_ptr;
    int  (*read_up) (void              *class_ptr,
		     unsigned long int  addr,
		     unsigned char      mask[],
		     unsigned char      rdata[],
		     int                data_len);
    int  (*write_up) (void              *class_ptr,
		      unsigned long int  addr,
		      unsigned char      mask[],
		      unsigned char      wdata[],
		      int                data_len);
  } ext;

  struct
  {
    int is_library;		/* Library operation */
    int debug;			/* Simulator debugging */
    int verbose;		/* Force verbose output */

    int profile;		/* Is profiler running */
    char *prof_fn;		/* Profiler filename */

    int mprofile;		/* Is memory profiler running */
    char *mprof_fn;		/* Memory profiler filename */

    int history;		/* instruction stream history analysis */
    int exe_log;		/* Print out RTL states? */
    int exe_log_type;		/* Type of log */
    long long int exe_log_start;	/* First instruction to log */
    long long int exe_log_end;	/* Last instr to log, -1 if continuous */
    int exe_log_marker;		/* If nonzero, place markers before */
    /* each exe_log_marker instructions */
    char *exe_log_fn;		/* RTL state comparison filename */
    int exe_bin_insn_log;       /* Binary dump log of exec. instructions */
    char* exe_bin_insn_log_fn;  /* Binary dump log of exec. instructions name */
    long clkcycle_ps;		/* Clock duration in ps */
    int strict_npc;		/* JPB. NPC flushes pipeline when changed */
    int  quiet;			/* Minimize message output */
    int  report_mem_errs;	/* Report out of memory accesses */
  } sim;

  struct
  {				/* Verification API */
    int enabled;		/* Whether is VAPI module enabled */
    int server_port;		/* user specified port for services */
    int log_enabled;		/* Whether to log the vapi requests */
    int hide_device_id;		/* Whether to log dev ID each request */
    char *vapi_fn;		/* vapi log filename */
  } vapi;

  struct
  {
    char *timings_fn;		/* Filename of the timing table */
    int memory_order;		/* Memory access stricness */
    int calling_convention;	/* Do funcs follow std calling conv? */
    int enable_bursts;		/* Whether burst are enabled */
    int no_multicycle;		/* Generate no multicycle paths */
  } cuc;

  struct
  {
    int superscalar;		/* superscalara analysis */
    int hazards;		/* dependency hazards analysis */
    int dependstats;		/* dependency statistics */
    int sbuf_len;		/* length of store buffer, 0=disabled */
    int hardfloat;      /* whether hardfloat is enabled */
  } cpu;

  struct
  {
    int enabled;		/* Whether data cache is enabled */
    int nways;			/* Number of DC ways */
    int nsets;			/* Number of DC sets */
    int blocksize;		/* DC entry size */
    int ustates;		/* number of DC usage states */
    int store_missdelay;	/* cycles a store miss costs */
    int store_hitdelay;		/* cycles a store hit costs */
    int load_missdelay;		/* cycles a load miss costs */
    int load_hitdelay;		/* cycles a load hit costs */
  } dc;

  struct pic
  {
    int enabled;		/* Is interrupt controller enabled? */
    int edge_trigger;		/* Are interrupts edge triggered? */
    int use_nmi;		/* Do we have non-maskable interrupts? */
  } pic;

  struct
  {
    int enabled;		/* Is power management operational? */
  } pm;

  struct
  {
    int enabled;                /* Are performance counters enabled? */
  } pcu;

  struct
  {
    int enabled;		/* branch prediction buffer analysis */
    int sbp_bnf_fwd;		/* Static BP for l.bnf uses fwd predn */
    int sbp_bf_fwd;		/* Static BP for l.bf uses fwd predn */
    int btic;			/* BP target insn cache analysis */
    int missdelay;		/* How much cycles does the miss cost */
    int hitdelay;		/* How much cycles does the hit cost */
  } bpb;

  struct
  {
    int enabled;		/* Is debug module enabled */
    int rsp_enabled;		/* Is RSP debugging with GDB possible */
    int rsp_port;		/* Port for RSP GDB connection */
    unsigned long vapi_id;	/* "Fake" vapi dev id for JTAG proxy */
    long int  jtagcycle_ps;	/* JTAG clock duration in ps */
  } debug;
};

/*! Data structure for run time data */
struct runtime
{
  struct
  {
    FILE *fprof;		/* Profiler file */
    FILE *fmprof;		/* Memory profiler file */
    FILE *fexe_log;		/* RTL state comparison file */
    FILE *fexe_bin_insn_log;	/* Binary instruction dump/log file */
    FILE *fout;			/* file for standard output */
    char *filename;		/* Original Command Simulator file (CZ) */
    int iprompt;		/* Interactive prompt */
    int iprompt_run;		/* Interactive prompt is running */
    long long cycles;		/* Cycles counts fetch stages */
    long long int end_cycles;	/* JPB. Cycles to end of quantum */
    double time_point;		/* JPB. Time point in the simulation */
    unsigned long int ext_int_set;	/* JPB. External interrupts to set */
    unsigned long int ext_int_clr;	/* DXL. External interrupts ti clear */

    int mem_cycles;		/* Each cycle has counter of mem_cycles;
				   this value is joined with cycles
				   at the end of the cycle; no sim
				   originated memory accesses should be
				   performed inbetween. */
    int loadcycles;		/* Load and store stalls */
    int storecycles;

    long long reset_cycles;

    int  hush;			/* Is simulator to do reg dumps */
    int  trace_phy;		/* Show physical instr addr when tracing */
    int  trace_virt;		/* Show virtual instr addr when tracing */
  } sim;

  struct
  {
    unsigned int  instr;	/* Current JTAG instruction */
    unsigned long int  mod_id;	/* Currently selected module */
    int  write_defined_p;	/* WRITE_COMMAND has set details for GO */
    unsigned char  acc_type;	/* Access type for GO */
    unsigned long int  addr;	/* Address to read/write for GO */
    unsigned long int  size;	/* Num bytes to read/write (up to 2^16) */
  } debug;

  struct
  {
    long long instructions;	/* Instructions executed */
    long long reset_instructions;

    int stalled;
    int halted;			/* When library hits exit. */
    int hazardwait;		/* how many cycles were wasted because of hazards */
    int supercycles;		/* Superscalar cycles */
  } cpu;

  struct
  {				/* Verification API, part of Adv Core Verif */
    int enabled;		/* Whether is VAPI module enabled */
    FILE *vapi_file;		/* vapi file */
    int server_port;		/* A user specified port number for services */
  } vapi;

/* CUC configuration parameters */
  struct
  {
    int mdelay[4];		/* average memory delays in cycles
				   {read single, read burst, write single, write burst} */
    double cycle_duration;	/* in ns */
  } cuc;
};

/*! Union of all possible paramter values */
union param_val
{
  char          *str_val;
  int            int_val;
  long long int  longlong_val;
  oraddr_t       addr_val;
};

/*! Enum of all possible paramter types */
enum param_t
{
  PARAMT_NONE = 0,		/* No parameter */
  PARAMT_STR,			/* String parm enclosed in double quotes (") */
  PARAMT_WORD,			/* String parm NOT enclosed in double quotes */
  PARAMT_INT,			/* Integer parameter */
  PARAMT_LONGLONG,		/* Long long int parameter */
  PARAMT_ADDR			/* Address parameter */
};

/* Generic structure for a configuration section */
struct config_section
{
  char *name;
  void *(*sec_start) (void);
  void (*sec_end) (void *);
  void *dat;
  struct config_param *params;
  struct config_section *next;
};

/* Externally visible data structures*/
extern struct config          config;
extern struct runtime         runtime;
extern struct config_section *cur_section;
extern int                    do_stats;

/* Prototypes for external use */
extern void  set_config_command (int argc, char **argv);
extern void  init_defconfig (void);
extern int   parse_args (int argc, char *argv[]);
extern void  print_config (void);
extern void  reg_config_param (struct config_section *sec,
			       const char            *param,
			       enum param_t           type,
			       void (*param_cb)  (union param_val,
						  void *));
extern struct config_section *reg_config_sec (const char *section,
					      void *(*sec_start) (void),
					      void  (*sec_end) (void *));

extern void  reg_config_secs ();

#endif /* SIM_CONFIG_H */
