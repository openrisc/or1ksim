/* Cache test */
#include "support.h"
#include "spr_defs.h"


#undef  UART

#define MEM_RAM 0x00100000

/* Number of IC sets (power of 2) */
#define IC_SETS 256
#define DC_SETS 256

/* Block size in bytes (1, 2, 4, 8, 16, 32 etc.) */
#define IC_BLOCK_SIZE 16
#define DC_BLOCK_SIZE 16
 
/* Number of IC ways (1, 2, 3 etc.). */
#define IC_WAYS 1
#define DC_WAYS 1
 
/* Cache size */
#define IC_SIZE (IC_WAYS*IC_SETS*IC_BLOCK_SIZE)
#define DC_SIZE (DC_WAYS*DC_SETS*DC_BLOCK_SIZE)

/* Memory access macros */
#define REG8(add) *((volatile unsigned char *)(add))
#define REG16(add) *((volatile unsigned short *)(add))
#define REG32(add) *((volatile unsigned long *)(add))

#if UART
#include "uart.h"
#define IN_CLK  20000000
#define UART_BASE  0x9c000000
#define UART_BAUD_RATE 9600
 
#define BOTH_EMPTY (UART_LSR_TEMT | UART_LSR_THRE)

#define WAIT_FOR_XMITR \
        do { \
                lsr = REG8(UART_BASE + UART_LSR); \
        } while ((lsr & BOTH_EMPTY) != BOTH_EMPTY)

#define WAIT_FOR_THRE \
        do { \
                lsr = REG8(UART_BASE + UART_LSR); \
        } while ((lsr & UART_LSR_THRE) != UART_LSR_THRE)

#define CHECK_FOR_CHAR \
        (REG8(UART_BASE + UART_LSR) & UART_LSR_DR)

#define WAIT_FOR_CHAR \
         do { \
                lsr = REG8(UART_BASE + UART_LSR); \
         } while ((lsr & UART_LSR_DR) != UART_LSR_DR)

#define UART_TX_BUFF_LEN 32
#define UART_TX_BUFF_MASK (UART_TX_BUFF_LEN -1)

#define print_n(x)  \
  { \
    uart_putc(s[((x) >> 28) & 0x0f]); \
    uart_putc(s[((x) >> 24) & 0x0f]); \
    uart_putc(s[((x) >> 20) & 0x0f]); \
    uart_putc(s[((x) >> 16) & 0x0f]); \
    uart_putc(s[((x) >> 12) & 0x0f]); \
    uart_putc(s[((x) >> 8) & 0x0f]);  \
    uart_putc(s[((x) >> 4) & 0x0f]);  \
    uart_putc(s[((x) >> 0) & 0x0f]);  \
  }

const char s[] = "0123456789abcdef";

void uart_init(void)
{
        int devisor;
 
        /* Reset receiver and transmiter */
        REG8(UART_BASE + UART_FCR) = UART_FCR_ENABLE_FIFO | UART_FCR_CLEAR_RCVR | UART_FCR_CLEAR_XMIT | UART_FCR_TRIGGER_14;
 
        /* Disable all interrupts */
        REG8(UART_BASE + UART_IER) = 0x00;
 
        /* Set 8 bit char, 1 stop bit, no parity */
        REG8(UART_BASE + UART_LCR) = UART_LCR_WLEN8 & ~(UART_LCR_STOP | UART_LCR_PARITY);
 
        /* Set baud rate */
        devisor = IN_CLK/(16 * UART_BAUD_RATE);
        REG8(UART_BASE + UART_LCR) |= UART_LCR_DLAB;
        REG8(UART_BASE + UART_DLL) = devisor & 0x000000ff;
        REG8(UART_BASE + UART_DLM) = (devisor >> 8) & 0x000000ff;
        REG8(UART_BASE + UART_LCR) &= ~(UART_LCR_DLAB);
 
        return;
}

static inline void uart_putc(char c)
{
        unsigned char lsr;
        
        WAIT_FOR_THRE;
        REG8(UART_BASE + UART_TX) = c;
        if(c == '\n') {
          WAIT_FOR_THRE;
          REG8(UART_BASE + UART_TX) = '\r';
        }
        WAIT_FOR_XMITR;
}

static inline void print_str(char *str)
{
  while(*str != 0) {
    uart_putc(*str);
    str++;
  }
}

static inline char uart_getc()
{
        unsigned char lsr;
        char c;

        WAIT_FOR_CHAR;
        c = REG8(UART_BASE + UART_RX);
        return c;
}
#endif

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
unsigned long jump_add[15*IC_WAYS];

void dummy();

void jump_and_link(void) 
{
	asm("_jalr:");
  asm("l.jr\tr9");
  asm("l.nop");
}

