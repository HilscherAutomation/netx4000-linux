/*
* SD/MMC driver for Hilscher netx4000 based platforms
*
* drivers/mmc/host/sdmmc-netx4000.c
*
* (C) Copyright 2015 Hilscher Gesellschaft fuer Systemautomation mbH
* http://www.hilscher.com
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License as
* published by the Free Software Foundation; version 2 of
* the License.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
*/

#define DRIVER_DESC  "SD/MMC driver for Hilscher netx4000 based platforms"
#define DRIVER_NAME  "sdmmc-netx4000"

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/of.h>
#include <linux/of_dma.h>
#include <linux/mmc/host.h>
#include <linux/irq.h>
#include <linux/scatterlist.h>
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/dma-direction.h>
#include <asm/highmem.h>
#include <mach/hardware.h>
#include <mach/platform.h>
#ifdef CONFIG_CRC7
	#include <linux/crc7.h>
#endif

#define MMC_CMD_APP_CMD 55
#define MMC_CMD_STOP_TRANSMISSION 12

#define FINISH_REQUEST           1
#define WAIT_FOR_DATA_ACCESS_END 2

#define mmc_pdata(host)		host->pdata

/* All errors */
#define SD_INFO2_MASK_ALL_ERR		( MSK_NX4000_SDIO_SD_INFO2_ERR0 \
					| MSK_NX4000_SDIO_SD_INFO2_ERR1 \
					| MSK_NX4000_SDIO_SD_INFO2_ERR2 \
					| MSK_NX4000_SDIO_SD_INFO2_ERR3 \
					| MSK_NX4000_SDIO_SD_INFO2_ERR4 \
					| MSK_NX4000_SDIO_SD_INFO2_ERR5 \
					| MSK_NX4000_SDIO_SD_INFO2_ERR6 \
					| MSK_NX4000_SDIO_SD_INFO2_ILA )

#define DMA_SDBUF_RW_EN 	0x2

/* Register Definitions */
struct netx4000_sdio_reg {
	u32 sd_cmd;           /* command type and response type */
	u32 sd_portsel;       /* port selection of the sd-card  */
	u32 sd_arg0;          /* command argument */
	u32 sd_arg1;          /* command argument */
	u32 sd_stop;          /* en-/disable block counting in case of multiple block transfer */
	u32 sd_seccnt;        /* number of blocks to transfer */
	u32 sd_rsp10;         /* response of the sd-card */
	u32 sd_rsp1;          /*  */
	u32 sd_rsp32;
	u32 sd_rsp3;
	u32 sd_rsp54;
	u32 sd_rsp5;
	u32 sd_rsp76;
	u32 sd_rsp7;
	u32 sd_info1;         /* response info */
	u32 sd_info2;         /* response info */
	u32 sd_info1_mask;    /* en-/disables sd_info1 interrupt */
	u32 sd_info2_mask;    /* en-/disables sd_info2 interrupt */
	u32 sd_clk_ctrl;      /* SDCLK configuration */
	u32 sd_size;          /* transfer data length */
	u32 sd_option;        /* sd-card access control */
	u32 reserved0;
	u32 sd_err_sts1;      /* sd-card error status register */
	u32 sd_err_sts2;      /* sd-card error status register */
	u32 sd_buf0;          /* read/write buffer */
	u32 reserved1;
	u32 sdio_mode;        /* command mode configuration */
	u32 sdio_info1;       /* interrupt flag register */
	u32 sdio_info1_mask;  /* en-/disables sdio_info1 interrupt */
	u32 reserved2[79];
	u32 cc_ext_mode;      /* dma mode configuration */
	u32 reserved3[3];
	u32 soft_rst;         /* software reset register */
	u32 version;          /* version */
	u32 host_mode;        /* host interface mode */
	u32 sdif_mode;        /* sd interface mode */
	u32 reserved4[4];
	u32 ext_swap;         /* swap control register */
	u32 sd_status;
	u32 ext_sdio;         /* en-/disables sdio interrupt */
	u32 ext_wp;           /* write protected state of the extended port1 */
	u32 ext_cd;           /* card detect of port1 */
	u32 ext_cd_dat3;      /* extended card detect */
	u32 ext_cd_mask;      /* interrupt mask register */
	u32 ext_cd_dat3_mask; /* interrupt mask register */
};

struct netx4000_hsmmc_host {
	struct	device				*dev;
	struct	mmc_host			*mmc;
	struct	mmc_request			*mrq;
	struct	mmc_command			*cmd;
	struct	mmc_data			*data;
	struct	clk				*clk;
	struct	clk				*fclk;
	struct	clk				*dbclk;
	struct netx4000_sdio_reg		*reg;
	spinlock_t				irq_lock; /* Prevent races with irq handler */
	int					irq[2];
	int					wake_irq;
	unsigned int				flags;
	struct	omap_hsmmc_platform_data	*pdata;

