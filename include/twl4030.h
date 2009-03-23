/*
 * (C) Copyright 2009
 * Windriver, <www.windriver.com>
 * Tom Rix <Tom.Rix@windriver.com>
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
#include <i2c.h>

/* I2C chip addresses */

/* USB ID */
#define TWL4030_CHIP_USB		0x48
/* AUD ID */
#define TWL4030_CHIP_AUDIO_VOICE	0x49
#define TWL4030_CHIP_GPIO		0x49
#define TWL4030_CHIP_INTBR		0x49
#define TWL4030_CHIP_PIH		0x49
#define TWL4030_CHIP_TEST		0x49
/* AUX ID */
#define TWL4030_CHIP_KEYPAD		0x4a
#define TWL4030_CHIP_MADC		0x4a
#define TWL4030_CHIP_INTERRUPTS		0x4a
#define TWL4030_CHIP_LED		0x4a
#define TWL4030_CHIP_MAIN_CHARGE	0x4a
#define TWL4030_CHIP_PRECHARGE		0x4a
#define TWL4030_CHIP_PWM0		0x4a
#define TWL4030_CHIP_PWM1		0x4a
#define TWL4030_CHIP_PWMA		0x4a
#define TWL4030_CHIP_PWMB		0x4a
/* POWER ID */
#define TWL4030_CHIP_BACKUP		0x4b
#define TWL4030_CHIP_INT		0x4b
#define TWL4030_CHIP_PM_MASTER		0x4b
#define TWL4030_CHIP_PM_RECIEVER	0x4b
#define TWL4030_CHIP_RTC		0x4b
#define TWL4030_CHIP_SECURED_REG	0x4b

/* Register base addresses */

/* USB ID */
#define TWL4030_BASEADD_USB		0x0000
/* AUD ID */
#define TWL4030_BASEADD_AUDIO_VOICE	0x0000
#define TWL4030_BASEADD_GPIO		0x0098
#define TWL4030_BASEADD_INTBR		0x0085
#define TWL4030_BASEADD_PIH		0x0080
#define TWL4030_BASEADD_TEST		0x004C
/* AUX ID */
#define TWL4030_BASEADD_INTERRUPTS	0x00B9
#define TWL4030_BASEADD_LED		0x00EE
#define TWL4030_BASEADD_MADC		0x0000
#define TWL4030_BASEADD_MAIN_CHARGE	0x0074
#define TWL4030_BASEADD_PRECHARGE	0x00AA
#define TWL4030_BASEADD_PWM0		0x00F8
#define TWL4030_BASEADD_PWM1		0x00FB
#define TWL4030_BASEADD_PWMA		0x00EF
#define TWL4030_BASEADD_PWMB		0x00F1
#define TWL4030_BASEADD_KEYPAD		0x00D2
/* POWER ID */
#define TWL4030_BASEADD_BACKUP		0x0014
#define TWL4030_BASEADD_INT		0x002E
#define TWL4030_BASEADD_PM_MASTER	0x0036
#define TWL4030_BASEADD_PM_RECIEVER	0x005B
#define TWL4030_BASEADD_RTC		0x001C
#define TWL4030_BASEADD_SECURED_REG	0x0000

/* Register addresses */

#define REG_STS_HW_CONDITIONS	(TWL4030_BASEADD_PM_MASTER + 0x0F)
#define 	STS_VBUS		0x080
#define 	STS_CHG			0x02
#define REG_BCICTL1 		(TWL4030_BASEADD_PM_MASTER + 0x023)
#define REG_BCICTL2 		(TWL4030_BASEADD_PM_MASTER + 0x024)
#define 	CGAIN			0x020
#define 	ITHEN			0x010
#define 	ITHSENS			0x007
#define REG_BCIMFTH1		(TWL4030_BASEADD_PM_MASTER + 0x016)
#define REG_BCIMFTH2		(TWL4030_BASEADD_PM_MASTER + 0x017)
#define BCIAUTOWEN		(TWL4030_BASEADD_PM_MASTER + 0x020)
#define 	CONFIG_DONE		0x010
#define 	BCIAUTOUSB		0x002
#define 	BCIAUTOAC		0x001
#define 	BCIMSTAT_MASK		0x03F
#define REG_BOOT_BCI		(TWL4030_BASEADD_PM_MASTER + 0x007)

#define REG_GPBR1		(TWL4030_BASEADD_INTBR + 0x0c)
#define 	MADC_HFCLK_EN		0x80
#define 	DEFAULT_MADC_CLK_EN	0x10

#define REG_CTRL1		(TWL4030_BASEADD_MADC + 0x00)
#define 	MADC_ON			0x01
#define 	REG_SW1SELECT_MSB	0x07
#define 	SW1_CH9_SEL		0x02
#define REG_CTRL_SW1		(TWL4030_BASEADD_MADC + 0x012)
#define 	SW1_TRIGGER		0x020
#define 	EOC_SW1			0x002
#define 	BUSY			0x001
#define REG_GPCH9		(TWL4030_BASEADD_MADC + 0x049)

