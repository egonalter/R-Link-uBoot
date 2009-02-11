/*
 * TWL4030 battery charger driver
 *
 * Copyright (C) 2007-2008 Texas Instruments, Inc.
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#include <common.h>
#include <i2c.h>
#include <asm/arch/led.h>

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

#define mdelay(n) ({ unsigned long msec = (n); while (msec--) udelay(1000); })

/* Functions to read and write from TWL4030 */
static inline int twl4030_i2c_write_u8(u8 chip_no, u8 val, u8 reg)
{
	return i2c_write(chip_no, reg, 1, &val, 1);
}

static inline int twl4030_i2c_read_u8(u8 chip_no, u8 *val, u8 reg)
{
	return i2c_read(chip_no, reg, 1, val, 1);
}

/*
 * Sets and clears bits on an given register on a given module
 */
static inline int clear_n_set(u8 chip_no, u8 clear, u8 set, u8 reg)
{
	int ret;
	u8 val = 0;

	/* Gets the initial register value */
	ret = twl4030_i2c_read_u8(chip_no, &val, reg);
	if (ret) {
		printf("a\n");
		return ret;
	}

	/* Clearing all those bits to clear */
	val &= ~(clear);

	/* Setting all those bits to set */
	val |= set;

	/* Update the register */
	ret = twl4030_i2c_write_u8(chip_no, val, reg);
	if (ret) {
		printf("b\n");
		return ret;
	}
	return 0;
}

/*
 * Disable/Enable AC Charge funtionality.
 */
static int twl4030_ac_charger_enable(int enable)
{
	int ret;

	if (enable) {
		/* forcing the field BCIAUTOAC (BOOT_BCI[0]) to 1 */
		ret = clear_n_set(TWL4030_CHIP_PM_MASTER, 0,
			       (CONFIG_DONE | BCIAUTOWEN | BCIAUTOAC),
			       REG_BOOT_BCI);
		if (ret)
			return ret;
	} else {
		/* forcing the field BCIAUTOAC (BOOT_BCI[0]) to 0 */
		ret = clear_n_set(TWL4030_CHIP_PM_MASTER, BCIAUTOAC,
			       (CONFIG_DONE | BCIAUTOWEN),
			       REG_BOOT_BCI);
		if (ret)
			return ret;
	}
	return 0;
}

/*
 * Disable/Enable USB Charge funtionality.
 */
static int twl4030_usb_charger_enable(int enable)
{
	u8 value;
	int ret;

	if (enable) {
		/* enable access to BCIIREF1 */
		ret = twl4030_i2c_write_u8(TWL4030_CHIP_MAIN_CHARGE, 0xE7,
			REG_BCIMFKEY);
		if (ret)
			return ret;

		/* set charging current = 852mA */
		ret = twl4030_i2c_write_u8(TWL4030_CHIP_MAIN_CHARGE, 0xFF,
			REG_BCIIREF1);
		if (ret)
			return ret;

		/* forcing the field BCIAUTOUSB (BOOT_BCI[1]) to 1 */
		ret = clear_n_set(TWL4030_CHIP_PM_MASTER, 0,
			(CONFIG_DONE | BCIAUTOWEN | BCIAUTOUSB),
			REG_BOOT_BCI);
		if (ret)
			return ret;

		/* Enabling interfacing with usb thru OCP */
		ret = clear_n_set(TWL4030_CHIP_USB, 0, PHY_DPLL_CLK,
			REG_PHY_CLK_CTRL);
		if (ret)
			return ret;

		value = 0;

		while (!(value & PHY_DPLL_CLK)) {
			udelay(10);
			ret = twl4030_i2c_read_u8(TWL4030_CHIP_USB, &value,
				REG_PHY_CLK_CTRL_STS);
			if (ret)
				return ret;
		}

		/* OTG_EN (POWER_CTRL[5]) to 1 */
		ret = clear_n_set(TWL4030_CHIP_USB, 0, OTG_EN,
			REG_POWER_CTRL);
		if (ret)
			return ret;

		mdelay(50);

		/* forcing USBFASTMCHG(BCIMFSTS4[2]) to 1 */
		ret = clear_n_set(TWL4030_CHIP_MAIN_CHARGE, 0,
			USBFASTMCHG, REG_BCIMFSTS4);
		if (ret)
			return ret;
	} else {
		ret = clear_n_set(TWL4030_CHIP_PM_MASTER, BCIAUTOUSB,
			(CONFIG_DONE | BCIAUTOWEN), REG_BOOT_BCI);
		if (ret)
			return ret;
	}

	return 0;
}

