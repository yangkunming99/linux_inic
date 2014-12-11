/******************************************************************************
 *
 * Copyright(c) 2007 - 2011 Realtek Corporation. All rights reserved.
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
#define _SDIO_HALINIT_C_
#include "autoconf.h"
#ifndef CONFIG_SDIO_HCI
#error "CONFIG_SDIO_HCI shall be on!\n"
#endif
#include "sdio_hal.h"
#include "sdio_ops.h"
#include "rtl8195a_xmit.h"
#include "rtl8195a_recv.h"

static u32 rtl8195as_hal_init(PADAPTER padapter){
	InitInterrupt8195ASdio(padapter);
	return _SUCCESS;
}

static u32 rtl8195as_hal_deinit(PADAPTER padapter){

	return _SUCCESS;
}

void rtl8195as_set_hal_ops(PADAPTER padapter)
{
	struct hal_ops *pHalFunc = &padapter->HalFunc;
_func_enter_;
	pHalFunc->hal_init = &rtl8195as_hal_init;
	pHalFunc->hal_deinit = &rtl8195as_hal_deinit;
	
	pHalFunc->init_xmit_priv = &rtl8195as_init_xmit_priv;
	pHalFunc->free_xmit_priv = &rtl8195as_free_xmit_priv;

	pHalFunc->init_recv_priv = &rtl8195as_init_recv_priv;
	pHalFunc->free_recv_priv = &rtl8195as_free_recv_priv;
	
	pHalFunc->hal_xmit = &rtl8195as_hal_xmit;
	pHalFunc->hal_mgnt_xmit = &rtl8195as_hal_mgnt_xmit;
	pHalFunc->xmit_thread_handler = &rtl8195as_hal_xmit_handler;
	
	pHalFunc->enable_interrupt = &EnableInterrupt8195ASdio;
	pHalFunc->disable_interrupt = &DisableInterrupt8195ASdio;
_func_exit_;
}