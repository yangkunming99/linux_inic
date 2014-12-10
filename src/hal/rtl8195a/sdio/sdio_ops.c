#include "rtw_debug.h"
#include "sdio_ops.h"
#include "8195_sdio_reg.h"
#include "sdio_ops_linux.h"
//
// Description:
//	The following mapping is for SDIO host local register space.
//
// Creadted by Roger, 2011.01.31.
//
static void HalSdioGetCmdAddr8195ASdio(
	IN	PADAPTER	padapter,
	IN 	u8				DeviceID,
	IN	u32				Addr,
	OUT	u32*			pCmdAddr
	)
{
	switch (DeviceID)
	{
		case SDIO_LOCAL_DEVICE_ID:
			*pCmdAddr = ((SDIO_LOCAL_DEVICE_ID << 13) | (Addr & SDIO_LOCAL_MSK));
			break;

		case WLAN_TX_FIFO_DEVICE_ID:
			*pCmdAddr = ((WLAN_TX_FIFO_DEVICE_ID << 13) | (Addr & WLAN_TX_FIFO_MSK));
			break;

		case WLAN_RX_FIFO_DEVICE_ID:
			*pCmdAddr = ((WLAN_RX_FIFO_DEVICE_ID << 13) | (Addr & WLAN_RX_FIFO_MSK));
			break;

		default:
			break;
	}
}
static u8 get_deviceid(u32 addr)
{
	u8 deviceId;
	u16 pseudoId;


	pseudoId = (u16)(addr >> 16);
	switch (pseudoId)
	{
		case 0x1025:
			deviceId = SDIO_LOCAL_DEVICE_ID;
			break;

		case 0x1031:
			deviceId = WLAN_TX_FIFO_DEVICE_ID;
			break;

		case 0x1034:
			deviceId = WLAN_RX_FIFO_DEVICE_ID;
			break;

		default:
			deviceId = SDIO_LOCAL_DEVICE_ID;
			break;
	}

	return deviceId;
}


/*
 * Ref:
 *	HalSdioGetCmdAddr8723ASdio()
 */
static u32 _cvrt2ftaddr(const u32 addr, u8 *pdeviceId, u16 *poffset)
{
	u8 deviceId;
	u16 offset;
	u32 ftaddr;


	deviceId = get_deviceid(addr);
	offset = 0;

	switch (deviceId)
	{
		case SDIO_LOCAL_DEVICE_ID:
			offset = addr & SDIO_LOCAL_MSK;
			break;

		case WLAN_TX_FIFO_DEVICE_ID:
			offset = addr & WLAN_TX_FIFO_MSK;
			break;

		case WLAN_RX_FIFO_DEVICE_ID:
			offset = addr & WLAN_RX_FIFO_MSK;
			break;

		default:
			deviceId = SDIO_LOCAL_DEVICE_ID;
			offset = addr & SDIO_LOCAL_MSK;
			break;
	}
	ftaddr = (deviceId << 13) | offset;

	if (pdeviceId) *pdeviceId = deviceId;
	if (poffset) *poffset = offset;

	return ftaddr;
}

u8 sdio_read8(PADAPTER padapter, u32 addr)
{
	u32 ftaddr;
	u8 val;

_func_enter_;

	ftaddr = _cvrt2ftaddr(addr, NULL, NULL);
	sd_cmd52_read(padapter, ftaddr, 1, (u8*)&val);	

_func_exit_;

	return val;
}

u8 sdio_f0_read8(PADAPTER padapter, u32 addr)
{
	u8 val;

_func_enter_;
	val = sd_f0_read8(padapter, addr, NULL);

_func_exit_;

	return val;
}

u16 sdio_read16(PADAPTER padapter, u32 addr)
{
	u32 ftaddr;
	u16 val;

_func_enter_;

	ftaddr = _cvrt2ftaddr(addr, NULL, NULL);
	sd_cmd52_read(padapter, ftaddr, 2, (u8*)&val);	

	val = le16_to_cpu(val);

_func_exit_;

	return val;
}

u32 sdio_read32(PADAPTER padapter, u32 addr)
{
	u32 ftaddr;
	u32 val;

_func_enter_;
	ftaddr = _cvrt2ftaddr(addr, NULL, NULL);
	sd_cmd52_read(padapter, ftaddr, 4, (u8*)&val);	
	val = le32_to_cpu(val);
_func_exit_;

	return val;
}

