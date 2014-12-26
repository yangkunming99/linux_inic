#include <linux/usb.h>

#include "drv_types.h"
#include "usb_ops.h"
#include "rtl8195a_recv.h"


static u8 usb_read8(PADAPTER padapter, u32 addr)
{
	u8 request;
	u8 requesttype;
	u16 wvalue;
	u16 index;
	u16 len;
	u8 data=0;
	
	_func_enter_;

	request = 0x05;
	requesttype = 0x01;//read_in
	index = 0;//n/a

	wvalue = (u16)(addr&0x0000ffff);
	len = 1;	
	
	usbctrl_vendorreq(padapter, request, wvalue, index, &data, len, requesttype);

	_func_exit_;

	return data;
		
}

static u16 usb_read16(PADAPTER padapter, u32 addr)
{       
	u8 request;
	u8 requesttype;
	u16 wvalue;
	u16 index;
	u16 len;
	u16 data=0;
	
	_func_enter_;

	request = 0x05;
	requesttype = 0x01;//read_in
	index = 0;//n/a

	wvalue = (u16)(addr&0x0000ffff);
	len = 2;	
	
	usbctrl_vendorreq(padapter, request, wvalue, index, &data, len, requesttype);

	_func_exit_;

	return data;
	
}

static u32 usb_read32(PADAPTER padapter, u32 addr)
{
	u8 request;
	u8 requesttype;
	u16 wvalue;
	u16 index;
	u16 len;
	u32 data=0;
	
	_func_enter_;

	request = 0x05;
	requesttype = 0x01;//read_in
	index = 0;//n/a

	wvalue = (u16)(addr&0x0000ffff);
	len = 4;	
	
	usbctrl_vendorreq(padapter, request, wvalue, index, &data, len, requesttype);

	_func_exit_;

	return data;
	
}


static int usb_write8(PADAPTER padapter, u32 addr, u8 val)
{
	u8 request;
	u8 requesttype;
	u16 wvalue;
	u16 index;
	u16 len;
	u8 data;
	int ret;
	
	_func_enter_;

	request = 0x05;
	requesttype = 0x00;//write_out
	index = 0;//n/a

	wvalue = (u16)(addr&0x0000ffff);
	len = 1;
	
	data = val;	
	
	 ret = usbctrl_vendorreq(padapter, request, wvalue, index, &data, len, requesttype);
	
	_func_exit_;
	
	return ret;
	
}


static int usb_write16(PADAPTER padapter, u32 addr, u16 val)
{	
	u8 request;
	u8 requesttype;
	u16 wvalue;
	u16 index;
	u16 len;
	u16 data;
	int ret;
	
	_func_enter_;

	request = 0x05;
	requesttype = 0x00;//write_out
	index = 0;//n/a

	wvalue = (u16)(addr&0x0000ffff);
	len = 2;
	
	data = val;
		
	ret = usbctrl_vendorreq(padapter, request, wvalue, index, &data, len, requesttype);
	
	_func_exit_;
	
	return ret;
	
}

static int usb_write32(PADAPTER padapter, u32 addr, u32 val)
{
	u8 request;
	u8 requesttype;
	u16 wvalue;
	u16 index;
	u16 len;
	u32 data;
	int ret;
	
	_func_enter_;

	request = 0x05;
	requesttype = 0x00;//write_out
	index = 0;//n/a

	wvalue = (u16)(addr&0x0000ffff);
	len = 4;
	data =val;		

	ret =usbctrl_vendorreq(padapter, request, wvalue, index, &data, len, requesttype);
	
	_func_exit_;
	
	return ret;
	
}

extern int rtw_recv_entry(PADAPTER padapter, struct recv_buf *precvbuf);

