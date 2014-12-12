#include "autoconf.h"
#include "rtw_debug.h"
#include "rtw_ioctl.h"
#include "rtw_xmit.h"
#include "rtw_cmd.h"
#include "hal_intf.h"
//define some private IOCTL options which are not in wireless.h
#define RTL_IOCTL_ATCMD				(SIOCDEVPRIVATE+1)
static int rtw_wx_atcmd(PADAPTER padapter, struct net_device *dev, struct iw_point *p)
{
	int ret = 0;
	u32 totlen;
	struct cmd_obj *pcmd;

	if (p->length <= 0 || !p->pointer){
		ret = -EINVAL;
		return ret;
	}
	pcmd = (struct cmd_obj*)rtw_zmalloc(sizeof(struct cmd_obj));
	if(pcmd == NULL)
	{
		ret = -ENOMEM;
		goto exit;
	}
	totlen = p->length;
	pcmd->parmbuf= rtw_zmalloc(totlen);
	pcmd->cmdsz = totlen;
	if (pcmd->parmbuf == NULL)
	{
		ret = -ENOMEM;
		rtw_mfree((u8*)pcmd, sizeof(struct cmd_obj));
		goto exit;
	}
	if (copy_from_user((pcmd->parmbuf), p->pointer, p->length))
	{
		rtw_free_cmd_obj(pcmd);
		ret = -EFAULT;
		goto exit;
	}
	if(rtw_hal_mgnt_xmit(padapter, pcmd) == _FALSE){
		ret = -ENOMEM;
		goto exit;
	}

exit:
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
