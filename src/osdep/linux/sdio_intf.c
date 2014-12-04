#include <linux/module.h>
#include <linux/init.h>
#include <linux/version.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/delay.h>
#include <linux/ethtool.h>
#include <linux/mii.h>
#include <linux/if_vlan.h>
#include <linux/crc32.h>
#include <linux/interrupt.h>
#include <linux/in.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/rtnetlink.h>
#include <linux/fs.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
#define dev_printk(A,B,fmt,args...) DBG_871X(A fmt,##args)
#else
#include <linux/dma-mapping.h>
#include <linux/moduleparam.h>
#endif

#include <asm/io.h>
#include <asm/irq.h>
#include <asm/uaccess.h>

#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/errno.h>
#include "../../include/autoconf.h"
#include "../../include/rtw_debug.h"
#include "../../include/sdio_ops.h"
#include "../../include/8195_desc.h"
#include "../../include/8195_sdio_reg.h"
#include "../../include/rtl8195a.h"
#include "../../include/osdep_service.h"
#include "../../include/sdio_ops_linux.h"
#include "../../include/rtw_ioctl.h"
#include "../../include/rtw_xmit.h"
#include "../../include/rtw_recv.h"
#include "../../include/rtw_io.h"
#include "../../include/rtw_cmd.h"
MODULE_AUTHOR("Realtek");
MODULE_DESCRIPTION("RealTek RTL-8195a iNIC");
MODULE_LICENSE("GPL");
MODULE_VERSION(RTL8195_VERSION);


#ifdef GET_SYS_TIME
#include <linux/time.h>
struct timeval time_out;
struct timeval time_back;
#endif

struct global_buf gDataBuf;
u32 rtw_start_drv_threads(PADAPTER padapter);
void rtw_stop_drv_threads (PADAPTER padapter);

TXDESC_8195A rtl8195a_fill_default_txdesc(u16 pktsize)
{
	TXDESC_8195A txdesc;
//DWORD 0
	txdesc.txpktsize = pktsize;

	txdesc.offset = sizeof(TXDESC_8195A);
#if 0
	txdesc.bmc = 1;
	txdesc.htc = 0;
	txdesc.ls = 1;
	txdesc.fs = 1;
	txdesc.linip = 0;
	txdesc.noacm = 0;
	txdesc.gf = 0;
	txdesc.own = 1;

//DWORD 1
	txdesc.macid = 0;
	txdesc.agg_en = 0;
	txdesc.bk = 0;
	txdesc.rdg_en = 0;

	txdesc.qsel = 6;
	txdesc.rdg_nav_ext = 0;
	txdesc.lsig_txop_en = 0;
	txdesc.pifs = 0;

	txdesc.rate_id = 0;
	txdesc.navusehdr = 0;
	txdesc.en_desc_id = 1;
	txdesc.sectype = 0;

	txdesc.rsvd2 = 0;
	txdesc.pkt_offset = 0;
	txdesc.rsvd3 = 0;

//DWORD 2
	txdesc.rts_rc = 0;
	txdesc.data_rc = 0;
	txdesc.rsvd8 = 0;
	txdesc.bar_rty_th = 0;

	txdesc.rsvd4 = 0;
	txdesc.morefrag = 0;
	txdesc.raw = 0;
	txdesc.ccx = 0;
	txdesc.ampdu_density = 7;
	txdesc.rsvd5 = 0;

	txdesc.antsel_a = 0;
	txdesc.antsel_b =0;
	txdesc.tx_ant_cck = 0;
	txdesc.tx_antl = 0;
	txdesc.tx_antht = 0;

//DWORD 3
	txdesc.nextheadpage = 0;
	txdesc.tailpage = 0;
	txdesc.seq = seqNum;
	txdesc.pkt_id = 0;

//DWORD 4
	txdesc.rtsrate = 3;
	txdesc.ap_dcfe = 0;
	txdesc.qos =1;
	txdesc.hwseq_en =0;

	txdesc.userate = 1;
	txdesc.disrtsfb =1;
	txdesc.disdatafb =1;
	txdesc.cts2self =0;
	txdesc.rtsen =0;
	txdesc.hw_rts_en = 0;
	txdesc.port_toggle =0;
	txdesc.rsvd6 =0;

	txdesc.rsvd7 =0;
	txdesc.wait_dcts =0;
	txdesc.cts2ap_en=0;
	txdesc.data_txsc=0;

	txdesc.data_short =0;
	txdesc.databw =0;
	txdesc.rts_short =0;
	txdesc.rtsbw =0;
	txdesc.rts_sc =0;
	txdesc.vcs_stbc=0;

//DWORD 5
	txdesc.datarate = 0;

	txdesc.data_ratefb_lmt =0;
	txdesc.rts_ratefb_lmt =0;
	txdesc.rty_en =0;
	txdesc.data_rt_lmt =0;

	txdesc.usb_txagg_num=0;

//DWORD 6
	txdesc.txagc_a = 0;
	txdesc.txagc_b =0;
	txdesc.use_max_len =0;
	txdesc.max_agg_num = 0xf;

	txdesc.mcsg1_max_len = 8;
	txdesc.mcsg2_max_len =8;

	txdesc.mcsg3_max_len = 8;
	txdesc.mcsg7_max_len =8;

//DWORD 7
	txdesc.txbuffsize = 0x7da0;

	txdesc.mcsg4_max_len =0;
	txdesc.mcsg5_max_len =0;
	txdesc.mcsg6_max_len =0;
	txdesc.mcsg15_max_len =0;


#endif	
	return txdesc;
}

