
/*
 * (C) Copyright 2004-2009
 * Texas Instruments, <www.ti.com>
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
#include <asm/arch/bits.h>
#include <asm/arch/mux.h>
#include <asm/arch/sys_proto.h>
#include <asm/arch/sys_info.h>
#include <asm/arch/clocks.h>
#include <asm/arch/mem.h>
#include <i2c.h>
#include <mmc.h>
#include <asm/mach-types.h>
#include <linux/mtd/nand_ecc.h>
#include <twl4030.h>
#include <flipflop.h>

/* Before including the padconfig settings, we have to set proper config */
#define BOOTLOADER_UBOOT_PADCONFIG
#ifdef __VARIANT_A1
# include <linux/padconfig_strasbourg.h>
#elif defined(__VARIANT_A2)
# include <linux/padconfig_strasbourg_a2.h>
#else
# error Unrecognized variant!
#endif

DECLARE_GLOBAL_DATA_PTR;

extern void detect_boot_mode(void);
extern void hw_watchdog_init(void);

int get_boot_type(void);
void v7_flush_dcache_all(int, int);
void l2cache_enable(void);
void setup_auxcr(int, int);
void eth_init(void *);

#ifdef __VARIANT_A1
# define BOOTDEV_SDCARD 0
# define BOOTDEV_MOVI   1

/* Fudge kernel bootdevice if sd card is present:
 * - root=/dev/mmcblk0p2 if no SD card present OR if booting from SD
 * - root=/dev/mmcblk1p2 if have SD card and booting from movi
 */
int hackdevice;

#else
# define BOOTDEV_MOVI   0
#endif

int bootdev;

/*******************************************************
 * Routine: delay
 * Description: spinning delay to use before udelay works
 ******************************************************/
static inline void delay(unsigned long loops)
{
	__asm__ volatile ("1:\n" "subs %0, %1, #1\n"
			  "bne 1b":"=r" (loops):"0"(loops));
}

/*****************************************
 * Routine: board_init
 * Description: Early hardware init.
 *****************************************/
int board_init(void)
{
	detect_boot_mode();

	if (SYSBOOT_MODE_COLD == gd->tomtom.sysboot_mode)
		/* cold boot => initialize the flipflop, we can't rely on the
		   state of the scratchpad register */
		flipflop_set(0);

	/* In general, load the kernel from MoviNAND; on A1 the SD slot is useful for recovery */
	bootdev = BOOTDEV_MOVI;

#ifdef BOOTDEV_SDCARD
	hackdevice = !mmc_init(0);

	if ((__raw_readl(0x480029c0) & 0xff) == 0x6) { /* Booted from the A1 SD slot? */
		bootdev = BOOTDEV_SDCARD;
		hackdevice = 0;
	}
#endif

	gpmc_init();		/* in SRAM or SDRAM, finish GPMC */
#if defined(__VARIANT_A2)
 	gd->bd->bi_arch_number = MACH_TYPE_STRASBOURG_A2; /* Linux mach id */
#else
 	gd->bd->bi_arch_number = MACH_TYPE_STRASBOURG; /* Linux mach id */
#endif
	gd->bd->bi_boot_params = (OMAP34XX_SDRC_CS0 + 0x100); /* boot param addr */

	return 0;
}

/*****************************************
 * Routine: secure_unlock
 * Description: Setup security registers for access
 * (GP Device only)
 *****************************************/
