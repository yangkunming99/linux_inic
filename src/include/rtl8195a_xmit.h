#ifndef __RTL8195A_XMIT_H__
#define __RTL8195A_XMIT_H__
#include "drv_types.h"
s32 rtl8195as_init_xmit_priv(PADAPTER padapter);
void rtl8195as_free_xmit_priv(PADAPTER padapter);
s32 rtl8195as_hal_xmit_handler(PADAPTER padapter);
s32 rtl8195as_hal_xmit(PADAPTER padapter, struct xmit_buf *pxmitbuf);
s32 rtl8195as_hal_mgnt_xmit(PADAPTER padapter, struct xmit_buf *pxmitbuf);

s32 rtl8195au_init_xmit_priv(PADAPTER padapter);
void rtl8195au_free_xmit_priv(PADAPTER padapter);
s32 rtl8195au_hal_xmit_handler(PADAPTER padapter);
s32 rtl8195au_hal_xmit(PADAPTER padapter, struct xmit_buf *pxmitbuf);
s32 rtl8195au_hal_mgnt_xmit(PADAPTER padapter, struct xmit_buf *pxmitbuf);

#endif