static void sd_sync_int_hdl(struct sdio_func *func)
{
	PADAPTER padapter = sdio_get_drvdata(func);
	PSDIO_DATA psdio = &padapter->intf_data;
	psdio->sdio_himr = (u32)(	\
								SDIO_HIMR_RX_REQUEST_MSK | 
								SDIO_HIMR_AVAL_MSK	|	
								0);
	rtw_sdio_set_irq_thd(padapter, current);
	sd_int_hal(padapter);
	rtw_sdio_set_irq_thd(padapter, NULL);
}

int sdio_alloc_irq(struct sdio_func *func)
{
	int err;
	sdio_claim_host(func);
	err = sdio_claim_irq(func, &sd_sync_int_hdl);
	if(err)
	{
		DBG_871X(KERN_CRIT "%s(): sdio_claim_irq FAIL(%d)!\n", __FUNCTION__, err);
	}
	sdio_release_host(func);

	return err?_FAIL:_SUCCESS;
}
void sdio_free_irq(struct sdio_func *func)
{
	int err;
	if(func)
	{
		sdio_claim_host(func);
		err = sdio_release_irq(func);
		if(err)
		{			
			DBG_871X("%s: sdio_release_irq FAIL(%d)!\n", __func__, err);
		}
		sdio_release_host(func);
	}
}
static int sdio_init(struct sdio_func *func)
{
	int rc = 0;

	DBG_871X("%s():\n", __FUNCTION__);
_func_enter_;
	sdio_claim_host(func);
	rc = sdio_enable_func(func);
	if(rc ){
		DBG_871X("%s():sdio_enable_func FAIL!\n",__FUNCTION__);
		goto release;
	}
	rc = sdio_set_block_size(func, 512);
	if(rc ){
		DBG_871X("%s():sdio_set_block_size FAIL!\n",__FUNCTION__);
		goto release;
	}
release:
    sdio_release_host(func);

_func_exit_;

	return rc;   
}
static void sdio_deinit(struct sdio_func *func)
{
	int rc;
	if(func)
	{
		sdio_claim_host(func);
		rc = sdio_disable_func(func);
		if(rc){
			DBG_871X("%s(): sdio_disable_func fail!\n", __FUNCTION__);
		}
		sdio_release_host(func);		
	}
}
int netdev_open(struct net_device *pnetdev)
{
	int ret = 0;
	PADAPTER padapter = ((struct rtw_netdev_priv_indicator *)netdev_priv(pnetdev))->priv;
	if(rtw_start_drv_threads(padapter)==_FAIL)
	{
		rtw_os_indicate_disconnect(pnetdev);
		return -ENOMEM;
	}
	return ret;
}
int netdev_close(struct net_device *pnetdev)
{
	int ret = 0;
	PADAPTER padapter = (PADAPTER)rtw_netdev_priv(pnetdev);
	DBG_871X("%s=======>\n", __FUNCTION__);
	if(pnetdev)
	{
		if (!rtw_netif_queue_stopped(pnetdev))
			rtw_netif_stop_queue(pnetdev);
		netif_carrier_off(pnetdev);
		rtw_stop_drv_threads(padapter);
	}
	return ret;
}

struct net_device_stats *rtw_net_get_stats(struct net_device *pnetdev)
{
	PADAPTER padapter = (PADAPTER)rtw_netdev_priv(pnetdev);
	return &padapter->stats;
}

