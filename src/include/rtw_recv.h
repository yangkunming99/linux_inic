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
s32 rtw_init_recv_priv(PADAPTER padapter);
void rtw_free_recv_priv(PADAPTER padapter);
s32 rtw_enqueue_recvbuf(struct recv_buf *precvbuf, _queue *queue);
struct recv_buf *rtw_dequeue_recvbuf (_queue *queue);
#endif
