#include "autoconf.h"
#include "rtw_debug.h"
#include "8195_desc.h"
#include "8195_sdio_reg.h"
#include "sdio_ops.h"
#include "osdep_service.h"
#include "sdio_ops_linux.h"
#include "rtw_recv.h"
#define _RTW_RECV_C_
struct recv_buf *rtw_dequeue_recvbuf (_queue *queue)
{
	_irqL irqL;
	struct recv_buf *precvbuf;
	_list	*plist, *phead;	


	_enter_critical_bh(&queue->lock, &irqL);
	
	if(_rtw_queue_empty(queue) == _TRUE)
	{
		precvbuf = NULL;
	}
	else
	{
		phead = get_list_head(queue);

		plist = get_next(phead);

		precvbuf = LIST_CONTAINOR(plist, struct recv_buf, list);

		rtw_list_delete(&precvbuf->list);
		
	}

	_exit_critical_bh(&queue->lock, &irqL);

	return precvbuf;

}
s32 rtw_enqueue_recvbuf(struct recv_buf *precvbuf, _queue *queue)
{
	_irqL irqL;	

	_enter_critical_bh(&queue->lock, &irqL);

	rtw_list_delete(&precvbuf->list);

	rtw_list_insert_tail(&precvbuf->list, get_list_head(queue));

	_exit_critical_bh(&queue->lock, &irqL);
	return _SUCCESS;
	
}
static void rtw_recv_entry(PADAPTER padapter, struct sk_buff *skb);
static void rtl8195a_recv_tasklet(void *priv)
{
	PADAPTER padapter;
	struct recv_buf *precvbuf;
	struct recv_priv *precvpriv;
	padapter = (PADAPTER)priv;
	precvpriv = &padapter->recvpriv;
	do {
		precvbuf = rtw_dequeue_recvbuf(&precvpriv->recv_buf_pending_queue);
		if (NULL == precvbuf) break;
		rtw_recv_entry(padapter, precvbuf->pskb);
		precvbuf->pskb = NULL;
		rtw_enqueue_recvbuf(precvbuf, &precvpriv->free_recv_buf_queue);
	} while (1);

}
/*
 * Initialize recv private variable for hardware dependent
 * 1. recv buf
 * 2. recv tasklet
 *
 */
s32 rtw_init_recv_priv(PADAPTER padapter)
{
	s32			res;
	u32			i, n;
	struct recv_buf		*precvbuf;
	struct recv_priv	*precvpriv = &padapter->recvpriv;

	res = _SUCCESS;

	//3 1. init recv buffer
	_rtw_init_queue(&precvpriv->free_recv_buf_queue);
	_rtw_init_queue(&precvpriv->recv_buf_pending_queue);

	n = NR_RECVBUFF * sizeof(struct recv_buf) + 4;
	precvpriv->pallocated_recv_buf = rtw_zmalloc(n);
	if (precvpriv->pallocated_recv_buf == NULL) {
		res = _FAIL;
		goto exit;
	}

	precvpriv->precv_buf = (u8*)N_BYTE_ALIGMENT((SIZE_PTR)(precvpriv->pallocated_recv_buf), 4);

	// init each recv buffer
	precvbuf = (struct recv_buf*)precvpriv->precv_buf;
	for (i = 0; i < NR_RECVBUFF; i++)
	{
		_rtw_init_listhead(&precvbuf->list);

		precvbuf->adapter = padapter;

		rtw_list_insert_tail(&precvbuf->list, &precvpriv->free_recv_buf_queue.queue);

		precvbuf++;
	}
	precvpriv->free_recv_buf_queue_cnt = i;

	//3 2. init tasklet
	tasklet_init(&precvpriv->recv_tasklet,
	     (void(*)(unsigned long))rtl8195a_recv_tasklet,
	     (unsigned long)padapter);

	goto exit;

exit:
	return res;
}

/*
 * Free recv private variable of hardware dependent
 * 1. recv buf
 * 2. recv tasklet
 *
 */
void rtw_free_recv_priv(PADAPTER padapter)
{
	u32 n;
	struct recv_priv *precvpriv = &padapter->recvpriv;

	//3 1. kill tasklet
	tasklet_kill(&precvpriv->recv_tasklet);

	//3 2. free all recv buffers
	if (precvpriv->pallocated_recv_buf) {
		n = NR_RECVBUFF * sizeof(struct recv_buf) + 4;
		rtw_mfree(precvpriv->pallocated_recv_buf, n);
		precvpriv->pallocated_recv_buf = NULL;
		precvpriv->precv_buf = NULL;
	}
}

static void rtw_recv_entry(PADAPTER padapter, struct sk_buff *skb)
{
	struct net_device *pnetdev = padapter->pnetdev;
	RXDESC_8195A rxdesc;
	AT_CMD_DESC atcmddesc;
	//_irqL irqL;
	unsigned char attype[2];
	struct cmd_priv *pcmdpriv;
	
	_rtw_memcpy(&rxdesc, skb->data, SIZE_RX_DESC_8195a);
	_rtw_memcpy(&atcmddesc, skb->data+rxdesc.offset, SIZE_AT_CMD_DESC);

	//remove the sdio header
	skb_pull(skb, rxdesc.offset);
	//remove the at cmd header
	skb_pull(skb, atcmddesc.offset);

	pnetdev->stats.rx_packets = (++(padapter->stats.rx_packets));
	pnetdev->stats.rx_bytes =(padapter->stats.rx_bytes+=atcmddesc.pktsize);

	if(atcmddesc.datatype == DATA_FRAME)//data pkt 
	{	
		skb->protocol = eth_type_trans(skb, pnetdev);
		skb->dev = pnetdev;
		skb->ip_summed = CHECKSUM_NONE;
		_rtw_netif_rx(pnetdev, skb);
	}
	else if(atcmddesc.datatype == MNGMT_FRAME)//cmd pkt
	{
		pcmdpriv = &padapter->cmdpriv;
		//check the at cmd type
		_rtw_memcpy(&attype, skb->data, 2);
		if(_rtw_memcmp(attype, AT_CMD_wifi_linked, SIZE_AT_CMD_TYPE))
		{
			DBG_871X("%s: Ameba connected!\n", __FUNCTION__);
			rtw_os_indicate_connect(pnetdev);
			return;
		}
		if(_rtw_memcmp(attype, AT_CMD_wifi_unlinked, SIZE_AT_CMD_TYPE))
		{
			DBG_871X("%s: Ameba disconnected!\n", __FUNCTION__);
			rtw_os_indicate_disconnect(pnetdev);
			return;
		}
		_rtw_skb_free(skb);		
	}
	else
		return;
}