struct rtnl_link_stats64 *rtw_net_get_stats64(struct net_device *pnetdev,
					     struct rtnl_link_stats64 *stats)
{
	PADAPTER padapter = (PADAPTER)rtw_netdev_priv(pnetdev);
	memset(stats, 0, sizeof(struct rtnl_link_stats64));
	/* Fill out the OS statistics structure */
	stats->rx_bytes = padapter->stats.rx_bytes;
	stats->rx_packets = padapter->stats.rx_packets;
	stats->tx_bytes = padapter->stats.tx_bytes;
	stats->tx_packets = padapter->stats.tx_packets;
	return stats;
}
static const struct net_device_ops rtw_netdev_ops = {
//	.ndo_init = rtw_ndev_init,
//	.ndo_uninit = rtw_ndev_uninit,
	.ndo_open = netdev_open,
	.ndo_stop = netdev_close,
	.ndo_start_xmit = rtw_xmit_entry,
#if (LINUX_VERSION_CODE>=KERNEL_VERSION(2,6,35))
//	.ndo_select_queue	= rtw_select_queue,
#endif
//	.ndo_set_mac_address = rtw_net_set_mac_address,
	.ndo_get_stats = rtw_net_get_stats,
	.ndo_get_stats64 = rtw_net_get_stats64,
	.ndo_do_ioctl = rtw_ioctl,
};
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
#if 0
int rtw_os_xmit_resource_alloc(PADAPTER padapter, struct xmit_buf *pxmitbuf, u32 alloc_sz, u8 flag)
{
	if (alloc_sz > 0) {
		
		pxmitbuf->pallocated_buf= rtw_zmalloc(alloc_sz);
		if (pxmitbuf->pallocated_buf == NULL)
		{
			return _FAIL;
		}

		pxmitbuf->pdata = (u8 *)N_BYTE_ALIGMENT((SIZE_PTR)(pxmitbuf->pallocated_buf), XMITBUF_ALIGN_SZ);
	}
	return _SUCCESS;	
}

void rtw_os_xmit_resource_free(PADAPTER padapter, struct xmit_buf *pxmitbuf,u32 free_sz, u8 flag)
{

	if (free_sz > 0 ) {
		if(pxmitbuf->pallocated_buf)
			rtw_mfree(pxmitbuf->pallocated_buf, free_sz);
	}
}
#endif
static int rtw_xmit_thread(void *context)
{
	s32 err;
	PADAPTER padapter;
	padapter = (PADAPTER)context;
	thread_enter("RTW_XMIT_THREAD");
	do{
		err = rtw_hal_xmit_handler(padapter);
		flush_signals_thread();
	}while(_SUCCESS == err);
	thread_exit();
}

static int rtw_cmd_thread(void *context)
{
	s32 err;
	PADAPTER padapter;
	//DBG_871X("%s=======>\n", __FUNCTION__);
	padapter = (PADAPTER)context;
	thread_enter("RTW_CMD_THREAD");
	do{
		err = rtw_cmd_handler(padapter);
		flush_signals_thread();
	}while(_SUCCESS == err);
	thread_exit();
}

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

static int __devinit rtl8195a_init_one(struct sdio_func *func, const struct sdio_device_id *id)
{
	struct net_device *pnetdev;
	PADAPTER padapter;
	PSDIO_DATA psdio;
	int rc = 0;
	u8 mac_addr[ETH_ALEN];
	DBG_871X("%s():++\n",__FUNCTION__);
	DBG_871X("+rtw_drv_init: vendor=0x%04x device=0x%04x class=0x%02x\n", func->vendor, func->device, func->class);

	_rtw_spinlock_init(&gDataBuf.lock);
	// 1.init SDIO bus and read chip version	
	rc = sdio_init(func);
	if(rc)
		return rc;
	
	// 2. init network device data and padapter
	pnetdev = rtw_alloc_etherdev(sizeof(ADAPTER));
	if(!pnetdev)
	{
		sdio_deinit(func);
		return -ENODEV;
	}
#if (LINUX_VERSION_CODE>=KERNEL_VERSION(2,6,29))
	pnetdev->netdev_ops = &rtw_netdev_ops;
#else
//	pnetdev->init = rtw_ndev_init;
//	pnetdev->uninit = rtw_ndev_uninit;
	pnetdev->open = netdev_open;
	pnetdev->stop = netdev_close;
	pnetdev->hard_start_xmit = rtw_xmit_entry;
//	pnetdev->set_mac_address = rtw_net_set_mac_address;
	pnetdev->get_stats = rtw_net_get_stats;
	pnetdev->do_ioctl = rtw_ioctl;
#endif
	padapter = (PADAPTER)rtw_netdev_priv(pnetdev);
	padapter->pnetdev = pnetdev;
	psdio = &padapter->intf_data;
	psdio->func= func;
	psdio->SdioRxFIFOCnt = 0;
	_rtw_spinlock_init(&padapter->lock);
	if(rtw_init_xmit_priv(padapter)==_FAIL)
	{
		DBG_871X("%s()====>rtw_init_xmit_freebuf() failed!\n", __FUNCTION__);
		sdio_deinit(func);
		rtw_free_netdev(pnetdev);
		return -ENOMEM;
	}
#ifdef USE_RECV_TASKLET
	if(rtw_init_recv_priv(padapter)==_FAIL)
	{
		DBG_871X("%s()====>rtw_init_recv_freebuf() failed!\n", __FUNCTION__);
		sdio_deinit(func);
		rtw_free_xmit_priv(padapter);
		rtw_free_netdev(pnetdev);
		return -ENOMEM;
	}
#endif
	if(rtw_init_cmd_priv(padapter)==_FAIL)
	{
		DBG_871X("%s()====>rtw_init_recv_freebuf() failed!\n", __FUNCTION__);
		sdio_deinit(func);
		rtw_free_xmit_priv(padapter);
#ifdef USE_RECV_TASKLET
		rtw_free_recv_priv(padapter);
#endif
		rtw_free_netdev(pnetdev);
		return -ENOMEM;
	}
	sdio_set_intf_ops(padapter, &padapter->io_ops);
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24)
	SET_MODULE_OWNER(pnetdev);
