#include "osdep_intf.h"
#include "rtw_xmit.h"
#include "xmit_osdep.h"
#define _OS_INTFS_C_


#ifdef CONFIG_FILE_FWIMG

//char *rtw_fw_file_path = "/system/etc/firmware/rtlwifi/FW_NIC.BIN";
char *rtw_fw_file_path = "/etc/FW_NIC.bin";

module_param(rtw_fw_file_path, charp, 0644);
MODULE_PARM_DESC(rtw_fw_file_path, "The path of fw image");

char *rtw_fw_wow_file_path = "/system/etc/firmware/rtlwifi/FW_WoWLAN.BIN";
module_param(rtw_fw_wow_file_path, charp, 0644);
MODULE_PARM_DESC(rtw_fw_wow_file_path, "The path of fw for Wake on Wireless image");

#endif // CONFIG_FILE_FWIMG




struct dvobj_priv *devobj_init(void)
{
	struct dvobj_priv *pdvobj = NULL;

	if ((pdvobj = (struct dvobj_priv*)rtw_zmalloc(sizeof(*pdvobj))) == NULL) 
	{
		return NULL;
	}

	_rtw_spinlock_init(&pdvobj->lock);

	return pdvobj;

}

void devobj_deinit(struct dvobj_priv *pdvobj)
{
	if(!pdvobj)
		return;

	_rtw_spinlock_free(&pdvobj->lock);

	rtw_mfree((u8*)pdvobj, sizeof(*pdvobj));
}	



#if 0
static const struct ethtool_ops rtl8195a_ethtool_ops = {
	.get_settings		= rtw_get_settings,
	.set_settings		= rtw_set_settings,
	.get_drvinfo		= rtw_get_drvinfo,
	.get_regs_len		= rtw_get_regs_len,
	.get_regs		= rtw_get_regs,
	.get_wol		= rtw_get_wol,
	.set_wol		= rtw_set_wol,
	.get_msglevel		= rtw_get_msglevel,
	.set_msglevel		= rtw_set_msglevel,
	.nway_reset		= rtw_nway_reset,
	.get_link		= ethtool_op_get_link,
	.get_eeprom_len		= rtw_get_eeprom_len,
	.get_eeprom		= rtw_get_eeprom,
	.set_eeprom		= rtw_set_eeprom,
	.get_ringparam		= rtw_get_ringparam,
	.set_ringparam		= rtw_set_ringparam,
	.get_pauseparam		= rtw_get_pauseparam,
	.set_pauseparam		= rtw_set_pauseparam,
	.self_test		= rtw_diag_test,
	.get_strings		= rtw_get_strings,
	.set_phys_id		= rtw_set_phys_id,
	.get_ethtool_stats	= rtw_get_ethtool_stats,
	.get_sset_count		= rtwe_get_sset_count,
	.get_coalesce		= rtw_get_coalesce,
	.set_coalesce		= rtw_set_coalesce,
	.get_rxnfc		= rtw_get_rxnfc,
	.get_ts_info		= rtw_get_ts_info,
	.get_eee		= rtw_get_eee,
	.set_eee		= rtw_set_eee,
};
void rtw_set_ethtool_ops(struct net_device *netdev)
{
	SET_ETHTOOL_OPS(netdev, &rtl8195a_ethtool_ops);
}
#endif
u32 rtw_start_drv_threads(PADAPTER padapter)
{
	u32 ret = _SUCCESS;
	struct xmit_priv *pxmitpriv = &padapter->xmitpriv;
	struct cmd_priv *pcmdpriv = &padapter->cmdpriv;
	pxmitpriv->xmitThread = kthread_run(rtw_xmit_thread, padapter, "RTW_XMIT_THREAD");
	if(IS_ERR(pxmitpriv->xmitThread))
	{	
		DBG_871X("%s()====>rtw_xmit_thread start Failed!\n", __FUNCTION__);
		ret = _FAIL;
	}
	pcmdpriv->cmdThread = kthread_run(rtw_cmd_thread, padapter, "RTW_CMD_THREAD");
	if(IS_ERR(pcmdpriv->cmdThread))
	{	
		DBG_871X("%s()====>rtw_cmd_thread start Failed!\n", __FUNCTION__);
		ret = _FAIL;
	}
	return ret;
}
void rtw_stop_drv_threads (PADAPTER padapter)
{
	struct xmit_priv *pxmitpriv = &padapter->xmitpriv;
	struct cmd_priv *pcmdpriv = &padapter->cmdpriv;
	if(pxmitpriv->xmitThread)
	{
		pxmitpriv->xmitThread=NULL;
	}
	if(pcmdpriv->cmdThread)
	{
		pcmdpriv->cmdThread=NULL;
	}
} 

