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
#include "autoconf.h"
#include "rtw_debug.h"
#include "sdio_ops.h"
#include "hal_intf.h"
#include "8195_desc.h"
#include "8195_sdio_reg.h"
#include "rtl8195a.h"
#include "rtl8195a_hal.h"
#include "osdep_service.h"
#include "osdep_intf.h"
#include "sdio_hal.h"
#include "sdio_ops_linux.h"
#include "rtw_ioctl.h"
#include "rtw_xmit.h"
#include "rtw_recv.h"
#include "rtw_io.h"
#include "rtw_cmd.h"

MODULE_AUTHOR("Realtek");
MODULE_DESCRIPTION("RealTek RTL-8195a iNIC");
MODULE_LICENSE("GPL");
MODULE_VERSION(RTL8195_VERSION);

static u32 sdio_init(struct dvobj_priv *dvobj)
{
	PSDIO_DATA psdio_data;
	struct sdio_func *func;
	int err;

_func_enter_;

	psdio_data = &dvobj->intf_data;
	func = psdio_data->func;

	// 1. init SDIO bus
	sdio_claim_host(func);

	err = sdio_enable_func(func);
	if (err) {
		DBG_871X(KERN_CRIT "%s: sdio_enable_func FAIL(%d)!\n", __func__, err);
		goto release;
	}

	err = sdio_set_block_size(func, 512);
	if (err) {
		DBG_871X(KERN_CRIT "%s: sdio_set_block_size FAIL(%d)!\n", __func__, err);
		goto release;
	}
	psdio_data->block_transfer_len = 512;
	psdio_data->tx_block_mode = 1;
	psdio_data->rx_block_mode = 1;

release:
	sdio_release_host(func);

_func_exit_;
	if (err) return _FAIL;
	return _SUCCESS;
}
static void sdio_deinit(struct dvobj_priv *dvobj)
{
	struct sdio_func *func;
	int err;

	func = dvobj->intf_data.func;

	if (func) {
		sdio_claim_host(func);
		err = sdio_disable_func(func);
		if (err)
		{
			DBG_871X(KERN_ERR "%s: sdio_disable_func(%d)\n", __func__, err);
		}

		if (dvobj->irq_alloc) {
			err = sdio_release_irq(func);
			if (err)
			{
				DBG_871X(KERN_ERR "%s: sdio_release_irq(%d)\n", __func__, err);
			}
		}

		sdio_release_host(func);
	}
}

static struct dvobj_priv *sdio_dvobj_init(struct sdio_func *func)
{
	int status = _FAIL;
	struct dvobj_priv *dvobj = NULL;
	PSDIO_DATA psdio;
_func_enter_;

	if((dvobj = devobj_init()) == NULL) {
		goto exit;
	}

	sdio_set_drvdata(func, dvobj);
	psdio = &dvobj->intf_data;
	psdio->func = func;
	psdio->SdioRxFIFOCnt = 0;	
	if (sdio_init(dvobj) != _SUCCESS) {
		RT_TRACE(_module_hci_intfs_c_, _drv_err_, ("%s: initialize SDIO Failed!\n", __FUNCTION__));
		goto free_dvobj;
	}
	rtw_reset_continual_io_error(dvobj);
	status = _SUCCESS;

free_dvobj:
	if (status != _SUCCESS && dvobj) {
		sdio_set_drvdata(func, NULL);
		
		devobj_deinit(dvobj);
		
		dvobj = NULL;
	}
exit:
_func_exit_;
	return dvobj;
}

static void sdio_dvobj_deinit(struct sdio_func *func)
{
	struct dvobj_priv *dvobj = sdio_get_drvdata(func);
_func_enter_;

	sdio_set_drvdata(func, NULL);
	if (dvobj) {
		sdio_deinit(dvobj);
		devobj_deinit(dvobj);
	}

_func_exit_;
	return;
}


static void sd_sync_int_hdl(struct sdio_func *func)
{
	struct dvobj_priv *psdpriv;
	psdpriv = sdio_get_drvdata(func);
	rtw_sdio_set_irq_thd(psdpriv, current);
	sd_int_hal(psdpriv->if1);
	rtw_sdio_set_irq_thd(psdpriv, NULL);
}