	int (*card_detect)(struct device *dev);

	u32 sd_info1;
	u32 sd_info2;
	u32 error;
	struct scatterlist *sg;
	uint32_t sg_remaining;
	uint8_t * buf_pointer;
	uint8_t state;
	int card_present;
	uint32_t detect_delay_ms;

	uint32_t can_use_dma;
	uint32_t finsh_request;
	struct dma_chan *dma_chan;
	struct dma_chan *tx_chan;
	struct dma_chan *rx_chan;
	phys_addr_t dma_addr;
};

static void netx4000_hsmmc_handle_irq(struct netx4000_hsmmc_host *host);

static int s_fNextAppCmd          = 0;
static int s_fAutomaticBlockCount = 1;

static int netx4000_hsmmc_card_detect(struct device *dev)
{
	struct netx4000_hsmmc_host *host = dev_get_drvdata(dev);
	return host->card_present;
}

static u32 check_irq_and_error(struct netx4000_hsmmc_host* host)
{
	u32 error = 0;
	u32 value = 0;
	u32 info1_mask = 0;

	/* sd_info1 */
	/* skip card detection since this is handled in separated irq routine */
	info1_mask = ~(MSK_NX4000_SDIO_SD_INFO1_INFO3 | MSK_NX4000_SDIO_SD_INFO1_INFO4 | MSK_NX4000_SDIO_SD_INFO1_INFO8 | MSK_NX4000_SDIO_SD_INFO1_INFO9);
	value = readl(&host->reg->sd_info1);
	if ( (value = (value & info1_mask)) ) {
		writel(~value,&host->reg->sd_info1);
		host->sd_info1 = value;
	}

	/* sd_info2 */
	value = readl(&host->reg->sd_info2);
	if (value) {
		u32 info = value & ~SD_INFO2_MASK_ALL_ERR;
		error = value & SD_INFO2_MASK_ALL_ERR;

		writel(~value,&host->reg->sd_info2);
		host->sd_info2 = info;
		host->error = error;
	}
	return error;
}

/*
 * MMC access IRQ handler
 */
static irqreturn_t netx4000_hsmmc_irq(int irq, void *dev_id)
{
	struct netx4000_hsmmc_host *host = dev_id;

	check_irq_and_error(host);
	/* are there interrupts that reuqire further handling? */
	if (host->sd_info1 || host->sd_info2) {
		netx4000_hsmmc_handle_irq(host);
	}
	return IRQ_HANDLED;
}

/*
 * MMC cd IRQ handler
 */
static irqreturn_t netx4000_cd_irq(int irq, void *dev_id)
{
	struct netx4000_hsmmc_host *host = dev_id;
	u32 value = 0;

	/* sd_info1 */
	value = readl(&host->reg->sd_info1);
	value &= (MSK_NX4000_SDIO_SD_INFO1_INFO3 | MSK_NX4000_SDIO_SD_INFO1_INFO4);
	if (value) {
		if (value & MSK_NX4000_SDIO_SD_INFO1_INFO4) {
			writel( ~MSK_NX4000_SDIO_SD_INFO1_INFO4,&host->reg->sd_info1);
			host->card_present = 1;
		}
		if (value & MSK_NX4000_SDIO_SD_INFO1_INFO3) {
			writel( ~MSK_NX4000_SDIO_SD_INFO1_INFO3, &host->reg->sd_info1);
			host->card_present = 0;
		}
		mmc_detect_change( host->mmc, host->detect_delay_ms);
	}
	/* also clear cd-dat3 */
	writel( ~(MSK_NX4000_SDIO_SD_INFO1_INFO8 | MSK_NX4000_SDIO_SD_INFO1_INFO9),&host->reg->sd_info1);

	return IRQ_HANDLED;
}

static void netx4000_hsmmc_enable_irq(struct netx4000_hsmmc_host *host)
{
	writel(0x00000000, &host->reg->sd_info1_mask);
	writel(0x00000800, &host->reg->sd_info2_mask);
}

static void netx4000_hsmmc_disable_irq(struct netx4000_hsmmc_host *host)
{
	/* disable everything but card detection */
	uint32_t info1_mask = 0x0000031D & ~ (MSK_NX4000_SDIO_SD_INFO1_INFO3 | MSK_NX4000_SDIO_SD_INFO1_INFO4 | MSK_NX4000_SDIO_SD_INFO1_INFO8 | MSK_NX4000_SDIO_SD_INFO1_INFO9);

	writel(info1_mask, &host->reg->sd_info1_mask);
	writel(0x00008B7F, &host->reg->sd_info2_mask);
}

