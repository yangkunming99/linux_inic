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
 *******************************************************************************/
#define _USB_OPS_LINUX_C_

#include "basic_types.h"
#include "rtw_debug.h"
#include "osdep_service_linux.h"
#include "drv_types.h"
#include "usb_ops.h"

int usbctrl_vendorreq(PADAPTER padapter, u8 request, u16 value, u16 index, void *pdata, u16 len, u8 requesttype)
{
	struct dvobj_priv  *pdvobjpriv = adapter_to_dvobj(padapter);
//	struct pwrctrl_priv *pwrctl = dvobj_to_pwrctl(pdvobjpriv);
	struct usb_device *udev = pdvobjpriv->pusbdev;
	int status = 0;

	unsigned int pipe;
	
	u8 reqtype;
	u8 *pIo_buf;
	int vendorreq_times = 0;
#ifndef CONFIG_USB_VENDOR_REQ_BUFFER_PREALLOC
	u32 tmp_buflen=0;
	#ifdef CONFIG_USB_VENDOR_REQ_BUFFER_DYNAMIC_ALLOCATE
	u8 *tmp_buf;
	#else // use stack memory
	u8 tmp_buf[MAX_USB_IO_CTL_SIZE];
	#endif
#endif
#ifdef CONFIG_CONCURRENT_MODE
	if(padapter->adapter_type > PRIMARY_ADAPTER)
	{
		padapter = padapter->pbuddy_adapter;
		pdvobjpriv = adapter_to_dvobj(padapter);
		udev = pdvobjpriv->pusbdev;
	}
#endif


	//DBG_871X("%s %s:%d\n",__FUNCTION__, current->comm, current->pid);

	if (RTW_CANNOT_IO(padapter)){
		RT_TRACE(_module_hci_ops_os_c_,_drv_err_,("usbctrl_vendorreq:(RTW_CANNOT_IO)!!!\n"));
		status = -EPERM; 
		goto exit;
	}	


	if(len>MAX_VENDOR_REQ_CMD_SIZE){
		DBG_8192C( "[%s] Buffer len error ,vendor request failed\n", __FUNCTION__ );
		status = -EINVAL;
		goto exit;
	}	

	#ifdef CONFIG_USB_VENDOR_REQ_MUTEX
	_enter_critical_mutex(&pdvobjpriv->usb_vendor_req_mutex, NULL);
	#endif	

	
	// Acquire IO memory for vendorreq
#ifdef CONFIG_USB_VENDOR_REQ_BUFFER_PREALLOC
	pIo_buf = pdvobjpriv->usb_vendor_req_buf;
#else
	#ifdef CONFIG_USB_VENDOR_REQ_BUFFER_DYNAMIC_ALLOCATE
	tmp_buf = rtw_malloc( (u32) len + ALIGNMENT_UNIT);
	tmp_buflen =  (u32)len + ALIGNMENT_UNIT;
	#else // use stack memory
	tmp_buflen = MAX_USB_IO_CTL_SIZE;
	#endif

	// Added by Albert 2010/02/09
	// For mstar platform, mstar suggests the address for USB IO should be 16 bytes alignment.
	// Trying to fix it here.
	pIo_buf = (tmp_buf==NULL)?NULL:tmp_buf + ALIGNMENT_UNIT -((SIZE_PTR)(tmp_buf) & 0x0f );	
#endif

	if ( pIo_buf== NULL) {
		DBG_8192C( "[%s] pIo_buf == NULL \n", __FUNCTION__ );
		status = -ENOMEM;
		goto release_mutex;
	}
	
	while(++vendorreq_times<= MAX_USBCTRL_VENDORREQ_TIMES)
	{
		_rtw_memset(pIo_buf, 0, len);
		
		if (requesttype == 0x01)
		{
			pipe = usb_rcvctrlpipe(udev, 0);//read_in
			reqtype =  REALTEK_USB_VENQT_READ;		
		} 
		else 
		{
			pipe = usb_sndctrlpipe(udev, 0);//write_out
			reqtype =  REALTEK_USB_VENQT_WRITE;		
			_rtw_memcpy( pIo_buf, pdata, len);
		}		
	
		status = rtw_usb_control_msg(udev, pipe, request, reqtype, value, index, pIo_buf, len, RTW_USB_CONTROL_MSG_TIMEOUT);
		
		if ( status == len)   // Success this control transfer.
		{
			rtw_reset_continual_io_error(pdvobjpriv);
			if ( requesttype == 0x01 )
			{   // For Control read transfer, we have to copy the read data from pIo_buf to pdata.
				_rtw_memcpy( pdata, pIo_buf,  len );
			}
		}
		else { // error cases
			DBG_8192C("reg 0x%x, usb %s %u fail, status:%d value=0x%x, vendorreq_times:%d\n"
				, value,(requesttype == 0x01)?"read":"write" , len, status, *(u32*)pdata, vendorreq_times);
			
			if (status < 0) {
				if(status == (-ESHUTDOWN)	|| status == -ENODEV	)
				{			
					padapter->bSurpriseRemoved = _TRUE;
				} else {
					#ifdef DBG_CONFIG_ERROR_DETECT
					{
						HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(padapter);
						pHalData->srestpriv.Wifi_Error_Status = USB_VEN_REQ_CMD_FAIL;
					}
					#endif
				}
			}
			else // status != len && status >= 0
			{
				if(status > 0) {
					if ( requesttype == 0x01 )
					{   // For Control read transfer, we have to copy the read data from pIo_buf to pdata.
						_rtw_memcpy( pdata, pIo_buf,  len );
					}
				}
			}

			if(rtw_inc_and_chk_continual_io_error(pdvobjpriv) == _TRUE ){
				padapter->bSurpriseRemoved = _TRUE;
				break;
			}
	
		}
	
		// firmware download is checksumed, don't retry
		if( (value >= FW_START_ADDRESS ) || status == len )
			break;
	
	}

	// release IO memory used by vendorreq
	#ifdef CONFIG_USB_VENDOR_REQ_BUFFER_DYNAMIC_ALLOCATE
	rtw_mfree(tmp_buf, tmp_buflen);
	#endif

release_mutex:
	#ifdef CONFIG_USB_VENDOR_REQ_MUTEX
	_exit_critical_mutex(&pdvobjpriv->usb_vendor_req_mutex, NULL);
	#endif
exit:
	return status;

}

