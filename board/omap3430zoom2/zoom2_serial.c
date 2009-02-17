/*
 * (C) Copyright 2009
 * Windriver, <www.windriver.com>
 * Tom Rix <Tom.Rix@windriver.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 *
 * This file was adapted from cpu/mpc5xxx/serial.c
 *
 */

#include "zoom2_serial.h"

int quad_init_dev(unsigned long base)
{
	NS16550_t port = (NS16550_t) base;
	int clock_divisor = CFG_NS16550_CLK / 16 / CONFIG_BAUDRATE;

	NS16550_init(port, clock_divisor);

	return 0;
}

void quad_putc_dev(unsigned long base, const char c)
{
	NS16550_t port = (NS16550_t) base;

	if (c == '\n')
		quad_putc_dev(base, '\r');

	NS16550_putc(port, c);
}

void quad_puts_dev(unsigned long base, const char *s)
{
	while (*s)
		quad_putc_dev(base, *s++);
}

int quad_getc_dev(unsigned long base)
{
	NS16550_t port = (NS16550_t) base;

	return NS16550_getc(port);
}

int quad_tstc_dev(unsigned long base)
{
	NS16550_t port = (NS16550_t) base;

	return NS16550_tstc(port);
}

void quad_setbrg_dev(unsigned long base)
{
	NS16550_t port = (NS16550_t) base;

	int clock_divisor = CFG_NS16550_CLK / 16 / CONFIG_BAUDRATE;

	NS16550_reinit(port, clock_divisor);
}

QUAD_INIT(0)
QUAD_INIT(1)
QUAD_INIT(2)
QUAD_INIT(3)

