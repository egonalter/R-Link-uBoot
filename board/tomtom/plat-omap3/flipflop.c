/*
 * (C) Copyright 2010 TomTom BV <http://www.tomtom.com>
 * Author: Matthias Kaehlcke <matthias.kaehlcke@tomtom.com> 
 *
 * See file CREDITS for list of people who contributed to this
 * project.
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
 */

/*
 * Implementation of the flip-flop. This can store 1 bit of information
 * across a (watchdog) reset.
 */

#include <config.h>

#ifdef CONFIG_SW_FLIPFLOP

#include <asm/io.h>
#include <asm/arch/omap3430.h>
#include <flipflop.h>

#define FLIP_FLOP_LOCATION (OMAP34XX_CTRL_BASE + 0x09FC)
#define FLIP_FLOP_BIT 31

void flipflop_set(int state)
{
	unsigned long value= readl(FLIP_FLOP_LOCATION);

	if (state)
		value |= (1 << FLIP_FLOP_BIT);
	else
		value &= ~(1 << FLIP_FLOP_BIT);

	writel(value, FLIP_FLOP_LOCATION);
}

int flipflop_get(void)
{
	const unsigned long value = readl(FLIP_FLOP_LOCATION);

	return (value & (1 << FLIP_FLOP_BIT));
}

void epicfail_reset(void)
{
	writel(readl(FLIP_FLOP_LOCATION) & ~(1 << EPICFAIL_BIT),
		FLIP_FLOP_LOCATION);
}

#endif