int netdev_open(struct net_device *pnetdev)
{
	uint status;
	PADAPTER padapter = (PADAPTER)rtw_netdev_priv(pnetdev);
	RT_TRACE(_module_os_intfs_c_,_drv_info_,("+871x_drv - dev_open\n"));
	DBG_871X("+871x_drv - drv_open, bup=%d\n", padapter->bup);
	padapter->netif_up = _TRUE;
	if(padapter->bup == _FALSE)
	{
		status = rtw_hal_init(padapter);
		if (status ==_FAIL)
		{
			RT_TRACE(_module_os_intfs_c_,_drv_err_,("rtw_hal_init(): Can't init h/w!\n"));
			goto netdev_open_error;
		}
		if(rtw_start_drv_threads(padapter) == _FAIL)
		{
			RT_TRACE(_module_os_intfs_c_,_drv_err_,("rtw_start_drv_threads: failed h/w!\n"));
			goto netdev_open_error;
		}
		if (padapter->intf_start)
		{
			padapter->intf_start(padapter);
		}
		padapter->bup = _TRUE;
	}
	padapter->netif_up = _FALSE;
	if(!rtw_netif_queue_stopped(pnetdev))
		rtw_netif_start_queue(pnetdev);
	else
		rtw_netif_wake_queue(pnetdev);
	
	RT_TRACE(_module_os_intfs_c_,_drv_info_,("-871x_drv - dev_open\n"));
	DBG_871X("-871x_drv - drv_open, bup=%d\n", padapter->bup);
	return 0;
netdev_open_error:
	padapter->bup = _FALSE;
	netif_carrier_off(pnetdev);
	rtw_netif_stop_queue(pnetdev);
	RT_TRACE(_module_os_intfs_c_,_drv_err_,("-871x_drv - dev_open, fail!\n"));
	DBG_871X("-871x_drv - drv_open fail, bup=%d\n", padapter->bup);
	return (-1);	
}
int netdev_close(struct net_device *pnetdev)
{
	int ret = 0;
	PADAPTER padapter = (PADAPTER)rtw_netdev_priv(pnetdev);
	RT_TRACE(_module_os_intfs_c_,_drv_info_,("+871x_drv - drv_close\n"));
	padapter->net_closed = _TRUE;
	padapter->netif_up = _FALSE;
	if(pnetdev)
	{
		if (!rtw_netif_queue_stopped(pnetdev))
			rtw_netif_stop_queue(pnetdev);
		netif_carrier_off(pnetdev);
		rtw_stop_drv_threads(padapter);
	}
	RT_TRACE(_module_os_intfs_c_,_drv_info_,("-871x_drv - drv_close\n"));
	DBG_871X("-871x_drv - drv_close, bup=%d\n", padapter->bup);
	return ret;
}
static int rtw_net_set_mac_address(struct net_device *pnetdev, void *p)
{
//	_adapter *padapter = (_adapter *)rtw_netdev_priv(pnetdev);
//	struct sockaddr *addr = p;
//to do, Chris
/*
	if(padapter->bup == _FALSE)
	{
		//DBG_871X("r8711_net_set_mac_address(), MAC=%x:%x:%x:%x:%x:%x\n", addr->sa_data[0], addr->sa_data[1], addr->sa_data[2], addr->sa_data[3],
		//addr->sa_data[4], addr->sa_data[5]);
		_rtw_memcpy(padapter->eeprompriv.mac_addr, addr->sa_data, ETH_ALEN);
		//_rtw_memcpy(pnetdev->dev_addr, addr->sa_data, ETH_ALEN);
		//padapter->bset_hwaddr = _TRUE;
	}
*/
	return 0;
}

static struct net_device_stats *rtw_net_get_stats(struct net_device *pnetdev)
{
	_adapter *padapter = (_adapter *)rtw_netdev_priv(pnetdev);
	struct xmit_priv *pxmitpriv = &(padapter->xmitpriv);
	struct recv_priv *precvpriv = &(padapter->recvpriv);

	padapter->stats.tx_packets = pxmitpriv->tx_pkts;//pxmitpriv->tx_pkts++;
	padapter->stats.rx_packets = precvpriv->rx_pkts;//precvpriv->rx_pkts++;
	padapter->stats.tx_dropped = pxmitpriv->tx_drop;
	padapter->stats.rx_dropped = precvpriv->rx_drop;
	padapter->stats.tx_bytes = pxmitpriv->tx_bytes;
	padapter->stats.rx_bytes = precvpriv->rx_bytes;
	
	return &padapter->stats;
}