/*
 * Setup the twl4030 MADC module to measure the backup
 * battery voltage.
 */
static int twl4030_madc_setup(void)
{
	int ret = 0;

	/* turning MADC clocks on */
	ret = clear_n_set(TWL4030_CHIP_INTBR, 0,
		(MADC_HFCLK_EN | DEFAULT_MADC_CLK_EN), REG_GPBR1);
	if (ret)
		return ret;

	/* turning adc_on */
	ret = twl4030_i2c_write_u8(TWL4030_CHIP_MADC, MADC_ON,
		REG_CTRL1);
	if (ret)
		return ret;

	/* setting MDC channel 9 to trigger by SW1 */
	ret = clear_n_set(TWL4030_CHIP_MADC, 0, SW1_CH9_SEL,
		REG_SW1SELECT_MSB);

	return ret;
}

/*
 * Charge backup battery through main battery
 */
static int twl4030_charge_backup_battery(void)
{
	int ret;

	ret = clear_n_set(TWL4030_CHIP_PM_RECIEVER, 0xff,
			  (BBCHEN | BBSEL_3200mV | BBISEL_150uA), REG_BB_CFG);
	if (ret)
		return ret;

	return 0;
}

/*
 * Helper function to read a 2-byte register on BCI module
 */
static int read_bci_val(u8 reg)
{
	int ret = 0, temp = 0;
	u8 val;

	/* reading MSB */
	ret = twl4030_i2c_read_u8(TWL4030_CHIP_MAIN_CHARGE, &val, reg + 1);
	if (ret)
		return ret;

	temp = ((int)(val & 0x03)) << 8;

	/* reading LSB */
	ret = twl4030_i2c_read_u8(TWL4030_CHIP_MAIN_CHARGE, &val, reg);
	if (ret)
		return ret;

	return temp + val;
}

/*
 * Triggers the sw1 request for the twl4030 module to measure the sw1 selected
 * channels
 */
static int twl4030_madc_sw1_trigger(void)
{
	u8 val;
	int ret;

	/* Triggering SW1 MADC convertion */
	ret = twl4030_i2c_read_u8(TWL4030_CHIP_MADC, &val, REG_CTRL_SW1);
	if (ret)
		return ret;

	val |= SW1_TRIGGER;

	ret = twl4030_i2c_write_u8(TWL4030_CHIP_MADC, val, REG_CTRL_SW1);
	if (ret)
		return ret;

	/* Waiting until the SW1 conversion ends*/
	val =  BUSY;

	while (!((val & EOC_SW1) && (!(val & BUSY)))) {
		ret = twl4030_i2c_read_u8(TWL4030_CHIP_MADC, &val,
					  REG_CTRL_SW1);
		if (ret)
			return ret;
		mdelay(10);
	}

	return 0;
}

/*
 * Return battery voltage
 */
static int twl4030_get_battery_voltage(void)
{
	int volt;

	volt = read_bci_val(T2_BATTERY_VOLT);
	return (volt * 588) / 100;
}

/*
 * Return the battery backup voltage
 */
