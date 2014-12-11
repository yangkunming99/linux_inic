#include "autoconf.h"
#include "rtw_debug.h"
#include "drv_types.h"
#include "rtw_io.h"
#define _RTW_IO_C_
/*
* Increase and check if the continual_io_error of this @param dvobjprive is larger than MAX_CONTINUAL_IO_ERR
* @return _TRUE:
* @return _FALSE:
*/
int rtw_inc_and_chk_continual_io_error(struct dvobj_priv *dvobj)
{
	int ret = _FALSE;
	int value;
	if( (value=ATOMIC_INC_RETURN(&dvobj->continual_io_error)) > MAX_CONTINUAL_IO_ERR) {
		DBG_871X("[dvobj:%p][ERROR] continual_io_error:%d > %d\n", dvobj, value, MAX_CONTINUAL_IO_ERR);
		ret = _TRUE;
	} else {
		//DBG_871X("[dvobj:%p] continual_io_error:%d\n", dvobj, value);
	}
	return ret;
}

/*
* Set the continual_io_error of this @param dvobjprive to 0
*/
void rtw_reset_continual_io_error(struct dvobj_priv *dvobj)
{
	ATOMIC_SET(&dvobj->continual_io_error, 0);	
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

void rtw_write_port_cancel(_adapter *adapter)
{
	void (*_write_port_cancel)(_adapter *adapter);
	
	_write_port_cancel = adapter->io_ops._write_port_cancel;

	if(_write_port_cancel)
		_write_port_cancel(adapter);
}


u32 rtw_read_port(_adapter *adapter, u32 addr, u32 cnt, u8 *pmem)
{
	u32 (*_read_port)(_adapter *adapter , u32 addr, u32 cnt, u8 *pmem);
	u32 ret = _SUCCESS;

	_func_enter_;

	if( (adapter->bDriverStopped ==_TRUE) || (adapter->bSurpriseRemoved == _TRUE))
	{
	     RT_TRACE(_module_rtl871x_io_c_, _drv_info_, ("rtw_read_port:bDriverStopped(%d) OR bSurpriseRemoved(%d)", adapter->bDriverStopped, adapter->bSurpriseRemoved));	    
	     return;
	}

	_read_port = adapter->io_ops._read_port;

	ret = _read_port(adapter, addr, cnt, pmem);

	_func_exit_;
	return ret;
}
void rtw_read_port_cancel(_adapter *adapter)
{
	void (*_read_port_cancel)(_adapter *adapter);

	_read_port_cancel = adapter->io_ops._read_port_cancel;

	if(_read_port_cancel)
		_read_port_cancel(adapter);
}
