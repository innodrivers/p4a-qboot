#ifndef _UART_H
#define _UART_H

extern int uart_init(int port, int baudrate, int parity, int bits, int stop, int flow);
extern void uart_deinit(int port);
extern int uart_tstc(int port);
extern int uart_getc(int port);
extern int uart_getc_timeout(int port, uint8_t* ch, uint32_t expires_ms);
extern void uart_putc(int port, const char c);

extern int serial_init(int port);
extern void serial_puts(const char* s);

#endif	/* _UART_H */
