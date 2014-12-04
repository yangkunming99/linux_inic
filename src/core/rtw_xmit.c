#include "../include/autoconf.h"
#include "../include/rtw_debug.h"
#include "../include/drv_types.h"
#include "../include/rtw_xmit.h"
#include "../include/rtw_ioctl.h"
#include "../include/8195_desc.h"
#include "../include/8195_sdio_reg.h"
#include "../include/sdio_ops.h"

void rtw_sctx_init(struct submit_ctx *sctx, int timeout_ms)
{
	sctx->timeout_ms = timeout_ms;
	sctx->submit_time= rtw_get_current_time();
#ifdef PLATFORM_LINUX /* TODO: add condition wating interface for other os */
	init_completion(&sctx->done);
#endif
	sctx->status = RTW_SCTX_SUBMITTED;
}

int rtw_sctx_wait(struct submit_ctx *sctx, const char *msg)
{
	int ret = _FAIL;
	unsigned long expire; 
	int status = 0;

#ifdef PLATFORM_LINUX
	expire= sctx->timeout_ms ? msecs_to_jiffies(sctx->timeout_ms) : MAX_SCHEDULE_TIMEOUT;
	if (!wait_for_completion_timeout(&sctx->done, expire)) {
		/* timeout, do something?? */
		status = RTW_SCTX_DONE_TIMEOUT;
		DBG_871X("%s timeout: %s\n", __func__, msg);
	} else {
		status = sctx->status;
	}
#endif

	if (status == RTW_SCTX_DONE_SUCCESS) {
		ret = _SUCCESS;
	}

	return ret;
}

bool rtw_sctx_chk_waring_status(int status)
{
	switch(status) {
	case RTW_SCTX_DONE_UNKNOWN:
	case RTW_SCTX_DONE_BUF_ALLOC:
	case RTW_SCTX_DONE_BUF_FREE:

	case RTW_SCTX_DONE_DRV_STOP:
	case RTW_SCTX_DONE_DEV_REMOVE:
		return _TRUE;
	default:
		return _FALSE;
	}
}

void rtw_sctx_done_err(struct submit_ctx **sctx, int status)
{
	if (*sctx) {
		if (rtw_sctx_chk_waring_status(status))
			DBG_871X("%s status:%d\n", __func__, status);
		(*sctx)->status = status;
		#ifdef PLATFORM_LINUX
		complete(&((*sctx)->done));
		#endif
		*sctx = NULL;
	}
}

void rtw_sctx_done(struct submit_ctx **sctx)
{
	rtw_sctx_done_err(sctx, RTW_SCTX_DONE_SUCCESS);
}


