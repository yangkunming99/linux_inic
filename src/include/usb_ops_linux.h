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
#ifndef __USB_OPS_LINUX_H__
#define __USB_OPS_LINUX_H__

#define VENDOR_CMD_MAX_DATA_LEN	254
#define FW_START_ADDRESS	0x1000

#define RTW_USB_CONTROL_MSG_TIMEOUT_TEST	10//ms
#define RTW_USB_CONTROL_MSG_TIMEOUT	500//ms

#define RECV_BULK_IN_ADDR		0x80//assign by drv,not real address 
#define RECV_INT_IN_ADDR		0x81//assign by drv,not real address 


#define RTW_USB_BULKOUT_TIMEOUT	5000//ms

unsigned int ffaddr2pipehdl(struct dvobj_priv *pdvobj, u32 addr);

void usb_read_mem(PADAPTER padapter, u32 addr, u32 cnt, u8 *rmem);
void usb_write_mem(PADAPTER padapter, u32 addr, u32 cnt, u8 *wmem);

void usb_read_port_cancel(PADAPTER padapter);

u32 usb_write_port(PADAPTER padapter, u32 addr, u32 cnt, u8 *wmem);
void usb_write_port_cancel(PADAPTER padapter);

int usbctrl_vendorreq(PADAPTER padapter, u8 request, u16 value, u16 index, void *pdata, u16 len, u8 requesttype);
#endif
