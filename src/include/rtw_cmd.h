#ifndef __RTW_CMD_H__
#define __RTW_CMD_H__
#include "drv_types.h"

#define MAX_CMDSZ	(1024)

#define CMDBUFF_ALIGN_SZ (512)

struct cmd_obj {
	_list list;
	u8 *parmbuf;
	u32	cmdsz;
	struct submit_ctx *sctx;
};
struct cmd_priv{
	_sema cmd_sema;
	_queue cmd_queue;
	u8 *cmd_allocated_buf;
	u8 *cmd_buf;
	void *cmdThread;
	_mutex sctx_mutex;
};
s32 rtw_init_cmd_priv (PADAPTER padapter);
void rtw_free_cmd_priv (PADAPTER padapter);
void rtw_free_cmd_obj(struct cmd_obj *pcmd);
s32	rtw_enqueue_cmd(_queue *queue, struct cmd_obj *obj);
s32 rtw_cmd_handler(PADAPTER padapter);
#endif
