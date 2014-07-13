/*
 * Initialize TomTom specific environment variables
 *
 * Some of this stuff we normally would do in the board file, but the
 * environment isn't initialized when board_init() is called
 *
 * Copyright (C) 2010 TomTom International B.V.
 * Author: Matthias Kaehlcke <matthias.kaehlcke@tomtom.com>
 *         Martin Jackson <martin.jackson@tomtom.com>
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

#include <common.h>
#include <bootscript.h>
#include <mem_regions.h>

DECLARE_GLOBAL_DATA_PTR;

extern void board_env_init (void) __attribute__((weak));

void tomtom_env_init(void)
{
	const char *str_bootmode;
	char ulong_str[16]; /* Needs to be as long as "0x12345678" + '\0' */

	switch (gd->tomtom.sysboot_mode) {
	case SYSBOOT_MODE_COLD:
		str_bootmode = SYSBOOT_MODE_STR_COLD;
		break;

	case SYSBOOT_MODE_WARM:
	case SYSBOOT_MODE_WATCHDOG:
		/* we treat warm and watchdog resets uniformly */
		str_bootmode = SYSBOOT_MODE_STR_WATCHDOG; /* TODO: consider changing to "warm" */
		break;

	default:
		printf("invalid value for sysboot_mode: %d\n", gd->tomtom.sysboot_mode);

		str_bootmode = SYSBOOT_MODE_STR_COLD;
	}

	setenv("sysboot_mode", str_bootmode);

	IMPORT_BOOTSCRIPT(preboot_plat, preboot_plat);
#ifdef CONFIG_DEBUG_BUILD
	IMPORT_BOOTSCRIPT(preboot_debug, preboot);
	IMPORT_BOOTSCRIPT(bootcmd_debug, bootcmd);
	IMPORT_BOOTSCRIPT(altbootcmd_debug, altbootcmd);
	IMPORT_BOOTSCRIPT(preboot, preboot.default);
	IMPORT_BOOTSCRIPT(bootcmd, bootcmd.default);
	IMPORT_BOOTSCRIPT(altbootcmd, altbootcmd.default);
#else
	IMPORT_BOOTSCRIPT(preboot, preboot);
	IMPORT_BOOTSCRIPT(bootcmd, bootcmd);
	IMPORT_BOOTSCRIPT(altbootcmd, altbootcmd);
#endif

	sprintf(ulong_str, "%p", CFG_LOAD_ADDR);
	setenv("loadaddr", ulong_str);

	sprintf(ulong_str, "%p", CFG_FDT_ADDR);
	setenv("fdaddr", ulong_str);

	sprintf(ulong_str, "%#x", MEMADDR_KERN_LEN);
	setenv("kernel.maxsize", ulong_str);

extern int bootdev; /* Defined in the board/.../<boardname>.c */
	sprintf(ulong_str, "%u", bootdev);
	setenv("bootdev", ulong_str);

#ifdef __VARIANT_A1
	/* make this work better :S */
extern int hackdevice; /* Defined in the board/.../<boardname>.c */
	sprintf(ulong_str, "%u", hackdevice);
#else
	sprintf(ulong_str, "%u", 0);
#endif
	setenv("hackdevice", ulong_str);

	if (board_env_init)
		board_env_init();
}

