
			========== WARNING ==========

This file is now obsolete. It is retained as a historical record of early
versions of Or1ksim and GDB for OpenRISC 1000. Consult the GDB 6.8
documentation for up to date advice on using GDB with Or1ksim.

		     ========== End of WARNING ==========


Originally by Chris Ziomkowski <chris@asics.ws>
Some Additions by Heiko Panther <heiko.panther@web.de>

Brief introduction to using GDB based debugging with or1ksim

GDB uses the JTAG proxy server included in or1ksim to communicate
directly with the simulator. Only 1 connection is allowed to the
proxy server at a time. Attempting a second connection will terminate
the previous connection. This is very useful when the gdb or1k
process terminates abnormally (such as when you are debugging the
debugger.) In this case it is impossible to notify the JTAG server
that the socket has shut down, and therefore it will assume that a
new connection implies the termination of the previous process.

The or1ksim will start the JTAG proxy server on the port specified
for service "jtag". If such a service is not specified, the server
will choose a random port number. This behavior can be overridden by
specifying the port number to the simulator using the -srv option.
As an example, "sim -srv 9999" starts up the simulator with the
JTAG proxy server on port 9999. This behavior is useful for those
people who do not have root access and can not add services to
the config files (such as university students operating in a shared
environment.)

As the JTAG proxy server runs only if there is data available for
reading, there is very little resource usage consumed by this
capability. However, in certain instances where gdb is not being
utilized, it is possible to disable the JTAG proxy server
entirely.  This will recover the few cycles necessary for the
poll() system call. (Tests indicate this has a negligible to
non existant impact on speed, however your mileage may vary.)
This behavior can be achieved by starting the simulator with
the command "sim -nosrv."

At startup, the simulator will execute random commands, just as
a real chip would do performing in this environment if the memory
was not initialized. If it is desired to simulate a ROM or FLASH
environment, these can be approximated by using the -loadmem option
to the simulator. For example, to simulate a 32k flash at location
0x8000, the command "sim -loadmem@0x8000 <filename>" could be
used, where <filename> represents the name of the initialization
file. If the optional '@0x8000' flag is left off of the loadmem
statement, then the load will occur at location 0. Several loadmem
flags can appear on the command line to simulate different
memory blocks.

It is also possible to initialize all RAM to a predefined value,
which is usually 0x00000000 or 0xFFFFFFFF. This would be equivalent
to what is normally observed in a real world environment. However,
specific sequences are possible in case this is necessary. Alternatively,
random values can be assigned to memory, to check the behavior of a
process under different conditions. All of these options can be
handled by the "-initmem" option of the simulator. The following
command would startup the simulator with all memory initialized to
"1":

sim -initmem 0xFFFFFFFF

while this command would generate random values:

sim -initmem random

Once the simulator is started, it will print out something like
the following:

> bash-2.03$ sim
> JTAG Proxy server started on port 42240
> Machine initialization...
> Data cache tag: physical
> Insn cache tag: physical
> BPB simulation on.
> BTIC simulation on.
> Clock cycle: 4 ns
> RAM: 0x0 to 0x7aa80 (490 KB)
> 
> simdebug off, interactive prompt off
> Building automata... done, num uncovered: 0/216.
> Parsing operands data... done.
> Resetting 4 UART(s).
> UART0 has problems with RX file stream.
> Resetting Tick Timer.
> Resetting Power Management.
> Resetting PIC.
> Exception 0x100 (Reset): Iqueue[0].insn_addr: 0x0  Eff ADDR: 0x0
>  pc: 0x0  pcnext: 0x4
>

Note that because we did not specify a server port, a random
port (42240) was selected for us (The "jtag" service does not
exist on this machine). We will need this value to create the
connection URL for the target command. It is now possible to
debug a program with gdb as follows:

