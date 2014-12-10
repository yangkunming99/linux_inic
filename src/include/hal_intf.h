/******************************************************************************
 *
 * Copyright(c) 2007 - 2012 Realtek Corporation. All rights reserved.
 *                                        
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110, USA
 *
 *
 ******************************************************************************/
#ifndef __HAL_INTF_H__
#define __HAL_INTF_H__
#include "autoconf.h"
#include "basic_types.h"
#include "drv_types.h"
enum RTL871X_HCI_TYPE {
	RTW_USB 	= BIT0,
	RTW_SDIO 	= BIT1,
};
struct hal_ops {
	s32	(*hal_xmit)(_adapter *padapter, struct xmit_buf *pxmitbuf);
	s32	(*hal_mgnt_xmit)(_adapter *padapter, struct cmd_obj *pcmd);
	s32 (*xmit_thread_handler)(_adapter *padapter);
};
s32	rtw_hal_xmit(PADAPTER padapter, struct xmit_buf *pxmitbuf);
s32	rtw_hal_mgnt_xmit(PADAPTER padapter, struct cmd_obj *pcmd);
s32 rtw_hal_xmit_thread_handler(_adapter *padapter);
#endif
