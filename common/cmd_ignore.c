/*
 * Prefix to ignore return code of a given command
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

#include <common.h>
#include <command.h>

#if defined(CONFIG_CMD_IGNORE)

/* Note - we still return error if the command is not well-formed */
int do_ignore (cmd_tbl_t * cmdtp, int flag, int argc, char *argv[])
{
	if (argc < 2) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return -1;
	}

	argv = &argv[1];
	argc--;

	clear_ctrlc();		/* forget any previous Control C */

	/* Look up command in command table */
	if ((cmdtp = find_cmd(argv[0])) == NULL) {
		printf ("Unknown command '%s' - try 'help'\n", argv[0]);
		return -1;
	}

	/* found - check max args */
	if (argc > cmdtp->maxargs) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return -1;
	}

	/* OK - call function to do the command */
	(cmdtp->cmd) (cmdtp, flag, argc, argv);

	/* Did the user stop this? */
	if (had_ctrlc ()) {
		return -1;	/* if stopped then not repeatable */
	}

	return 0;
}

U_BOOT_CMD(
	ignore, CFG_MAXARGS, 0,	do_ignore,
	"ignore- Ignore return code of argument\n",
	"<command> [args ...]\n"
	"    - Execute 'command' with 'args' and carry on\n"
	"      with boot regardless\n"
);

#endif /* CONFIG_CMD_IGNORE */

