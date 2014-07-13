/*
 * BCM4760 'flipflop' - Retains 1-bit across a watchdog reset.
 *
 * Copyright (C) 2010 TomTom International B.V.
 * Author: Martin Jackson <martin.jackson@tomtom.com>
 * 
 ************************************************************************
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 
 * 02110-1301, USA.
 ************************************************************************
 */

/* #define DEBUG */

#include <common.h>
#include <flipflop.h>

DECLARE_GLOBAL_DATA_PTR;

static int get_bootlimit(void)
{
	char *s;

#ifdef CONFIG_BOOTCOUNT_LIMIT
	if ((s=getenv("bootlimit")) != NULL) {
		int n = simple_strtoul(s, NULL, 10);
		debug("bootlimit = %d\n", n);
		return n;
	}
#endif
	debug("bootlimit hardcoded to 1\n");
	return 1;
}

void bootcount_store(ulong count)
{
	debug("Storing bootcount [%lu]\n", count);
	gd->tomtom.bootcount = count;
	if (count <= get_bootlimit())
		flipflop_set(1);
	else
		flipflop_set(0);
}

ulong bootcount_load(void)
{
	ulong r = gd->tomtom.bootcount;
	char *s;

#ifdef __NON_LEGACY
	if ((s=getenv("sysboot_mode")) != NULL &&
	    strcmp(s, "watchdog") == 0)
#else
	if (gd->tomtom.sysboot_mode == SYSBOOT_MODE_WATCHDOG &&
	    gd->tomtom.bootcount == 0)
#endif
	{
		debug("sysboot_mode is watchdog\n");
		if (flipflop_get()) {
			r += get_bootlimit();
		}
	} else {
		debug("sysboot_mode is cold\n");
	}

	debug("Returning bootcount %lu\n", r);
	return r;
}