#ifdef CONFIG_USE_USB_BUFFER_ALLOC_RX
void rtl8195au_recv_tasklet(void *priv)
{	
	struct recv_buf *precvbuf = NULL;
	_adapter	*padapter = (_adapter*)priv;
	struct recv_priv	*precvpriv = &padapter->recvpriv;

	while (NULL != (precvbuf = rtw_dequeue_recvbuf(&precvpriv->recv_buf_pending_queue)))
	{
		if ((padapter->bDriverStopped == _TRUE)||(padapter->bSurpriseRemoved== _TRUE))
		{
			DBG_871X("recv_tasklet => bDriverStopped or bSurpriseRemoved \n");
			
			break;
		}
	
		if (rtw_recv_entry(padapter, precvbuf) != _SUCCESS)
		{
			RT_TRACE(_module_rtl871x_recv_c_, _drv_err_, ("%s: rtw_recv_entry(padapter, precvbuf) != _SUCCESS\n",__FUNCTION__));
		}
		//recvbuf2recvframe(padapter, precvbuf);
		rtw_read_port(padapter, USB_READ_ADD, 0, (unsigned char *)precvbuf);
	}	
	
}


static void usb_read_port_complete(struct urb *purb){
	struct recv_buf *precvbuf = (struct recv_buf *)purb->context;	
	_adapter			*padapter =(_adapter *)precvbuf->adapter;
	struct recv_priv	*precvpriv = &padapter->recvpriv;
_func_enter_;

	RT_TRACE(_module_hci_ops_os_c_,_drv_err_,("usb_read_port_complete!!!\n"));
	
	precvpriv->rx_pending_cnt --;
		
	if (RTW_CANNOT_RX(padapter))
	{
		RT_TRACE(_module_hci_ops_os_c_,_drv_err_,("usb_read_port_complete:bDriverStopped(%d) OR bSurpriseRemoved(%d)\n", padapter->bDriverStopped, padapter->bSurpriseRemoved));		

		goto exit;
	}

	if(purb->status==0)//SUCCESS
	{
		if ((purb->actual_length > MAX_RECVBUF_SZ))// || (purb->actual_length < RXDESC_SIZE))
		{
			RT_TRACE(_module_hci_ops_os_c_,_drv_err_,("usb_read_port_complete: (purb->actual_length > MAX_RECVBUF_SZ) || (purb->actual_length < RXDESC_SIZE)\n"));

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
	
		DBG_871X("###=> usb_read_port_complete => urb status(%d)\n", purb->status);

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
				rtw_read_port(padapter, USB_READ_ADD, 0, (unsigned char *)precvbuf);
				break;
			case -EINPROGRESS:
				DBG_8192C("ERROR: URB IS IN PROGRESS!/n");
				break;
			default:
				break;				
		}
		
	}	

exit:	
	
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
	
	if (RTW_CANNOT_RX(padapter) || (precvbuf == NULL))
	{
		RT_TRACE(_module_hci_ops_os_c_,_drv_err_,("usb_read_port:( RTW_CANNOT_RX ) || precvbuf == NULL!!!\n"));
		return _FAIL;
	}

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
#else
void rtl8195au_recv_tasklet(void *priv)
{
	_pkt			*pskb;
	_adapter		*padapter = (_adapter*)priv;
	struct recv_priv	*precvpriv = &padapter->recvpriv;
	
	while (NULL != (pskb = skb_dequeue(&precvpriv->rx_skb_queue)))
	{
		if ((padapter->bDriverStopped == _TRUE)||(padapter->bSurpriseRemoved== _TRUE))
		{
			DBG_8192C("recv_tasklet => bDriverStopped or bSurpriseRemoved \n");
			rtw_skb_free(pskb);
			break;
		}
		//recvbuf2recvframe(padapter, pskb);

#ifdef CONFIG_PREALLOC_RECV_SKB

		skb_reset_tail_pointer(pskb);

		pskb->len = 0;
		
		skb_queue_tail(&precvpriv->free_recv_skb_queue, pskb);
		
#else
		rtw_skb_free(pskb);
#endif
				
	}
	
}


static void usb_read_port_complete(struct urb *purb, struct pt_regs *regs)
{
	_irqL irqL;
	uint isevt, *pbuf;
	struct recv_buf	*precvbuf = (struct recv_buf *)purb->context;	
	_adapter 			*padapter =(_adapter *)precvbuf->adapter;
	struct recv_priv	*precvpriv = &padapter->recvpriv;	
_func_enter_;
	RT_TRACE(_module_hci_ops_os_c_,_drv_err_,("usb_read_port_complete!!!\n"));
	
	//_enter_critical(&precvpriv->lock, &irqL);
	//precvbuf->irp_pending=_FALSE;
	//precvpriv->rx_pending_cnt --;
	//_exit_critical(&precvpriv->lock, &irqL);
		
	precvpriv->rx_pending_cnt --;
		
	//if(precvpriv->rx_pending_cnt== 0)
	//{		
	//	RT_TRACE(_module_hci_ops_os_c_,_drv_err_,("usb_read_port_complete: rx_pending_cnt== 0, set allrxreturnevt!\n"));
	//	_rtw_up_sema(&precvpriv->allrxreturnevt);	
	//}

	if (RTW_CANNOT_RX(padapter))
	{
		RT_TRACE(_module_hci_ops_os_c_,_drv_err_,("usb_read_port_complete:bDriverStopped(%d) OR bSurpriseRemoved(%d)\n", padapter->bDriverStopped, padapter->bSurpriseRemoved));		
		
	#ifdef CONFIG_PREALLOC_RECV_SKB
		precvbuf->reuse = _TRUE;
	#else
		if(precvbuf->pskb){
			DBG_8192C("==> free skb(%p)\n",precvbuf->pskb);
			rtw_skb_free(precvbuf->pskb);
		}	
	#endif
		DBG_8192C("%s() RX Warning! bDriverStopped(%d) OR bSurpriseRemoved(%d) \n", 
		__FUNCTION__,padapter->bDriverStopped, padapter->bSurpriseRemoved);
		goto exit;
	}

	if(purb->status==0)//SUCCESS
	{
		if ((purb->actual_length > MAX_RECVBUF_SZ)) // || (purb->actual_length < RXDESC_SIZE))
		{
			RT_TRACE(_module_hci_ops_os_c_,_drv_err_,("usb_read_port_complete: (purb->actual_length > MAX_RECVBUF_SZ) || (purb->actual_length < RXDESC_SIZE)\n"));
//			precvbuf->reuse = _TRUE;
			rtw_read_port(padapter, USB_READ_ADD, 0, (unsigned char *)precvbuf);
			DBG_8192C("%s()-%d: RX Warning!\n", __FUNCTION__, __LINE__);	
		}
		else 
		{	
			rtw_reset_continual_io_error(padapter->dvobj);
			printk("received data: data = %s\n",precvbuf->pskb->data);
			precvbuf->transfer_len = purb->actual_length;			
			skb_put(precvbuf->pskb, purb->actual_length);

			
			
			skb_queue_tail(&precvpriv->rx_skb_queue, precvbuf->pskb);

			if (skb_queue_len(&precvpriv->rx_skb_queue)<=1)
				tasklet_schedule(&precvpriv->recv_tasklet);

			precvbuf->pskb = NULL;
//			precvbuf->reuse = _FALSE;
			rtw_read_port(padapter, USB_READ_ADD, 0, (unsigned char *)precvbuf);			
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
					HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(padapter);
					pHalData->srestpriv.Wifi_Error_Status = USB_READ_PORT_FAIL;			
				}
				#endif
//				precvbuf->reuse = _TRUE;
				rtw_read_port(padapter, USB_READ_ADD, 0, (unsigned char *)precvbuf);			
				break;
			case -EINPROGRESS:
				DBG_8192C("ERROR: URB IS IN PROGRESS!/n");
				break;
			default:
				break;				
		}
		
	}	

exit:	
	
_func_exit_;
	
}

