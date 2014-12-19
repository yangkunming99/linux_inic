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


#include <linux/usb.h>

#include "autoconf.h"
#include "drv_types.h"
#include "usb_ops.h"
#include "rtl8195a.h"
#include "rtw_debug.h"
#include "osdep_service.h"
#include "rtw_ioctl.h"
#include "rtw_xmit.h"
#include "rtw_recv.h"
#include "rtw_io.h"
#include "osdep_intf.h"
#include "usb_hal.h"


MODULE_AUTHOR("Realtek");
MODULE_DESCRIPTION("RealTek RTL-8195a iNIC");
MODULE_LICENSE("GPL");
MODULE_VERSION(RTL8195_VERSION);
#define FUC_IN DBG_871X("%s()====>\n", __FUNCTION__)
#define FUC_OUT DBG_871X("%s()<====\n", __FUNCTION__)
#define FUC_TRACE DBG_871X("%s()[line:%d]\n", __FUNCTION__,__LINE__)

#define USB_VENDER_ID_REALTEK		0x0BDA


static inline int RT_usb_endpoint_dir_in(const struct usb_endpoint_descriptor *epd)
{
	return ((epd->bEndpointAddress & USB_ENDPOINT_DIR_MASK) == USB_DIR_IN);
}

static inline int RT_usb_endpoint_dir_out(const struct usb_endpoint_descriptor *epd)
{
	return ((epd->bEndpointAddress & USB_ENDPOINT_DIR_MASK) == USB_DIR_OUT);
}

