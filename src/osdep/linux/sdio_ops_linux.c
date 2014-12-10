#include "sdio_ops.h"
#include "rtw_debug.h"
//#include "8195_sdio_reg.h"
#include "rtw_io.h"

static bool rtw_sdio_claim_host_needed(struct sdio_func *func)
{
	struct dvobj_priv *dvobj = sdio_get_drvdata(func);
	PSDIO_DATA psdio= &dvobj->intf_data;
	if (psdio->sys_sdio_irq_thd && psdio->sys_sdio_irq_thd == current)
		return _FALSE;
	return _TRUE;
}

inline void rtw_sdio_set_irq_thd(struct dvobj_priv *dvobj, _thread_hdl_ thd_hdl)
{
	PSDIO_DATA sdio_data = &dvobj->intf_data;
	sdio_data->sys_sdio_irq_thd = thd_hdl;
}

/*
 * Return:
 *	0		Success
 *	others	Fail
 */
s32 _sd_cmd52_read(struct sdio_func *func, u32 addr, u32 cnt, u8 *pdata)
{
//	PADAPTER padapter;
//	struct dvobj_priv *psdiodev;
//	PSDIO_DATA psdio;

	int err=0, i;
	struct sdio_func *pfunc;

_func_enter_;

	pfunc = func;
//	printk("%s(): block size is %d\n", __func__, pfunc->cur_blksize);
	for (i = 0; i < cnt; i++) {
		pdata[i] = sdio_readb(pfunc, addr+i, &err);
		if (err) {
			printk("%s(): sdio_readb failed!\n", __func__);
			break;
		}
	}

_func_exit_;

	return err;
}

/*
 * Return:
 *	0		Success
 *	others	Fail
 */
s32 sd_cmd52_read(PADAPTER padapter, u32 addr, u32 cnt, u8 *pdata)
{

	int err=0;
	struct sdio_func *func;
	bool claim_needed;	
	PSDIO_DATA psdio= &padapter->dvobj->intf_data;
_func_enter_;
	
	func = psdio->func;
	claim_needed = rtw_sdio_claim_host_needed(func);

	if (claim_needed)
		sdio_claim_host(func);
	err = _sd_cmd52_read(func, addr, cnt, pdata);
	printk("%s(): err from _sd_cmd52_read is : %d\n", __func__, err);
	if (claim_needed)
		sdio_release_host(func);

_func_exit_;

	return err;
}

/*
 * Return:
 *	0		Success
 *	others	Fail
 */
s32 _sd_cmd52_write(PADAPTER padapter, u32 addr, u32 cnt, u8 *pdata)
{
	
	int err=0, i;
	PSDIO_DATA psdio= &padapter->dvobj->intf_data;
	struct sdio_func *func;

_func_enter_;
	
	func = psdio->func;

	for (i = 0; i < cnt; i++) {
		sdio_writeb(func, pdata[i], addr+i, &err);
		if (err) {
			DBG_871X(KERN_ERR "%s: FAIL!(%d) addr=0x%05x val=0x%02x\n", __func__, err, addr+i, pdata[i]);
			break;
		}
	}

_func_exit_;

	return err;
}

/*
 * Return:
 *	0		Success
 *	others	Fail
 */
s32 sd_cmd52_write(PADAPTER padapter, u32 addr, u32 cnt, u8 *pdata)
{
	int err=0;
	struct sdio_func *func;
	bool claim_needed;
	PSDIO_DATA psdio= &padapter->dvobj->intf_data;
_func_enter_;
	func = psdio->func;
	claim_needed = rtw_sdio_claim_host_needed(func);

	if (claim_needed)
		sdio_claim_host(func);
	err = _sd_cmd52_write(padapter, addr, cnt, pdata);
	if (claim_needed)
		sdio_release_host(func);
_func_exit_;

	return err;
}

u8 _sd_read8(PADAPTER padapter, u32 addr, s32 *err)
{
	u8 v=0;
	struct sdio_func *func;
	PSDIO_DATA psdio= &padapter->dvobj->intf_data;
_func_enter_;
	
	func = psdio->func;

	v = sdio_readb(func, addr, err);

	if (err && *err)
		DBG_871X(KERN_ERR "%s: FAIL!(%d) addr=0x%05x\n", __func__, *err, addr);

_func_exit_;

	return v;
}

