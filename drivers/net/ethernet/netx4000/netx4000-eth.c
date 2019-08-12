/*
* MDIO XC driver for Hilscher netx4000 based platforms
*
* drivers/net/ethernet/netx4000-eth.c
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
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
*/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>

#include <linux/etherdevice.h>
#include <linux/ethtool.h>
#include <linux/mii.h>

#include <linux/of.h>
#include <linux/of_net.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_mdio.h>
#include <linux/phy.h>

#include <linux/list.h>

#include "netx4000-xc.h"
#include "netx4000-xceth-hal.h"

#define MSK_ETHMAC_IRQ_IND_HI        0x00000001U
#define MSK_ETHMAC_IRQ_IND_LO        0x00000002U
#define MSK_ETHMAC_IRQ_CON_HI        0x00000004U
#define MSK_ETHMAC_IRQ_CON_LO        0x00000008U
#define MSK_ETHMAC_IRQ_LINK_CHANGED  0x00000010U
#define MSK_ETHMAC_IRQ_COL           0x00000020U
#define MSK_ETHMAC_IRQ_EARLY_RCV     0x00000040U
#define MSK_ETHMAC_IRQ_RX_ERR        0x00000080U
#define MSK_ETHMAC_IRQ_TX_ERR        0x00000100U

#define RESERVED_SEND_BUFFER 4

struct send_buffer {
	struct list_head list;

	ETHERNET_FRAME_T *ptFrame;
	void* hFrame;
};

struct netx_eth_priv {
	int portno;
	uint32_t msg_enable;
	struct port* port;
	spinlock_t lock;
	struct phy_device* phy;
	struct device* dev;
	struct mii_if_info mii_if;

	struct list_head txdesc_free;
	struct list_head txdesc_used;
};

static void netx_eth_ethtool_getdrvinfo(struct net_device *ndev, struct ethtool_drvinfo *ed)
{
	/* FIXME: Insert reasonable driver informations! */
	strlcpy(ed->driver, "netx4000-xceth", sizeof(ed->driver));
	strlcpy(ed->version, "6.2.0.2", sizeof(ed->version));
}

static const struct ethtool_ops netx_xceth_ethtool_ops = {
	/* FIXME: Check which function we should support! */
	.get_drvinfo = netx_eth_ethtool_getdrvinfo,
	.get_link = ethtool_op_get_link,
	.get_link_ksettings = phy_ethtool_get_link_ksettings,
	.set_link_ksettings = phy_ethtool_set_link_ksettings,
};

void netx_eth_ethtool_ops(struct net_device *netdev)
{
	netdev->ethtool_ops = &netx_xceth_ethtool_ops;
}

static void netx_eth_set_multicast_list(struct net_device *ndev)
{
	//TODO
}

static void netx_eth_set_rx_mode(struct net_device *ndev)
{
	struct netx_eth_priv *priv = netdev_priv(ndev);

	if (ndev->flags & IFF_PROMISC) {
		netx4000_xceth_mode_promisc( priv->portno, 1);
	} else {
		netx4000_xceth_mode_promisc( priv->portno, 0);
	}
	netx_eth_set_multicast_list( ndev);
}

static void netx_eth_timeout(struct net_device *ndev)
{
// 	struct netx_eth_priv *priv = netdev_priv(ndev);
	//int i;

	printk(KERN_ERR "%s: transmit timed out, resetting\n", ndev->name);
	//spin_lock_irq(&priv->lock);
	//printk(".... -> timeout \n");

	//port_reset( priv->port);
	//port_start( priv->port);

	//for (i=2; i<=18; i++)
	//	pfifo_push(EMPTY_PTR_FIFO(priv->id), FIFO_PTR_FRAMENO(i) | FIFO_PTR_SEGMENT(priv->id));

	//spin_unlock_irq(&priv->lock);
	netif_wake_queue(ndev);
}

