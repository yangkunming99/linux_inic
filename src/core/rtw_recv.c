#include "autoconf.h"
#include "rtw_debug.h"
//#include "sdio_ops.h"
#include "osdep_service.h"
//#include "sdio_ops_linux.h"
#include "drv_types.h"
#include "rtw_recv.h"
#include "hal_intf.h"
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

s32 rtw_init_recv_priv(PADAPTER padapter)
{
	s32			res;

	res = rtw_hal_init_recv_priv(padapter);

	return res;
}

void rtw_free_recv_priv(PADAPTER padapter)
{
	rtw_hal_free_recv_priv(padapter);
}

void rtw_recv_entry(PADAPTER padapter, struct sk_buff *skb)
{
	struct net_device *pnetdev = padapter->pnetdev;
	AT_CMD_DESC atcmddesc;
	//_irqL irqL;
	unsigned char attype[2];
	struct cmd_priv *pcmdpriv;
	

	_rtw_memcpy(&atcmddesc, skb->data, SIZE_AT_CMD_DESC);

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