static u32 usb_read_port(PADAPTER padapter, u32 addr, u32 cnt, u8 *rmem)
{	
	_irqL irqL;
	int err;
	unsigned int pipe;
	SIZE_PTR tmpaddr=0;
	SIZE_PTR alignment=0;
	u32 ret = _SUCCESS;
	PURB purb = NULL;
	struct recv_buf	*precvbuf = (struct recv_buf *)rmem;
	struct dvobj_priv	*pdvobj = padapter->dvobj;
	struct recv_priv	*precvpriv = &padapter->recvpriv;
	struct usb_device	*pusbd = pdvobj->pusbdev;
	

_func_enter_;
	if (RTW_CANNOT_RX(padapter) || (precvbuf == NULL))
	{
		RT_TRACE(_module_hci_ops_os_c_,_drv_err_,("usb_read_port:( RTW_CANNOT_RX ) || precvbuf == NULL!!!\n"));
		return _FAIL;
	}

#ifdef CONFIG_PREALLOC_RECV_SKB
	if((precvbuf->reuse == _FALSE) || (precvbuf->pskb == NULL))
	{
		if (NULL != (precvbuf->pskb = skb_dequeue(&precvpriv->free_recv_skb_queue)))
		{
			precvbuf->reuse = _TRUE;
		}
	}
#endif

	rtl8195au_init_recvbuf(padapter, precvbuf);		

	//re-assign for linux based on skb
//	if((precvbuf->reuse == _FALSE) || (precvbuf->pskb == NULL))
	if(precvbuf->pskb == NULL)
	{
		precvbuf->pskb = rtw_skb_alloc(MAX_RECVBUF_SZ + RECVBUFF_ALIGN_SZ);

		if(precvbuf->pskb == NULL)		
		{
			RT_TRACE(_module_hci_ops_os_c_,_drv_err_,("init_recvbuf(): alloc_skb fail!\n"));
			DBG_8192C("#### usb_read_port() alloc_skb fail!#####\n");
			return _FAIL;
		}	

		tmpaddr = (SIZE_PTR)precvbuf->pskb->data;
        	alignment = tmpaddr & (RECVBUFF_ALIGN_SZ-1);
		skb_reserve(precvbuf->pskb, (RECVBUFF_ALIGN_SZ - alignment));
		
		precvbuf->phead = precvbuf->pskb->head;
	   	precvbuf->pdata = precvbuf->pskb->data;
		precvbuf->ptail = skb_tail_pointer(precvbuf->pskb);
		precvbuf->pend = skb_end_pointer(precvbuf->pskb);
		precvbuf->pbuf = precvbuf->pskb->data;
	}	
	else//reuse skb
	{
		precvbuf->phead = precvbuf->pskb->head;
		precvbuf->pdata = precvbuf->pskb->data;
		precvbuf->ptail = skb_tail_pointer(precvbuf->pskb);
		precvbuf->pend = skb_end_pointer(precvbuf->pskb);
   		precvbuf->pbuf = precvbuf->pskb->data;

//		precvbuf->reuse = _FALSE;
	}

	//_enter_critical(&precvpriv->lock, &irqL);
	//precvpriv->rx_pending_cnt++;
	//precvbuf->irp_pending = _TRUE;
	//_exit_critical(&precvpriv->lock, &irqL);

	precvpriv->rx_pending_cnt++;

	purb = precvbuf->purb;

	//translate DMA FIFO addr to pipehandle
	
	pipe = padapter->dvobj->recv_bulk_Pipe;
	printk("received data: data = %s\n",precvbuf->pskb->data);
	usb_fill_bulk_urb(purb, pusbd, pipe, 
						precvbuf->pbuf,
            				MAX_RECVBUF_SZ,
            				usb_read_port_complete,
            				precvbuf);//context is precvbuf

	err = usb_submit_urb(purb, GFP_ATOMIC);
	if((err) && (err != (-EPERM)))
	{
		RT_TRACE(_module_hci_ops_os_c_,_drv_err_,("cannot submit rx in-token(err=0x%.8x), URB_STATUS =0x%.8x", err, purb->status));
		DBG_8192C("cannot submit rx in-token(err = 0x%08x),urb_status = %d\n",err,purb->status);
		ret = _FAIL;
	}

_func_exit_;

	return ret;
}

#endif
u32 rtl8195au_hal_init(PADAPTER padapter){
	return _SUCCESS;
}

u32 rtl8195au_hal_deinit(PADAPTER padapter){
	return _SUCCESS;
}

void rtl8195au_set_intf_ops(struct _io_ops	 *pops)
{				
	_rtw_memset((u8 *)pops, 0, sizeof(struct _io_ops));	

	pops->_read8 = &usb_read8;
	pops->_read16 = &usb_read16;
	pops->_read32 = &usb_read32;
//	pops->_read_mem = &usb_read_mem;
	pops->_read_port = &usb_read_port;	
	
	pops->_write8 = &usb_write8;
	pops->_write16 = &usb_write16;
	pops->_write32 = &usb_write32;
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


