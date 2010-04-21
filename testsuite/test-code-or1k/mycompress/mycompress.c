/* mycompress.c. Test of Or1ksim compression program

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

#include <stdarg.h>
#include "support.h"

#define BYTES_TO_COMPRESS 1000

/* 
 * Compress - data compression program 
 */
#define	min(a,b)	((a>b) ? b : a)

/*
 * machine variants which require cc -Dmachine:  pdp11, z8000, pcxt
 */

/*
 * Set USERMEM to the maximum amount of physical user memory available
 * in bytes.  USERMEM is used to determine the maximum BITS that can be used
 * for compression.
 *
 * SACREDMEM is the amount of physical memory saved for others; compress
 * will hog the rest.
 */
#ifndef SACREDMEM
#define SACREDMEM	0
#endif

/* #ifndef USERMEM */
#define USERMEM 	60000	/* default user memory */
/* #endif */

#ifdef interdata		/* (Perkin-Elmer) */
#define SIGNED_COMPARE_SLOW	/* signed compare is slower than unsigned */
#endif

#ifdef USERMEM
# if USERMEM >= (433484+SACREDMEM)
#  define PBITS	16
# else
#  if USERMEM >= (229600+SACREDMEM)
#   define PBITS	15
#  else
#   if USERMEM >= (127536+SACREDMEM)
#    define PBITS	14
#   else
#    if USERMEM >= (73464+SACREDMEM)
#     define PBITS	13
#    else
#     define PBITS	12
#    endif
#   endif
#  endif
# endif
# undef USERMEM
#endif /* USERMEM */

#ifdef PBITS		/* Preferred BITS for this memory size */
# ifndef BITS
#  define BITS PBITS
# endif /* BITS */
#endif /* PBITS */

#if BITS == 16
# define HSIZE	69001		/* 95% occupancy */
#endif
#if BITS == 15
# define HSIZE	35023		/* 94% occupancy */
#endif
#if BITS == 14
# define HSIZE	18013		/* 91% occupancy */
#endif
#if BITS == 13
# define HSIZE	9001		/* 91% occupancy */
#endif
#if BITS <= 12
# define HSIZE	5003		/* 80% occupancy */
#endif

/*
 * a code_int must be able to hold 2**BITS values of type int, and also -1
 */
#if BITS > 15
typedef long int	code_int;
#else
typedef int		code_int;
#endif

#ifdef SIGNED_COMPARE_SLOW
typedef unsigned long int count_int;
typedef unsigned short int count_short;
#else
typedef long int	  count_int;
#endif

#ifdef NO_UCHAR
 typedef char	char_type;
#else
 typedef	unsigned char	char_type;
#endif /* UCHAR */
char_type magic_header[] = { "\037\235" };	/* 1F 9D */

/* Defines for third byte of header */
#define BIT_MASK	0x1f
#define BLOCK_MASK	0x80
/* Masks 0x40 and 0x20 are free.  I think 0x20 should mean that there is
   a fourth header byte (for expansion).
*/
#define INIT_BITS 9			/* initial number of bits/code */

/*
 * compress.c - File compression ala IEEE Computer, June 1984.
 *
 * Authors:	Spencer W. Thomas	(decvax!harpo!utah-cs!utah-gr!thomas)
 *		Jim McKie		(decvax!mcvax!jim)
 *		Steve Davies		(decvax!vax135!petsd!peora!srd)
 *		Ken Turkowski		(decvax!decwrl!turtlevax!ken)
 *		James A. Woods		(decvax!ihnp4!ames!jaw)
 *		Joe Orost		(decvax!vax135!petsd!joe)
 *
 */

#if i386
#include <stdio.h>
#include <stdio.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#endif

#define ARGVAL() (*++(*argv) || (--argc && *++argv))

int n_bits;				/* number of bits/code */
int maxbits = BITS;			/* user settable max # bits/code */
code_int maxcode;			/* maximum code, given n_bits */
code_int maxmaxcode = 1L << BITS;	/* should NEVER generate this code */
#ifdef COMPATIBLE		/* But wrong! */
# define MAXCODE(n_bits)	(1L << (n_bits) - 1)
#else
# define MAXCODE(n_bits)	((1L << (n_bits)) - 1)
#endif /* COMPATIBLE */

