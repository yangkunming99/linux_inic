#include "autoconf.h"
#include "basic_types.h"
#include "drv_types.h"

s32	rtl8195au_init_xmit_priv(_adapter *padapter)
{
	return _SUCCESS;
}
void	rtl8195au_free_xmit_priv(_adapter *padapter)
{
	
}


s32 rtl8195au_dequeue_writeport(PADAPTER padapter)
{
	struct xmit_buf *pxmitbuf;

	pxmitbuf = rtw_dequeue_xmitbuf(padapter);
	if(pxmitbuf == NULL)
		return _TRUE;
	rtw_write_port(padapter, USB_WRITE_ADD , pxmitbuf->pkt_len, (u8*)pxmitbuf);
	rtw_free_xmitbuf(padapter, pxmitbuf);
	return _FAIL;
}
s32 rtl8195au_hal_xmit_handler(PADAPTER padapter)
{
	s32 ret;
	u8 is_queue_pending;
	u8 is_queue_empty ;
	struct xmit_priv *pxmitpriv = &padapter->xmitpriv;
	ret = _rtw_down_sema(&pxmitpriv->xmit_sema);
	if(ret == _FAIL)
		return _FAIL;
	is_queue_pending = check_pending_xmitbuf(padapter);
	if(is_queue_pending == _FALSE)
		return _SUCCESS;
	do{
		is_queue_empty = rtl8195au_dequeue_writeport(padapter);
	}while(!is_queue_empty);
	return _SUCCESS;
}

s32 rtl8195au_hal_xmit(PADAPTER padapter, struct xmit_buf *pxmitbuf)
{
	struct xmit_priv 	*pxmitpriv = &padapter->xmitpriv;
	_irqL irqL;
	_pkt *pkt = pxmitbuf->pkt;
	if(pkt == NULL)
		return _FALSE;
	/* copy skb from pbuf and free pkt*/
	pxmitbuf->pkt_len = pkt->len; 
	_rtw_memcpy(pxmitbuf->pbuf, pkt->data, pkt->len);	
	pxmitbuf->pkt = NULL;

	_enter_critical_bh(&pxmitpriv->xmitbuf_pending_queue.lock, &irqL);
	rtw_list_insert_tail(&pxmitbuf->list, get_list_head(&pxmitpriv->xmitbuf_pending_queue));
	_exit_critical_bh(&pxmitpriv->xmitbuf_pending_queue.lock, &irqL);

	/* wake up transmit thread */
	_rtw_up_sema(&pxmitpriv->xmit_sema);
	return _TRUE;
}
s32 rtl8195au_hal_mgnt_xmit(PADAPTER padapter, struct cmd_obj *pcmd)
{
#if 0
	struct xmit_priv 	*pxmitpriv = &padapter->xmitpriv;
	struct xmit_buf *pxmitbuf;
	_irqL irqL;
	s32 err;
	PTXDESC_8195A ptxdesc;
	PAT_CMD_DESC patcmd; 
	//enqueue pkt
	pxmitbuf = rtw_alloc_xmitbuf(padapter);
	if(!pxmitbuf)
	{
		DBG_871X("%s(): pxmitbuf allocated failed!\n", __FUNCTION__);
		return _FALSE;
	}
	ptxdesc = (PTXDESC_8195A)pxmitbuf->pdata;
	patcmd = (PAT_CMD_DESC)(pxmitbuf->pdata + SIZE_TX_DESC_8195a);
	pxmitbuf->pkt_len = pcmd->cmdsz+SIZE_AT_CMD_DESC+ SIZE_TX_DESC_8195a;
	ptxdesc->txpktsize = pcmd->cmdsz+SIZE_AT_CMD_DESC;
	ptxdesc->offset = SIZE_TX_DESC_8195a;
	patcmd->datatype = 1;
	patcmd->pktsize = pcmd->cmdsz;
	patcmd->offset = SIZE_AT_CMD_DESC;
	_rtw_memcpy(pxmitbuf->pdata+sizeof(TX_DESC)+sizeof(AT_CMD_DESC), pcmd->parmbuf, pcmd->cmdsz);	
	pxmitbuf->pkt = NULL;
	rtw_free_cmd_obj(pcmd);
_enter_critical_bh(&pxmitpriv->xmitbuf_pending_queue.lock, &irqL);
	rtw_list_insert_tail(&pxmitbuf->list, get_list_head(&pxmitpriv->xmitbuf_pending_queue));
_exit_critical_bh(&pxmitpriv->xmitbuf_pending_queue.lock, &irqL);
	_rtw_up_sema(&pxmitpriv->xmit_sema);
#endif
	return _TRUE;
}

