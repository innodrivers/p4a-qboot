#include <types.h>
#include <common.h>
#include <io.h>
#include <uart.h>

	
static int serial_port;

int serial_tstc(void)
{
	return uart_tstc(serial_port);
}

char serial_getc(void)
{
	return (char)uart_getc(serial_port);
}

void serial_putc(char ch)
{
	if(ch=='\n')
		serial_putc('\r');
	
	uart_putc(serial_port, ch);
}

void serial_puts(const char* s)
{
	while (*s)
		serial_putc(*s++);
}

int serial_init(int port)
{
	serial_port = port;

	return uart_init(serial_port, 115200, 'n', 8, 1, 'n');
}