static void netx4000_hsmmc_set_clock(struct netx4000_hsmmc_host *host)
{
	u32 clock_setting = 0;
	u32 div;
	unsigned long max = host->mmc->f_max;
	u32 clock = host->mmc->ios.clock;

	/* do not write to clk-ctrl while SCLKDIVEN is not set */
	while ( !(MSK_NX4000_SDIO_SD_INFO2_SCLKDIVEN & readl(&host->reg->sd_info2)) ){ndelay(1);};
	/* disable clock */
	writel( clock_setting, &host->reg->sd_clk_ctrl);
	/* calculate divider */
	if (clock>=max) {
		writel( 0xFF, &host->reg->sd_clk_ctrl);
	} else {
		div = 2;
		while (div < 1024) {
			if ((max/div)<=clock)
				break;

			div <<= 1;
		}
		/* shift for register */
		/* clear clock settings */
		clock_setting &= ~0xFF;
		clock_setting |= MSK_NX4000_SDIO_SD_CLK_CTRL_DIV & (div >> 2);
	}
	clock_setting |= MSK_NX4000_SDIO_SD_CLK_CTRL_SD_CLK_EN; /* enable clock */

	/* do not write to clk-ctrl while SCLKDIVEN is not set */
	while ( !(MSK_NX4000_SDIO_SD_INFO2_SCLKDIVEN & readl(&host->reg->sd_info2)) ){ndelay(1);};
	/* set new rate */
	writel( clock_setting | MSK_NX4000_SDIO_SD_CLK_CTRL_SD_CLK_OFFEN, &host->reg->sd_clk_ctrl);
}

static void netx4000_hsmmc_set_bus_width(struct netx4000_hsmmc_host *host)
{
	struct mmc_ios *ios = &host->mmc->ios;
	u32 val = 0;

	val = readl(&host->reg->sd_option);
	switch (ios->bus_width) {
		case MMC_BUS_WIDTH_4:
			val &= ~(1 << SRT_NX4000_SDIO_SD_OPTION_WIDTH);
			break;
		case MMC_BUS_WIDTH_1:
			val |= (1 << SRT_NX4000_SDIO_SD_OPTION_WIDTH);
			break;
		case MMC_BUS_WIDTH_8:/* check if supported */
		default:
			break;
	}
	writel( val, &host->reg->sd_option);
}

static int get_resp(struct netx4000_hsmmc_host *host, struct mmc_command *cmd)
{
	struct netx4000_sdio_reg __iomem *regs = host->reg;

	/* ----- Select RESP Register Depend on the Response Type ----- */
	switch(mmc_resp_type(cmd)) {
		/* No Response */
		case MMC_RSP_NONE:
			break;
		case MMC_RSP_R2:
		{/* CID (->CMD2,CMD10) / CSD (->CMD9) */
			uint8_t CRC = 1;

			/* read all register and shift 8bit < */
			cmd->resp[3] = readl(&regs->sd_rsp10);
			cmd->resp[2] = readl(&regs->sd_rsp32);
			cmd->resp[1] = readl(&regs->sd_rsp54);
			cmd->resp[0] = readl(&regs->sd_rsp76);

			/* shifting and crc calc */
 			cmd->resp[0] = (cmd->resp[0] << 8);
 			cmd->resp[0] |= 0xFF & (cmd->resp[1] >> 24);
 			cmd->resp[1]  = (cmd->resp[1] << 8);
 			cmd->resp[1] |= 0xFF & (cmd->resp[2] >> 24);
 			cmd->resp[2]  = (cmd->resp[2] << 8);
 			cmd->resp[2] |= 0xFF & (cmd->resp[3] >> 24);
 			cmd->resp[3]  = (cmd->resp[3] << 8);
#ifdef CONFIG_CRC7
			CRC = crc7_be(0, (uint8_t*)cmd->resp, 16) | 0x01;
#endif
 			cmd->resp[3] |= 0xFF & CRC;
			break;
		}
			/* Nomal Response (32bits Length) */
		case MMC_RSP_R1:
			//TODO: fix MMC_RSP_R1b
		//case MMC_RSP_R1b: /* Normal Response with an Optional Busy Signal */
		case MMC_RSP_R3: /* OCR Register (32bits Length) */
			/* MMC_RSP_R4 MMC_RSP_R5 MMC_RSP_R6 MMC_RSP_R7 same format */
			cmd->resp[0] = readl(&regs->sd_rsp10);
			break;
		default:
			/* unknown type */
			break;
	}
	return 0;
}