s32 sdio_readN(PADAPTER padapter, u32 addr, u32 cnt, u8* pbuf)
{

	u8 deviceId;
	u16 offset;
	u32 ftaddr;
	u8 shift;
	s32 err;

_func_enter_;

	err = 0;

	ftaddr = _cvrt2ftaddr(addr, &deviceId, &offset);

	// 4 bytes alignment
	shift = ftaddr & 0x3;
	if (shift == 0) {
		err = sd_read(padapter, ftaddr, cnt, pbuf);
	} else {
		u8 *ptmpbuf;
		u32 n;

		ftaddr &= ~(u16)0x3;
		n = cnt + shift;
		ptmpbuf = rtw_malloc(n);
		if (NULL == ptmpbuf) return -1;
		err = sd_read(padapter, ftaddr, n, ptmpbuf);
		if (!err)
			_rtw_memcpy(pbuf, ptmpbuf+shift, cnt);
		rtw_mfree(ptmpbuf, n);
	}

_func_exit_;

	return err;
}

void sdio_read_mem(PADAPTER padapter, u32 addr, u32 cnt, u8 *rmem)
{
	s32 err;

_func_enter_;

	err = sdio_readN(padapter, addr, cnt, rmem);

_func_exit_;
}

/*
 * Description:
 *	Read from RX FIFO
 *	Round read size to block size,
 *	and make sure data transfer will be done in one command.
 *
 * Parameters:
 *	func		a pointer of sdio func
 *	addr		port ID
 *	cnt			size to read
 *	rmem		address to put data
 *
 * Return:
 *	_SUCCESS(1)		Success
 *	_FAIL(0)		Fail
 */

u32 sdio_read_port(
//	struct sdio_func *func,
	PADAPTER padapter,
	u32 addr,
	u32 cnt,
	u8 *mem)
{
	s32 err;
	PSDIO_DATA psdio= &padapter->dvobj->intf_data;
//	printk("%s(): addr is %d\n", __func__, addr);
//	printk("%s(): SDIORxFIFOCnt is %d\n", __func__, padapter->SdioRxFIFOCnt);
	HalSdioGetCmdAddr8195ASdio(padapter, addr, psdio->SdioRxFIFOCnt++, &addr);

//	printk("%s(): Get Cmd Addr is 0x%x\n", __func__, addr);


	cnt = _RND4(cnt);
	if (cnt > psdio->func->cur_blksize)
		cnt = _RND(cnt, psdio->func->cur_blksize);
	
//	cnt = sdio_align_size(cnt);
 	//printk("%s(): cnt is %d\n", __func__, cnt);
	err = _sd_read(psdio->func, addr, cnt, mem);
	//err = sd_read(padapter, addr, cnt, mem);
	
	if (err) return _FAIL;
	return _SUCCESS;
}


s32 sdio_write8(PADAPTER padapter, u32 addr, u8 val)
{
	u32 ftaddr;
	s32 err;

_func_enter_;
	ftaddr = _cvrt2ftaddr(addr, NULL, NULL);
	sd_write8(padapter, ftaddr, val, &err);

_func_exit_;

	return err;
}

s32 sdio_write16(PADAPTER padapter, u32 addr, u16 val)
{
	u32 ftaddr;
//	u8 shift;
	s32 err;

_func_enter_;

	ftaddr = _cvrt2ftaddr(addr, NULL, NULL);
	val = cpu_to_le16(val);
	err = sd_cmd52_write(padapter, ftaddr, 2, (u8*)&val);

_func_exit_;

	return err;
}