# ifdef sel
/* support gould base register problems */
/*NOBASE*/
count_int htab [HSIZE];
unsigned short codetab [HSIZE];
/*NOBASE*/
# else /* !gould */
count_int htab [HSIZE];
unsigned short codetab [HSIZE];
# endif /* !gould */
#define htabof(i)	htab[i]
#define codetabof(i)	codetab[i]
code_int hsize = HSIZE;			/* for dynamic table sizing */
count_int fsize;

/*
 * To save much memory, we overlay the table used by compress() with those
 * used by decompress().  The tab_prefix table is the same size and type
 * as the codetab.  The tab_suffix table needs 2**BITS characters.  We
 * get this from the beginning of htab.  The output stack uses the rest
 * of htab, and contains characters.  There is plenty of room for any
 * possible stack (stack used to be 8000 characters).
 */

#define tab_prefixof(i)	codetabof(i)
#ifdef XENIX_16
# define tab_suffixof(i)	((char_type *)htab[(i)>>15])[(i) & 0x7fff]
# define de_stack		((char_type *)(htab2))
#else	/* Normal machine */
# define tab_suffixof(i)	((char_type *)(htab))[i]
# define de_stack		((char_type *)&tab_suffixof(1<<BITS))
#endif	/* XENIX_16 */

code_int free_ent = 0;			/* first unused entry */
int exit_stat = 0;

code_int getcode();

int nomagic = 0;	/* Use a 3-byte magic number header, unless old file */
int zcat_flg = 0;	/* Write output on stdout, suppress messages */
int quiet = 1;		/* don't tell me about compression */

/*
 * block compression parameters -- after all codes are used up,
 * and compression rate changes, start over.
 */
int block_compress = BLOCK_MASK;
int clear_flg = 0;
long int ratio = 0;
#define CHECK_GAP 10000	/* ratio check interval */
count_int checkpoint = CHECK_GAP;
/*
 * the next two codes should not be changed lightly, as they must not
 * lie within the contiguous general code space.
 */ 
#define FIRST	257	/* first free entry */
#define	CLEAR	256	/* table clear output code */

int force = 0;
char ofname [100];
int (*bgnd_flag)();

/* reset code table */
void cl_hash(register count_int hsize);
/* table clear for block compress */
void cl_block ();
void output(code_int  code);


int do_decomp = 0;

static int offset;
long int in_count = 1;			/* length of input */
long int bytes_out;			/* length of compressed output */
long int out_count = 0;			/* # of codes output (for debugging) */

/*
 * compress stdin to stdout
 *
 * Algorithm:  use open addressing double hashing (no chaining) on the 
 * prefix code / next character combination.  We do a variant of Knuth's
 * algorithm D (vol. 3, sec. 6.4) along with G. Knott's relatively-prime
 * secondary probe.  Here, the modular division first probe is gives way
 * to a faster exclusive-or manipulation.  Also do block compression with
 * an adaptive reset, whereby the code table is cleared when the compression
 * ratio decreases, but after the table fills.  The variable-length output
 * codes are re-sized at this point, and a special CLEAR code is generated
 * for the decompressor.  Late addition:  construct the table according to
 * file size for noticeable speed improvement on small files.  Please direct
 * questions about this implementation to ames!jaw.
 */

