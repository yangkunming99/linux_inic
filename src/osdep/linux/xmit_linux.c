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
//	pxmitbuf->pkt_len = pkt->len+SIZE_AT_CMD_DESC+ SIZE_TX_DESC_8195a;
//	pxmitbuf->ptxdesc->txpktsize=pkt->len+SIZE_AT_CMD_DESC;
//	pxmitbuf->patcmd->datatype = 0;
//	pxmitbuf->patcmd->pktsize = pkt->len;
//	_rtw_memcpy(pxmitbuf->pdata+sizeof(TX_DESC)+sizeof(AT_CMD_DESC), pkt->data, pkt->len);	
//_enter_critical_bh(&pxmitpriv->xmitbuf_pending_queue.lock, &irqL);
//	rtw_list_insert_tail(&pxmitbuf->list, get_list_head(&pxmitpriv->xmitbuf_pending_queue));
//_exit_critical_bh(&pxmitpriv->xmitbuf_pending_queue.lock, &irqL);
//	pnetdev->stats.tx_packets=(++(padapter->stats.tx_packets));
//	pnetdev->stats.tx_bytes=(padapter->stats.tx_bytes+=pkt->len);
	pxmitbuf->pkt = pkt;
	if (rtw_hal_xmit(padapter, pxmitbuf) == _FALSE)
		goto drop_packet;
//	_rtw_up_sema(&pxmitpriv->xmit_sema);
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
