#ifndef __RTW_RECV_H__
#define __RTW_RECV_H__

#include "drv_types.h"
//#include "sdio_ops.h"

#ifdef PLATFORM_LINUX//PLATFORM_LINUX /PLATFORM_BSD

	#ifdef CONFIG_SINGLE_RECV_BUF
		#define NR_RECVBUFF (1)
	#else
		#if defined(CONFIG_GSPI_HCI)
			#define NR_RECVBUFF (32)
		#elif defined(CONFIG_SDIO_HCI)
			#define NR_RECVBUFF (8)	
		#else
			#define NR_RECVBUFF (8)
		#endif	
	#endif //CONFIG_SINGLE_RECV_BUF

	#define NR_PREALLOC_RECV_SKB (8)	
#endif


struct recv_priv
{
	_lock lock;
	PADAPTER adapter;
	u8 *pallocated_recv_buf;
	u8 *precv_buf;

#ifdef CONFIG_USB_HCI
		//u8 *pallocated_urb_buf;
		_sema allrxreturnevt;
//		uint	ff_hwaddr;
		u8	rx_pending_cnt;
	
#ifdef CONFIG_USB_INTERRUPT_IN_PIPE
#ifdef PLATFORM_LINUX
		PURB	int_in_urb;
#endif
	
		u8	*int_in_buf;
#endif //CONFIG_USB_INTERRUPT_IN_PIPE
	
#endif

#if defined(PLATFORM_LINUX) || defined(PLATFORM_FREEBSD)
#ifdef PLATFORM_FREEBSD
	struct task recv_tasklet;
#else
	struct tasklet_struct recv_tasklet;
#endif
#endif

	struct sk_buff_head free_recv_skb_queue;
	struct sk_buff_head rx_skb_queue;

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
	u8	*phead;
	u8	*pdata;
	u8	*ptail;
	u8	*pend;
	
	u8	*pbuf;
	u8	*pallocated_buf;
	u32 ref_cnt;

	
#ifdef CONFIG_USB_HCI
	
	#if defined(PLATFORM_OS_XP)||defined(PLATFORM_LINUX)||defined(PLATFORM_FREEBSD)
		PURB	purb;
		dma_addr_t dma_transfer_addr;	/* (in) dma addr for transfer_buffer */
		u32 alloc_sz;
	#endif
	
	#ifdef PLATFORM_OS_XP
		PIRP		pirp;
	#endif
	
	#ifdef PLATFORM_OS_CE
		USB_TRANSFER	usb_transfer_read_port;
	#endif
	
		u8	irp_pending;
		int  transfer_len;
	
#endif
#ifdef PLATFORM_LINUX
	_pkt *pskb;
#endif
};


__inline static u8 *recvbuf_push(struct recv_buf *precvbuf, sint sz)
{
	// append data before rx_data

	/* add data to the start of recv_frame
 *
 *      This function extends the used data area of the recv_frame at the buffer
 *      start. rx_data must be still larger than rx_head, after pushing.
 */

	if(precvbuf==NULL)
		return NULL;


	precvbuf->pdata -= sz ;
	if( precvbuf->pdata < precvbuf->phead)
	{
		precvbuf->pdata += sz ;
		return NULL;
	}

	precvbuf->len +=sz;

	return precvbuf->pdata;

}


__inline static u8 *recvbuf_pull(struct recv_buf *precvbuf, sint sz)
{
	// rx_data += sz; move rx_data sz bytes  hereafter

	//used for extract sz bytes from rx_data, update rx_data and return the updated rx_data to the caller


	if(precvbuf==NULL)
		return NULL;


	precvbuf->pdata += sz;

	if(precvbuf->pdata > precvbuf->ptail)
	{
		precvbuf->pdata -= sz;
		return NULL;
	}

	precvbuf->len -=sz;

	return precvbuf->pdata;

}

__inline static u8 *recvbuf_put(struct recv_buf *precvbuf, sint sz)
{
	// rx_tai += sz; move rx_tail sz bytes  hereafter

	//used for append sz bytes from ptr to rx_tail, update rx_tail and return the updated rx_tail to the caller
	//after putting, rx_tail must be still larger than rx_end.
 	unsigned char * prev_rx_tail;

	if(precvbuf==NULL)
		return NULL;

	prev_rx_tail = precvbuf->ptail;

	precvbuf->ptail += sz;

	if(precvbuf->ptail > precvbuf->pend)
	{
		precvbuf->ptail -= sz;
		return NULL;
	}

	precvbuf->len +=sz;

	return precvbuf->ptail;

}



__inline static u8 *recvbuf_pull_tail(struct recv_buf *precvbuf, sint sz)
{
	// rmv data from rx_tail (by yitsen)

	//used for extract sz bytes from rx_end, update rx_end and return the updated rx_end to the caller
	//after pulling, rx_end must be still larger than rx_data.

	if(precvbuf==NULL)
		return NULL;

	precvbuf->ptail -= sz;

	if(precvbuf->ptail < precvbuf->pdata)
	{
		precvbuf->ptail += sz;
		return NULL;
	}

	precvbuf->len -=sz;

	return precvbuf->ptail;

}

s32 rtw_init_recv_priv(_adapter *padapter);
void rtw_free_recv_priv(_adapter *padapter);
s32 rtw_enqueue_recvbuf(struct recv_buf *precvbuf, _queue *queue);
struct recv_buf *rtw_dequeue_recvbuf (_queue *queue);
#endif