static void usb_write_port_complete(PURB purb)
{
	_irqL irqL;
	struct xmit_buf *pxmitbuf = (struct xmit_buf *)purb->context;
	_adapter	*padapter = pxmitbuf->padapter;
	struct xmit_priv	*pxmitpriv = &padapter->xmitpriv;		
	   
_func_enter_;

/*	
	if (RTW_CANNOT_TX(padapter))
	{
		RT_TRACE(_module_hci_ops_os_c_,_drv_err_,("usb_write_port_complete:bDriverStopped(%d) OR bSurpriseRemoved(%d)", padapter->bDriverStopped, padapter->bSurpriseRemoved));
		DBG_8192C("%s(): TX Warning! bDriverStopped(%d) OR bSurpriseRemoved(%d)\n", 
		__FUNCTION__,padapter->bDriverStopped, padapter->bSurpriseRemoved);	

		goto check_completion;
	}
*/

	if (purb->status==0) {
	
	} else {
		RT_TRACE(_module_hci_ops_os_c_,_drv_err_,("usb_write_port_complete : purb->status(%d) != 0 \n", purb->status));
		DBG_871X("###=> urb_write_port_complete status(%d)\n",purb->status);
		if((purb->status==-EPIPE)||(purb->status==-EPROTO))
		{
			//usb_clear_halt(pusbdev, purb->pipe);	
			//msleep(10);
			//sreset_set_wifi_error_status(padapter, USB_WRITE_PORT_FAIL);
		} else if (purb->status == -EINPROGRESS) {
			RT_TRACE(_module_hci_ops_os_c_,_drv_err_,("usb_write_port_complete: EINPROGESS\n"));
			goto check_completion;

		} else if (purb->status == -ENOENT) {
			DBG_871X("%s: -ENOENT\n", __func__);
			goto check_completion;
			
		} else if (purb->status == -ECONNRESET) {
			DBG_871X("%s: -ECONNRESET\n", __func__);
			goto check_completion;

		} else if (purb->status == -ESHUTDOWN) {
			RT_TRACE(_module_hci_ops_os_c_,_drv_err_,("usb_write_port_complete: ESHUTDOWN\n"));
			padapter->bDriverStopped=_TRUE;
			RT_TRACE(_module_hci_ops_os_c_,_drv_err_,("usb_write_port_complete:bDriverStopped=TRUE\n"));

			goto check_completion;
		}
		else
		{					
			padapter->bSurpriseRemoved=_TRUE;
			DBG_8192C("bSurpriseRemoved=TRUE\n");
			//rtl8192cu_trigger_gpio_0(padapter);
			RT_TRACE(_module_hci_ops_os_c_,_drv_err_,("usb_write_port_complete:bSurpriseRemoved=TRUE\n"));

			goto check_completion;
		}
	}

	#ifdef DBG_CONFIG_ERROR_DETECT
	{	
		HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(padapter);
		pHalData->srestpriv.last_tx_complete_time = rtw_get_current_time();		
	}
	#endif

check_completion:

	_enter_critical(&pxmitpriv->lock_sctx, &irqL);
	rtw_sctx_done_err(&pxmitbuf->sctx,
	purb->status ? RTW_SCTX_DONE_WRITE_PORT_ERR : RTW_SCTX_DONE_SUCCESS);
	_exit_critical(&pxmitpriv->lock_sctx, &irqL);

	rtw_free_xmitbuf(padapter, pxmitbuf);

	//if(rtw_txframes_pending(padapter))	
	{
//		tasklet_hi_schedule(&pxmitpriv->xmit_tasklet);
	}
	
_func_exit_;	
}

