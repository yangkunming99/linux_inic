#include "autoconf.h"
#include "rtw_debug.h"
#include "osdep_service.h"
#include "drv_types.h"
#include "rtw_recv.h"
#include "recv_osdep.h"
#include "hal_intf.h"
#define _RTW_RECV_C_
struct recv_buf *rtw_dequeue_recvbuf (_queue *queue)
{
	_irqL irqL;
	struct recv_buf *precvbuf;
	_list	*plist, *phead;	


#ifdef CONFIG_SDIO_HCI
	_enter_critical_bh(&queue->lock, &irqL);
#else
	_enter_critical_ex(&queue->lock, &irqL);
#endif/*#ifdef  CONFIG_SDIO_HCI*/
	
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

#ifdef CONFIG_SDIO_HCI
	_exit_critical_bh(&queue->lock, &irqL);
#else
	_exit_critical_ex(&queue->lock, &irqL);
#endif/*#ifdef  CONFIG_SDIO_HCI*/

	return precvbuf;

}
s32 rtw_enqueue_recvbuf(struct recv_buf *precvbuf, _queue *queue)
{
	_irqL irqL;	

#ifdef CONFIG_SDIO_HCI
	_enter_critical_bh(&queue->lock, &irqL);
#else
	_enter_critical_ex(&queue->lock, &irqL);
#endif/*#ifdef  CONFIG_SDIO_HCI*/

	rtw_list_delete(&precvbuf->list);

	rtw_list_insert_tail(&precvbuf->list, get_list_head(queue));

#ifdef CONFIG_SDIO_HCI	
	_exit_critical_bh(&queue->lock, &irqL);
#else
	_exit_critical_ex(&queue->lock, &irqL);
#endif/*#ifdef  CONFIG_SDIO_HCI*/
	return _SUCCESS;
	
}

s32 rtw_init_recv_priv(PADAPTER padapter)
{
	s32	res;

	res = rtw_hal_init_recv_priv(padapter);

	return res;
}

void rtw_free_recv_priv(PADAPTER padapter)
{
	rtw_hal_free_recv_priv(padapter);
}

int rtw_recv_entry(PADAPTER padapter, struct recv_buf *precvbuf)
{
	int ret = _SUCCESS;
	struct recv_priv *precvpriv = &padapter->recvpriv;
	AT_CMD_DESC atcmddesc;
	//_irqL irqL;
	unsigned char attype[2];
	struct cmd_priv *pcmdpriv;
	

	_rtw_memcpy(&atcmddesc, precvbuf->pskb->data, SIZE_AT_CMD_DESC);
	//remove the at cmd header
	skb_pull(precvbuf->pskb, atcmddesc.offset);

	if(atcmddesc.datatype == DATA_FRAME)//data pkt 
	{	
		if ((padapter->bDriverStopped == _FALSE) && (padapter->bSurpriseRemoved == _FALSE))
		{
			RT_TRACE(_module_rtl871x_recv_c_, _drv_alert_, ("@@@@ recv_func: recv_func rtw_recv_indicatepkt\n" ));
			//indicate this recv_frame
			ret = rtw_recv_indicatepkt(padapter, precvbuf);
			if (ret != _SUCCESS)
			{	
				#ifdef DBG_RX_DROP_FRAME
				DBG_871X("DBG_RX_DROP_FRAME %s rtw_recv_indicatepkt fail!\n", __FUNCTION__);
				#endif
				goto _recv_data_drop;
			}
		}
		else
		{
			RT_TRACE(_module_rtl871x_recv_c_, _drv_alert_, ("@@@@  recv_func: rtw_free_recvframe\n" ));
			RT_TRACE(_module_rtl871x_recv_c_, _drv_debug_, ("recv_func:bDriverStopped(%d) OR bSurpriseRemoved(%d)", padapter->bDriverStopped, padapter->bSurpriseRemoved));
			#ifdef DBG_RX_DROP_FRAME
			DBG_871X("DBG_RX_DROP_FRAME %s ecv_func:bDriverStopped(%d) OR bSurpriseRemoved(%d)\n", __FUNCTION__,
				padapter->bDriverStopped, padapter->bSurpriseRemoved);
			#endif
			ret = _FAIL;
			goto _recv_data_drop;
		}
	}
	else if(atcmddesc.datatype == MNGMT_FRAME)//cmd pkt
	{
		pcmdpriv = &padapter->cmdpriv;
		//check the at cmd type
		_rtw_memcpy(&attype, precvbuf->pskb->data, 2);
		if(_rtw_memcmp(attype, AT_CMD_wifi_linked, SIZE_AT_CMD_TYPE))
		{
			DBG_871X("%s: Ameba connected!\n", __FUNCTION__);
			rtw_os_indicate_connect(padapter->pnetdev);
		}
		if(_rtw_memcmp(attype, AT_CMD_wifi_unlinked, SIZE_AT_CMD_TYPE))
		{
			DBG_871X("%s: Ameba disconnected!\n", __FUNCTION__);
			rtw_os_indicate_disconnect(padapter->pnetdev);
		}
		//todo: handler other cmd frame
		_rtw_skb_free(precvbuf->pskb);
		goto _free_recv_buf;
	}
_recv_data_drop:
	if(ret == _FAIL){
		precvpriv->rx_drop++;
	}
_free_recv_buf:
	precvbuf->pskb = NULL;
	rtw_enqueue_recvbuf(precvbuf, &precvpriv->free_recv_buf_queue);
	precvpriv->free_recv_buf_queue_cnt++;
	return ret;
}