void secure_unlock_mem(void)
{
	/* Permission values for registers -Full fledged permissions to all */
	#define UNLOCK_1 0xFFFFFFFF
	#define UNLOCK_2 0x00000000
	#define UNLOCK_3 0x0000FFFF

	/* Protection Module Register Target APE (PM_RT)*/
	__raw_writel(UNLOCK_1, RT_REQ_INFO_PERMISSION_1);
	__raw_writel(UNLOCK_1, RT_READ_PERMISSION_0);
	__raw_writel(UNLOCK_1, RT_WRITE_PERMISSION_0);
	__raw_writel(UNLOCK_2, RT_ADDR_MATCH_1);

	__raw_writel(UNLOCK_3, GPMC_REQ_INFO_PERMISSION_0);
	__raw_writel(UNLOCK_3, GPMC_READ_PERMISSION_0);
	__raw_writel(UNLOCK_3, GPMC_WRITE_PERMISSION_0);

	__raw_writel(UNLOCK_3, OCM_REQ_INFO_PERMISSION_0);
	__raw_writel(UNLOCK_3, OCM_READ_PERMISSION_0);
	__raw_writel(UNLOCK_3, OCM_WRITE_PERMISSION_0);
	__raw_writel(UNLOCK_2, OCM_ADDR_MATCH_2);

	/* IVA Changes */
	__raw_writel(UNLOCK_3, IVA2_REQ_INFO_PERMISSION_0);
	__raw_writel(UNLOCK_3, IVA2_READ_PERMISSION_0);
	__raw_writel(UNLOCK_3, IVA2_WRITE_PERMISSION_0);

	__raw_writel(UNLOCK_3, IVA2_REQ_INFO_PERMISSION_1);
	__raw_writel(UNLOCK_3, IVA2_READ_PERMISSION_1);
	__raw_writel(UNLOCK_3, IVA2_WRITE_PERMISSION_1);

	__raw_writel(UNLOCK_3, IVA2_REQ_INFO_PERMISSION_2);
	__raw_writel(UNLOCK_3, IVA2_READ_PERMISSION_2);
	__raw_writel(UNLOCK_3, IVA2_WRITE_PERMISSION_2);

	__raw_writel(UNLOCK_3, IVA2_REQ_INFO_PERMISSION_3);
	__raw_writel(UNLOCK_3, IVA2_READ_PERMISSION_3);
	__raw_writel(UNLOCK_3, IVA2_WRITE_PERMISSION_3);

	__raw_writel(UNLOCK_1, SMS_RG_ATT0); /* SDRC region 0 public */
}


/**********************************************************
 * Routine: secureworld_exit()
 * Description: If chip is EMU and boot type is external
 *		configure secure registers and exit secure world
 *  general use.
 ***********************************************************/
void secureworld_exit(void)
{
	unsigned long i;

	/* configrue non-secure access control register */
	__asm__ __volatile__("mrc p15, 0, %0, c1, c1, 2":"=r" (i));
	/* enabling co-processor CP10 and CP11 accesses in NS world */
	__asm__ __volatile__("orr %0, %0, #0xC00":"=r"(i));
	/* allow allocation of locked TLBs and L2 lines in NS world */
	/* allow use of PLE registers in NS world also */
	__asm__ __volatile__("orr %0, %0, #0x70000":"=r"(i));
	__asm__ __volatile__("mcr p15, 0, %0, c1, c1, 2":"=r" (i));

	/* Enable ASA and IBE in ACR register */
	__asm__ __volatile__("mrc p15, 0, %0, c1, c0, 1":"=r" (i));
	__asm__ __volatile__("orr %0, %0, #0x50":"=r"(i));
	__asm__ __volatile__("mcr p15, 0, %0, c1, c0, 1":"=r" (i));

	/* Exiting secure world */
	__asm__ __volatile__("mrc p15, 0, %0, c1, c1, 0":"=r" (i));
	__asm__ __volatile__("orr %0, %0, #0x31":"=r"(i));
	__asm__ __volatile__("mcr p15, 0, %0, c1, c1, 0":"=r" (i));
}

/**********************************************************
 * Routine: try_unlock_sram()
 * Description: If chip is GP/EMU(special) type, unlock the SRAM for
 *  general use.
 ***********************************************************/
