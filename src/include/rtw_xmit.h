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
	u16 pkt_len;
	u8 *pdata;
	PTXDESC_8195A ptxdesc;
	PAT_CMD_DESC patcmd;
};
struct xmit_priv
{
	_lock lock;
	_sema xmit_sema;
	void *xmitThread;
	_queue free_xmit_queue;
	_queue xmitbuf_pending_queue;	
	u8 *pallocated_freebuf;
	u8 *xmit_freebuf;
	u8 *pallocated_pdata;
	u8 *xmit_pdata;
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
s32 rtw_hal_xmit_handler(PADAPTER padapter);
int rtw_xmit_entry(struct sk_buff *pkt, struct net_device *pnetdev);
#endif