s32 rtw_init_xmit_freebuf(struct xmit_priv *pxmitpriv)
{
	struct xmit_buf *pxmitbuf;
	int i;
	pxmitpriv->pallocated_freebuf = rtw_zvmalloc(NR_XMITBUFF*sizeof(struct xmit_buf));
	if(pxmitpriv->pallocated_freebuf==NULL)
	{
		DBG_871X("%s: pallocated_freebuf failed!\n", __FUNCTION__);
		return _FAIL;
	}
	pxmitpriv->xmit_freebuf = (u8 *)N_BYTE_ALIGMENT((SIZE_PTR)(pxmitpriv->pallocated_freebuf), 4);
	pxmitpriv->pallocated_pdata = rtw_zmalloc(NR_XMITBUFF*MAX_XMITBUF_SZ);
	if(pxmitpriv->pallocated_pdata==NULL)
	{
		rtw_vmfree(pxmitpriv->pallocated_freebuf, NR_XMITBUFF*sizeof(struct xmit_buf));
		DBG_871X("%s: pallocated_pdata failed!\n", __FUNCTION__);
		return _FAIL;
	}
	pxmitpriv->xmit_pdata = (u8 *)N_BYTE_ALIGMENT((SIZE_PTR)(pxmitpriv->pallocated_pdata), XMITBUF_ALIGN_SZ);
	pxmitbuf = (struct xmit_buf *)pxmitpriv->xmit_freebuf;
	for (i = 0; i < NR_XMITBUFF; i++)
	{
		_rtw_init_listhead(&(pxmitbuf->list));

		pxmitbuf->pdata = pxmitpriv->xmit_pdata+i*MAX_XMITBUF_SZ;
		pxmitbuf->pkt_len=0;
 		pxmitbuf->ptxdesc = (PTXDESC_8195A)pxmitbuf->pdata;
		pxmitbuf->patcmd = (PAT_CMD_DESC)(pxmitbuf->pdata+SIZE_TX_DESC_8195a);
		pxmitbuf->ptxdesc->offset = SIZE_TX_DESC_8195a;
		pxmitbuf->patcmd->offset = SIZE_AT_CMD_DESC;

		rtw_list_insert_tail(&(pxmitbuf->list), &(pxmitpriv->free_xmit_queue.queue));

		pxmitbuf++;
	}
	
	return _SUCCESS;
}
s32 rtw_init_xmit_priv(PADAPTER padapter)
{
	struct xmit_priv *pxmitpriv = &padapter->xmitpriv;
	_rtw_init_queue(&pxmitpriv->free_xmit_queue);
	_rtw_init_queue(&pxmitpriv->xmitbuf_pending_queue);
	_rtw_init_sema(&pxmitpriv->xmit_sema, 0);
	//_rtw_init_sema(&padapter->XmitTerminateSema, 0);
	if(rtw_init_xmit_freebuf(pxmitpriv) == _FAIL)
	{
		return _FAIL;
	}
	return _SUCCESS;
}
void rtw_free_xmit_priv(PADAPTER padapter)
{
	struct xmit_priv *pxmitpriv = &padapter->xmitpriv;
	if(pxmitpriv->pallocated_pdata)
		rtw_mfree(pxmitpriv->pallocated_pdata, NR_XMITBUFF*MAX_XMITBUF_SZ);
	if(pxmitpriv->pallocated_freebuf)
		rtw_vmfree(pxmitpriv->pallocated_freebuf, NR_XMITBUFF*sizeof(struct xmit_buf));
}
struct xmit_buf *rtw_alloc_xmitbuf(PADAPTER padapter)//(_queue *pfree_xmit_queue)
{
	/*
		Please remember to use all the osdep_service api,
		and lock/unlock or _enter/_exit critical to protect 
		pfree_xmit_queue
	*/

	_irqL irqL;
	struct xmit_buf *pxmitbuf = NULL;
	struct xmit_priv *pxmitpriv = &padapter->xmitpriv;
	_list *plist, *phead;
	_queue *pfree_xmit_queue = &pxmitpriv->free_xmit_queue;

_func_enter_;

	_enter_critical_bh(&pfree_xmit_queue->lock, &irqL);

	if (_rtw_queue_empty(pfree_xmit_queue) == _TRUE) {
		DBG_871X("rtw_alloc_xmitframe failed!\n");
		pxmitbuf=  NULL;
	} else {
		phead = get_list_head(pfree_xmit_queue);

		plist = get_next(phead);

		pxmitbuf = LIST_CONTAINOR(plist, struct xmit_buf, list);

		rtw_list_delete(&(pxmitbuf->list));
	}

	_exit_critical_bh(&pfree_xmit_queue->lock, &irqL);

_func_exit_;

	return pxmitbuf;
}
s32 rtw_free_xmitbuf(PADAPTER padapter, struct xmit_buf *pxmitbuf)
{	
	_irqL irqL;
	_queue *queue = NULL;
	struct xmit_priv *pxmitpriv = &padapter->xmitpriv;
_func_enter_;	
	queue = &pxmitpriv->free_xmit_queue;
	if (pxmitbuf == NULL) {
		DBG_871X("======rtw_free_xmitbuf():pxmitbuf==NULL!!!!!!!!!!\n");
		goto exit;
	}

	_enter_critical_bh(&queue->lock, &irqL);

	rtw_list_delete(&pxmitbuf->list);	
	rtw_list_insert_tail(&pxmitbuf->list, get_list_head(queue));

	_exit_critical_bh(&queue->lock, &irqL);

exit:

_func_exit_;

	return _SUCCESS;
}

