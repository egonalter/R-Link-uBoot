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
 * Detect the boot mode evaluating the PRM_RSTTST and a register in the
 * scratchpad holding a magic value on software reset
 */

#include <asm/io.h>
#include <common.h>

#define BOOT_MODE_AUX_REG	(OMAP34XX_CTRL_BASE + 0x09FC)
#define BOOT_MODE_AUX_MASK	0x3fffffff
#define BOOT_MODE_AUX_MAGIC	0x3eafc35b

DECLARE_GLOBAL_DATA_PTR;

void detect_boot_mode(void)
{
	u32 val = __raw_readl(PRM_RSTTST);

	if (val & GLOBAL_COLD_RST) {
		/* cold boot or software reset */

		val = __raw_readl(BOOT_MODE_AUX_REG);

		if ((val & BOOT_MODE_AUX_MASK) == BOOT_MODE_AUX_MAGIC) {
			/* the register contains the magic value => software reset */
			gd->tomtom.sysboot_mode = SYSBOOT_MODE_WARM;
		} else {
			gd->tomtom.sysboot_mode = SYSBOOT_MODE_COLD;

			/* write the magic value to the register */
			val = ((val & ~BOOT_MODE_AUX_MASK) | BOOT_MODE_AUX_MAGIC);
			__raw_writel(val, BOOT_MODE_AUX_REG);
		}
	} else if (val & MPU_WD_RST) {
		gd->tomtom.sysboot_mode = SYSBOOT_MODE_WATCHDOG;
	} else {
		/* treat any other case as watchdog reset */
		gd->tomtom.sysboot_mode = SYSBOOT_MODE_WATCHDOG;
	}

	/* reset all bits in PRM_RSTTST */
	__raw_writel(0xFFFFFFFF, PRM_RSTTST);
}
