#include "../include/autoconf.h"
#include "../include/rtw_debug.h"
#include "../include/drv_types.h"
#include "../include/rtw_xmit.h"
#include "../include/rtw_ioctl.h"
#include "../include/8195_desc.h"
#include "../include/8195_sdio_reg.h"
#include "../include/sdio_ops.h"
#include "../include/rtw_io.h"
#include "../include/osdep_service.h"

/*
* Increase and check if the continual_io_error of this @param dvobjprive is larger than MAX_CONTINUAL_IO_ERR
* @return _TRUE:
* @return _FALSE:
*/
int rtw_inc_and_chk_continual_io_error(PADAPTER padapter)
{
	int ret = _FALSE;
	int value;
	if( (value=ATOMIC_INC_RETURN(&padapter->continual_io_error)) > MAX_CONTINUAL_IO_ERR) {
		DBG_871X("[padapter:%p][ERROR] continual_io_error:%d > %d\n", padapter, value, MAX_CONTINUAL_IO_ERR);
		ret = _TRUE;
	} else {
		//DBG_871X("[dvobj:%p] continual_io_error:%d\n", dvobj, value);
	}
	return ret;
}

/*
* Set the continual_io_error of this @param dvobjprive to 0
*/
void rtw_reset_continual_io_error(PADAPTER padapter)
{
	ATOMIC_SET(&padapter->continual_io_error, 0);	
}

u32 rtw_write_port(PADAPTER padapter, u32 addr, u32 cnt, u8 *pmem)
{
	u32 (*_write_port)(PADAPTER padapter, u32 addr, u32 cnt, u8 *pmem);

	u32 ret = _SUCCESS;

	_func_enter_;

	_write_port = padapter->io_ops._write_port;
	
	ret = _write_port(padapter, addr, cnt, pmem);

	 _func_exit_;

	return ret;
}