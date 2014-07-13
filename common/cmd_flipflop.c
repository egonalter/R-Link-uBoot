/*
 * Command to set/activate the flipflop fail flags.
 *
 * Copyright (C) 2010 TomTom International B.V.
 * 
 ************************************************************************
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
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

#include <config.h>
#include <common.h>
#include <command.h>
#include <flipflop.h>

#if defined(CONFIG_CMD_FLIPFLOP)


static int do_flipflop (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	if (argc != 2) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

	if (strcmp(argv[1], "0") == 0) {
		flipflop_set(0);
		debug("set flipflop 0\n");
	} else {
		flipflop_set(1); /* set boot fail flag */
		debug("set flipflop 1\n");
	}

	return 0;
}

U_BOOT_CMD(
 	flipflop,	2,	0,	do_flipflop,
 	"flipflop- [de]activate boot fail flag\n",
	"flipflop [ 1 | 0 ]\n"
	"    - activate or deactivate boot fail flag\n"
);


#ifndef __NON_LEGACY
static int do_getbootfail (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	if (argc != 2) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

	if (flipflop_get()) {
		setenv (argv[1], "fail");
	} else {
		setenv (argv[1], "pass");
	}

	return 0;
}

U_BOOT_CMD(
 	getbootfail,	2,	0,	do_getbootfail,
 	"getbootfail- return boot fail status\n",
	"getbootfail <name>\n"
	"    - return boot fail status ('fail' or 'pass') in 'name'\n"
);
#endif /* __NON_LEGACY */

#endif  /* CONFIG_CMD_FLIPFLOP */