static int netx_eth_hard_start_xmit(struct sk_buff *skb, struct net_device *ndev)
{
	struct netx_eth_priv *priv = netdev_priv(ndev);
	struct send_buffer *txdesc;
	uint32_t length = 0;
	int ret = NETDEV_TX_BUSY;

	spin_lock_irq(&priv->lock);

	txdesc = list_first_entry_or_null(&priv->txdesc_free, struct send_buffer, list);
	if (txdesc == NULL) {
		netif_stop_queue(ndev);
		ret = NETDEV_TX_BUSY;
		goto exit;
	}

	memcpy_toio(txdesc->ptFrame, skb->data, skb->len);
	length = skb->len;
	if (skb->len < 60) {
		memset((uint8_t*)txdesc->ptFrame + length, 0, 60 - skb->len);
		length = 60;
	}

	ret = netx4000_xceth_send_frame(priv->port->portno, txdesc->hFrame, length, 0 /* LowPrio */);
	if (ret) {
		/* do not release frame here since we keep it all the time to make sure to have a resource to send */
		dev_err(priv->dev,"error sending frame!\n");
	}
	else {
		list_move_tail(priv->txdesc_free.next, &priv->txdesc_used);

		ndev->stats.tx_packets++;
		ndev->stats.tx_bytes += length;
		ret = NETDEV_TX_OK;
	}

	dev_kfree_skb(skb);

exit:
	spin_unlock_irq(&priv->lock);

	return ret;
}

static void netx_eth_receive(struct net_device *ndev)
{
	struct netx_eth_priv *priv = netdev_priv(ndev);
	ETHERNET_FRAME_T *ptFrame = NULL;
	void* hFrame = NULL;
	uint32_t ulLen;
	struct sk_buff *skb;
	uint8_t *data;

	do {
		if (netx4000_xceth_recv_frame(priv->port->portno, &ptFrame, &hFrame, &ulLen, 0 /* LowPrio */))
			break;

		skb = netdev_alloc_skb(ndev, ulLen + 2);
		if (unlikely(!skb)) {
			netx4000_xceth_release_frame(priv->port->portno, hFrame);
			ndev->stats.rx_dropped++;
			printk(KERN_NOTICE "%s: Low memory, packet dropped.\n", ndev->name);
			break;
		}

		skb_reserve(skb, 2); /* align IP on 16B boundary */
		data = skb_put(skb, ulLen);

		memcpy_fromio(data, ptFrame, ulLen);

		netx4000_xceth_release_frame(priv->port->portno, hFrame);

		skb->protocol = eth_type_trans(skb, ndev);

		netif_rx(skb);

		ndev->stats.rx_packets++;
		ndev->stats.rx_bytes += ulLen;
	} while (0);
}

static irqreturn_t netx_eth_interrupt(int irq, void *dev_id)
{
	struct net_device *ndev = dev_id;
	struct netx_eth_priv *priv = netdev_priv(ndev);
	unsigned long flags;
	int status;
	uint32_t txlevel, rxlevel;

	spin_lock_irqsave(&priv->lock, flags);

	status = netx4000_xceth_get_irq(priv->port->portno);
	if (status) {
		netx4000_xceth_confirm_irq(priv->port->portno, status);

		netx4000_xceth_get_send_cnf_fill_level(priv->port->portno, 0 /* low prio */, &txlevel);
		netx4000_xceth_get_recv_fill_level(priv->port->portno, 0 /* low prio */, &rxlevel);

		if (status & (MSK_ETHMAC_IRQ_CON_HI | MSK_ETHMAC_IRQ_CON_LO)) {
			while (txlevel--) {
				struct send_buffer *txdesc;
				uint32_t ulLength;
				int rc;

				txdesc = list_first_entry_or_null(&priv->txdesc_used, struct send_buffer, list);
				if (txdesc) {
					rc = netx4000_xceth_get_send_cnf(priv->port->portno, &txdesc->ptFrame, &txdesc->hFrame, &ulLength, 0 /* low prio */);
					if (rc != -EBUSY) {
						/* Confirmation frame received! */
						list_move_tail(priv->txdesc_used.next, &priv->txdesc_free);
						netif_wake_queue(ndev);

						if (rc < -EIO)
							ndev->stats.tx_errors++;
					}
				}
				else
					/* TODO: Error handling */
					dev_err(priv->dev, "Unexpected confirmation frame received. Skipping IRQ!\n");
			}
		}

		if (status & (MSK_ETHMAC_IRQ_IND_HI | MSK_ETHMAC_IRQ_IND_LO)) {
			while (rxlevel--)
				netx_eth_receive( ndev);
		}
	}

	spin_unlock_irqrestore(&priv->lock, flags);

	return IRQ_HANDLED;
}

