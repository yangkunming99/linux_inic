#ifndef __RTL8195A_XMIT_H__
#define __RTL8195A_XMIT_H__
#include "drv_types.h"
s32 rtl8195as_hal_xmit_handler(PADAPTER padapter);
s32 rtl8195as_hal_xmit(PADAPTER padapter, struct xmit_buf *pxmitbuf);
s32 rtl8195as_hal_mgnt_xmit(PADAPTER padapter, struct cmd_obj *pcmd);
#endif