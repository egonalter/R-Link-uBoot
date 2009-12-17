/*
 * (C) Copyright 2008
 * Texas Instruments, <www.ti.com>
 * Syed Mohammed Khasim <khasim@ti.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation's version 2 of
 * the License.
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
#include <config.h>
#include <common.h>
#include <mmc.h>
#include <part.h>

#if CONFIG_DRIVER_OMAP34XX_I2C /* don't compile for x-loader */
#include <i2c.h>
#endif

#ifdef DEBUG
#define MMC_DPRINT(args...)	printf(args)
#else
#define MMC_DPRINT(args...)
#endif

#ifdef CONFIG_MMC
#include "mmc_host_def.h"
#include "mmc_protocol.h"

#define OMAP_MMC_MASTER_CLOCK   96000000

mmc_card_data cur_card_data[2];
static block_dev_desc_t mmc_blk_dev[2];

block_dev_desc_t *mmc_get_dev(int dev)
{

	if ((dev == 0) || (dev == 1)) {
		if (cur_card_data[dev].size == 0)
			printf("Card is not Initalized (e.g. mmc init 0)\n");
		else
			return &mmc_blk_dev[dev];
	}
	return NULL;
}

#if CONFIG_DRIVER_OMAP34XX_I2C /* don't compile for x-loader */
static void twl4030_mmc_config(unsigned int slot)
{
	unsigned char data;

	/* configure the LDO */
	if (slot == 0) {
		data = 0x20;
		i2c_write(0x4B, 0x82, 1, &data, 1);
		data = 0x2;
		i2c_write(0x4B, 0x85, 1, &data, 1);
	} else {
		data = 0x20;
		i2c_write(0x4B, 0x86, 1, &data, 1);
		data = 0xB;
		i2c_write(0x4B, 0x89, 1, &data, 1);
	}
	return;
}
#endif

static int mmc_board_init(unsigned int slot)
{
	unsigned int value = 0;

	MMC_DPRINT("+mmc_board_init : slot=%d\n", slot);

#if CONFIG_DRIVER_OMAP34XX_I2C /* don't compile for x-loader */
	twl4030_mmc_config(slot);
#endif

	if (slot == 0) {
		value = CONTROL_PBIAS_LITE;
		CONTROL_PBIAS_LITE = value | (1 << 2) | (1 << 1) | (1 << 9);

		value = CONTROL_DEV_CONF0;
		CONTROL_DEV_CONF0 = value | (1 << 24);

	} else if (slot == 1) {
		value = CONTROL_DEV_CONF1;
		CONTROL_DEV_CONF1 = value | (1 << 6);
		value = (*(volatile unsigned int *)CM_FCLKEN1_CORE);
		(*(volatile unsigned int *)CM_FCLKEN1_CORE) = value | (1 << 25);
		value = (*(volatile unsigned int *)CM_ICLKEN1_CORE);
		(*(volatile unsigned int *)CM_ICLKEN1_CORE) = value | (1 << 25);
	}
	return 0;
}

static void mmc_init_stream(unsigned int base)
{
	MMC_DPRINT("+mmc_init_stream\n");
	OMAP_HSMMC_CON(base) |= INIT_INITSTREAM;

	OMAP_HSMMC_CMD(base) = MMC_CMD0;
	while (!(OMAP_HSMMC_STAT(base) & CC_MASK))
		;

	OMAP_HSMMC_STAT(base) = CC_MASK;

	OMAP_HSMMC_CMD(base) = MMC_CMD0;
	while (!(OMAP_HSMMC_STAT(base) & CC_MASK))
		;

	OMAP_HSMMC_STAT(base) = OMAP_HSMMC_STAT(base);
	OMAP_HSMMC_CON(base) &= ~INIT_INITSTREAM;
}

