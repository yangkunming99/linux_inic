#ifndef __RTW_IOCTL_H__
#define __RTW_IOCTL_H__
#include "drv_types.h"
//define some private IOCTL options which are not in wireless.h
#define RTL_IOCTL_ATCMD				(SIOCDEVPRIVATE+1)
int rtw_ioctl(struct net_device *pnetdev, struct ifreq *rq, int cmd);
#endif
