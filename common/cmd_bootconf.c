/*
 * Bootloader configuration file parser
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
#include <command.h>
#include <linux/ctype.h>

#if defined(CONFIG_CMD_BOOTCONF)

#define FLAG(f, v, s, u) { \
	.flag = f, \
	.var = v, \
	.setval = s, \
	.unsetval = u, \
}
#define FLAG_YESNO(f, v) FLAG(f, v, "yes", "no")

struct {
	char flag;
	char *var;
	char *setval;
	char *unsetval;
}
bootconf_flags[] = {
	FLAG_YESNO('M', "run_memorytest"),
	FLAG('R', "force_altboot", "yes", NULL),
	FLAG('S', "sysboot_mode", "warm", NULL),
	FLAG_YESNO('W', "disable_watchdog"),
	FLAG_YESNO('E', "exec_applet"),
	FLAG_YESNO('Y', "no_trybooty"),
	FLAG('F', "force_flasher_mode","flasher", ""),
};
#define BC_I bootconf_flags[i]

int do_bootconf (cmd_tbl_t * cmdtp, int flag, int argc, char *argv[])
{
	char *addr = NULL;
	int i;

	if (argc < 2) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return -1;
	}

	if ((addr = (char *) simple_strtoul(argv[1], NULL, 16)) == NULL) {
		eprintf("Address is NULL\n");
		goto unset_all;
		return -1;
	}

	/* Might also be that uboot.conf couldn't be loaded here */
	if (addr[0] != '-') {
		printf("'uboot.conf' ignored\n");
		goto unset_all;
	}

	for(i=0; i<ARRAY_SIZE(bootconf_flags); i++) {
		int j, found=0;

		debug("Checking flag [%c]\n", BC_I.flag);

		for(j=1; j<ARRAY_SIZE(bootconf_flags)+1 &&
		    isalpha(addr[j]); j++) {
			if (addr[j] == BC_I.flag) {
				found = 1;
				debug("Flag [%c] is set\n", BC_I.flag);
				break;
			}
		}
		/* Leave the variable alone if [un]setval NULL */
		if(found && BC_I.setval != NULL) {
			debug("Setting variable %s\n", BC_I.var);
			setenv(BC_I.var, BC_I.setval);
		} else if (BC_I.unsetval != NULL) {
			debug("Unsetting variable %s\n", BC_I.var);
			setenv(BC_I.var, BC_I.unsetval);
		}
	}

	return 0;

unset_all:
	for(i=0; i<ARRAY_SIZE(bootconf_flags); i++) {
		if (BC_I.unsetval != NULL) {
			debug("Unsetting variable %s\n", BC_I.var);
			setenv(BC_I.var, BC_I.unsetval);
		}
	}
	return -1;
}

U_BOOT_CMD(
	bootconf, 2, 0,	do_bootconf,
	"bootconf- Parse the uboot.conf file\n",
	"<addr>\n"
	"    - Parse configuration at 'addr'\n"
);

#endif /* CONFIG_CMD_BOOTCONF */