static void netx4000_xceth_update_linkspeed(struct net_device *ndev)
{
	struct netx_eth_priv *priv = netdev_priv(ndev);

	dev_dbg(priv->dev, "%s: called for %p\n", __func__, ndev);

	port_set_link_mode(priv->port, ndev->phydev->link, ndev->phydev->speed, ndev->phydev->duplex);

	phy_print_status(ndev->phydev);
}

static int netx4000_init_txdesc_lists(struct netx_eth_priv *priv)
{
	struct send_buffer *desc;
	int i, rc;

	INIT_LIST_HEAD(&priv->txdesc_free);
	INIT_LIST_HEAD(&priv->txdesc_used);

	for (i = 0; i < RESERVED_SEND_BUFFER; i++) {
		desc = kmalloc(sizeof(*desc), GFP_KERNEL);
		rc = netx4000_xceth_get_frame(priv->port->portno, &desc->ptFrame, &desc->hFrame);
		if (rc) {
			kfree(desc);
			return -ENOMEM;
		}
		list_add_tail(&desc->list, &priv->txdesc_free);
	}

	return 0;
}

static void netx4000_free_txdesc_lists(struct netx_eth_priv *priv)
{
	struct send_buffer *desc, *desc_tmp;

	/* Go through the txdesc lists and free related memory. */
	list_for_each_entry_safe(desc, desc_tmp, &priv->txdesc_free, list) {
		list_del(&desc->list);
		netx4000_xceth_release_frame(priv->port->portno, desc->hFrame);
		kfree(desc);
	}
	list_for_each_entry_safe(desc, desc_tmp, &priv->txdesc_used, list) {
		list_del(&desc->list);
		netx4000_xceth_release_frame(priv->port->portno, desc->hFrame);
		kfree(desc);
	}
}

static int netx_eth_open(struct net_device *ndev)
{
	struct netx_eth_priv *priv = netdev_priv(ndev);
	int ret = 0;
	struct device_node* phy_node = NULL;
	phy_interface_t interface;

	dev_dbg(priv->dev, "%s: called for %p\n", __func__, ndev);

	ret = netx4000_init_txdesc_lists(priv);
	if (ret) {
		dev_err(priv->dev, "Failed to initialize txdesc lists!\n");
		return ret;
	}

	netx4000_xceth_confirm_irq( priv->portno, 0xffffffff);
	if ((ret = request_irq(ndev->irq, netx_eth_interrupt, 0, ndev->name, ndev)) != 0) {
		dev_err(priv->dev, "%s: Failed to request irq %d (ret=%d)\n", __func__, ndev->irq, ret);
		return -EAGAIN;
	}
	netx4000_xceth_set_irq( priv->portno, MSK_ETHMAC_INTERRUPTS_ENABLE_CON_LO_VAL |
				MSK_ETHMAC_INTERRUPTS_ENABLE_CON_HI_VAL |
				MSK_ETHMAC_INTERRUPTS_ENABLE_IND_LO_VAL |
				MSK_ETHMAC_INTERRUPTS_ENABLE_IND_HI_VAL);

	port_start(priv->port);

	phy_node = of_parse_phandle(priv->dev->of_node, "phy-handle", 0);
	interface = of_get_phy_mode(phy_node);
	if ((priv->phy = of_phy_connect( ndev, phy_node, netx4000_xceth_update_linkspeed, 0,  interface)) == NULL) {
		dev_err(priv->dev, "%s: Failed to connect to phy\n", __func__);
		ret =-EAGAIN;
		goto err;
	}
	phy_attached_info(priv->phy);
	phy_start( priv->phy);

	netif_start_queue(ndev);

	return 0;

err:
	port_stop(priv->port);

	netx4000_xceth_set_irq( priv->portno, 0);
	free_irq(ndev->irq, ndev);

	netx4000_free_txdesc_lists(priv);

	return ret;
}

