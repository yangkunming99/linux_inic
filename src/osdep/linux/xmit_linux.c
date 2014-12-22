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
#define _XMIT_OSDEP_C_
#include "drv_types.h"
#include "xmit_osdep.h"

void rtw_os_pkt_complete(_adapter *padapter, _pkt *pkt)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,35))
	u16	queue;

	queue = skb_get_queue_mapping(pkt);

	if(__netif_subqueue_stopped(padapter->pnetdev, queue))
		netif_wake_subqueue(padapter->pnetdev, queue);

#else
	if (netif_queue_stopped(padapter->pnetdev))
		netif_wake_queue(padapter->pnetdev);
#endif

	rtw_skb_free(pkt);
}


void rtw_os_xmit_complete(_adapter *padapter, struct xmit_buf *pxbuf)
{
	if(pxbuf->pkt)
		rtw_os_pkt_complete(padapter, pxbuf->pkt);

	pxbuf->pkt = NULL;
}


int rtw_os_xmit_resource_alloc(_adapter *padapter, struct xmit_buf *pxmitbuf, u32 alloc_sz, u8 flag)
{
	if (alloc_sz > 0) {
#ifdef CONFIG_USE_USB_BUFFER_ALLOC_TX
		struct dvobj_priv	*pdvobjpriv = padapter->dvobj;
		struct usb_device	*pusbd = pdvobjpriv->pusbdev;

		pxmitbuf->pallocated_buf = rtw_usb_buffer_alloc(pusbd, (size_t)alloc_sz, &pxmitbuf->dma_transfer_addr);
		pxmitbuf->pbuf = pxmitbuf->pallocated_buf;
		if(pxmitbuf->pallocated_buf == NULL)
			return _FAIL;
#else // CONFIG_USE_USB_BUFFER_ALLOC_TX
		
		pxmitbuf->pallocated_buf = rtw_zmalloc(alloc_sz);
		if (pxmitbuf->pallocated_buf == NULL)
		{
			return _FAIL;
		}

		pxmitbuf->pbuf = (u8 *)N_BYTE_ALIGMENT((SIZE_PTR)(pxmitbuf->pallocated_buf), XMITBUF_ALIGN_SZ);

#endif // CONFIG_USE_USB_BUFFER_ALLOC_TX
	}

	if (flag) {
#ifdef CONFIG_USB_HCI
		int i;
		for(i=0; i<8; i++)
	      	{
	      		pxmitbuf->pxmit_urb[i] = usb_alloc_urb(0, GFP_KERNEL);
	             	if(pxmitbuf->pxmit_urb[i] == NULL) 
	             	{
	             		DBG_871X("pxmitbuf->pxmit_urb[i]==NULL");
		        	return _FAIL;	 
	             	}
	      	}
#endif
	}

	return _SUCCESS;	
}

void rtw_os_xmit_resource_free(_adapter *padapter, struct xmit_buf *pxmitbuf,u32 free_sz, u8 flag)
{
	if (flag) {
#ifdef CONFIG_USB_HCI
		int i;

		for(i=0; i<8; i++)
		{
			if(pxmitbuf->pxmit_urb[i])
			{
				//usb_kill_urb(pxmitbuf->pxmit_urb[i]);
				usb_free_urb(pxmitbuf->pxmit_urb[i]);
			}
		}
#endif
	}

	if (free_sz > 0 ) {
#ifdef CONFIG_USE_USB_BUFFER_ALLOC_TX
		struct dvobj_priv	*pdvobjpriv = padapter->dvobj;
		struct usb_device	*pusbd = pdvobjpriv->pusbdev;

		rtw_usb_buffer_free(pusbd, (size_t)free_sz, pxmitbuf->pallocated_buf, pxmitbuf->dma_transfer_addr);
		pxmitbuf->pallocated_buf =  NULL;
		pxmitbuf->dma_transfer_addr = 0;
#else	// CONFIG_USE_USB_BUFFER_ALLOC_TX
		if(pxmitbuf->pallocated_buf)
			rtw_mfree(pxmitbuf->pallocated_buf, free_sz);
#endif	// CONFIG_USE_USB_BUFFER_ALLOC_TX
	}
}







int _rtw_xmit_entry(_pkt *pkt, _nic_hdl pnetdev)
{
	//int ret = 0;
	PADAPTER padapter;
	struct xmit_buf *pxmitbuf;
	struct xmit_priv *pxmitpriv;
	//_irqL irqL;
_func_enter_;
	padapter = (PADAPTER)rtw_netdev_priv(pnetdev);
	pxmitpriv = &padapter->xmitpriv;
#ifdef GET_SYS_TIME
#include <linux/time.h>
extern struct timeval time_out;
do_gettimeofday(&time_out);
#endif
	//enqueue pkt
	pxmitbuf = rtw_alloc_xmitbuf(padapter);
	if(!pxmitbuf)
	{
		DBG_871X("%s(): pxmitbuf allocated failed!\n", __FUNCTION__);
		goto drop_packet;
	}
	
	pxmitbuf->pkt = pkt;
	if (rtw_hal_xmit(padapter, pxmitbuf) == _FALSE)
		goto drop_packet;

	goto exit;

drop_packet:
	pxmitpriv->tx_drop++;
	rtw_skb_free(pkt);

exit:

_func_exit_;

	return 0;
}

int rtw_xmit_entry(_pkt *pkt, _nic_hdl pnetdev)
{
	int ret = 0;

	if (pkt) {
		ret =  _rtw_xmit_entry(pkt, pnetdev);
	}
	return ret;
}