> bash-2.03$ gdb
> GNU gdb 5.0
> Copyright 2000 Free Software Foundation, Inc.
> GDB is free software, covered by the GNU General Public License, and you are
> welcome to change it and/or distribute copies of it under certain conditions.
> Type "show copying" to see the conditions.
> There is absolutely no warranty for GDB.  Type "show warranty" for details.
> This GDB was configured as "--host=sparc-sun-solaris2.7 --target=or32-rtems".
> (or1k) file "dhry.or32"
> Reading symbols from dhry.or32...done.
> (or1k) target jtag jtag://localhost:42240
> Remote or1k debugging using jtag://localhost:42240
> 0x0 in ?? ()
> (or1k) load dhry.or32
> Loading section .text, size 0x14fc lma 0x100
> Loading section .data, size 0x2804 lma 0x15fc
> Start address 0x100 , load size 15616
> Transfer rate: 124928 bits/sec, 488 bytes/write.
> (or1k) b main
> Breakpoint 1 at 0x51c: file dhry.c, line 176.
> (or1k) run
> Starting program: /usr3/home/chris/opencores/or1k/gdb-build/gdb/dhry.or32 
> 
> Breakpoint 1, main () at dhry.c:176
> 176       Next_Ptr_Glob = (Rec_Pointer) &x;
> (or1k) 

The simulator window will have printed out the following, showing that
a breakpoint exception was asserted.

> Exception 0xd00 (Break): Iqueue[0].insn_addr: 0x51c  Eff ADDR: 0x0
> pc: 0x51c  pcnext: 0x520

Note that when the "run" command is given, the simulator will start
by jumping to the reset vector at location 0x100. You must start off
by placing a small bootloader at this location. A simple environment
capable of running C programs can be established by placing the
following code in a file called "start.s" and linking it to your
executable. As an example, the following will work. The file start.s
was derived from the output of a file start.c compiled by gcc:

or32-rtems-gcc -g -c -o start.o start.s
or32-rtems-gcc -g -c -DOR1K -o dhry.o dhry.c
or32-rtems-ld -Ttext 0x0 -o dhry.or32 start.o dhry.o

---------------------- CUT HERE -------------------------

# file start.s
.file   "start.s"

# This is the general purpose start routine. It
# sets up the stack register, and jumps to the
# _main program location. It should be linked at
# the start of all programs.

.text
        .align	4
	.org	0x100			# The reset routine goes at 0x100
.proc _rst
        .def    _rst
        .val    _rst
        .scl    2
        .type   041
        .endef
        .global _rst
_rst:
        .def    .bf
        .val    .
        .scl    101
        .endef
	l.addi		r1,r0,0x7f00	# Set STACK to value 0x7f00
	l.addi		r2,r1,0x0	# FRAME and STACK are the same
	l.mfspr		r3,r0,17	# Get SR value
	l.ori		r3,r3,2		# Set exception enable bit
	l.jal   	_main		# Jump to main routine
	l.mtspr		r0,r3,17	# Enable exceptions (DELAY SLOT)

.endproc _rst
        .def    _rst
        .val    .
        .scl    -1
        .endef

	.org	0xFFC
	l.nop				# Guarantee the exception vector space
					# does not have general purpose code

# C code starts at 0x1000

---------------------- CUT HERE -------------------------


Setting registers

"info spr" commands give info about special purpose registers, "spr" commands set them.
"info spr" - display the SPR groups
"info spr <group>" - display SPRs in <group>
"info spr <spr>" - display value in <spr>
"spr <spr> <value>" - set <spr> to <value>

Breaking for exceptions

You have to set a bit in the Debug Stop Register "dsr" for each exception you want 
to stop on. Use "spr dsr <value>".


			========== WARNING ==========

This file is now obsolete. It is retained as a historical record of early
versions of Or1ksim and GDB for OpenRISC 1000. Consult the GDB 6.8
documentation for up to date advice on using GDB with Or1ksim.

		     ========== End of WARNING ==========
