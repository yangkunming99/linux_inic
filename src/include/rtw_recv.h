#ifndef __RTW_RECV_H__
#define __RTW_RECV_H__
#include "sdio_ops.h"

#define NR_RECVBUFF (128)
struct recv_priv
{
	_lock lock;
	PADAPTER adapter;
	u8 *pallocated_recv_buf;
	u8 *precv_buf;
	struct tasklet_struct recv_tasklet;
	u32 free_recv_buf_queue_cnt;
	_queue free_recv_buf_queue;
	_queue recv_buf_pending_queue;
};
struct recv_buf
{
	_list list;
	PADAPTER adapter;
	u32	len;
	struct sk_buff *pskb;
};
s32 rtw_init_recv_priv(PADAPTER padapter);
void rtw_free_recv_priv(PADAPTER padapter);
s32 rtw_enqueue_recvbuf(struct recv_buf *precvbuf, _queue *queue);
struct recv_buf *rtw_dequeue_recvbuf (_queue *queue);
#endif
