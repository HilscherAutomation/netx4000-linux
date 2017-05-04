/*
 * ARM PL35X NAND Flash Controller Driver
 *
 * Copyright (C) 2009 - 2014 Xilinx, Inc.
 *
 * This driver is based on plat_nand.c and mxc_nand.c drivers
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License version 2 as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */
/*
 * drivers/mtd/nand/smc353-nand.c
 *
 * (C) Copyright 2017 Hilscher Gesellschaft fuer Systemautomation mbH
 * http://www.hilscher.com
 * - Apdapted ECC initialization for Hilscher netx4000 based platforms.
 * - Add bch-ecc support.
 */

//#define DRIVER_DESC  "SMC353 NAND driver for Hilscher netx4000 based platforms"
#define DRIVER_DESC  "ARM PL35X NAND Flash Driver"
#define DRIVER_NAME  "smc353-nand-netx4000"

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/version.h>
#include <asm/io.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/slab.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/nand_ecc.h>
#include <linux/jiffies.h>
#include <linux/delay.h>
#include <linux/mtd/partitions.h>
#include <linux/mtd/nand_bch.h>

#include <linux/memory/smc353-mc.h>

#define ONDIE_ECC_FEATURE_ADDR		0x90

#define PL353_NAND_LAST_TRANSFER_LENGTH	4

#define PL353_NAND_ECC_BUSY_TIMEOUT	100000 /*(1 * HZ)*/
#define PL353_NAND_DEV_BUSY_TIMEOUT	(1* HZ )

/* AXI Address definitions */
#define AXI_CMD_ADDR_SHIFT      	24
#define AXI_CMD_ADDR_MASK      		0xFF000000
#define AXI_CMD_ADDR_CYCLES_SHIFT	21
#define AXI_CMD_ADDR_CYCLES_MASK	0x00E00000
#define AXI_CMD_CLEAR_CS_SHIFT		21
#define AXI_CMD_CLEAR_CS_MASK		0x00200000
#define AXI_CMD_END_CMD_VALID_SHIFT	20
#define AXI_CMD_END_CMD_VALID_MASK	0x00100000
#define AXI_CMD_CMD_PHASE_SHIFT		19
#define AXI_CMD_CMD_PHASE_MASK		0x00080000
#define AXI_CMD_DATA_PHASE_SHIFT	19
#define AXI_CMD_DATA_PHASE_MASK		0x00080000
#define AXI_CMD_END_CMD_SHIFT		11
#define AXI_CMD_END_CMD_MASK		0x0007F800
#define AXI_CMD_ECC_LAST_SHIFT		10
#define AXI_CMD_ECC_LAST_MASK		0x00000400
#define AXI_CMD_START_CMD_SHIFT		3
#define AXI_CMD_START_CMD_MASK		0x000007F8

#define PL353_NAND_ECC_LAST		BIT(AXI_CMD_ECC_LAST_SHIFT)		/* Set ECC_Last */
#define PL353_NAND_CLEAR_CS		BIT(AXI_CMD_CLEAR_CS_SHIFT)		/* Clear chip select */
#define PL353_NAND_END_CMD_VALID	BIT(AXI_CMD_END_CMD_VALID_SHIFT)	/* Clear chip select */

#define ECC_BYTES_03	3
#define ECC_BYTES_12	12
#define ECC_BYTES_24	24
#define ECC_BYTES_32	32


struct pl353_nand_info {
	struct nand_chip chip;
	struct mtd_info mtd;
	char ecc_mode[30];
	void __iomem *nand_base;
	struct device *pdev;
	unsigned long end_cmd_pending;
	unsigned long end_cmd;
	int ifc;
	int cs_addr;
};

struct pl353_nand_command_format {
	int start_cmd;
	int end_cmd;
	u8 addr_cycles;
	u8 end_cmd_valid;
};

/* NAND flash driver defines */
#define PL353_NAND_CMD_PHASE	1	/* End command valid in command phase */
#define PL353_NAND_DATA_PHASE	2	/* End command valid in data phase */
#define PL353_NAND_ECC_SIZE	512	/* Size of data for ECC operation */

static struct pl353_nand_info *s_nand = NULL;


int get_ondie_ecc_state(struct mtd_info *mtd){
	struct nand_chip *nand_chip = mtd->priv;
	u8 get_feature;
	nand_chip->cmdfunc(mtd, NAND_CMD_GET_FEATURES,
                        ONDIE_ECC_FEATURE_ADDR, -1);
    
	nand_chip->IO_ADDR_R = (void __iomem*)((((uint32_t)(nand_chip->IO_ADDR_R)) | (1UL<<21)));
	get_feature = readb(nand_chip->IO_ADDR_R);

	return !!(get_feature & 0x08);
}


int enable_ondie_ecc(struct mtd_info *mtd,int enable) {
	struct nand_chip *nand_chip = mtd->priv;
	u8 set_feature[4] = { 0x08, 0x00, 0x00, 0x00 };
	int i;

	if (!enable)
		set_feature[0] = 0;

	nand_chip->cmdfunc(mtd, NAND_CMD_SET_FEATURES,
			ONDIE_ECC_FEATURE_ADDR, -1);
	for (i = 0; i < 4; i++)
		writeb(set_feature[i], nand_chip->IO_ADDR_W);

	ndelay(1000);

	if (enable && get_ondie_ecc_state(mtd))
		return 0;
	else
		return enable;
}

ssize_t show_ecc_state(struct device_driver *drv, char * buf)
{
		return sprintf(buf,"%s",s_nand->ecc_mode);
}
ssize_t store_ecc_state(struct device_driver *drv, const char * buf,
                         size_t count)
{
	//sscanf(buf,"%s",s_ecc_mode);
	//if (!strcmp(s_ecc_mode,"1") || !strcmp(s_ecc_mode,"ondie_ecc"))
	//	ondie_ecc = 1;

	//ret = enable_ondie_ecc(s_mtd,ondie_ecc);
	//printk("ondie_ecc change returned %d (enabled:%d)\n",ret,ondie_ecc);

	return count;
}

static DRIVER_ATTR(ecc_mode, S_IWUSR | S_IRUGO, show_ecc_state, store_ecc_state);

/*
 * The NAND flash operations command format
 */
