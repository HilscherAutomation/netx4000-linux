/*
* I2C bus driver for Hilscher netx4000 based platforms
*
* drivers/i2c/busses/i2c-netx4000.c
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

#define DRIVER_DESC  "I2C bus driver for Hilscher netx4000 based platforms"
#define DRIVER_NAME "i2c-netx4000"

/* --- Includes --- */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/jiffies.h>

#include <linux/of_irq.h>
#include <linux/of_address.h>

/* --- Chip definitions --- */

/* Register offsets */
#define I2C_MCR      0x00 /* I2C master control register */
#define I2C_SCR      0x04 /* I2C slave control register */
#define I2C_CMD      0x08 /* I2C master command register */
#define I2C_MDR      0x0c /* I2C master data register (master FIFO) */
#define I2C_SDR      0x10 /* I2C slave data register (slave FIFO) */
#define I2C_MFIFO_CR 0x14 /* I2C master FIFO control register */
#define I2C_SFIFO_CR 0x18 /* I2C slave FIFO control register */
#define I2C_SR       0x1c /* I2C status register */
#define I2C_IRQMSK   0x20 /* I2C interrupt mask set or clear register */
#define I2C_IRQSR    0x24 /* I2C interrupt state register (raw interrupt before masking) */
#define I2C_IRQMSKED 0x28 /* I2C masked interrupt state register */
#define I2C_DMACR    0x2c /* I2C DMA control register */
#define I2C_PIO      0x30 /* Direct I2C IO controlling */

/* I2C master control register (i2c_mcr) */
#define MCR_EN_TIMEOUT (1 << 18)
#define MCR_RST_I2C    (1 << 17)
#define MCR_PIO_MODE   (1 << 16)
#define MCR_SADR_SHIFT 4
#define MCR_SADR_MASK  (0x7f << MCR_SADR_SHIFT)
#define MCR_MODE_SHIFT 1
#define MCR_MODE_MASK  (0x7 << MCR_MODE_SHIFT)
#define MCR_MODE_50K   (0b000 << MCR_MODE_SHIFT) /* Fast/Standard, 50kbit/s */
#define MCR_MODE_100K  (0b001 << MCR_MODE_SHIFT) /* Fast/Standard, 100kbit/s */
#define MCR_MODE_200K  (0b010 << MCR_MODE_SHIFT) /* Fast/Standard, 200kbit/s */
#define MCR_MODE_400K  (0b011 << MCR_MODE_SHIFT) /* Fast/Standard, 400kbit/s */
#define MCR_MODE_800K  (0b100 << MCR_MODE_SHIFT) /* High-speed, 800kbit/s */
#define MCR_MODE_1200K (0b101 << MCR_MODE_SHIFT) /* High-speed, 1.2Mbit/s */
#define MCR_MODE_1700K (0b110 << MCR_MODE_SHIFT) /* High-speed, 1.7Mbit/s */
#define MCR_MODE_3400K (0b111 << MCR_MODE_SHIFT) /* High-speed, 3.4Mbit/s) */
#define MCR_EN_I2C     (1 << 0)

/* I2C slave control register (i2c_scr) */
#define SCR_AUTORESET_AC_START (1 << 20)
#define SCR_AC_GCALL           (1 << 18)
#define SCR_AC_START           (1 << 17)
#define SCR_AC_SRX             (1 << 16)
#define SCR_SID10              (1 << 10)
#define SCR_SID_SHIFT          0
#define SCR_SID_MASK           (0x3ff << SCR_SID_SHIFT)