static int mmc_clock_config(unsigned int base, unsigned int iclk,
							unsigned short clk_div)
{
	unsigned int val;

	MMC_DPRINT("+mmc_clock_config : base=0x%x\n", base);
	mmc_reg_out(OMAP_HSMMC_SYSCTL(base), (ICE_MASK | DTO_MASK | CEN_MASK),
		    (ICE_STOP | DTO_15THDTO | CEN_DISABLE));

	switch (iclk) {
	case CLK_INITSEQ:
		val = MMC_INIT_SEQ_CLK / 2;
		break;
	case CLK_400KHZ:
		val = MMC_400kHz_CLK;
		break;
	case CLK_MISC:
		val = clk_div;
		break;
	default:
		return 1;
	}
	mmc_reg_out(OMAP_HSMMC_SYSCTL(base),
		    ICE_MASK | CLKD_MASK, (val << CLKD_OFFSET) | ICE_OSCILLATE);

	while ((OMAP_HSMMC_SYSCTL(base) & ICS_MASK) == ICS_NOTREADY)
		;


	OMAP_HSMMC_SYSCTL(base) |= CEN_ENABLE;
	return 0;
}

static int mmc_init_setup(mmc_card_data *mmc_c)
{
	unsigned int reg_val;
	unsigned int base = mmc_c->base;

	MMC_DPRINT("+mmc_init_setup\n");
	mmc_board_init(mmc_c->slot);

	OMAP_HSMMC_SYSCONFIG(base) |= MMC_SOFTRESET;
	while ((OMAP_HSMMC_SYSSTATUS(base) & RESETDONE) == 0)
		;

	OMAP_HSMMC_SYSCTL(base) |= SOFTRESETALL;
	while ((OMAP_HSMMC_SYSCTL(base) & SOFTRESETALL) != 0x0)
		;

	OMAP_HSMMC_HCTL(base) = DTW_1_BITMODE | SDBP_PWROFF | SDVS_3V0;
	OMAP_HSMMC_CAPA(base) |= VS30_3V0SUP | VS18_1V8SUP;

	reg_val = OMAP_HSMMC_CON(base) & RESERVED_MASK;

	OMAP_HSMMC_CON(base) = CTPL_MMC_SD | reg_val | WPP_ACTIVEHIGH |
	    CDP_ACTIVEHIGH | MIT_CTO | DW8_1_4BITMODE | MODE_FUNC |
	    STR_BLOCK | HR_NOHOSTRESP | INIT_NOINIT | NOOPENDRAIN;

	mmc_clock_config(base, CLK_INITSEQ, 0);
	OMAP_HSMMC_HCTL(base) |= SDBP_PWRON;

	OMAP_HSMMC_IE(base) = OMAP_HSMMC_STATUS_REQ;

	mmc_init_stream(base);
	return 0;
}

static int mmc_send_cmd(unsigned int base, unsigned int cmd,
				unsigned int arg, unsigned int *response)
{
	unsigned int mmc_stat;

	MMC_DPRINT("+mmc_send_cmd cmd=0x%x arg=0x%x\n", cmd, arg);
	while ((OMAP_HSMMC_PSTATE(base) & DATI_MASK) == DATI_CMDDIS)
		;

	OMAP_HSMMC_BLK(base) = BLEN_512BYTESLEN | NBLK_STPCNT;
	OMAP_HSMMC_STAT(base) = 0xFFFFFFFF;
	OMAP_HSMMC_ARG(base) = arg;
	OMAP_HSMMC_CMD(base) = cmd | CMD_TYPE_NORMAL | CICE_NOCHECK |
	    CCCE_NOCHECK | MSBS_SGLEBLK | ACEN_DISABLE | BCE_DISABLE |
	    DE_DISABLE;

	while (1) {
		do {
			mmc_stat = OMAP_HSMMC_STAT(base);
		} while (mmc_stat == 0);

		if ((mmc_stat & ERRI_MASK) != 0) {
			MMC_DPRINT("+mmc_send_cmd err=0x%x\n", mmc_stat);
			return (unsigned int)mmc_stat;
		}

		if (mmc_stat & CC_MASK) {
			OMAP_HSMMC_STAT(base) = CC_MASK;
			response[0] = OMAP_HSMMC_RSP10(base);
			if ((cmd & RSP_TYPE_MASK) == RSP_TYPE_LGHT136) {
				response[1] = OMAP_HSMMC_RSP32(base);
				response[2] = OMAP_HSMMC_RSP54(base);
				response[3] = OMAP_HSMMC_RSP76(base);
			}
			break;
		}
	}
	return 0;
}

