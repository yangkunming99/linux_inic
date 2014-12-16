#include <linux/usb.h>

#include "drv_types.h"
#include "usb_ops.h"
#include "rtl8195a_recv.h"

extern int rtw_recv_entry(PADAPTER padapter, struct recv_buf *precvbuf);
void rtl8195au_recv_tasklet(void *priv)
{	
	struct recv_buf *precvbuf = NULL;
	_adapter	*padapter = (_adapter*)priv;
	struct recv_priv	*precvpriv = &padapter->recvpriv;

	while (NULL != (precvbuf = rtw_dequeue_recvbuf(&precvpriv->recv_buf_pending_queue)))
	{
	#if 0
		if ((padapter->bDriverStopped == _TRUE)||(padapter->bSurpriseRemoved== _TRUE))
		{
			DBG_8192C("recv_tasklet => bDriverStopped or bSurpriseRemoved \n");
			
			break;
		}
	#endif
		printk("package received\n");
		if (rtw_recv_entry(padapter, precvbuf) != _SUCCESS)
		{
			RT_TRACE(_module_rtl871x_recv_c_, _drv_err_, ("%s: rtw_recv_entry(padapter, precvbuf) != _SUCCESS\n",__FUNCTION__));
		}
		//recvbuf2recvframe(padapter, precvbuf);
		rtw_read_port(padapter, USB_READ_ADD, 0, (unsigned char *)precvbuf);
		//rtw_read_port(padapter, precvpriv->ff_hwaddr, 0, (unsigned char *)precvbuf);
	}	
	
}