static const struct pl353_nand_command_format pl353_nand_commands[] = {
	{NAND_CMD_READ0, NAND_CMD_READSTART, 5, PL353_NAND_CMD_PHASE},
	{NAND_CMD_RNDOUT, NAND_CMD_RNDOUTSTART, 2, PL353_NAND_CMD_PHASE},
	{NAND_CMD_READID, NAND_CMD_NONE, 1, NAND_CMD_NONE},
	{NAND_CMD_STATUS, NAND_CMD_NONE, 0, NAND_CMD_NONE},
	{NAND_CMD_SEQIN, NAND_CMD_PAGEPROG, 5, PL353_NAND_DATA_PHASE},
	{NAND_CMD_RNDIN, NAND_CMD_NONE, 2, NAND_CMD_NONE},
	{NAND_CMD_ERASE1, NAND_CMD_ERASE2, 3, PL353_NAND_CMD_PHASE},
	{NAND_CMD_RESET, NAND_CMD_NONE, 0, NAND_CMD_NONE},
	{NAND_CMD_PARAM, NAND_CMD_NONE, 1, NAND_CMD_NONE},
	{NAND_CMD_GET_FEATURES, NAND_CMD_NONE, 1, NAND_CMD_NONE},
	{NAND_CMD_SET_FEATURES, NAND_CMD_NONE, 1, NAND_CMD_NONE},
	{NAND_CMD_NONE, NAND_CMD_NONE, 0, 0},
	/* Add all the flash commands supported by the flash device and Linux */
	/*
	 * The cache program command is not supported by driver because driver
	 * cant differentiate between page program and cached page program from
	 * start command, these commands can be differentiated through end
	 * command, which doesn't fit in to the driver design. The cache program
	 * command is not supported by NAND subsystem also, look at 1612 line
	 * number (in nand_write_page function) of nand_base.c file.
	 * {NAND_CMD_SEQIN, NAND_CMD_CACHEDPROG, 5, PL353_NAND_YES},
	 */
};

/* Define default oob placement schemes for large and small page devices */
static struct nand_ecclayout nand_oob_16 = {
	.eccbytes = ECC_BYTES_03,
	.eccpos = {0, 1, 2},
	.oobfree = {
		{.offset = 8,
		 . length = 8
		}
	}
};

static struct nand_ecclayout nand_oob_64_ecc12 = {
	.eccbytes = ECC_BYTES_12,
	.eccpos = {
		   52, 53, 54, 55, 56, 57,
		   58, 59, 60, 61, 62, 63},
	.oobfree = {
		{.offset = 2,
		 .length = 50}
	}
};

static struct nand_ecclayout nand_oob_64_ecc24 = {
	.eccbytes = ECC_BYTES_24,
	.eccpos = {
		40, 41, 42, 43, 44, 45,
		46, 47, 48, 49, 50, 51,
		52, 53, 54, 55, 56, 57,
		58, 59, 60, 61, 62, 63},
	.oobfree = {
		{.offset = 2,
		.length = 38}
	}
};

/* currently not used */
// static struct nand_ecclayout nand_oob_64_ecc28 = {
// 	.eccbytes = 28,
// 	.eccpos = {
// 		36, 37, 38, 39, 40, 41, 42, 
// 		43, 44, 45, 46, 47, 48, 49,
// 		50, 51, 52, 53, 54, 55, 56,
// 		57, 58, 59, 60, 61, 62, 63},
// 	.oobfree = {
// 		{.offset = 2,
// 		.length = 34}
// 	}
// };

static struct nand_ecclayout ondie_nand_oob_64 = {
	.eccbytes = ECC_BYTES_32,

	.eccpos = {
		8, 9, 10, 11, 12, 13, 14, 15,
		24, 25, 26, 27, 28, 29, 30, 31,
		40, 41, 42, 43, 44, 45, 46, 47,
		56, 57, 58, 59, 60, 61, 62, 63
	},
	.oobfree = {
		{ .offset = 4, .length = 4 },
		{ .offset = 20, .length = 4 },
		{ .offset = 36, .length = 4 },
		{ .offset = 52, .length = 4 }
	}
};

/* Generic flash bbt decriptors */
static uint8_t bbt_pattern[] = { 'B', 'b', 't', '0' };
static uint8_t mirror_pattern[] = { '1', 't', 'b', 'B' };

static struct nand_bbt_descr bbt_main_descr = {
	.options = NAND_BBT_LASTBLOCK | NAND_BBT_CREATE | NAND_BBT_WRITE
		| NAND_BBT_2BIT | NAND_BBT_VERSION | NAND_BBT_PERCHIP,
	.offs = 4,
	.len = 4,
	.veroffs = 20,
	.maxblocks = 16,
	.pattern = bbt_pattern
};

static struct nand_bbt_descr bbt_mirror_descr = {
	.options = NAND_BBT_LASTBLOCK | NAND_BBT_CREATE | NAND_BBT_WRITE
		| NAND_BBT_2BIT | NAND_BBT_VERSION | NAND_BBT_PERCHIP,
	.offs = 4,
	.len = 4,
	.veroffs = 20,
	.maxblocks = 16,
	.pattern = mirror_pattern
};

static void pl353_nand_read_buf(struct mtd_info *mtd, uint8_t *buf, int len);

/**
 * pl353_nand_calculate_hwecc - Calculate Hardware ECC
 * @mtd:	Pointer to the mtd_info structure
 * @data:	Pointer to the page data
 * @ecc_code:	Pointer to the ECC buffer where ECC data needs to be stored
 *
 * This function retrieves the Hardware ECC data from the controller and returns
 * ECC data back to the MTD subsystem.
 *
 * Return:	0 on success or error value on failure
 */
static int pl353_nand_calculate_hwecc(struct mtd_info *mtd,
				const u8 *data, u8 *ecc_code)
{
	struct nand_chip *nand_chip = (struct nand_chip*)mtd->priv;
	struct pl353_nand_info *info = (struct pl353_nand_info*)nand_chip->priv;
	u32 ecc_value, ecc_status;
	u8 ecc_reg, ecc_byte;
	unsigned long timeout = jiffies + PL353_NAND_ECC_BUSY_TIMEOUT;

	/* Wait till the ECC operation is complete or timeout */
	do {
		if (smc35x_ecc_is_busy(info->pdev)) {
			//cpu_relax();TODO
		} else {
			break;
		}
	} while (jiffies<timeout);

	if (jiffies>timeout) {
		pr_err("%s timed out\n", __func__);
		return -ETIMEDOUT;
	}

	for (ecc_reg = 0; ecc_reg < 4; ecc_reg++) {
		/* Read ECC value for each block */
		ecc_value = smc35x_get_ecc_val(info->pdev, ecc_reg);
		ecc_status = (ecc_value >> 24) & 0xFF;
		/* ECC value valid */
		if (ecc_status & 0x40) {
			for (ecc_byte = 0; ecc_byte < 3; ecc_byte++) {
				/* Copy ECC bytes to MTD buffer */
				*ecc_code = ecc_value & 0xFF;
				ecc_value = ecc_value >> 8;
				ecc_code++;
			}
		} else {
			pr_warn("%s status failed\n", __func__);
			return -1;
		}
	}
	return 0;
}

/**
 * onehot - onehot function
 * @value:	Value to check for onehot
 *
 * This function checks whether a value is onehot or not.
 * onehot is if and only if onebit is set.
 *
 * Return:	1 if it is onehot else 0
 */
static int onehot(unsigned short value)
{
	return (value & (value - 1)) == 0;
}

/**
 * pl353_nand_detect_ondie_ecc - Get the flash ondie ecc state
 * @mtd:	Pointer to the mtd_info structure
 *
 * This function enables the ondie ecc for the Micron ondie ecc capable devices
 *
 * Return:	1 on detect, 0 if fail to detect
 */
