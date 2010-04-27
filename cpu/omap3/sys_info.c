/*
 * Copyright (c) 2009 Wind River Systems, Inc.
 * Tom Rix <Tom.Rix@windriver.com>
 *
 * Derived from board specific omap code by
 * Richard Woodruff <r-woodruff2@ti.com>
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <asm/arch/cpu.h>
#include <asm/arch/sys_info.h>
#include <asm/io.h>
#include <asm/arch/rev.h>

/*
 * get_cpu_rev(void) - extract version info
 */
u32 get_cpu_rev(void)
{
	u32 cpuid = 0;
	ctrl_id_t *id_base;
	/*
	 * On ES1.0 the IDCODE register is not exposed on L4
	 * so using CPU ID to differentiate between ES1.0 and > ES1.0.
	 */
	__asm__ __volatile__("mrc p15, 0, %0, c0, c0, 0":"=r" (cpuid));
	if ((cpuid  & 0xf) == 0x0)
		return CPU_3XX_ES10;
	else {
		/* Decode the IDs on > ES1.0 */
		id_base = (ctrl_id_t *) OMAP34XX_ID_L4_IO_BASE;

		cpuid = (__raw_readl(&id_base->idcode) >> CPU_3XX_ID_SHIFT) & 0xf;

		/* Some early ES2.0 seem to report ID 0, fix this */
		if (cpuid == 0)
			cpuid = CPU_3XX_ES20;

		return cpuid;
	}
}

/*
 * dieid_num_r(void) - read and set die ID
 */
void dieid_num_r(void)
{
	ctrl_id_t *id_base = (ctrl_id_t *)OMAP34XX_ID_L4_IO_BASE;
	char *uid_s, die_id[34];
	u32 id[4];

	memset(die_id, 0, sizeof(die_id));

	uid_s = getenv("dieid#");

	if (uid_s == NULL) {
		id[3] = __raw_readl(&id_base->die_id_0);
		id[2] = __raw_readl(&id_base->die_id_1);
		id[1] = __raw_readl(&id_base->die_id_2);
		id[0] = __raw_readl(&id_base->die_id_3);
		sprintf(die_id, "%08x%08x%08x%08x", id[0], id[1], id[2], id[3]);
		setenv("dieid#", die_id);
		uid_s = die_id;
	}
}

/*
 * get_cortex_a8_rev(void) - read Main ID register and extract release
 * and patch information. If the value is not ARM Cortex-A8 the
function
 * returns zero. Otherwise, the return value is an 8-bit number where
 * bit[7:4] indicates release number and bit[3:0] indicates patch number.
 */
u32 get_cortex_a8_rev(void)
{
	u32 cortexrev = 0;

	/* read Main ID register */
	__asm__ __volatile__("mrc p15, 0, r0, c0, c0, 0");

	/* extract processor type and version into r10 */
	__asm__ __volatile__("adr r2, proc_mask");
	__asm__ __volatile__("ldr r10, [r2]");
	__asm__ __volatile__("and r10, r10, r0");

	/* load cortex-a8 constant in r8 for comparison */
	__asm__ __volatile__("adr r2, arm_cortex_a8_proc");
	__asm__ __volatile__("ldr r8, [r2]");
	__asm__ __volatile__("teq r10, r8");

	/* extract release & patch numbers of cortex-a8 */
	__asm__ __volatile__("andeq r4, r0, #0x00f00000");
	__asm__ __volatile__("andeq r5, r0, #0x0000000f");
	__asm__ __volatile__("orreq %0, r5, r4, lsr #20-4" : "=r"
(cortexrev));

	__asm__ __volatile__("b end");

	__asm__ __volatile__("proc_mask:\
				.word 0xff00fff0");
	__asm__ __volatile__("arm_cortex_a8_proc:\
				.word 0x4100c080");
	__asm__ __volatile__("end:");

	return cortexrev;
}
