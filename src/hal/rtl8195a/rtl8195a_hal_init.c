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
#define MAX_REG_BOLCK_SIZE	196 

#ifdef CONFIG_SDIO_HCI 
	#include "sdio_ops_linux.h"
#elif defined(CONFIG_USB_HCI)
	#include "usb_ops_linux.h"
#endif

static void _fill_tx_des(
	IN	PVOID	tx_buff,
	u8	agg_num,
	u32 pktsize,
	u32 code
	){
	/* fill tx descriptor here*/

	PTXDESC_8195A	ptx_des;
	
	ptx_des = (PTXDESC_8195A)tx_buff;
	ptx_des->bus_agg_num = agg_num;
	ptx_des->offset = SIZE_TX_DESC_8195a;
	ptx_des->txpktsize = pktsize;
	ptx_des->type = 0x53;
}

static int
_BlockWrite(
	IN		PADAPTER		padapter,
	IN		PVOID		tx_buff,
	IN		PVOID		pdata,
	IN		u32			buffSize
	)
{
	int ret = _SUCCESS;
	u32		blockSize	= 128;
	u32		remainSize	= 0;
	u32		blockCount	= 0;
	u8		*bufferPtr	= (u8*)pdata;
	u32		i=0, offset=0;	
#ifdef CONFIG_USB_HCI
	blockSize = MAX_REG_BOLCK_SIZE;
#endif

	blockCount = buffSize / blockSize;
	remainSize = buffSize % blockSize;

	if (blockCount) {
			RT_TRACE(_module_hal_init_c_, _drv_notice_,
					("_BlockWrite: [P1] buffSize(%d) blockSize(%d) blockCount(%d) remainSize(%d)\n",
					buffSize, blockSize, blockCount, remainSize));
		}

	for (i = 0; i < blockCount; i++)
	{
		_fill_tx_des(tx_buff, 1, blockSize, MEM_WRITE);
		_rtw_memcpy(tx_buff + SIZE_TX_DESC_8195a, bufferPtr+i*blockSize, blockSize);
		
		ret = rtw_writeN(padapter, (FW_8195A_START_ADDRESS + i * blockSize), blockSize+SIZE_TX_DESC_8195a, tx_buff);

		if(ret == _FAIL)
			goto exit;
	}
	if (remainSize)
	{
		offset = blockCount * blockSize;

		_fill_tx_des(tx_buff, 1, remainSize, MEM_WRITE);
		_rtw_memcpy(tx_buff + SIZE_TX_DESC_8195a, bufferPtr+offset, remainSize);

			
		ret = rtw_writeN(padapter, (FW_8195A_START_ADDRESS + offset), remainSize, tx_buff);
			
		if(ret == _FAIL)
			goto exit;
	}

#if 0
	u32			blockSize_p1 = 4;	// (Default) Phase #1 : PCI muse use 4-byte write to download FW
	u32			blockSize_p2 = 8;	// Phase #2 : Use 8-byte, if Phase#1 use big size to write FW.
	u32			blockSize_p3 = 1;	// Phase #3 : Use 1-byte, the remnant of FW image.
	u32			blockCount_p1 = 0, blockCount_p2 = 0, blockCount_p3 = 0;
	u32			remainSize_p1 = 0, remainSize_p2 = 0;
	u8			*bufferPtr	= (u8*)buffer;
	u32			i=0, offset=0;

#ifdef CONFIG_USB_HCI
	blockSize_p1 = MAX_REG_BOLCK_SIZE;
#endif

	//3 Phase #1
	blockCount_p1 = buffSize / blockSize_p1;
	remainSize_p1 = buffSize % blockSize_p1;

	if (blockCount_p1) {
		RT_TRACE(_module_hal_init_c_, _drv_notice_,
				("_BlockWrite: [P1] buffSize(%d) blockSize_p1(%d) blockCount_p1(%d) remainSize_p1(%d)\n",
				buffSize, blockSize_p1, blockCount_p1, remainSize_p1));
	}

	for (i = 0; i < blockCount_p1; i++)
	{
#ifdef CONFIG_USB_HCI
		ret = rtw_writeN(padapter, (FW_8195A_START_ADDRESS + i * blockSize_p1), blockSize_p1, (bufferPtr + i * blockSize_p1));
#else
		ret = rtw_write32(padapter, (FW_8195A_START_ADDRESS + i * blockSize_p1), le32_to_cpu(*((u32*)(bufferPtr + i * blockSize_p1))));
#endif

		if(ret == _FAIL)
			goto exit;
	}


	//3 Phase #2
	if (remainSize_p1)
	{
		offset = blockCount_p1 * blockSize_p1;

		blockCount_p2 = remainSize_p1/blockSize_p2;
		remainSize_p2 = remainSize_p1%blockSize_p2;

		if (blockCount_p2) {
				RT_TRACE(_module_hal_init_c_, _drv_notice_,
						("_BlockWrite: [P2] buffSize_p2(%d) blockSize_p2(%d) blockCount_p2(%d) remainSize_p2(%d)\n",
						(buffSize-offset), blockSize_p2 ,blockCount_p2, remainSize_p2));
		}

#ifdef CONFIG_USB_HCI
		for (i = 0; i < blockCount_p2; i++) {
			ret = rtw_writeN(padapter, (FW_8195A_START_ADDRESS + offset + i*blockSize_p2), blockSize_p2, (bufferPtr + offset + i*blockSize_p2));
			
			if(ret == _FAIL)
				goto exit;
		}
#endif
	}

	//3 Phase #3
	if (remainSize_p2)
	{
		offset = (blockCount_p1 * blockSize_p1) + (blockCount_p2 * blockSize_p2);

		blockCount_p3 = remainSize_p2 / blockSize_p3;

		RT_TRACE(_module_hal_init_c_, _drv_notice_,
				("_BlockWrite: [P3] buffSize_p3(%d) blockSize_p3(%d) blockCount_p3(%d)\n",
				(buffSize-offset), blockSize_p3, blockCount_p3));

		for(i = 0 ; i < blockCount_p3 ; i++){
			ret =rtw_write8(padapter, (FW_8195A_START_ADDRESS + offset + i), *(bufferPtr + offset + i));
			
			if(ret == _FAIL)
				goto exit;
		}
	}

exit:
	return ret;
}

