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

#define _HAL_INTF_C_
#include "drv_types.h"
#include "hal_intf.h"
uint	 rtw_hal_init(_adapter *padapter) 
{
	if(padapter->HalFunc.hal_init)
		return padapter->HalFunc.hal_init(padapter);

	return _FAIL;	
}
uint	 rtw_hal_deinit(_adapter *padapter) 
{
	if(padapter->HalFunc.hal_deinit)
		return padapter->HalFunc.hal_deinit(padapter);

	return _FAIL;	
}
s32	rtw_hal_xmit(PADAPTER padapter, struct xmit_buf *pxmitbuf)
{
	if(padapter->HalFunc.hal_xmit)
		return padapter->HalFunc.hal_xmit(padapter, pxmitbuf);

	return _FAIL;	
}
s32	rtw_hal_mgnt_xmit(PADAPTER padapter, struct cmd_obj *pcmd)
{
	if(padapter->HalFunc.hal_mgnt_xmit)
		return padapter->HalFunc.hal_mgnt_xmit(padapter, pcmd);

	return _FAIL;	
}
s32 rtw_hal_xmit_thread_handler(_adapter *padapter)
{
	if(padapter->HalFunc.xmit_thread_handler)
		return padapter->HalFunc.xmit_thread_handler(padapter);
	return _FAIL;
}
void rtw_hal_enable_interrupt(_adapter *padapter)
{
	if (padapter->HalFunc.enable_interrupt)
		padapter->HalFunc.enable_interrupt(padapter);
	else 
		DBG_871X("%s: HalFunc.enable_interrupt is NULL!\n", __FUNCTION__);
	
}
void rtw_hal_disable_interrupt(_adapter *padapter)
{
	if (padapter->HalFunc.disable_interrupt)
		padapter->HalFunc.disable_interrupt(padapter);
	else 
		DBG_871X("%s: HalFunc.disable_interrupt is NULL!\n", __FUNCTION__);	
}