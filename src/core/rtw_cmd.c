#include "../include/autoconf.h"
#include "../include/rtw_debug.h"
#include "../include/drv_types.h"
#include "../include/rtw_xmit.h"
#include "../include/rtw_ioctl.h"
#include "../include/8195_desc.h"
#include "../include/8195_sdio_reg.h"
#include "../include/sdio_ops.h"
#include "../include/rtw_cmd.h"
/*
void rtw_sctx_init(struct completion *done)
{
	init_completion(done);
}
void rtw_sctx_complete(struct completion *done)
{
	complete(done);
}
s8 rtw_sctx_wait_timeout(struct completion *done, unsigned int timeout_ms)
{
	unsigned long expire;
	expire= timeout_ms ? msecs_to_jiffies(timeout_ms) : MAX_SCHEDULE_TIMEOUT;
	if(!wait_for_completion_timeout(done, expire))
	{	
		printk("%s timeout!\n", __FUNCTION__);
		return _FAIL;
	}
	return _SUCCESS;
}
*/
/*
Caller and the rtw_cmd_thread can protect cmd_q by spin_lock.
No irqsave is necessary.
*/
s32	rtw_init_cmd_priv (PADAPTER padapter)
{
	s32 res=_SUCCESS;
	struct cmd_priv *pcmdpriv = &padapter->cmdpriv;	
_func_enter_;	

	_rtw_init_sema(&(pcmdpriv->cmd_sema), 0);
	
	_rtw_init_queue(&(pcmdpriv->cmd_queue));
	
	pcmdpriv->cmd_allocated_buf = rtw_zmalloc(MAX_CMDSZ + CMDBUFF_ALIGN_SZ);
	
	if (pcmdpriv->cmd_allocated_buf == NULL){
		res= _FAIL;
		goto exit;
	}
	
	pcmdpriv->cmd_buf = pcmdpriv->cmd_allocated_buf  +  CMDBUFF_ALIGN_SZ - ( (SIZE_PTR)(pcmdpriv->cmd_allocated_buf) & (CMDBUFF_ALIGN_SZ-1));

	_rtw_mutex_init(&pcmdpriv->sctx_mutex);

exit:
	
_func_exit_;	  

	return res;
	
}	


void rtw_free_cmd_priv (PADAPTER padapter)
{
	struct cmd_priv *pcmdpriv = &padapter->cmdpriv;
_func_enter_;

	if(pcmdpriv){
		_rtw_spinlock_free(&(pcmdpriv->cmd_queue.lock));
		_rtw_free_sema(&(pcmdpriv->cmd_sema));

		if (pcmdpriv->cmd_allocated_buf)
			rtw_mfree(pcmdpriv->cmd_allocated_buf, MAX_CMDSZ + CMDBUFF_ALIGN_SZ);

		_rtw_mutex_free(&pcmdpriv->sctx_mutex);
	}
_func_exit_;		
}


void rtw_free_cmd_obj(struct cmd_obj *pcmd)
{
_func_enter_;

		//free parmbuf in cmd_obj
	rtw_mfree((unsigned char*)pcmd->parmbuf, pcmd->cmdsz);
#if 0	
	if(pcmd->rsp!=NULL)
	{
		if(pcmd->rspsz!= 0)
		{
			//free rsp in cmd_obj
			rtw_mfree((unsigned char*)pcmd->rsp, pcmd->rspsz);
		}	
	}	
#endif
	//free cmd_obj
	rtw_mfree((unsigned char*)pcmd, sizeof(struct cmd_obj));
	
_func_exit_;		
}

/*
Calling Context:

rtw_enqueue_cmd can only be called between kernel thread, 
since only spin_lock is used.

ISR/Call-Back functions can't call this sub-function.

*/

s32	rtw_enqueue_cmd(_queue *queue, struct cmd_obj *obj)
{
	_irqL irqL;

_func_enter_;

	if (obj == NULL)
		goto exit;

	//_enter_critical_bh(&queue->lock, &irqL);
	_enter_critical(&queue->lock, &irqL);	

	rtw_list_insert_tail(&obj->list, &queue->queue);

	//_exit_critical_bh(&queue->lock, &irqL);	
	_exit_critical(&queue->lock, &irqL);

exit:	

_func_exit_;

	return _SUCCESS;
}

struct	cmd_obj	*rtw_dequeue_cmd(_queue *queue)
{
	_irqL irqL;
	struct cmd_obj *obj;

_func_enter_;

	_enter_critical(&queue->lock, &irqL);
	if (rtw_is_list_empty(&(queue->queue)))
		obj = NULL;
	else
	{
		obj = LIST_CONTAINOR(get_next(&(queue->queue)), struct cmd_obj, list);
		rtw_list_delete(&obj->list);
	}
	_exit_critical(&queue->lock, &irqL);

_func_exit_;	

	return obj;
}

s32 rtw_cmd_dequeue_writeport(PADAPTER padapter)
{
	struct cmd_obj *pcmdobj;
	u16 len = 0;
	PTXDESC_8195A ptxdesc;
	PAT_CMD_DESC patcmddesc;
	struct cmd_priv *pcmdpriv = &padapter->cmdpriv;

	pcmdobj = rtw_dequeue_cmd(&pcmdpriv->cmd_queue);
	if(pcmdobj == NULL)
		return _TRUE;
	len = SIZE_TX_DESC_8195a + SIZE_AT_CMD_DESC + pcmdobj->cmdsz;
	ptxdesc = (PTXDESC_8195A)pcmdpriv->cmd_buf;
	ptxdesc->txpktsize = SIZE_AT_CMD_DESC + pcmdobj->cmdsz;
	ptxdesc->offset = SIZE_TX_DESC_8195a;
	patcmddesc = (PAT_CMD_DESC)(pcmdpriv->cmd_buf + SIZE_TX_DESC_8195a);
	patcmddesc->datatype = MNGMT_FRAME;
	patcmddesc->offset = SIZE_AT_CMD_DESC;
	patcmddesc->pktsize = pcmdobj->cmdsz;
	
	if(pcmdobj->parmbuf)
	{
		_rtw_memcpy(pcmdpriv->cmd_buf+SIZE_TX_DESC_8195a+SIZE_AT_CMD_DESC, pcmdobj->parmbuf, pcmdobj->cmdsz);
	}
	rtw_write_port(padapter, WLAN_TX_FIFO_DEVICE_ID, len, pcmdpriv->cmd_buf);
	rtw_free_cmd_obj(pcmdobj);
	return _FAIL;
}

s32 rtw_cmd_handler(PADAPTER padapter)
{
	s32 ret;
	u8 is_queue_empty ;
	struct cmd_priv *pcmdpriv = &padapter->cmdpriv;
	ret = _rtw_down_sema(&pcmdpriv->cmd_sema);
	if(ret == _FAIL)
		return _FAIL;
	do{
	is_queue_empty=rtw_cmd_dequeue_writeport(padapter);
	}while(!is_queue_empty);
	return _SUCCESS;
}

int rtw_cmd_thread(void *context)
{
	s32 err;
	PADAPTER padapter;
	//DBG_871X("%s=======>\n", __FUNCTION__);
	padapter = (PADAPTER)context;
	thread_enter("RTW_CMD_THREAD");
	do{
		err = rtw_cmd_handler(padapter);
		flush_signals_thread();
	}while((_SUCCESS == err)&&(padapter->cmdpriv->cmdThread));
	thread_exit();
}