void try_unlock_memory(void)
{
	int mode;
	int in_sdram = running_in_sdram();

	/* if GP device unlock device SRAM for general use */
	/* secure code breaks for Secure/Emulation device - HS/E/T*/
	mode = get_device_type();
	if (mode == GP_DEVICE) {
		secure_unlock_mem();
	}
	/* If device is EMU and boot is XIP external booting
	 * Unlock firewalls and disable L2 and put chip
	 * out of secure world
	 */
	/* Assuming memories are unlocked by the demon who put us in SDRAM */
	if ((mode <= EMU_DEVICE) && (get_boot_type() == 0x1F)
		&& (!in_sdram)) {
		secure_unlock_mem();
		secureworld_exit();
	}

	return;
}

/**********************************************************
 * Routine: s_init
 * Description: Does early system init of muxing and clocks.
 * - Called path is with SRAM stack.
 **********************************************************/
void s_init(void)
{
	int i;
	int external_boot = 0;
	int in_sdram = running_in_sdram();

#ifdef CONFIG_3430VIRTIO
	in_sdram = 0;  /* allow setup from memory for Virtio */
#endif
	hw_watchdog_init();

	external_boot = (get_boot_type() == 0x1F) ? 1 : 0;
	/* Right now flushing at low MPU speed. Need to move after clock init */
	v7_flush_dcache_all(get_device_type(), external_boot);

	try_unlock_memory();

#ifdef CONFIG_3430_AS_3410
	/* setup the scalability control register for
	 * 3430 to work in 3410 mode
	 */
	__raw_writel(0x5A80, CONTROL_SCALABLE_OMAP_OCP);
#endif

	if (cpu_is_3410()) {
		/* Lock down 6-ways in L2 cache so that effective size of L2 is 64K */
		__asm__ __volatile__("mov %0, #0xFC":"=r" (i));
		__asm__ __volatile__("mcr p15, 1, %0, c9, c0, 0":"=r" (i));
	}

#ifndef CONFIG_ICACHE_OFF
	icache_enable();
#endif

#ifdef CONFIG_L2_OFF
	l2cache_disable();
#else
	l2cache_enable();
#endif
	set_muxconf_regs();
	delay(100);
	
	/* Writing to AuxCR in U-boot using SMI for GP/EMU DEV */
	/* Currently SMI in Kernel on ES2 devices seems to have an isse
	 * Once that is resolved, we can postpone this config to kernel
	 */
	setup_auxcr(get_device_type(), external_boot);

	prcm_init();

	per_clocks_enable();
}

/*******************************************************
 * Routine: misc_init_r
 * Description: Init ethernet (done here so udelay works)
 ********************************************************/
int misc_init_r(void)
{
#ifdef CONFIG_DRIVER_OMAP34XX_I2C
    unsigned char data;
 
    i2c_init(CFG_I2C_SPEED, CFG_I2C_SLAVE);

	twl4030_usb_init();

	/* see if we need to activate the power button startup */
	char *s = getenv("pbboot");
	if (s) {
		/* figure out why we have booted */
		i2c_read(0x4b, 0x3a, 1, &data, 1);

		/* if status is non-zero, we didn't transition
		 * from WAIT_ON state
		 */
		if (data) {
			printf("Transitioning to Wait State (%x)\n", data);

			/* clear status */
			data = 0;
			i2c_write(0x4b, 0x3a, 1, &data, 1);

			/* put PM into WAIT_ON state */
			data = 0x01;
			i2c_write(0x4b, 0x46, 1, &data, 1);

			/* no return - wait for power shutdown */
			while (1) {;}
		}
		printf("Transitioning to Active State (%x)\n", data);

		/* turn on long pwr button press reset*/
		data = 0x40;
		i2c_write(0x4b, 0x46, 1, &data, 1);
		printf("Power Button Active\n");
	}
#endif
	dieid_num_r();
	return (0);
}

/**********************************************
 * Routine: dram_init
 * Description: sets uboots idea of sdram size
 **********************************************/