u8 sd_read8(PADAPTER padapter, u32 addr, s32 *err)
{
	u8 v=0;
	struct sdio_func *func;
	bool claim_needed;
	PSDIO_DATA psdio= &padapter->dvobj->intf_data;
_func_enter_;

	func = psdio->func;
	claim_needed = rtw_sdio_claim_host_needed(func);

	if (claim_needed)
		sdio_claim_host(func);
	v = sdio_readb(func, addr, err);
	if (claim_needed)
		sdio_release_host(func);
	if (err && *err)
		DBG_871X(KERN_ERR "%s: FAIL!(%d) addr=0x%05x\n", __func__, *err, addr);

_func_exit_;

	return v;
}

u8 sd_f0_read8(PADAPTER padapter,u32 addr, s32 *err)
{

	u8 v=0;
	struct sdio_func *func;
	bool claim_needed;
	PSDIO_DATA psdio= &padapter->dvobj->intf_data;
_func_enter_;

	func = psdio->func;
	claim_needed = rtw_sdio_claim_host_needed(func);

	if (claim_needed)
		sdio_claim_host(func);
	v = sdio_f0_readb(func, addr, err);
	if (claim_needed)
		sdio_release_host(func);
	if (err && *err)
		DBG_871X(KERN_ERR "%s: FAIL!(%d) addr=0x%05x\n", __func__, *err, addr);

_func_exit_;

	return v;
}


u16 sd_read16(PADAPTER padapter, u32 addr, s32 *err)
{
	u16 v=0;
	struct sdio_func *func;
	bool claim_needed;
	PSDIO_DATA psdio= &padapter->dvobj->intf_data;
_func_enter_;
	
	func = psdio->func;
	claim_needed = rtw_sdio_claim_host_needed(func);

	if (claim_needed)
		sdio_claim_host(func);
	v = sdio_readw(func, addr, err);
	if (claim_needed)
		sdio_release_host(func);
	if (err && *err)
		DBG_871X(KERN_ERR "%s: FAIL!(%d) addr=0x%05x\n", __func__, *err, addr);

_func_exit_;

	return  v;
}

u32 sd_read32(PADAPTER padapter, u32 addr, s32 *err)
{	
	u32 v=0;
	struct sdio_func *func;
	bool claim_needed;
	PSDIO_DATA psdio= &padapter->dvobj->intf_data;
_func_enter_;

	func = psdio->func;
	claim_needed = rtw_sdio_claim_host_needed(func);

	if (claim_needed)
		sdio_claim_host(func);
	v = sdio_readl(func, addr, err);
	if (claim_needed)
		sdio_release_host(func);

	if (err && *err)
	{
		int i;

		DBG_871X(KERN_ERR "%s: (%d) addr=0x%05x, val=0x%x\n", __func__, *err, addr, v);

		*err = 0;
		for(i=0; i<SD_IO_TRY_CNT; i++)
		{
			if (claim_needed) sdio_claim_host(func);
			v = sdio_readl(func, addr, err);
			if (claim_needed) sdio_release_host(func);
			
			if (*err == 0){
				rtw_reset_continual_io_error(padapter->dvobj);
				break;
			}else{				
				DBG_871X(KERN_ERR "%s: (%d) addr=0x%05x, val=0x%x, try_cnt=%d\n", __func__, *err, addr, v, i);
				if(( -ESHUTDOWN == *err ) || ( -ENODEV == *err)){			
					//padapter->bSurpriseRemoved = _TRUE;
				}

				if(rtw_inc_and_chk_continual_io_error(padapter->dvobj) == _TRUE ){
					//padapter->bSurpriseRemoved = _TRUE;
					break;
				}
			}
		}

		if (i==SD_IO_TRY_CNT)
			DBG_871X(KERN_ERR "%s: FAIL!(%d) addr=0x%05x, val=0x%x, try_cnt=%d\n", __func__, *err, addr, v, i);
		else
			DBG_871X(KERN_ERR "%s: (%d) addr=0x%05x, val=0x%x, try_cnt=%d\n", __func__, *err, addr, v, i);

	}

_func_exit_;

	return  v;
}

