#include "basic_types.h"
#include "rtw_debug.h"
#include "osdep_service_linux.h"
#include "drv_types.h"


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
//	struct pwrctrl_priv *pwrctl = dvobj_to_pwrctl(pdvobj);
//	struct xmit_priv	*pxmitpriv = &padapter->xmitpriv;
	struct xmit_buf *pxmitbuf = (struct xmit_buf *)wmem;
//	struct xmit_frame *pxmitframe = (struct xmit_frame *)pxmitbuf->priv_data;
	struct usb_device *pusbd = pdvobj->pusbdev;
//	struct pkt_attrib *pattrib = &pxmitframe->attrib;

	struct submit_ctx sctx;

	rtw_sctx_init(&sctx, 1000);
	pxmitbuf->sctx = &sctx;
	
_func_enter_;	
	RT_TRACE(_module_hci_ops_os_c_,_drv_err_,("+usb_write_port\n"));
/*
	if (RTW_CANNOT_TX(padapter)) {
		#ifdef DBG_TX
		DBG_871X(" DBG_TX %s:%d bDriverStopped%d, bSurpriseRemoved:%d\n",__FUNCTION__, __LINE__
			,padapter->bDriverStopped, padapter->bSurpriseRemoved);
		#endif
		RT_TRACE(_module_hci_ops_os_c_,_drv_err_,("usb_write_port:( padapter->bDriverStopped ||padapter->bSurpriseRemoved )!!!\n"));
//		rtw_sctx_done_err(&pxmitbuf->sctx, RTW_SCTX_DONE_TX_DENY);
		goto exit;
	}
*/		
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