static int pl353_nand_detect_ondie_ecc(struct mtd_info *mtd)
{
	struct nand_chip *nand_chip = mtd->priv;
	u8 maf_id, dev_id, i, get_feature;
	u8 set_feature[4] = { 0x08, 0x00, 0x00, 0x00 };

	/* Check if On-Die ECC flash */
	nand_chip->cmdfunc(mtd, NAND_CMD_RESET, -1, -1);
	nand_chip->cmdfunc(mtd, NAND_CMD_READID, 0x00, -1);

	/* Read manufacturer and device IDs */
	maf_id = readb(nand_chip->IO_ADDR_R);
	/* clear chip select to mark last transfer */
	nand_chip->IO_ADDR_R = (void __iomem*)((((uint32_t)(nand_chip->IO_ADDR_R)) | (1UL<<21)));
	dev_id = readb(nand_chip->IO_ADDR_R);

	if ((maf_id == NAND_MFR_MICRON) &&
	    ((dev_id == 0xf1) || (dev_id == 0xa1) ||
	     (dev_id == 0xb1) || (dev_id == 0xaa) ||
	     (dev_id == 0xba) || (dev_id == 0xda) ||
	     (dev_id == 0xca) || (dev_id == 0xac) ||
	     (dev_id == 0xbc) || (dev_id == 0xdc) ||
	     (dev_id == 0xcc) || (dev_id == 0xa3) ||
	     (dev_id == 0xb3) ||
	     (dev_id == 0xd3) || (dev_id == 0xc3))) {

		nand_chip->cmdfunc(mtd, NAND_CMD_RESET, -1, -1);
		nand_chip->cmdfunc(mtd, NAND_CMD_GET_FEATURES,
				   ONDIE_ECC_FEATURE_ADDR, -1);

		nand_chip->IO_ADDR_R = (void __iomem*)((((uint32_t)(nand_chip->IO_ADDR_R)) | (1UL<<21)));
		get_feature = readb(nand_chip->IO_ADDR_R);

		if (get_feature & 0x08) {
			return 1;
		} else {
			nand_chip->cmdfunc(mtd, NAND_CMD_SET_FEATURES,
					   ONDIE_ECC_FEATURE_ADDR, -1);
			for (i = 0; i < 4; i++)
				writeb(set_feature[i], nand_chip->IO_ADDR_W);

			ndelay(1000);

			nand_chip->cmdfunc(mtd, NAND_CMD_GET_FEATURES,
					   ONDIE_ECC_FEATURE_ADDR, -1);
			
			nand_chip->IO_ADDR_R = (void __iomem*)((((uint32_t)(nand_chip->IO_ADDR_R)) | (1UL<<21)));
			get_feature = readb(nand_chip->IO_ADDR_R);

			if (get_feature & 0x08)
				return 1;

		}
	}
	return 0;
}

/**
 * pl353_nand_correct_data - ECC correction function
 * @mtd:	Pointer to the mtd_info structure
 * @buf:	Pointer to the page data
 * @read_ecc:	Pointer to the ECC value read from spare data area
 * @calc_ecc:	Pointer to the calculated ECC value
 *
 * This function corrects the ECC single bit errors & detects 2-bit errors.
 *
 * Return:	0 if no ECC errors found
 *		1 if single bit error found and corrected.
 *		-1 if multiple ECC errors found.
 */
static int pl353_nand_correct_data(struct mtd_info *mtd, unsigned char *buf,
				unsigned char *read_ecc,
				unsigned char *calc_ecc)
{
	unsigned char bit_addr;
	unsigned int byte_addr;
	unsigned short ecc_odd, ecc_even, read_ecc_lower, read_ecc_upper;
	unsigned short calc_ecc_lower, calc_ecc_upper;

	read_ecc_lower = (read_ecc[0] | (read_ecc[1] << 8)) & 0xfff;
	read_ecc_upper = ((read_ecc[1] >> 4) | (read_ecc[2] << 4)) & 0xfff;

	calc_ecc_lower = (calc_ecc[0] | (calc_ecc[1] << 8)) & 0xfff;
	calc_ecc_upper = ((calc_ecc[1] >> 4) | (calc_ecc[2] << 4)) & 0xfff;

	ecc_odd = read_ecc_lower ^ calc_ecc_lower;
	ecc_even = read_ecc_upper ^ calc_ecc_upper;

	if ((ecc_odd == 0) && (ecc_even == 0))
		return 0;       /* no error */

	if (ecc_odd == (~ecc_even & 0xfff)) {
		/* bits [11:3] of error code is byte offset */
		byte_addr = (ecc_odd >> 3) & 0x1ff;
		/* bits [2:0] of error code is bit offset */
		bit_addr = ecc_odd & 0x7;
		/* Toggling error bit */
		buf[byte_addr] ^= (1 << bit_addr);
		return 1;
	}

	if (onehot(ecc_odd | ecc_even) == 1)
		return 1; /* one error in parity */

	return -1; /* Uncorrectable error */
}

/**
 * pl353_nand_read_oob - [REPLACABLE] the most common OOB data read function
 * @mtd:	Pointer to the mtd info structure
 * @chip:	Pointer to the NAND chip info structure
 * @page:	Page number to read
 *
 * Return:	Always return zero
 */
static int pl353_nand_read_oob(struct mtd_info *mtd, struct nand_chip *chip,
			    int page)
{
	unsigned long data_phase_addr;
	uint8_t *p;

        /* cmdfunc will replace NAND_CMD_READOOB by NAND_CMD_READ (offset start-of-oob-area) */
	chip->cmdfunc(mtd, NAND_CMD_READOOB, 0, page);

	p = chip->oob_poi;
	chip->read_buf(mtd, p,
			(mtd->oobsize - PL353_NAND_LAST_TRANSFER_LENGTH));
	p += (mtd->oobsize - PL353_NAND_LAST_TRANSFER_LENGTH);

	data_phase_addr = (unsigned long __force)chip->IO_ADDR_R;
	data_phase_addr |= PL353_NAND_CLEAR_CS;
	chip->IO_ADDR_R = (void __iomem * __force)data_phase_addr;
	chip->read_buf(mtd, p, PL353_NAND_LAST_TRANSFER_LENGTH);

	return 0;
}

/**
 * pl353_nand_write_oob - [REPLACABLE] the most common OOB data write function
 * @mtd:	Pointer to the mtd info structure
 * @chip:	Pointer to the NAND chip info structure
 * @page:	Page number to write
 *
 * Return:	Zero on success and EIO on failure
 */
static int pl353_nand_write_oob(struct mtd_info *mtd, struct nand_chip *chip,
			     int page)
{
	int status = 0;
	const uint8_t *buf = chip->oob_poi;
	unsigned long data_phase_addr;

	chip->cmdfunc(mtd, NAND_CMD_SEQIN, mtd->writesize, page);

	chip->write_buf(mtd, buf,
			(mtd->oobsize - PL353_NAND_LAST_TRANSFER_LENGTH));
	buf += (mtd->oobsize - PL353_NAND_LAST_TRANSFER_LENGTH);

	data_phase_addr = (unsigned long __force)chip->IO_ADDR_W;
	data_phase_addr |= PL353_NAND_CLEAR_CS;
	data_phase_addr |= PL353_NAND_END_CMD_VALID;
	chip->IO_ADDR_W = (void __iomem * __force)data_phase_addr;
	chip->write_buf(mtd, buf, PL353_NAND_LAST_TRANSFER_LENGTH);

	/* Send command to program the OOB data */
	chip->cmdfunc(mtd, NAND_CMD_PAGEPROG, -1, -1);
	status = chip->waitfunc(mtd, chip);

	return status & NAND_STATUS_FAIL ? -EIO : 0;
}

