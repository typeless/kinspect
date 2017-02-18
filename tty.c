/*
 * (C) 2014 Mura Li <mura_li@castech.com.tw>
 */

#include <linux/device.h>
#include <linux/amba/bus.h>
#include <linux/amba/serial.h>

#include "include/global.h"

unsigned long port = 0;

volatile void *uart_base[4] = {
	(volatile void *)0xd00d0000, 
	(volatile void *)0xd00d1000, 
	(volatile void *)0xd00d2000, 
	(volatile void *)0xd00d3000,
};

#define ICRNL	0x00000001
#define ECHO	0x00000002

static void simple_putchar(char ch)
{
	unsigned long status;

	do {
		status = *(volatile unsigned long *)(uart_base[port] + UART01x_FR);
	} while (status & (UART01x_FR_BUSY | UART01x_FR_TXFF));

	*(volatile unsigned long *)(uart_base[port] + UART01x_DR) = ch;
}

static void simple_puts(char *buf, unsigned long size)
{
	unsigned long i;

	for (i = 0; i < size; i++) {
		if (!buf[i])
			break;
		simple_putchar(buf[i]);
	}
}

static char fake_printk_buf[2048];
int fake_printk(const char *fmt, ...)
{
	va_list args;
	int count;

	va_start(args, fmt);
	count = vsprintf(fake_printk_buf, fmt, args);
	va_end(args);

	if (count > 0)
		simple_puts(fake_printk_buf, count);
	return count;
}


static unsigned long simple_readchar(void)
{	
	unsigned long status, ch;
	
	do {
		status = *(volatile unsigned long *)(uart_base[port] + UART01x_FR);
		if ((status & UART01x_FR_RXFE) == 0) {
			ch = *(volatile unsigned long *)(uart_base[port] + UART01x_DR);
			return ch;
		}
	} while (1);
	return -1;
}

static void tty_echo(char ch, unsigned long flags)
{
	switch (ch) {
	case '\r':
		simple_putchar('\r');
		if (flags & ICRNL)
			simple_putchar('\n');
		break;
	case '\b':
		break;
	default:
		simple_putchar(ch);
	}
}
unsigned long tty_read_line(char *buf, unsigned long size, unsigned long flags)
{
	unsigned long data, ret, count = 0;
	char ch;

	while (count < size) {

		data = simple_readchar();
		if (data & 0xff00)
			goto exit;

		ch = (char)data;

		switch (ch) {
		case '\b':
			if ((flags & ECHO) && count > 0) 
				simple_puts("\x08\x1b\x5b\x4a", 4);
			if (count > 0)
				count--;
			break;
		case '\r':
			if (flags & ECHO) {
				simple_putchar('\r');
				if (flags & ICRNL)
					simple_putchar('\n');
			}
			break;
		default:
			if (flags & ECHO)
				simple_putchar(ch);
			buf[count++] = ch;
		}


		if (ch == '\r')
			break;
	}
exit:
	buf[count] = '\0';
	ret = count;
	count = 0;
	return ret;
}
unsigned long ntty_read_line(char *buf, unsigned long size)
{
	return tty_read_line(buf, size, ECHO | ICRNL);
}

