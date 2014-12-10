#ifndef __DRV_TYPES_H__
#define __DRV_TYPES_H__
#include "autoconf.h"
#include "osdep_service.h"
#include "8195_desc.h"

typedef struct _ADAPTER _adapter,ADAPTER,*PADAPTER;
typedef struct _AT_CMD_DESC AT_CMD_DESC, *PAT_CMD_DESC;


#ifdef CONFIG_SDIO_HCI
#include "drv_types_sdio.h"
#define INTF_DATA SDIO_DATA
#elif defined(CONFIG_USB_HCI)
#include "drv_types_usb.h"
#define INTF_DATA USB_DATA *
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
};

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

#include "rtw_xmit.h"
#include "rtw_recv.h"
#include "rtw_cmd.h"
#include "hal_intf.h"
#include "rtw_io.h"
struct _ADAPTER
{
	_lock lock;
#ifdef PLATFORM_LINUX
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

