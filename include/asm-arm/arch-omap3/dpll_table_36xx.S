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

mpu_dpll_param:
/* 12MHz */
/* ES1 */
.word 0x0FE,0x07,0x05,0x01
/* ES2 */
.word 0x14A,0x05,0x07,0x01
/* 3410 */
.word 0x085,0x05,0x07,0x01

/* 13MHz */
/* ES1 */
.word 0x17D,0x0C,0x03,0x01
/* ES2 */
.word 0x226,0x0C,0x04,0x01
/* 3410 */
.word 0x10A,0x0C,0x03,0x01

/* 19.2MHz */
/* ES1 */
.word 0x179,0x12,0x04,0x01
/* ES2 */
.word 0x191,0x0D,0x05,0x01
/* 3410 */
.word 0x14C,0x17,0x03,0x01

/* 26MHz */
/* ES1 */
/*.word 0x17D,0x19,0x03,0x01*/
.word 0x12C,0x0C,0x07,0x01
/* ES2 */
.word 0x14A,0x0C,0x07,0x01
/* 3410 */
.word 0x085,0x0C,0x07,0x01

/* 38.4MHz */
/* ES1 */
.word 0x1FA,0x32,0x03,0x01
/* ES2 */
.word 0x110,0x12,0x07,0x01
/* 3410 */
.word 0x14C,0x2F,0x03,0x01

iva_dpll_param:
/* 12MHz */
/* ES1 */
.word 0x07D,0x05,0x07,0x01
/* ES2 */
.word 0x0B4,0x05,0x07,0x01
/* 3410 */
.word 0x085,0x05,0x07,0x01

/* 13MHz */
/* ES1 */
.word 0x0FA,0x0C,0x03,0x01
/* ES2 */
.word 0x168,0x0C,0x03,0x01
/* 3410 */
.word 0x10A,0x0C,0x03,0x01

/* 19.2MHz */
/* ES1 */
.word 0x082,0x09,0x07,0x01
/* ES2 */
.word 0x0E1,0x0B,0x06,0x01
/* 3410 */
.word 0x14C,0x17,0x03,0x01

/* 26MHz */
/* ES1 */
/*.word 0x07D,0x0C,0x07,0x01*/
.word  0x104,0x0C,0x7,0x1
/* ES2 */
.word 0x0B4,0x0C,0x07,0x01
/* 3410 */
.word 0x085,0x0C,0x07,0x01

/* 38.4MHz */
/* ES1 */
.word 0x13F,0x30,0x03,0x01
/* ES2 */
.word 0x0E1,0x17,0x06,0x01
/* 3410 */
.word 0x14C,0x2F,0x03,0x01

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

/* PER DPLL values are same for both ES1 and ES2 */
per_dpll_param:
/* 12MHz */
.word 0xD8,0x05,0x07,0x09
/* 13MHz */
.word 0x1B0,0x0C,0x03,0x09
/* 19.2MHz */
.word 0xE1,0x09,0x07,0x09
/* 26MHz */
.word 0x1B0,0x0C,0x07,0x09
/* 38.4MHz */
.word 0xE1,0x13,0x07,0x09