/* I2C master command register (i2c_cmd) */
#define CMD_ACPOLLMAX_SHIFT 20
#define CMD_ACPOLLMAX_MASK (0xff << CMD_ACPOLLMAX_SHIFT)
#define CMD_TSIZE_SHIFT    8
#define CMD_TSIZE_MASK     (0x3ff << CMD_TSIZE_SHIFT)
#define CMD_CMD_SHIFT      1
#define CMD_CMD_MASK       (0x7  << CMD_CMD_SHIFT)
#define CMD_CMD_START      (0b000 << CMD_CMD_SHIFT) /* (r)START-condition */
#define CMD_CMD_S_AC       (0b001 << CMD_CMD_SHIFT) /* + Acknowledge-polling */
#define CMD_CMD_S_AC_T     (0b010 << CMD_CMD_SHIFT) /* + data */
#define CMD_CMD_S_AC_TC    (0b011 << CMD_CMD_SHIFT) /* + data ... */
#define CMD_CMD_CT         (0b100 << CMD_CMD_SHIFT) /* ... data */
#define CMD_CMD_CTC        (0b101 << CMD_CMD_SHIFT) /* ... data ... */
#define CMD_CMD_STOP       (0b110 << CMD_CMD_SHIFT) /* STOP-condition */
#define CMD_CMD_IDLE       (0b111 << CMD_CMD_SHIFT) /* Nothing to do, last cmd finished, break current cmd */
#define CMD_NWR            (1 << 0)

/* I2C master data register (master FIFO) (i2c_mdr) */
#define MDR_MDATA_SHIFT 0
#define MDR_MDATA_MASK  (0xff << MDR_MDATA_SHIFT)

/* I2C slave data register (slave FIFO) (i2c_sdr) */
#define SDR_SDATA_SHIFT 0
#define SDR_SDATA_MASK  (0xff << SDR_SDATA_SHIFT)

/* I2C master FIFO control register (i2c_mfifo_cr) */
#define MFIFO_CR_MFIFO_CLR      (1 << 8)
#define MFIFO_CR_MFIFO_WM_SHIFT 0
#define MFIFO_CR_MFIFO_WM_MASK  (0xf << MFIFO_CR_MFIFO_WM_SHIFT)

/* I2C slave FIFO control register (i2c_sfifo_cr) */
#define SFIFO_CR_SFIFO_CLR      (1 << 8)
#define SFIFO_CR_SFIFO_WM_SHIFT 0
#define SFIFO_CR_SFIFO_WM_MASK  (0xf << SFIFO_CR_SFIFO_WM_SHIFT)

/* I2C status register (i2c_sr) */
#define SR_SDA_STATE         (1 << 31)
#define SR_SCL_STATE         (1 << 30)
#define SR_TIMEOUT           (1 << 28)
#define SR_SID10_ACED        (1 << 27)
#define SR_GCALL_ACED        (1 << 26)
#define SR_NWR_ACED          (1 << 25)
#define SR_LAST_AC           (1 << 24)
#define SR_SLAVE_ACCESS      (1 << 23)
#define SR_STARTED           (1 << 22)
#define SR_NWR               (1 << 21)
#define SR_BUS_MASTER        (1 << 20)
#define SR_SFIFO_ERR_UNDR    (1 << 19)
#define SR_SFIFO_ERR_OVFL    (1 << 18)
#define SR_SFIFO_FULL        (1 << 17)
#define SR_SFIFO_EMPTY       (1 << 16)
#define SR_SFIFO_LEVEL_SHIFT 10
#define SR_SFIFO_LEVEL_MASK  (0x1f << SR_SFIFO_LEVEL_SHIFT)
#define SR_MFIFO_ERR_UNDR    (1 << 9)
#define SR_MFIFO_ERR_OVFL    (1 << 8)
#define SR_MFIFO_FULL        (1 << 7)
#define SR_MFIFO_EMPTY       (1 << 6)
#define SR_MFIFO_LEVEL_SHIFT 0
#define SR_MFIFO_LEVEL_MASK  (0x1f << SR_MFIFO_LEVEL_SHIFT)

/* I2C Common IRQ defines */
#define SREQ     (1 << 6)
#define SFIFO    (1 << 5)
#define MFIFO    (1 << 4)
#define BUS_BUSY (1 << 3)
#define FIFO_ERR (1 << 2)
#define CMD_ERR  (1 << 1)
#define CMD_OK   (1 << 0)

