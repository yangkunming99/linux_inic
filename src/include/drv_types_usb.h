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
#ifndef __DRV_TYPES_USB_H__
#define __DRV_TYPES_USB_H__

#include "autoconf.h"

// USB Header Files
#ifdef PLATFORM_LINUX
	#include <linux/usb.h> 

#if defined(CONFIG_WOWLAN) || defined(CONFIG_AP_WOWLAN) || defined(CONFIG_PLATFORM_SPRD)
	#include <linux/mmc/host.h>
	#include <linux/mmc/card.h>
#endif

#ifdef CONFIG_PLATFORM_SPRD
	#include <linux/gpio.h>
	#include <custom_gpio.h>
#endif // CONFIG_PLATFORM_SPRD
#endif

#ifdef PLATFORM_OS_XP
#include <wdm.h>
#include <ntddsd.h>
#endif

#ifdef PLATFORM_OS_CE
#include <sdcardddk.h>
#endif

typedef struct usb_data
{
	u8 numInPipes;
	u8 numOutPipes;

	int InPipe[4];
	int OutPipe[4];

	uint recv_bulk_Pipe;
	uint send_bulk_Pipe;
	
	u8	usb_speed; // 1.1, 2.0 or 3.0
	u8	nr_endpoint;
	int	ep_num[6]; //endpoint number

	int	RegUsbSS;
	u8 NumInterfaces;
	u8 InterfaceNumber;
	_sema	usb_suspend_sema;
	_lock	devlock;
	_mutex 	io_mutex;

	_sema tx_urb_done;
	PURB rx_urb;
	PURB tx_urb;
	PURB intr_urb;

	PADAPTER padapter;
	
#ifdef PLATFORM_LINUX
struct usb_device	 *dev;
struct usb_interface *intf;
#endif

#ifdef PLATFORM_OS_XP

#endif

#ifdef PLATFORM_OS_CE

#endif
} USB_DATA, *PUSB_DATA;

#endif