int main() {
    register long fcode;
    register code_int i = 0;
    register int c;
    register code_int ent;
#ifdef XENIX_16
    register code_int disp;
#else	/* Normal machine */
    register int disp;
#endif
    register code_int hsize_reg;
    register int hshift;

#ifndef COMPATIBLE
    if (nomagic == 0) {
	/* putchar(magic_header[0]); putchar(magic_header[1]);
	putchar((char)(maxbits | block_compress)); */
    }
#endif /* COMPATIBLE */

    offset = 0;
    bytes_out = 3;		/* includes 3-byte header mojo */
    out_count = 0;
    clear_flg = 0;
    ratio = 0;
    in_count = 1;

    printf("main: bytes_out %d... hsize %d\n", (int)bytes_out, (int)hsize);

    checkpoint = CHECK_GAP;
    maxcode = MAXCODE(n_bits = INIT_BITS);
    free_ent = ((block_compress) ? FIRST : 256 );


    ent = '\0'; /* getchar (); */

    hshift = 0;
    for ( fcode = (long) hsize;  fcode < 65536L; fcode *= 2L )
    	hshift++;
    hshift = 8 - hshift;		/* set hash code range bound */
    printf("main: hshift %d...\n", hshift);

    hsize_reg = hsize;
    cl_hash( (count_int) hsize_reg);		/* clear hash table */

/*#ifdef SIGNED_COMPARE_SLOW
    while ( (c = getchar()) != (unsigned) EOF ) {
#else
    while ( (c = getchar()) != EOF ) {
#endif*/
    printf("main: bytes_out %d...\n", (int)bytes_out);
    printf("main: hsize_reg %d...\n", (int)hsize_reg);
    printf("main: before compress %d...\n", (int)in_count);
    while (in_count < BYTES_TO_COMPRESS) {
        c = in_count % 255;
        
        printf("main: compressing %d...\n", (int)in_count);
	in_count++;
	fcode = (long) (((long) c << maxbits) + ent);
 	i = (((long)c << hshift) ^ ent);	/* xor hashing */
	
	if ( htabof (i) == fcode ) {
	    ent = codetabof (i);
	    continue;
	} else if ( (long)htabof (i) < 0 ) 	/* empty slot */
	    goto nomatch;
	 
 	disp = hsize_reg - i;		/* secondary hash (after G. Knott) */
	if ( i == 0 )
	    disp = 1;
probe:
	if ( (i -= disp) < 0 )
	    i += hsize_reg;

	if ( htabof (i) == fcode ) {
	    ent = codetabof (i);
	    continue;
	}
	if ( (long)htabof (i) > 0 ) 
	    goto probe;
nomatch:
	output ( (code_int) ent );
	out_count++;
 	ent = c;
#ifdef SIGNED_COMPARE_SLOW
	if ( (unsigned) free_ent < (unsigned) maxmaxcode) {
#else
	if ( free_ent < maxmaxcode ) {
#endif
 	    codetabof (i) = free_ent++;	/* code -> hashtable */
	    htabof (i) = fcode;
	}
	else if ( (count_int)in_count >= checkpoint && block_compress )
	    cl_block ();
    }
    /*
     * Put out the final code.
     */
    printf("main: output...\n");
    output( (code_int)ent );
    out_count++;
    output( (code_int)-1 );

    if(bytes_out > in_count)	/* exit(2) if no savings */
	exit_stat = 2;
    printf("main: end...\n");
    report (0xdeaddead);
    return 0;
}

/*****************************************************************
 * TAG( output )
 *
 * Output the given code.
 * Inputs:
 * 	code:	A n_bits-bit integer.  If == -1, then EOF.  This assumes
 *		that n_bits =< (long)wordsize - 1.
 * Outputs:
 * 	Outputs code to the file.
 * Assumptions:
 *	Chars are 8 bits long.
 * Algorithm:
 * 	Maintain a BITS character long buffer (so that 8 codes will
 * fit in it exactly).  Use the VAX insv instruction to insert each
 * code in turn.  When the buffer fills up empty it and start over.
 */

static char buf[BITS];

#ifndef vax
char_type lmask[9] = {0xff, 0xfe, 0xfc, 0xf8, 0xf0, 0xe0, 0xc0, 0x80, 0x00};
char_type rmask[9] = {0x00, 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff};
#endif /* vax */

void output( code )
code_int  code;
{

    /*
     * On the VAX, it is important to have the register declarations
     * in exactly the order given, or the asm will break.
     */
    register int r_off = offset, bits= n_bits;
    register char * bp = buf;

    if ( code >= 0 ) {
#ifdef vax
	/* VAX DEPENDENT!! Implementation on other machines is below.
	 *
	 * Translation: Insert BITS bits from the argument starting at
	 * offset bits from the beginning of buf.
	 */
	0;	/* Work around for pcc -O bug with asm and if stmt */
	asm( "insv	4(ap),r11,r10,(r9)" );
#else /* not a vax */
/* 
 * byte/bit numbering on the VAX is simulated by the following code
 */
	/*
	 * Get to the first byte.
	 */
	bp += (r_off >> 3);
	r_off &= 7;
	/*
	 * Since code is always >= 8 bits, only need to mask the first
	 * hunk on the left.
	 */
	*bp = (*bp & rmask[r_off]) | ((code << r_off) & lmask[r_off]);
	bp++;
	bits -= (8 - r_off);
	code >>= 8 - r_off;
	/* Get any 8 bit parts in the middle (<=1 for up to 16 bits). */
	if ( bits >= 8 ) {
	    *bp++ = code;
	    code >>= 8;
	    bits -= 8;
	}
	/* Last bits. */
	if(bits)
	    *bp = code;
#endif /* vax */
	offset += n_bits;
	if ( offset == (n_bits << 3) ) {
	    bp = buf;
	    bits = n_bits;
	    bytes_out += bits;
	/*    do
		putchar(*bp++); */
	    while(--bits);
	    offset = 0;
	}

	/*
	 * If the next entry is going to be too big for the code size,
	 * then increase it, if possible.
	 */
	if ( free_ent > maxcode || (clear_flg > 0))
	{
	    /*
	     * Write the whole buffer, because the input side won't
	     * discover the size increase until after it has read it.
	     */
	    if ( offset > 0 ) {
		/* if( fwrite( buf, 1, n_bits, stdout ) != n_bits)
			writeerr(); */
		bytes_out += n_bits;
	    }
	    offset = 0;

	    if ( clear_flg ) {
    	        maxcode = MAXCODE (n_bits = INIT_BITS);
	        clear_flg = 0;
	    }
	    else {
	    	n_bits++;
	    	if ( n_bits == maxbits )
		    maxcode = maxmaxcode;
	    	else
		    maxcode = MAXCODE(n_bits);
	    }
	}
    } else {
	/*
	 * At EOF, write the rest of the buffer.
	 */
	/* if ( offset > 0 )
	    fwrite( buf, 1, (offset + 7) / 8, stdout ); */
	bytes_out += (offset + 7) / 8;
	offset = 0;
	/* fflush( stdout ); */
	/* if( ferror( stdout ) )
		writeerr(); */
    }
}

/*
 * Decompress stdin to stdout.  This routine adapts to the codes in the
 * file building the "string" table on-the-fly; requiring no table to
 * be stored in the compressed file.  The tables used herein are shared
 * with those of the compress() routine.  See the definitions above.
 */

void decompress() {
    register char_type *stackp;
    register int finchar;
    register code_int code, oldcode, incode;

    /*
     * As above, initialize the first 256 entries in the table.
     */
    maxcode = MAXCODE(n_bits = INIT_BITS);
    for ( code = 255; code >= 0; code-- ) {
	tab_prefixof(code) = 0;
	tab_suffixof(code) = (char_type)code;
    }
    free_ent = ((block_compress) ? FIRST : 256 );

    finchar = oldcode = getcode();
    if(oldcode == -1)	/* EOF already? */
	return;			/* Get out of here */
 /*   putchar( (char)finchar );	*/	/* first code must be 8 bits = char */
   /* if(ferror(stdout))		
	writeerr(); */
    stackp = de_stack;

    while ( (code = getcode()) > -1 ) {

	if ( (code == CLEAR) && block_compress ) {
	    for ( code = 255; code >= 0; code-- )
		tab_prefixof(code) = 0;
	    clear_flg = 1;
	    free_ent = FIRST - 1;
	    if ( (code = getcode ()) == -1 )	/* O, untimely death! */
		break;
	}
	incode = code;
	/*
	 * Special case for KwKwK string.
	 */
	if ( code >= free_ent ) {
            *stackp++ = finchar;
	    code = oldcode;
	}

	/*
	 * Generate output characters in reverse order
	 */
#ifdef SIGNED_COMPARE_SLOW
	while ( ((unsigned long)code) >= ((unsigned long)256) ) {
#else
	while ( code >= 256 ) {
#endif
	    *stackp++ = tab_suffixof(code);
	    code = tab_prefixof(code);
	}
	*stackp++ = finchar = tab_suffixof(code);

	/*
	 * And put them out in forward order
	 */
	/* do
	    putchar ( *--stackp );
	while ( stackp > de_stack );*/

	/*
	 * Generate the new entry.
	 */
	if ( (code=free_ent) < maxmaxcode ) {
	    tab_prefixof(code) = (unsigned short)oldcode;
	    tab_suffixof(code) = finchar;
	    free_ent = code+1;
	} 
	/*
	 * Remember previous code.
	 */
	oldcode = incode;
    }
  /*  fflush( stdout ); */
   /* if(ferror(stdout))
	writeerr(); */
}

/*****************************************************************
 * TAG( getcode )
 *
 * Read one code from the standard input.  If EOF, return -1.
 * Inputs:
 * 	stdin
 * Outputs:
 * 	code or -1 is returned.
 */

code_int
getcode() {
    /*
     * On the VAX, it is important to have the register declarations
     * in exactly the order given, or the asm will break.
     */
    register code_int code;
    static int offset = 0, size = 0;
    static char_type buf[BITS];
    register int r_off, bits;
    register char_type *bp = buf;

    if ( clear_flg > 0 || offset >= size || free_ent > maxcode ) {
	/*
	 * If the next entry will be too big for the current code
	 * size, then we must increase the size.  This implies reading
	 * a new buffer full, too.
	 */
	if ( free_ent > maxcode ) {
	    n_bits++;
	    if ( n_bits == maxbits )
		maxcode = maxmaxcode;	/* won't get any bigger now */
	    else
		maxcode = MAXCODE(n_bits);
	}
	if ( clear_flg > 0) {
    	    maxcode = MAXCODE (n_bits = INIT_BITS);
	    clear_flg = 0;
	}
	/* size = fread( buf, 1, n_bits, stdin ); */
	if ( size <= 0 )
	    return -1;			/* end of file */
	offset = 0;
	/* Round size down to integral number of codes */
	size = (size << 3) - (n_bits - 1);
    }
    r_off = offset;
    bits = n_bits;
#ifdef vax
    asm( "extzv   r10,r9,(r8),r11" );
#else /* not a vax */
	/*
	 * Get to the first byte.
	 */
	bp += (r_off >> 3);
	r_off &= 7;
	/* Get first part (low order bits) */
#ifdef NO_UCHAR
	code = ((*bp++ >> r_off) & rmask[8 - r_off]) & 0xff;
#else
	code = (*bp++ >> r_off);
#endif /* NO_UCHAR */
	bits -= (8 - r_off);
	r_off = 8 - r_off;		/* now, offset into code word */
	/* Get any 8 bit parts in the middle (<=1 for up to 16 bits). */
	if ( bits >= 8 ) {
#ifdef NO_UCHAR
	    code |= (*bp++ & 0xff) << r_off;
#else
	    code |= *bp++ << r_off;
#endif /* NO_UCHAR */
	    r_off += 8;
	    bits -= 8;
	}
	/* high order bits. */
	code |= (*bp & rmask[bits]) << r_off;
#endif /* vax */
    offset += n_bits;

    return code;
}

/* For those who don't have it in libc.a */
char *
rindex(const char *s,
       int c)
{
	const char *p;
	for (p = NULL; *s; s++)
	    if (*s == c)
		p = s;
	return((char *)p);
}

/*
writeerr()
{
    perror ( ofname );
    unlink ( ofname );
    exit ( 1 );
}
*/
void cl_block ()		/* table clear for block compress */
{
    register long int rat;

    checkpoint = in_count + CHECK_GAP;

    if(in_count > 0x007fffff) {	/* shift will overflow */
	rat = bytes_out >> 8;
	if(rat == 0) {		/* Don't divide by zero */
	    rat = 0x7fffffff;
	} else {
	    rat = in_count / rat;
	}
    } else {
	rat = (in_count << 8) / bytes_out;	/* 8 fractional bits */
    }
    if ( rat > ratio ) {
	ratio = rat;
    } else {
	ratio = 0;
 	cl_hash ( (count_int) hsize );
	free_ent = FIRST;
	clear_flg = 1;
	output ( (code_int) CLEAR );
    }
}

void cl_hash(hsize)		/* reset code table */
	register count_int hsize;
{
#ifndef XENIX_16	/* Normal machine */
	register count_int *htab_p = htab+hsize;
#else
	register j;
	register long k = hsize;
	register count_int *htab_p;
#endif
	register long i;
	register long m1 = -1;

#ifdef XENIX_16
    for(j=0; j<=8 && k>=0; j++,k-=8192) {
	i = 8192;
	if(k < 8192) {
		i = k;
	}
	htab_p = &(htab[j][i]);
	i -= 16;
	if(i > 0) {
#else
	i = hsize - 16;
#endif
 	do {				/* might use Sys V memset(3) here */
		*(htab_p-16) = m1;
		*(htab_p-15) = m1;
		*(htab_p-14) = m1;
		*(htab_p-13) = m1;
		*(htab_p-12) = m1;
		*(htab_p-11) = m1;
		*(htab_p-10) = m1;
		*(htab_p-9) = m1;
		*(htab_p-8) = m1;
		*(htab_p-7) = m1;
		*(htab_p-6) = m1;
		*(htab_p-5) = m1;
		*(htab_p-4) = m1;
		*(htab_p-3) = m1;
		*(htab_p-2) = m1;
		*(htab_p-1) = m1;
		htab_p -= 16;
	} while ((i -= 16) >= 0);
#ifdef XENIX_16
	}
    }
#endif
    	for ( i += 16; i > 0; i-- )
		*--htab_p = m1;
}

