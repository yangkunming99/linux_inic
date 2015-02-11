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
#define _HAL_INIT_C_

#include <drv_types.h>
#include <rtw_debug.h>
#include <rtl8195a_hal.h>
#include <8195_com_reg.h>
#include <rtw_io.h>
#include <8195_desc.h>


#ifdef CONFIG_SDIO_HCI 
	#include "sdio_ops_linux.h"
	#include "8195_sdio_reg.h"
#elif defined(CONFIG_USB_HCI)
	#include "usb_ops_linux.h"
#endif

#ifdef CONFIG_SDIO_HCI
static int
_FwPageWrite(
	IN		PADAPTER	padapter,
	IN		u32			offset,
	IN		PVOID		pdata,
	IN		u32			size
	)
{
	u8 ret = _FAIL;
	u8 * tx_buff;
	TX_DESC_MW	*ptx_des;
	u32 buf_sz;
	//    RX_DESC_MW rxdesc;
	u32 i;
	u16 old_hcpwm2;
	u16 new_hcpwm2;

	old_hcpwm2 = rtw_read16(padapter, SDIO_REG_HCPWM2); 
        
	buf_sz = (((size + SIZE_TX_DESC_8195a - 1) >> 9)+1) << 9;
	tx_buff = rtw_zmalloc(buf_sz);
	if(tx_buff == NULL)
		goto exit;

	ptx_des = (TX_DESC_MW *)tx_buff;
	ptx_des->txpktsize = cpu_to_le16(size);
	ptx_des->offset = SIZE_TX_DESC_8195a;
	ptx_des->bus_agg_num = 1;
	ptx_des->type = TX_MEM_WRITE;
	ptx_des->reply = 0;
	ptx_des->start_addr = cpu_to_le32(offset);
	ptx_des->write_len = cpu_to_le16(size);

	_rtw_memcpy(tx_buff + SIZE_TX_DESC_8195a, pdata, size);
	ret = rtw_write_port(padapter, WLAN_TX_FIFO_DEVICE_ID, (size + SIZE_TX_DESC_8195a), tx_buff);
	rtw_msleep_os(50); // wait memory write done

	for (i=0;i<100;i++) {
		new_hcpwm2 = rtw_read16(padapter, SDIO_REG_HCPWM2); 
		if ((new_hcpwm2 & BIT15) != (old_hcpwm2 & BIT15)) {
			// toggle bit(15)  is changed, it means the 8195a update its register value
			old_hcpwm2 = new_hcpwm2;
			if (new_hcpwm2 & SDIO_MEM_WR_DONE) {
				// 8195a memory write done
				ret = _SUCCESS;
				break;
			}
			rtw_msleep_os(10);
		}
		else {
			rtw_msleep_os(10);
		}        
	}

#if 0
    // wait and check the memory write reply from NIC device
    if (ptx_des->reply) {
        for (i=0;i<100;i++) {
            ret = rtw_read_port(padapter, WLAN_RX_FIFO_DEVICE_ID, sizeof(RX_DESC_MW), (u8*)&rxdesc);
            if (ret == _SUCCESS) {
                if ((rxdesc.type == MEM_WRITE_RSP) && 
                    (le32_to_cpu(rxdesc.start_addr) == offset)) {
                    DBG_871X("%s: Got the Memory Write Reply %x\n", __FUNCTION__, rxdesc.result);
                    if (rxdesc.result != 0) {
                        ret = _FAIL;
                    }
                    break;
                }
                else { 
                    // TODO: handle this packet, it should be a memory write reply packet , but it's not.
                }
            }
            else {
                DBG_871X("%s: Read port error\n", __FUNCTION__);
                msleep(10);
            }
        }

        if(i==100) {
            ret = _FAIL;
        }
    }
#endif

exit:	
	if(tx_buff)
		rtw_mfree(tx_buff, buf_sz);
	return ret;
}
#endif
#ifdef CONFIG_USB_HCI
static int
_FwPageWrite(
	IN		PADAPTER	padapter,
	IN		u32			offset,
	IN		PVOID		pdata,
	IN		u32			size
	)
{
	u8 ret = _FAIL;
//todo: usb write fw to ameba
	return ret;
}
#endif
static int
_WriteFW(
	IN		PADAPTER		padapter,
	IN		PVOID			buffer,
	IN		u32			size
	)
{
	int ret = _SUCCESS;
	u32 	pageNums,remainSize ;
	u32 	page, offset;
	u8	*bufferPtr = (u8*)buffer;
	u32  fw_startaddr;

	pageNums = size / MAX_DLFW_PAGE_SIZE ;
	remainSize = size % MAX_DLFW_PAGE_SIZE;

	fw_startaddr = padapter->FirmwareStartAddr;

	for (page = 0; page < pageNums; page++) {
		offset = page * MAX_DLFW_PAGE_SIZE;
		
		DBG_871X("%s: Write Mem: StartAddr=0x%08x Len=%d\n", __FUNCTION__
		    ,(fw_startaddr+offset), MAX_DLFW_PAGE_SIZE);

		ret = _FwPageWrite(padapter, (fw_startaddr+offset), bufferPtr+offset, MAX_DLFW_PAGE_SIZE);
		if(ret == _FAIL) {
			DBG_871X("%s: Error!", __FUNCTION__);
			goto exit;
		}
	}
	
	if (remainSize) {
		offset = pageNums * MAX_DLFW_PAGE_SIZE;
		page = pageNums;

	        DBG_871X("%s: Write Mem (Remain): StartAddr=0x%08x Len=%d\n", __FUNCTION__
	            ,(fw_startaddr+offset), remainSize);
			
		ret = _FwPageWrite(padapter, (fw_startaddr+offset), bufferPtr+offset, remainSize);
		
		if(ret == _FAIL)
			goto exit;

	}
	RT_TRACE(_module_hal_init_c_, _drv_info_, ("_WriteFW Done- for Normal chip.\n"));

exit:
	return ret;
}
static s32 check_firmware_status(_adapter *padapter){
	u8 fw_ready;
	s32 ret = _FAIL;
#ifdef CONFIG_SDIO_HCI
	fw_ready = rtw_read8(padapter, SDIO_REG_CPU_IND); 
	DBG_871X("%s: cpu_ind @ 0x%02x\n", __FUNCTION__, fw_ready);
	if (fw_ready&SDIO_SYSTEM_TRX_RDY_IND) {
		ret = _SUCCESS;
		DBG_871X("%s: firmware is already running!\n", __FUNCTION__);
	}
#endif
	return ret;
}
extern u8 g_fwdl_chksum_fail;
extern u8 g_fwdl_wintint_rdy_fail;
static s32 polling_fwdl_chksum(_adapter *adapter, u32 min_cnt, u32 timeout_ms)
{
	s32 ret = _FAIL;
	u32 value32 = 0;
	u32 start = rtw_get_current_time();
	u32 cnt = 0;
#if 0

	/* polling CheckSum report */
	do {
		cnt++;
		value32 = rtw_read32(adapter, REG_MCUFWDL);
		if (value32 & FWDL_ChkSum_rpt || adapter->bSurpriseRemoved || adapter->bDriverStopped)
			break;
		rtw_yield_os();
	} while (rtw_get_passing_time_ms(start) < timeout_ms || cnt < min_cnt);

	if (!(value32 & FWDL_ChkSum_rpt)) {
		goto exit;
	}

	if (g_fwdl_chksum_fail) {
		DBG_871X("%s: fwdl test case: fwdl_chksum_fail\n", __FUNCTION__);
		g_fwdl_chksum_fail--;
		goto exit;
	}
#endif
	ret = _SUCCESS;

//exit:
	DBG_871X("%s: Checksum report %s! (%u, %dms), REG_MCUFWDL:0x%08x\n", __FUNCTION__
	, (ret==_SUCCESS)?"OK":"Fail", cnt, rtw_get_passing_time_ms(start), value32);

	return ret;
}
static s32 _FWFreeToGo(_adapter *adapter, u32 min_cnt, u32 timeout_ms)
{
	s32 ret = _FAIL;
	//u32 start = rtw_get_current_time();
	//u8 fw_ready;
	u32 i;

	TX_DESC_JS	tx_des;
	
	tx_des.txpktsize = 0;
	tx_des.offset = SIZE_TX_DESC_8195a;
	tx_des.bus_agg_num = 1;
	tx_des.type = TX_FM_FREETOGO;
	tx_des.start_fun = cpu_to_le32(adapter->FirmwareEntryFun);

	DBG_871X("%s: Jump to Entry Func @ 0x%08x\n", __FUNCTION__
	    ,adapter->FirmwareEntryFun);
	ret = rtw_write_port(adapter, WLAN_TX_FIFO_DEVICE_ID, SIZE_TX_DESC_8195a, (u8 *)&tx_des);
#if 0
    // wait for the firmware going to re-load indication
    rtw_msleep_os(80);
    for (i=0;i<100;i++) {
        fw_ready = rtw_read8(adapter, SDIO_REG_CPU_IND); 
        if ((fw_ready & BIT0) == 0) {
            // it means the boot firmware aware the jump command
            break;
        }
        rtw_msleep_os(10);
    }

    // wait for the new downloaded firmware started
    rtw_msleep_os(500);
    for (i=0;i<100;i++) {
        fw_ready = rtw_read8(adapter, SDIO_REG_CPU_IND); 
        if (fw_ready & (BIT0)) {
            break;
        }
        rtw_msleep_os(10);
    }
    
    if (i==100) {
        DBG_871X("%s: Wait Firmware Start Timeout!!\n", __FUNCTION__);
	    ret = _FAIL;
    }
    else {
	ret = _SUCCESS;
    }
#endif
	// TODO: Pooling firmware ready here
	 for (i=0;i<100;i++) {
		if(check_firmware_status(adapter) == _SUCCESS)
			break;
		rtw_msleep_os(10);
	}
	if (i==100) {
		DBG_871X("%s: Wait Firmware Start Timeout!!\n", __FUNCTION__);
		ret = _FAIL;
	}
	else {
		ret = _SUCCESS;
	}
	return ret;
}