s32 sdio_write32(PADAPTER padapter, u32 addr, u32 val)
{
	u8 deviceId;
	u16 offset;
	u32 ftaddr;
	u8 shift;
	s32 err;

_func_enter_;

	err = 0;

	ftaddr = _cvrt2ftaddr(addr, &deviceId, &offset);

	// 4 bytes alignment
	shift = ftaddr & 0x3;
#if 1
	if (shift == 0)
	{
		sd_write32(padapter, ftaddr, val, &err);
	}
	else
	{
		val = cpu_to_le32(val);
		err = sd_cmd52_write(padapter, ftaddr, 4, (u8*)&val);
	}
#else
	if (shift == 0) {	
		sd_write32(pintfhdl, ftaddr, val, &err);
	} else {
		u8 *ptmpbuf;

		ptmpbuf = (u8*)rtw_malloc(8);
		if (NULL == ptmpbuf) return (-1);

		ftaddr &= ~(u16)0x3;
		err = sd_read(pintfhdl, ftaddr, 8, ptmpbuf);
		if (err) {
			rtw_mfree(ptmpbuf, 8);
			return err;
		}
		val = cpu_to_le32(val);
		_rtw_memcpy(ptmpbuf+shift, &val, 4);
		err = sd_write(pintfhdl, ftaddr, 8, ptmpbuf);
		
		rtw_mfree(ptmpbuf, 8);
	}
#endif	

_func_exit_;

	return err;
}

s32 sdio_writeN(PADAPTER padapter, u32 addr, u32 cnt, u8* pbuf)
{

	u8 deviceId;
	u16 offset;
	u32 ftaddr;
	u8 shift;
	s32 err;
_func_enter_;

	err = 0;

	ftaddr = _cvrt2ftaddr(addr, &deviceId, &offset);

	shift = ftaddr & 0x3;
	if (shift == 0) {
		err = sd_write(padapter, ftaddr, cnt, pbuf);
	} else {
		u8 *ptmpbuf;
		u32 n;

		ftaddr &= ~(u16)0x3;
		n = cnt + shift;
		ptmpbuf = rtw_malloc(n);
		if (NULL == ptmpbuf) return -1;
		err = sd_read(padapter, ftaddr, 4, ptmpbuf);
		if (err) {
			rtw_mfree(ptmpbuf, n);
			return err;
		}
		_rtw_memcpy(ptmpbuf+shift, pbuf, cnt);
		err = sd_write(padapter, ftaddr, n, ptmpbuf);
		rtw_mfree(ptmpbuf, n);
	}

_func_exit_;

	return err;
}

void sdio_write_mem(PADAPTER padapter, u32 addr, u32 cnt, u8 *wmem)
{
_func_enter_;

	sdio_writeN(padapter, addr, cnt, wmem);

_func_exit_;
}

/*
 * Description:
 *	Write to TX FIFO
 *	Align write size block size,
 *	and make sure data could be written in one command.
 *
 * Parameters:
 *	pintfhdl	a pointer of intf_hdl
 *	addr		port ID
 *	cnt			size to write
 *	wmem		data pointer to write
 *
 * Return:
 *	_SUCCESS(1)		Success
 *	_FAIL(0)		Fail
 */
u32 sdio_write_port(
	PADAPTER padapter,
	u32 addr,
	u32 cnt,
	u8 *mem)
{

	s32 err;
	PSDIO_DATA psdio= &padapter->dvobj->intf_data;
	cnt = _RND4(cnt);

	HalSdioGetCmdAddr8195ASdio(padapter, addr, cnt >> 2, &addr);
	
	if (cnt > psdio->func->cur_blksize)
		cnt = _RND(cnt, psdio->func->cur_blksize);

	err = sd_write(padapter, addr, cnt, mem);	
	if (err)
	{
		printk("%s, error=%d\n", __func__, err);
		return _FAIL;
	}
	return _SUCCESS;
}

/*
 * Todo: align address to 4 bytes.
 */
s32 _sdio_local_read(
	PADAPTER padapter,
	u32		addr,
	u32		cnt,
	u8		*pbuf)
{
	s32 err;
	u8 *ptmpbuf;
	u32 n;
	PSDIO_DATA psdio= &padapter->dvobj->intf_data;
	HalSdioGetCmdAddr8195ASdio(padapter, SDIO_LOCAL_DEVICE_ID, addr, &addr);

#if 0
		{
			err = sd_cmd52_read(pfunc, addr, cnt, pbuf);
			return err;
		}
#endif	
       n = RND4(cnt);
	ptmpbuf = (u8 *)rtw_malloc(n);
	if(!ptmpbuf)
		return (-1);
	
	err = _sd_read(psdio->func, addr, n, ptmpbuf);
	
	if (!err)
		_rtw_memcpy(pbuf, ptmpbuf, cnt);

	if(ptmpbuf)
		rtw_mfree(ptmpbuf, n);	

	return err;
}