/* I2C interrupt mask set or clear register (i2c_irqmsk) */
#define IRQMSK_SREQ      SREQ
#define IRQMSK_SFIFO_REQ SFIFO
#define IRQMSK_MFIFO_REQ MFIFO
#define IRQMSK_BUS_BUSY  BUS_BUSY
#define IRQMSK_FIFO_ERR  FIFO_ERR
#define IRQMSK_CMD_ERR   CMD_ERR
#define IRQMSK_CMD_OK    CMD_OK
#define IRQMSK_SHIFT     0
#define IRQMSK_MASK      (0x3f << IRQMSK_SHIFT)

/* I2C interrupt state register (raw interrupt before masking) (i2c_irqsr) */
#define IRQSR_SREQ      SREQ
#define IRQSR_SFIFO_REQ SFIFO
#define IRQSR_MFIFO_REQ MFIFO
#define IRQSR_BUS_BUSY  BUS_BUSY
#define IRQSR_FIFO_ERR  FIFO_ERR
#define IRQSR_CMD_ERR   CMD_ERR
#define IRQSR_CMD_OK    CMD_OK
#define IRQSR_SHIFT     0
#define IRQSR_MASK      (0x3f << IRQMSK_SHIFT)

/* I2C masked interrupt state register (i2c_irqmsked) */
#define IRQMSKED_SREQ      SREQ
#define IRQMSKED_SFIFO_REQ SFIFO
#define IRQMSKED_MFIFO_REQ MFIFO
#define IRQMSKED_BUS_BUSY  BUS_BUSY
#define IRQMSKED_FIFO_ERR  FIFO_ERR
#define IRQMSKED_CMD_ERR   CMD_ERR
#define IRQMSKED_CMD_OK    CMD_OK
#define IRQMSKED_SHIFT     0
#define IRQMSKED_MASK      (0x3f << IRQMSK_SHIFT)

/* I2C DMA control register (i2c_dmacr) */
#define DMACR_SDMAB_EN (1 << 3)
#define DMACR_SDMAS_EN (1 << 2)
#define DMACR_MDMAB_EN (1 << 1)
#define DMACR_MDMAS_EN (1 << 0)

/* Direct I2C IO controlling (i2c_pio) */
#define PIO_SDA_IN_RO (1 << 6)
#define PIO_SDA_OE    (1 << 5)
#define PIO_SDA_OUT   (1 << 4)
#define PIO_SCL_IN_RO (1 << 2)
#define PIO_SCL_OE    (1 << 1)
#define PIO_SCL_OUT   (1 << 0)

/* --- Global definitions --- */

#define I2C_TIMEOUT msecs_to_jiffies(1000)

struct netx4000_i2c_pdata {
	void __iomem *base;
	uint32_t bus_clock; /* Hz */
	uint32_t speed_mode; /* chip mode */
	uint32_t acpollmax;
	uint32_t irq;
	uint32_t irqsr;
	uint32_t irq_pended;
	wait_queue_head_t wait_queue;
	struct device *dev;
	struct i2c_adapter adapter;
	struct i2c_msg dup_msg;
	struct i2c_msg *msg;
};

/* --- Macros --- */

#define IOSET32(mask, addr) { uint32_t val; val = readl(addr); writel(val | mask, addr); }
#define IOCLEAR32(mask, addr) { uint32_t val; val = readl(addr); writel(val & ~mask, addr); }
#define IRQENABLE(mask, base) {	uint32_t val; val = readl(base + I2C_IRQMSK); writel(val | (mask << IRQMSK_SHIFT & IRQMSK_MASK), base + I2C_IRQMSK); }
#define IRQDISABLE(mask, base) { uint32_t val; val = readl(base + I2C_IRQMSK); writel(val & ~(mask << IRQMSK_SHIFT & IRQMSK_MASK), base + I2C_IRQMSK); }

/* --- Source code --- */

