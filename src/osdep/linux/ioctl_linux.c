#include "../../include/autoconf.h"
#include "../../include/rtw_debug.h"
#include "../../include/rtw_ioctl.h"
#include "../../include/rtw_xmit.h"
#include "../../include/8195_desc.h"
#include "../../include/8195_sdio_reg.h"
#include "../../include/sdio_ops.h"

static int rtw_wx_atcmd(PADAPTER padapter, struct net_device *dev, struct iw_point *p)
{
	int ret = 0;
	u32 totlen;
	struct cmd_obj *pcmd;
	struct cmd_priv *pcmdpriv = &padapter->cmdpriv;
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
		rtw_mfree((u8*)pcmd->parmbuf, totlen);
		rtw_mfree((u8*)pcmd, sizeof(struct cmd_obj));
		ret = -EFAULT;
		goto exit;
	}
	_rtw_init_listhead(&pcmd->list);
	ret = rtw_enqueue_cmd(&pcmdpriv->cmd_queue, pcmd);
	if(ret == _SUCCESS)
		_rtw_up_sema(&pcmdpriv->cmd_sema);	
	return 0;
exit:
	return ret;
}
int rtw_ioctl(struct net_device *pnetdev, struct ifreq *rq, int cmd)
{
	PADAPTER padapter = ((struct rtw_netdev_priv_indicator *)netdev_priv(pnetdev))->priv;
	struct iwreq *wrq = (struct iwreq *)rq;
	int ret = 0;
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

	return ret;
}