static int read_data( struct netx4000_hsmmc_host *host, uint8_t *buff, long num)
{
	uint32_t data32 = 0;
	uint32_t count = num/sizeof(uint32_t);
	while(count>0) {
		data32 = readl(&host->reg->sd_buf0);
		*(buff++) = (uint8_t) (data32 & 0x000000FF);
		*(buff++) = (uint8_t)((data32 & 0x0000FF00)>> 8);
		*(buff++) = (uint8_t)((data32 & 0x00FF0000)>>16);
		*(buff++) = (uint8_t)((data32 & 0xFF000000)>>24);
		num -= sizeof(uint32_t);
		count--;
	}
	if(num>0) {
		int i;
		data32 = readl(&host->reg->sd_buf0);
		for(i=0;i<num;i++) {
			*(buff++) = data32 & 0xFF;
			data32    = (data32 >> (8*i));
		}
	}
	return 0;
}

static int write_data( struct netx4000_hsmmc_host *host, const uint8_t *buff, long num)
{
	uint32_t data32 = 0;
	uint32_t count = num/sizeof(uint32_t);
	while(count>0) {
		data32  = (*buff++);
		data32 |= (*buff++ <<  8);
		data32 |= (*buff++ << 16);
		data32 |= (*buff++ << 24);
		writel(data32, &host->reg->sd_buf0);
		num -= sizeof(uint32_t);
		count--;
	}
	if(num>0) {
		int i;
		data32 = 0;
		for(i=0;i<num;i--) {
			data32 |= (*buff++ << (8*i));
		}
		writel( data32, &host->reg->sd_buf0);
	}
	return 0;

}

void release_buffer(struct netx4000_hsmmc_host *host)
{
	if (PageHighMem(sg_page(host->sg))) {
		__kunmap_atomic(host->buf_pointer);
	}
}

uint8_t* get_buffer_addr(struct netx4000_hsmmc_host *host)
{
	if (PageHighMem(sg_page(host->sg))) {
		return kmap_atomic(sg_page(host->sg));
	} else {
		return sg_virt(host->sg);
	}
}

uint8_t* set_new_host_sg(struct netx4000_hsmmc_host *host, struct scatterlist *sg)
{
	if (sg) {
		host->sg = sg;
		host->sg_remaining = host->sg->length;
	}
	return host->buf_pointer;
}

uint8_t* get_next_buffer(struct netx4000_hsmmc_host *host)
{
	host->buf_pointer = get_buffer_addr(host);
	return (host->buf_pointer + host->sg->length - host->sg_remaining);
}


static int transfer_data( struct netx4000_hsmmc_host *host, struct mmc_data *data)
{
	uint8_t *buf = get_next_buffer(host);
	long size = 0;
	int err = 0;

	size = data->blksz;
	if ((buf != NULL)) {
		if (data->flags & MMC_DATA_READ) {
			/* read to SD_BUF */
			if(read_data(host, buf, size) != 0){
				err = -EIO;
			}
		} else {
			/* write to SD_BUF */
			if(write_data(host, buf, size) != 0){
				err = -EIO;
			}
		}
		host->sg_remaining -= size;
		release_buffer(host);
	}
	data->bytes_xfered += size;
	return err;
}

static int handle_stop_cmd( struct netx4000_hsmmc_host *host)
{
	struct netx4000_sdio_reg __iomem *regs = host->reg;
	/* signal transfer stop */
	writel( 1, &regs->sd_stop);
	return 0;
}

void netx4000_hsmmc_finish_request(struct netx4000_hsmmc_host *host, int force)
{
	struct mmc_request *mrq = host->mrq;
	unsigned long flags;

	spin_lock_irqsave(&host->irq_lock, flags);

	/* if data is transfered via DMA and callback is registered, 
	 * wait for DMA callback AND sdio irq to finish request */
	if (force || host->finsh_request) {
		host->finsh_request = 0;
		if (mrq->data && host->can_use_dma) {
			if (force || mrq->cmd->error)
				dmaengine_terminate_all(host->dma_chan);

			/* unmap sgl */
			dma_unmap_sg(host->dma_chan->device->dev,
				host->mrq->data->sg, host->mrq->data->sg_len,
				host->mrq->data->flags & MMC_DATA_WRITE ? DMA_TO_DEVICE : DMA_FROM_DEVICE);
		}
		host->mrq = NULL;
		host->sg_remaining = 0;
		spin_unlock_irqrestore(&host->irq_lock, flags);

		mmc_request_done(host->mmc, mrq);
	} else {
		/* next call to netx4000_hsmmc_finish_request() signal end of transfer */
		host->finsh_request = 1;
		spin_unlock_irqrestore(&host->irq_lock, flags);
	}
}

static void dma_done(struct netx4000_hsmmc_host *host);