static int32_t netx4000_fifo_handler(void *dev_id) {
	struct netx4000_i2c_pdata *pdata = dev_id;
	void __iomem *base = pdata->base;
	uint32_t nbytes, wm;

	dev_dbg(pdata->dev, "%s() called\n", __func__);

	if (pdata->msg) {
		if ((pdata->msg->flags & I2C_M_RD) != 0) { /* Read cycle */
			nbytes = (readl(base + I2C_SR) & SR_MFIFO_LEVEL_MASK) >> SR_MFIFO_LEVEL_SHIFT;
			while (nbytes--) {
				*pdata->msg->buf++ = (uint8_t)(readl(base + I2C_MDR) & MDR_MDATA_MASK);
				pdata->msg->len--;
			}
			wm = min(pdata->msg->len, (u16)(16*3/4));
			wm = (wm) ? (wm-1) : (0);
			writel(wm << MFIFO_CR_MFIFO_WM_SHIFT, base + I2C_MFIFO_CR);
		}
		if ((pdata->msg->flags & I2C_M_RD) == 0) {  /* Write cycle */
			nbytes = 16 - ((readl(base + I2C_SR) & SR_MFIFO_LEVEL_MASK) >> SR_MFIFO_LEVEL_SHIFT);
			nbytes = min((u16)nbytes, pdata->msg->len);
			while (nbytes--) {
				writel((uint32_t)*pdata->msg->buf++, base + I2C_MDR);
				pdata->msg->len--;
			}
			wm = min(pdata->msg->len, (u16)(16*3/4-1));
			wm = (wm) ? (16-wm) : (0);
			writel(wm << MFIFO_CR_MFIFO_WM_SHIFT, base + I2C_MFIFO_CR);
		}
	}

	return 0;
}

static irqreturn_t netx4000_i2c_isr(int32_t irq, void *dev_id)
{
	struct netx4000_i2c_pdata *pdata = dev_id;
	void __iomem *base = pdata->base;

	dev_dbg(pdata->dev, "%s() called\n", __func__);

	/* Device IRQ ? */
	if (unlikely(readl(base + I2C_IRQMSKED) == 0))
		return IRQ_NONE;

	if (pdata->irq_pended)
		dev_err(pdata->dev, "%s() - lost IRQ (0x%08x)\n", __func__, pdata->irqsr);

	/* Save pending IRQs */
	pdata->irqsr = readl(base + I2C_IRQSR);

	if (pdata->irqsr & FIFO_ERR) {
		dev_err(pdata->dev, "%s() Unsupported FIFO_ERR IRQ occurred!\n", __func__);
	}
	if (pdata->irqsr & BUS_BUSY) {
		dev_err(pdata->dev, "%s() Unsupported BUS_BUSY IRQ occurred!\n", __func__);
	}
	if (pdata->irqsr & MFIFO) {
		netx4000_fifo_handler(dev_id);
	}
	if (pdata->irqsr & SFIFO) {
		dev_err(pdata->dev, "%s() Unsupported SFIFO IRQ occurred!\n", __func__);
	}
	if (pdata->irqsr & SREQ) {
		dev_err(pdata->dev, "%s() Unsupported SREQ IRQ occurred!\n", __func__);
	}
	if (pdata->irqsr & CMD_OK) {
		pdata->irq_pended = 1;
		wake_up(&pdata->wait_queue);
	}
	if (pdata->irqsr & CMD_ERR) {
		pdata->irq_pended = 1;
		wake_up(&pdata->wait_queue);
	}

	/* Clear pending IRQs */
	writel(pdata->irqsr, base + I2C_IRQSR);

	return IRQ_HANDLED;
}

static int32_t netx4000_i2c_wait_for_idle(struct netx4000_i2c_pdata *pdata)
{
	void __iomem *base = pdata->base;
	int32_t rc = 0;

	dev_dbg(pdata->dev, "%s() called\n", __func__);

	do {
		if (pdata->irq) { /* IRQ */
			rc = wait_event_timeout(pdata->wait_queue, pdata->irq_pended, I2C_TIMEOUT);
			if (likely(rc >= 1)) {
				pdata->irq_pended = 0;
				if (pdata->irqsr & CMD_OK)
					rc = 0;
				else
					rc = -EIO;
			}
			else {
				dev_err(pdata->dev, "%s() - timed out\n", __func__);
				rc = -ETIMEDOUT;
				break;
			}
			break;
		}
		else { /* Polling */
			if (readl(base + I2C_SR) & SR_TIMEOUT) {
				IOSET32(SR_TIMEOUT, base + I2C_SR); /* Clear the timeout flag */
				dev_err(pdata->dev, "%s() - timed out\n", __func__);
				rc = -ETIMEDOUT;
				break;
			}
		}
	} while ((readl(base + I2C_CMD) & CMD_CMD_MASK) != CMD_CMD_IDLE);

	/* Handle raw interrupts */
	if (pdata->irq == 0) {
		pdata->irqsr = readl(base + I2C_IRQSR);
		writel(pdata->irqsr, base + I2C_IRQSR);
	}

	if (pdata->irqsr & BUS_BUSY) {
		rc = -EBUSY;
	}
	else if (pdata->irqsr & CMD_ERR) {
		rc = -EIO;
	}

	if (rc)
		return rc; /* Error */

	return 0; /* Success */
}

