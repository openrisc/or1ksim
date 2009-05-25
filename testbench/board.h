#ifndef _BOARD_H_
#define _BOARD_H_

#define MC_CSR_VAL      0x0B000300
#define MC_MASK_VAL     0x000003f0
#define FLASH_BASE_ADDR 0xf0000000
#define FLASH_TMS_VAL   0x00000103
#define SDRAM_BASE_ADDR 0x00000000
#define SDRAM_TMS_VAL   0x19220057


#define UART_BASE  	    0x90000000
#define UART_IRQ        2
#define ETH_BASE       	0x92000000
#define ETH_IRQ         4
#define KBD_BASE_ADD    0x94000000
#define KBD_IRQ         5
#define MC_BASE_ADDR    0x93000000
#define DMA_BASE        0xb8000000

#endif
