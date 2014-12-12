#include "autoconf.h"
#include "rtw_debug.h"
#include "drv_types.h"
#include "rtw_xmit.h"
#include "rtw_ioctl.h"
#include "sdio_ops.h"
#include "hal_intf.h"
#include "xmit_osdep.h"
#define _RTW_XMIT_C_
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


s32 rtw_init_xmit_freebuf(struct xmit_priv *pxmitpriv, PADAPTER padapter)
{
	struct xmit_buf *pxmitbuf;
	sint	res=_SUCCESS;
	int i , j=0;
	pxmitpriv->pallocated_freebuf = rtw_zvmalloc(NR_XMITBUFF*sizeof(struct xmit_buf)+4);
	if(pxmitpriv->pallocated_freebuf==NULL)
	{
		DBG_871X("%s: pallocated_freebuf failed!\n", __FUNCTION__);
		res = _FAIL;
		goto exit;
	}
	pxmitpriv->xmit_freebuf = (u8 *)N_BYTE_ALIGMENT((SIZE_PTR)(pxmitpriv->pallocated_freebuf), 4);
/*
	pxmitpriv->pallocated_pdata = rtw_zmalloc(NR_XMITBUFF*MAX_XMITBUF_SZ);
	if(pxmitpriv->pallocated_pdata==NULL)
	{
		rtw_vmfree(pxmitpriv->pallocated_freebuf, NR_XMITBUFF*sizeof(struct xmit_buf));
		DBG_871X("%s: pallocated_pdata failed!\n", __FUNCTION__);
		return _FAIL;
	}
	pxmitpriv->xmit_pdata = (u8 *)N_BYTE_ALIGMENT((SIZE_PTR)(pxmitpriv->pallocated_pdata), XMITBUF_ALIGN_SZ);
*/
	pxmitbuf = (struct xmit_buf *)pxmitpriv->xmit_freebuf;
	for (i = 0; i < NR_XMITBUFF; i++)
	{
		_rtw_init_listhead(&(pxmitbuf->list));

		pxmitbuf->padapter = padapter;

		/* Tx buf allocation may fail sometimes, so sleep and retry. */
		if((res=rtw_os_xmit_resource_alloc(padapter, pxmitbuf,(MAX_XMITBUF_SZ + XMITBUF_ALIGN_SZ), _TRUE)) == _FAIL) {
			rtw_msleep_os(10);
			res = rtw_os_xmit_resource_alloc(padapter, pxmitbuf,(MAX_XMITBUF_SZ + XMITBUF_ALIGN_SZ), _TRUE);
			if (res == _FAIL) {
				goto free_os_resource;
			}
		}
#if defined(CONFIG_SDIO_HCI) || defined(CONFIG_GSPI_HCI)
		pxmitbuf->phead = pxmitbuf->pbuf;
		pxmitbuf->pend = pxmitbuf->pbuf + MAX_XMITBUF_SZ;
		pxmitbuf->pkt_len = 0;
		pxmitbuf->pdata = pxmitbuf->ptail = pxmitbuf->phead;
#endif

		rtw_list_insert_tail(&(pxmitbuf->list), &(pxmitpriv->free_xmit_queue.queue));

		pxmitbuf++;
	}
free_os_resource:
	if(res == _FAIL){
		pxmitbuf = (struct xmit_buf *)pxmitpriv->xmit_freebuf;
		for(j=1;j<i;j++)
		{
			rtw_os_xmit_resource_free(padapter, pxmitbuf,(MAX_XMITBUF_SZ + XMITBUF_ALIGN_SZ), _TRUE);			
			pxmitbuf++;
		}
	}		
free_xmitbuf:
	if((res == _FAIL)&&(pxmitpriv->pallocated_freebuf))
		rtw_vmfree(pxmitpriv->pallocated_freebuf, NR_XMITBUFF*sizeof(struct xmit_buf)+4);
exit:

_func_exit_;	

	return res;
}
s32 rtw_init_xmit_priv(PADAPTER padapter)
{
	struct xmit_priv *pxmitpriv = &padapter->xmitpriv;
	_rtw_init_queue(&pxmitpriv->free_xmit_queue);
	_rtw_init_queue(&pxmitpriv->xmitbuf_pending_queue);
	_rtw_init_sema(&pxmitpriv->xmit_sema, 0);
	//_rtw_init_sema(&padapter->XmitTerminateSema, 0);
	pxmitpriv->padapter = padapter;
	if(rtw_init_xmit_freebuf(pxmitpriv, padapter) == _FAIL)
	{
		return _FAIL;
	}
	return _SUCCESS;
}
void rtw_free_xmit_priv(PADAPTER padapter)
{
	int i = 0;
	struct xmit_priv *pxmitpriv = &padapter->xmitpriv;
	struct xmit_buf *pxmitbuf = (struct xmit_buf *)pxmitpriv->xmit_freebuf;
 _func_enter_;   

	rtw_hal_free_xmit_priv(padapter);

	for(i=0; i<NR_XMITBUFF; i++)
	{
		rtw_os_xmit_complete(padapter, pxmitbuf);
	
		rtw_os_xmit_resource_free(padapter, pxmitbuf,(MAX_XMITBUF_SZ + XMITBUF_ALIGN_SZ), _TRUE);
		
		pxmitbuf++;
	}	
//	if(pxmitpriv->pallocated_pdata)
//		rtw_mfree(pxmitpriv->pallocated_pdata, NR_XMITBUFF*MAX_XMITBUF_SZ);
	if(pxmitpriv->pallocated_freebuf)
		rtw_vmfree(pxmitpriv->pallocated_freebuf, NR_XMITBUFF*sizeof(struct xmit_buf)+4);

_func_exit_;
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
		DBG_871X("rtw_alloc_xmitbuf failed!\n");
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
_func_enter_;
	pqueue = &pxmitpriv->xmitbuf_pending_queue;

	_enter_critical_bh(&pqueue->lock, &irql);

	if(_rtw_queue_empty(pqueue) == _FALSE)
		ret = _TRUE;

	_exit_critical_bh(&pqueue->lock, &irql);
_func_exit_;
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
_func_enter_;
_enter_critical_bh(&pframe_queue->lock, &irqL);
	phead = get_list_head(pframe_queue);
	plist = get_next(phead);
	if(plist != phead)
	{
		pxmitbuf = LIST_CONTAINOR(plist, struct xmit_buf, list);
		rtw_list_delete(&pxmitbuf->list);
	}
_exit_critical_bh(&pframe_queue->lock, &irqL);
_func_exit_;
	return pxmitbuf;
}
thread_return rtw_xmit_thread(void *context)
{
	s32 err;
	PADAPTER padapter;
	struct xmit_priv *pxmitpriv;
	padapter = (PADAPTER)context;
	pxmitpriv = &padapter->xmitpriv;
	thread_enter("RTW_XMIT_THREAD");
	do{
		err = rtw_hal_xmit_thread_handler(padapter);
		flush_signals_thread();
	}while((_SUCCESS == err)&&(pxmitpriv->xmitThread));
	thread_exit();
}