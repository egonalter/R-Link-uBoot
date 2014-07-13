/*
 * (C) Copyright 2010
 * TomTom International B.V.
 * Martin Jackson <martin.jackson@tomtom.com>
 *
 *
 * Check whether we have previously tried all happy booting scenarios,
 * and power off if there are no more options (an 'epic fail')
 *
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <asm/arch/mux.h>
#include <config.h>

#ifdef EPICFAIL_POWEROFF

/* Unfortunately, we need this pin mapping _really_ early */
# ifdef __VARIANT_A1
# define  CP_PWR_HOLD			CONTROL_PADCONF_GPMC_A9		/* GPIO 42 */
/* GPIO 42 Bank related */
#  define CLKEN_PER_EN_PWR_HOLD_BIT	CLKEN_PER_EN_GPIO2_BIT		/* Bit to toggle bank clock */
#  define OMAP34XX_PWR_HOLD_BASE	OMAP34XX_GPIO2_BASE		/* Base of bank */
#  define PWR_HOLD_BANK_GPIO		10				/* Offset within bank */
# elif defined(__VARIANT_A2)
# define  CP_PWR_HOLD			CONTROL_PADCONF_GPMC_nBE1	/* GPIO 61 */
/* GPIO 61 Bank related */
#  define CLKEN_PER_EN_PWR_HOLD_BIT	CLKEN_PER_EN_GPIO2_BIT		/* Bit to toggle bank clock */
#  define OMAP34XX_PWR_HOLD_BASE	OMAP34XX_GPIO2_BASE		/* Base of bank */
#  define PWR_HOLD_BANK_GPIO		29				/* Offset within bank */
# else
#  error Unrecognized variant!
# endif

#endif /* EPICFAIL_POWEROFF */


#define TOUT_01S	0xffff7fff		/* Watchdog timeout of 1s  */
#define TOUT_10S	0xfffaffff		/* Watchdog timeout of 10s */
#define TOUT_60S	0xffe1ffff		/* Watchdog timeout of 60s */
#define WD_TIMEOUT	TOUT_60S


@ Return value in r0
.macro	readl		addr
	ldr	r0, =\addr
	ldr	r0, [r0]
.endm

@ Clobbers r0, r1
.macro	writel		val, addr
	ldr	r1, =\addr
	ldr	r0, =\val
	str	r0, [r1]
.endm

@ Clobbers r0, r1
.macro	writew		val, addr
	ldr	r1, =\addr
	ldr	r0, =\val
	strh	r0, [r1]
.endm

@ Clobbers r0, r1
.macro	__modify_bit	nr, addr, op
	ldr	r1, =\addr
	ldr	r0, [r1]
	\op	r0, r0, #1<<\nr
	str	r0, [r1]
.endm

@ Clobbers r0, r1
.macro	set_bit		nr, addr
	__modify_bit	\nr, \addr, orr
.endm

@ Clobbers r0, r1
.macro	clr_bit		nr, addr
	__modify_bit	\nr, \addr, bic
.endm

@ Sets CPSR status bits, clobbers r0
.macro	tst_bit		nr, addr
	readl	\addr
	ands	r0, r0, #1<<\nr
.endm

.macro	wdsync
	ldr	r0, =WD2_BASE+WWPS			@ Wait for watchdog reg writes to drain
1:	ldr	r1, [r0]
	cmp	r1, #0
	bne	1b
.endm

.macro	wd_wr	val, reg
	writel	\val, WD2_BASE + \reg
	wdsync
.endm


@ Check whether an epic fail condition occurred
.global check_epicfail

/*
 * We don't push any context in case the stack or bits of memory are buggered.
 * Clobbers:
 *  - r0: Scratch register
 *  - r1: Scratch register
 *  - r3: Saved CPSR state
 *  - r12: Saved LR
 */
