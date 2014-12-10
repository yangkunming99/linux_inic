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


void rtw_set_hal_ops(PADAPTER padapter)
{
	rtl8195a_set_hal_ops(padapter);
}

_adapter *rtw_sdio_if1_init(struct sdio_func *func, const struct sdio_device_id  *pdid){
	int status = _FAIL;
	struct net_device *pnetdev;
	PADAPTER padapter = NULL;
	PSDIO_DATA psdio;
	u8 mac_addr[ETH_ALEN];
	if ((padapter = (_adapter *)rtw_zvmalloc(sizeof(*padapter))) == NULL) {
		goto exit;
	}
	//3 1. init network device data
	pnetdev = rtw_init_netdev(padapter);
	if (!pnetdev)
		goto free_adapter;
	SET_NETDEV_DEV(pnetdev, &func->dev);
	padapter = rtw_netdev_priv(pnetdev);
	//3 3. init driver special setting, interface, OS and hardware relative
	rtw_set_hal_ops(padapter);

	sdio_set_intf_ops(padapter, &padapter->io_ops);

	//4 4. init driver common data
	if (rtw_init_drv_sw(padapter) == _FAIL) {
		goto free_adapter;
	}

	//5 5. get MAC address
	mac_addr[0] = 0x00;
	mac_addr[1] = 0xe0;
	mac_addr[2] = 0x4c;
	mac_addr[3] = 0x87;
	mac_addr[4] = 0x00;
	mac_addr[5] = 0x00;
	_rtw_memcpy(pnetdev->dev_addr, mac_addr, ETH_ALEN);

	psdio = &padapter->intf_data;
	psdio->func= func;
	psdio->SdioRxFIFOCnt = 0;	
	sdio_set_drvdata(func, padapter);
	
free_adapter:
	if (status != _SUCCESS) {
		if (pnetdev)
			rtw_free_netdev(pnetdev);
		else
			rtw_vmfree((u8*)padapter, sizeof(*padapter));
		padapter = NULL;
	}	
exit:
	return padapter;
}

static void rtw_sdio_if1_deinit(_adapter *if1)
{
	struct net_device *pnetdev = if1->pnetdev;
	PSDIO_DATA psdio = if1->intf_data;
	struct sdio_func *func = psdio->func;
	rtw_free_drv_sw(if1);
	if(pnetdev)
		rtw_free_netdev(pnetdev);
	sdio_set_drvdata(func, NULL);
}
static int __devinit rtl8195a_init_one(struct sdio_func *func, const struct sdio_device_id *id)
{
	struct net_device *pnetdev;
	PADAPTER padapter;

	int rc = 0;

	DBG_871X("%s():++\n",__FUNCTION__);
	DBG_871X("+rtw_drv_init: vendor=0x%04x device=0x%04x class=0x%02x\n", func->vendor, func->device, func->class);
	
	
	// 1.init SDIO bus and read chip version	
	rc = sdio_init(func);
	if(rc)
		goto exit;
	
	// 2. init SDIO interface 
	if ((padapter = rtw_sdio_if1_init(func, id)) == NULL) {
		DBG_871X("rtw_init_adapter Failed!\n");
		rc = -ENODEV;
		goto free_sdio;
	}
	
	// 2.dev_alloc_name && register_netdev
	if((rtw_drv_register_netdev(padapter)) != _SUCCESS) {
		rc = -ENODEV;
		goto free_adapter;
	}	

	if (sdio_alloc_irq(func) != _SUCCESS)
	{
		rc = -ENODEV;
		goto free_netdev;
	}

	goto exit;
free_netdev:
	rtw_drv_unregister_netdev(padapter);
	
free_adapter:
	rtw_sdio_if1_deinit(padapter);
	
free_sdio:
	sdio_deinit(func);
	
exit:
	
	return rc;
}

static void __devexit rtl8195a_remove_one(struct sdio_func *func)

{

	int rc = 0;
	int err;
	struct net_device *pnetdev;
	PADAPTER padapter;
	DBG_871X("%s():++\n", __FUNCTION__);
	padapter = sdio_get_drvdata(func);
	
	if(padapter)
	{	
		rtw_drv_unregister_netdev(padapter);
		/* test surprise remove */
		sdio_claim_host(func);
		sdio_readb(func, 0, &err);
		sdio_release_host(func);
		if (err == -ENOMEDIUM) {
			DBG_871X(KERN_NOTICE "%s: device had been removed!\n", __func__);
		}
		rtw_sdio_if1_deinit(padapter);
	}
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