static inline int RT_usb_endpoint_xfer_int(const struct usb_endpoint_descriptor *epd)
{
	return ((epd->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_INT);
}

static inline int RT_usb_endpoint_xfer_bulk(const struct usb_endpoint_descriptor *epd)
{
 	return ((epd->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_BULK);
}

static inline int RT_usb_endpoint_is_bulk_in(const struct usb_endpoint_descriptor *epd)
{
	return (RT_usb_endpoint_xfer_bulk(epd) && RT_usb_endpoint_dir_in(epd));
}

static inline int RT_usb_endpoint_is_bulk_out(const struct usb_endpoint_descriptor *epd)
{
	return (RT_usb_endpoint_xfer_bulk(epd) && RT_usb_endpoint_dir_out(epd));
}

static inline int RT_usb_endpoint_is_int_in(const struct usb_endpoint_descriptor *epd)
{
	return (RT_usb_endpoint_xfer_int(epd) && RT_usb_endpoint_dir_in(epd));
}

static inline int RT_usb_endpoint_num(const struct usb_endpoint_descriptor *epd)
{
	return epd->bEndpointAddress & USB_ENDPOINT_NUMBER_MASK;
}


static u8 rtw_init_intf_priv(struct dvobj_priv *dvobj){
	if(dvobj == NULL)
		return _FAIL;
//	_rtw_spinlock_init(&(dvobj->devlock));
	_rtw_init_sema(&(dvobj->usb_suspend_sema), 0);
//	_rtw_mutex_init(&(dvobj->io_mutex));	
//	_rtw_init_sema(&(dvobj->tx_urb_done),0);
	return _SUCCESS;
}
static void rtw_deinit_intf_priv(struct dvobj_priv *dvobj){
//	_rtw_spinlock_free(&(dvobj->devlock));
	_rtw_free_sema(&(dvobj->usb_suspend_sema));
//	_rtw_mutex_free(&(dvobj->io_mutex));
//	_rtw_free_sema(&(dvobj->tx_urb_done));
}

static int rtl8195a_get_pipes(struct dvobj_priv *pdvobjpriv)
{
	
	struct usb_host_interface *altsetting =
		pdvobjpriv->pusbintf->cur_altsetting;
	
	struct usb_endpoint_descriptor *ep;
	struct usb_endpoint_descriptor *ep_in = NULL;
	struct usb_endpoint_descriptor *ep_out = NULL;

	int i;
	
	for (i = 0; i < altsetting->desc.bNumEndpoints; i++) {
		ep = &altsetting->endpoint[i].desc;

		if (RT_usb_endpoint_xfer_bulk(ep)) {
			if (RT_usb_endpoint_is_bulk_in(ep)) {
				if (!ep_in) 
					ep_in = ep;
			} else {
				if (!ep_out)
					ep_out = ep;
			}
		}
	}

	if (!ep_in || !ep_out) {
		DBG_871X("Endpoint sanity check failed! Rejecting dev.\n");
		return -EIO;
	}

	/* Calculate and store the pipe values */
//	pusb->send_ctrl_pipe = usb_sndctrlpipe(us->pusb_dev, 0);
//	pusb->recv_ctrl_pipe = usb_rcvctrlpipe(us->pusb_dev, 0);
	pdvobjpriv->send_bulk_Pipe = usb_sndbulkpipe(pdvobjpriv->pusbdev,
		RT_usb_endpoint_num(ep_out));
	pdvobjpriv->recv_bulk_Pipe = usb_rcvbulkpipe(pdvobjpriv->pusbdev,
		RT_usb_endpoint_num(ep_in));
	return 0;
}

static struct dvobj_priv *usb_dvobj_init(struct usb_interface *intf ){

	struct dvobj_priv *pdvobjpriv;
	struct usb_device_descriptor 	*pdev_desc;
	struct usb_host_config			*phost_conf;
	struct usb_config_descriptor	*pconf_desc;
	struct usb_host_interface		*phost_iface;
	struct usb_interface_descriptor	*piface_desc;
	struct usb_host_endpoint		*phost_endp;
	struct usb_endpoint_descriptor	*pendp_desc;
	struct usb_device				*pusbd;
	int i;
	int status = _FAIL;

	if((pdvobjpriv = devobj_init()) == NULL) {
		goto exit;
	}
	pdvobjpriv->pusbintf = intf;
	pusbd = pdvobjpriv->pusbdev = interface_to_usbdev(intf);
	usb_set_intfdata(intf, pdvobjpriv);
	pdvobjpriv->RtNumInPipes= 0;
	pdvobjpriv->RtNumOutPipes= 0;

	pdev_desc = &pusbd->descriptor;

	phost_conf = pusbd->actconfig;
	pconf_desc = &phost_conf->desc;

	phost_iface = &intf->altsetting[0];
	piface_desc = &phost_iface->desc;

	pdvobjpriv->NumInterfaces = pconf_desc->bNumInterfaces;
	pdvobjpriv->InterfaceNumber = piface_desc->bInterfaceNumber;
	pdvobjpriv->nr_endpoint = piface_desc->bNumEndpoints;

	for (i = 0; i < pdvobjpriv->nr_endpoint; i++)
	{
		phost_endp = phost_iface->endpoint + i;
		if (phost_endp)
		{
			pendp_desc = &phost_endp->desc;

			DBG_871X("\nusb_endpoint_descriptor(%d):\n", i);
			DBG_871X("bLength=%x\n",pendp_desc->bLength);
			DBG_871X("bDescriptorType=%x\n",pendp_desc->bDescriptorType);
			DBG_871X("bEndpointAddress=%x\n",pendp_desc->bEndpointAddress);
			//DBG_871X("bmAttributes=%x\n",pendp_desc->bmAttributes);
			DBG_871X("wMaxPacketSize=%d\n",le16_to_cpu(pendp_desc->wMaxPacketSize));
			DBG_871X("bInterval=%x\n",pendp_desc->bInterval);
			//DBG_871X("bRefresh=%x\n",pendp_desc->bRefresh);
			//DBG_871X("bSynchAddress=%x\n",pendp_desc->bSynchAddress);

			if (RT_usb_endpoint_is_bulk_in(pendp_desc))
			{
				DBG_871X("RT_usb_endpoint_is_bulk_in = %x\n", RT_usb_endpoint_num(pendp_desc));
				pdvobjpriv->RtInPipe[pdvobjpriv->RtNumInPipes] = RT_usb_endpoint_num(pendp_desc);
				pdvobjpriv->RtNumInPipes++;
			}
			else if (RT_usb_endpoint_is_int_in(pendp_desc))
			{
				DBG_871X("RT_usb_endpoint_is_int_in = %x, Interval = %x\n", RT_usb_endpoint_num(pendp_desc),pendp_desc->bInterval);
				pdvobjpriv->RtInPipe[pdvobjpriv->RtNumInPipes] = RT_usb_endpoint_num(pendp_desc);
				pdvobjpriv->RtNumInPipes++;
			}
			else if (RT_usb_endpoint_is_bulk_out(pendp_desc))
			{
				DBG_871X("RT_usb_endpoint_is_bulk_out = %x\n", RT_usb_endpoint_num(pendp_desc));
				pdvobjpriv->RtOutPipe[pdvobjpriv->RtNumOutPipes] = RT_usb_endpoint_num(pendp_desc);
				pdvobjpriv->RtNumOutPipes++;
			}
			pdvobjpriv->ep_num[i] = RT_usb_endpoint_num(pendp_desc);
		}
	}
	
	
	DBG_871X("nr_endpoint=%d, in_num=%d, out_num=%d\n\n", pdvobjpriv->nr_endpoint, pdvobjpriv->RtNumInPipes, pdvobjpriv->RtNumOutPipes);
	
		switch(pusbd->speed) {
			case USB_SPEED_LOW:
				DBG_871X("USB_SPEED_LOW\n");
				pdvobjpriv->usb_speed = RTW_USB_SPEED_1_1;
				break;
			case USB_SPEED_FULL:
				DBG_871X("USB_SPEED_FULL\n");
				pdvobjpriv->usb_speed = RTW_USB_SPEED_1_1;
				break;
			case USB_SPEED_HIGH:
				DBG_871X("USB_SPEED_HIGH\n");
				pdvobjpriv->usb_speed = RTW_USB_SPEED_2;
				break;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,31))
			case USB_SPEED_SUPER:
				DBG_871X("USB_SPEED_SUPER\n");
				pdvobjpriv->usb_speed = RTW_USB_SPEED_3;
				break;
#endif
			default:
				DBG_871X("USB_SPEED_UNKNOWN(%x)\n",pusbd->speed);
				pdvobjpriv->usb_speed = RTW_USB_SPEED_UNKNOWN;
				break;
		}


	if (rtw_init_intf_priv(pdvobjpriv) == _FAIL) {
			DBG_871X("Can't INIT rtw_init_intf_priv\n");
			goto free_dvobj;
		}

	FUC_TRACE;

	if (rtl8195a_get_pipes(pdvobjpriv))
		goto free_dvobj;
	
	usb_get_dev(pusbd);	
	status = _SUCCESS;
	
free_dvobj:
		if (status != _SUCCESS && pdvobjpriv) {
			usb_set_intfdata(intf, NULL);
			
			devobj_deinit(pdvobjpriv);
			
			pdvobjpriv = NULL;
		}

exit:
	return pdvobjpriv;
}
static void *usb_dvobj_deinit(struct usb_interface *intf ){
	struct dvobj_priv *dvobj = usb_get_intfdata(intf);

	usb_set_intfdata(intf, NULL);
	if (dvobj) {
#if 0
		//Modify condition for 92DU DMDP 2010.11.18, by Thomas
		if ((pusb_data->NumInterfaces != 2 && pusb_data->NumInterfaces != 3)
			|| (pusb_data->InterfaceNumber == 1)) {
			if (interface_to_usbdev(intf)->state != USB_STATE_NOTATTACHED) {
				//If we didn't unplug usb dongle and remove/insert modlue, driver fails on sitesurvey for the first time when device is up .
				//Reset usb port for sitesurvey fail issue. 2009.8.13, by Thomas
				DBG_871X("usb attached..., try to reset usb device\n");
				usb_reset_device(interface_to_usbdev(intf));
			}
		}
#endif
		
		rtw_deinit_intf_priv(dvobj);
		devobj_deinit(dvobj);		
	}

	//DBG_871X("%s %d\n", __func__, ATOMIC_READ(&usb_intf->dev.kobj.kref.refcount));
	usb_put_dev(interface_to_usbdev(intf));
}

