#ifndef __RTL8195A_RECV_H__
#define __RTL8195A_RECV_H__
#include "drv_types.h"

#if defined(CONFIG_USB_HCI)

#ifdef PLATFORM_OS_CE
#define MAX_RECVBUF_SZ (8192+1024) // 8K+1k
#else
	#ifdef CONFIG_MINIMAL_MEMORY_USAGE
		#define MAX_RECVBUF_SZ (4000) // about 4K
	#else
		#ifdef CONFIG_PLATFORM_MSTAR
			#define MAX_RECVBUF_SZ (8192) // 8K
		#else
		#define MAX_RECVBUF_SZ (32768) // 32k
		#endif
		//#define MAX_RECVBUF_SZ (20480) //20K
		//#define MAX_RECVBUF_SZ (10240) //10K 
		//#define MAX_RECVBUF_SZ (16384) //  16k - 92E RX BUF :16K
		//#define MAX_RECVBUF_SZ (8192+1024) // 8K+1k		
	#endif
#endif

#elif defined(CONFIG_PCI_HCI)
//#ifndef CONFIG_MINIMAL_MEMORY_USAGE
//	#define MAX_RECVBUF_SZ (9100)
//#else
	#define MAX_RECVBUF_SZ (4000) // about 4K
//#endif


#elif defined(CONFIG_SDIO_HCI)

#define MAX_RECVBUF_SZ (10240)

#endif


s32 rtl8195as_init_recv_priv(PADAPTER padapter);
void rtl8195as_free_recv_priv(PADAPTER padapter);


s32 rtl8195au_init_recv_priv(PADAPTER padapter);
void rtl8195au_free_recv_priv(PADAPTER padapter);
void rtl8195au_init_recvbuf(PADAPTER padapter, struct recv_buf *precvbuf);

#endif