/**
 * pl353_nand_read_page_raw - [Intern] read raw page data without ecc
 * @mtd:		Pointer to the mtd info structure
 * @chip:		Pointer to the NAND chip info structure
 * @buf:		Pointer to the data buffer
 * @oob_required:	Caller requires OOB data read to chip->oob_poi
 * @page:		Page number to read
 *
 * Return:	Always return zero
 */
static int pl353_nand_read_page_raw(struct mtd_info *mtd,
				struct nand_chip *chip,
				uint8_t *buf, int oob_required, int page)
{
	unsigned long data_phase_addr;
	uint8_t *p;

	chip->read_buf(mtd, buf, mtd->writesize);

	p = chip->oob_poi;
	chip->read_buf(mtd, p,
			(mtd->oobsize - PL353_NAND_LAST_TRANSFER_LENGTH));
	p += (mtd->oobsize - PL353_NAND_LAST_TRANSFER_LENGTH);

	data_phase_addr = (unsigned long __force)chip->IO_ADDR_R;
	data_phase_addr |= PL353_NAND_CLEAR_CS;
	chip->IO_ADDR_R = (void __iomem * __force)data_phase_addr;

	chip->read_buf(mtd, p, PL353_NAND_LAST_TRANSFER_LENGTH);
        
	return 0;
}

/**
 * pl353_nand_write_page_raw - [Intern] raw page write function
 * @mtd:		Pointer to the mtd info structure
 * @chip:		Pointer to the NAND chip info structure
 * @buf:		Pointer to the data buffer
 * @oob_required:	Caller requires OOB data read to chip->oob_poi
 *
 * Return:	Always return zero
 */
static int pl353_nand_write_page_raw(struct mtd_info *mtd,
				    struct nand_chip *chip,
				    const uint8_t *buf, int oob_required
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 4, 0)
	    			    ,int page
#endif
	)
{
	unsigned long data_phase_addr;
	uint8_t *p;

	chip->write_buf(mtd, buf, mtd->writesize);
        
	p = chip->oob_poi;
	chip->write_buf(mtd, p,
			(mtd->oobsize - PL353_NAND_LAST_TRANSFER_LENGTH));
	p += (mtd->oobsize - PL353_NAND_LAST_TRANSFER_LENGTH);

	data_phase_addr = (unsigned long __force)chip->IO_ADDR_W;
	data_phase_addr |= PL353_NAND_CLEAR_CS;
	data_phase_addr |= PL353_NAND_END_CMD_VALID;
	chip->IO_ADDR_W = (void __iomem * __force)data_phase_addr;

	chip->write_buf(mtd, p, PL353_NAND_LAST_TRANSFER_LENGTH);
        
	return 0;
}

/**
 * pl353_nand_read_page_hwecc - Hardware ECC based page read function
 * @mtd:		Pointer to the mtd info structure
 * @chip:		Pointer to the NAND chip info structure
 * @buf:		Pointer to the buffer to store read data
 * @oob_required:	Caller requires OOB data read to chip->oob_poi
 * @page:		Page number to read
 *
 * This functions reads data and checks the data integrity by comparing hardware
 * generated ECC values and read ECC values from spare area.
 *
 * Return:	0 always and updates ECC operation status in to MTD structure
 */
static int pl353_nand_read_page_hwecc(struct mtd_info *mtd,
				     struct nand_chip *chip,
				     uint8_t *buf, int oob_required, int page)
{
	int i, stat, eccsize = chip->ecc.size;
	int eccbytes = chip->ecc.bytes;
	int eccsteps = chip->ecc.steps;
	uint8_t *p = buf;
	uint8_t *ecc_calc = chip->buffers->ecccalc;
	uint8_t *ecc_code = chip->buffers->ecccode;
	uint32_t *eccpos = chip->ecc.layout->eccpos;
	unsigned long data_phase_addr;
	uint8_t *oob_ptr;

	for ( ; (eccsteps - 1); eccsteps--) {
		chip->read_buf(mtd, p, eccsize);
		p += eccsize;
	}
	chip->read_buf(mtd, p, (eccsize - PL353_NAND_LAST_TRANSFER_LENGTH));
	p += (eccsize - PL353_NAND_LAST_TRANSFER_LENGTH);

	/* Set ECC Last bit to 1 */
	data_phase_addr = (unsigned long __force)chip->IO_ADDR_R;
	data_phase_addr |= PL353_NAND_ECC_LAST;
	chip->IO_ADDR_R = (void __iomem * __force)data_phase_addr;
	chip->read_buf(mtd, p, PL353_NAND_LAST_TRANSFER_LENGTH);

	/* Read the calculated ECC value */
	p = buf;
	chip->ecc.calculate(mtd, p, &ecc_calc[0]);

	/* Clear ECC last bit */
	data_phase_addr = (unsigned long __force)chip->IO_ADDR_R;
	data_phase_addr &= ~PL353_NAND_ECC_LAST;
	chip->IO_ADDR_R = (void __iomem * __force)data_phase_addr;

	/* Read the stored ECC value */
	oob_ptr = chip->oob_poi;
	chip->read_buf(mtd, oob_ptr,
			(mtd->oobsize - PL353_NAND_LAST_TRANSFER_LENGTH));

	/* de-assert chip select */
	data_phase_addr = (unsigned long __force)chip->IO_ADDR_R;
	data_phase_addr |= PL353_NAND_CLEAR_CS;
	chip->IO_ADDR_R = (void __iomem * __force)data_phase_addr;

	oob_ptr += (mtd->oobsize - PL353_NAND_LAST_TRANSFER_LENGTH);
	chip->read_buf(mtd, oob_ptr, PL353_NAND_LAST_TRANSFER_LENGTH);

	for (i = 0; i < chip->ecc.total; i++)
		ecc_code[i] = ~(chip->oob_poi[eccpos[i]]);

	eccsteps = chip->ecc.steps;
	p = buf;

	/* Check ECC error for all blocks and correct if it is correctable */
	for (i = 0 ; eccsteps; eccsteps--, i += eccbytes, p += eccsize) {
		stat = chip->ecc.correct(mtd, p, &ecc_code[i], &ecc_calc[i]);
		if (stat < 0)
			mtd->ecc_stats.failed++;
		else
			mtd->ecc_stats.corrected += stat;
	}
	return 0;
}

/**
 * pl353_nand_read_page_swecc - [REPLACABLE] software ecc based page read function
 * @mtd:		Pointer to the mtd info structure
 * @chip:		Pointer to the NAND chip info structure
 * @buf:		Pointer to the buffer to store read data
 * @oob_required:	Caller requires OOB data read to chip->oob_poi
 * @page:		Page number to read
 *
 * Return:	Always return zero
 */
