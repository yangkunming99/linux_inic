#include "autoconf.h"
#include "drv_types.h"
#include "8195_desc.h"
#include "8195_sdio_reg.h"
#include "rtw_xmit.h"
#include "rtl8195a_xmit.h"
s32 rtl8195as_init_xmit_priv(PADAPTER padapter)
{
	s32 res = _SUCCESS;
	return res;
}

void rtl8195as_free_xmit_priv(PADAPTER padapter)
{

}
s32 rtl8195as_dequeue_writeport(PADAPTER padapter)
{
	struct xmit_buf *pxmitbuf;
_func_enter_;
	pxmitbuf = rtw_dequeue_xmitbuf(padapter);
	if(pxmitbuf == NULL)
		return _TRUE;
	rtw_write_port(padapter, WLAN_TX_FIFO_DEVICE_ID, pxmitbuf->pkt_len, pxmitbuf->pdata);
	rtw_free_xmitbuf(padapter, pxmitbuf);
_func_exit_;
	return _FAIL;
}
s32 rtl8195as_hal_xmit_handler(PADAPTER padapter)
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
	is_queue_empty=rtl8195as_dequeue_writeport(padapter);
	}while(!is_queue_empty);
	return _SUCCESS;
}

s32 rtl8195as_hal_xmit(PADAPTER padapter, struct xmit_buf *pxmitbuf)
{
	struct xmit_priv 	*pxmitpriv = &padapter->xmitpriv;
	_irqL irqL;
_func_enter_;
	//enqueue xmitbuf
_enter_critical_bh(&pxmitpriv->xmitbuf_pending_queue.lock, &irqL);
	rtw_list_insert_tail(&pxmitbuf->list, get_list_head(&pxmitpriv->xmitbuf_pending_queue));
_exit_critical_bh(&pxmitpriv->xmitbuf_pending_queue.lock, &irqL);
_func_exit_;
	_rtw_up_sema(&pxmitpriv->xmit_sema);
	return _TRUE;
}
s32 rtl8195as_hal_mgnt_xmit(PADAPTER padapter, struct xmit_buf *pxmitbuf)
{
	rtw_write_port(padapter, WLAN_TX_FIFO_DEVICE_ID, pxmitbuf->pkt_len, pxmitbuf->pdata);
	rtw_free_xmitbuf(padapter, pxmitbuf);
	return _TRUE;
}