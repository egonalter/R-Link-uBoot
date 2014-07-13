/*
 * U-Boot configuration for Strasbourg
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

#ifndef __BOARD_CONFIG_H
#define __BOARD_CONFIG_H

#define EPICFAIL_POWEROFF

/************************************************************************
* Temporary settings                                                    *
************************************************************************/
#define __VARIANT_A1		/* Eventually, no-one will be using A1 anymore */

/************************************************************************
* Machine settings                                                      *
************************************************************************/

#define CONFIG_OMAP
#define CONFIG_OMAP36XX
#define CONFIG_OMAP34XX		/* reuse the 34XX setup */
#define CONFIG_STRASBOURG

/************************************************************************
* Platform specific shell commands                                      *
************************************************************************/

#define CONFIG_CMD_VOLTAGE
#define CONFIG_CMD_CLOCK
#define CONFIG_CMD_CLOCK_INFO_CPU

/************************************************************************
* Hardware settings                                                     *
************************************************************************/

#include <asm/arch/cpu.h>

#define CONFIG_TWL4030_USB	1    /* Initialize twl usb */
#define V_OSCK			26000000  /* Clock output from T2 */
#define V_SCLK			(V_OSCK >> 1)

/* 2430 has 12 GP timers, they can be driven by the SysClk (12/13/19.2) or by
 * 32KHz clk, or from external sig. This rate is divided by a local divisor.
 */
#define V_PVT                    7
#define CFG_TIMERBASE            OMAP34XX_GPT2
#define CFG_PVT                  V_PVT  /* 2^(pvt+1) */
#define CFG_HZ                   ((V_SCLK)/(2 << CFG_PVT))

#define PRCM_CLK_CFG2_400MHZ   1    /* VDD2=1.15v - 200MHz DDR */
//#define PRCM_CLK_CFG2_332MHZ     1    /* VDD2=1.15v - 166MHz DDR */
#define PRCM_PCLK_OPP2           1    /* ARM=500MHz - VDD1=1.20v */

#define CONFIG_MISC_INIT_R

#define V_NS16550_CLK            (48000000)  /* 48 MHz */
#define CFG_NS16550
#define CFG_NS16550_SERIAL
#define CFG_NS16550_REG_SIZE     (-4)
#define CFG_NS16550_CLK          V_NS16550_CLK
//#define CFG_NS16550_COM1         OMAP34XX_UART1
//#define CFG_NS16550_COM2         OMAP34XX_UART2
#define CFG_NS16550_COM1         OMAP34XX_UART1

/* select serial console configuration */
#define CONFIG_SERIAL1		1
#define CONFIG_CONS_INDEX	1

#define CONFIG_MMC
#define CFG_MMC_BASE		0xF0000000
#define CFG_I2C_SPEED		400
#define CFG_I2C_SLAVE		1
#define CFG_I2C_BUS		0
#define CFG_I2C_BUS_SELECT
#define CONFIG_DRIVER_OMAP34XX_I2C 1
#define CONFIG_NR_DRAM_BANKS	1
#define PHYS_SDRAM_1		OMAP34XX_SDRC_CS0
#define PHYS_SDRAM_1_SIZE	SZ_128M
#define SDRC_R_B_C		1
#define CONFIG_SW_FLIPFLOP

#ifndef TBD
/* Configure the PISMO */
/** REMOVE ME ***/
#define PISMO1_NOR_SIZE_SDPV2	GPMC_SIZE_128M
#define PISMO1_NOR_SIZE		GPMC_SIZE_64M
#define PISMO1_NAND_SIZE	GPMC_SIZE_128M
#define PISMO1_ONEN_SIZE	GPMC_SIZE_128M
#define DBG_MPDB_SIZE		GPMC_SIZE_16M
#define PISMO2_SIZE		0
#define SERIAL_TL16CP754C_SIZE	GPMC_SIZE_16M
#define CFG_MAX_FLASH_SECT	(520)		/* max number of sectors on one chip */
#define CFG_MAX_FLASH_BANKS      2		/* max number of flash banks */
#define CFG_MONITOR_LEN		SZ_256K 	/* Reserve 2 sectors */
#define PHYS_FLASH_SIZE_SDPV2	SZ_128M
#define PHYS_FLASH_SIZE		SZ_32M
#define CFG_FLASH_BASE		boot_flash_base
#define PHYS_FLASH_SECT_SIZE	boot_flash_sec
#define CFG_FLASH_BANKS_LIST	{0, 0}
#define CFG_MONITOR_BASE	CFG_FLASH_BASE /* Monitor at start of flash */

#ifndef __ASSEMBLY__
extern unsigned int nand_cs_base;
extern unsigned int boot_flash_base;
extern volatile unsigned int boot_flash_env_addr;
extern unsigned int boot_flash_off;
extern unsigned int boot_flash_sec;
extern unsigned int boot_flash_type;
#endif

#endif /* !TBD */

/************************************************************************
* Generic settings                                                      *
************************************************************************/

#include "plat-tomtom.conf"

/************************************************************************
* Android settings                                                      *
************************************************************************/

/* Fastboot variables */
//#define CONFIG_FASTBOOT	        1    /* Using fastboot interface */
#define CFG_FASTBOOT_TRANSFER_BUFFER (PHYS_SDRAM_1 + SZ_16M)
#define CFG_FASTBOOT_TRANSFER_BUFFER_SIZE (SZ_256M - SZ_16M)
#define CFG_FASTBOOT_PREBOOT_KEYS         1
#define CFG_FASTBOOT_PREBOOT_KEY1         0x37 /* 'ok' */
#define CFG_FASTBOOT_PREBOOT_KEY2         0x00 /* unused */
#define CFG_FASTBOOT_PREBOOT_INITIAL_WAIT (0)
#define CFG_FASTBOOT_PREBOOT_LOOP_MAXIMUM (1)
#define CFG_FASTBOOT_PREBOOT_LOOP_WAIT    (0)

#define CONFIG_SW_FLIPFLOP

#endif /* __BOARD_CONFIG_H */

