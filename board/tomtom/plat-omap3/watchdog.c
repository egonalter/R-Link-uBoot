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

/* Watchdog driver for OMAP3 */

#include <common.h>

#if defined(CONFIG_HW_WATCHDOG)

#include <asm/arch/bits.h>
#include <asm/io.h>
#include <asm/arch/sys_proto.h>
#include <watchdog.h>

#define WDT_DISABLE1	0xAAAA
#define WDT_DISABLE2	0x5555
#define WDT_ENABLE1	0xBBBB
#define WDT_ENABLE2	0x4444

#define WDT_TIMEOUT	60 /* watchdog timeout in seconds */
#define WDT_LOAD_VAL	(0xFFFFFFFFUL - (WDT_TIMEOUT * 32768)) /* default clock: 32kHz */

struct {
#define WD_NOT_ARMED 0
#define WD_ARMED 1
	int armed; /* Is the watchdog armed (enabled)? */
}
wd_state;

#define is_armed(wd) (wd.armed == WD_ARMED)

static inline void wd_write(unsigned int value, unsigned int reg)
{
	__raw_writel(value, WD2_BASE + reg);
}

static inline unsigned int wd_read(unsigned int reg)
{
	return __raw_readl(WD2_BASE + reg);
}

static inline void wd_write_sync(unsigned int value, unsigned int reg)
{
	wd_write(value, reg);

	while (wd_read(WWPS))
		;
}

static void __wd_start(void)
{
	wd_write_sync(WDT_ENABLE1, WSPR);
	wd_write_sync(WDT_ENABLE2, WSPR);
}

static void wd_start(void)
{
	__wd_start();
	wd_state.armed = WD_ARMED;
}

static void __wd_stop(void)
{
	wd_write_sync(WDT_DISABLE1, WSPR);
	wd_write_sync(WDT_DISABLE2, WSPR);
}

static void wd_stop(void)
{
	__wd_stop();
	wd_state.armed = WD_NOT_ARMED;
}

/* initialize the watchdog counter */
static void wd_mod_count_loadval_prescaler(ulong val, ulong *reg)
{
	if (is_armed(wd_state))
		__wd_stop();

	wd_write_sync(val, reg);

	if (is_armed(wd_state))
		__wd_start();
}

void hw_watchdog_activate(unsigned int activate)
{
	if (activate) {
		/* reload the counter */
		hw_watchdog_reset();
		wd_start();
	} else {
		wd_stop();
	}
}

void hw_watchdog_reset(void)
{
	/* read the current trigger value */
	unsigned int trigger_val = wd_read(WTGR);

	/* reload the watchdog counter by changing the value in WTGR */
	trigger_val++;
	wd_write_sync(trigger_val, WTGR);
}

void hw_watchdog_init(void)
{
#if 0 /* Enabled in x-loader */
	/* enable the watchdog clocks */
	sr32(CM_FCLKEN_WKUP, 5, 1, 1);
	sr32(CM_ICLKEN_WKUP, 5, 1, 1);

	/* wait for the watchdog to become accesible */
	wait_on_value(BIT5, 0x00, CM_IDLEST_WKUP, 5);

	wd_state.armed = WD_NOT_ARMED;

	/* start the watchdog */
	wd_mod_count_loadval_prescaler(WDT_LOAD_VAL, WLDR);
	hw_watchdog_activate(1);
#else
	wd_state.armed = WD_ARMED;
#endif

	/* Linux takes care of things like smart-idle, clockactivity, ... */
}

#endif