static int pl353_nand_read_page_swecc(struct mtd_info *mtd,
									  struct nand_chip *chip,
									  uint8_t *buf,  int oob_required, int page)
{
	int i, eccsize = chip->ecc.size;
	int eccbytes = chip->ecc.bytes;
	int eccsteps = chip->ecc.steps;
	uint8_t *p = buf;
	uint8_t *ecc_calc = chip->buffers->ecccalc;
	uint8_t *ecc_code = chip->buffers->ecccode;
	uint32_t *eccpos = chip->ecc.layout->eccpos;
	unsigned int max_bitflips = 0;

	chip->ecc.read_page_raw(mtd, chip, buf, 1, page);

	for (i = 0; eccsteps; eccsteps--, i += eccbytes, p += eccsize)
		chip->ecc.calculate(mtd, p, &ecc_calc[i]);

	for (i = 0; i < chip->ecc.total; i++)
		ecc_code[i] = chip->oob_poi[eccpos[i]];

	eccsteps = chip->ecc.steps;
	p = buf;

	for (i = 0 ; eccsteps; eccsteps--, i += eccbytes, p += eccsize) {
		int stat;

		stat = chip->ecc.correct(mtd, p, &ecc_code[i], &ecc_calc[i]);
		if (stat < 0) {
			mtd->ecc_stats.failed++;
		} else {
			mtd->ecc_stats.corrected += stat;
			max_bitflips = max_t(unsigned int, max_bitflips, stat);
		}
	}
	return max_bitflips;
}

/**
 * nand_write_page_hwecc - Hardware ECC based page write function
 * @mtd:		Pointer to the mtd info structure
 * @chip:		Pointer to the NAND chip info structure
 * @buf:		Pointer to the data buffer
 * @oob_required:	Caller requires OOB data read to chip->oob_poi
 *
 * This functions writes data and hardware generated ECC values in to the page.
 *
 * Return:	Always return zero
 */
static int pl353_nand_write_page_hwecc(struct mtd_info *mtd,
				    struct nand_chip *chip, const uint8_t *buf,
				    int oob_required
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 4, 0)
				    , int page
#endif
	)
{
	int i, eccsize = chip->ecc.size;
	int eccsteps = chip->ecc.steps;
	uint8_t *ecc_calc = chip->buffers->ecccalc;
	const uint8_t *p = buf;
	uint32_t *eccpos = chip->ecc.layout->eccpos;
	unsigned long data_phase_addr;
	uint8_t *oob_ptr;

	for ( ; (eccsteps - 1); eccsteps--) {
		chip->write_buf(mtd, p, eccsize);
		p += eccsize;
	}
	chip->write_buf(mtd, p, (eccsize - PL353_NAND_LAST_TRANSFER_LENGTH));
	p += (eccsize - PL353_NAND_LAST_TRANSFER_LENGTH);

	/* Set ECC Last bit to 1 */
	data_phase_addr = (unsigned long __force)chip->IO_ADDR_W;
	data_phase_addr |= PL353_NAND_ECC_LAST;
	chip->IO_ADDR_W = (void __iomem * __force)data_phase_addr;
	chip->write_buf(mtd, p, PL353_NAND_LAST_TRANSFER_LENGTH);

	/* Wait for ECC to be calculated and read the error values */
	p = buf;
	chip->ecc.calculate(mtd, p, &ecc_calc[0]);

	for (i = 0; i < chip->ecc.total; i++)
		chip->oob_poi[eccpos[i]] = ~(ecc_calc[i]);

	/* Clear ECC last bit */
	data_phase_addr = (unsigned long __force)chip->IO_ADDR_W;
	data_phase_addr &= ~PL353_NAND_ECC_LAST;
	chip->IO_ADDR_W = (void __iomem * __force)data_phase_addr;

	/* Write the spare area with ECC bytes */
	oob_ptr = chip->oob_poi;
	chip->write_buf(mtd, oob_ptr,
			(mtd->oobsize - PL353_NAND_LAST_TRANSFER_LENGTH));

	data_phase_addr = (unsigned long __force)chip->IO_ADDR_W;
	data_phase_addr |= PL353_NAND_CLEAR_CS;
	data_phase_addr |= PL353_NAND_END_CMD_VALID;
	chip->IO_ADDR_W = (void __iomem * __force)data_phase_addr;
	oob_ptr += (mtd->oobsize - PL353_NAND_LAST_TRANSFER_LENGTH);
	chip->write_buf(mtd, oob_ptr, PL353_NAND_LAST_TRANSFER_LENGTH);

	return 0;
}

/**
 * pl353_nand_write_page_swecc - [REPLACABLE] software ecc based page write function
 * @mtd:		Pointer to the mtd info structure
 * @chip:		Pointer to the NAND chip info structure
 * @buf:		Pointer to the data buffer
 * @oob_required:	Caller requires OOB data read to chip->oob_poi
 *
 * Return:	Always return zero
 */
static int pl353_nand_write_page_swecc(struct mtd_info *mtd,
				       struct nand_chip *chip, const uint8_t *buf,
				       int oob_required
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 4, 0)
				       , int page
#endif
	)
{
	int i, eccsize = chip->ecc.size;
	int eccbytes = chip->ecc.bytes;
	int eccsteps = chip->ecc.steps;
	uint8_t *ecc_calc = chip->buffers->ecccalc;
	const uint8_t *p = buf;
	uint32_t *eccpos = chip->ecc.layout->eccpos;

	/* Software ecc calculation */
	for (i = 0; eccsteps; eccsteps--, i += eccbytes, p += eccsize)
		chip->ecc.calculate(mtd, p, &ecc_calc[i]);


	for (i = 0; i < chip->ecc.total; i++)
		chip->oob_poi[eccpos[i]] = ecc_calc[i];

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 4, 0)
	chip->ecc.write_page_raw(mtd, chip, buf, 1);
#else
	chip->ecc.write_page_raw(mtd, chip, buf, 1, page);
#endif

	return 0;
}


/**
 * pl353_nand_ecc_init - Initialize the ecc information as per the ecc mode
 * @mtd:	Pointer to the mtd_info structure
 * @ondie_ecc_state:	ondie ecc status
 *
 * This function initializes the ecc block and functional pointers as per the
 * ecc mode
 */