static int mmc_read_data(unsigned int base, unsigned int *output_buf)
{
	unsigned int mmc_stat;
	unsigned int read_count = 0;

	/*
	 * Start Polled Read
	 */
	while (1) {
		do {
			mmc_stat = OMAP_HSMMC_STAT(base);
		} while (mmc_stat == 0);

		if ((mmc_stat & ERRI_MASK) != 0)
			return mmc_stat;

		if (mmc_stat & BRR_MASK) {
			unsigned int k;

			OMAP_HSMMC_STAT(base) |= BRR_MASK;
			for (k = 0; k < MMCSD_SECTOR_SIZE / 4; k++) {
				*output_buf = OMAP_HSMMC_DATA(base);
				output_buf++;
				read_count += 4;
			}
		}

		if (mmc_stat & BWR_MASK)
			OMAP_HSMMC_STAT(base) |= BWR_MASK;

		if (mmc_stat & TC_MASK) {
			OMAP_HSMMC_STAT(base) |= TC_MASK;
			break;
		}
	}
	return 0;
}

#if CONFIG_DRIVER_OMAP34XX_I2C /* don't compile for x-loader */
unsigned int mmc_write_data(unsigned int base, unsigned int *output_buf)
{
	unsigned int mmc_stat;
	unsigned int write_count = 0;

	/* Start Polled write */
	while (1) {
		MMC_DPRINT("+mmc_write_data\n");
		do {
			mmc_stat = OMAP_HSMMC_STAT(base);
		} while (mmc_stat == 0);

		if ((mmc_stat & ERRI_MASK) != 0)
			return mmc_stat;

		if (mmc_stat & BWR_MASK) {
			unsigned int k;

			OMAP_HSMMC_STAT(base) |= BRR_MASK;
			for (k = 0; k < MMCSD_SECTOR_SIZE / 4; k++) {
				OMAP_HSMMC_DATA(base) = *output_buf;
				output_buf++;
				write_count += 4;
			}
		}

		if (mmc_stat & BWR_MASK)
			OMAP_HSMMC_STAT(base) |= BWR_MASK;

		if (mmc_stat & TC_MASK) {
			OMAP_HSMMC_STAT(base) |= TC_MASK;
			break;
		}
	}
	MMC_DPRINT("-mmc_write_data(%d)\n", write_count);
	return 0;
}
#endif /* CFG_CMD_MMC - not to used x-loader build */