static struct rtnl_link_stats64 *rtw_net_get_stats64(struct net_device *pnetdev,
					     struct rtnl_link_stats64 *stats)
{
	PADAPTER padapter = (PADAPTER)rtw_netdev_priv(pnetdev);
	struct xmit_priv *pxmitpriv = &(padapter->xmitpriv);
	struct recv_priv *precvpriv = &(padapter->recvpriv);
	memset(stats, 0, sizeof(struct rtnl_link_stats64));
	/* Fill out the OS statistics structure */
	stats->rx_bytes = precvpriv->rx_bytes;
	stats->rx_packets = precvpriv->rx_pkts;
	stats->tx_bytes = pxmitpriv->tx_bytes;
	stats->tx_packets = pxmitpriv->tx_pkts;
	return stats;
}

static const struct net_device_ops rtw_netdev_ops = {
	.ndo_open = netdev_open,
	.ndo_stop = netdev_close,
	.ndo_start_xmit = rtw_xmit_entry,
	.ndo_set_mac_address = rtw_net_set_mac_address,
	.ndo_get_stats = rtw_net_get_stats,
	.ndo_get_stats64 = rtw_net_get_stats64,
	.ndo_do_ioctl = rtw_ioctl,
};

struct net_device *rtw_init_netdev(_adapter *old_padapter)
{
	_adapter *padapter;
	struct net_device *pnetdev;
	RT_TRACE(_module_os_intfs_c_,_drv_info_,("+init_net_dev\n"));
	if(old_padapter != NULL)
		pnetdev = rtw_alloc_etherdev_with_old_priv(sizeof(_adapter), (void *)old_padapter);
	else
		pnetdev = rtw_alloc_etherdev(sizeof(_adapter));

	if (!pnetdev)
		return NULL;

	padapter = rtw_netdev_priv(pnetdev);
	padapter->pnetdev = pnetdev;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24)
	SET_MODULE_OWNER(pnetdev);
#endif

#if (LINUX_VERSION_CODE>=KERNEL_VERSION(2,6,29))
	DBG_871X("register rtw_netdev_ops to netdev_ops\n");
	pnetdev->netdev_ops = &rtw_netdev_ops;
#else
	pnetdev->open = netdev_open;
	pnetdev->stop = netdev_close;
	pnetdev->hard_start_xmit = rtw_xmit_entry;
	pnetdev->set_mac_address = rtw_net_set_mac_address;
	pnetdev->get_stats = rtw_net_get_stats;
	pnetdev->do_ioctl = rtw_ioctl;
#endif

	return pnetdev;

}

void rtw_unregister_netdevs(struct net_device *pnetdev)
{
	if(pnetdev)
		unregister_netdev(pnetdev); //will call netdev_close()
}	

u8 rtw_init_drv_sw(_adapter *padapter)
{

	u8	ret8 = _SUCCESS;

_func_enter_;
	_rtw_spinlock_init(&padapter->lock);
	if(rtw_init_xmit_priv(padapter) == _FAIL)
	{
		DBG_871X("%s()====>rtw_init_xmit_freebuf() failed!\n", __FUNCTION__);
		ret8 = _FAIL;
		goto free_spinlock;
	}

#ifdef USE_RECV_TASKLET
	if(rtw_init_recv_priv(padapter) == _FAIL)
	{
		DBG_871X("%s()====>rtw_init_recv_freebuf() failed!\n", __FUNCTION__);
		ret8 = _FAIL;
		goto free_xmitpriv;
	}
#endif

	if(rtw_init_cmd_priv(padapter) == _FAIL)
	{
		DBG_871X("%s()====>rtw_init_recv_freebuf() failed!\n", __FUNCTION__);
		ret8 = _FAIL;
		goto free_recvpriv;
	}

	goto exit;

free_recvpriv:
#ifdef USE_RECV_TASKLET
	rtw_free_recv_priv(padapter);
#endif

free_xmitpriv:
	rtw_free_xmit_priv(padapter);

free_spinlock:
	_rtw_spinlock_free(&padapter->lock);
	
exit:

	_func_exit_;

	return ret8;

}

u8 rtw_free_drv_sw(_adapter *padapter)
{
	//struct net_device *pnetdev = (struct net_device*)padapter->pnetdev;
_func_enter_;
	_rtw_spinlock_free(&padapter->lock);

	rtw_free_xmit_priv(padapter);

	rtw_free_recv_priv(padapter);

	rtw_free_cmd_priv(padapter);

_func_exit_;
	return _SUCCESS;
}

int rtw_drv_register_netdev(_adapter * padapter){

	int ret = _SUCCESS;
	struct net_device *pnetdev = padapter->pnetdev;
_func_enter_;
	netif_carrier_off(pnetdev);

	if(register_netdev(pnetdev)!=0){
		ret = _FAIL;
	}
_func_exit_;
	return ret;
}

void rtw_drv_unregister_netdev(_adapter * padapter){

	struct net_device *pnetdev = padapter->pnetdev;
_func_enter_;
	netif_carrier_off(pnetdev);

	unregister_netdev(pnetdev);
_func_exit_;
}