static void usb_read_port_complete(struct urb *purb){
	struct recv_buf *precvbuf = (struct recv_buf *)purb->context;	
	_adapter			*padapter =(_adapter *)precvbuf->adapter;
	struct recv_priv	*precvpriv = &padapter->recvpriv;
_func_enter_;

	RT_TRACE(_module_hci_ops_os_c_,_drv_err_,("usb_read_port_complete!!!\n"));
	
	precvpriv->rx_pending_cnt --;
#if 0		
	if (RTW_CANNOT_RX(padapter))
	{
		RT_TRACE(_module_hci_ops_os_c_,_drv_err_,("usb_read_port_complete:bDriverStopped(%d) OR bSurpriseRemoved(%d)\n", padapter->bDriverStopped, padapter->bSurpriseRemoved));		

		goto exit;
	}
#endif
	if(purb->status==0)//SUCCESS
	{
		if ((purb->actual_length > MAX_RECVBUF_SZ))// || (purb->actual_length < RXDESC_SIZE))
		{
			RT_TRACE(_module_hci_ops_os_c_,_drv_err_,("usb_read_port_complete: (purb->actual_length > MAX_RECVBUF_SZ) || (purb->actual_length < RXDESC_SIZE)\n"));

			//rtw_read_port(padapter, precvpriv->ff_hwaddr, 0, (unsigned char *)precvbuf);

			rtw_read_port(padapter, USB_READ_ADD, 0, (unsigned char *)precvbuf);
		}
		else 
		{			
			rtw_reset_continual_io_error(padapter->dvobj);
			
			precvbuf->transfer_len = purb->actual_length;	
			precvbuf->pskb = _rtw_skb_alloc(precvbuf->transfer_len);
			if(precvbuf->pskb == NULL)
				return ;
			_rtw_memcpy(precvbuf->pskb->data,precvbuf->pbuf,precvbuf->transfer_len);
			skb_put(precvbuf->pskb, precvbuf->transfer_len);

			//rtw_enqueue_rx_transfer_buffer(precvpriv, rx_transfer_buf);			
			rtw_enqueue_recvbuf(precvbuf, &precvpriv->recv_buf_pending_queue);

			tasklet_schedule(&precvpriv->recv_tasklet); 		
		}		
	}
	else
	{
		RT_TRACE(_module_hci_ops_os_c_,_drv_err_,("usb_read_port_complete : purb->status(%d) != 0 \n", purb->status));
	
		DBG_8192C("###=> usb_read_port_complete => urb status(%d)\n", purb->status);

		if(rtw_inc_and_chk_continual_io_error(padapter->dvobj) == _TRUE ){
			padapter->bSurpriseRemoved = _TRUE;
		}

		switch(purb->status) {
			case -EINVAL:
			case -EPIPE:			
			case -ENODEV:
			case -ESHUTDOWN:
				//padapter->bSurpriseRemoved=_TRUE;
				//RT_TRACE(_module_hci_ops_os_c_,_drv_err_,("usb_read_port_complete:bSurpriseRemoved=TRUE\n"));
			case -ENOENT:
				padapter->bDriverStopped=_TRUE; 		
				RT_TRACE(_module_hci_ops_os_c_,_drv_err_,("usb_read_port_complete:bDriverStopped=TRUE\n"));
				break;
			case -EPROTO:
			case -EILSEQ:
			case -ETIME:
			case -ECOMM:
			case -EOVERFLOW:
			#ifdef DBG_CONFIG_ERROR_DETECT	
				{	
//					HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(padapter);
//					pHalData->srestpriv.Wifi_Error_Status = USB_READ_PORT_FAIL; 		
				}
			#endif
				//rtw_read_port(padapter, precvpriv->ff_hwaddr, 0, (unsigned char *)precvbuf);		
				rtw_read_port(padapter, USB_READ_ADD, 0, (unsigned char *)precvbuf);
				break;
			case -EINPROGRESS:
				DBG_8192C("ERROR: URB IS IN PROGRESS!/n");
				break;
			default:
				break;				
		}
		
	}	

//exit:	
	
_func_exit_;

}
u32 usb_read_port(PADAPTER padapter, u32 addr, u32 cnt, u8 *rmem)
{
	u32 ret = _SUCCESS;
	int err;
	unsigned int pipe;
	PURB purb = NULL;	
	struct recv_buf	*precvbuf = (struct recv_buf *)rmem;
	struct dvobj_priv	*pdvobj = padapter->dvobj;

	struct recv_priv	*precvpriv = &padapter->recvpriv;
	struct usb_device	*pusbd = pdvobj->pusbdev;

_func_enter_;
#if 0	
	if (RTW_CANNOT_RX(padapter) || (precvbuf == NULL))
	{
		RT_TRACE(_module_hci_ops_os_c_,_drv_err_,("usb_read_port:( RTW_CANNOT_RX ) || precvbuf == NULL!!!\n"));
		return _FAIL;
	}
#endif

	rtl8195au_init_recvbuf(padapter, precvbuf);

	if(precvbuf->pbuf)
	{			
		precvpriv->rx_pending_cnt++;
	
		purb = precvbuf->purb;		

		//translate DMA FIFO addr to pipehandle
		pipe = padapter->dvobj->recv_bulk_Pipe;

		usb_fill_bulk_urb(purb, pusbd, pipe, 
							precvbuf->pbuf,
            				MAX_RECVBUF_SZ,
            				usb_read_port_complete,
            				precvbuf);//context is precvbuf

		purb->transfer_dma = precvbuf->dma_transfer_addr;
		purb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;								

		err = usb_submit_urb(purb, GFP_ATOMIC);	
		if((err) && (err != (-EPERM)))
		{
			RT_TRACE(_module_hci_ops_os_c_,_drv_err_,("cannot submit rx in-token(err=0x%.8x), URB_STATUS =0x%.8x", err, purb->status));
			DBG_8192C("cannot submit rx in-token(err = 0x%08x),urb_status = %d\n",err,purb->status);
			ret = _FAIL;
		}
		
	}

_func_exit_;
	return ret;
}

u32 rtl8195au_hal_init(PADAPTER padapter){
	return _SUCCESS;
}

u32 rtl8195au_hal_deinit(PADAPTER padapter){
	return _SUCCESS;
}

void rtl8195au_set_intf_ops(struct _io_ops	 *pops)
{				
	_rtw_memset((u8 *)pops, 0, sizeof(struct _io_ops));	

//	pops->_read8 = &usb_read8;
//	pops->_read16 = &usb_read16;
//	pops->_read32 = &usb_read32;
//	pops->_read_mem = &usb_read_mem;
	pops->_read_port = &usb_read_port;	
	
//	pops->_write8 = &usb_write8;
//	pops->_write16 = &usb_write16;
//	pops->_write32 = &usb_write32;
//	pops->_writeN = &usb_writeN;
	
#ifdef CONFIG_USB_SUPPORT_ASYNC_VDN_REQ	
	pops->_write8_async= &usb_async_write8;
	pops->_write16_async = &usb_async_write16;
	pops->_write32_async = &usb_async_write32;
#endif	
//	pops->_write_mem = &usb_write_mem;
	pops->_write_port = &usb_write_port;

//	pops->_read_port_cancel = &usb_read_port_cancel;
//	pops->_write_port_cancel = &usb_write_port_cancel;

#ifdef CONFIG_USB_INTERRUPT_IN_PIPE
	pops->_read_interrupt = &usb_read_interrupt;
#endif
}