static int mmc_detect_card(mmc_card_data *mmc_card_cur)
{
	unsigned int err;
	unsigned int argument = 0;
	unsigned int ocr_value, ocr_recvd, ret_cmd41, hcs_val;
	unsigned int resp[4];
	unsigned short retry_cnt = 2000;

	MMC_DPRINT("+mmc_detect_card : slot=%d\n", mmc_card_cur->slot);
	/* Set to Initialization Clock */
	err = mmc_clock_config(mmc_card_cur->base, CLK_400KHZ, 0);
	if (err)
		return err;

	mmc_card_cur->RCA = MMC_RELATIVE_CARD_ADDRESS;
	argument = 0x00000000;

	if (mmc_card_cur->slot == 0)
		ocr_value = (0x1FF << 15);
	else
		ocr_value = (0x80);

	err = mmc_send_cmd(mmc_card_cur->base, MMC_CMD0, argument, resp);
	if (err)
		return err;

	argument = SD_CMD8_CHECK_PATTERN | SD_CMD8_2_7_3_6_V_RANGE;
	err = mmc_send_cmd(mmc_card_cur->base, MMC_SDCMD8, argument, resp);
	hcs_val = (err == 0) ?
	    MMC_OCR_REG_HOST_CAPACITY_SUPPORT_SECTOR :
	    MMC_OCR_REG_HOST_CAPACITY_SUPPORT_BYTE;

	argument = 0x0000 << 16;
	err = mmc_send_cmd(mmc_card_cur->base, MMC_CMD55, argument, resp);
	if (err == 0) {
		mmc_card_cur->card_type = SD_CARD;
		ocr_value |= hcs_val;
		ret_cmd41 = MMC_ACMD41;
		MMC_DPRINT("Card found : SD_CARD\n");
	} else {
		mmc_card_cur->card_type = MMC_CARD;
		ocr_value |= MMC_OCR_REG_ACCESS_MODE_SECTOR;
		ret_cmd41 = MMC_CMD1;
		OMAP_HSMMC_CON(mmc_card_cur->base) &= ~OD;
		OMAP_HSMMC_CON(mmc_card_cur->base) |= OPENDRAIN;
		MMC_DPRINT("Card found : MMC_CARD\n");
	}

	argument = ocr_value;
	err = mmc_send_cmd(mmc_card_cur->base, ret_cmd41, argument, resp);
	if (err)
		return err;

	ocr_recvd = ((mmc_resp_r3 *) resp)->ocr;

	while (!(ocr_recvd & (0x1 << 31)) && (retry_cnt > 0)) {
		retry_cnt--;
		if (mmc_card_cur->card_type == SD_CARD) {
			argument = 0x0000 << 16;
			err = mmc_send_cmd(mmc_card_cur->base,
						MMC_CMD55, argument, resp);
		}

		argument = ocr_value;
		err = mmc_send_cmd(mmc_card_cur->base,
						ret_cmd41, argument, resp);
		if (err)
			return err;
		ocr_recvd = ((mmc_resp_r3 *) resp)->ocr;
	}

	/* check if busy */
	if (!(ocr_recvd & (0x1 << 31)))
		return 1;

	if (mmc_card_cur->card_type == MMC_CARD) {
		if ((ocr_recvd & MMC_OCR_REG_ACCESS_MODE_MASK) ==
		    MMC_OCR_REG_ACCESS_MODE_SECTOR) {
			mmc_card_cur->mode = SECTOR_MODE;
		} else {
			mmc_card_cur->mode = BYTE_MODE;
		}

		ocr_recvd &= ~MMC_OCR_REG_ACCESS_MODE_MASK;
	} else {
		if ((ocr_recvd & MMC_OCR_REG_HOST_CAPACITY_SUPPORT_MASK)
		    == MMC_OCR_REG_HOST_CAPACITY_SUPPORT_SECTOR) {
			mmc_card_cur->mode = SECTOR_MODE;
		} else {
			mmc_card_cur->mode = BYTE_MODE;
		}
		ocr_recvd &= ~MMC_OCR_REG_HOST_CAPACITY_SUPPORT_MASK;
	}

	ocr_recvd &= ~(0x1 << 31);
	if (!(ocr_recvd & ocr_value))
		return 1;

	err = mmc_send_cmd(mmc_card_cur->base, MMC_CMD2, argument, resp);
	if (err)
		return err;

	if (mmc_card_cur->card_type == MMC_CARD) {
		argument = mmc_card_cur->RCA << 16;
		err = mmc_send_cmd(mmc_card_cur->base,
						MMC_CMD3, argument, resp);
		if (err)
			return err;
	} else {
		argument = 0x00000000;
		err = mmc_send_cmd(mmc_card_cur->base,
						MMC_SDCMD3, argument, resp);
		if (err)
			return err;

		mmc_card_cur->RCA = ((mmc_resp_r6 *) resp)->newpublishedrca;
	}

	OMAP_HSMMC_CON(mmc_card_cur->base) &= ~OD;
	OMAP_HSMMC_CON(mmc_card_cur->base) |= NOOPENDRAIN;
	MMC_DPRINT("-mmc_detect_card\n");
	return 0;
}