int dram_init(void)
{
    #define NOT_EARLY 0
    DECLARE_GLOBAL_DATA_PTR;
	unsigned int size0 = 0;
	u32 mtype, btype;

	btype = get_board_type();
	mtype = get_mem_type();
#ifndef CONFIG_3430ZEBU
	/* fixme... dont know why this func is crashing in ZeBu */
	display_board_info(btype);
#endif
    /* If a second bank of DDR is attached to CS1 this is
     * where it can be started.  Early init code will init
     * memory on CS0.
     */
	if ((mtype == DDR_COMBO) || (mtype == DDR_STACKED)) {
		do_sdrc_init(SDRC_CS1_OSET, NOT_EARLY);
	}
	size0 = get_sdr_cs_size(SDRC_CS0_OSET);

	gd->bd->bi_dram[0].start = PHYS_SDRAM_1;
	gd->bd->bi_dram[0].size = size0;

	return 0;
}

#define 	MUX_VAL(OFFSET,VALUE)\
		__raw_writew((VALUE), OMAP34XX_CTRL_BASE + (OFFSET));

#define		CP(x)	(CONTROL_PADCONF_##x)
/*
 * IEN  - Input Enable
 * IDIS - Input Disable
 * PTD  - Pull type Down
 * PTU  - Pull type Up
 * DIS  - Pull type selection is inactive
 * EN   - Pull type selection is active
 * M0   - Mode 0
 * The commented string gives the final mux configuration for that pin
 */