static const struct usb_device_id rtl8195a_usb_ids[] =
{
	{ USB_DEVICE(USB_VENDER_ID_REALTEK, 0x8195)},
};
/* the number of entries in array above */
int const rtl8195a_usb_id_len =
	sizeof(rtl8195a_usb_ids) / sizeof(struct usb_device_id);

int rtw_ndev_init(struct net_device *dev){

	return 0;
}

void rtw_ndev_uninit(struct net_device *dev){
;
}

void usb_set_intf_ops(PADAPTER padapter,struct _io_ops *pops){
		rtl8195au_set_intf_ops(pops);
}


void rtw_set_hal_ops(PADAPTER padapter)
{
	rtl8195au_set_hal_ops(padapter);
}


static void usb_intf_start(_adapter *padapter)
{
//	struct net_device *pnetdev = padapter->pnetdev;
	RT_TRACE(_module_hci_intfs_c_,_drv_err_,("+usb_intf_start\n"));

	rtw_hal_inirp_init(padapter);
	
	RT_TRACE(_module_hci_intfs_c_,_drv_err_,("-usb_intf_start\n"));

}

static void usb_intf_stop(_adapter *padapter)
{

	RT_TRACE(_module_hci_intfs_c_,_drv_err_,("+usb_intf_stop\n"));

	//disabel_hw_interrupt
	if(padapter->bSurpriseRemoved == _FALSE)
	{
		//device still exists, so driver can do i/o operation
		//TODO:
		RT_TRACE(_module_hci_intfs_c_,_drv_err_,("SurpriseRemoved==_FALSE\n"));
	}

	//cancel in irp
	rtw_hal_inirp_deinit(padapter);

	//cancel out irp
	rtw_write_port_cancel(padapter);

	//todo:cancel other irps

	RT_TRACE(_module_hci_intfs_c_,_drv_err_,("-usb_intf_stop\n"));

}