static int
_PageWrite(
	IN		PADAPTER	padapter,
	IN		u32			page,
	IN		PVOID		buffer,
	IN		u32			size
	)
{
	u8 ret = _SUCCESS;
	u8 * tx_buff;
		
	tx_buff = rtw_zmalloc(MAX_REG_BOLCK_SIZE + SIZE_TX_DESC_8195a);
	if(tx_buff == NULL)
		goto exit;
		
#if 0
	u8 value8;
	u8 u8Page = (u8) (page & 0x07) ;

	value8 = (rtw_read8(padapter, REG_MCUFWDL+2) & 0xF8) | u8Page ;
	rtw_write8(padapter, REG_MCUFWDL+2,value8);

#endif 

	ret = _BlockWrite(padapter,tx_buff,pdata,size);

exit:	
	if(tx_buff)
		rtw_mfree(tx_buff,MAX_REG_BOLCK_SIZE + SIZE_TX_DESC_8195a);
	return ret;
}


static int
_WriteFW(
	IN		PADAPTER		padapter,
	IN		PVOID			buffer,
	IN		u32			size
	)
{
	// Since we need dynamic decide method of dwonload fw, so we call this function to get chip version.
	// We can remove _ReadChipVersion from ReadpadapterInfo8192C later.
	int ret = _SUCCESS;
	u32 	pageNums,remainSize ;
	u32 	page, offset;
	u8		*bufferPtr = (u8*)buffer;

	pageNums = size / MAX_DLFW_PAGE_SIZE ;
	//RT_ASSERT((pageNums <= 4), ("Page numbers should not greater then 4 \n"));
	remainSize = size % MAX_DLFW_PAGE_SIZE;

	for (page = 0; page < pageNums; page++) {
		offset = page * MAX_DLFW_PAGE_SIZE;
		ret = _PageWrite(padapter, page, bufferPtr+offset, MAX_DLFW_PAGE_SIZE);
		
		if(ret == _FAIL)
			goto exit;
	}
	if (remainSize) {
		offset = pageNums * MAX_DLFW_PAGE_SIZE;
		page = pageNums;
		ret = _PageWrite(padapter, page, bufferPtr+offset, remainSize);
		
		if(ret == _FAIL)
			goto exit;

	}
	RT_TRACE(_module_hal_init_c_, _drv_info_, ("_WriteFW Done- for Normal chip.\n"));

exit:
	return ret;
}

extern u8 g_fwdl_chksum_fail;
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

void _MCUIO_Reset88E(PADAPTER padapter,u8 bReset)
{
/*
	u8 u1bTmp;

	if(bReset==_TRUE){
		// Reset MCU IO Wrapper- sugggest by SD1-Gimmy
		u1bTmp = rtw_read8(padapter, REG_RSV_CTRL+1);
		rtw_write8(padapter,REG_RSV_CTRL+1, (u1bTmp&(~BIT3)));
	}else{
		// Enable MCU IO Wrapper
		u1bTmp = rtw_read8(padapter, REG_RSV_CTRL+1);
		rtw_write8(padapter, REG_RSV_CTRL+1, u1bTmp|BIT3);
	}
*/
}