static void netx4000_hsmmc_handle_irq(struct netx4000_hsmmc_host *host)
{
	struct mmc_request *mrq = host->mrq;
	struct mmc_data *data   = NULL;

	if (mrq)
		data = mrq->data;

	if (host->error == 0) {
		if (host->sd_info1 & MSK_NX4000_SDIO_SD_INFO1_INFO0) { /* RESPONSE_END */
			get_resp(host, mrq->cmd);
			host->sd_info1 &= ~MSK_NX4000_SDIO_SD_INFO1_INFO0;
		}
		if (host->sd_info1 & MSK_NX4000_SDIO_SD_INFO1_INFO2 /* ACCESS END */) {
			data = NULL;
			host->sd_info1 &=  ~MSK_NX4000_SDIO_SD_INFO1_INFO2;
			host->state = FINISH_REQUEST;
		}
		/* only transfer it if dma is not used! */
		if (data) {
			if (host->can_use_dma) {
				/* NOTE: in case of enabled callback dma_done will be called from dmaengine */
// 				if (host->state == FINISH_REQUEST) {
// 					dma_done(host);
// 				}
			} else {
				if (host->sd_info2 & (MSK_NX4000_SDIO_SD_INFO2_BRE | MSK_NX4000_SDIO_SD_INFO2_BWE)) {
					host->sd_info2 &= ~(MSK_NX4000_SDIO_SD_INFO2_BRE | MSK_NX4000_SDIO_SD_INFO2_BWE);
					if (transfer_data(host, data)) {
						mrq->cmd->error = -EIO;
						host->state = FINISH_REQUEST;
						//TODO:abort transfer
						handle_stop_cmd(host);
					}
				}
			}
		}
	} else {
		host->error = 0;
		/* in case of an error, stop transmission and return error*/
		if (data)
			handle_stop_cmd(host);

		mrq->cmd->error = -EIO;
		host->state = FINISH_REQUEST;
	}
	if (host->state == FINISH_REQUEST) {
		if (mrq)
			netx4000_hsmmc_finish_request(host,(mrq->cmd->error) ? 1:0);
	}
}

static void dma_done(struct netx4000_hsmmc_host *host)
{
	/* If we got to the end of DMA, assume everything went well */
	host->mrq->cmd->error = 0;
	host->mrq->data->bytes_xfered += host->mrq->data->blocks * host->mrq->data->blksz;
}

static void netx4000_hsmmc_dma_callback(void *priv)
{
	struct netx4000_hsmmc_host *host = priv;

	dma_done(host);
	netx4000_hsmmc_finish_request(host,0);
}

struct dma_chan * prepare_dma_buffer( struct netx4000_hsmmc_host *host, struct mmc_data *data)
{
	struct dma_async_tx_descriptor *desc;
	struct dma_slave_config cfg;
	uint32_t len;

	if (data->flags & MMC_DATA_WRITE) {
		host->dma_chan = host->tx_chan;
		cfg.direction = DMA_MEM_TO_DEV;
		
	} else {
		host->dma_chan = host->rx_chan;
		cfg.direction = DMA_DEV_TO_MEM;
	}
	/* TODO: slave configuration should be done once if it does not change */
	cfg.src_addr = host->dma_addr;
	cfg.dst_addr = host->dma_addr;
	cfg.src_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
	cfg.dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
	cfg.src_maxburst = 128;//1;//128;/* no bursts */
	cfg.dst_maxburst = 128;//1;//128;/* no bursts */

	if (dmaengine_slave_config(host->dma_chan, &cfg))
		printk(KERN_ERR "Error dmaengine slave config\n");

	len = dma_map_sg(host->dma_chan->device->dev, data->sg, data->sg_len,
					data->flags & MMC_DATA_WRITE ? DMA_MEM_TO_DEV : DMA_DEV_TO_MEM);

	desc = dmaengine_prep_slave_sg(host->dma_chan, data->sg, len,
		data->flags & MMC_DATA_WRITE ? DMA_MEM_TO_DEV : DMA_DEV_TO_MEM, DMA_PREP_INTERRUPT | DMA_CTRL_ACK);
	if (!desc) {
		printk(KERN_ERR "failedd dmaengine_prep_slave_sg\n");
		dma_unmap_sg(host->dma_chan->device->dev,
				data->sg, data->sg_len,
				data->flags & MMC_DATA_WRITE ? DMA_TO_DEVICE : DMA_FROM_DEVICE);
		return NULL;
	} else {
		/* set flags to DMA_PREP_INTERRUPT to get a callback (dmaengine_prep_slave_sg) */
		desc->callback = netx4000_hsmmc_dma_callback;
		desc->callback_param = host;
		/* NOTE: in case of enabled callback set finsh_request to 0, this prevents irq handler from 
		 *       finishing mrq imediately since we need to wait also for dma callback */
		host->finsh_request = 0;
		dmaengine_submit(desc);

		return host->dma_chan;
	}
}

/*
 * Configure the response type and send the cmd.
 */