static VOID
_FWDownloadEnable_8195A(
	IN	PADAPTER		padapter,
	IN	BOOLEAN			enable
	)
{
#if 0
	u8	tmp;
	if(enable)
	{

		// MCU firmware download enable.
		tmp = rtw_read8(padapter, REG_MCUFWDL);
		rtw_write8(padapter, REG_MCUFWDL, tmp|0x01);

		// 8051 reset
		tmp = rtw_read8(padapter, REG_MCUFWDL+2);
		rtw_write8(padapter, REG_MCUFWDL+2, tmp&0xf7);

	}
	else
	{		
		
		// MCU firmware download disable.
		tmp = rtw_read8(padapter, REG_MCUFWDL);
		rtw_write8(padapter, REG_MCUFWDL, tmp&0xfe);

		// Reserved for fw extension.
		rtw_write8(padapter, REG_MCUFWDL+1, 0x00);
	}
#endif
}

#ifdef CONFIG_FILE_FWIMG
extern char *rtw_fw_file_path;
extern char *rtw_fw_wow_file_path;
#endif //CONFIG_FILE_FWIMG


//
//	Description:
//		Download 8195a firmware code.
//
//
s32 rtl8195a_FirmwareDownload(PADAPTER padapter, BOOLEAN  bUsedWoWLANFw)
{
	s32	rtStatus = _SUCCESS;
	u8 write_fw = 0;
	u32 fwdl_start_time;
	
#ifdef CONFIG_WOWLAN
	u8			*FwImageWoWLAN;
	u32			FwImageWoWLANLen;
#endif

	PRT_FIRMWARE_8195A	pFirmware = NULL;
	PRT_8195A_FIRMWARE_HDR		pFwHdr = NULL;
    u8 *FwBuffer8195a=NULL;
	
	u8			*pFirmwareBuf;
	u32			FirmwareLen;
#ifdef CONFIG_FILE_FWIMG
	u8 *fwfilepath;
#endif // CONFIG_FILE_FWIMG

	RT_TRACE(_module_hal_init_c_, _drv_info_, ("+%s\n", __FUNCTION__));

	//check if firmware is already running
	if(check_firmware_status(padapter) == _SUCCESS)
		goto exit;

	pFirmware = (PRT_FIRMWARE_8195A)rtw_zmalloc(sizeof(RT_FIRMWARE_8195A));
	if(!pFirmware)
	{
		rtStatus = _FAIL;
		goto exit;
	}

	FwBuffer8195a = rtw_zmalloc(FW_8195A_SIZE);
	if(!FwBuffer8195a)
	{
		rtStatus = _FAIL;
		goto exit;
	}


#ifdef CONFIG_FILE_FWIMG
#ifdef CONFIG_WOWLAN
		if (bUsedWoWLANFw)
		{
			fwfilepath = rtw_fw_wow_file_path;
		}
		else
#endif // CONFIG_WOWLAN
		{
			fwfilepath = rtw_fw_file_path;
		}
#endif // CONFIG_FILE_FWIMG


#ifdef CONFIG_FILE_FWIMG
	if(rtw_is_file_readable(fwfilepath) == _TRUE)
	{
		DBG_871X("%s accquire FW from file:%s\n", __FUNCTION__, fwfilepath);
		pFirmware->eFWSource = FW_SOURCE_IMG_FILE;
	}
	else
#endif //CONFIG_FILE_FWIMG
	{
		pFirmware->eFWSource = FW_SOURCE_HEADER_FILE;
	}

	switch(pFirmware->eFWSource)
	{
		case FW_SOURCE_IMG_FILE:
			#ifdef CONFIG_FILE_FWIMG
			rtStatus = rtw_retrive_from_file(fwfilepath, FwBuffer8195a, FW_8195A_SIZE);
			pFirmware->ulFwLength = rtStatus>=0?rtStatus:0;
			pFirmware->szFwBuffer = FwBuffer8195a;
			#endif //CONFIG_FILE_FWIMG
			break;
		case FW_SOURCE_HEADER_FILE:
			
			break;
	}


	if (pFirmware->ulFwLength > FW_8195A_SIZE) {
		rtStatus = _FAIL;
		DBG_871X_LEVEL(_drv_emerg_, "Firmware size:%u exceed %u\n", pFirmware->ulFwLength, FW_8195A_SIZE);
		goto exit;
	}
	
	pFirmwareBuf = pFirmware->szFwBuffer;
	FirmwareLen = pFirmware->ulFwLength;

	// To Check Fw header. Added by tynli. 2009.12.04.
	pFwHdr = (PRT_8195A_FIRMWARE_HDR)pFirmwareBuf;

	padapter->FirmwareVersion =  le16_to_cpu(pFwHdr->Version);
	padapter->FirmwareSubVersion = le16_to_cpu(pFwHdr->Subversion);
	//	padapter->FirmwareSignature = le16_to_cpu(pFwHdr->Signature);

	padapter->FirmwareEntryFun = le32_to_cpu(pFwHdr->StartFunc);
	padapter->FirmwareStartAddr = le32_to_cpu(pFwHdr->StartAddr);
	padapter->FirmwareSize = le32_to_cpu(pFwHdr->FwSize);

	DBG_871X("%s: fw_ver=%04x fw_subver=%04x sig=%s\n",
		  __FUNCTION__, padapter->FirmwareVersion, padapter->FirmwareSubVersion, pFwHdr->Signature);

	DBG_871X("%s: fw_startaddr=0x%08x fw_size=%d fw_entry=0x%08x\n",
	      __FUNCTION__, padapter->FirmwareStartAddr, padapter->FirmwareSize, padapter->FirmwareEntryFun);

#if 0		
	if (IS_FW_HEADER_EXIST_95A(pFwHdr))
	{
		// Shift 32 bytes for FW header
		pFirmwareBuf = pFirmwareBuf + 32;
		FirmwareLen = FirmwareLen - 32;
	}
#else    
    // skip first 16 bytes
    pFirmwareBuf = pFirmwareBuf + 16;
    FirmwareLen = FirmwareLen - 16;
#endif    
/*
	// Suggested by Filen. If 8051 is running in RAM code, driver should inform Fw to reset by itself,
	// or it will cause download Fw fail. 2010.02.01. by tynli.
	if (rtw_read8(padapter, REG_MCUFWDL) & RAM_DL_SEL) //8051 RAM code
	{
		rtw_write8(padapter, REG_MCUFWDL, 0x00);
		_8051Reset88E(padapter);		
	}
*/
	_FWDownloadEnable_8195A(padapter, _TRUE);
	fwdl_start_time = rtw_get_current_time();
	while(!padapter->bDriverStopped && !padapter->bSurpriseRemoved
			&& (write_fw++ < 3 || rtw_get_passing_time_ms(fwdl_start_time) < 500))
	{
		rtStatus = _WriteFW(padapter, pFirmwareBuf, FirmwareLen);
		if (rtStatus != _SUCCESS)
			continue;

		rtStatus = polling_fwdl_chksum(padapter, 5, 50);
		if (rtStatus == _SUCCESS)
			break;
	}
	_FWDownloadEnable_8195A(padapter, _FALSE);
	if(_SUCCESS != rtStatus)
		goto fwdl_stat;

	rtStatus = _FWFreeToGo(padapter, 10, 200);
	if (_SUCCESS != rtStatus)
		goto fwdl_stat;

fwdl_stat:
	DBG_871X("FWDL %s. write_fw:%u, %dms\n"
		, (rtStatus == _SUCCESS)?"success":"fail"
		, write_fw
		, rtw_get_passing_time_ms(fwdl_start_time)
	);

exit:
	if (pFirmware)
		rtw_mfree((u8*)pFirmware, sizeof(RT_FIRMWARE_8195A));

	if (FwBuffer8195a)
		rtw_mfree((u8*)FwBuffer8195a, FW_8195A_SIZE);

	return rtStatus;
}