static int mmc_read_cardsize(mmc_card_data *mmc_dev_data,
				mmc_csd_reg_t *cur_csd)
{
	mmc_extended_csd_reg_t ext_csd;
	unsigned int size, count, blk_len, blk_no, card_size, argument;
	int err;
	unsigned int resp[4];
	unsigned int base = mmc_dev_data->base;

	mmc_dev_data->size = 0;
	if (mmc_dev_data->mode == SECTOR_MODE) {
		if (mmc_dev_data->card_type == SD_CARD) {
			card_size =
			    (((mmc_sd2_csd_reg_t *) cur_csd)->
			     c_size_lsb & MMC_SD2_CSD_C_SIZE_LSB_MASK) |
			    ((((mmc_sd2_csd_reg_t *) cur_csd)->
			      c_size_msb & MMC_SD2_CSD_C_SIZE_MSB_MASK)
			     << MMC_SD2_CSD_C_SIZE_MSB_OFFSET);
			mmc_dev_data->size = card_size * 1024;
			if (mmc_dev_data->size == 0)
				return 1;
		} else {
			argument = 0x00000000;
			err = mmc_send_cmd(base, MMC_CMD8, argument, resp);
			if (err)
				return err;
			err = mmc_read_data(base, (unsigned int *)&ext_csd);
			if (err)
				return err;
			mmc_dev_data->size = ext_csd.sectorcount;

			if (mmc_dev_data->size == 0)
				mmc_dev_data->size = 8388608;
		}
		MMC_DPRINT("Card Size=%d Sectors\n", mmc_dev_data->size);
	} else {
		if (cur_csd->c_size_mult >= 8)
			return 1;

		if (cur_csd->read_bl_len >= 12)
			return 1;

		/* Compute size */
		count = 1 << (cur_csd->c_size_mult + 2);
		card_size = (cur_csd->c_size_lsb & MMC_CSD_C_SIZE_LSB_MASK) |
		    ((cur_csd->c_size_msb & MMC_CSD_C_SIZE_MSB_MASK)
		     << MMC_CSD_C_SIZE_MSB_OFFSET);
		blk_no = (card_size + 1) * count;
		blk_len = 1 << cur_csd->read_bl_len;
		size = blk_no * blk_len;
		mmc_dev_data->size = size / MMCSD_SECTOR_SIZE;
		if (mmc_dev_data->size == 0)
			return 1;

		MMC_DPRINT("Card Size=%d Bytes\n", mmc_dev_data->size);
	}
	return 0;
}

int mmc_read_block(int dev_num, unsigned int blknr,
			unsigned int blkcnt, unsigned char *dst)
{
	unsigned int start_sec = blknr;
	unsigned int num_bytes = (blkcnt * MMCSD_SECTOR_SIZE);
	mmc_card_data *mmc_c = &cur_card_data[dev_num];
	unsigned int *output_buf = (unsigned int *)dst;

	int err;
	unsigned int argument;
	unsigned int resp[4];
	unsigned int num_sec_val =
	    (num_bytes + (MMCSD_SECTOR_SIZE - 1)) / MMCSD_SECTOR_SIZE;
	unsigned int sec_inc_val;

	MMC_DPRINT("mmc_read_block Pos=0x%x 0x%x-bytes Add=0x%x\n",
					start_sec, num_bytes, output_buf);
	if (num_sec_val == 0)
		return 1;

	if (mmc_c->mode == SECTOR_MODE) {
		argument = start_sec;
		sec_inc_val = 1;
		MMC_DPRINT("SECTOR_MODE\n");
	} else {
		argument = start_sec * MMCSD_SECTOR_SIZE;
		sec_inc_val = MMCSD_SECTOR_SIZE;
		MMC_DPRINT("BYTE_MODE\n");
	}

	while (num_sec_val) {
		err = mmc_send_cmd(mmc_c->base, MMC_CMD17, argument, resp);
		if (err)
			break;

		err = mmc_read_data(mmc_c->base, output_buf);
		if (err)
			break;

		output_buf += (MMCSD_SECTOR_SIZE / 4);
		argument += sec_inc_val;
		num_sec_val--;
	}
	/* FAT file system expects 1 as success */
	if (err == 1)
		return 0;
	else
		return 1;
}

