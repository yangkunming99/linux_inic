#define _RTL8195AS_RECV_C_
#include "autoconf.h"
#include "drv_types.h"
#include "8195_desc.h"
#include "8195_sdio_reg.h"
extern int rtw_recv_entry(PADAPTER padapter, struct recv_buf *precvbuf);
static void rtl8195as_recv_tasklet(void *priv)
{
	PADAPTER padapter;
	RXDESC_8195A rxdesc;
	struct recv_buf *precvbuf;
	struct recv_priv *precvpriv;
	padapter = (PADAPTER)priv;
	precvpriv = &padapter->recvpriv;
	do {
		precvbuf = rtw_dequeue_recvbuf(&precvpriv->recv_buf_pending_queue);
		if (NULL == precvbuf) break;
		_rtw_memcpy(&rxdesc, precvbuf->pdata, SIZE_RX_DESC_8195a);
		//remove the sdio header
		skb_pull(precvbuf->pskb, rxdesc.offset);
		precvbuf->pdata = precvbuf->pskb->data;
		if (rtw_recv_entry(padapter, precvbuf) != _SUCCESS)
		{
			RT_TRACE(_module_rtl871x_recv_c_, _drv_err_, ("%s: rtw_recv_entry(padapter, precvbuf) != _SUCCESS\n",__FUNCTION__));
		}

	} while (1);

}

/*
 * Initialize recv private variable for hardware dependent
 * 1. recv buf
 * 2. recv tasklet
 *
 */
s32 rtl8195as_init_recv_priv(PADAPTER padapter)
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
	     (void(*)(unsigned long))rtl8195as_recv_tasklet,
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
void rtl8195as_free_recv_priv(PADAPTER padapter)
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


