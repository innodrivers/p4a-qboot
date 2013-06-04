/**
 * @file
 * @brief	I10 uart driver
 * @date	2010/08/02
 * 
 * Copyright (c) 2010 Innofidei Inc. All rights reserved.
 */
#include <types.h>
#include <common.h>
#include <io.h>
#include <hardware.h>
#include <uart.h>
#include <clk.h>

static unsigned int uart_base[] = {
		P4A_UART1_PHYS,
		P4A_UART2_PHYS,
		P4A_UART4W_PHYS,
};

/*
 * Uart Registers
 */
#define UMR			(0x00)		/* Uart Match register */
#define URBR		(0x04)		/* Uart Receive Buffer register */
#define UTBR		(0x08)		/* Uart Transmit Buffer register */
#define USR			(0x0c)		/* Uart Status register */
#define UCR			(0x10)		/* Uart Control register */
#define UISR		(0x14)		/* Uart Interrupt Status register */
#define UICR		(0x18)		/* Uart Interrupt Clear register */
#define UIER		(0x1c)		/* Uart Interrupt Enable register */
#define URTSCR		(0x20)		/* Uart RTS Control register */

/*
 * Uart Match Register (UMR) bit fields
 */
#define UMR_VALUE(_clock_, _baudrate_)		( ((_clock_) << 4)/(_baudrate_) - 256 )
#define UMR_VALUE_OVERSAMPLE(_clock_, _oversample_, _baudrate_)		((_clock_)/(_baudrate_)*256/(_oversample_) - 256)

/*
 * Uart Status Register (USR) bit fields
 */
#define USR_RFLEVEL_SHIFT		(0)
#define USR_RFLEVEL_MASK		(0x7f << USR_RFLEVEL_SHIFT)
#define USR_RFLEVEL_FULL		(64)
#define USR_TFLEVEL_SHIFT		(7)
#define USR_TFLEVEL_MASK		(0x7f << USR_TFLEVEL_SHIFT)
#define USR_TFLEVEL_FULL		(64)
#define USR_RXEVENT				(1 << 14)
#define USR_TXEVENT				(1 << 15)
#define USR_RTS					(1 << 16)
#define USR_CTS					(1 << 17)

/* Uart Status Register (USR) fifo level bit fields for uart1 and uart2 */
#define USR1_RFLEVEL_SHIFT		(0)
#define USR1_RFLEVEL_MASK		(0x3f << USR_RFLEVEL_SHIFT)
#define USR1_TFLEVEL_SHIFT		(6)
#define USR1_TFLEVEL_MASK		(0x3f << USR_TFLEVEL_SHIFT)



/*
 * Uart Control Register (UCR) bit fields
 */
#define UCR_RX_TL_MASK		(0x3F)
#define UCR_RX_TL(x)		((x) << 0)		/* RX FIFO interrupt trigger throld level */
#define UCR_TX_TL_MASK		(0x3F << 6)
#define UCR_TX_TL(x)		((x) << 6)		/* TX FIFO interrupt trigger throld level*/
#define UCR_UART_EN			(1 << 12)		/* Uart Enable */
#define	UCR_TXPAR_EN		(1 << 13)		/* Tx Parity Check Enable */
#define UCR_RXPAR_EN		(1 << 14)		/* Rx Parity Check Enable */
#define UCR_PAR_SEL_MASK	(0x3 << 15)
#define UCR_PAR_ODD         (0x0 <<15)		/* parity odd */
#define UCR_PAR_EVEN        (0x1<<15)   	/* parity even */
#define UCR_PAR_SPACE       (0x2<<15)		/* space is a 0-bit (or logic 0) */
#define UCR_PAR_MARK        (0x3<<15)		/* mark is a 1-bit (or logic 1) */
#define UCR_NBSTOP_SEL_MASK	(1 << 17)
#define UCR_NBSTOP_1        (0 << 17)		/* one stop bit */
#define UCR_NBSTOP_2        (1 << 17)		/* two stop bit */
#define UCR_TXFIFO_RESET	(1 << 18)		/* reset TX FIFO */
#define UCR_RXFIFO_RESET	(1 << 19)		/* reset RX FIFO */
#define UCR_CTS_EN			(1 << 20)		/* CTS Flow Control Enable */
#define UCR_RTS_EN			(1 << 21)		/* RTS Flow Control Enable */
#define UCR_LOOPBACK_EN		(1 << 22)		/* loopback mode enable */
#define UCR_DMA_MODE		(1 << 23)
#define UCR_CTS_POL			(1 << 24)		/* CTS Polarity */
#define UCR_RTS_POL			(1 << 25)		/* RTS Polarity */
#define UCR_OVERSAMPLE_RATE(x)	(((x) & 0x1f) << 27)

/*
 * Uart Interrupt Status Register (UISR) bit fields
 */
#define UISR_RX				(1 << 0)
#define UISR_TX				(1 << 1)
#define UISR_RX_PAR_ERR		(1 << 2)
#define UISR_RX_OVERFLOW	(1 << 3)
#define UISR_TX_OVERFLOW	(1 << 4)

/*
 * Uart Interrupt Clear Register (UICR) bit fields
 */