int sdio_alloc_irq(struct dvobj_priv *dvobj)
{
	PSDIO_DATA psdio_data;
	struct sdio_func *func;
	int err = 0;

	psdio_data = &dvobj->intf_data;
	func = psdio_data->func;

	sdio_claim_host(func);

	err = sdio_claim_irq(func, &sd_sync_int_hdl);
	if (err)
	{
		printk(KERN_CRIT "%s: sdio_claim_irq FAIL(%d)!\n", __func__, err);
	}
	else
	{
		dvobj->irq_alloc = 1;
	}

	sdio_release_host(func);

	return err?_FAIL:_SUCCESS;
}

void sdio_free_irq(struct dvobj_priv *dvobj)
{
    PSDIO_DATA psdio_data;
    struct sdio_func *func;
    int err;

    if (dvobj->irq_alloc) {
        psdio_data = &dvobj->intf_data;
        func = psdio_data->func;

        if (func) {
            sdio_claim_host(func);
            err = sdio_release_irq(func);
            if (err)
            {
				DBG_871X("%s: sdio_release_irq FAIL(%d)!\n", __func__, err);
            }
            sdio_release_host(func);
        }
        dvobj->irq_alloc = 0;
    }
}


void rtw_set_hal_ops(PADAPTER padapter)
{
	rtl8195as_set_hal_ops(padapter);
}

static void sd_intf_start(PADAPTER padapter)
{
	if (padapter == NULL) {
		DBG_871X(KERN_ERR "%s: padapter is NULL!\n", __func__);
		return;
	}

	// hal dep
	rtw_hal_enable_interrupt(padapter);   
}

static void sd_intf_stop(PADAPTER padapter)
{
	if (padapter == NULL) {
		DBG_871X(KERN_ERR "%s: padapter is NULL!\n", __func__);
		return;
	}

	// hal dep
	rtw_hal_disable_interrupt(padapter);
}


_adapter *rtw_sdio_if1_init(struct dvobj_priv *dvobj, const struct sdio_device_id  *pdid){
	int status = _FAIL;
	struct net_device *pnetdev;
	PADAPTER padapter = NULL;
	u8 mac_addr[ETH_ALEN];
	u16 fw_ready;
	u32 i;
  
_func_enter_;
	if ((padapter = (_adapter *)rtw_zvmalloc(sizeof(*padapter))) == NULL) {
		DBG_871X("%s: vmalloc for padapter failed!\n", __FUNCTION__);
		goto exit;
	}
	padapter->dvobj = dvobj;
	dvobj->if1 = padapter;
	padapter->interface_type = RTW_SDIO;
	// 1. init network device data
	pnetdev = rtw_init_netdev(padapter);
	if (!pnetdev)
		goto free_adapter;
	SET_NETDEV_DEV(pnetdev, &dvobj->intf_data.func->dev);
	padapter = rtw_netdev_priv(pnetdev);

	// 2. init driver special setting, interface, OS and hardware relative
	rtw_set_hal_ops(padapter);
	
	// 3. initialize Chip version
	padapter->intf_start = &sd_intf_start;
	padapter->intf_stop = &sd_intf_stop;

	padapter->intf_init = &sdio_init;
	padapter->intf_deinit = &sdio_deinit;
	padapter->intf_alloc_irq = &sdio_alloc_irq;
	padapter->intf_free_irq = &sdio_free_irq;
	
	sdio_set_intf_ops(padapter, &padapter->io_ops);

	// 4. init driver common data
	if (rtw_init_drv_sw(padapter) == _FAIL) {
		goto free_adapter;
	}

	// 5. get MAC address
	mac_addr[0] = 0x00;
	mac_addr[1] = 0xe0;
	mac_addr[2] = 0x4c;
	mac_addr[3] = 0xB7;
	mac_addr[4] = 0x23;
	mac_addr[5] = 0x00;
	_rtw_memcpy(pnetdev->dev_addr, mac_addr, ETH_ALEN);
#ifdef CONFIG_FWDL
	// wait for the device boot code ready
	for (i=0;i<100;i++) {
		fw_ready = rtw_read16(padapter, SDIO_REG_HCPWM2);
		if (fw_ready & SDIO_INIT_DONE) {
			break;
		}
		rtw_msleep_os(10);
	}
	if (i==100) {
		DBG_871X("%s: Wait Device Firmware Ready Timeout!!SDIO_REG_HCPWM2 @ 0x%04x\n", __FUNCTION__, fw_ready);
		goto free_adapter;
	}
#else
	// wait for the device firmware ready
	for (i=0;i<100;i++) {
		fw_ready = rtw_read8(padapter, SDIO_REG_CPU_IND);
		if (fw_ready & SDIO_SYSTEM_TRX_RDY_IND) {
			break;
		}
		rtw_msleep_os(10);
	}
	if (i==100) {
		DBG_871X("%s: Wait Device Firmware Ready Timeout!!SDIO_REG_CPU_IND @ 0x%04x\n", __FUNCTION__, fw_ready);
		goto free_adapter;
	}
#endif
	rtw_hal_disable_interrupt(padapter);
	DBG_871X("bDriverStopped:%d, bSurpriseRemoved:%d, bup:%d\n"
		,padapter->bDriverStopped
		,padapter->bSurpriseRemoved
		,padapter->bup
	);
	status = _SUCCESS;
free_adapter:
	if (status != _SUCCESS) {
		if (pnetdev)
			rtw_free_netdev(pnetdev);
		else
			rtw_vmfree((u8*)padapter, sizeof(*padapter));
		padapter = NULL;
	}	
exit:
	_func_exit_;
	return padapter;
}