void _8051Reset88E(PADAPTER padapter)
{
/*
	u8 u1bTmp;
	
	_MCUIO_Reset88E(padapter,_TRUE);
	u1bTmp = rtw_read8(padapter, REG_SYS_FUNC_EN+1);
	rtw_write8(padapter, REG_SYS_FUNC_EN+1, u1bTmp&(~BIT2));
	_MCUIO_Reset88E(padapter,_FALSE);
	rtw_write8(padapter, REG_SYS_FUNC_EN+1, u1bTmp|(BIT2));
	
	DBG_871X("=====> _8051Reset88E(): 8051 reset success .\n");
*/
}

extern u8 g_fwdl_wintint_rdy_fail;
static s32 _FWFreeToGo(_adapter *adapter, u32 min_cnt, u32 timeout_ms)
{
	s32 ret = _FAIL;
	u32 start = rtw_get_current_time();

	u32	value32 = 0;
	u32 cnt = 0;
#if 0


	value32 = rtw_read32(adapter, REG_MCUFWDL);
	value32 |= MCUFWDL_RDY;
	value32 &= ~WINTINI_RDY;
	rtw_write32(adapter, REG_MCUFWDL, value32);

	_8051Reset88E(adapter);

	/*  polling for FW ready */
	do {
		cnt++;
		value32 = rtw_read32(adapter, REG_MCUFWDL);
		if (value32 & WINTINI_RDY || adapter->bSurpriseRemoved || adapter->bDriverStopped)
			break;
		rtw_yield_os();
	} while (rtw_get_passing_time_ms(start) < timeout_ms || cnt < min_cnt);

	if (!(value32 & WINTINI_RDY)) {
		goto exit;
	}

	if (g_fwdl_wintint_rdy_fail) {
		DBG_871X("%s: fwdl test case: wintint_rdy_fail\n", __FUNCTION__);
		g_fwdl_wintint_rdy_fail--;
		goto exit;
	}
#endif
	ret = _SUCCESS;

//exit:
	DBG_871X("%s: Polling FW ready %s! (%u, %dms), REG_MCUFWDL:0x%08x\n", __FUNCTION__
		, (ret==_SUCCESS)?"OK":"Fail", cnt, rtw_get_passing_time_ms(start), value32);
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
u8	FwBuffer8195a[FW_8195A_SIZE];
#endif //CONFIG_FILE_FWIMG


//
//	Description:
//		Download 8192C firmware code.
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
	
	u8			*pFirmwareBuf;
	u32			FirmwareLen;
#ifdef CONFIG_FILE_FWIMG
	u8 *fwfilepath;
#endif // CONFIG_FILE_FWIMG

	RT_TRACE(_module_hal_init_c_, _drv_info_, ("+%s\n", __FUNCTION__));
	pFirmware = (PRT_FIRMWARE_8195A)rtw_zmalloc(sizeof(RT_FIRMWARE_8195A));
	if(!pFirmware)
	{
		rtStatus = _FAIL;
		goto exit;
	}



//	RT_TRACE(_module_hal_init_c_, _drv_err_, ("rtl8723a_FirmwareDownload: %s\n", pFwImageFileName));

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
	padapter->FirmwareSubVersion = pFwHdr->Subversion;
	padapter->FirmwareSignature = le16_to_cpu(pFwHdr->Signature);

	DBG_871X("%s: fw_ver=%x fw_subver=%04x sig=0x%x, Month=%02x, Date=%02x, Hour=%02x, Minute=%02x\n",
		  __FUNCTION__, padapter->FirmwareVersion, padapter->FirmwareSubVersion, padapter->FirmwareSignature
		  ,pFwHdr->Month,pFwHdr->Date,pFwHdr->Hour,pFwHdr->Minute);
		
	if (IS_FW_HEADER_EXIST_95A(pFwHdr))
	{
		// Shift 32 bytes for FW header
		pFirmwareBuf = pFirmwareBuf + 32;
		FirmwareLen = FirmwareLen - 32;
	}
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
		/* reset FWDL chksum */
//		rtw_write8(padapter, REG_MCUFWDL, rtw_read8(padapter, REG_MCUFWDL)|FWDL_ChkSum_rpt);
		
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
	return rtStatus;
}