/*
 * drv_init() - a device potentially for us
 *
 * notes: drv_init() is called when the bus driver has located a card for us to support.
 *        We accept the new device by returning 0.
*/

_adapter *rtw_usb_if1_init(struct dvobj_priv *dvobj,
	struct usb_interface *intf, const struct usb_device_id *pdid)
{
	_adapter *padapter = NULL;
	struct net_device *pnetdev = NULL;
	int status = _FAIL;
	u8 mac_addr[ETH_ALEN];


	if ((padapter = (_adapter *)rtw_zvmalloc(sizeof(*padapter))) == NULL) {
		goto exit;
	}
	padapter->dvobj = dvobj;
	dvobj->if1 = padapter;
	padapter->interface_type = RTW_USB;
	
	//padapter->bDriverStopped = _TRUE;
	padapter->bDriverStopped = _FALSE;

	
	//3 1. init network device data
	pnetdev = rtw_init_netdev(padapter);
	if (!pnetdev)
		goto free_adapter;
	SET_NETDEV_DEV(pnetdev, &intf->dev);
	padapter = rtw_netdev_priv(pnetdev);

	//2. init driver special setting, interface, OS and hardware relative
 	rtw_set_hal_ops(padapter);

 	padapter->intf_start = &usb_intf_start;
	padapter->intf_stop = &usb_intf_stop;
	
 	usb_set_intf_ops(padapter, &padapter->io_ops);

	//4 3. init driver common data
	if (rtw_init_drv_sw(padapter) == _FAIL) {
		goto free_adapter;
	}