static void rtw_sdio_if1_deinit(_adapter *if1)
{
	struct net_device *pnetdev = if1->pnetdev;
_func_enter_;
	rtw_dev_unload(if1);
	rtw_free_drv_sw(if1);
	if(pnetdev)
		rtw_free_netdev(pnetdev);
_func_exit_;
}
static int __devinit rtl8195a_init_one(struct sdio_func *func, const struct sdio_device_id *id)
{
	int status = _FAIL;
	PADAPTER padapter;
	struct dvobj_priv *dvobj;

	DBG_871X("%s():++\n",__FUNCTION__);
	DBG_871X("+rtw_drv_init: vendor=0x%04x device=0x%04x class=0x%02x\n", func->vendor, func->device, func->class);
	
	// 1. init sdio bus
	if ((dvobj = sdio_dvobj_init(func)) == NULL) {
		goto exit;
	}
	
  	// 2. init device interface 
	if ((padapter = rtw_sdio_if1_init(dvobj, id)) == NULL) {
		DBG_871X("rtw_init_adapter Failed!\n");
		goto free_dvobj;
	}
	
	// 3. dev_alloc_name && register_netdev
	if((rtw_drv_register_netdev(padapter)) != _SUCCESS) {
		goto free_adapter;
	}	

	// 4. register for irq hander
	if (sdio_alloc_irq(dvobj) != _SUCCESS)
	{
		goto free_adapter;
	}

	status = _SUCCESS;
	
free_adapter:
	if (status != _SUCCESS && padapter) {
		rtw_sdio_if1_deinit(padapter);
	}	
free_dvobj:
	if (status != _SUCCESS)
		sdio_dvobj_deinit(func);
	
exit:
	return status == _SUCCESS?0:-ENODEV;
}

static void __devexit rtl8195a_remove_one(struct sdio_func *func)

{
	PADAPTER padapter;
	struct dvobj_priv *dvobj = sdio_get_drvdata(func);
	padapter = dvobj->if1;
	DBG_871X("%s():++\n", __FUNCTION__);	
	if(padapter)
	{	
		rtw_drv_unregister_netdev(padapter);
		if (padapter->bSurpriseRemoved == _FALSE) {
			int err;

			/* test surprise remove */
			sdio_claim_host(func);
			sdio_readb(func, 0, &err);
			sdio_release_host(func);
			if (err == -ENOMEDIUM) {
				padapter->bSurpriseRemoved = _TRUE;
				DBG_871X(KERN_NOTICE "%s: device had been removed!\n", __func__);
			}
		}
		rtw_sdio_if1_deinit(padapter);
	}
	sdio_dvobj_deinit(func);
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