/*
 * Todo: align address to 4 bytes.
 */
s32 sdio_local_read(
	PADAPTER	padapter,
	u32		addr,
	u32		cnt,
	u8		*pbuf)
{

	s32 err;
	u8 *ptmpbuf;
	u32 n;
	HalSdioGetCmdAddr8195ASdio(padapter, SDIO_LOCAL_DEVICE_ID, addr, &addr);

        n = RND4(cnt);
	ptmpbuf = (u8*)rtw_malloc(n);
	if(!ptmpbuf)
		return (-1);

	err = sd_read(padapter, addr, n, ptmpbuf);
	if (!err)
		_rtw_memcpy(pbuf, ptmpbuf, cnt);

	if(ptmpbuf)
		rtw_mfree(ptmpbuf, n);	

	return err;
}


/*
 * Todo: align address to 4 bytes.
 */
s32 _sdio_local_write(
	PADAPTER	padapter,
	u32		addr,
	u32		cnt,
	u8		*pbuf)
{

	s32 err;
	u8 *ptmpbuf;
	PSDIO_DATA psdio= &padapter->dvobj->intf_data;
	if(addr & 0x3)
		DBG_871X("%s, address must be 4 bytes alignment\n", __FUNCTION__);

	if(cnt  & 0x3)
		DBG_871X("%s, size must be the multiple of 4 \n", __FUNCTION__);
	
	HalSdioGetCmdAddr8195ASdio(padapter, SDIO_LOCAL_DEVICE_ID, addr, &addr);

        ptmpbuf = (u8*)rtw_malloc(cnt);
	if(!ptmpbuf)
		return (-1);

	_rtw_memcpy(ptmpbuf, pbuf, cnt);

	err = _sd_write(psdio->func, addr, cnt, ptmpbuf);
	
	if (ptmpbuf)
		rtw_mfree(ptmpbuf, cnt);

	return err;
}


/*
 * Todo: align address to 4 bytes.
 */
s32 sdio_local_write(
//	PADAPTER	padapter,
	PADAPTER padapter,
	u32		addr,
	u32		cnt,
	u8		*pbuf)
{
//		struct intf_hdl * pintfhdl;
//		u8 bMacPwrCtrlOn;
	s32 err;
	u8 *ptmpbuf;
	if(addr & 0x3)
		printk("%s, address must be 4 bytes alignment\n", __FUNCTION__);

	if(cnt  & 0x3)
		printk("%s, size must be the multiple of 4 \n", __FUNCTION__);

//	pintfhdl=&padapter->iopriv.intf;
	
	HalSdioGetCmdAddr8195ASdio(padapter, SDIO_LOCAL_DEVICE_ID, addr, &addr);

//		rtw_hal_get_hwreg(padapter, HW_VAR_APFM_ON_MAC, &bMacPwrCtrlOn);
//		if ((_FALSE == bMacPwrCtrlOn)
//	#ifdef CONFIG_LPS_LCLK
//	//		|| (_TRUE == adapter_to_pwrctl(padapter)->bFwCurrentInPSMode)
//	#endif
//			)
//		{
//			err = _sd_cmd52_write(pintfhdl, addr, cnt, pbuf);
//			return err;
//		}

        ptmpbuf = (u8*)rtw_malloc(cnt);
	if(!ptmpbuf)
		return (-1);

	_rtw_memcpy(ptmpbuf, pbuf, cnt);

	err = sd_write(padapter, addr, cnt, ptmpbuf);
	
	if (ptmpbuf)
		rtw_mfree(ptmpbuf, cnt);

	return err;
}

static void sd_rxhandler(PADAPTER padapter, struct recv_buf *precvbuf)
{
	struct recv_priv *precvpriv = &padapter->recvpriv;
	_queue *ppending_queue;

	ppending_queue = &precvpriv->recv_buf_pending_queue;

	//3 1. enqueue recvbuf
	rtw_enqueue_recvbuf(precvbuf, ppending_queue);	
	//3 2. schedule tasklet
	tasklet_schedule(&precvpriv->recv_tasklet);
}

static struct recv_buf* sd_recv_rxfifo(PADAPTER padapter, u32 size)
{
	u32 readsize, allocsize, ret;
	u8 *preadbuf;
	struct sk_buff  *ppkt;
	struct recv_priv *precvpriv;
	struct recv_buf	*precvbuf;
	PSDIO_DATA psdio= &padapter->dvobj->intf_data;