void jump(void)
{
	asm("_jr:");
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
  asm("l.movhi\tr11,hi(_jump_indx)" : :);
  asm("l.ori\tr11,r11,lo(_jump_indx)" : :);
	asm("l.jalr\t\t%0" : : "r" (add) : "r11", "r9");
	asm("l.nop" : :);
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
        unsigned long base, add;
 
        base = (((unsigned long)MEM_RAM / (IC_SETS*IC_BLOCK_SIZE)) * IC_SETS*IC_BLOCK_SIZE) + IC_SETS*IC_BLOCK_SIZE;
  
        /* Copy jr to various location */
        add = base;
        for(i = 0; i < IC_WAYS; i++) {
                copy_jr(add);
                copy_jr(add + 2*IC_BLOCK_SIZE + 4);
                copy_jr(add + 4*IC_BLOCK_SIZE + 8);
                copy_jr(add + 6*IC_BLOCK_SIZE + 12);
 
                copy_jr(add + (IC_SETS - 2)*IC_BLOCK_SIZE + 0);
                copy_jr(add + (IC_SETS - 4)*IC_BLOCK_SIZE + 4);
                copy_jr(add + (IC_SETS - 6)*IC_BLOCK_SIZE + 8);
                copy_jr(add + (IC_SETS - 8)*IC_BLOCK_SIZE + 12);
                add += IC_SETS*IC_BLOCK_SIZE;
        }
 
        /* Load execution table which starts at address 4 (at address 0 is table index) */
        add = base;
        for(i = 0; i < IC_WAYS; i++) {
                /* Cache miss */
                jump_add[15*i + 0] = add + 2*IC_BLOCK_SIZE + 4;
                jump_add[15*i + 1] = add + 4*IC_BLOCK_SIZE + 8;
                jump_add[15*i + 2] = add + 6*IC_BLOCK_SIZE + 12;
                /* Cache hit/miss */
                jump_add[15*i + 3] = add;
                jump_add[15*i + 4] = add + (IC_SETS - 2)*IC_BLOCK_SIZE + 0;
                jump_add[15*i + 5] = add + 2*IC_BLOCK_SIZE + 4;
                jump_add[15*i + 6] = add + (IC_SETS - 4)*IC_BLOCK_SIZE + 4;
                jump_add[15*i + 7] = add + 4*IC_BLOCK_SIZE + 8;
                jump_add[15*i + 8] = add + (IC_SETS - 6)*IC_BLOCK_SIZE + 8;
                jump_add[15*i + 9] = add + 6*IC_BLOCK_SIZE + 12;
                jump_add[15*i + 10] = add + (IC_SETS - 8)*IC_BLOCK_SIZE + 12;
                /* Cache hit */
                jump_add[15*i + 11] = add + (IC_SETS - 2)*IC_BLOCK_SIZE + 0;
                jump_add[15*i + 12] = add + (IC_SETS - 4)*IC_BLOCK_SIZE + 4;
                jump_add[15*i + 13] = add + (IC_SETS - 6)*IC_BLOCK_SIZE + 8;
                jump_add[15*i + 14] = add + (IC_SETS - 8)*IC_BLOCK_SIZE + 12;

                add += IC_SETS*IC_BLOCK_SIZE;
        }
 
        /* Go home */
        jump_add[15*i] = (unsigned long)&jalr;
 
        /* Initilalize table index */
        jump_indx = (unsigned long)&jump_add[0];
 
        ic_enable();

        /* Go */
        call(base);
 
        ic_disable();
 
        return 0;
}

int main(void)
{
	unsigned long rc, ret = 0;

#ifdef UART
  /* Initialize controller */
  uart_init();
#endif

#ifdef UART
  print_str("DC test :            ");
#endif
	rc = dc_test();
  ret += rc;
#ifdef UART
  print_n(rc+0xdeaddca1);
  print_str("\n");
#else
	report(rc + 0xdeaddca1);
#endif
	
#ifdef UART
  print_str("DC invalidate test : ");
#endif
	rc = dc_inv_test(MEM_RAM);
  ret += rc;
#ifdef UART
  print_n(rc + 0x9e8daa91);
  print_str("\n");
#else
	report(rc + 0x9e8daa91);
#endif

#ifdef UART
  print_str("IC test :            ");
#endif
	rc = ic_test();
  ret += rc;
#ifdef UART
  print_n(rc + 0xdeaddead);
  print_str("\n");
#else
	report(rc + 0xdeaddead);
#endif


#ifdef UART
  print_str("IC invalidate test : ");
#endif
  ic_enable();
  rc = ic_inv_test();
  ret += rc;
#ifdef UART
  print_n(rc + 0xdeadde8f);
  print_str("\n");
  while(1);
#else
	report(rc + 0xdeadde8f);
#endif


	report(ret + 0x9e8da867);
  exit(0);

	return 0;
}

/* just for size calculation */
void dummy() 
{
}