u32 usb_write_port(_adapter *padapter, u32 addr, u32 cnt, u8 *wmem)
{   
	unsigned int pipe;
	int status;
	u32 ret = _FAIL;
	PURB	purb = NULL;
	struct dvobj_priv	*pdvobj = padapter->dvobj;
	struct xmit_buf *pxmitbuf = (struct xmit_buf *)wmem;
	struct usb_device *pusbd = pdvobj->pusbdev;

	struct submit_ctx sctx;

	rtw_sctx_init(&sctx, 1000);
	pxmitbuf->sctx = &sctx;
	
_func_enter_;	
	RT_TRACE(_module_hci_ops_os_c_,_drv_err_,("+usb_write_port\n"));

	if (RTW_CANNOT_TX(padapter)) {
		#ifdef DBG_TX
		DBG_871X(" DBG_TX %s:%d bDriverStopped%d, bSurpriseRemoved:%d\n",__FUNCTION__, __LINE__
			,padapter->bDriverStopped, padapter->bSurpriseRemoved);
		#endif
		RT_TRACE(_module_hci_ops_os_c_,_drv_err_,("usb_write_port:( padapter->bDriverStopped ||padapter->bSurpriseRemoved )!!!\n"));
		rtw_sctx_done_err(&pxmitbuf->sctx, RTW_SCTX_DONE_TX_DENY);
		goto exit;
	}
		
	purb = pxmitbuf->pxmit_urb[0];
	pipe = pdvobj->send_bulk_Pipe;

#ifdef CONFIG_REDUCE_USB_TX_INT	
	if ( (pxmitpriv->free_xmitbuf_cnt%NR_XMITBUFF == 0)
		|| (pxmitbuf->buf_tag > XMITBUF_DATA) )
	{
		purb->transfer_flags  &=  (~URB_NO_INTERRUPT);
	} else {
		purb->transfer_flags  |=  URB_NO_INTERRUPT;
		//DBG_8192C("URB_NO_INTERRUPT ");
	}
#endif
	
	usb_fill_bulk_urb(purb, pusbd, pipe, 
       					pxmitbuf->pbuf,
              			cnt,
              			usb_write_port_complete,
              			pxmitbuf);//context is pxmitbuf

#ifdef CONFIG_USE_USB_BUFFER_ALLOC_TX
	purb->transfer_dma = pxmitbuf->dma_transfer_addr;
	purb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;
//	purb->transfer_flags |= URB_ZERO_PACKET;
#endif	// CONFIG_USE_USB_BUFFER_ALLOC_TX

#ifdef USB_PACKET_OFFSET_SZ
#if (USB_PACKET_OFFSET_SZ == 0)
	purb->transfer_flags |= URB_ZERO_PACKET;
#endif
#endif

#if 0
	if (bwritezero)
        {
            purb->transfer_flags |= URB_ZERO_PACKET;           
        }			
#endif

	status = usb_submit_urb(purb, GFP_ATOMIC);
	if (!status) {
		#ifdef DBG_CONFIG_ERROR_DETECT	
		{	
			HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(padapter);
			pHalData->srestpriv.last_tx_time = rtw_get_current_time();		
		}
		#endif
	} else {
		rtw_sctx_done_err(&pxmitbuf->sctx, RTW_SCTX_DONE_WRITE_PORT_ERR);
		DBG_871X("usb_write_port, status=%d\n", status);
		RT_TRACE(_module_hci_ops_os_c_,_drv_err_,("usb_write_port(): usb_submit_urb, status=%x\n", status));
		
		switch (status) {
		case -ENODEV:
			padapter->bDriverStopped = _TRUE;
			break;
		default:
			break;
		}
		goto exit;
	}
	ret= _SUCCESS;

	/*submit urb done, wait here until timeout or response successfully*/
	if (ret == _SUCCESS)
		ret = rtw_sctx_wait(&sctx, __func__);

//   Commented by Albert 2009/10/13
//   We add the URB_ZERO_PACKET flag to urb so that the host will send the zero packet automatically.
/*	
	if(bwritezero == _TRUE)
	{
		usb_bulkout_zero(pintfhdl, addr);
	}
*/

	RT_TRACE(_module_hci_ops_os_c_,_drv_err_,("-usb_write_port\n"));

exit:
	if (ret != _SUCCESS)
		rtw_free_xmitbuf(padapter, pxmitbuf);
_func_exit_;
	return ret;
}