unsigned char configure_mmc(mmc_card_data *mmc_card_cur)
{
	int ret_val;
	unsigned int argument;
	unsigned int resp[4];
	unsigned int trans_fact, trans_unit, retries = 2;
	unsigned int max_dtr;
	int dsor;
	mmc_csd_reg_t Card_CSD;
	unsigned char trans_speed;

	MMC_DPRINT("+configure_mmc\n");
	ret_val = mmc_init_setup(mmc_card_cur);
	if (ret_val)
		return ret_val;


	do {
		ret_val = mmc_detect_card(mmc_card_cur);
		retries--;
	} while ((retries > 0) && (ret_val));

	argument = mmc_card_cur->RCA << 16;
	ret_val = mmc_send_cmd(mmc_card_cur->base, MMC_CMD9, argument, resp);
	if (ret_val)
		return ret_val;

	((unsigned int *)&Card_CSD)[3] = resp[3];
	((unsigned int *)&Card_CSD)[2] = resp[2];
	((unsigned int *)&Card_CSD)[1] = resp[1];
	((unsigned int *)&Card_CSD)[0] = resp[0];

	if (mmc_card_cur->card_type == MMC_CARD)
		mmc_card_cur->version = Card_CSD.spec_vers;

	trans_speed = Card_CSD.tran_speed;
	ret_val = mmc_send_cmd(mmc_card_cur->base,
					MMC_CMD4, MMC_DSR_DEFAULT << 16, resp);
	if (ret_val)
		return ret_val;

	trans_unit = trans_speed & MMC_CSD_TRAN_SPEED_UNIT_MASK;
	trans_fact = trans_speed & MMC_CSD_TRAN_SPEED_FACTOR_MASK;

	if (trans_unit > MMC_CSD_TRAN_SPEED_UNIT_100MHZ)
		return 0;

	if ((trans_fact < MMC_CSD_TRAN_SPEED_FACTOR_1_0) ||
	    (trans_fact > MMC_CSD_TRAN_SPEED_FACTOR_8_0))
		return 0;

	trans_unit >>= 0;
	trans_fact >>= 3;

	max_dtr = tran_exp[trans_unit] * tran_mant[trans_fact];
	dsor = OMAP_MMC_MASTER_CLOCK / max_dtr;

/* Following lines commented to build in x-loader; otherwise its including
 * division library and creating a linking error.
	if (OMAP_MMC_MASTER_CLOCK / dsor > max_dtr)
		dsor++;
*/
	if (dsor == 4)
		dsor = 5;
	else if (dsor == 3)
		dsor = 4;
	else
		return 1;
	
	ret_val = mmc_clock_config(mmc_card_cur->base, CLK_MISC, dsor);
	if (ret_val)
		return ret_val;

	argument = mmc_card_cur->RCA << 16;
	ret_val = mmc_send_cmd(mmc_card_cur->base,
					MMC_CMD7_SELECT, argument, resp);
	if (ret_val)
		return ret_val;

	/* Configure the block length to 512 bytes */
	argument = MMCSD_SECTOR_SIZE;
	ret_val = mmc_send_cmd(mmc_card_cur->base, MMC_CMD16, argument, resp);
	if (ret_val)
		return ret_val;

	/* get the card size in sectors */
	ret_val = mmc_read_cardsize(mmc_card_cur, &Card_CSD);

	MMC_DPRINT("-configure_mmc(%d)\n", ret_val);
		return ret_val;
}

#if CONFIG_DRIVER_OMAP34XX_I2C /* don't compile for x-loader */
int mmc_write_block(int dev_num, unsigned int blknr, unsigned int blkcnt,
			unsigned char *src, unsigned int *total)
{
	unsigned int start_sec = blknr;
	unsigned int num_bytes = (blkcnt * MMCSD_SECTOR_SIZE);
	mmc_card_data *mmc_c = &cur_card_data[dev_num];
	unsigned int *output_buf = (unsigned int *)src;

	int err;
	unsigned int argument;
	unsigned int resp[4];
	unsigned int sec_inc_val;
	unsigned int num_sec_val = (num_bytes + (MMCSD_SECTOR_SIZE - 1)) /
							MMCSD_SECTOR_SIZE;

	MMC_DPRINT("+mmc_write_block(0x%x sector)\n", num_sec_val);
	if (blkcnt == 0)
		return 1;

	if (mmc_c->mode == SECTOR_MODE) {
		argument = start_sec;
		sec_inc_val = 1;
	} else {
		argument = start_sec * MMCSD_SECTOR_SIZE;
		sec_inc_val = MMCSD_SECTOR_SIZE;
	}

	while (num_sec_val) {
		err = mmc_send_cmd(mmc_c->base, MMC_CMD24, argument, resp);
		if (err)
			break;

		err = mmc_write_data(mmc_c->base, output_buf);
		if (err)
			break;

		output_buf += (MMCSD_SECTOR_SIZE / 4);
		argument += sec_inc_val;
		num_sec_val--;
	}
	*total = blkcnt - num_sec_val;

	MMC_DPRINT("-mmc_write_block\n");
	/* return 1 as success */
	if (err == 0)
		return 1;
	else
		return err;
}
#endif /*CFG_CMD_MMC - not to used x-loader build */

