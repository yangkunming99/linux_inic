#ifndef __DRV_TYPES_H__
#define __DRV_TYPES_H__

#include "osdep_service.h"
#include "8195_desc.h"
typedef struct _ADAPTER ADAPTER,*PADAPTER;
typedef struct _AT_CMD_DESC AT_CMD_DESC, *PAT_CMD_DESC;
#include "rtw_xmit.h"
#include "rtw_recv.h"
#include "rtw_io.h"
#include "rtw_cmd.h"
#ifdef CONFIG_SDIO_HCI
#include "drv_types_sdio.h"
#define INTF_DATA SDIO_DATA
#elif defined(CONFIG_USB_HCI)
#include "drv_types_usb.h"
#define INTF_DATA USB_DATA
#endif
struct _ADAPTER
{
	_lock lock;
	_nic_hdl pnetdev;
	struct net_device_stats stats;
#ifdef INTF_DATA
	INTF_DATA intf_data;
#endif
	//For xmit priv
	struct xmit_priv xmitpriv;
	//For recv priv
	struct recv_priv recvpriv;
	//For cmd priv
	struct cmd_priv cmdpriv;

	ATOMIC_T continual_io_error;
	struct _io_ops io_ops;
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
#define AT_CMD_wifi_connect 	"C0"
#define AT_CMD_wifi_disconnect 	"CD"
#define AT_CMD_wifi_on 			"P1"
#define AT_CMD_wifi_off 			"P0"
#define AT_CMD_wifi_ap 			"A0"
#define AT_CMD_wifi_scan 		"F0"
#define AT_CMD_wifi_get_rssi 	"CR"
#define AT_CMD_wifi_iwpriv 		"IW"
#define AT_CMD_wifi_ping 		"T0"
#define AT_CMD_wifi_info 		"I?"
#define AT_CMD_wifi_ioctl		"IO"
#define AT_CMD_wifi_linked		"LK"
#define AT_CMD_wifi_unlinked	"UL"

/*a global buf for the received ioctl message from Ameba*/
struct global_buf{
	_lock lock;
	unsigned char buf[2048];
	unsigned short buf_size;
};

#endif