static void pl353_nand_ecc_init(struct mtd_info *mtd, struct device_node *of)
{
	struct nand_chip *nand_chip = mtd->priv;
	struct pl353_nand_info *info = (struct pl353_nand_info*)nand_chip->priv;
	char* ecc_mode = NULL;

	/* We need to set always to ECC_HW since we can not use standard nand read/write functions */
	nand_chip->ecc.mode = NAND_ECC_HW;
	nand_chip->ecc.read_oob = pl353_nand_read_oob;
	nand_chip->ecc.read_page_raw = pl353_nand_read_page_raw;
	nand_chip->ecc.write_oob = pl353_nand_write_oob;
	nand_chip->ecc.write_page_raw = pl353_nand_write_page_raw;

	nand_chip->options |= NAND_NO_SUBPAGE_WRITE;
	
	nand_chip->bbt_td = &bbt_main_descr;
	nand_chip->bbt_md = &bbt_mirror_descr;

	of_property_read_string(of, "ecc-mode", (const char**)&ecc_mode);

	if (ecc_mode != NULL)
		strncpy(info->ecc_mode,ecc_mode,sizeof(info->ecc_mode));
	else
		sprintf(info->ecc_mode,"unspecified");

	/* special case ondie-ecc: */
	if (!strcmp("ondie-ecc",info->ecc_mode) && pl353_nand_detect_ondie_ecc(mtd)) {
		/* currently choose ondie since it is 4/5bit */
		smc35x_set_ecc_mode(info->pdev, PL353_SMC_ECCMODE_BYPASS);
                
		nand_chip->ecc.read_page = pl353_nand_read_page_raw;
		nand_chip->ecc.write_page = pl353_nand_write_page_raw;
                
		/* in case of ondie ecc datasheet ecc values are the same as the values of the ecc ctl */
		nand_chip->ecc.size = nand_chip->ecc_step_ds;
		nand_chip->ecc.strength = nand_chip->ecc_strength_ds;
		
		/* get layout (on-die layout) */
		nand_chip->ecc.layout = &ondie_nand_oob_64;
		nand_chip->ecc.bytes = 8;/* in case of onfi 1.x ecc.bytes not given */
		/*
		 * On-Die ECC spare bytes offset 8 is used for ECC codes
		 * Use the BBT pattern descriptors
		 */
		nand_chip->bbt_td = &bbt_main_descr;
		nand_chip->bbt_md = &bbt_mirror_descr;
	} else {
		/* hardware-ecc: */
		if (get_ondie_ecc_state(mtd))
			enable_ondie_ecc( mtd, 0);

		/* TODO: we may need to disable ondie-ecc */
		/* User pl353 or software ecc handling */
		if (!strcmp("ondie-ecc",info->ecc_mode)) { /* failed to enable ondie-ecc */
			sprintf(info->ecc_mode,"bch-ecc");
			printk(KERN_ERR "No ondie-ecc supported by chip! Choose %s!\n",info->ecc_mode);
		}
		if (!strcmp("hw-ecc",info->ecc_mode)) {
			/* NOTE: !!!!Not implemented yet!!!! */
			/* Hardware ECC generates 3 bytes ECC code for each 512 bytes */
			nand_chip->ecc.bytes = 3; /* ecc bytes per step */
			nand_chip->ecc.calculate = pl353_nand_calculate_hwecc;
			nand_chip->ecc.correct = pl353_nand_correct_data;
			nand_chip->ecc.hwctl = NULL;
			nand_chip->ecc.read_page = pl353_nand_read_page_hwecc;
			nand_chip->ecc.size = PL353_NAND_ECC_SIZE; /* data bytes per ecc step */
			nand_chip->ecc.write_page = pl353_nand_write_page_hwecc;

			nand_chip->ecc_step_ds = 512;
			nand_chip->ecc_strength_ds = 
			smc35x_set_ecc_pg_size(info->pdev, mtd->writesize);
			/*switch (mtd->writesize) {
			case 512:
			case 1024:
			case 2048:
				smc35x_set_ecc_mode(info->pdev, PL353_SMC_ECCMODE_APB);
				break;
			default:*/
			//}
			
			switch(nand_chip->ecc.bytes*(mtd->writesize/nand_chip->ecc.size))
			{
				case ECC_BYTES_12:
					nand_chip->ecc.layout = &nand_oob_64_ecc12;
				break;
				case ECC_BYTES_24:
					nand_chip->ecc.layout = &nand_oob_64_ecc24;
				break;
			default:
				if (mtd->oobsize == 16)
					nand_chip->ecc.layout = &nand_oob_16;
				else if (mtd->oobsize == 64)
					nand_chip->ecc.layout = &nand_oob_64_ecc12;
				break;
			}
		} else {
			/* software-ecc: */
			/* NOTE: do not set mode to SOFT-ECC (nand_chip->ecc.mode = NAND_ECC_SOFT) 
			*       since all functions will be overwritten (we can not use the standard 
			*       read/write functions with this controller) */
			smc35x_set_ecc_mode(info->pdev, PL353_SMC_ECCMODE_BYPASS);
			
			//TODO: which ecc-mode to choose (in case it is no valid given) depends on the flash type

			/* read/write_swecc  are not exported ;( */
			nand_chip->ecc.read_page = pl353_nand_read_page_swecc;
			nand_chip->ecc.write_page = pl353_nand_write_page_swecc;

			if (!strcmp("sw-ecc",info->ecc_mode)) {
				/* use standard functions here */
				nand_chip->ecc.calculate = nand_calculate_ecc;
				nand_chip->ecc.correct = nand_correct_data;

				nand_chip->ecc.size = 512;

				if (mtd->oobsize == 16) {
					nand_chip->ecc.strength = 1;
					nand_chip->ecc.bytes = 3; /* ecc bytes per step */
					nand_chip->ecc.layout = &nand_oob_16;
				} else if (mtd->oobsize == 64) {
					nand_chip->ecc.strength = 1;
					nand_chip->ecc.bytes = 12; /* ecc bytes per step */
					nand_chip->ecc.layout = &nand_oob_64_ecc12;
				}
			} else {
				/* defaulting to bch-ecc mode */
				if (strcmp("bch-ecc",info->ecc_mode)) {
					/* no valid ecc-mode defined so set to bch- */
					sprintf(info->ecc_mode,"bch-ecc");
					printk(KERN_ERR "No valid ecc-mode given! Defaulting to %s.\n",info->ecc_mode);
				}
				/* use standard functions here */
				nand_chip->ecc.calculate = nand_bch_calculate_ecc;
				nand_chip->ecc.correct = nand_bch_correct_data;

				nand_chip->ecc.size = 512;
				
				if (mtd->oobsize == 16) {
					nand_chip->ecc.strength = 1;
					nand_chip->ecc.layout = &nand_oob_16;
					nand_chip->ecc.bytes = 8; /* ecc bytes per step */
				} else if (mtd->oobsize == 64) {
					nand_chip->ecc.strength = 4;
					nand_chip->ecc.bytes = 7; /* ecc bytes per step */
					/* Do not pass oob layout, let bch_init create layout */
					//nand_chip->ecc.layout = &nand_oob_64_ecc28;
					nand_chip->ecc.layout = NULL;
				}
				nand_chip->ecc.priv = nand_bch_init(mtd,nand_chip->ecc.size,nand_chip->ecc.bytes,&nand_chip->ecc.layout);
			}
		}
	}
}

/**
 * pl353_nand_device_ready - Check device ready/busy line
 * @mtd:	Pointer to the mtd_info structure
 *
 * Return:	0 on busy or 1 on ready state
 */
static int pl353_nand_device_ready(struct mtd_info *mtd)
{
	struct nand_chip *nand_chip = (struct nand_chip*)mtd->priv;
	struct pl353_nand_info *info = (struct pl353_nand_info*)nand_chip->priv;

	if (smc35x_get_nand_int_status_raw(info->pdev, info->ifc)) {
		smc35x_clr_nand_int(info->pdev, info->ifc);
		return 1;
	}
	return 0;
}

/**
 * pl353_nand_select_chip - Select the flash device
 * @mtd:	Pointer to the mtd info structure
 * @chip:	Pointer to the NAND chip info structure
 *
 * This function is empty as the NAND controller handles chip select line
 * internally based on the chip address passed in command and data phase.
 */