s32 check_pending_xmitbuf(PADAPTER padapter)
{
	_irqL irql;
	_queue *pqueue;
	s32	ret = _FALSE;
	struct xmit_priv *pxmitpriv = &padapter->xmitpriv;

	pqueue = &pxmitpriv->xmitbuf_pending_queue;

	_enter_critical_bh(&pqueue->lock, &irql);

	if(_rtw_queue_empty(pqueue) == _FALSE)
		ret = _TRUE;

	_exit_critical_bh(&pqueue->lock, &irql);

	return ret;
}
struct xmit_buf* rtw_dequeue_xmitbuf(PADAPTER padapter)
{
	_irqL irqL;
	_list *plist, *phead;
	struct xmit_buf *pxmitbuf = NULL;
	_queue *pframe_queue = NULL;
	struct xmit_priv *pxmitpriv = &padapter->xmitpriv;
	pframe_queue = &pxmitpriv->xmitbuf_pending_queue;
_enter_critical_bh(&pframe_queue->lock, &irqL);
	phead = get_list_head(pframe_queue);
	plist = get_next(phead);
	if(plist != phead)
	{
		pxmitbuf = LIST_CONTAINOR(plist, struct xmit_buf, list);
		rtw_list_delete(&pxmitbuf->list);
	}
_exit_critical_bh(&pframe_queue->lock, &irqL);
	return pxmitbuf;
}
s32 rtl8195a_dequeue_writeport(PADAPTER padapter)
{
	struct xmit_buf *pxmitbuf;

	pxmitbuf = rtw_dequeue_xmitbuf(padapter);
	if(pxmitbuf == NULL)
		return _TRUE;
	rtw_write_port(padapter, WLAN_TX_FIFO_DEVICE_ID, pxmitbuf->pkt_len, pxmitbuf->pdata);
	rtw_free_xmitbuf(padapter, pxmitbuf);
	return _FAIL;
}
s32 rtw_hal_xmit_handler(PADAPTER padapter)
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
	is_queue_empty=rtl8195a_dequeue_writeport(padapter);
	}while(!is_queue_empty);
	return _SUCCESS;
}
int rtw_xmit_entry(struct sk_buff *pkt, struct net_device *pnetdev)
{
	int ret = 0;
	PADAPTER padapter;
	struct xmit_buf *pxmitbuf;
	struct xmit_priv *pxmitpriv;
	_irqL irqL;

	padapter = (PADAPTER)rtw_netdev_priv(pnetdev);
	pxmitpriv = &padapter->xmitpriv;
#ifdef GET_SYS_TIME
#include <linux/time.h>
extern struct timeval time_out;
do_gettimeofday(&time_out);
#endif
	//enqueue pkt
	pxmitbuf = rtw_alloc_xmitbuf(padapter);
	if(!pxmitbuf)
	{
		DBG_871X("%s(): pxmitbuf allocated failed!\n", __FUNCTION__);
		pnetdev->stats.tx_dropped = (++(padapter->stats.tx_dropped));
		return ret;
	}
	pxmitbuf->pkt_len = pkt->len+SIZE_AT_CMD_DESC+ SIZE_TX_DESC_8195a;
	pxmitbuf->ptxdesc->txpktsize=pkt->len+SIZE_AT_CMD_DESC;
	pxmitbuf->patcmd->datatype = 0;
	pxmitbuf->patcmd->pktsize = pkt->len;
	_rtw_memcpy(pxmitbuf->pdata+sizeof(TX_DESC)+sizeof(AT_CMD_DESC), pkt->data, pkt->len);	
_enter_critical_bh(&pxmitpriv->xmitbuf_pending_queue.lock, &irqL);
	rtw_list_insert_tail(&pxmitbuf->list, get_list_head(&pxmitpriv->xmitbuf_pending_queue));
_exit_critical_bh(&pxmitpriv->xmitbuf_pending_queue.lock, &irqL);
	pnetdev->stats.tx_packets=(++(padapter->stats.tx_packets));
	pnetdev->stats.tx_bytes=(padapter->stats.tx_bytes+=pkt->len);
	_rtw_up_sema(&pxmitpriv->xmit_sema);
	if(pkt)
		_rtw_skb_free(pkt);
	return ret;
}

