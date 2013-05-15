#ifndef _SERIAL_H_
#define _SERIAL_H_

int serial_tstc(void);
char serial_getc(void);
void serial_putc(char ch);
void serial_puts(const char* s);
int serial_init(int port);

#endif	// _SERIAL_H_