static int netx_eth_close(struct net_device *ndev)
{
	struct netx_eth_priv *priv = netdev_priv(ndev);

	netif_stop_queue(ndev);

	port_stop(priv->port);

	netx4000_xceth_set_irq( priv->portno, 0);
	free_irq(ndev->irq, ndev);

	netx4000_free_txdesc_lists(priv);

	phy_stop( priv->phy);
	phy_disconnect( priv->phy);

	return 0;
}

static const struct net_device_ops netx_eth_netdev_ops = {
	.ndo_open = netx_eth_open,
	.ndo_stop = netx_eth_close,
	.ndo_start_xmit = netx_eth_hard_start_xmit,
	.ndo_tx_timeout = netx_eth_timeout,
	.ndo_set_rx_mode = netx_eth_set_rx_mode,
	.ndo_change_mtu = eth_change_mtu,
	.ndo_validate_addr = eth_validate_addr,
	.ndo_set_mac_address = eth_mac_addr,
};

static int netx_eth_enable(struct net_device *ndev)
{
	struct netx_eth_priv *priv = netdev_priv(ndev);
	int ret = 0;
	char* mac = NULL;

	ether_setup(ndev);

	ndev->netdev_ops = &netx_eth_netdev_ops;
	ndev->watchdog_timeo = msecs_to_jiffies(5000);

	/* this function will also donwload the firmware if necessary */
	priv->port = port_request( priv->portno, priv->dev);
	if (priv->port == NULL) {
		dev_dbg(priv->dev, "%s: requesting firmware failed %d\n", __func__, ret);
		return -ENODEV;
	}
	netx4000_xceth_initFifoUnit( priv->portno);

	/* Configure MAC address */
	if (NULL != (mac = (char*)of_get_mac_address(priv->dev->of_node))) {
		port_set_mac_addr( priv->port, ETH_MAC_ADDRESS_CHASSIS, mac);
		memcpy( ndev->dev_addr, mac, 6);
	} else {
		dev_err(priv->dev, "Failed to get mac addr from device-tree!\n");
	}
	dev_dbg(priv->dev, "%s: Registering net device netx4000-xceth%u...\n", __func__, priv->portno);

	return 0;
}

static void netx_eth_disable(struct net_device *ndev)
{
	struct netx_eth_priv *priv = netdev_priv(ndev);

	if ( priv->port != NULL) {
		port_free( priv->port);
		priv->port = NULL;

		netx4000_xceth_fifo_release( priv->portno);
	}
}

int s_fifo_reset_done = 0;