static int32_t netx4000_i2c_send(struct netx4000_i2c_pdata *pdata, struct i2c_msg *msg, uint32_t nwr, uint32_t stop)
{
	void __iomem *base = pdata->base;
	uint32_t nbytes = msg->len, cmd, copy;
	unsigned long ts;
	int32_t rc = 0;

	dev_dbg(pdata->dev, "%s() called - addr: 0x%04x, len: %d, nwr: %d, stop: %d, flags: 0x%x\n"
			, __func__, msg->addr, msg->len, nwr, stop, msg->flags);

	if (pdata->irq) { /* IRQ */
		pdata->msg = msg;
		/* Trigger/Enable IRQ */
		writel(MFIFO_CR_MFIFO_WM_MASK << MFIFO_CR_MFIFO_WM_SHIFT, base + I2C_MFIFO_CR);
	}

	/* Prepare and send the command register */
	cmd = ((msg->len - 1) << CMD_TSIZE_SHIFT) | ((stop) ? CMD_CMD_CT : CMD_CMD_CTC) | nwr;
	writel(cmd, base + I2C_CMD);

	if (pdata->irq) { /* IRQ */
		rc = wait_event_timeout(pdata->wait_queue, pdata->irq_pended, I2C_TIMEOUT);
		if (likely(rc >= 1)) {
			pdata->irq_pended = 0;
			if (pdata->irqsr & CMD_OK)
				rc = 0;
			else
				rc = -EIO;
		}
		else {
			dev_err(pdata->dev, "%s() - FIFO timed out\n", __func__);
			rc = -ETIMEDOUT;
		}
		pdata->msg = NULL;
	}
	else { /* Polling */
		/* Copy the data from message buffer to fifo */
		while (nbytes) {
			/* We like to process a data size of 75% of fifo size */
			copy = min(nbytes, (uint32_t)(16*3/4));

			ts = jiffies + I2C_TIMEOUT;
			while (((readl(base + I2C_SR) & SR_MFIFO_LEVEL_MASK) >> SR_MFIFO_LEVEL_SHIFT) > (16 - copy)) {
				if (time_before(jiffies, ts))
					continue;
				dev_err(pdata->dev, "%s() - FIFO timed out\n", __func__);
				rc = -ETIMEDOUT;
				break;
			}

			if (rc)
				break;

			while (copy--) {
				writel((uint32_t)*msg->buf++, base + I2C_MDR);
				nbytes--;
			}
		}
		rc = netx4000_i2c_wait_for_idle(pdata);
	}

	if (rc)
		return rc; /* Error */

	return 0; /* Success */
}