static int twl4030_get_backup_battery_voltage(void)
{
	int ret, temp;
	u8 volt;

	/* trigger MADC convertion */
	twl4030_madc_sw1_trigger();

	ret = twl4030_i2c_read_u8(TWL4030_CHIP_MADC, &volt, REG_GPCH9 + 1);
	if (ret)
		return ret;

	temp = ((int) volt) << 2;

	ret = twl4030_i2c_read_u8(TWL4030_CHIP_MADC, &volt, REG_GPCH9);
	if (ret)
		return ret;

	temp = temp + ((int) ((volt & MADC_LSB_MASK) >> 6));

	return  (temp * 441) / 100;
}

#if 0 /* Maybe used in future */
/*
 * Return the AC power supply voltage
 */
static int twl4030_get_ac_charger_voltage(void)
{
	int volt = read_bci_val(T2_BATTERY_ACVOLT);
	return (volt * 735) / 100;
}

/*
 * Return the USB power supply voltage
 */
static int twl4030_get_usb_charger_voltage(void)
{
	int volt = read_bci_val(T2_BATTERY_USBVOLT);
	return (volt * 2058) / 300;
}
#endif

/*
 * Battery charging main function called from board-specific file
 */

int twl4030_init_battery_charging(void)
{
	u8 batstsmchg, batstspchg, hwsts;
	int battery_volt = 0, charger_present = 0;
	int ret = 0;
	

#ifdef CONFIG_3430ZOOM2
	/* For Zoom2 enable Main charge Automatic mode:
	 * by enabling MADC clocks
	 */

	/* Red LED - on  */
	omap3_zoom2_led_red_on();

	/* Enable AC charging */
	ret = clear_n_set(TWL4030_CHIP_INTBR, 0,
		(MADC_HFCLK_EN | DEFAULT_MADC_CLK_EN), REG_GPBR1);

	udelay(100);


	ret = twl4030_i2c_write_u8(TWL4030_CHIP_MAIN_CHARGE, 0xE7,
			REG_BCIMFKEY);
	/* set MAX charging current */
	ret = twl4030_i2c_write_u8(TWL4030_CHIP_MAIN_CHARGE, 0xFF,
			REG_BCIIREF1);

	/* Red LED - off  */
	omap3_zoom2_led_red_off();


	/* Done for Zoom2 */
	return 0;
#endif

	/* check for battery presence */
	ret = twl4030_i2c_read_u8(TWL4030_CHIP_MAIN_CHARGE, &batstsmchg,
				  REG_BCIMFSTS3);
	if (ret)
		return ret;

	ret = twl4030_i2c_read_u8(TWL4030_CHIP_PRECHARGE, &batstspchg,
				  REG_BCIMFSTS1);
	if (ret)
		return ret;

	if (!((batstspchg & BATSTSPCHG) || (batstsmchg & BATSTSMCHG)))
		return ret;	/* no battery */

	ret = twl4030_madc_setup();
	if (ret) {
		printf("twl4030 madc setup error %d\n", ret);
		return ret;
	}
	/* backup battery charges through main battery */
	ret = twl4030_charge_backup_battery();
	if (ret) {
		printf("backup battery charging error\n");
		return ret;
	}
	/* check for charger presence */
	ret = twl4030_i2c_read_u8(TWL4030_CHIP_PM_MASTER, &hwsts,
				  REG_STS_HW_CONDITIONS);
	if (ret)
		return ret;

	if (hwsts & STS_CHG) {
		printf("AC charger detected\n");
		ret = twl4030_ac_charger_enable(1);
		if (ret)
			return ret;
		charger_present = 1;
	} else {
		if (hwsts & STS_VBUS) {
			printf("USB charger detected\n");
			charger_present = 1;
		}
		/* usb charging is enabled regardless of the whether the
		 * charger is attached, otherwise the main battery voltage
		 * cannot be read
		 */
		ret = twl4030_usb_charger_enable(1);
		if (ret)
			return ret;
	}
	battery_volt = twl4030_get_battery_voltage();
	printf("Battery levels: main %d mV, backup %d mV\n",
		battery_volt, twl4030_get_backup_battery_voltage());
	if (!charger_present && (battery_volt < 3300)) {
		printf("Main battery charge too low!\n");
	}

	return ret;
}