#endif
	SET_NETDEV_DEV(pnetdev, &func->dev);
	sdio_set_drvdata(func, padapter);

	// 3. register net device
	netif_carrier_off(pnetdev);

	mac_addr[0] = 0x00;
	mac_addr[1] = 0xe0;
	mac_addr[2] = 0x4c;
	mac_addr[3] = 0x87;
	mac_addr[4] = 0x00;
	mac_addr[5] = 0x00;
	_rtw_memcpy(pnetdev->dev_addr, mac_addr, ETH_ALEN);
	rc = register_netdev(pnetdev);
	if (rc) {
		DBG_871X("%s()====>register net device failed!\n", __FUNCTION__);
		if(padapter)
		{
			rtw_free_xmit_priv(padapter);
#ifdef USE_RECV_TASKLET
			rtw_free_recv_priv(padapter);
#endif
		}
		rtw_free_netdev(pnetdev);
		sdio_set_drvdata(func, NULL);
		sdio_deinit(func);
		return rc;
	}

	if (sdio_alloc_irq(func) != _SUCCESS)
	{
		DBG_871X("%s()====>sdio_alloc_irq failed!\n", __FUNCTION__);
		unregister_netdev(pnetdev);
		if(padapter)
		{
			rtw_free_xmit_priv(padapter);
#ifdef USE_RECV_TASKLET
			rtw_free_recv_priv(padapter);
#endif
			rtw_free_cmd_priv(padapter);
		}
		rtw_free_netdev(pnetdev);
		sdio_set_drvdata(func, NULL);
		sdio_deinit(func);
		return -ENODEV;		
	}
	return 0;
}

static void __devexit rtl8195a_remove_one(struct sdio_func *func)

{

	int rc = 0;
	struct net_device *pnetdev;
	PADAPTER padapter;
	DBG_871X("%s():++\n", __FUNCTION__);
	padapter = sdio_get_drvdata(func);

	if(padapter)
	{	
		pnetdev = padapter->pnetdev;
		if(pnetdev)
			unregister_netdev(pnetdev);
		rtw_free_xmit_priv(padapter);
#ifdef USE_RECV_TASKLET
		rtw_free_recv_priv(padapter);
#endif
		rtw_stop_drv_threads(padapter);
		rtw_free_netdev(pnetdev);
	}
	sdio_set_drvdata(func, NULL);

	sdio_claim_host(func);
	rc = sdio_disable_func(func);
	if(rc){
		DBG_871X("%s(): sdio_disable_func fail!\n", __FUNCTION__);
	}
	rc = sdio_release_irq(func);
	if(rc){
		DBG_871X("%s(): sdio_disable_irq fail!\n", __FUNCTION__);
	}	
	sdio_release_host(func);
}


static const struct sdio_device_id sdio_ids[] =
{
	{ SDIO_DEVICE(0x024c, 0x8195),.driver_data = RTL8195A},
};

static struct sdio_driver rtl8195a_sdio_driver = {
	.probe	= rtl8195a_init_one,
	.remove	= __devexit_p(rtl8195a_remove_one),
	.name	= MODULENAME,
	.id_table	= sdio_ids,
};



static int __init rtl8195a_init_module(void)
{

	int ret;
	ret = sdio_register_driver(&rtl8195a_sdio_driver);
	if(ret!=0)
		DBG_871X("sdio register driver Failed!\n");
	return ret;
}

static void __exit rtl8195a_cleanup_module(void)
{
	sdio_unregister_driver(&rtl8195a_sdio_driver);
}

module_init(rtl8195a_init_module);
module_exit(rtl8195a_cleanup_module);