static void netx4000_hsmmc_start_command(struct netx4000_hsmmc_host *host, struct mmc_request *req)
{
	struct netx4000_sdio_reg __iomem *regs = host->reg;
	struct mmc_command *cmd = req->cmd;
	struct mmc_data *data = req->data;
	u32 cmd_tmp = 0;
	struct dma_chan *chan = NULL;

	/* wait for idle */
	while(host->sd_info2 & MSK_NX4000_SDIO_SD_INFO2_CBSY){}

	netx4000_hsmmc_disable_irq(host);

	/* handle stop command separate */
	if (cmd->opcode == MMC_CMD_STOP_TRANSMISSION) {
		if (s_fAutomaticBlockCount) {
			mmc_request_done(host->mmc, host->mrq);
			return;/* stop command is handle by hardware -> so return */
		} else {
			handle_stop_cmd( host);
			mmc_request_done(host->mmc, host->mrq);
			return;
		}
	}

	cmd_tmp = (cmd->opcode & MSK_NX4000_SDIO_SD_CMD_CF);
	if (cmd->opcode == MMC_CMD_APP_CMD)
		s_fNextAppCmd = 1;

	/* handle application commands (followed of CMD55 -> s_fNextAppCmd = 1) */
	switch (cmd->opcode)
	{
		case 6:
		case 13:
		case 22:
		case 23:
		case 41:
		case 42:
		case 51:
			if (s_fNextAppCmd) {
				s_fNextAppCmd = 0;
				cmd_tmp |= (1<<6);
				break;
			}
		default:
			break;
	}
	host->state = FINISH_REQUEST;
	/* this is required for dma */
	host->finsh_request = 1;
	/* build command */
	if (data) {/* additional data transmitted */
		/* init miter */
		if (host->can_use_dma) {
			/* setup dma list */
			chan = prepare_dma_buffer(host, data);
		}
		//TODO: in case prepare_dma_buffer fails (chan=NULL) try without dma  
		set_new_host_sg(host, data->sg);

		host->state = WAIT_FOR_DATA_ACCESS_END;

		cmd_tmp |= 1 << SRT_NX4000_SDIO_SD_CMD_MD3;

		if (MMC_DATA_READ & data->flags)
			cmd_tmp |= 1 << SRT_NX4000_SDIO_SD_CMD_MD4;

		/* disable block counting (sends stop command after all blocks are transferred) */
		writel(data->blksz, &regs->sd_size);
		/* not required in since automatic mode is disabled */
		writel( data->blocks, &regs->sd_seccnt); /* deliver number of blocks */
		if (s_fAutomaticBlockCount)
			writel( (1<<8), &regs->sd_stop);/* enable automatic block count */
		else
			writel( 0, &regs->sd_stop);/* disable */

		if (data->blocks>1) {
			cmd_tmp |= MSK_NX4000_SDIO_SD_CMD_MD5; /* enable multiple block mode */
			if (!s_fAutomaticBlockCount)
				cmd_tmp |= MSK_NX4000_SDIO_SD_CMD_MD_MLT_BLK;/* stop command required */
		}
	}
	host->mrq = req;
	writel( cmd->arg, &regs->sd_arg0);
	/* issue command */
	writel(cmd_tmp, &regs->sd_cmd);
	netx4000_hsmmc_enable_irq(host);
	
	if (host->can_use_dma && chan)
		dma_async_issue_pending(chan);
}

/*
 * Request function. for read/write operation
 */
static void netx4000_hsmmc_request(struct mmc_host *mmc, struct mmc_request *req)
{
	struct netx4000_hsmmc_host *host = mmc_priv(mmc);

	if (!host->card_present) {
		req->cmd->error = -ENOMEDIUM;
		goto request_done;
	}

	//spin_lock_irqrestore(&host->irq_lock, flags);
	//TODO:lock
	while(host->mrq){}
	//TODO:lock
	//spin_unlock_irqrestore(&host->irq_lock, flags);

	netx4000_hsmmc_start_command(host, req);
	return;

request_done:
	mmc_request_done(host->mmc, req);
}

/* Routine to configure clock values. Exposed API to core */
static void netx4000_hsmmc_set_ios(struct mmc_host *mmc, struct mmc_ios *ios)
{
	struct netx4000_hsmmc_host *host = mmc_priv(mmc);

	if (!host->card_present)
		return;

	//TODO: power/voltage handling...

	/* FIXME: set registers based only on changes to ios */
	netx4000_hsmmc_set_bus_width(host);
	netx4000_hsmmc_set_clock(host);

	//netx4000_hsmmc_set_bus_mode(host);
}

static int netx4000_hsmmc_get_cd(struct mmc_host *mmc)
{
	struct netx4000_hsmmc_host *host = mmc_priv(mmc);

	if (!host->card_present)
		return -ENOMEDIUM;

	if (!host->card_detect)
		return -ENOSYS;
	return host->card_detect(host->dev);
}

static void netx4000_hsmmc_init_card(struct mmc_host *mmc, struct mmc_card *card)
{
	//TODO:
}

