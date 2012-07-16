/*
 *
 *  Bluetooth HCI Three-wire UART driver
 *
 *  Copyright (C) 2012  Intel Corporation
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/skbuff.h>

#include <net/bluetooth/bluetooth.h>
#include <net/bluetooth/hci_core.h>

#include "hci_uart.h"

#define H5_TXWINSIZE	4

#define H5_ACK_TIMEOUT	msecs_to_jiffies(250)

struct h5 {
	struct sk_buff_head unack;	/* Unack'ed packets queue */
	struct sk_buff_head rel;	/* Reliable packets queue */
	struct sk_buff_head unrel;	/* Unreliable packets queue */

	struct sk_buff *rx_skb;

	struct timer_list timer;	/* Retransmission timer */

	bool txack_req;

	u8 msgq_txseq;
};

static void h5_timed_event(unsigned long arg)
{
	struct hci_uart *hu = (struct hci_uart *) arg;
	struct h5 *h5 = hu->priv;
	struct sk_buff *skb;
	unsigned long flags;

	BT_DBG("hu %p retransmitting %u pkts", hu, h5->unack.qlen);

	spin_lock_irqsave_nested(&h5->unack.lock, flags, SINGLE_DEPTH_NESTING);

	while ((skb = __skb_dequeue_tail(&h5->unack)) != NULL) {
		h5->msgq_txseq = (h5->msgq_txseq - 1) & 0x07;
		skb_queue_head(&h5->rel, skb);
	}

	spin_unlock_irqrestore(&h5->unack.lock, flags);

	hci_uart_tx_wakeup(hu);
}

static int h5_open(struct hci_uart *hu)
{
	struct h5 *h5;

	BT_DBG("hu %p", hu);

	h5 = kzalloc(sizeof(*h5), GFP_KERNEL);
	if (!h5)
		return -ENOMEM;

	hu->priv = h5;

	skb_queue_head_init(&h5->unack);
	skb_queue_head_init(&h5->rel);
	skb_queue_head_init(&h5->unrel);

	init_timer(&h5->timer);
	h5->timer.function = h5_timed_event;
	h5->timer.data = (unsigned long) hu;

	return 0;
}

static int h5_close(struct hci_uart *hu)
{
	struct h5 *h5 = hu->priv;

	skb_queue_purge(&h5->unack);
	skb_queue_purge(&h5->rel);
	skb_queue_purge(&h5->unrel);

	del_timer(&h5->timer);

	kfree(h5);

	return 0;
}

static int h5_recv(struct hci_uart *hu, void *data, int count)
{
	return -ENOSYS;
}

static int h5_enqueue(struct hci_uart *hu, struct sk_buff *skb)
{
	struct h5 *h5 = hu->priv;

	if (skb->len > 0xfff) {
		BT_ERR("Packet too long (%u bytes)", skb->len);
		kfree_skb(skb);
		return 0;
	}

	switch (bt_cb(skb)->pkt_type) {
	case HCI_ACLDATA_PKT:
	case HCI_COMMAND_PKT:
		skb_queue_tail(&h5->rel, skb);
		break;

	case HCI_SCODATA_PKT:
		skb_queue_tail(&h5->unrel, skb);
		break;

	default:
		BT_ERR("Unknown packet type %u", bt_cb(skb)->pkt_type);
		kfree_skb(skb);
		break;
	}

	return 0;
}

static struct sk_buff *h5_prepare_pkt(struct h5 *h5, struct sk_buff *skb)
{
	h5->txack_req = false;
	return NULL;
}

static struct sk_buff *h5_prepare_ack(struct h5 *h5)
{
	h5->txack_req = false;
	return NULL;
}

static struct sk_buff *h5_dequeue(struct hci_uart *hu)
{
	struct h5 *h5 = hu->priv;
	unsigned long flags;
	struct sk_buff *skb, *nskb;

	if ((skb = skb_dequeue(&h5->unrel)) != NULL) {
		nskb = h5_prepare_pkt(h5, skb);
		if (nskb) {
			kfree_skb(skb);
			return nskb;
		}

		skb_queue_head(&h5->unrel, skb);
		BT_ERR("Could not dequeue pkt because alloc_skb failed");
	}

	spin_lock_irqsave_nested(&h5->unack.lock, flags, SINGLE_DEPTH_NESTING);

	if (h5->unack.qlen >= H5_TXWINSIZE)
		goto unlock;

	if ((skb = skb_dequeue(&h5->rel)) != NULL) {
		nskb = h5_prepare_pkt(h5, skb);

		if (nskb) {
			__skb_queue_tail(&h5->unack, skb);
			mod_timer(&h5->timer, jiffies + H5_ACK_TIMEOUT);
			spin_unlock_irqrestore(&h5->unack.lock, flags);
			return nskb;
		}

		skb_queue_head(&h5->rel, skb);
		BT_ERR("Could not dequeue pkt because alloc_skb failed");
	}

unlock:
	spin_unlock_irqrestore(&h5->unack.lock, flags);

	if (h5->txack_req)
		return h5_prepare_ack(h5);

	return NULL;
}

static int h5_flush(struct hci_uart *hu)
{
	BT_DBG("hu %p", hu);
	return 0;
}

static struct hci_uart_proto h5p = {
	.id		= HCI_UART_3WIRE,
	.open		= h5_open,
	.close		= h5_close,
	.recv		= h5_recv,
	.enqueue	= h5_enqueue,
	.dequeue	= h5_dequeue,
	.flush		= h5_flush,
};

int __init h5_init(void)
{
	int err = hci_uart_register_proto(&h5p);

	if (!err)
		BT_INFO("HCI Three-wire UART (H5) protocol initialized");
	else
		BT_ERR("HCI Three-wire UART (H5) protocol init failed");

	return err;
}

int __exit h5_deinit(void)
{
	return hci_uart_unregister_proto(&h5p);
}