int mmc_init(int slot)
{
	int ret = 0;

	MMC_DPRINT("mmc_init - %d\n", slot);
	switch (slot) {
	case 0:
		cur_card_data[0].slot = 0;
		cur_card_data[0].base = OMAP_HSMMC1_BASE;
		break;
	case 1:
		cur_card_data[1].slot = 1;
		cur_card_data[1].base = OMAP_HSMMC2_BASE;
		break;
	default:
		ret = -1;
	}

	if (ret == 0)
		ret = configure_mmc(&cur_card_data[slot]);

	if (ret == 0) {
		/* update mmc_blk_dev info */
		mmc_blk_dev[slot].if_type =
				(slot == 0) ? IF_TYPE_MMC : IF_TYPE_MMC2;

		mmc_blk_dev[slot].part_type = PART_TYPE_DOS;
		mmc_blk_dev[slot].dev = 0;
		mmc_blk_dev[slot].lun = 0;
		mmc_blk_dev[slot].type = 0;

		/* FIXME fill in the correct size (is set to 32MByte) */
		mmc_blk_dev[slot].blksz = MMCSD_SECTOR_SIZE;
		mmc_blk_dev[slot].lba = 0x10000;
		mmc_blk_dev[slot].removable = 0;
		mmc_blk_dev[slot].block_read = (void *)mmc_read_block;

	} else
		MMC_DPRINT("MMC#%d Initalization FAILED\n", slot);

	/* return */
	return ret;
}

#if CONFIG_DRIVER_OMAP34XX_I2C /* don't compile for x-loader */
/* for partial read or used to read in byte mode */
int mmc_read_opts(int dev_num, unsigned int bytepos,
				unsigned int bytecnt, unsigned char *dst)
{
	int err = 1;
	unsigned int i, startsec, pos, cursize = 0, blkcnt;
	unsigned char buf[MMCSD_SECTOR_SIZE];

	MMC_DPRINT("+mmc_read_opts dev=%d\n", dev_num);

	/* check, if length is not larger than device */
	MMC_DPRINT("mmc_read_opts Pos=0x%x 0x%x-bytes Add=0x%x\n",
					bytepos, bytecnt, dst);
	if (bytecnt == 0)
		return 1;

	/* check if bytepos is Sector bounday */
	startsec = bytepos/MMCSD_SECTOR_SIZE;
	if (startsec * MMCSD_SECTOR_SIZE < bytepos) {
		pos = bytepos - (startsec*MMCSD_SECTOR_SIZE);
		err = mmc_read_block(dev_num, startsec, 1, buf);
		/* 1 is success */
		if (err == 0x01) {
			/* check how much to read from this sector */
			if ((MMCSD_SECTOR_SIZE-pos) > bytecnt)
				cursize = bytecnt;
			else
				cursize = (MMCSD_SECTOR_SIZE-pos);

			for (i = pos; i < (cursize+pos); i++)
				*dst++ = buf[i];
			bytecnt = bytecnt - cursize;
			startsec++;
		}
	}

	if (bytecnt >= MMCSD_SECTOR_SIZE) {
		err = mmc_read_block(dev_num,
				startsec, bytecnt/MMCSD_SECTOR_SIZE, dst);

		/* 1 is success */
		if (err == 0x01) {
			blkcnt = bytecnt/MMCSD_SECTOR_SIZE;
			startsec = startsec + blkcnt;
			dst = dst + (blkcnt * MMCSD_SECTOR_SIZE);
			bytecnt = bytecnt - (blkcnt * MMCSD_SECTOR_SIZE);
		}
	}

	/* check if any fraction to read */
	if ((err == 0x01) && (bytecnt > 0)) {
		err = mmc_read_block(dev_num, startsec, 1, buf);
		if (err == 0x01) {
			for (i = 0; i < bytecnt; i++)
				*dst++ = buf[i];
		}
	}

	/* return 1 as success */
	return err;
}

