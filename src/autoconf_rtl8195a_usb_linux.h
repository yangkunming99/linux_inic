/******************************************************************************
 *
 * Copyright(c) 2007 - 2011 Realtek Corporation. All rights reserved.
 *                                        
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110, USA
 *
 *
 ******************************************************************************/
	 //***** temporarily flag *******
	 
#define AUTOCONF_INCLUDED
#define RTL871X_MODULE_NAME "RTL8195A"
#define DRV_NAME "rtl8195a"
	 
#define CONFIG_USE_VMALLOC
#define CONFIG_RTL8195A

#define CONFIG_USB_HCI

#define USB_WRITE_ADD 	0
#define USB_READ_ADD	1
#define CONFIG_USE_USB_BUFFER_ALLOC_RX
#define CONFIG_USE_USB_BUFFER_ALLOC_TX

	 
#define PLATFORM_LINUX

//#define GET_SYS_TIME
#define USE_RECV_TASKLET
#define ETH_ALEN		6 //ethernet address length
#define SUPPORT_SCAN_BUF 0


	 
 /* 
  * USB VENDOR REQ BUFFER ALLOCATION METHOD
  * if not set we'll use function local variable (stack memory)
  */
 //#define CONFIG_USB_VENDOR_REQ_BUFFER_DYNAMIC_ALLOCATE
#define CONFIG_USB_VENDOR_REQ_BUFFER_PREALLOC
	 
#define CONFIG_USB_VENDOR_REQ_MUTEX
#define CONFIG_VENDOR_REQ_RETRY
	 
 //#define CONFIG_USB_SUPPORT_ASYNC_VDN_REQ 

 /*******debug relative config*********/
#define DBG 1
#define CONFIG_DEBUG
//#define CONFIG_DEBUG_RTL871X

//#define CONFIG_FWDL
#define CONFIG_EMBEDDED_FWIMG	
#define CONFIG_FILE_FWIMG