void sd_write8(PADAPTER padapter, u32 addr, u8 v, s32 *err)
{	
	struct sdio_func *func;
	bool claim_needed;
	PSDIO_DATA psdio= &padapter->dvobj->intf_data;
_func_enter_;
	
	func = psdio->func;
	claim_needed = rtw_sdio_claim_host_needed(func);

	if (claim_needed)
		sdio_claim_host(func);
	sdio_writeb(func, v, addr, err);
	if (claim_needed)
		sdio_release_host(func);
	if (err && *err)
		DBG_871X(KERN_ERR "%s: FAIL!(%d) addr=0x%05x val=0x%02x\n", __func__, *err, addr, v);

_func_exit_;
}

void sd_write16(PADAPTER padapter, u32 addr, u16 v, s32 *err)
{	
	struct sdio_func *func;
	bool claim_needed;
	PSDIO_DATA psdio= &padapter->dvobj->intf_data;
_func_enter_;
	
	func = psdio->func;
	claim_needed = rtw_sdio_claim_host_needed(func);

	if (claim_needed)
		sdio_claim_host(func);
	sdio_writew(func, v, addr, err);
	if (claim_needed)
		sdio_release_host(func);
	if (err && *err)
		DBG_871X(KERN_ERR "%s: FAIL!(%d) addr=0x%05x val=0x%04x\n", __func__, *err, addr, v);

_func_exit_;
}

void sd_write32(PADAPTER padapter, u32 addr, u32 v, s32 *err)
{
	struct sdio_func *func;
	bool claim_needed;
	PSDIO_DATA psdio= &padapter->dvobj->intf_data;
_func_enter_;
	
	func = psdio->func;
	claim_needed = rtw_sdio_claim_host_needed(func);

	if (claim_needed)
		sdio_claim_host(func);
	sdio_writel(func, v, addr, err);
	if (claim_needed)
		sdio_release_host(func);

	if (err && *err)
	{
		int i;

		DBG_871X(KERN_ERR "%s: (%d) addr=0x%05x val=0x%08x\n", __func__, *err, addr, v);

		*err = 0;
		for(i=0; i<SD_IO_TRY_CNT; i++)
		{
			if (claim_needed) sdio_claim_host(func);
			sdio_writel(func, v, addr, err);
			if (claim_needed) sdio_release_host(func);
			if (*err == 0){
				rtw_reset_continual_io_error(padapter->dvobj);
				break;
			}else{				
				DBG_871X(KERN_ERR "%s: (%d) addr=0x%05x, val=0x%x, try_cnt=%d\n", __func__, *err, addr, v, i);
				if(( -ESHUTDOWN == *err ) || ( -ENODEV == *err)){			
					//padapter->bSurpriseRemoved = _TRUE;
				}

				if(rtw_inc_and_chk_continual_io_error(padapter->dvobj) == _TRUE ){
					//padapter->bSurpriseRemoved = _TRUE;
					break;
				}
			}
		}

		if (i==SD_IO_TRY_CNT)
			DBG_871X(KERN_ERR "%s: FAIL!(%d) addr=0x%05x val=0x%08x, try_cnt=%d\n", __func__, *err, addr, v, i);
		else
			DBG_871X(KERN_ERR "%s: (%d) addr=0x%05x val=0x%08x, try_cnt=%d\n", __func__, *err, addr, v, i);
	}

_func_exit_;
}


/*
 * Use CMD53 to read data from SDIO device.
 * This function MUST be called after sdio_claim_host() or
 * in SDIO ISR(host had been claimed).
 *
 * Parameters:
 *	psdio	pointer of SDIO_DATA
 *	addr	address to read
 *	cnt		amount to read
 *	pdata	pointer to put data, this should be a "DMA:able scratch buffer"!
 *
 * Return:
 *	0		Success
 *	others	Fail
 */
