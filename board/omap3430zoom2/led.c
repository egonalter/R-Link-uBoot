/*
 * (C) Copyright 2008
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
 */
#include <common.h>
#include <asm/arch/cpu.h>
#include <asm/io.h>
#include <asm/arch/mux.h>

#define 	MUX_VAL(OFFSET,VALUE)\
		__raw_writew((VALUE), OMAP34XX_CTRL_BASE + (OFFSET));

#define		CP(x)	(CONTROL_PADCONF_##x)

#if defined (CONFIG_ZOOM2_LED)

void omap3_zoom2_led_red_on ()
{
  	MUX_VAL(CP(McBSP4_DX),    (IEN  | PTD | EN | M4))  /* gpio_154 blue */
	MUX_VAL(CP(McSPI1_SOMI),  (IEN  | PTU | EN | M4))  /* gpio_172 red */
}

void omap3_zoom2_led_red_off ()
{
	MUX_VAL(CP(McSPI1_SOMI),  (IEN  | PTD | EN | M4))  /* gpio_172 red */
}

void omap3_zoom2_led_blue_on ()
{
	MUX_VAL(CP(McSPI1_SOMI),  (IEN  | PTD | EN | M4))  /* gpio_172 red */
 	MUX_VAL(CP(McBSP4_DX),    (IEN  | PTU | EN | M4))  /* gpio_154 blue */
}

void omap3_zoom2_led_blue_off ()
{
 	MUX_VAL(CP(McBSP4_DX),    (IEN  | PTD | EN | M4))  /* gpio_154 blue */
}

#endif /* CONFIG_ZOOM2_LED */
