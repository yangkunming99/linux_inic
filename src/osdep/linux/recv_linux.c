#define _RECV_OSDEP_C_
#include "autoconf.h"
#include "rtw_debug.h"
#include "osdep_service.h"
#include "drv_types.h"
#include "rtw_recv.h"
#include "hal_intf.h"

void rtw_os_recv_indicate_pkt(_adapter *padapter, _pkt *pkt)
{
	/* Indicat the packets to upper layer */
	if (pkt) {
		pkt->protocol = eth_type_trans(pkt, padapter->pnetdev);
		pkt->dev = padapter->pnetdev;
		pkt->ip_summed = CHECKSUM_NONE;
		rtw_netif_rx(padapter->pnetdev, pkt);
	}
}

int rtw_recv_indicatepkt(_adapter *padapter, struct recv_buf *precvbuf)
{
	struct recv_priv *precvpriv;
	_pkt *skb;

_func_enter_;

	precvpriv = &(padapter->recvpriv);

	skb = precvbuf->pskb;
	if(skb == NULL)
	{
		RT_TRACE(_module_recv_osdep_c_,_drv_err_,("rtw_recv_indicatepkt():skb==NULL something wrong!!!!\n"));
		goto _recv_indicatepkt_drop;
	}

	RT_TRACE(_module_recv_osdep_c_,_drv_info_,("rtw_recv_indicatepkt():skb != NULL !!!\n"));		
/*
	skb->data = precv_frame->u.hdr.rx_data;

	skb_set_tail_pointer(skb, precv_frame->u.hdr.len);

	skb->len = precv_frame->u.hdr.len;
*/
	RT_TRACE(_module_recv_osdep_c_,_drv_info_,("\n skb->head=%p skb->data=%p skb->tail=%p skb->end=%p skb->len=%d\n", skb->head, skb->data, skb_tail_pointer(skb), skb_end_pointer(skb), skb->len));

	rtw_os_recv_indicate_pkt(padapter, skb);
	precvpriv->rx_bytes += skb->len;
	precvpriv->rx_pkts++;
	RT_TRACE(_module_recv_osdep_c_,_drv_info_,("\n rtw_recv_indicatepkt :after rtw_os_recv_indicate_pkt!!!!\n"));

_func_exit_;
        return _SUCCESS;

_recv_indicatepkt_drop:
_func_exit_;
	 return _FAIL;
}

//alloc os related resource in struct recv_buf
int rtw_os_recvbuf_resource_alloc(_adapter *padapter, struct recv_buf *precvbuf)
{
	int res=_SUCCESS;

#ifdef CONFIG_USB_HCI
	struct dvobj_priv	*pdvobjpriv = padapter->dvobj;
	struct usb_device	*pusbd = pdvobjpriv->pusbdev;

	precvbuf->irp_pending = _FALSE;
	precvbuf->purb = usb_alloc_urb(0, GFP_KERNEL);
	if(precvbuf->purb == NULL){
		res = _FAIL;
	}

	precvbuf->pskb = NULL;

//	precvbuf->reuse = _FALSE;

	precvbuf->pallocated_buf  = precvbuf->pbuf = NULL;

	precvbuf->pdata = precvbuf->phead = precvbuf->ptail = precvbuf->pend = NULL;

	precvbuf->transfer_len = 0;

	precvbuf->len = 0;

	#ifdef CONFIG_USE_USB_BUFFER_ALLOC_RX
	precvbuf->pallocated_buf = rtw_usb_buffer_alloc(pusbd, (size_t)precvbuf->alloc_sz, &precvbuf->dma_transfer_addr);
	precvbuf->pbuf = precvbuf->pallocated_buf;
	if(precvbuf->pallocated_buf == NULL)
		return _FAIL;
	#endif //CONFIG_USE_USB_BUFFER_ALLOC_RX
	
#endif //CONFIG_USB_HCI

	return res;
}

//free os related resource in struct recv_buf
int rtw_os_recvbuf_resource_free(_adapter *padapter, struct recv_buf *precvbuf)
{
	int ret = _SUCCESS;

#ifdef CONFIG_USB_HCI

#ifdef CONFIG_USE_USB_BUFFER_ALLOC_RX

	struct dvobj_priv	*pdvobjpriv = padapter->dvobj;
	struct usb_device	*pusbd = pdvobjpriv->pusbdev;

	rtw_usb_buffer_free(pusbd, (size_t)precvbuf->alloc_sz, precvbuf->pallocated_buf, precvbuf->dma_transfer_addr);
	precvbuf->pallocated_buf =  NULL;
	precvbuf->dma_transfer_addr = 0;

#endif //CONFIG_USE_USB_BUFFER_ALLOC_RX

	if(precvbuf->purb)
	{
		//usb_kill_urb(precvbuf->purb);
		usb_free_urb(precvbuf->purb);
	}

#endif //CONFIG_USB_HCI


	if(precvbuf->pskb)
		rtw_skb_free(precvbuf->pskb);


	return ret;

}

