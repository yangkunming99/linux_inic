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
#include "rtl8195a_hal.h"
#include "8195_sdio_reg.h"


static void HalRxAggr8195ASdio(PADAPTER padapter)
{
	u8	valueTimeout;
	u8	valueBDCount;

	// 2010.04.27 hpfan
	// Adjust RxAggrTimeout to close to zero disable RxAggr, suggested by designer
	// Timeout value is calculated by 34 / (2^n)
	valueTimeout = 0x0f;
	valueBDCount = 0x01;

	rtw_write8(padapter, SDIO_REG_RX_AGG_CFG+1, valueTimeout | 0x80);
	rtw_write8(padapter, SDIO_REG_RX_AGG_CFG, valueBDCount);
}

void _initSdioAggregationSetting(PADAPTER padapter)
{

	// Rx aggregation setting
	HalRxAggr8195ASdio(padapter);

}


void _InitInterrupt(PADAPTER padapter)
{

	//HISR write one to clear
	rtw_write32(padapter, SDIO_REG_HISR, 0xFFFFFFFF);
	
	// HIMR - turn all off
	rtw_write32(padapter, SDIO_REG_HIMR, 0);

	//
	// Initialize and enable SDIO Host Interrupt.
	//
	InitInterrupt8195ASdio(padapter);
	
}

static u32 rtl8195as_hal_init(PADAPTER padapter){
	u8 res = _SUCCESS;
//	u8 value8;
	//
	// Configure SDIO TxRx Control to enable Rx DMA timer masking.
	// 2010.02.24.
	//
//	value8 = SdioLocalCmd52Read1Byte(padapter, SDIO_REG_TX_CTRL);
//	SdioLocalCmd52Write1Byte(padapter, SDIO_REG_TX_CTRL, 0x02);

//	rtw_write8(padapter, SDIO_LOCAL_OFFSET|SDIO_REG_HRPWM, 0);

#ifdef CONFIG_FWDL
	res = rtl8195a_FirmwareDownload(padapter,_FAIL);
#endif
	_initSdioAggregationSetting(padapter);
	_InitInterrupt(padapter);
	return res;
}

static u32 rtl8195as_hal_deinit(PADAPTER padapter){
	u8 res = _SUCCESS;
	return res;
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