static int32_t netx4000_i2c_recv(struct netx4000_i2c_pdata *pdata, struct i2c_msg *msg, uint32_t nwr, uint32_t stop)
{
	void __iomem *base = pdata->base;
	uint32_t nbytes = msg->len, cmd, copy;
	unsigned long ts;
	int32_t rc = 0;

	dev_dbg(pdata->dev, "%s() called - addr: 0x%04x, len: %d, nwr: %d, stop: %d, flags: 0x%x\n"
			, __func__, msg->addr, msg->len, nwr, stop, msg->flags);

	if (pdata->irq) { /* IRQ */
		pdata->msg = msg;
		/* Trigger/Enable IRQ */
		writel(0 << MFIFO_CR_MFIFO_WM_SHIFT, base + I2C_MFIFO_CR);
	}

	/* Prepare and send the command register */
	cmd = ((msg->len - 1) << CMD_TSIZE_SHIFT) | ((stop) ? CMD_CMD_CT : CMD_CMD_CTC) | nwr;
	writel(cmd, base + I2C_CMD);

	if (pdata->irq) { /* IRQ */
		rc = wait_event_timeout(pdata->wait_queue, pdata->irq_pended, I2C_TIMEOUT);
		if (likely(rc >= 1)) {
			pdata->irq_pended = 0;
			if (pdata->irqsr & CMD_OK)
				rc = 0;
			else
				rc = -EIO;
		}
		else {
			dev_err(pdata->dev, "%s() - FIFO timed out\n", __func__);
			rc = -ETIMEDOUT;
		}
		pdata->msg = NULL;
	}
	else { /* Polling */
		/* Copy the data from fifo to message buffer */
		while (nbytes) {
			/* We like to process a data size of 75% of fifo size */
			copy = min(nbytes, (uint32_t)(16*3/4));

			ts = jiffies + I2C_TIMEOUT;
			while (((readl(base + I2C_SR) & SR_MFIFO_LEVEL_MASK) >> SR_MFIFO_LEVEL_SHIFT) < copy) {
				if (time_before(jiffies, ts))
					continue;
				dev_err(pdata->dev, "%s() - FIFO timed out\n", __func__);
				rc = -ETIMEDOUT;
				break;
			}

			if (rc)
				break;

			while (copy--) {
				*msg->buf++ = (uint8_t)(readl(base + I2C_MDR) & MDR_MDATA_MASK);
				nbytes--;
			}
		}
		rc = netx4000_i2c_wait_for_idle(pdata);
	}

	if (rc)
		return rc; /* Error */

	return 0; /* Success */
}

static int32_t netx4000_i2c_master_xfer(struct i2c_adapter *adapter, struct i2c_msg msgs[], int32_t num)
{
	struct netx4000_i2c_pdata *pdata = i2c_get_adapdata(adapter);
	void __iomem *base = pdata->base;
	uint32_t i, nwr, start, stop;
	int32_t rc = 0;

	dev_dbg(pdata->dev, "%s() called\n", __func__);

	/* Clear master FIFO */
	writel(MFIFO_CR_MFIFO_CLR, base + I2C_MFIFO_CR);

	/* Clear any pending interrupts */
	writel(IRQSR_MASK, base + I2C_IRQSR);

	/* Enable command timeout detection, set the slave address, the mode (speed) and enable I2C master */
	writel(MCR_EN_TIMEOUT | ((msgs[0].addr << MCR_SADR_SHIFT) & MCR_SADR_MASK) | pdata->speed_mode | MCR_EN_I2C, base + I2C_MCR);

	/* Enable IRQ */
	if (pdata->irq)
		IRQENABLE((MFIFO | CMD_ERR | CMD_OK), base);

	/* Handle all given i2c messages */
	for (i = 0; (i < num) && (rc == 0); i++) {
		/* Check error criteria */
		if (msgs[i].len > 1024)
			rc = -EINVAL;
		if (msgs[i].flags & ~(I2C_M_RD | I2C_M_NOSTART | I2C_M_STOP | I2C_M_REV_DIR_ADDR)) /* These are supported flags */
			rc = -EOPNOTSUPP;
		if (rc)
			break;

		/* Handle the flags */
		nwr = (msgs[i].flags & I2C_M_RD) ? 1 : 0;
		start = (msgs[i].flags & I2C_M_NOSTART) ? 0 : 1;
		stop = ((msgs[i].flags & I2C_M_STOP) || (i == (num - 1))) ? 1 : 0;

		if (msgs[i].flags & I2C_M_REV_DIR_ADDR)
			nwr ^= 1;

		/* Handle the start condition */
		if (start) {
			writel((pdata->acpollmax << CMD_ACPOLLMAX_SHIFT) | CMD_CMD_S_AC | nwr, base + I2C_CMD);
			rc = netx4000_i2c_wait_for_idle(pdata);
			if (rc)
				break;
		}

		/* Handle the data transfer (read/write) */
		if (msgs[i].len > 0) {
			pdata->dup_msg = msgs[i];
			if (nwr)
				rc = netx4000_i2c_recv(pdata, &pdata->dup_msg, nwr, stop);
			else
				rc = netx4000_i2c_send(pdata, &pdata->dup_msg, nwr, stop);
			if (rc) {
				break;
			}
		}

		/* Handle the stop condition */
		if (stop) {
			writel(CMD_CMD_STOP, base + I2C_CMD);
			rc = netx4000_i2c_wait_for_idle(pdata);
			if (rc)
				break;
		}
	}

	/* Disable IRQ */
	if (pdata->irq)
		IRQDISABLE((MFIFO | CMD_ERR | CMD_OK), base);

	/* Disable i2c master */
	writel(0, base + I2C_MCR);

	if (rc)
		return rc; /* Error */

	return num; /* Success */
}

