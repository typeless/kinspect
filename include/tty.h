/*
 * (C) 2014 Mura Li <mura_li@castech.com.tw>
 */


extern unsigned long tty_read_line(char *buf, unsigned long size);
extern unsigned long ntty_read_line(char *buf, unsigned long size);

extern int fake_printk(const char *fmt, ...);