static void pl353_nand_select_chip(struct mtd_info *mtd, int chip)
{
	/* TODO: check if chip select is required, or selection may conflict to
	 other interfaced memories */
	return;
}

static inline void pl353_nand_write64(void __iomem *addr, u64 val)
{
	volatile uint64_t* tmp = (volatile uint64_t*)addr;
	*tmp = val;
}

static inline void pl353_nand_write32(void __iomem *addr, u32 val)
{
	writel((val), (addr));
}

/* pl353_nand_cmd_function - Send command to NAND device
 * @mtd:	Pointer to the mtd_info structure
 * @command:	The command to be sent to the flash device
 * @column:	The column address for this command, -1 if none
 * @page_addr:	The page address for this command, -1 if none
 */
static void pl353_nand_cmd_function(struct mtd_info *mtd, unsigned int command,
				 int column, int page_addr)
{
	struct nand_chip *chip = mtd->priv;
	const struct pl353_nand_command_format *curr_cmd = NULL;
	struct pl353_nand_info *xnand =
		container_of(mtd, struct pl353_nand_info, mtd);
	void __iomem *cmd_addr;
	unsigned long cmd_data = 0, end_cmd_valid = 0;
	unsigned long cmd_phase_addr, data_phase_addr, end_cmd, i;
	unsigned long timeout = jiffies + PL353_NAND_DEV_BUSY_TIMEOUT;
	u8 addr_cycles = 0;
	u8 start_cmd = 0;

	if (xnand->end_cmd_pending) {
		/*
		 * Check for end command if this command request is same as the
		 * pending command then return
		 */
		if (xnand->end_cmd == command) {
			xnand->end_cmd = 0;
			xnand->end_cmd_pending = 0;
			return;
		}
	}

	/* Emulate NAND_CMD_READOOB for large page device */
	if ((mtd->writesize > PL353_NAND_ECC_SIZE) &&
	    (command == NAND_CMD_READOOB)) {
		column += mtd->writesize;
		command = NAND_CMD_READ0;
	}

	/* Get the command format */
	for (i = 0; (pl353_nand_commands[i].start_cmd != NAND_CMD_NONE ||
		     pl353_nand_commands[i].end_cmd != NAND_CMD_NONE); i++)
		if (command == pl353_nand_commands[i].start_cmd)
			break;
			
	curr_cmd = &pl353_nand_commands[i];
	if ((pl353_nand_commands[i].start_cmd == NAND_CMD_NONE) &&
		     (pl353_nand_commands[i].end_cmd == NAND_CMD_NONE)) {
		pr_err("%s command not found (%d)\n", __func__, command);
		return;
	}

	/* Clear interrupt */
	smc35x_clr_nand_int( xnand->pdev, xnand->ifc);

	addr_cycles   = curr_cmd->addr_cycles;
	end_cmd_valid = (curr_cmd->end_cmd_valid == PL353_NAND_CMD_PHASE ? 1 : 0);
	end_cmd       = (curr_cmd->end_cmd == NAND_CMD_NONE ? 0x00 : curr_cmd->end_cmd);
	start_cmd     = curr_cmd->start_cmd;

	cmd_phase_addr = (addr_cycles << AXI_CMD_ADDR_CYCLES_SHIFT)	|           /* addr cycles */
			 (end_cmd_valid << AXI_CMD_END_CMD_VALID_SHIFT)	|       /* end cmd info */
			 (0 << AXI_CMD_CMD_PHASE_SHIFT)			|                   /* mark as cmd phase addr */
			 (end_cmd << AXI_CMD_END_CMD_SHIFT)		|                 /* end cmd info */
			 (start_cmd << AXI_CMD_START_CMD_SHIFT);                /* start cmd info */

	cmd_addr = (void __iomem * __force)(cmd_phase_addr | (unsigned long)xnand->nand_base);
	data_phase_addr = (1 << AXI_CMD_CLEAR_CS_SHIFT)		| /* do not hold chip select */
			  (1 << AXI_CMD_DATA_PHASE_SHIFT)		| /* mark as data phase addr */
			  (0x0 << AXI_CMD_ECC_LAST_SHIFT);

        //data_phase_addr += (unsigned long)xnand->nand_base;
	chip->IO_ADDR_R = (void __iomem * __force)(data_phase_addr | (unsigned long)xnand->nand_base);
	/* for write transaction add end cmd info */
	chip->IO_ADDR_W = (void __iomem * __force)(data_phase_addr | 
	                  (end_cmd_valid << AXI_CMD_END_CMD_VALID_SHIFT) | 
	                  (end_cmd << AXI_CMD_END_CMD_SHIFT));
	chip->IO_ADDR_W =  (void __iomem * __force)((unsigned long)chip->IO_ADDR_W | (unsigned long)xnand->nand_base);

	/* Command phase AXI write */
	/* Read & Write */
	if (column != -1 && page_addr != -1) {
		/* Adjust columns for 16 bit bus width */
		if (chip->options & NAND_BUSWIDTH_16)
			column >>= 1;
		cmd_data = column;
		if (mtd->writesize > PL353_NAND_ECC_SIZE) {
			cmd_data |= page_addr << 16;
			/* Another address cycle for devices > 128MiB */
			if (chip->chipsize > (128 << 20)) {
				pl353_nand_write32(cmd_addr, cmd_data);
				cmd_data = (page_addr >> 16);
			}
		} else {
			cmd_data |= page_addr << 8;
		}
	} else if (page_addr != -1) {
		/* Erase */
		cmd_data = page_addr;
	} else if (column != -1) {
		/*
		 * Change read/write column, read id etc
		 * Adjust columns for 16 bit bus width
		 */
		if ((chip->options & NAND_BUSWIDTH_16) &&
			((command == NAND_CMD_READ0) ||
			(command == NAND_CMD_SEQIN) ||
			(command == NAND_CMD_RNDOUT) ||
			(command == NAND_CMD_RNDIN)))
				column >>= 1;
		cmd_data = column;
	}

	/* TODO: if more than four cycles */
	/*if (curr_cmd->addr_cycles>4)
		pl353_nand_write64(cmd_addr, (u64)cmd_data);
	else*/
		pl353_nand_write32(cmd_addr, cmd_data);

	if (curr_cmd->end_cmd_valid) {
		xnand->end_cmd = curr_cmd->end_cmd;
		xnand->end_cmd_pending = 1;
	}

	/* skip NAND_CMD_STATUS, since frame work will call chip->dev_ready() */
	if ((command == NAND_CMD_READ0) ||
	    (command == NAND_CMD_RESET) ||
	    (command == NAND_CMD_PARAM) ||
	    (command == NAND_CMD_GET_FEATURES)) {

		/* Wait till the device is ready or timeout */
		do {
			if (chip->dev_ready(mtd)) {
				break;
			} else {
				cpu_relax();
			}
		} while (!time_after_eq(jiffies,timeout));

		if (time_after_eq(jiffies,timeout))
			pr_err("%s (%d) timed out\n", __func__, command);

		return;
	}
}

/**
 * pl353_nand_read_buf - read chip data into buffer
 * @mtd:	Pointer to the mtd info structure
 * @buf:	Pointer to the buffer to store read data
 * @len:	Number of bytes to read
 */