/* for partial Write or used to write in byte mode */
int mmc_write_opts(int dev_num, unsigned int bytepos, unsigned int bytecnt,
				unsigned char *src, unsigned int *total)
{
	int err = 1;
	unsigned int i, startsec, pos, cursize = 0, blkcnt, tmptotal;
	unsigned char buf[MMCSD_SECTOR_SIZE];

	MMC_DPRINT("+mmc_read_opts dev=%d\n", dev_num);

	/* check, if length is not larger than device */
	MMC_DPRINT("mmc_read_opts Pos=0x%x 0x%x-bytes Add=0x%x\n",
					bytepos, bytecnt, src);
	if (bytecnt == 0)
		return 1;

	/* check if bytepos is Sector bounday */
	*total = bytecnt;
	startsec = bytepos/MMCSD_SECTOR_SIZE;
	if (startsec*MMCSD_SECTOR_SIZE < bytepos) {
		pos = bytepos - (startsec*MMCSD_SECTOR_SIZE);
		err = mmc_read_block(dev_num, startsec, 1, buf);
		/* 1 is success */
		if (err == 0x01) {
			/* check how much to read from this sector */
			if ((MMCSD_SECTOR_SIZE-pos) > bytecnt)
				cursize = bytecnt;
			else
				cursize = (MMCSD_SECTOR_SIZE-pos);

			/* make the buffer */
			for (i = pos; i < (cursize+pos); i++)
				buf[i] = *src++;
			/* write the buffer */
			err = mmc_write_block(dev_num,
						startsec, 1, buf, &tmptotal);

			bytecnt = bytecnt - cursize;
			startsec++;
		}
	}

	if (bytecnt >= MMCSD_SECTOR_SIZE) {
		err = mmc_write_block(dev_num,
			startsec, bytecnt/MMCSD_SECTOR_SIZE, src, &tmptotal);

		/* 1 is success */
		if (err == 0x01) {
			blkcnt = bytecnt/MMCSD_SECTOR_SIZE;
			startsec = startsec + blkcnt;
			src = src + (blkcnt * MMCSD_SECTOR_SIZE);
			bytecnt = bytecnt - (blkcnt * MMCSD_SECTOR_SIZE);
		}
	}

	/* check if any fraction to read */
	if ((err == 0x01) && (bytecnt > 0)) {
		err = mmc_read_block(dev_num, startsec, 1, src);
		if (err == 0x01) {
			/* make the buffer */
			for (i = 0; i < bytecnt; i++)
				buf[i] = *src++;
			/* write the buffer */
			err = mmc_write_block(dev_num,
						startsec, 1, buf, &tmptotal);
		}
	}

	/* return 1 as success */
	if (err != 1)
		*total = 0;

	return err;
}
#endif /* CFG_CMD_MMC - not to used x-loader build */

int mmc_support_rawboot_check(int dev)
{
	unsigned char buff[MMCSD_SECTOR_SIZE];

	/* read the 1st sector */
	if (mmc_read_block(dev, 0x00, 0x01, buff) != 1)
		return 1;

	/* search for "CHSETTINGS" @ 0x14*/
	if ((buff[TOC_NAME_OFFSET] != 'C') ||
		(buff[TOC_NAME_OFFSET+1] != 'H') ||
		(buff[TOC_NAME_OFFSET+2] != 'S') ||
		(buff[TOC_NAME_OFFSET+3] != 'E') ||
		(buff[TOC_NAME_OFFSET+4] != 'T') ||
		(buff[TOC_NAME_OFFSET+5] != 'T') ||
		(buff[TOC_NAME_OFFSET+6] != 'I') ||
		(buff[TOC_NAME_OFFSET+7] != 'N') ||
		(buff[TOC_NAME_OFFSET+8] != 'G') ||
		(buff[TOC_NAME_OFFSET+9] != 'S'))
		return 1; /* raw boot not supported */
	else
		return 0;
}

int mmc_read(unsigned int src, unsigned char *dst, int size)
{
	printf("mmc_read: NOT Implemented\n");
	return 0;
}
int mmc_write(unsigned char *src, unsigned long dst, int size)
{
	printf("mmc_write: NOT Implemented\n");
	return 0;
}

int mmc2info(unsigned int addr)
{
	printf("mmc2info: NOT Implemented\n");
	return 0;
}
#endif