s32 _sd_read(struct sdio_func *func, u32 addr, u32 cnt, void *pdata)
{
	
	int err= -EPERM;
	struct sdio_func *pfunc;
	
_func_enter_;
		
	pfunc = func;

	if (unlikely((cnt==1) || (cnt==2)))
	{
		int i;
		u8 *pbuf = (u8*)pdata;

		for (i = 0; i < cnt; i++)
		{
			*(pbuf+i) = sdio_readb(pfunc, addr+i, &err);

			if (err) {
				printk("%s: FAIL!(%d) addr=0x%05x\n", __func__, err, addr);
				break;
			}
		}
		return err;
	}

	err = sdio_memcpy_fromio(pfunc, pdata, addr, cnt);
	if (err) {
		printk("%s: FAIL(%d)! ADDR=%#x Size=%d\n", __func__, err, addr, cnt);
	}

_func_exit_;

	return err;
}

/*
 * Use CMD53 to read data from SDIO device.
 *
 * Parameters:
 *	psdio	pointer of SDIO_DATA
 *	addr	address to read
 *	cnt		amount to read
 *	pdata	pointer to put data, this should be a "DMA:able scratch buffer"!
 *
 * Return:
 *	0		Success
 *	others	Fail
 */
s32 sd_read(PADAPTER padapter, u32 addr, u32 cnt, void *pdata)
{	
	struct sdio_func *func;
	bool claim_needed;
	s32 err= -EPERM;
	PSDIO_DATA psdio= &padapter->dvobj->intf_data;
_func_enter_;
	func = psdio->func;
	claim_needed = rtw_sdio_claim_host_needed(func);

	if (claim_needed)
		sdio_claim_host(func);
	err = _sd_read(func, addr, cnt, pdata);
	if (claim_needed)
		sdio_release_host(func);
_func_exit_;
	return err;
}


/*
 * Use CMD53 to write data to SDIO device.
 * This function MUST be called after sdio_claim_host() or
 * in SDIO ISR(host had been claimed).
 *
 * Parameters:
 *	psdio	pointer of SDIO_DATA
 *	addr	address to write
 *	cnt		amount to write
 *	pdata	data pointer, this should be a "DMA:able scratch buffer"!
 *
 * Return:
 *	0		Success
 *	others	Fail
 */
s32 _sd_write(struct sdio_func *func, u32 addr, u32 cnt, void *pdata)
{
	
	struct sdio_func *pfunc;
	u32 size;
	s32 err=-EPERM;

_func_enter_;
	
	pfunc = func;
//	size = sdio_align_size(func, cnt);

	if (unlikely((cnt==1) || (cnt==2)))
	{
		int i;
		u8 *pbuf = (u8*)pdata;

		for (i = 0; i < cnt; i++)
		{
			sdio_writeb(pfunc, *(pbuf+i), addr+i, &err);
			if (err) {
				printk("%s: FAIL!(%d) addr=0x%05x val=0x%02x\n", __func__, err, addr, *(pbuf+i));
				break;
			}
		}

		return err;
	}

	size = cnt;
//	printk("%s(): write to addr 0x%x\n", __func__, addr);
//	printk("%s(): write size %d\n", __func__, size);
	err = sdio_memcpy_toio(pfunc, addr, pdata, size);
	if (err) {
		printk("%s: FAIL(%d)! ADDR=%#x Size=%d(%d)\n", __func__, err, addr, cnt, size);
	}

_func_exit_;

	return err;
}

/*
 * Use CMD53 to write data to SDIO device.
 *
 * Parameters:
 *  psdio	pointer of SDIO_DATA
 *  addr	address to write
 *  cnt		amount to write
 *  pdata	data pointer, this should be a "DMA:able scratch buffer"!
 *
 * Return:
 *  0		Success
 *  others	Fail
 */
s32 sd_write(PADAPTER padapter, u32 addr, u32 cnt, void *pdata)
{
	struct sdio_func *func;
	bool claim_needed;
	s32 err=-EPERM;
	PSDIO_DATA psdio= &padapter->dvobj->intf_data;
_func_enter_;

	func = psdio->func;
	claim_needed = rtw_sdio_claim_host_needed(func);

	if (claim_needed)
		sdio_claim_host(func);
	err = _sd_write(func, addr, cnt, pdata);
	if (claim_needed)
		sdio_release_host(func);
_func_exit_;
	return err;
}

