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
#if defined(PLATFORM_LINUX) || defined(PLATFORM_FREEBSD)
#ifdef PLATFORM_FREEBSD
	struct task recv_tasklet;
#else
	struct tasklet_struct recv_tasklet;
#endif
#endif
	u32 free_recv_buf_queue_cnt;
	_queue free_recv_buf_queue;
	_queue recv_buf_pending_queue;

	u64	rx_bytes;
	u64	rx_pkts;
	u64	rx_drop;
};
struct recv_buf
{
	_list list;
	PADAPTER adapter;
	u32	len;
	_pkt *pskb;
};
s32 rtw_init_recv_priv(PADAPTER padapter);
void rtw_free_recv_priv(PADAPTER padapter);
s32 rtw_enqueue_recvbuf(struct recv_buf *precvbuf, _queue *queue);
struct recv_buf *rtw_dequeue_recvbuf (_queue *queue);
#endif
