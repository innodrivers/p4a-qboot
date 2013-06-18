
#ifndef __ASM_ARM_ARCH_CLK_H_
#define __ASM_ARM_ARCH_CLK_H_


unsigned long get_uart_clk(int idx);
unsigned long get_timer_clk(void);

enum clock_id {
	CLK_TIMER = 0,
	CLK_P4TIMER,
	CLK_UART1,
	CLK_UART2,
	CLK_UART4W,
	CLK_NAND,
};

void clock_switch(enum clock_id clk, int on);

#endif
