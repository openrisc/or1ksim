/* DMA test */

#include "support.h"
#include "board.h"
#include "../peripheral/fields.h"
#include "../peripheral/dma_defs.h"

typedef volatile unsigned long *DMA_REG;

DMA_REG csr = (unsigned long *)(DMA_BASE + DMA_CSR),
    int_msk_a = (unsigned long *)(DMA_BASE + DMA_INT_MSK_A),
    int_msk_b = (unsigned long *)(DMA_BASE + DMA_INT_MSK_B),
    int_src_a = (unsigned long *)(DMA_BASE + DMA_INT_SRC_A),
    int_src_b = (unsigned long *)(DMA_BASE + DMA_INT_SRC_B),
    ch0_csr = (unsigned long *)(DMA_BASE + DMA_CH_BASE + DMA_CH_CSR),
    ch0_sz = (unsigned long *)(DMA_BASE + DMA_CH_BASE + DMA_CH_SZ),
    ch0_a0 = (unsigned long *)(DMA_BASE + DMA_CH_BASE + DMA_CH_A0),
    ch0_am0 = (unsigned long *)(DMA_BASE + DMA_CH_BASE + DMA_CH_AM0),
    ch0_a1 = (unsigned long *)(DMA_BASE + DMA_CH_BASE + DMA_CH_A1),
    ch0_am1 = (unsigned long *)(DMA_BASE + DMA_CH_BASE + DMA_CH_AM1),
    ch0_desc = (unsigned long *)(DMA_BASE + DMA_CH_BASE + DMA_CH_DESC);

struct DMA_DESCRIPTOR
{
    unsigned long csr;
    unsigned long adr0;
    unsigned long adr1;
    unsigned long next;
};


/* Test simplest DMA operation */
int simple( void )
{
    int ok;
    unsigned long src[2], dst[2];

    /* Set transfer Size */
    *ch0_sz = 0x00000002;

    /* Set addresses */
    *ch0_a0 = (unsigned long)src;
    *ch0_a1 = (unsigned long)dst;

    /* Fill source */
    src[0] = 0x01234567LU;
    src[1] = 0x89ABCDEFLU;

    /* Now set channel CSR */
    *ch0_csr = FLAG_MASK( DMA_CH_CSR, CH_EN ) | FLAG_MASK( DMA_CH_CSR, INC_SRC ) | FLAG_MASK( DMA_CH_CSR, INC_DST );

    /* Wait till the channel finishes */
    while ( TEST_FLAG( *ch0_csr, DMA_CH_CSR, BUSY ) )
        ;

    /* Dump contents of memory */
    ok = (dst[0] == src[0] && dst[1] == src[1]);
    report( ok );

    return ok;
}


/* Test simple transfer with chunks */
int chunks( void )
{
    unsigned i, ok;
    unsigned long src[6], dst[6];
    
    /* Set transfer size */
    *ch0_sz = 6LU | (3LU << DMA_CH_SZ_CHK_SZ_OFFSET);

    /* Set addresses */
    *ch0_a0 = (unsigned long)src;
    *ch0_a1 = (unsigned long)dst;

    /* Fill source */
    for ( i = 0; i < 6; ++ i )
        src[i] = 0xA63F879CLU + i;

    /* Now set channel CSR */
    *ch0_csr = FLAG_MASK( DMA_CH_CSR, CH_EN ) | FLAG_MASK( DMA_CH_CSR, INC_SRC ) | FLAG_MASK( DMA_CH_CSR, INC_DST );

    /* Wait till the channel finishes */
    while ( TEST_FLAG( *ch0_csr, DMA_CH_CSR, BUSY ) )
        ;

    /* Dump contents of memory */
    ok = 1;
    for ( i = 0; i < 6 && ok; ++ i )
        if ( dst[i] != src[i] )
            ok = 0;
    report( i );

    return ok;
}

/* Test transfer using linked list */
int list( void )
{
    struct DMA_DESCRIPTOR desc[2];
    unsigned long src[10], dst[10];
    unsigned i, ok;

    /* Set transfer size for each list element */
    desc[0].csr = 6;
    desc[1].csr = 4;

    /* Set chunk size */
    *ch0_sz = 2UL << DMA_CH_SZ_CHK_SZ_OFFSET;

    /* Set addresses */
    desc[0].adr0 = (unsigned long)src;
    desc[0].adr1 = (unsigned long)dst;
    desc[1].adr0 = (unsigned long)(src + 6);
    desc[1].adr1 = (unsigned long)(dst + 6);

    /* Fill source */
    for ( i = 0; i < 10; ++ i )
        src[i] = 0x110BD540FLU + i;

    /* Set descriptor CSR */
    desc[0].csr |= FLAG_MASK( DMA_DESC_CSR, INC_SRC ) | FLAG_MASK( DMA_DESC_CSR, INC_DST );
    desc[1].csr |= FLAG_MASK( DMA_DESC_CSR, EOL ) | FLAG_MASK( DMA_DESC_CSR, INC_SRC ) | FLAG_MASK( DMA_DESC_CSR, INC_DST );

    /* Point channel to descriptor */
    *ch0_desc = (unsigned)desc;

    /* Link the list */
    desc[0].next = (unsigned)&(desc[1]);
    desc[1].next = 0xDEADDEADUL;

    /* Set channel CSR */
    *ch0_csr = FLAG_MASK( DMA_CH_CSR, CH_EN ) | FLAG_MASK( DMA_CH_CSR, USE_ED );

    /* Wait till the channel finishes */
    while ( TEST_FLAG( *ch0_csr, DMA_CH_CSR, BUSY ) )
        ;

    ok = TEST_FLAG( *ch0_csr, DMA_CH_CSR, DONE );
    
    /* Dump contents of memory */
    for ( i = 0; i < 10 && ok; ++ i )
        if ( dst[i] != src[i] )
            ok = 0;
    report( i );

    return ok;
}


int main()
{
    int pass_simple, pass_chunks, pass_list;
    printf( "Starting DMA test\n" );

    printf( "  Simple DMA: " );
    printf( (pass_simple = simple()) ? "Passed\n" : "Failed\n" );
    printf( "  Chunks DMA: " );
    printf( (pass_chunks = chunks()) ? "Passed\n" : "Failed\n" );
    printf( "  List DMA: " );
    printf( (pass_list = list()) ? "Passed\n" : "Failed\n" );

    printf( "Ending DMA test\n" );
    if (pass_simple && pass_chunks && pass_list) {
        report (0xdeaddead);
        return 0;
    } else
        return 3 - pass_simple - pass_chunks - pass_list;
}