static uint32_t netx4000_i2c_func(struct i2c_adapter *adapter)
{
	struct netx4000_i2c_pdata *pdata = i2c_get_adapdata(adapter);

	dev_dbg(pdata->dev, "%s() called\n", __func__);

	return I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL;

	/* To support the block functions, support for I2C_M_RECV_LEN is required */
	/* return I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL | I2C_FUNC_SMBUS_READ_BLOCK_DATA | I2C_FUNC_SMBUS_BLOCK_PROC_CALL; */

}

static struct i2c_algorithm netx4000_i2c_algorithm = {
	.master_xfer	= netx4000_i2c_master_xfer,
	.smbus_xfer	= NULL, /* not supported */
	.functionality	= netx4000_i2c_func,
};


static int32_t netx4000_i2c_chip_init(struct netx4000_i2c_pdata *pdata)
{
	void __iomem *base = pdata->base;

	dev_dbg(pdata->dev, "%s() called\n", __func__);

	/* Chip reset */
	writel(MCR_RST_I2C, base + I2C_MCR);

	return 0;
}

static int netx4000_i2c_probe(struct platform_device *pdev)
{
	struct netx4000_i2c_pdata *pdata = NULL;
	uint8_t bus_clock_default = 0, bus_clock_fallback = 0;
	int rc = 0;

	dev_dbg(&pdev->dev, "%s() called\n", __func__);

	/* Allocate memory for private data */
	pdata = devm_kzalloc(&pdev->dev, sizeof(*pdata), GFP_KERNEL);
	if (pdata == NULL) {
		dev_err(&pdev->dev, "devm_kzalloc() failed\n");
		rc = -ENOMEM;
		goto err_out;
	}

	platform_set_drvdata(pdev, pdata);

	pdata->dev = &pdev->dev;

	/* Read the register base address from DT and map it */
	pdata->base = of_iomap(pdev->dev.of_node, 0);
	if (pdata->base == NULL) {
		dev_err(pdata->dev, "of_iomap() failed\n");
		rc = -EIO;
		goto err_out;
	}

	/* Read the IRQ number from DT (optional) */
	pdata->irq = irq_of_parse_and_map(pdev->dev.of_node, 0);
	if (pdata->irq > 0) {
		init_waitqueue_head(&pdata->wait_queue);

		rc = devm_request_irq(pdata->dev, pdata->irq, netx4000_i2c_isr, 0, dev_name(&pdev->dev), pdata);
		if (rc) {
			dev_err(pdata->dev, "devm_request_irq() failed\n");
			pdata->irq = 0; /* fallback to polling mode */
		}
	}

	/* Read, parse and handle the 'clock-frequency' from DT (optional) */
	{
		uint32_t i;
		const uint32_t clock[][2] = {
			{3400000, MCR_MODE_3400K},
			{1700000, MCR_MODE_1700K},
			{1200000, MCR_MODE_1200K},
			{800000, MCR_MODE_800K},
			{400000, MCR_MODE_400K},
			{200000, MCR_MODE_200K},
			{100000, MCR_MODE_100K},
			{50000, MCR_MODE_50K},
			{0, 0}
		};

		/* Read 'clock-frequency' from DT */
		rc = of_property_read_u32(pdev->dev.of_node, "clock-frequency", &pdata->bus_clock);
		if (rc)
			pdata->bus_clock = 0;

		/* Parse it */
		for (i = 0; pdata->bus_clock < clock[i][0]; i++);

		/* Check for fallback */
		if (pdata->bus_clock > clock[i][0])
			bus_clock_fallback = 1;

		/* Check for default bus clock */
		if (clock[i][0] == 0) {
			bus_clock_default = 1;
			i -= 2; /* 100kHz */
		}

		/* Set up the private data */
		pdata->bus_clock = clock[i][0];
		pdata->speed_mode = clock[i][1];
	}

	pdata->acpollmax = 255;

	/* Initialize the i2c chip */
	rc = netx4000_i2c_chip_init(pdata);
	if (rc) {
		dev_err(pdata->dev, "netx4000_i2c_chip_init() failed\n");
		goto err_out;
	}

	/* Set up the i2c adapter */
	i2c_set_adapdata(&pdata->adapter, pdata);

	snprintf(pdata->adapter.name, sizeof(pdata->adapter.name), "netx4000-i2c");
	pdata->adapter.owner = THIS_MODULE;
	pdata->adapter.algo = &netx4000_i2c_algorithm;
	pdata->adapter.dev.parent = &pdev->dev;
	pdata->adapter.nr = pdev->id;
	pdata->adapter.dev.of_node = pdev->dev.of_node;

	/* Register the i2c adapter (chip) */
	rc = i2c_add_numbered_adapter(&pdata->adapter);
	if (rc) {
		dev_err(pdata->dev, "i2c_add_numbered_adapter() failed\n");
		goto err_out;
	}

	if (pdata->irq == 0)
		dev_warn(pdata->dev, "i2c%d invalid or missing IRQ in device tree => polling mode\n", pdata->adapter.nr);
	if (bus_clock_fallback | bus_clock_default)
		dev_warn(pdata->dev, "i2c%d invalid or missing clock-frequency in device tree => %s\n"
			, pdata->adapter.nr, (bus_clock_default) ? "default frequency" : "fallback");

	dev_info(pdata->dev, "i2c%d (%dkHz) successfully initialized!\n", pdata->adapter.nr, pdata->bus_clock / 1000);

	return 0;

err_out:
	if (pdata)
		devm_kfree(&pdev->dev, pdata);

	return rc;
}

