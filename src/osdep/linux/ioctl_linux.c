#include "autoconf.h"
#include "rtw_debug.h"
#include "rtw_ioctl.h"
#include "rtw_xmit.h"
#include "rtw_cmd.h"
#include "hal_intf.h"
#include "8195_desc.h"
#define _IOCTL_LINUX_C_
//define some private IOCTL options which are not in wireless.h
#define RTL_IOCTL_ATCMD				(SIOCDEVPRIVATE+1)
static int rtw_wx_atcmd(PADAPTER padapter, struct net_device *dev, struct iw_point *p)
{
	int ret = 0;

	struct xmit_buf *pxmitbuf;
	struct xmit_priv *pxmitpriv;
	PTXDESC_8195A ptxdesc;

_func_enter_;
	pxmitpriv = &padapter->xmitpriv;
	if (p->length <= 0 || !p->pointer){
		ret = -EINVAL;
		return ret;
	}

	pxmitbuf = rtw_alloc_xmitbuf(padapter);
	if(!pxmitbuf)
	{
		DBG_871X("%s(): pxmitbuf allocated failed!\n", __FUNCTION__);
		ret = -ENOMEM;
		goto exit;
	}
	
	pxmitbuf->pkt_len = p->length + SIZE_TX_DESC_8195a;
	ptxdesc = (PTXDESC_8195A)pxmitbuf->pbuf;
	ptxdesc->txpktsize = p->length;
	ptxdesc->offset = SIZE_TX_DESC_8195a;
	ptxdesc->type = TX_H2C_CMD;//indicate transmittion of H2C packet
	ptxdesc->bus_agg_num = 0;//to do
	if (copy_from_user((pxmitbuf->pbuf + SIZE_TX_DESC_8195a), p->pointer, p->length))
	{
		rtw_free_xmitbuf(padapter, pxmitbuf);
		ret = -EFAULT;
		goto exit;
	}
	
	if(rtw_hal_mgnt_xmit(padapter, pxmitbuf) == _FALSE){
		ret = -ENOMEM;
		goto exit;
	}

exit:
_func_exit_;
	return ret;
}
int rtw_ioctl(struct net_device *pnetdev, struct ifreq *rq, int cmd)
{
	PADAPTER padapter = ((struct rtw_netdev_priv_indicator *)netdev_priv(pnetdev))->priv;
	struct iwreq *wrq = (struct iwreq *)rq;
	int ret = 0;
_func_enter_;
	DBG_871X("%s=======> cmd: 0x%04x\n", __FUNCTION__, cmd);
	switch (cmd)
	{
		case RTL_IOCTL_ATCMD:
			ret = rtw_wx_atcmd(padapter, pnetdev, &wrq->u.data);
			break;
		default:
			ret = -EOPNOTSUPP;
			break;
	}
_func_exit_;
	return ret;
}