static int netx_eth_drv_probe(struct platform_device *pdev)
{
	struct netx_eth_priv *priv;
	struct net_device *ndev;
	int ret;

	dev_dbg( &pdev->dev, "%s: probing netX4000 Ethernet device...\n",__func__);

	ndev = alloc_etherdev(sizeof (struct netx_eth_priv));
	if (!ndev) {
		return -ENOMEM;
	}
	priv = netdev_priv(ndev);
	priv->dev = &pdev->dev;
	ndev->dev.parent = &pdev->dev;
	ndev->irq = of_irq_get(pdev->dev.of_node, 0);
	netx_eth_ethtool_ops(ndev);
	spin_lock_init(&priv->lock);

	if (0 != (ret = of_property_read_u32( pdev->dev.of_node, "port", &priv->portno))) {
		dev_err( &pdev->dev, "%s: Failed to retrieve port information (ret=%d)\n",__func__, ret);
		goto err;
	}
	/* the request is done global for instance 0 & 1 so make sure to do it only once */
	if (netx4000_xceth_fifo_request( &ndev->dev, priv->portno)) {
		ret = -EIO;
		dev_err( &pdev->dev, "%s: Failed to request fifo!\n",__func__);
		goto err;
	}

	do { /* Resetting pfifo only once per XC unit (2 ports) */
		uint8_t xcNo = priv->portno >> 1;

		if (s_fifo_reset_done & (1 << xcNo))
			break;

		s_fifo_reset_done |= (1 << xcNo);
		netx4000_pfifo_reset(xcNo);
	} while (0);

	ret = netx_eth_enable(ndev);
	if (ret<0) {
		dev_err( &pdev->dev, "%s: Failed to enable ethernet device port %d\n",__func__, priv->portno);
		goto err_enable;
	}
	if (!is_valid_ether_addr(ndev->dev_addr))
		printk("%s: Invalid ethernet MAC address.  Please set using ifconfig\n", ndev->name);

	if ((ret = register_netdev(ndev)) != 0) {
		goto err_register;
	}
	platform_set_drvdata( pdev, ndev);
	return 0;

err_register:
	netx_eth_disable(ndev);
err_enable:
	netx4000_xceth_fifo_release( priv->portno);
err:
	free_netdev(ndev);

	return ret;
}

static int netx_eth_drv_remove(struct platform_device *pdev)
{
	struct net_device *ndev = dev_get_drvdata(&pdev->dev);

	if (ndev != NULL) {
		unregister_netdev(ndev);

		netx_eth_disable( ndev);

		free_netdev(ndev);
	}
	return 0;
}

static int netx_eth_drv_suspend(struct platform_device *pdev, pm_message_t state)
{
	dev_err(&pdev->dev, "suspend not implemented\n");
	return 0;
}

static int netx_eth_drv_resume(struct platform_device *pdev)
{
	dev_err(&pdev->dev, "resume not implemented\n");
	return 0;
}

static struct of_device_id const netx4000_xceth_of_match[] __refconst = {
	{ .compatible = "hilscher,netx4000-xceth", },
	{ .compatible = "hilscher,netx4000-xceth0", },
	{ .compatible = "hilscher,netx4000-xceth1", },
	{}
};

MODULE_DEVICE_TABLE(of, netx4000_xceth_of_match);

static struct platform_driver netx4000_xceth_platform_driver = {
	.probe = netx_eth_drv_probe,
	.remove = netx_eth_drv_remove,
	.suspend = netx_eth_drv_suspend,
	.resume = netx_eth_drv_resume,
	.driver = {
		.name = "netx4000-xceth",
		.owner = THIS_MODULE,
		.of_match_table = netx4000_xceth_of_match,
	},
};

struct platform_device *debug_device = NULL;

static int __init netx_eth_init(void)
{
	int ret = 0;

	ret = platform_driver_register( &netx4000_xceth_platform_driver);
	printk("Registering netX4000 Ethernet driver (ret=%d)\n", ret);

	return ret;
}

static void __exit netx_eth_cleanup(void)
{
	printk("De-registering netX4000 Ethernet driver\n");
	platform_driver_unregister( &netx4000_xceth_platform_driver);
}

module_init(netx_eth_init);
module_exit(netx_eth_cleanup);

MODULE_SOFTDEP("pre: mdio-xc-netx4000");

MODULE_AUTHOR(", hilscher");
MODULE_LICENSE("GPL");
MODULE_FIRMWARE("hethmac-xc0");
MODULE_FIRMWARE("hethmac-xc1");
MODULE_FIRMWARE("hethmac-xc2");
MODULE_FIRMWARE("hethmac-xc3");
