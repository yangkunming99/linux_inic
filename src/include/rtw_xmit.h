#ifndef __RTW_XMIT_H__
#define __RTW_XMIT_H__

struct  submit_ctx{
	u32 submit_time; /* */
	u32 timeout_ms; /* <0: not synchronous, 0: wait forever, >0: up to ms waiting */
	int status; /* status for operation */
#ifdef PLATFORM_LINUX
	struct completion done;
#endif
};

enum {
	RTW_SCTX_SUBMITTED = -1,
	RTW_SCTX_DONE_SUCCESS = 0,
	RTW_SCTX_DONE_UNKNOWN,
	RTW_SCTX_DONE_TIMEOUT,
	RTW_SCTX_DONE_BUF_ALLOC,
	RTW_SCTX_DONE_BUF_FREE,
	RTW_SCTX_DONE_WRITE_PORT_ERR,
	RTW_SCTX_DONE_TX_DESC_NA,
	RTW_SCTX_DONE_TX_DENY,
	RTW_SCTX_DONE_CCX_PKT_FAIL,
	RTW_SCTX_DONE_DRV_STOP,
	RTW_SCTX_DONE_DEV_REMOVE,
	RTW_SCTX_DONE_CMD_ERROR,
};

void rtw_sctx_init(struct submit_ctx *sctx, int timeout_ms);
int rtw_sctx_wait(struct submit_ctx *sctx, const char *msg);
void rtw_sctx_done_err(struct submit_ctx **sctx, int status);
void rtw_sctx_done(struct submit_ctx **sctx);

struct xmit_buf
{
	_list list;
	PADAPTER padapter;
	_pkt *pkt;
	u16 pkt_len;
	u8 *pallocated_buf;
	u8 *pbuf;
#if defined(CONFIG_SDIO_HCI) || defined(CONFIG_GSPI_HCI)
	u8 *phead;
	u8 *pdata;
	u8 *ptail;
	u8 *pend;
#endif

	
#ifdef CONFIG_USB_HCI

#if defined(PLATFORM_OS_XP)||defined(PLATFORM_LINUX) || defined(PLATFORM_FREEBSD)
	PURB	pxmit_urb[8];
	dma_addr_t dma_transfer_addr;	/* (in) dma addr for transfer_buffer */
#endif

#ifdef PLATFORM_OS_XP
	PIRP		pxmit_irp[8];
#endif

#ifdef PLATFORM_OS_CE
	USB_TRANSFER	usb_transfer_write_port;
#endif

	u8 bpending[8];

	sint last[8];

#endif
#if defined(DBG_XMIT_BUF )|| defined(DBG_XMIT_BUF_EXT)
	u8 no;
#endif
};
struct xmit_priv
{
	_lock lock;
	PADAPTER padapter;
	_queue free_xmit_queue;
	_queue xmitbuf_pending_queue;
	
	u8 *pallocated_freebuf;
	u8 *xmit_freebuf;
	uint free_xmitbuf_cnt;

	u64	tx_bytes;
	u64	tx_pkts;
	u64	tx_drop;

	_sema xmit_sema;
	_thread_hdl_ xmitThread;
};
#define XMITBUF_ALIGN_SZ		4

#ifdef CONFIG_TX_AGGREGATION
#define MAX_XMITBUF_SZ	(20480)	// 20k
#define NR_XMITBUFF	(16)
#else
#define NR_XMITFRAME			128
#define NR_XMITBUFF				128
#define MAX_XMITBUF_SZ (1664)
#define SDIO_TX_AGG_MAX	1
#endif

s32 rtw_init_xmit_priv(PADAPTER padapter);
void rtw_free_xmit_priv(PADAPTER padapter);
struct xmit_buf *rtw_alloc_xmitbuf(PADAPTER padapter);
s32 rtw_free_xmitbuf(PADAPTER padapter, struct xmit_buf *pxmitbuf);
struct xmit_buf* rtw_dequeue_xmitbuf(PADAPTER padapter);
s32 check_pending_xmitbuf(PADAPTER padapter);
thread_return rtw_xmit_thread(void *context);
#endif