	//5 5. get MAC address
	mac_addr[0] = 0x00;
	mac_addr[1] = 0xe0;
	mac_addr[2] = 0x4c;
	mac_addr[3] = 0x87;
	mac_addr[4] = 0x00;
	mac_addr[5] = 0x01;
	_rtw_memcpy(pnetdev->dev_addr, mac_addr, ETH_ALEN);

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
	return padapter;
}

static void rtw_usb_if1_deinit(_adapter *adapter){
	
	struct net_device *pnetdev = adapter->pnetdev;
	rtw_free_drv_sw(adapter);
	if(pnetdev)
		rtw_free_netdev(pnetdev);
}


static int rtl8195a_usb_probe( struct usb_interface *intf,
				const struct usb_device_id *id_table){
	PADAPTER padapter;
	struct dvobj_priv *dvobj;
	int status = _FAIL;


	//1. check usb driver and init usb 
	int i;
	for (i = 0; i < rtl8195a_usb_id_len; i++)
		{
			if (id_table->idVendor == rtl8195a_usb_ids[i].idVendor &&
				id_table->idProduct == rtl8195a_usb_ids[i].idProduct)
				break;
		}

	if (i == rtl8195a_usb_id_len) {
		DBG_871X("Device Descriptor not matching\n");
		goto exit;
	}

	if((dvobj = usb_dvobj_init(intf)) == NULL){
		DBG_871X("%s()====>usb_dvobj_init() failed!\n", __FUNCTION__);
		goto exit;
		}
	
	if ((padapter = rtw_usb_if1_init(dvobj, intf, id_table)) == NULL) {
		DBG_871X("rtw_usb_if1_init Failed!\n");
		goto free_dvobj;
	}
	//dev_alloc_name && register_netdev
	if((status = rtw_drv_register_netdev(padapter)) != _SUCCESS) {
		goto free_all;
	}

	status = _SUCCESS;
#if 0

	if (rtl81951_usb_reset(pusb))
		DBG_871X("rtl81951_usb_reset done\n");
	else 
		DBG_871X("rtl81951_usb_reset fail\n");
	
	pnetdev = rtw_alloc_etherdev(sizeof(ADAPTER));
	if(!pnetdev)
	{
		usb_deinit(intf);
		return -ENODEV;
	}
#endif
#if 0
	if (!usb_send_data(pusb))
		printk("send data done=>\n");
	else 
		printk("send data fail=>\n");
#endif 

//usb_read_data(pusb);

free_all:
	if (status != _SUCCESS && padapter)
		rtw_usb_if1_deinit(padapter);

free_dvobj:
	if (status != _SUCCESS)
		usb_dvobj_deinit(intf);

exit:
	return status == _SUCCESS?0:-ENODEV;
}

static void rtl8195a_usb_disconnect(struct usb_interface *intf){
	struct dvobj_priv *dvobj = usb_get_intfdata(intf);		
	PADAPTER padapter = dvobj->if1;
FUC_IN;
	if(padapter){
		rtw_drv_unregister_netdev(padapter);
		rtw_usb_if1_deinit(padapter);
	}
	usb_dvobj_deinit(intf);
FUC_OUT;
}


static int rtl8195a_usb_suspend(struct usb_interface *intf, pm_message_t message){
	DBG_871X("%s\n",__FUNCTION__);
	return 0;
}
static void rtl8195a_usb_resume(struct usb_interface *intf){
	DBG_871X("%s\n",__FUNCTION__);
}

static struct usb_driver rtl8195a_usb_driver = {
	.name =		MODULENAME,
	.probe = 	rtl8195a_usb_probe,
	.disconnect=rtl8195a_usb_disconnect,
	.suspend =	rtl8195a_usb_suspend,
	.resume =	rtl8195a_usb_resume,
	.id_table=	rtl8195a_usb_ids,
};


module_usb_driver(rtl8195a_usb_driver);

MODULE_LICENSE("GPL");