#define MUX_DEFAULT_ES2()\
	/*Die to Die */\
	MUX_VAL(CP(d2d_mcad0),      (IEN  | PTD | EN  | M0)) /*d2d_mcad0*/\
	MUX_VAL(CP(d2d_mcad1),      (IEN  | PTD | EN  | M0)) /*d2d_mcad1*/\
	MUX_VAL(CP(d2d_mcad2),      (IEN  | PTD | EN  | M0)) /*d2d_mcad2*/\
	MUX_VAL(CP(d2d_mcad3),      (IEN  | PTD | EN  | M0)) /*d2d_mcad3*/\
	MUX_VAL(CP(d2d_mcad4),      (IEN  | PTD | EN  | M0)) /*d2d_mcad4*/\
	MUX_VAL(CP(d2d_mcad5),      (IEN  | PTD | EN  | M0)) /*d2d_mcad5*/\
	MUX_VAL(CP(d2d_mcad6),      (IEN  | PTD | EN  | M0)) /*d2d_mcad6*/\
	MUX_VAL(CP(d2d_mcad7),      (IEN  | PTD | EN  | M0)) /*d2d_mcad7*/\
	MUX_VAL(CP(d2d_mcad8),      (IEN  | PTD | EN  | M0)) /*d2d_mcad8*/\
	MUX_VAL(CP(d2d_mcad9),      (IEN  | PTD | EN  | M0)) /*d2d_mcad9*/\
	MUX_VAL(CP(d2d_mcad10),     (IEN  | PTD | EN  | M0)) /*d2d_mcad10*/\
	MUX_VAL(CP(d2d_mcad11),     (IEN  | PTD | EN  | M0)) /*d2d_mcad11*/\
	MUX_VAL(CP(d2d_mcad12),     (IEN  | PTD | EN  | M0)) /*d2d_mcad12*/\
	MUX_VAL(CP(d2d_mcad13),     (IEN  | PTD | EN  | M0)) /*d2d_mcad13*/\
	MUX_VAL(CP(d2d_mcad14),     (IEN  | PTD | EN  | M0)) /*d2d_mcad14*/\
	MUX_VAL(CP(d2d_mcad15),     (IEN  | PTD | EN  | M0)) /*d2d_mcad15*/\
	MUX_VAL(CP(d2d_mcad16),     (IEN  | PTD | EN  | M0)) /*d2d_mcad16*/\
	MUX_VAL(CP(d2d_mcad17),     (IEN  | PTD | EN  | M0)) /*d2d_mcad17*/\
	MUX_VAL(CP(d2d_mcad18),     (IEN  | PTD | EN  | M0)) /*d2d_mcad18*/\
	MUX_VAL(CP(d2d_mcad19),     (IEN  | PTD | EN  | M0)) /*d2d_mcad19*/\
	MUX_VAL(CP(d2d_mcad20),     (IEN  | PTD | EN  | M0)) /*d2d_mcad20*/\
	MUX_VAL(CP(d2d_mcad21),     (IEN  | PTD | EN  | M0)) /*d2d_mcad21*/\
	MUX_VAL(CP(d2d_mcad22),     (IEN  | PTD | EN  | M0)) /*d2d_mcad22*/\
	MUX_VAL(CP(d2d_mcad23),     (IEN  | PTD | EN  | M0)) /*d2d_mcad23*/\
	MUX_VAL(CP(d2d_mcad24),     (IEN  | PTD | EN  | M0)) /*d2d_mcad24*/\
	MUX_VAL(CP(d2d_mcad25),     (IEN  | PTD | EN  | M0)) /*d2d_mcad25*/\
	MUX_VAL(CP(d2d_mcad26),     (IEN  | PTD | EN  | M0)) /*d2d_mcad26*/\
	MUX_VAL(CP(d2d_mcad27),     (IEN  | PTD | EN  | M0)) /*d2d_mcad27*/\
	MUX_VAL(CP(d2d_mcad28),     (IEN  | PTD | EN  | M0)) /*d2d_mcad28*/\
	MUX_VAL(CP(d2d_mcad29),     (IEN  | PTD | EN  | M0)) /*d2d_mcad29*/\
	MUX_VAL(CP(d2d_mcad30),     (IEN  | PTD | EN  | M0)) /*d2d_mcad30*/\
	MUX_VAL(CP(d2d_mcad31),     (IEN  | PTD | EN  | M0)) /*d2d_mcad31*/\
	MUX_VAL(CP(d2d_mcad32),     (IEN  | PTD | EN  | M0)) /*d2d_mcad32*/\
	MUX_VAL(CP(d2d_mcad33),     (IEN  | PTD | EN  | M0)) /*d2d_mcad33*/\
	MUX_VAL(CP(d2d_mcad34),     (IEN  | PTD | EN  | M0)) /*d2d_mcad34*/\
	MUX_VAL(CP(d2d_mcad35),     (IEN  | PTD | EN  | M0)) /*d2d_mcad35*/\
	MUX_VAL(CP(d2d_mcad36),     (IEN  | PTD | EN  | M0)) /*d2d_mcad36*/\
	MUX_VAL(CP(d2d_clk26mi),    (IEN  | PTD | DIS | M0)) /*d2d_clk26mi  */\
	MUX_VAL(CP(d2d_nrespwron ), (IEN  | PTD | EN  | M0)) /*d2d_nrespwron*/\
	MUX_VAL(CP(d2d_nreswarm),   (IEN  | PTU | EN  | M0)) /*d2d_nreswarm */\
	MUX_VAL(CP(d2d_arm9nirq),   (IEN  | PTD | DIS | M0)) /*d2d_arm9nirq */\
	MUX_VAL(CP(d2d_uma2p6fiq ), (IEN  | PTD | DIS | M0)) /*d2d_uma2p6fiq*/\
	MUX_VAL(CP(d2d_spint),      (IEN  | PTD | EN  | M0)) /*d2d_spint*/\
	MUX_VAL(CP(d2d_frint),      (IEN  | PTD | EN  | M0)) /*d2d_frint*/\
	MUX_VAL(CP(d2d_dmareq0),    (IEN  | PTD | DIS | M0)) /*d2d_dmareq0  */\
	MUX_VAL(CP(d2d_dmareq1),    (IEN  | PTD | DIS | M0)) /*d2d_dmareq1  */\
	MUX_VAL(CP(d2d_dmareq2),    (IEN  | PTD | DIS | M0)) /*d2d_dmareq2  */\
	MUX_VAL(CP(d2d_dmareq3),    (IEN  | PTD | DIS | M0)) /*d2d_dmareq3  */\
	MUX_VAL(CP(d2d_n3gtrst),    (IEN  | PTD | DIS | M0)) /*d2d_n3gtrst  */\
	MUX_VAL(CP(d2d_n3gtdi),     (IEN  | PTD | DIS | M0)) /*d2d_n3gtdi*/\
	MUX_VAL(CP(d2d_n3gtdo),     (IEN  | PTD | DIS | M0)) /*d2d_n3gtdo*/\
	MUX_VAL(CP(d2d_n3gtms),     (IEN  | PTD | DIS | M0)) /*d2d_n3gtms*/\
	MUX_VAL(CP(d2d_n3gtck),     (IEN  | PTD | DIS | M0)) /*d2d_n3gtck*/\
	MUX_VAL(CP(d2d_n3grtck),    (IEN  | PTD | DIS | M0)) /*d2d_n3grtck  */\
	MUX_VAL(CP(d2d_mstdby),     (IEN  | PTU | EN  | M0)) /*d2d_mstdby*/\
	MUX_VAL(CP(d2d_swakeup),    (IEN  | PTD | EN  | M0)) /*d2d_swakeup  */\
	MUX_VAL(CP(d2d_idlereq),    (IEN  | PTD | DIS | M0)) /*d2d_idlereq  */\
	MUX_VAL(CP(d2d_idleack),    (IEN  | PTU | EN  | M0)) /*d2d_idleack  */\
	MUX_VAL(CP(d2d_mwrite),     (IEN  | PTD | DIS | M0)) /*d2d_mwrite*/\
	MUX_VAL(CP(d2d_swrite),     (IEN  | PTD | DIS | M0)) /*d2d_swrite*/\
	MUX_VAL(CP(d2d_mread),      (IEN  | PTD | DIS | M0)) /*d2d_mread*/\
	MUX_VAL(CP(d2d_sread),      (IEN  | PTD | DIS | M0)) /*d2d_sread*/\
	MUX_VAL(CP(d2d_mbusflag),   (IEN  | PTD | DIS | M0)) /*d2d_mbusflag */\
	MUX_VAL(CP(d2d_sbusflag),   (IEN  | PTD | DIS | M0)) /*d2d_sbusflag */\
	MUX_VAL(CP(sdrc_cke0),      (IDIS | PTU | EN  | M0)) /*sdrc_cke0 */\
	MUX_VAL(CP(sdrc_cke1),      (IDIS | PTD | DIS | M7)) /*sdrc_cke1 unused*/

/**********************************************************
 * Routine: set_muxconf_regs
 * Description: Setting up the configuration Mux registers
 *              specific to the hardware. Many pins need
 *              to be moved from protect to primary mode.
 *********************************************************/
void set_muxconf_regs(void)
{
	PADCONFIG_SETTINGS_UBOOT
	PADCONFIG_SETTINGS_COMMON
	MUX_DEFAULT_ES2();
}

/******************************************************************************
 * Routine: update_mux()
 * Description:Update balls which are different between boards.  All should be
 *             updated to match functionality.  However, I'm only updating ones
 *             which I'll be using for now.  When power comes into play they
 *             all need updating.
 *****************************************************************************/
void update_mux(u32 btype, u32 mtype)
{
	/* NOTHING as of now... */
}

void board_env_init()
{
	/* Replace with factory data when available */
	switch(gd->bd->bi_arch_number) {
	case MACH_TYPE_STRASBOURG_A2:
		setenv("kernel.console", "ttyO2");
		break;
	case MACH_TYPE_STRASBOURG:
	default:
		setenv("kernel.console", "ttyO0");
		break;
	}
}