static int netx4000_i2c_remove(struct platform_device *pdev)
{
	struct netx4000_i2c_pdata *pdata = platform_get_drvdata(pdev);
	uint32_t adapter_nr = pdata->adapter.nr;

	dev_dbg(&pdev->dev, "%s() called\n", __func__);

	if (pdata->irq > 0)
		devm_free_irq(pdata->dev, pdata->irq, pdata);

	i2c_del_adapter(&pdata->adapter);

	if (pdata)
		devm_kfree(&pdev->dev, pdata);

	dev_info(pdata->dev, "i2c%d successfully removed!\n", adapter_nr);

	return 0;
}

static const struct of_device_id netx4000_i2c_of_match[] = {
	{ .compatible = "hilscher,netx4000-i2c", },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, netx4000_i2c_of_match);

static struct platform_driver netx4000_i2c_driver = {
	.probe		= netx4000_i2c_probe,
	.remove		= netx4000_i2c_remove,
	.driver	= {
		.name = DRIVER_NAME,
		.of_match_table = of_match_ptr(netx4000_i2c_of_match),
	},
};

static int __init netx4000_i2c_init(void)
{
	pr_info("%s: %s\n", DRIVER_NAME, DRIVER_DESC);
	return platform_driver_register(&netx4000_i2c_driver);
}
module_init(netx4000_i2c_init);

static void __exit netx4000_i2c_exit(void)
{
	platform_driver_unregister(&netx4000_i2c_driver);
}
module_exit(netx4000_i2c_exit);

/* --- Module information --- */

MODULE_AUTHOR("Hilscher Gesellschaft fuer Systemautomation mbH");
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL v2");
