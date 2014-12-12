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