check_epicfail:
#ifdef EPICFAIL_POWEROFF
	mov	r12, lr

	mrs	r3, cpsr				@ Disable all interrupts
	mov	r0, r3
	orr	r0, r0, #0xc0
	msr	cpsr, r0

	@ PWR_HOLD probably isn't asserted here anyway, but deassert it just in case.
	bl	deassert_pwrhold

	@ Arm the watchdog with timeout of 60s
	set_bit	5, CM_FCLKEN_WKUP			@ Enable watchdog FCLK
	set_bit	5, CM_ICLKEN_WKUP			@ Enable watchdog ICLK

	ldr	r0, =CM_IDLEST_WKUP			@ Wait for watchdog to become accessible
1:	ldr	r1, [r0]
	ands	r1, #0x20
	bne	1b

	bl	wd_disable				@ Changing the timeout fails if we don't do this
	
	wd_wr	WD_TIMEOUT, WLDR			@ Load timeout value
	wd_wr	0xaa55aa55, WTGR			@ Reload the watchdog timeout

	@ Write the magic sequence 0xbbbb, 0x4444 that starts the watchdog count
	wd_wr	0xbbbb, WSPR
	wd_wr	0x4444, WSPR

	tst_bit	4, PRM_RSTTST				@ Check whether it's a watchdog reset
	beq	no_epicfail

	@ Check the epic fail bit: did boot and altboot both fail already?
	tst_bit	EPICFAIL_BIT, FLIP_FLOP_LOCATION
	bne	epicfail				@ epic fail bit set + watchdog reboot == epic fail
	
no_epicfail:
	@ Set the epic fail bit: this has to be cleared later by userland
	@ (after the kernel boots) to prove no epic fail occurred.
	set_bit	EPICFAIL_BIT, FLIP_FLOP_LOCATION

	msr	cpsr, r3				@ Restore interrupts

#endif /* EPICFAIL_POWEROFF */
	mov	pc, r12

.ltorg


@ Epic fail occurred: power the system down. Does NOT return
.global epicfail
epicfail:
#ifdef EPICFAIL_POWEROFF
	clr_bit	EPICFAIL_BIT, FLIP_FLOP_LOCATION	@ System will attempt to boot next time SYSTEM_ON is asserted
	bl	deassert_pwrhold
	bl	wd_disable

	@ Could print a helpful message saying what happened. But don't want to
	@ risk crashing in the UART code :P
1:	b	1b					@ Hang here until power goes down
#else
	mov	pc, lr
#endif /* EPICFAIL_POWEROFF */

.ltorg


#ifdef EPICFAIL_POWEROFF
@ Disable the watchdog
wd_disable:
	@ Disable the watchdog with the magic 0xaaaa, 0x5555 sequence
	wd_wr	0xaaaa, WSPR
	wd_wr	0x5555, WSPR

.ltorg


@ Deassert the PWR_HOLD pin
deassert_pwrhold:
	writew	(IDIS | PTD | DIS | M4), \
		OMAP34XX_CTRL_BASE+(CP_PWR_HOLD)	@ Configure PWR_HOLD pin mux as GPIO
	set_bit	CLKEN_PER_EN_PWR_HOLD_BIT, \
		CM_FCLKEN_PER				@ Enable PWR_HOLD GPIO bank FCLK
	set_bit	CLKEN_PER_EN_PWR_HOLD_BIT, \
		CM_ICLKEN_PER				@ Enable PWR_HOLD GPIO bank ICLK
	clr_bit	PWR_HOLD_BANK_GPIO, OMAP34XX_PWR_HOLD_BASE + \
		OMAP34XX_GPIO_OE			@ Set PWR_HOLD pin to output
	clr_bit	PWR_HOLD_BANK_GPIO, OMAP34XX_PWR_HOLD_BASE + \
		OMAP34XX_GPIO_DATAOUT			@ Set PWR_HOLD pin low
	mov	pc, lr
.ltorg
#endif /* EPICFAIL_POWEROFF */


.end

