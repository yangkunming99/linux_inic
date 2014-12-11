#ifndef __DRV_TYPES_H__
#define __DRV_TYPES_H__
#include "autoconf.h"


typedef struct _ADAPTER _adapter,ADAPTER,*PADAPTER;
typedef struct _AT_CMD_DESC AT_CMD_DESC, *PAT_CMD_DESC;
#include "rtw_debug.h"
#include "osdep_service.h"

#ifdef CONFIG_SDIO_HCI
#include "drv_types_sdio.h"
#define INTF_DATA SDIO_DATA
#elif	defined(CONFIG_USB_HCI)
#include "drv_types_usb.h"
#endif

struct dvobj_priv
{
	_lock lock;
	/*-------- below is common data --------*/	
	_adapter *if1; //PRIMARY_ADAPTER
	u8	irq_alloc;
	ATOMIC_T continual_io_error;

	/*-------- below is for SDIO INTERFACE --------*/
#ifdef INTF_DATA
	INTF_DATA intf_data;
#endif


/*-------- below is for USB INTERFACE --------*/

#ifdef CONFIG_USB_HCI

	u8	usb_speed; // 1.1, 2.0 or 3.0
	u8	nr_endpoint;
	u8	RtNumInPipes;
	u8	RtNumOutPipes;
	int	ep_num[6]; //endpoint number

	int	RegUsbSS;

	_sema	usb_suspend_sema;

	PURB rx_urb;
	PURB tx_urb;
	PURB intr_urb;

	uint recv_bulk_Pipe;
	uint send_bulk_Pipe;
	
	//For 92D, DMDP have 2 interface.
	u8	InterfaceNumber;
	u8	NumInterfaces;
	
	//In /Out Pipe information
	int RtInPipe[2];
	int RtOutPipe[4];


#ifdef CONFIG_USB_VENDOR_REQ_MUTEX
	_mutex  usb_vendor_req_mutex;
#endif

#ifdef CONFIG_USB_VENDOR_REQ_BUFFER_PREALLOC
	u8 * usb_alloc_vendor_req_buf;
	u8 * usb_vendor_req_buf;
#endif

#ifdef PLATFORM_WINDOWS
	//related device objects
	PDEVICE_OBJECT	pphysdevobj;//pPhysDevObj;
	PDEVICE_OBJECT	pfuncdevobj;//pFuncDevObj;
	PDEVICE_OBJECT	pnextdevobj;//pNextDevObj;

	u8	nextdevstacksz;//unsigned char NextDeviceStackSize;	//= (CHAR)CEdevice->pUsbDevObj->StackSize + 1;

	//urb for control diescriptor request

#ifdef PLATFORM_OS_XP
	struct _URB_CONTROL_DESCRIPTOR_REQUEST descriptor_urb;
	PUSB_CONFIGURATION_DESCRIPTOR	pconfig_descriptor;//UsbConfigurationDescriptor;
#endif

#ifdef PLATFORM_OS_CE
	WCHAR			active_path[MAX_ACTIVE_REG_PATH];	// adapter regpath
	USB_EXTENSION	usb_extension;

	_nic_hdl		pipehdls_r8192c[0x10];
#endif

	u32	config_descriptor_len;//ULONG UsbConfigurationDescriptorLength;
#endif//PLATFORM_WINDOWS

#ifdef PLATFORM_LINUX
			struct usb_interface *pusbintf;
			struct usb_device *pusbdev;
#endif//PLATFORM_LINUX


#ifdef PLATFORM_FREEBSD
	struct usb_interface *pusbintf;
	struct usb_device *pusbdev;
#endif//PLATFORM_FREEBSD
	
#endif//CONFIG_USB_HCI

};
/*
#ifdef PLATFORM_LINUX
static struct device *dvobj_to_dev(struct dvobj_priv *dvobj)
{
#ifdef CONFIG_USB_HCI
	return &dvobj->intf_data->intf->dev;
#endif
#ifdef CONFIG_SDIO_HCI
	return &dvobj->intf_data.func->dev;
#endif
}
#endif
*/
#include "rtw_io.h"
#include "rtw_xmit.h"
#include "rtw_recv.h"
#include "rtw_cmd.h"
#include "hal_intf.h"
struct _ADAPTER
{
	_lock lock;
	s32	bDriverStopped;
	s32	bSurpriseRemoved;
	int net_closed;	
	u8 netif_up;
#ifdef PLATFORM_LINUX
	int bup;
	_nic_hdl pnetdev;
	struct net_device_stats stats;
#endif
	u16	interface_type;//USB,SDIO
	struct dvobj_priv *dvobj;
	//For xmit priv
	struct xmit_priv xmitpriv;
	//For recv priv
	struct recv_priv recvpriv;
	//For cmd priv
	struct cmd_priv cmdpriv;
	struct _io_ops io_ops;
	struct hal_ops HalFunc;
	
 	u32 (*intf_init)(struct dvobj_priv *dvobj);
	void (*intf_deinit)(struct dvobj_priv *dvobj);
	int (*intf_alloc_irq)(struct dvobj_priv *dvobj);
	void (*intf_free_irq)(struct dvobj_priv *dvobj);
	void (*intf_start)(_adapter * adapter);
	void (*intf_stop)(_adapter * adapter);
};
struct _AT_CMD_DESC{
//DWORD 0
unsigned int pktsize: 16; //=tx_desc.pktsize - cmd_desc.offset
unsigned int offset: 8; //cmd header size
unsigned int resv: 7;
unsigned int datatype: 1; // only one bit used, 0: data frame 1: management frame
};
#define SIZE_AT_CMD_DESC (sizeof(AT_CMD_DESC))

/*********AT command set***********/
#define SIZE_AT_CMD_TYPE		2
#define MNGMT_FRAME			1
#define DATA_FRAME				0

#define AT_CMD_wifi_linked		"LK"
#define AT_CMD_wifi_unlinked	"UL"

#endif