static void pl353_nand_read_buf(struct mtd_info *mtd, uint8_t *buf, int len)
{
	struct nand_chip *chip = mtd->priv;
	uint32_t data32 = 0;
	uint16_t data16 = 0;
	uint32_t count  = 0;

	count = len/sizeof(uint32_t);
	while(count>0) {
		data32 = readl(chip->IO_ADDR_R);
		*(buf++) = (uint8_t)(data32 & 0x000000FF);
		*(buf++) = (uint8_t)((data32 & 0x0000FF00)>>8);
		*(buf++) = (uint8_t)((data32 & 0x00FF0000)>>16);
		*(buf++) = (uint8_t)((data32 & 0xFF000000)>>24);
		len -= sizeof(uint32_t);
		count--;
	}

	count = len/sizeof(uint16_t);
	while(count>0) {
		data16 = readw(chip->IO_ADDR_R);
		*(buf++) = (uint8_t)(data16 & 0x00FF);
		*(buf++) = (uint8_t)((data16 & 0xFF00)>>8);
		len -= sizeof(uint16_t);
		count--;
	}

	if (chip->options & NAND_BUSWIDTH_16) {
		while(len>0) {
			data16 = readw(chip->IO_ADDR_R);
			*(buf++) = (uint8_t)(data16 & 0x00FF);
			len -= sizeof(uint8_t);
		}
	} else {
		while(len>0) {
			*(buf++) = readb(chip->IO_ADDR_R);
			len -= sizeof(uint8_t);
		}
	}
}

/**
 * pl353_nand_write_buf - write buffer to chip
 * @mtd:	Pointer to the mtd info structure
 * @buf:	Pointer to the buffer to store read data
 * @len:	Number of bytes to write
 */
static void pl353_nand_write_buf(struct mtd_info *mtd, const uint8_t *buf,
				int len)
{
	int i;
	struct nand_chip *chip = mtd->priv;
	u32 *ptr = (u32*)buf;
	u32 addr = (u32)chip->IO_ADDR_W;
        
	/* clear end cmd until we write the last buffer */
	addr &= ~((u32)((unsigned long)chip->IO_ADDR_W & PL353_NAND_END_CMD_VALID));
	len >>=2;
	for (i = 0; i < len; i++) {
		if (i == (len-1))
			addr = (u32)chip->IO_ADDR_W;

		writel(ptr[i], (void __iomem*)addr);
	}
}

struct device_driver* s_driver = NULL;

char szMTDName[] = "mtd";
/**
 * @brief nand device probe.
 *
 * @param dev -matching device
 *
 * @return -failure reason or give 0
 */
static int smc35x_nand_probe(struct platform_device *dev)
{
	struct pl353_nand_info *nand;
	struct mtd_info *mtd;
	struct nand_chip *nand_chip;
	struct resource *r;
	int ret = -ENOMEM;
	struct mtd_part_parser_data pdata;

	nand = kzalloc(sizeof(*nand), GFP_KERNEL);
	if (!nand)
		return ret;

	/* Map physical address of NAND flash */
	r = platform_get_resource(dev, IORESOURCE_MEM, 0);
	if (!(request_mem_region(r->start,(r->end - r->start), NULL)))
		goto freemem;
	else if (!(nand->nand_base = ioremap(r->start, (r->end - r->start))))
		goto freemem;

	of_property_read_u32(dev->dev.of_node, "if", &nand->ifc);
	of_property_read_u32(dev->dev.of_node, "cs-addr", &nand->cs_addr);
	nand->pdev = dev->dev.parent;

	/* Link the private data with the MTD structure */
	mtd = &nand->mtd;
	nand_chip = &nand->chip;
	nand_chip->priv = nand;
	mtd->priv = nand_chip;

	/* Set address of NAND IO lines */
	nand_chip->IO_ADDR_R = nand->nand_base;
	nand_chip->IO_ADDR_W = nand->nand_base;

	/* Set the driver entry points for MTD */
	nand_chip->cmdfunc     = pl353_nand_cmd_function; 
	nand_chip->dev_ready   = pl353_nand_device_ready; /* always required */
	/* overwrite default since chip selection is encoded in addressing phase */
	nand_chip->select_chip = pl353_nand_select_chip;

	/* Buffer read/write routines */
	/* since we are talking using the AXI interface */
	/* 32-bit read/write is required, this is not supported by nand_base */
	nand_chip->read_buf  = pl353_nand_read_buf;
	nand_chip->write_buf = pl353_nand_write_buf;

	/* Set the device option and flash width */
	nand_chip->options = NAND_BUSWIDTH_AUTO;
	nand_chip->bbt_options = NAND_BBT_USE_FLASH;

	/* If we don't set this delay driver sets 20us by default */
	//TODO: check the delay satisfies the chips tR_ECC value 
	nand_chip->chip_delay = 30;

	/* first scan to find the device and get the page size */
	if (nand_scan_ident(mtd, 1, NULL)) {
		dev_dbg(&dev->dev, "nand_scan_ident for NAND failed\n");
		ret = ENXIO;
		goto scanfailed;
	}
	/* enable and configure ecc mode */
	pl353_nand_ecc_init(mtd, dev->dev.of_node);

	if (nand_chip->options & NAND_BUSWIDTH_16)
		smc35x_set_buswidth(dev->dev.parent, nand->cs_addr, PL353_SMC_MEM_WIDTH_16);
	/* second phase scan */
	if (nand_scan_tail(mtd)) {
		dev_dbg(&dev->dev, "nand_scan_tail for NAND failed\n");
		ret = ENXIO;
		goto scanfailed;
	}
	mtd->name = szMTDName;

	pdata.of_node = dev->dev.of_node;
	ret = mtd_device_parse_register(mtd, NULL, &pdata, NULL, 0);
        
	/* next two lines are temporarily only */
	s_nand = nand;
	driver_create_file(s_driver, &driver_attr_ecc_mode);

	dev_info(&dev->dev, "successfully initialized!\n");

	return ret;

scanfailed:
	iounmap(nand->nand_base);
freemem:
	kfree(nand);
	return ret;
}

/** smc35x nand driver -> device registered by platforms */
static __maybe_unused struct of_device_id smc35x_nand_id_table[] = {
	{ .compatible = "arm,smc35x-nand" },
	{ .compatible = "arm,pl353-nand-r2p1" },
	{ /* sentinel */ }
};

static struct platform_driver smc35x_nand_driver = {
	.probe	= smc35x_nand_probe,
	.driver		= {
		.name	= DRIVER_NAME,
		.owner	= THIS_MODULE,
		.of_match_table	= of_match_ptr(smc35x_nand_id_table),
	},
};

static int smc35x_nand_init(void)
{
	pr_info("%s: %s\n", DRIVER_NAME, DRIVER_DESC);
	s_driver = &smc35x_nand_driver.driver;
	return platform_driver_register(&smc35x_nand_driver);
}
module_init(smc35x_nand_init);

MODULE_AUTHOR("Xilinx, Inc.");
MODULE_AUTHOR("Hilscher Gesellschaft fuer Systemautomation mbH");
MODULE_ALIAS("platform:" PL35X_NAND_DRIVER_NAME);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL");