static int netx4000_gpio_get_ro(struct mmc_host *mmc)
{
	return 0;
}

static void netx4000_sdmmc_hardware_reset(struct mmc_host *mmc)
{
	struct netx4000_hsmmc_host *host = mmc_priv(mmc);
	u32 tmpreg = 0;

	/* reset */
	writel( 0, &host->reg->soft_rst);
	ndelay(50);
	writel( 1, &host->reg->soft_rst);

	/* clear all interrupts */
	writel( 0x0000031D, &host->reg->sd_info1_mask); /* disable all interrupts */
	writel( 0x00008B7F, &host->reg->sd_info2_mask); /* disable all interrupts */
	writel( 0x0000C007, &host->reg->sdio_info1_mask); /* disable all interrupts */

	/* initialize all register */
	writel( 0x00000000, &host->reg->cc_ext_mode);
	/* enable DMA if requested */
	if (host->can_use_dma)
		writel( DMA_SDBUF_RW_EN, &host->reg->cc_ext_mode);
	writel( 0x00000000, &host->reg->sdif_mode);
	writel( 0x00000000, &host->reg->host_mode);/* 32-bit access */
	writel( 0x00000000, &host->reg->sdio_mode);
	writel( 0x00000000, &host->reg->ext_swap);
	writel( 0x00000000, &host->reg->sd_portsel);

	/* reset pending infos */
	tmpreg = readl( &host->reg->sd_info1);
	tmpreg |= ~(1|4);
	writel( tmpreg, &host->reg->sd_info1);
	writel( 0x00000000, &host->reg->sd_info2);
	writel( 0x00000000, &host->reg->sdio_info1);
	writel( 0x0000C0EE, &host->reg->sd_option);/* max. timeout */

	netx4000_hsmmc_disable_irq(host);

	/* update card state since we cleared all interrupts */
	host->card_present = 0;
	tmpreg = readl(&host->reg->sd_info1);
	if (tmpreg & MSK_NX4000_SDIO_SD_INFO1_INFO5)
		host->card_present = 1;
}

static struct mmc_host_ops netx4000_hsmmc_ops = {
	.request = netx4000_hsmmc_request,
	.set_ios = netx4000_hsmmc_set_ios,
	.get_cd = netx4000_hsmmc_get_cd,
	.get_ro = netx4000_gpio_get_ro,
	.hw_reset = netx4000_sdmmc_hardware_reset,
	.init_card = netx4000_hsmmc_init_card,
};

static int netx4000_hsmmc_probe(struct platform_device *pdev)
{
	struct omap_hsmmc_platform_data *pdata = pdev->dev.platform_data;
	struct mmc_host *mmc;
	struct netx4000_hsmmc_host *host = NULL;
	struct resource *res;
	int ret, irq_cd,irq_access;
	void __iomem *base;
	struct device_node *np = pdev->dev.of_node;
        
        netx4000_periph_clock_enable(NETX4000_USB_SDIO_EN);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);

	irq_cd = platform_get_irq_byname(pdev, "card");
	irq_access = platform_get_irq_byname(pdev, "access");
	if ((res == NULL) || (irq_cd < 0) || (irq_access < 0))
		return -ENXIO;

	base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(base))
		return PTR_ERR(base);

	mmc = mmc_alloc_host(sizeof(struct netx4000_hsmmc_host), &pdev->dev);
	if (!mmc) {
		ret = -ENOMEM;
		goto err;
	}
	ret = mmc_of_parse(mmc);
	if (ret)
		goto err1;

	mmc_of_parse_voltage(np, &mmc->ocr_avail);
	mmc->ops = &netx4000_hsmmc_ops;
	//TODO: limits??
	mmc->max_segs = 1024;
	mmc->max_blk_size = 512;       /* Block Length at max can be 1024 */
	/* this must be 8 for not using DMA (see buffer managment/mapping) in case of DMA this will be 0xFFFF */
	mmc->max_blk_count = 8; /* No. of Blocks is 16 bits */
	mmc->max_req_size = mmc->max_blk_size * mmc->max_blk_count;
	mmc->max_seg_size = mmc->max_req_size;
	mmc->f_max = clk_get_rate(of_clk_get(np,0));

	mmc->caps = 0;
	mmc->caps |= MMC_CAP_MMC_HIGHSPEED | MMC_CAP_SD_HIGHSPEED | MMC_CAP_WAIT_WHILE_BUSY | MMC_CAP_ERASE;
	mmc->caps |= MMC_CAP_4_BIT_DATA | MMC_CAP_HW_RESET;

	host		= mmc_priv(mmc);
	host->mmc	= mmc;
	host->pdata	= pdata;
	host->dev	= &pdev->dev;
	host->irq[0]	= irq_cd;
	host->irq[1]	= irq_access;
	host->dma_addr  = (phys_addr_t)(res->start + (uint32_t)&((struct netx4000_sdio_reg*)(NULL))->sd_buf0);
	host->reg	= base;
	host->card_detect = netx4000_hsmmc_card_detect;

	of_property_read_u32(np, "card-detect-delay", &host->detect_delay_ms);

	platform_set_drvdata(pdev, host);

	spin_lock_init(&host->irq_lock);

	host->can_use_dma = 0;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 5, 0) 
	host->rx_chan = dma_request_chan(&pdev->dev, "rx");
	if (IS_ERR(host->rx_chan)) {
		dev_err(mmc_dev(host->mmc), "RX DMA channel request failed - not using DMA!\n");
		ret = PTR_ERR(host->rx_chan);
		goto skip_dma;
	}
	host->tx_chan = dma_request_chan(&pdev->dev, "tx");
	if (IS_ERR(host->tx_chan)) {
		dev_err(mmc_dev(host->mmc), "TX DMA channel request failed - not using DMA!\n");
		ret = PTR_ERR(host->tx_chan);
		goto skip_dma;
	}