	readsize = size;

	//3 1. alloc skb
	// align to block size
	allocsize = _RND(readsize, psdio->func->cur_blksize);

	ppkt = _rtw_skb_alloc(allocsize);

	if (ppkt == NULL) {
		return NULL;
	}

	//3 2. read data from rxfifo
	preadbuf = skb_put(ppkt, readsize);
	ret = sdio_read_port(padapter, WLAN_RX_FIFO_DEVICE_ID, readsize, preadbuf);
	if (ret == _FAIL) {
		_rtw_skb_free(ppkt);
		return NULL;
	}

	//3 3. alloc recvbuf
	precvpriv = &padapter->recvpriv;
	precvbuf = rtw_dequeue_recvbuf(&precvpriv->free_recv_buf_queue);
	precvpriv->free_recv_buf_queue_cnt--;
	if (precvbuf == NULL) {
		_rtw_skb_free(ppkt);
		return NULL;
	}

	//3 4. init recvbuf
	precvbuf->pskb = ppkt;
	precvbuf->len = ppkt->len;

	return precvbuf;
}

void sd_int_dpc(PADAPTER padapter)
{
	//struct sk_buff *skb;
	PSDIO_DATA psdio= &padapter->dvobj->intf_data;
	struct recv_buf *precvbuf;
	if (psdio->sdio_hisr & SDIO_HISR_RX_REQUEST)
	{

		psdio->sdio_hisr ^= SDIO_HISR_RX_REQUEST;

		do {
			//Sometimes rx length will be zero. driver need to use cmd53 read again.
			if(psdio->SdioRxFIFOSize == 0)
			{
				u8 data[4];
				_sdio_local_read(padapter, SDIO_RX0_REQ_LEN, 4, data);
				psdio->SdioRxFIFOSize = le16_to_cpu(*(u16*)data);
			}

			if(psdio->SdioRxFIFOSize)
			{
				precvbuf = sd_recv_rxfifo(padapter, psdio->SdioRxFIFOSize);
				psdio->SdioRxFIFOSize = 0;
				if (precvbuf)
				{
					sd_rxhandler(padapter, precvbuf);
				}
				else
					break;
			}
			else
				break;

		} while (1);

	}
}

void sd_int_hal(PADAPTER padapter)
{
	u8 data[6];
	PSDIO_DATA psdio= &padapter->dvobj->intf_data;
	//read directly from SDIO_HISR(32bits) and SDIO_RX0_REQ_LEN(0~23), but 16bits are enough
	_sdio_local_read(padapter, SDIO_HISR, 6, data); 
	psdio->sdio_hisr = le32_to_cpu(*(u32*)data);
	psdio->SdioRxFIFOSize = le16_to_cpu(*(u16*)&data[4]);
	if (psdio->sdio_hisr & psdio->sdio_himr)
	{
		u32 v32;

		psdio->sdio_hisr &= psdio->sdio_himr;

		// clear HISR
		v32 = psdio->sdio_hisr & MASK_SDIO_HISR_CLEAR;
		if (v32) {
			v32 = cpu_to_le32(v32);
			_sdio_local_write(padapter, SDIO_HISR, 4, (u8*)&v32);
		}

		sd_int_dpc(padapter);
		
	} 
	else 
	{
		DBG_871X("%s: HISR(0x%08x) and HIMR(0x%08x) not match!\n",
				__FUNCTION__, psdio->sdio_hisr, psdio->sdio_himr);
	}	
}

void sdio_set_intf_ops(PADAPTER padapter,struct _io_ops *pops)
{
_func_enter_;

	pops->_read8 = &sdio_read8;
	pops->_read16 = &sdio_read16;
	pops->_read32 = &sdio_read32;
	pops->_read_mem = &sdio_read_mem;
	pops->_read_port = &sdio_read_port;

	pops->_write8 = &sdio_write8;
	pops->_write16 = &sdio_write16;
	pops->_write32 = &sdio_write32;
	pops->_writeN = &sdio_writeN;
	pops->_write_mem = &sdio_write_mem;
	pops->_write_port = &sdio_write_port;

	pops->_sd_f0_read8 = sdio_f0_read8;

_func_exit_;
}
