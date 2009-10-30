/*
 * Board specific setup info
 *
 * (C) Copyright 2009
 * Texas Instruments, <www.ti.com>
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

/* DPLL(1-4) PARAM TABLES */
/* Each of the tables has M, N, FREQSEL, M2 values defined for nominal
 * OPP (1.2V). The fields are defined according to dpll_param struct(clock.c).
 * The values are defined for all possible sysclk and for ES1 and ES2.
 */

/* WARNING : Only the 26MHz timing has been tested */

/* Vdd1 = 1.20 for mpu and iva default */

mpu_dpll_param:
/* 12MHz */
/* ES1 */
.word 60, 0, 7, 1
/* ES2 */
.word 60, 0, 7, 1
/* 3410 */
.word 60, 0, 7, 1

/* 13MHz */
/* ES1 */
.word 720, 12, 4, 1
/* ES2 */
.word 720, 12, 4, 1
/* 3410 */
.word 720, 12, 4, 1

/* 19.2MHz */
/* ES1 */
.word 75, 1, 5, 1
/* ES2 */
.word 75, 1, 5, 1
/* 3410 */
.word 75, 1, 5, 1

/* 26MHz */
/* ES1 */
.word 360, 12, 7, 1
/* ES2 */
.word 360, 12, 7, 1
/* 3410 */
.word 360, 12, 7, 1

/* 38.4MHz */
/* ES1 */
.word 75, 3, 7, 1
/* ES2 */
.word 75, 3, 7, 1
/* 3410 */
.word 75, 3, 7, 1

iva_dpll_param:
/* 12MHz */
/* ES1 */
.word 50, 0, 7, 1
/* ES2 */
.word 50, 0, 7, 1
/* 3410 */
.word 50, 0, 7, 1

/* 13MHz */
/* ES1 */
.word 600, 12, 3, 1
/* ES2 */
.word 600, 12, 3, 1
/* 3410 */
.word 600, 12, 3, 1

/* 19.2MHz */
/* ES1 */
.word 125, 3, 7, 1
/* ES2 */
.word 125, 3, 7, 1
/* 3410 */
.word 125, 3, 7, 1

/* 26MHz */
/* ES1 */
.word 300, 12, 7, 1
/* ES2 */
.word 300, 12, 7, 1
/* 3410 */
.word 300, 12, 7, 1

/* 38.4MHz */
/* ES1 */
.word 125, 7, 3, 1
/* ES2 */
.word 125, 7, 3, 1
/* 3410 */
.word 125, 7, 3, 1


#if defined(PRCM_CLK_CFG2_400MHZ)

core_dpll_param:
/* 12MHz */
/* ES1 */
.word 100, 2, 7, 1
/* ES2 */
.word 100, 2, 7, 1
/* 3410 */
.word 100, 2, 7, 1

/* 13MHz */
/* ES1 */
.word 400, 12, 3, 1
/* ES2 */
.word 400, 12, 3, 1
/* 3410 */
.word 400, 12, 3, 1

/* 19.2MHz */
/* ES1 */
.word 375, 17, 3, 1
/* ES2 */
.word 375, 17, 3, 1
/* 3410 */
.word 375, 17, 3, 1

/* 26MHz */
/* ES1 */
.word 200, 12, 7, 1
/* ES2 */
.word 200, 12, 7, 1
/* 3410 */
.word 200, 12, 7, 1

/* 38.4MHz */
/* ES1 */
.word 375, 35, 3, 1
/* ES2 */
.word 375, 35, 3, 1
/* 3410 */
.word 375, 35, 3, 1

#elif defined(PRCM_CLK_CFG2_332MHZ)

core_dpll_param:
/* 12MHz */
/* ES1 */
.word 0x0E, 0x03, 0x07, 0x1
/* ES2 */
.word 0xA6, 0x05, 0x07, 0x1
/* 3410 */
.word 0xA6, 0x05, 0x07, 0x1

/* 13MHz */
/* ES1 */
.word 0x1B2, 0x10, 0x3, 0x1
/* ES2 */
.word 0x14C, 0x0c, 0x3, 0x1
/* 3410 */
.word 0x14C, 0x0c, 0x3, 0x1

/* 19.2MHz */
/* ES1 */
.word 0x19F, 0x17, 0x03, 01
/* ES2 */
.word 0x19F, 0x17, 0x03, 01
/* 3410 */
.word 0x19F, 0x17, 0x03, 01

/* 26MHz */
/* ES1 */
/* this is different from TRM */
.word 0x1B2, 0x21, 0x3, 0x1
/* ES2 */
.word 0xA6, 0x0C, 0x7, 0x1
/* 3410 */
.word 0xA6, 0x0C, 0x7, 0x1

/* 38.4MHz */
/* ES1 */
.word 0x19F, 0x2F, 0x03, 0x01
/* ES2 */
.word 0x19F, 0x2F, 0x03, 0x01
/* 3410 */
.word 0x19F, 0x2F, 0x03, 0x01

#else
#error "Undefined memory speed"
#endif /* PRCM_CLK_CFG2_* */


per_dpll_param:
#if defined(CONFIG_PER_M2_192)
/*	sys(kHz), m,	n,	clkin,	sd,	dco,	m2,	m3,	m4,	m5,	m6,	m2div */
.word 12000,	720,	4,	0,	7,	4,	9,	32,	10,	8,	6,	1
.word 13000,	864,	0,	1,	7,	4,	9,	32,	10,	8,	6,	1
.word 19200,	720,	7,	0,	7,	4,	9,	32,	10,	8,	6,	1
.word 26000,	864,	12,	0,	7,	4,	9,	32,	10,	8,	6,	1
.word 38400,	720,	15,	0,	7,	4,	9,	32,	10,	8,	6,	1
.word 0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0
#elif defined(CONFIG_PER_SGX_192)
/*	sys(kHz), m,	n,	clkin,	sd,	dco,	m2,	m3,	m4,	m5,	m6,	m2div */
.word 12000,	720,	4,	0,	7,	4,	9,	32,	10,	8,	6,	2
.word 13000,	864,	0,	1,	7,	4,	9,	32,	10,	8,	6,	2
.word 19200,	720,	7,	0,	7,	4,	9,	32,	10,	8,	6,	2
.word 26000,	864,	12,	0,	7,	4,	9,	32,	10,	8,	6,	2
.word 38400,	720,	15,	0,	7,	4,	9,	32,	10,	8,	6,	2
.word 0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0	
#else /* Default to 96 MHz M2 */
/*	sys(kHz), m,	n,	clkin,	sd,	dco,	m2,	m3,	m4,	m5,	m6,	m2div */
.word 12000,	360,	4,	0,	4,	2,	9,	16,	5,	4,	3,	1
.word 13000,	432,	0,	1,	4,	2,	9,	16,	5,	4,	3,	1
.word 19200,	360,	7,	0,	4,	2,	9,	16,	5,	4,	3,	1
.word 26000,	432,	12,	0,	4,	2,	9,	16,	5,	4,	3,	1
.word 38400,	360,	15,	0,	4,	2,	9,	16,	5,	4,	3,	1
.word 0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0	
#endif
