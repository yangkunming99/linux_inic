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
#define _USB_HALINIT_C_
#include "autoconf.h"
#include "usb_ops.h"
#include "rtw_debug.h"
#include "rtl8195a_xmit.h"
#include "rtl8195a_recv.h"

unsigned int rtl8195au_inirp_init(_adapter * padapter)
{	
	u8 i;	
	struct recv_buf *precvbuf;
	uint	status;
	struct recv_priv *precvpriv = &(padapter->recvpriv);
	u32 (*_read_port)(_adapter * padapter, u32 addr, u32 cnt, u8 *pmem);
#ifdef CONFIG_USB_INTERRUPT_IN_PIPE
	u32 (*_read_interrupt)(_adapter * padapter, u32 addr);
#endif

_func_enter_;

	_read_port = padapter->io_ops._read_port;

	status = _SUCCESS;

	RT_TRACE(_module_hci_hal_init_c_,_drv_info_,("===> usb_inirp_init \n"));	
		
	//issue Rx irp to receive data	
	precvbuf = (struct recv_buf *)precvpriv->precv_buf;	
	for(i=0; i<NR_RECVBUFF; i++)
	{
		if(_read_port(padapter, USB_READ_ADD, 0, (unsigned char *)precvbuf) == _FALSE )
		{
			RT_TRACE(_module_hci_hal_init_c_,_drv_err_,("usb_rx_init: usb_read_port error \n"));
			status = _FAIL;
			goto exit;
		}
		
		precvbuf++;		
		precvpriv->free_recv_buf_queue_cnt--;
	}

#ifdef CONFIG_USB_INTERRUPT_IN_PIPE
	_read_interrupt = padapter->io_ops._read_interrupt;
	if(_read_interrupt(padapter, RECV_INT_IN_ADDR) == _FALSE )
	{
		RT_TRACE(_module_hci_hal_init_c_,_drv_err_,("usb_rx_init: usb_read_interrupt error \n"));
		status = _FAIL;
	}
#endif

exit:
	
	RT_TRACE(_module_hci_hal_init_c_,_drv_info_,("<=== usb_inirp_init \n"));

_func_exit_;

	return status;
}

unsigned int rtl8195au_inirp_deinit(_adapter * padapter);
unsigned int rtl8195au_inirp_deinit(_adapter * padapter)
{	

	RT_TRACE(_module_hci_hal_init_c_,_drv_info_,("\n ===> usb_rx_deinit \n"));
	
	rtw_read_port_cancel(padapter);

	RT_TRACE(_module_hci_hal_init_c_,_drv_info_,("\n <=== usb_rx_deinit \n"));
	
	return _SUCCESS;
}


void rtl8195au_set_hal_ops(PADAPTER padapter)
{
	struct hal_ops *pHalFunc = &padapter->HalFunc;
_func_enter_;

	pHalFunc->hal_init = &rtl8195au_hal_init;
	pHalFunc->hal_deinit = &rtl8195au_hal_deinit;
	
	pHalFunc->inirp_init = &rtl8195au_inirp_init;
	pHalFunc->inirp_deinit = &rtl8195au_inirp_deinit;

	pHalFunc->hal_xmit = &rtl8195au_hal_xmit;
	pHalFunc->hal_mgnt_xmit = &rtl8195au_hal_mgnt_xmit;
	pHalFunc->xmit_thread_handler = &rtl8195au_hal_xmit_handler;

	pHalFunc->init_xmit_priv = &rtl8195au_init_xmit_priv;
	pHalFunc->free_xmit_priv = &rtl8195au_free_xmit_priv;

	pHalFunc->init_recv_priv = &rtl8195au_init_recv_priv;
	pHalFunc->free_recv_priv = &rtl8195au_free_recv_priv;

_func_exit_;
}