#else
	host->rx_chan = of_dma_request_slave_channel(np, "rx");
	if (IS_ERR(host->rx_chan)) {
		dev_err(mmc_dev(host->mmc), "unable to obtain RX DMA engine channel - not using DMA!\n");
		ret = -ENXIO;
		goto skip_dma;
	}
	host->tx_chan = of_dma_request_slave_channel(np, "tx");
	if (IS_ERR(host->tx_chan)) {
		dev_err(mmc_dev(host->mmc), "unable to obtain TX DMA engine channel - not using DMA!\n");
		dma_release_channel(host->rx_chan);
		ret = -ENXIO;
		goto skip_dma;
	}
#endif
	/* enable DMA usage */
	host->can_use_dma = 1;
	mmc->max_blk_count = 0xFFFF; /* No. of Blocks is 16 bits */
	mmc->max_req_size = mmc->max_blk_size * mmc->max_blk_count;
	mmc->max_seg_size = mmc->max_req_size;
skip_dma:
	/* Request IRQ for MMC operations */
	ret = devm_request_irq(&pdev->dev, host->irq[0], netx4000_cd_irq, 0,
			mmc_hostname(mmc), host);
	if (ret) {
		dev_err(mmc_dev(host->mmc), "Unable to grab HSMMC IRQ\n");
		goto err_irq;
	}
	ret = devm_request_irq(&pdev->dev, host->irq[1], netx4000_hsmmc_irq, 0,
			       mmc_hostname(mmc), host);
	if (ret) {
		dev_err(mmc_dev(host->mmc), "Unable to grab HSMMC IRQ\n");
		goto err_irq;
	}
	netx4000_sdmmc_hardware_reset(host->mmc);

	ret = mmc_add_host(mmc);

	dev_info(&pdev->dev, "successfully initialized!\n");

	return 0;
err_irq:
	if (host->dbclk)
		clk_disable_unprepare(host->dbclk);
	if (!IS_ERR(host->tx_chan))
		dma_release_channel(host->tx_chan);
	if (!IS_ERR(host->rx_chan))
		dma_release_channel(host->rx_chan);
err1:
	mmc_free_host(mmc);
err:
	return ret;
}

static int netx4000_hsmmc_remove(struct platform_device *pdev)
{
	struct netx4000_hsmmc_host *host = platform_get_drvdata(pdev);

	if (!IS_ERR(host->tx_chan))
		dma_release_channel(host->tx_chan);
	if (!IS_ERR(host->rx_chan))
		dma_release_channel(host->rx_chan);

	mmc_remove_host(host->mmc);

	if (host->dbclk)
		clk_disable_unprepare(host->dbclk);

	mmc_free_host(host->mmc);

	dev_info(&pdev->dev, "successfully removed!\n");

	return 0;
}

static const struct of_device_id netx4000_mmc_of_match[] = {
	{
		.compatible = "hilscher,netx4000-sdio",
	},
	{},
};
MODULE_DEVICE_TABLE(of, netx4000_mmc_of_match);

static struct platform_driver netx4000_hsmmc_driver = {
	.probe		= netx4000_hsmmc_probe,
	.remove		= netx4000_hsmmc_remove,
	.driver		= {
		.name = DRIVER_NAME,
		.of_match_table = of_match_ptr(netx4000_mmc_of_match),
	},
};

static int __init netx4000_hsmmc_init(void)
{
	pr_info("%s: %s\n", DRIVER_NAME, DRIVER_DESC);
	return platform_driver_register(&netx4000_hsmmc_driver);
}
module_init(netx4000_hsmmc_init);

MODULE_AUTHOR("Hilscher Gesellschaft fuer Systemautomation mbH");
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL v2");

