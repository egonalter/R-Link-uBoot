/*
 * (C) Copyright 2003
 * Kyle Harris, kharris@nexus-tech.net
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

#include <common.h>
#include <command.h>

#if (CONFIG_COMMANDS & CFG_CMD_MMC)

#include <mmc.h>

static inline int str2long(char *p, unsigned int *num)
{
	char *endptr;

	*num = simple_strtoul(p, &endptr, 16);
	return (*p != '\0' && *endptr == '\0') ? 1 : 0;
}

int do_mmc (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int  ret = -1;
	unsigned int dev_num, addr = 0, offset = 0, size = 0, total;

	switch (argc) {
	case 2:
	if ((strncmp(argv[0], "mmcinit", 7) == 0) &&
					(argv[0][7] == '\0')) {
		if (!(str2long(argv[1], &dev_num))) {
			printf("'%s' is not a number\n", argv[1]);
			goto usage;
			}
		if ((dev_num != 0) && (dev_num != 1))
			goto usage;
		ret = mmc_init(dev_num);
		if (ret)
			printf("No MMC card found!\n");
		else
			printf("MMC%d Initalization OK\n", dev_num+1);

		return ret;
		}
		break;
	case 3:
	if ((strncmp(argv[0], "mmc", 3) == 0) && (argv[0][3] == '\0')) {

		if ((strncmp(argv[1], "init", 4) == 0)
					&& (argv[1][4] == '\0')) {
			if (!(str2long(argv[2], &dev_num))) {
				printf("'%s' is not a number\n", argv[2]);
				goto usage;
		}
			if ((dev_num != 0) && (dev_num != 1))
				goto usage;
			ret = mmc_init(dev_num);
			if (ret)
				printf("No MMC card found!\n");
			else
				printf("MMC%d Initalization OK\n", dev_num+1);

			return ret;
		} else if (argc  == 6) {
			total = 0;
			/* validate the input */
			if (!(str2long(argv[2], &dev_num))) {
				printf("'%s' is not a number\n", argv[2]);
				goto usage;
			} else if (!(str2long(argv[3], &addr))) {
				printf("'%s' is not a number\n", argv[3]);
				goto usage;
			} else if (!(str2long(argv[4], &offset))) {
				printf("'%s' is not a number\n", argv[4]);
				goto usage;
			} else if (!(str2long(argv[5], &size))) {
				printf("'%s' is not a number\n", argv[5]);
				goto usage;
			}
			/* validate the params */
			if (((dev_num != 0) && (dev_num != 1)) || (size == 0))
				ret = -1;
			else
				ret = 0;
		}

			if (ret)
				goto usage;

		if ((strncmp(argv[1], "write.i", 7) == 0)
						&& (argv[1][7] == '\0')) {
			ret = mmc_write_opts(dev_num,
				offset, size, (unsigned char *)addr, &total);
			printf("%d bytes written: %s\n",
				total, ret ? "OK" : "ERROR");
		} else if ((strncmp(argv[1], "read.i", 6) == 0) &&
						(argv[1][6] == '\0')) {
			ret = mmc_read_opts(dev_num, offset, size,
				(unsigned char *)addr);
			printf("%d bytes read: %s\n",
				size, ret ? "OK" : "ERROR");
		} else if ((strncmp(argv[1], "write", 5) == 0) &&
				(argv[1][5] == '\0')) {
			ret = mmc_write_block(dev_num, offset, size,
						(unsigned char *)addr, &total);
			printf("%d sectors written: %s\n",
				total, ret ? "OK" : "ERROR");
		} else if ((strncmp(argv[1], "read", 4) == 0) &&
							(argv[1][4] == '\0')) {
			ret = mmc_read_block(dev_num, offset, size,
						(unsigned char *)addr);
			printf("Sectors read: %s\n", ret ? "OK" : "ERROR");
			} else {

			goto usage;
			}
		/* status of the operation */
		return ret;
		}

		break;

	default:
		goto usage;
		break;
	}

usage:
	printf("Usage:\n%s\n", cmdtp->usage);
	return 1;
}

U_BOOT_CMD(
	mmc, 6, 1, do_mmc,
	"mmc - MMC sub-system\n"
	"mmc init <dev>\n"
	"mmc read[.i]/write[.i] <dev> <addr> <block offset> [size]\n",
	"mmc init - device (0 for MMC1, 1 for MMC2)\n"
	"mmc read[.i] device addr offset size - \n"
	"mmc write[.i] device addr offset size - \n"
	"--- write `size' starting at offset `offset' to/from eMMC device\n"
	"device - 0/1 for MMC1/MMC2\n" "`addr' is memory location\n"
);
U_BOOT_CMD(
	mmcinit, 2, 0, do_mmc,
	"mmcinit <dev> - init mmc card (0 for MMC1, 1 for MMC2)\n",
	NULL
);

#endif	/* CFG_CMD_MMC */