#define REG_BCIMSTATEC		(TWL4030_BASEADD_MAIN_CHARGE + 0x002)
#define REG_BCIMFSTS2		(TWL4030_BASEADD_MAIN_CHARGE + 0x00E)
#define REG_BCIMFSTS3 		(TWL4030_BASEADD_MAIN_CHARGE + 0x00F)
#define REG_BCIMFSTS4		(TWL4030_BASEADD_MAIN_CHARGE + 0x010)
#define REG_BCIMFKEY		(TWL4030_BASEADD_MAIN_CHARGE + 0x011)
#define REG_BCIIREF1		(TWL4030_BASEADD_MAIN_CHARGE + 0x027)

#define REG_BCIMFSTS1		(TWL4030_BASEADD_PRECHARGE + 0x001)
#define 	USBFASTMCHG		0x004
#define 	BATSTSPCHG		0x004
#define 	BATSTSMCHG		0x040
#define 	VBATOV4			0x020
#define 	VBATOV3			0x010
#define 	VBATOV2			0x008
#define 	VBATOV1			0x004
#define 	MADC_LSB_MASK		0xC0
#define REG_BB_CFG		(TWL4030_BASEADD_PM_RECIEVER + 0x12)
#define 	BBCHEN			0x10
#define		BBSEL_2500mV		0x00
#define		BBSEL_3000mV		0x04
#define		BBSEL_3100mV		0x08
#define		BBSEL_3200mV		0x0C
#define		BBISEL_25uA		0x00
#define		BBISEL_150uA		0x01
#define		BBISEL_500uA		0x02
#define		BBISEL_1000uA		0x03

#define REG_POWER_CTRL		(TWL4030_BASEADD_USB + 0x0AC)
#define REG_POWER_CTRL_SET 	(TWL4030_BASEADD_USB + 0x0AD)
#define REG_POWER_CTRL_CLR	(TWL4030_BASEADD_USB + 0x0AE)
#define 	OTG_EN			0x020
#define REG_PHY_CLK_CTRL	(TWL4030_BASEADD_USB + 0x0FE)
#define REG_PHY_CLK_CTRL_STS 	(TWL4030_BASEADD_USB + 0x0FF)
#define 	PHY_DPLL_CLK		0x01

/*  TWL4030 battery measuring parameters */
#define T2_BATTERY_VOLT		(TWL4030_BASEADD_MAIN_CHARGE + 0x04)
#define T2_BATTERY_TEMP		(TWL4030_BASEADD_MAIN_CHARGE + 0x06)
#define T2_BATTERY_CUR		(TWL4030_BASEADD_MAIN_CHARGE + 0x08)
#define T2_BATTERY_ACVOLT	(TWL4030_BASEADD_MAIN_CHARGE + 0x0A)
#define T2_BATTERY_USBVOLT	(TWL4030_BASEADD_MAIN_CHARGE + 0x0C)

/* Keypad */
#define KEYPAD_KEYP_CTRL_REG    0xD2
#define KEYPAD_KEY_DEB_REG      0xD3
#define KEYPAD_LONG_KEY_REG1    0xD4
#define KEYPAD_LK_PTV_REG       0xD5
#define KEYPAD_TIME_OUT_REG1    0xD6
#define KEYPAD_TIME_OUT_REG2    0xD7
#define KEYPAD_KBC_REG          0xD8
#define KEYPAD_KBR_REG          0xD9
#define KEYPAD_KEYP_SMS         0xDA
#define KEYPAD_FULL_CODE_7_0    0xDB
#define KEYPAD_FULL_CODE_15_8   0xDC
#define KEYPAD_FULL_CODE_23_16  0xDD
#define KEYPAD_FULL_CODE_31_24  0xDE
#define KEYPAD_FULL_CODE_39_32  0xDF
#define KEYPAD_FULL_CODE_47_40  0xE0
#define KEYPAD_FULL_CODE_55_48  0xE1
#define KEYPAD_FULL_CODE_63_56  0xE2
#define KEYPAD_KEYP_ISR1        0xE3
#define KEYPAD_KEYP_IMR1        0xE4
#define KEYPAD_KEYP_ISR2        0xE5
#define KEYPAD_KEYP_IMR2        0xE6
#define KEYPAD_KEYP_SIR         0xE7
#define KEYPAD_KEYP_EDR         0xE8
#define KEYPAD_KEYP_SIH_CTRL    0xE9

#define CTRL_KBD_ON             (1 << 6)
#define CTRL_RP_EN              (1 << 5)
#define CTRL_TOLE_EN            (1 << 4)
#define CTRL_TOE_EN             (1 << 3)
#define CTRL_LK_EN              (1 << 2)
#define CTRL_SOFTMODEN          (1 << 1)
#define CTRL_SOFT_NRST          (1 << 0)

/* Declarations for users of the keypad, stubs for everyone else. */
#if (defined(CONFIG_TWL4030_KEYPAD) && (CONFIG_TWL4030_KEYPAD))
int twl4030_keypad_init(void);
int twl4030_keypad_reset(void);
int twl4030_keypad_keys_pressed(unsigned char *key1, unsigned char *key2);
#else
#define twl4030_keypad_init() 0
#define twl4030_keypad_reset() 0
#define twl4030_keypad_keys_pressed(a, b) 0
#endif