#define UICR_RX				(1 << 0)
#define UICR_TX				(1 << 1)
#define UICR_RX_PAR_ERR		(1 << 2)
#define UICR_RX_OVERFLOW	(1 << 3)
#define UICR_TX_OVERFLOW	(1 << 4)

/*
 * Uart Interrupt Enable Register (UIER) bit fields
 */
#define UIER_RX				(1 << 0)
#define UIER_TX				(1 << 1)
#define UIER_RX_PAR_ERR		(1 << 2)
#define UIER_RX_OVERFLOW	(1 << 3)
#define UIER_TX_OVERFLOW	(1 << 4)


/*
 * Uart RTS Control Register (URTSCR) bit fields
 */
#define URTSCR_DEASSERT_THRESHOLD(x)	(((x) & 0x7f)<<0)
#define URTSCR_REASSERT_THRESHOLD(x)	(((x) & 0x7f)<<8)

#define TX_FIFO_DEPTH		(64)
#define RX_FIFO_DEPTH		(64)

static __INLINE__ unsigned long __get_uart_clock_rate(int port)
{
	return get_uart_clk(port);
}

static __INLINE__ unsigned int portaddr(int port, int reg)
{
	return (uart_base[port] + reg);
}

static __INLINE__ unsigned long rd_regl(int port, int reg)
{
	return GET_REG(portaddr(port, reg));
}

static __INLINE__ void wr_regl(int port, int reg, unsigned int val)
{
	SET_REG(portaddr(port, reg), val);
}

static void enable_uart_clock(int port, int enable)
{
	if (port == 2) {	//UART4W
		*REG32(GBL_CFG_SOFTRST_REG) |= (1 << 26);
		*REG32(GBL_ARM_RST_REG) |= (1<<20);
		*REG32(GBL_CFG_PERI_CLK_REG) |= (1<<5);
	}
}

int uart_init(int port, int baudrate, int parity, int bits, int stop, int flow)
{
#define OVERSAMPLE		(16)
	unsigned long uart_clk;
	unsigned int ucr_val;

	enable_uart_clock(port, 1);

	uart_clk = __get_uart_clock_rate(port);
	ucr_val = 0;
	wr_regl(port, UCR, UCR_TXFIFO_RESET | UCR_RXFIFO_RESET);	// FIFOs reset
	wr_regl(port, UCR, 0);	// FIFOs reset

	if (port == 2) {	//UART 4Wires
		wr_regl(port, UMR, (((uart_clk << 4) / baudrate) - 256));
	} else {	//UART 1 & 2
		wr_regl(port, UMR, (((uart_clk / 16) / baudrate) - 1));	// baudrate set
	}
	ucr_val = (UCR_RX_TL(1))|(UCR_TX_TL(20));		//fifo level
	
	if (parity != 'n') {
		ucr_val |= UCR_TXPAR_EN | UCR_RXPAR_EN;
		if (parity == 'e')	// Even
			ucr_val |= UCR_PAR_EVEN;
		else if (parity == 'm')	// Mark, always 1
			ucr_val |= UCR_PAR_MARK;
		else if (parity == 's')	// Space, always 0
			ucr_val |= UCR_PAR_SPACE;
	}

	if (stop == 2)
		ucr_val |= UCR_NBSTOP_2;
	
	if (flow == 'r') { // RTS/CTS hardware flow control
		wr_regl(port, URTSCR, URTSCR_DEASSERT_THRESHOLD(48) | URTSCR_REASSERT_THRESHOLD(16));
		ucr_val |= UCR_CTS_EN | UCR_RTS_EN | UCR_CTS_POL |UCR_RTS_POL;
	}

	wr_regl(port, UCR, ucr_val | UCR_UART_EN);

	return 0;
}

void uart_deinit(int port)
{
	wr_regl(port, UCR, 0);

	enable_uart_clock(port, 0);
}

int uart_tstc(int port)
{
	if(1 == port || 2 == port)/*uart 1, uart2*/
	{
		return (rd_regl(port, USR) & USR1_RFLEVEL_MASK);
	}
	else if(3 == port)/*4 wire uart*/
	{
		return (rd_regl(port, USR) & USR_RFLEVEL_MASK);
	}
}


int uart_getc(int port)
{
	while( ! uart_tstc(port) )
		;
	return (rd_regl(port, URBR) & 0xff);
}

int uart_getc_timeout(int port, uint8_t* ch, uint32_t expires_ms)
{
	if (expires_ms) {
		do {
			int mscount = 1000;
		
			do {
				if (uart_tstc(port))
					break;
				udelay(1);
			} while (--mscount);

			if (mscount) {
				*ch = (uint8_t)(rd_regl(port, URBR) & 0xff);
				return 0;
			}

			expires_ms--;
		} while (--expires_ms);

		return -1;	// timeout
	}

	*ch = uart_getc(port);
	return 0;
}

void uart_putc(int port, const char c)
{
	int timeout = 100000;
	int tx_dlt_depth;

	do {
		tx_dlt_depth  = (rd_regl(port, USR) & 0x00000fc0) >> 6;
	} while (tx_dlt_depth == 0x3f && --timeout);

	if(timeout)
		wr_regl(port, UTBR, c);
}
